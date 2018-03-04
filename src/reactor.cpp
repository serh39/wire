/*
 * Copyright © 2018 Maks Mazurov (fox.cpp) <foxcpp [at] yandex [dot] ru>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include "libwire/reactor.hpp"

#include "libwire/internal/socket_utils.hpp"

#ifdef __linux__
#   include <sys/epoll.h>
#endif

#ifdef _WIN32
#   include <ws2tcpip.h>
#   define EAGAIN WSAEWOULDBLOCK
#   define EWOULDBLOCK WSAEWOULDBLOCK
#endif

namespace libwire {
    using namespace internal_;
    using namespace std::literals;

    /*
     * Store last used socket's queue to skip "slow" hash map lookup in case of
     * repeated access.
     *
     * Some simple cases can benefit from this caching:
     *
     * void handler(size_t, std::error_code ec) {
     *     if (ec) {
     *         stop = true;
     *         return;
     *     }
     *     // No hash map lookup: We touched sock's queue recently (in poll_once).
     *     reactor_.enqueue(sock.implementation(), {
     *         operation_type::read,
     *         out_buffer, op_size, 0, handler
     *     });
     * };
     *
     * // Somewhere else:
     *
     * // Hash map insert.
     * reactor_.add_socket(sock.implementation());
     *
     * // No hash map lookup: We touched sock's queue recently.
     * reactor_.enqueue(sock.implementation(), {
     *    operation_type::read,
     *    out_buffer, op_size, 0, handler
     * });
     *
     *
     * *** Caveats ***
     *
     * 1. Cached pointer **must** be invalidated when socket removed from selector.
     * 2. Cache must be per-thread to avoid frequent misses and other
     * threading issues (solved by thread-local storage).
     * 3. Cache must be per-reactor to avoid accessing wrong reactor's data.
     * (solved by last_reactor_ptr flag)
     */
    thread_local struct queue_ptr_cache {
        internal_::socket_data* data = nullptr;

        socket::native_handle_t socket_handle = socket::not_initialized;
        reactor* reactor_ptr = nullptr;
    } operations_queue_cache;

    void reactor::add_socket(internal_::socket& sock) {
        socket_data& data = selector.register_socket(sock, event_code::readable);

        operations_queue_cache.data = &data;
        operations_queue_cache.socket_handle = sock.handle;
        operations_queue_cache.reactor_ptr = this;
    }

    void reactor::remove_socket(internal_::socket& sock) {
        if (operations_queue_cache.socket_handle == sock.handle &&
            operations_queue_cache.reactor_ptr == this) {

            operations_queue_cache.data = nullptr;
            operations_queue_cache.socket_handle = socket::not_initialized;
            operations_queue_cache.reactor_ptr = nullptr;
        }

        // XXX: Correctly handle case where socket get removed from active
        // reactor.
        selector.remove_socket(sock.handle);
    }

    void reactor::enqueue(internal_::socket& sock, internal_::operation operation) {
        internal_::socket_data& socket_data =
            operations_queue_cache.socket_handle == sock.handle &&
            operations_queue_cache.reactor_ptr == this
            ? *operations_queue_cache.data
            : selector.user_data(sock.handle);

        socket_data.pending_operations.push(operation);
    }

    void reactor::cancel_oldest_operation(internal_::socket& sock) {
    }

    void reactor::cancel_all_operations(internal_::socket& sock) {

    }

    void reactor::run_once() {
        // Handle cases where there is no sockets or sockets without operations.
        bool have_work = false;
        for (auto& [_, data] : selector.sockets) {
            if (!data.pending_operations.empty()) {
                have_work = true;
            }
        }
        if (!have_work) return;

        std::array<event_t, 16> events_buffer;
        memory_view<event_t> events(events_buffer.data(), events_buffer.size());
        selector.poll(events, 1h);

        for (const auto& event : events) {
            auto codes = selector.event_codes(event);
            auto& operations = selector.user_data(event).pending_operations;
            auto& socket = selector.user_data(event).socket;

            if (operations.empty()) continue;

            operations_queue_cache.data = &selector.user_data(event);
            operations_queue_cache.socket_handle = socket.handle;
            operations_queue_cache.reactor_ptr = this;

            if (codes.get(event_code::error)) {
                std::error_code ec = {last_async_socket_error(socket.handle), error::system_category()};
                // Pass error to all completion handlers.
                // (most of I/O errors we can get here is unrecoverable)
                while (!operations.empty()) {
                    operations.front().handler(0, ec);
                    operations.pop();
                }
                return;
            }

            if (codes.get(event_code::readable)) {
                process_reads(socket, operations);
            } else if (codes.get(event_code::writable)) {
                process_writes(socket, operations);
            }

            if (!operations.empty()) {
                if (operations.front().opcode == operation_type::write) {
                    selector.change_event_mask(socket.handle, event_code::writable);
                }
                if (operations.front().opcode == operation_type::read) {
                    selector.change_event_mask(socket.handle, event_code::readable);
                }
            }
        }
    }

    void reactor::process_reads(internal_::socket& socket, std::queue<internal_::operation>& operations) {
        // TODO: Rewrite to use vectored I/O.
        // (is it possible to implement it in efficient way or we should
        // stick to "keep it simple" principe?)
        do {
            auto& operation = operations.front();

            if (operations.front().opcode != operation_type::read) {
                break;
            }

            std::error_code ec;
            size_t wanted_to_read = operation.buffer_size - operation.already_processed;
            size_t got_bytes = socket.nonblocking_read((char*) operation.buffer + operation.already_processed,
                                                       operation.buffer_size - operation.already_processed, ec);
            operation.already_processed += got_bytes;
            // Note direct comparision of error_code to EAGAIN.
            // Virtual calls to std::error_category::equivalent is
            // costly because this place is really hot.
            if (ec.value() == EAGAIN || ec.value() == EWOULDBLOCK || got_bytes < wanted_to_read) {
                // We got 0 bytes (Operation would block) or we got less than requested
                // (next operation obviously will block so there is no reason to try more).
                break;
            }
            if (operation.already_processed == operation.buffer_size || ec) {
                operation.handler(operation.already_processed, ec);
                operations.pop();
            }
        } while (!operations.empty());
    }

    void reactor::process_writes(internal_::socket& socket, std::queue<internal_::operation>& operations) {
        do {
            auto& operation = operations.front();

            if (operations.front().opcode != operation_type::write) {
                break;
            }

            std::error_code ec;
            size_t wanted_to_write = operation.buffer_size - operation.already_processed;
            size_t sent_bytes = socket.nonblocking_write((char*) operation.buffer + operation.already_processed,
                                                       operation.buffer_size - operation.already_processed, ec);

            operation.already_processed += sent_bytes;
            // Note direct comparision of error_code to EAGAIN.
            // Virtual calls to std::error_category::equivalent is
            // costly because this place is really hot.
            if (ec.value() == EAGAIN || ec.value() == EWOULDBLOCK || sent_bytes < wanted_to_write) {
                // We got 0 bytes (Operation would block) or we got less than requested
                // (next operation obviously will block so there is no reason to try more).
                break;
            }
            if (operation.already_processed == operation.buffer_size || ec) {
                operation.handler(operation.already_processed, ec);
                operations.pop();
            }
        } while (!operations.empty());
    }
} // namespace libwire

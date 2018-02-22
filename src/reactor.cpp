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
        std::queue<operation>* last_queue = nullptr;

        socket::native_handle_t socket_handle = socket::not_initialized;
        reactor* reactor_ptr = nullptr;
    } operations_queue_cache;

    void reactor::add_socket(internal_::socket& sock) {
        socket_data& data = selector.register_socket(sock, event_code::readable | event_code::error | event_code::eof);

        operations_queue_cache.last_queue = &data.pending_operations;
        operations_queue_cache.socket_handle = sock.handle;
        operations_queue_cache.reactor_ptr = this;
    }

    void reactor::remove_socket(internal_::socket& sock) {
        if (operations_queue_cache.socket_handle == sock.handle &&
            operations_queue_cache.reactor_ptr == this) {

            operations_queue_cache.last_queue = nullptr;
            operations_queue_cache.socket_handle = socket::not_initialized;
            operations_queue_cache.reactor_ptr = nullptr;
        }

        // XXX: Correctly handle case where socket get removed from active
        // reactor.
        selector.remove_socket(sock.handle);
    }

    void reactor::enqueue(internal_::socket& sock, internal_::operation operation) {
        if (operations_queue_cache.socket_handle == sock.handle &&
            operations_queue_cache.reactor_ptr == this) {

            operations_queue_cache.last_queue->push(operation);
        } else {
            auto& queue = selector.user_data(sock.handle).pending_operations;

            queue.push(operation);

            operations_queue_cache.last_queue = &queue;
            operations_queue_cache.socket_handle = sock.handle;
            operations_queue_cache.reactor_ptr = this;
        }
    }

    void reactor::cancel_oldest_operation(internal_::socket& sock) {
    }

    void reactor::cancel_all_operations(internal_::socket& sock) {

    }

    void reactor::run_once() {
        std::array<event_t, 16> events_buffer;
        memory_view<event_t> events(events_buffer.data(), events_buffer.size());
        selector.poll(events, 1h);

        for (const auto& event : events) {
            auto codes = selector.event_codes(event);
            auto& operations = selector.user_data(event).pending_operations;
            auto& socket = selector.user_data(event).socket;

            operations_queue_cache.last_queue = &operations;
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

            if (operations.empty()) continue;

            if (codes.get(event_code::readable)) {
                // TODO: Rewrite to use vectored I/O.
                // (is it possible to implement it in efficient way or we should
                // stick to "keep it simple" principe?)
                do {
                    auto& operation = operations.front();

                    if (operation.opcode != operation_type::read) break;

                    std::error_code ec;
                    size_t wanted_to_read = operation.buffer_size - operation.already_read;
                    size_t got_bytes = socket.nonblocking_read((char*) operation.buffer + operation.already_read,
                                                               operation.buffer_size - operation.already_read, ec);
                    operation.already_read += got_bytes;
                    // Note direct comparision of error_code to EAGAIN.
                    // Virtual calls to std::error_category::equivalent is
                    // costly because this place is really hot.
                    if (ec.value() == EAGAIN || ec.value() == EWOULDBLOCK || got_bytes < wanted_to_read) {
                        // We got 0 bytes (Operation would block) or we got less than requested
                        // (next operation obviously will block so there is no reason to try more).
                        break;
                    }
                    if (operation.already_read == operation.buffer_size || ec) {
                        operation.handler(operation.already_read, ec);
                        operations.pop();
                    }
                } while (!operations.empty());
            } else if (codes.get(event_code::writable)) {
                // TODO: We don't care for now.
            }
        }
    }
} // namespace libwire

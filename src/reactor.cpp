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

namespace libwire {
    using namespace internal_;
    using namespace std::literals;

    void reactor::add_socket(internal_::socket& sock) {
        selector.register_socket(sock, event_code::readable | event_code::error | event_code::eof);
    }

    void reactor::enqueue(internal_::socket& sock, internal_::operation operation) {
        selector.user_data(sock.handle).pending_operations.push(operation);
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
                do {
                    auto& operation = operations.front();

                    std::error_code ec;
                    size_t wanted_to_read = operation.buffer_size - operation.already_read;
                    size_t got_bytes = socket.nonblocking_read((char*) operation.buffer + operation.already_read,
                                                               operation.buffer_size - operation.already_read, ec);
                    operation.already_read += got_bytes;
                    // We got 0 bytes (Operation would block) or we got less than requested
                    // (next operation obviously will block so there is no reason to try more).
                    if (ec == error::try_again || got_bytes < wanted_to_read) {
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

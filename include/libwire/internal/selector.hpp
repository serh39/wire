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

#pragma once

#include <cstddef>
#include <functional>
#include <system_error>
#include <queue>
#include <chrono>
#include <libwire/internal/socket.hpp>
#include <libwire/flags.hpp>

/**
 * This file defines interface used by high-level event loop code to access
 * platform-dependent I/O selector.
 */

#ifdef __linux__
struct epoll_event;
#endif

namespace libwire::internal_ {
#ifdef __linux__
    using event_t = epoll_event;
#endif

    enum class operation_type {
        read = 1<<1,
    };

    enum class event_code {
        // Values picked to match corresponding EPOLL* codes.
        readable = 1,
        // exceptional condition = 2
        writable = 4,
        error = 8,
        eof = 16,

    };

    struct operation {
        operation_type opcode;
        void* buffer;
        size_t buffer_size;
        size_t already_read;
        std::function<void(size_t, std::error_code)> handler;
    };

    /**
     * Structure that will be stored together with socket in selector.
     *
     * Selector on some platforms can optimize access to user-defined data
     * for socket. For example. pointer to data will be returned with
     * each event when using epoll.
     */
    struct socket_data {
        socket& socket;
        std::queue<operation> pending_operations;
    };

    class selector {
    public:
        selector() noexcept;
        ~selector() noexcept;

        selector(const selector&) = delete;
        selector(selector&&) = default;

        selector& operator=(const selector&) = delete;
        selector& operator=(selector&&) = default;

        socket_data& register_socket(socket& socket, flags<event_code> interested_events) noexcept;
        void change_event_mask(socket::native_handle_t handle, flags<event_code> interested_events) noexcept;
        void remove_socket(socket::native_handle_t handle) noexcept;

        void poll(memory_view<event_t>& event_buffer, std::chrono::milliseconds timeout) noexcept;

        flags<event_code> event_codes(const event_t&) const noexcept;
        socket_data& user_data(const event_t&) const noexcept;
        socket_data& user_data(socket::native_handle_t handle) const noexcept;


#ifdef __linux__
        int epoll_fd = -1;
#endif

        // Changes to attached "data" will not affect selector state.
        mutable std::unordered_map<socket::native_handle_t, socket_data> sockets;
    };

    LIBWIRE_DECLARE_FLAGS_OPERATORS(event_code, int);
} // namespace libwire::internal_

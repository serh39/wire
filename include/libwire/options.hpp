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

#include <chrono>
#include <libwire/tcp/socket.hpp>

namespace libwire {
    /**
     * Dummy type for \ref nonblocking option.
     */
    struct non_blocking_t {
        template<typename Socket>
        static bool get(const Socket& sock) noexcept {
            return get_impl(sock.implementation());
        }

        template<typename Socket>
        static void set(Socket& sock, bool value) noexcept {
            set_impl(sock.implementation(), value);
        }

    private:
        static bool get_impl(const internal_::socket&) noexcept;
        static void set_impl(internal_::socket&, bool) noexcept;
    };

    /**
     * Toggle non-blocking I/O mode on sockets.
     *
     * If this option enabled then read() and write() will
     * fail with error::would_block if operation can't be completed
     * immediately.
     *
     * If this option is disabled then read() can block if there is no
     * received data in system buffer. write() can block too if there is no
     * space left in buffer.
     */
    constexpr non_blocking_t non_blocking{};

    struct receive_timeout_t {
        template<typename Socket, typename Duration>
        static void set(Socket& socket, Duration d) noexcept {
            namespace ch = std::chrono;

            set_impl(socket.implementation(), ch::duration_cast<ch::milliseconds>(d));
        }

        template<typename Socket>
        static std::chrono::milliseconds get(const Socket& socket) noexcept {
            return get_impl(socket.implementation());
        }

    private:
        static std::chrono::milliseconds get_impl(const internal_::socket&) noexcept;
        static void set_impl(internal_::socket&, std::chrono::milliseconds) noexcept;
    };

    /**
     * Specify timeout for blocking read operations.
     *
     * Doesn't affects asynchronous and non-blocking operations.
     *
     * \warning Socket may be left in inconsistent state after timeout, it's
     * unsafe to use it and your only choice is close().
     */
    constexpr receive_timeout_t receive_timeout{};

    struct send_timeout_t {
        template<typename Socket, typename Duration>
        static void set(Socket& socket, Duration d) noexcept {
            namespace ch = std::chrono;

            set_impl(socket.implementation(), ch::duration_cast<ch::milliseconds>(d));
        }

        template<typename Socket>
        static std::chrono::milliseconds get(const Socket& socket) noexcept {
            return get_impl(socket.implementation());
        }

    private:
        static std::chrono::milliseconds get_impl(const internal_::socket&) noexcept;
        static void set_impl(internal_::socket&, std::chrono::milliseconds) noexcept;
    };

    /**
     * Specify timeout for blocking write operations.
     *
     * Doesn't affects asynchronous and non-blocking operations.
     *
     * \warning Socket may be left in inconsistent state after timeout, it's
     * unsafe to use it and your only choice is close().
     */
    constexpr send_timeout_t send_timeout{};

    struct send_buffer_size_t {
        template<typename Socket>
        static void set(Socket& socket, size_t size) noexcept {
            set_impl(socket.implementation(), size);
        }

        template<typename Socket>
        static size_t get(const Socket& socket) noexcept {
            return get_impl(socket.implementation());
        }

    private:
        static size_t get_impl(const internal_::socket&) noexcept;
        static void set_impl(internal_::socket&, size_t) noexcept;
    };

    constexpr send_buffer_size_t send_buffer_size{};

    struct receive_buffer_size_t {
        template<typename Socket>
        static void set(Socket& socket, size_t size) noexcept {
            set_impl(socket.implementation(), size);
        }

        template<typename Socket>
        static size_t get(const Socket& socket) noexcept {
            return get_impl(socket.implementation());
        }

    private:
        static size_t get_impl(const internal_::socket&) noexcept;
        static void set_impl(internal_::socket&, size_t) noexcept;
    };

    constexpr receive_buffer_size_t receive_buffer_size{};
} // namespace libwire

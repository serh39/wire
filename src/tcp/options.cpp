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
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include "libwire/tcp/options.hpp"
#include "libwire/tcp/socket.hpp"
#include <cassert>

#ifdef _WIN32
#    include <ws2tcpip.h>
#else
#    include <sys/socket.h>
#    include <netinet/in.h>
#    include <netinet/tcp.h>
#endif

/**
 * This file implements options specified in options.hpp header
 * for POSIX-ish platforms such as Linux and BSD.
 */

// TODO (PR's welcomed): There is a lot of code duplication, find a way to clean
// it.

using namespace std::literals::chrono_literals;

namespace libwire::tcp {
    inline namespace options {
        void keep_alive_t::set(socket& sock, bool enabled) noexcept {
            assert(sock.native_handle() != internal_::socket::not_initialized);

            setsockopt(sock.native_handle(), IPPROTO_TCP, TCP_NODELAY, (const char*)&enabled, sizeof(bool));
        }

        bool keep_alive_t::get(const socket& sock) noexcept {
            assert(sock.native_handle() != internal_::socket::not_initialized);

            int result;
            socklen_t result_size = sizeof(result);
            getsockopt(sock.native_handle(), SOL_SOCKET, SO_KEEPALIVE, (char*)&result, &result_size);
            assert(result_size == sizeof(result));
            return bool(result);
        }

        void linger_t::set_impl(socket& sock, bool enabled, std::chrono::seconds timeout) noexcept {
            assert(sock.native_handle() != internal_::socket::not_initialized);

            struct linger linger_opt {};
            linger_opt.l_onoff = int(enabled);
            linger_opt.l_linger = int(timeout.count());
            setsockopt(sock.native_handle(), SOL_SOCKET, SO_LINGER, (const char*)&linger_opt, sizeof(linger_opt));
        }

        std::tuple<bool, std::chrono::seconds> linger_t::get(const socket& sock) noexcept {
            assert(sock.native_handle() != internal_::socket::not_initialized);

            struct linger linger_opt {};
            socklen_t opt_size = sizeof(linger_opt);
            getsockopt(sock.native_handle(), SOL_SOCKET, SO_LINGER, (char*)&linger_opt, &opt_size);
            assert(opt_size == sizeof(linger_opt));
            return {bool(linger_opt.l_onoff), std::chrono::seconds(linger_opt.l_linger)};
        }

        void retransmission_timeout_t::set_impl([[maybe_unused]] socket& sock,
                                                [[maybe_unused]] std::chrono::milliseconds timeout) noexcept {
            assert(sock.native_handle() != internal_::socket::not_initialized);

#ifdef TCP_USER_TIMEOUT
            auto timeout_count = unsigned(timeout.count());
            setsockopt(sock.native_handle(), IPPROTO_TCP, TCP_USER_TIMEOUT, &timeout_count, sizeof(timeout_count));
#else
#endif
        }

        std::chrono::milliseconds retransmission_timeout_t::get([[maybe_unused]] const socket& sock) noexcept {
            assert(sock.native_handle() != internal_::socket::not_initialized);

#ifdef TCP_USER_TIMEOUT
            unsigned result;
            socklen_t result_size = sizeof(result);
            getsockopt(sock.native_handle(), IPPROTO_TCP, TCP_USER_TIMEOUT, &result, &result_size);
            assert(result_size == sizeof(result));
            return std::chrono::milliseconds(result);
#else
            return std::chrono::duration_cast<std::chrono::milliseconds>(2h);
#endif
        }

        void no_delay_t::set(socket& sock, bool enabled) noexcept {
            assert(sock.native_handle() != internal_::socket::not_initialized);

            setsockopt(sock.native_handle(), IPPROTO_TCP, TCP_NODELAY, (const char*)&enabled, sizeof(bool));
        }

        bool no_delay_t::get(const socket& sock) noexcept {
            assert(sock.native_handle() != internal_::socket::not_initialized);

            int result;
            socklen_t result_size = sizeof(result);
            getsockopt(sock.native_handle(), IPPROTO_TCP, TCP_NODELAY, (char*)&result, &result_size);
            assert(result_size == sizeof(result));
            return bool(result);
        }
    } // namespace options
} // namespace libwire::tcp

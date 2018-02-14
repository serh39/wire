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

#include "libwire/options.hpp"

#include <cassert>

#ifdef _WIN32
#    include <ws2tcpip.h>
#else
#    include <sys/socket.h>
#    include <fcntl.h>
#endif

namespace libwire {
    bool non_blocking_t::get_impl(const internal_::socket& socket) noexcept {
        return socket.state.user_non_blocking;
    }

    void non_blocking_t::set_impl(internal_::socket& socket, bool enable) noexcept {
        assert(socket);

#ifdef _WIN32
        unsigned long mode = unsigned(enable);
        int status = ioctlsocket(socket.handle, FIONBIO, &mode);
        if (status == 0) {
            socket.state.user_non_blocking = enable;
            socket.state.internal_non_blocking = enable;
        }
#else
        int flags = fcntl(socket.handle, F_GETFL, 0);
        flags = enable ? (flags | O_NONBLOCK) : (flags & ~O_NONBLOCK);
        int status = fcntl(socket.handle, F_SETFL, flags);
        if (status != -1) {
            socket.state.user_non_blocking = enable;
            socket.state.internal_non_blocking = enable;
        }
#endif
    }

    std::chrono::milliseconds receive_timeout_t::get_impl(const internal_::socket& socket) noexcept {
        assert(socket);

        timeval timeval{};
        socklen_t size = sizeof(timeval);
        getsockopt(socket.handle, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeval, &size);
        assert(size == sizeof(timeval));
        return std::chrono::milliseconds((timeval.tv_sec * 1000) + (timeval.tv_usec / 1000));
    }

    std::chrono::milliseconds send_timeout_t::get_impl(const internal_::socket& socket) noexcept {
        assert(socket);

        timeval timeval{};
        socklen_t size = sizeof(timeval);
        getsockopt(socket.handle, SOL_SOCKET, SO_SNDTIMEO, (char*)&timeval, &size);
        assert(size == sizeof(timeval));
        return std::chrono::milliseconds((timeval.tv_sec * 1000) + (timeval.tv_usec / 1000));
    }

    void send_timeout_t::set_impl(internal_::socket& socket, std::chrono::milliseconds ms) noexcept {
        assert(socket);

        timeval timeval{};
        timeval.tv_usec = ms.count() * 1000;
        setsockopt(socket.handle, SOL_SOCKET, SO_SNDTIMEO, (const char*)&timeval, sizeof(timeval));
    }

    void receive_timeout_t::set_impl(internal_::socket& socket, std::chrono::milliseconds ms) noexcept {
        assert(socket);

        timeval timeval{};
        timeval.tv_usec = ms.count() * 1000;
        setsockopt(socket.handle, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeval, sizeof(timeval));
    }

    size_t send_buffer_size_t::get_impl(const internal_::socket& socket) noexcept {
        assert(socket);

        size_t result = 0;
        socklen_t result_size = sizeof(result);
        getsockopt(socket.handle, SOL_SOCKET, SO_RCVBUF, (char*)&result, &result_size);
        assert(result_size == sizeof(result));
        return result;
    }

    size_t receive_buffer_size_t::get_impl(const internal_::socket& socket) noexcept {
        assert(socket);

        size_t result = 0;
        socklen_t result_size = sizeof(result);
        getsockopt(socket.handle, SOL_SOCKET, SO_SNDBUF, (char*)&result, &result_size);
        assert(result_size == sizeof(result));
        return result;
    }

    void receive_buffer_size_t::set_impl(internal_::socket& socket, size_t size) noexcept {
        assert(socket);

        setsockopt(socket.handle, SOL_SOCKET, SO_RCVBUF, (const char*)&size, sizeof(size));
    }

    void send_buffer_size_t::set_impl(internal_::socket& socket, size_t size) noexcept {
        assert(socket);

        setsockopt(socket.handle, SOL_SOCKET, SO_SNDBUF, (const char*)&size, sizeof(size));
    }
} // namespace libwire

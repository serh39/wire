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

/**
 * This files several useful functions that used in socket wrapper.
 *
 * It must not be included from public headers because it defines
 * macros without prefix.
 */

#include <libwire/internal/socket.hpp>
#include <libwire/internal/error/system_category.hpp>

#ifdef _WIN32
#    include <winsock2.h>
#else
#    include <sys/socket.h>
#endif

#if !defined(EINTR) && defined(_WIN32)
#    define EINTR WASEINTR
#endif

namespace libwire::internal_ {
    std::tuple<address, uint16_t> sockaddr_to_endpoint(sockaddr in);
    sockaddr endpoint_to_sockaddr(std::tuple<address, uint16_t> in);

    int last_socket_error();

    template<typename Call, typename... Args>
    auto error_wrapper(std::error_code& ec, Call call, Args&&... args) {
        decltype(call(args...)) status = 0;
        int last_error = 0;

        do {
            status = call(std::forward<Args>(args)...);

            if (status < 0) {
                last_error = last_socket_error();
                break;
            }
        } while (last_error == EINTR);

        if (last_error != 0) {
            ec = {last_error, error::system_category()};
        }

        return status;
    }
} // namespace libwire::internal_

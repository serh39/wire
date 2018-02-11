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

#include <cstring>
#include <libwire/internal/error/system_category.hpp>
#include <ws2tcpip.h>

// clang-format off
#define ERRORS_MAP \
    MAP_CODE  (0,                       error::success); \
    MAP_CODE  (WSAEINVAL,               error::invalid_argument); \
    MAP_CODE  (WSAEACCES,               error::permission_denied); \
    MAP_CODE  (WSAEWOULDBLOCK,          error::try_again); \
    MAP_CODE_3(WSAENOBUFS,              error::out_of_memory, error::generic::no_resources); \
    MAP_CODE_3(WSA_NOT_ENOUGH_MEMORY,   error::out_of_memory, error::generic::no_resources); \
    MAP_CODE  (WSAEINPROGRESS,          error::in_progress); \
    MAP_CODE  (WSAEALREADY,             error::already); \
    MAP_CODE  (WSAEINTR,                error::interrupted); \
    MAP_CODE_3(WSAEMFILE,               error::process_limit_reached, error::generic::no_resources); \
    MAP_CODE  (WSAEPROTONOSUPPORT,      error::protocol_not_supported); \
    MAP_CODE  (WSAEAFNOSUPPORT,         error::protocol_not_supported); \
    MAP_CODE_3(WSAECONNREFUSED,         error::connection_refused, error::generic::no_destination); \
    MAP_CODE  (WSAEADDRINUSE,           error::already_in_use); \
    MAP_CODE  (WSAEADDRNOTAVAIL,        error::address_not_available); \
    MAP_CODE_3(WSAECONNABORTED,         error::connection_aborted, error::generic::disconnected); \
    MAP_CODE_3(WSAECONNRESET,           error::connection_reset, error::generic::disconnected); \
    MAP_CODE_3(WSAESHUTDOWN,            error::shutdown, error::generic::disconnected); \
    MAP_CODE_3(WSAEHOSTDOWN,            error::host_down, error::generic::no_destination); \
    MAP_CODE_3(WSAEHOSTUNREACH,         error::host_unreachable, error::generic::no_destination); \
    \
    /* Our custom code. */ \
    MAP_CODE_3(EOF,                     error::end_of_file, error::generic::disconnected); \
    \
    MAP_CODE(WSAEFAULT,             error::unexpected); \
    MAP_CODE(WSAEISCONN,            error::unexpected); \
    MAP_CODE(WSAEBADF,              error::unexpected); \
    MAP_CODE(WSAEPROTOTYPE,         error::unexpected); \
    MAP_CODE(WSAENOTSOCK,           error::unexpected); \
    MAP_CODE(WSAEOPNOTSUPP,         error::unexpected); \
    MAP_CODE(WSA_INVALID_HANDLE,    error::unexpected); \
    MAP_CODE(WSA_INVALID_PARAMETER, error::unexpected);
// clang-format on

const char* libwire::internal_::system_category::name() const noexcept {
    return "system";
}

std::string libwire::internal_::system_category::message(int code) const noexcept {
    // clang-format off
    switch (code) {
    case 0:                     return "Success";
    case EOF:                   return "End of file";
    case WSAEINVAL:             return "Invalid argument";
    case WSAEACCES:             return "Permission denied";
    case WSAEWOULDBLOCK:        return "Operation would block";
    case WSAENOBUFS:            return "No buffer space";
    case WSA_NOT_ENOUGH_MEMORY: return "Out of memory";
    case WSAEINPROGRESS:        return "Operation in progress";
    case WSAEALREADY:           return "Already running";
    case WSAEINTR:              return "System call interrupted";
    case WSAEMFILE:             return "Per-process limit hit";
    case WSAEPROTONOSUPPORT:    return "Protocol not supported";
    case WSAEAFNOSUPPORT:       return "Protocol not supported";
    case WSAECONNREFUSED:       return "Connection refused";
    case WSAEADDRINUSE:         return "Address already in use";
    case WSAEADDRNOTAVAIL:      return "Address not available";
    case WSAECONNABORTED:       return "Connection aborted";
    case WSAECONNRESET:         return "Connection reset";
    case WSAESHUTDOWN:          return "Endpoint shutdown";
    case WSAEHOSTDOWN:          return "Host is down";
    case WSAEHOSTUNREACH:       return "Host is unreachable";
    default:                    return "Unknown error";
    }
    // clang-format on
}

std::error_condition libwire::internal_::system_category::default_error_condition(int code) const noexcept {
#define MAP_CODE(errno_code, condition_code) \
    if (code == (errno_code)) return std::error_condition((condition_code), *this)
#define MAP_CODE_3(errno_code, condition_code, generic_condition) \
    MAP_CODE(errno_code, condition_code)

    ERRORS_MAP;

    return std::error_condition(error::unknown, *this);
#undef MAP_CODE
#undef MAP_CODE_3
}

bool libwire::internal_::system_category::equivalent(int code, const std::error_condition& condition) const
noexcept {
#define MAP_CODE(errno_code, condition_code) \
    if (code == (errno_code)) return condition.value() == (condition_code)
#define MAP_CODE_3(errno_code, condition_code, generic_code) \
    do { \
        if (code == (errno_code)) { \
            return condition.value() == (condition_code) || condition.value() == int(generic_code); \
        } \
    } while (false)

    ERRORS_MAP;

    return condition.value() == error::unknown;
}

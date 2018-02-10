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

#include <libwire/internal/error/dns_category.hpp>

#ifdef __WIN32
    #include <ws2tcpip.h>
#else
    #include <netdb.h>
#endif

const char* libwire::internal_::dns_category::name() const noexcept {
    return "dns";
}

std::string libwire::internal_::dns_category::message(int code) const noexcept {
    switch (code) {
    case 0: return "Success";
    case WSATRY_AGAIN: return "Host not found (try again)";
    case WSANO_DATA: return "No address";
    case WSAEAFNOSUPPORT: return "No address";
    default: return error::system_category().message(code);
    }
}

std::error_condition libwire::internal_::dns_category::default_error_condition(int code) const noexcept {
    if (code == 0) {
        return std::error_condition(error::success, error::system_category());
    }

    if (code == WSATRY_AGAIN) {
        return std::error_condition(error::host_not_found_try_again, error::dns_category());
    }

    if (code == WSANO_DATA || code == WSAEAFNOSUPPORT) {
        return std::error_condition(error::no_address, error::dns_category());
    }

    if (code == WSAEINVAL || code == WSATYPE_NOT_FOUND|| code == WSAESOCKTNOSUPPORT) {
        return std::error_condition(error::unexpected, error::system_category());
    }

    return error::system_category().default_error_condition(code);
}

bool libwire::internal_::dns_category::equivalent(int code, const std::error_condition& condition) const noexcept {
    if (code == 0) {
        return condition.value() == error::success;
    }

    if (code == WSATRY_AGAIN) {
        return condition.value() == error::host_not_found_try_again;
    }

    if (code == WSANO_DATA || code == WSAEAFNOSUPPORT) {
        return condition.value() == error::no_address;
    }

    if (code == WSAEINVAL || code == WSATYPE_NOT_FOUND || code == WSAESOCKTNOSUPPORT) {
        return condition.value() == error::unexpected;
    }

    return error::system_category().equivalent(code, condition);
}

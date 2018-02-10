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

#include "libwire/udp/socket.hpp"
#include <cassert>

namespace libwire::udp {
    socket::socket(ip ip_version) {
        std::error_code ec;
        implementation = internal_::socket(ip_version, transport::udp, ec);
        if (ec) implementation = internal_::socket();
    }

    internal_::socket::native_handle_t socket::native_handle() const {
        return implementation.handle;
    }

    void socket::bind(address source, uint16_t port, std::error_code& ec) noexcept {
        implementation.bind(port, source, ec);
    }

#ifdef __cpp_exceptions
    void socket::listen(address source, uint16_t port) {
        std::error_code ec;
        bind(source, port, ec);
        if (ec) throw std::system_error(ec);
    }
#endif

    void socket::associate(address destination, uint16_t port) noexcept {
        std::error_code ec;
        implementation.connect(destination, port, ec);
        assert(!ec);
    }

    void socket::open(ip ip_version, std::error_code& ec) noexcept {
        implementation = internal_::socket(ip_version, transport::udp, ec);
    }

#ifdef __cpp_exceptions
    void socket::open(ip ip_version) {
        std::error_code ec;
        open(ip_version, ec);
        if (ec) throw std::system_error(ec);
    }
#endif

    void socket::close() noexcept {
        implementation = internal_::socket();
    }
} // namespace libwire::udp

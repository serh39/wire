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

#include "libwire/internal/socket_utils.hpp"

#include <cassert>
#include "libwire/internal/endianess.hpp"

#ifdef _WIN32
#    include <ws2tcpip.h>
#else
#    include <unistd.h>
#    include <sys/socket.h>
#    include <netinet/ip.h>
#endif

std::tuple<libwire::address, uint16_t> libwire::internal_::sockaddr_to_endpoint(sockaddr_storage in) {
    if (in.ss_family == AF_INET) {
        auto& sock_address_v4 = reinterpret_cast<sockaddr_in&>(in);
        return {memory_view((uint8_t*)&sock_address_v4.sin_addr, sizeof(sock_address_v4.sin_addr)),
                network_to_host(sock_address_v4.sin_port)};
    }
    if (in.ss_family == AF_INET6) {
        auto& sock_address_v6 = reinterpret_cast<sockaddr_in6&>(in);
        return {memory_view((uint8_t*)&sock_address_v6.sin6_addr, sizeof(sock_address_v6.sin6_addr)),
                network_to_host(sock_address_v6.sin6_port)};
    }
    assert(false);
}

sockaddr_storage libwire::internal_::endpoint_to_sockaddr(std::tuple<libwire::address, uint16_t> in) {
    sockaddr_storage addr{};
    if (std::get<0>(in).version == ip::v4) {
        auto& addr_v4 = reinterpret_cast<sockaddr_in&>(addr);
        addr_v4.sin_family = AF_INET;
        addr_v4.sin_addr.s_addr = *reinterpret_cast<uint32_t*>(std::get<0>(in).parts.data());
        addr_v4.sin_port = host_to_network(std::get<1>(in));
    }
    if (std::get<0>(in).version == ip::v6) {
        auto& addr_v6 = reinterpret_cast<sockaddr_in6&>(addr);
        addr_v6.sin6_family = AF_INET6;
        addr_v6.sin6_addr = *reinterpret_cast<in6_addr*>(std::get<0>(in).parts.data());
        addr_v6.sin6_port = host_to_network(std::get<1>(in));
    }
    return addr;
    assert(false);
}

int libwire::internal_::last_socket_error() {
#ifdef _WIN32
    return WSAGetLastError();
#else
    return errno;
#endif
}

int libwire::internal_::last_async_socket_error(libwire::internal_::socket::native_handle_t handle) {
    int err = 0;
    socklen_t errlength = sizeof(err);
    getsockopt(handle, SOL_SOCKET, SO_ERROR, &err, &errlength);
    assert(errlength == sizeof(err));
    return err;
}

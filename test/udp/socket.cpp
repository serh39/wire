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

#include "../gtest.hpp"
#include <libwire/udp.hpp>
#include <libwire/options.hpp>

using namespace libwire;
using namespace std::literals;

TEST(UdpSocket, SimpleTransmission) {
    udp::socket sock1(ip::v4), sock2(ip::v4);
    std::vector<uint8_t> out_buffer(32, 0xAF), in_buffer;
    sock2.set_option(libwire::receive_timeout, 10s);
    sock2.set_option(libwire::send_timeout, 10s);

    sock1.bind(ipv4::loopback, 7777);
    sock2.write(out_buffer, {{ipv4::loopback, 7777}});
    sock1.read(out_buffer.size(), in_buffer);
    ASSERT_EQ(out_buffer, in_buffer);
}

TEST(UdpSocket, QueryEndpointWithoutConnection) {
    tcp::socket sock1;
    ASSERT_EQ(sock1.local_endpoint(), std::tuple(address{0, 0, 0, 0}, 0u));
    ASSERT_EQ(sock1.remote_endpoint(), std::tuple(address{0, 0, 0,0}, 0u));
}

TEST(UdpSocket, DoubleBindShouldFail) {
    udp::socket sock(ip::v4);

    sock.bind(ipv4::loopback, 7777);
    ASSERT_THROW(sock.bind(ipv4::loopback, 7777), std::system_error);
}

TEST(UdpSocket, Associate) {
    udp::socket sock1(ip::v4), sock2(ip::v4);
    std::vector<uint8_t> out_buffer(32, 0xAF), in_buffer;
    sock2.set_option(libwire::receive_timeout, 10s);
    sock2.set_option(libwire::send_timeout, 10s);

    sock1.bind(ipv4::loopback, 7777);
    sock2.associate(ipv4::loopback, 7777);
    sock2.write(out_buffer);
    sock1.read(out_buffer.size(), in_buffer);
    ASSERT_EQ(out_buffer, in_buffer);
}


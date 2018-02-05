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

#include <iostream>
#include <libwire/udp/socket.hpp>

// https://stackoverflow.com/questions/1098897
constexpr size_t max_datagram_size = 512;

int main(int argc, char** argv) {
    using namespace libwire;
    using namespace std::literals::string_view_literals;

    if (argc != 2) {
        std::cerr << "Usage: udp-echo-server <port>\n";
        return 1;
    }

    uint16_t port = std::stoi(argv[1]);

    udp::socket socket(ip::v4);
    socket.listen(ipv4::any, port);

    std::cout << "Listening on " << port << " port.\n";

    std::error_code ec;
    while (true) {
        auto [ datagram, source_endpoint ] = socket.read<std::string>(max_datagram_size, ec);
        if (ec) {
            std::cerr << "Read error: " << ec.message();
            continue;
        }
        std::cout << std::get<0>(source_endpoint).to_string() << ':' << std::get<1>(source_endpoint) << " > "
                  << datagram << '\n';

        socket.write(datagram, ec, source_endpoint);
        if (ec) {
            std::cerr << "Write error: " << ec.message();
            continue;
        }
        std::cout << std::get<0>(source_endpoint).to_string() << ':' << std::get<1>(source_endpoint) << " < "
                  << datagram << '\n';
    }
}

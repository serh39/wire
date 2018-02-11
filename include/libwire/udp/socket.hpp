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

#include <vector>
#include <system_error>
#include <optional>
#include <libwire/internal/socket.hpp>

namespace libwire::udp {
    /**
     * Wrapper for UDP socket descriptor.
     *
     * UDP (User Datagram Protocol) is a lightweight, unreliable,
     * datagram-oriented, connectionless protocol. It can be used when
     * reliability isn't important.
     *
     * Remember that UDP is not connection-oriented, association functions
     * provided by library just allow you to omit destination address in
     * every write call.
     *
     * **Current restrictions**
     *
     * Current API doesn't supports IP-version agnostic UDP interface. You have
     * to specify IP version in constructor or open() call.
     */
    class socket {
    public:
        /**
         * Create socket handle and allocate UDP socket with specified
         * IP version.
         */
        explicit socket(ip ip_version);

        /**
         * Create socket handle without associated socket.
         */
        socket() = default;

        socket(const socket&) = delete;
        socket(socket&&) = default;

        socket& operator=(const socket&) = delete;
        socket& operator=(socket&&) = default;

        ~socket() = default;

        internal_::socket::native_handle_t native_handle() const;

        /**
         * Get option value for socket.
         *
         * See \ref tcp::socket::option and \ref tcp::socket::set_option
         * for better explanation. There is no special options implemented
         * for UDP sockets.
         */
        template<typename Option>
        auto option(const Option& tag) const;

        /**
         * Set option value for socket.
         *
         * See \ref tcp::socket::option and \ref tcp::socket::set_option
         * for better explanation. There is no special options implemented
         * for UDP sockets.
         */
        template<typename Option, typename... Value>
        void set_option(const Option& tag, Value&&... value);

        /**
         * Start accepting datagrams on specified local endpoint.
         */
        void bind(address source, uint16_t port, std::error_code& ec) noexcept;

        /**
         * Same as overload with error code argument but will throw
         * std::system_error.
         */
        void listen(address source, uint16_t port);

        /**
         * Associate remote endpoint with UDP socket.
         *
         * This allows you to omit destination argument from \ref write call
         * (allows but not requires, you can still pass destination argument
         * and it will override association).
         *
         * Also you will receive datagrams only from specified source.
         *
         * This function will replace previous destination information if
         * any.
         *
         * \note This function never fail because there is no way to test
         * whether remote endpoint is valid before trying to use it in
         * \ref read or \ref write operation.
         */
        void associate(address destination, uint16_t port) noexcept;

        /**
         * Open socket using specified IP protocol version.
         */
        void open(ip ip_version, std::error_code& ec) noexcept;

#ifdef __cpp_exceptions
        /**
         * Same as overload with error code argument but will throw
         * std::system_error instead of setting ec argument.
         */
        void open(ip ip_version);
#endif

        /**
         * Free underlying socket file descriptor.
         */
        void close() noexcept;

        /**
         * Read datagram from socket.
         *
         * Errors will be reporting using ec argument.
         * Return value is undefined if ec is changed by this function.
         *
         * \warning If you set max_size to smaller value than pending
         * datagram it **will be truncated with no way to receive remaining
         * information**.
         */
        template<typename Buffer = std::vector<uint8_t>>
        std::tuple<address, uint16_t> read(size_t max_size, Buffer& output, std::error_code& ec) noexcept;

#ifdef __cpp_exceptions
        /**
         * Same as overload with error code but throws std::system_error instead
         * of setting error code argument.
         */
        template<typename Buffer = std::vector<uint8_t>>
        std::tuple<address, uint16_t> read(size_t max_size, Buffer& output);
#endif

        /**
         * Same as overload with buffer argument but will return newly allocated
         * buffer every time.
         */
        template<typename Buffer = std::vector<uint8_t>>
        std::tuple<Buffer, std::tuple<address, uint16_t>> read(size_t max_size, std::error_code& ec) noexcept;

#ifdef __cpp_exceptions
        /**
         * Same as overload with error code but throws std::system_error instead
         * of setting error code argument.
         */
        template<typename Buffer = std::vector<uint8_t>>
        std::tuple<Buffer, std::tuple<address, uint16_t>> read(size_t max_size);
#endif

        /**
         * Send datagram to endpoint specified by destination argument or by
         * socket association set using \ref associate.
         *
         * Errors will be reported using ec argument.
         */
        template<typename Buffer = std::vector<uint8_t>>
        void write(const Buffer& input, std::error_code& ec,
                   std::optional<std::tuple<address, uint16_t>> destination = {}) noexcept;

#ifdef __cpp_exceptions
        /**
         * Same as overload with error code but throws std::system_error instead
         * of setting error code argument.
         */
        template<typename Buffer = std::vector<uint8_t>>
        void write(const Buffer& input, std::optional<std::tuple<address, uint16_t>> destination = {});
#endif
    private:
        internal_::socket implementation;
    };

    template<typename Option>
    auto socket::option(const Option& /* tag */) const {
        return Option::get(*this);
    }

    template<typename Option, typename... Value>
    void socket::set_option(const Option& /* tag */, Value&&... value) {
        Option::set(*this, std::forward<Value>(value)...);
    }

    template<typename Buffer>
    std::tuple<address, uint16_t> socket::read(size_t max_size, Buffer& output, std::error_code& ec) noexcept {
        static_assert(sizeof(typename Buffer::value_type) == sizeof(uint8_t),
                      "socket::read can't be used with container with non-byte elements");

        output.resize(max_size);
        auto [address, port, size] = implementation.receive_from(output.data(), max_size, ec);
        output.resize(size);
        return {address, port};
    }

#ifdef __cpp_exceptions
    template<typename Buffer>
    std::tuple<address, uint16_t> socket::read(size_t max_size, Buffer& output) {
        std::error_code ec;
        auto endpoint = read(max_size, output, ec);
        if (ec) throw std::system_error(ec);
        return endpoint;
    }
#endif

    template<typename Buffer>
    std::tuple<Buffer, std::tuple<address, uint16_t>> socket::read(size_t max_size, std::error_code& ec) noexcept {
        Buffer buffer;
        return {buffer, read(max_size, buffer, ec)};
    }

#ifdef __cpp_exceptions
    template<typename Buffer>
    std::tuple<Buffer, std::tuple<address, uint16_t>> socket::read(size_t max_size) {
        Buffer buffer;
        return {buffer, read(max_size, buffer)};
    }
#endif

    template<typename Buffer>
    void socket::write(const Buffer& input, std::error_code& ec,
                       std::optional<std::tuple<address, uint16_t>> destination) noexcept {
        static_assert(sizeof(typename Buffer::value_type) == sizeof(uint8_t),
                      "socket::write can't be used with container with non-byte elements");

        implementation.send_to(input.data(), input.size(), ec, destination);
    }

#ifdef __cpp_exceptions
    template<typename Buffer>
    void socket::write(const Buffer& input, std::optional<std::tuple<address, uint16_t>> destination) {
        std::error_code ec;
        write(input, ec, destination);
        if (ec) throw std::system_error(ec);
    }
#endif
} // namespace libwire::udp

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

#include <cstdint>
#include <tuple>
#include <system_error>
#include <optional>
#include <libwire/address.hpp>
#include <libwire/protocols.hpp>

#ifdef _WIN32
#    include <winsock2.h>
#endif

namespace libwire::internal_ {
    /**
     * Thin C++ wrapper for BSD-like sockets.
     */
    struct socket {
#ifdef _WIN32
        using native_handle_t = SOCKET;
        static constexpr native_handle_t not_initialized = INVALID_SOCKET;
#else
        using native_handle_t = int;

        /**
         * Constant native_handle() can be compared against to
         * check whether socket handle refers to alive socket.
         *
         * \note Prefer to use operator bool() for this check.
         */
        static constexpr native_handle_t not_initialized = -1;
#endif

        static unsigned max_pending_connections;

        /**
         * Construct handle without allocating socket.
         */
        socket() noexcept = default;

        explicit socket(native_handle_t handle, ip ip_version, transport transport_protocol)
            : ip_version(ip_version), transport_protocol(transport_protocol), handle(handle) {
        }

        /**
         * Allocate new socket with specified family (network protocol)
         * and socket type (transport protocol).
         */
        socket(ip ipver, transport transport, std::error_code& ec) noexcept;

        socket(const socket&) = delete;
        socket(socket&&) noexcept;
        socket& operator=(const socket&) = delete;
        socket& operator=(socket&&) noexcept;

        /**
         * Close underlying file descriptor if any.
         *
         * May block if there are unsent data left.
         */
        ~socket();

        /**
         * Connect socket to remote endpoint, set ec if any error
         * occurred.
         */
        void connect(address target, uint16_t port, std::error_code& ec) noexcept;

        /**
         * Shutdown read/write parts of full-duplex connection.
         */
        void shutdown(bool read = true, bool write = true) noexcept;

        /**
         * Bind socket to local port using interface specified in interface_address,
         * set ec if any error occurred.
         */
        void bind(uint16_t port, address interface_address, std::error_code& ec) noexcept;

        /**
         * Start accepting connections on this listener socket.
         *
         * backlog is a hint for underlying implementation it can use
         * to limit number of pending connections in socket's queue.
         */
        void listen(int backlog, std::error_code& ec) noexcept;

        /**
         * Extract and accept first connection from queue and create socket for it,
         * set ec if any error occurred.
         */
        socket accept(std::error_code& ec) noexcept;

        /**
         * Write length_bytes from input to socket, set ec if any error
         * occurred and return real count of data written.
         */
        size_t write(const void* input, size_t length_bytes, std::error_code& ec) noexcept;

        /**
         * Read up to length_bytes from input to socket, set ec if any error
         * occurred and return real count of data read.
         */
        size_t read(void* output, size_t length_bytes, std::error_code& ec) noexcept;

        /**
         * Send length_bytes from input to destination, set ec if any error
         * occurred.
         */
        size_t send_to(const void* input, size_t length_bytes, std::error_code& ec,
                       std::optional<std::tuple<address, uint16_t>> destination) noexcept;

        /**
         * Read length_bytes from socket datagram queue to output, set ec if
         * any occurred, return source endpoint and actual size of datagram.
         */
        std::tuple<address, uint16_t, size_t> receive_from(void* output, size_t length_bytes,
                                                           std::error_code& ec) noexcept;

        /**
         * Allows to check whether socket is initialized and can be operated on.
         */
        operator bool() const noexcept;

        std::tuple<address, uint16_t> local_endpoint() const noexcept;

        std::tuple<address, uint16_t> remote_endpoint() const noexcept;

        const ip ip_version = ip(0);
        const transport transport_protocol = transport(0);

        struct {
            /// Is user requested non-blocking I/O mode?
            bool user_non_blocking : 1;

            /// Do we really have non-blocking socket now?
            bool internal_non_blocking : 1;
        } state{};

        native_handle_t handle = not_initialized;
    };
} // namespace libwire::internal_

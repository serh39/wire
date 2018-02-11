#include "libwire/internal/socket.hpp"

#include <cassert>
#include <optional>
#include "libwire/internal/socket_utils.hpp"
#include "libwire/internal/endianess.hpp"
#include "libwire/error.hpp"

#ifdef _WIN32
#    include <ws2tcpip.h>
#    define SHUT_RD SD_RECEIVE
#    define SHUT_WR SD_SEND
#    define SHUT_RDWR SD_BOTH
#    define close closesocket
#    define ssize_t int64_t
#else
#    include <unistd.h>
#    include <sys/socket.h>
#    include <netinet/ip.h>
#    define INVALID_SOCKET (-1)
#endif

namespace libwire::internal_ {
#ifdef _WIN32
    struct Initializer {
        Initializer() {
            WSAData data;
            [[maybe_unused]] int status = WSAStartup(MAKEWORD(2, 2), &data);
            assert(status == 0);
        }

        ~Initializer() {
            WSACleanup();
        }
    };
#endif

    unsigned socket::max_pending_connections = SOMAXCONN;

    socket::socket(ip ipver, transport transport, std::error_code& ec) noexcept {
#ifdef _WIN32
        static Initializer init;
#endif

        int domain;
        switch (ipver) {
        case ip::v4: domain = AF_INET; break;
        case ip::v6: domain = AF_INET6; break;
        }

        int type, protocol;
        switch (transport) {
        case transport::tcp:
            type = SOCK_STREAM;
            protocol = IPPROTO_TCP;
            break;
        case transport::udp:
            type = SOCK_DGRAM;
            protocol = IPPROTO_UDP;
            break;
        }

        handle = ::socket(domain, type, protocol);
        if (handle == INVALID_SOCKET) {
            ec = std::error_code(last_socket_error(), error::system_category());
            assert(ec != error::unexpected);
        }

#ifdef SO_NOSIGPIPE
        int one = 1;
        setsockopt(handle, SOL_SOCKET, SO_NOSIGPIPE, &one, sizeof(one));
#endif
    }

    socket::socket(socket&& o) noexcept {
        std::swap(o.handle, this->handle);
    }

    socket& socket::operator=(socket&& o) noexcept {
        std::swap(o.handle, this->handle);
        return *this;
    }

    socket::~socket() {
        if (handle != not_initialized) close(handle);
    }

    void socket::shutdown(bool read, bool write) noexcept {
        assert(handle != not_initialized);

        int how = 0;
        if (read && !write) how = SHUT_RD;
        if (!read && write) how = SHUT_WR;
        if (read && write) how = SHUT_RDWR;
        assert(how != 0);

        ::shutdown(handle, how);
    }

    void socket::connect(address target, uint16_t port, std::error_code& ec) noexcept {
        assert(handle != not_initialized);

        struct sockaddr address = endpoint_to_sockaddr({target, port});

        error_wrapper(ec, ::connect, handle, &address, socklen_t(sizeof(address)));
    }

    void socket::bind(uint16_t port, address interface_address, std::error_code& ec) noexcept {
        assert(handle != not_initialized);

        sockaddr_in address{};
        address.sin_family = AF_INET;
        address.sin_port = host_to_network(port);
        address.sin_addr = *reinterpret_cast<in_addr*>(interface_address.parts.data());

        error_wrapper(ec, ::bind, handle, reinterpret_cast<sockaddr*>(&address), socklen_t(sizeof(address)));
    }

    void socket::listen(int backlog, std::error_code& ec) noexcept {
        assert(handle != not_initialized);

        error_wrapper(ec, ::listen, handle, backlog);
    }

    socket socket::accept(std::error_code& ec) noexcept {
        assert(handle != not_initialized);

        int accepted_fd = ::accept(handle, nullptr, nullptr);
        // TODO (PR's welcomed): Allow to get client address.

        if (accepted_fd == INVALID_SOCKET) {
            if (last_socket_error() == EINTR) {
                return accept(ec);
            }
            ec = std::error_code(last_socket_error(), error::system_category());
            assert(ec != error::unexpected);
            return socket();
        }

        return socket(accepted_fd, this->ip_version, this->transport_protocol);
    }

#ifdef MSG_NOSIGNAL
#    define IO_FLAGS MSG_NOSIGNAL
#else
#    define IO_FLAGS 0
#endif

    size_t socket::write(const void* input, size_t length_bytes, std::error_code& ec) noexcept {
        assert(handle != not_initialized);

        ssize_t actually_written =
            error_wrapper(ec, ::send, handle, reinterpret_cast<const char*>(input), length_bytes, IO_FLAGS);
        return size_t(actually_written);
    }

    size_t socket::read(void* output, size_t length_bytes, std::error_code& ec) noexcept {
        assert(handle != not_initialized);

        ssize_t actually_readen =
            error_wrapper(ec, recv, handle, reinterpret_cast<char*>(output), length_bytes, IO_FLAGS);
        if (actually_readen == 0 && length_bytes != 0) {
            // We wanted more than zero bytes but got zero, looks like EOF.
            ec = std::error_code(EOF, error::system_category());
            return 0;
        }
        return size_t(actually_readen);
    }

    socket::operator bool() const noexcept {
        return handle != not_initialized;
    }

    std::tuple<address, uint16_t> socket::local_endpoint() const noexcept {
        assert(handle != not_initialized);

        sockaddr sock_address{};
        socklen_t length = sizeof(sock_address);
        [[maybe_unused]] int status = getsockname(handle, &sock_address, &length);
#ifndef NDEBUG
        if (status < 0) return {{0, 0, 0, 0}, 0u};
#endif
        return sockaddr_to_endpoint(sock_address);
    }

    std::tuple<address, uint16_t> socket::remote_endpoint() const noexcept {
        assert(handle != not_initialized);

        sockaddr sock_address{};
        socklen_t length = sizeof(sock_address);
        [[maybe_unused]] int status = getpeername(handle, &sock_address, &length);
#ifndef NDEBUG
        if (status < 0) return {{0, 0, 0, 0}, 0u};
#endif
        return sockaddr_to_endpoint(sock_address);
    }

    size_t socket::send_to(const void* input, size_t length_bytes, std::error_code& ec,
                           std::optional<std::tuple<address, uint16_t>> destination) noexcept {
        assert(handle != not_initialized);

        ssize_t actually_written;
        if (destination) {
            sockaddr addr = endpoint_to_sockaddr(*destination);
            actually_written = error_wrapper(ec, sendto, handle, reinterpret_cast<const char*>(input), length_bytes,
                                             IO_FLAGS, &addr, uint32_t(sizeof(sockaddr)));
        } else {
            actually_written = error_wrapper(ec, sendto, handle, reinterpret_cast<const char*>(input), length_bytes,
                                             IO_FLAGS, nullptr, 0u);
        }
        return actually_written < 0 ? 0 : size_t(actually_written);
    }

    std::tuple<address, uint16_t, size_t> socket::receive_from(void* output, size_t length_bytes,
                                                               std::error_code& ec) noexcept {
        assert(handle != not_initialized);

        sockaddr sockaddr;
        socklen_t socklen = sizeof(sockaddr);

        ssize_t received_bytes = error_wrapper(ec, ::recvfrom, handle, reinterpret_cast<char*>(output), length_bytes,
                                               IO_FLAGS, &sockaddr, &socklen);

        std::tuple<address, uint16_t> endpoint = sockaddr_to_endpoint(sockaddr);
        return {std::get<0>(endpoint), std::get<1>(endpoint), received_bytes};
    }
} // namespace libwire::internal_

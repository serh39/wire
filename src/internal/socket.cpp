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
            handle = not_initialized;
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

        sockaddr_storage address = endpoint_to_sockaddr({target, port});

        error_wrapper(ec, ::connect, handle, reinterpret_cast<sockaddr*>(&address), socklen_t(sizeof(address)));
    }

    void socket::bind(uint16_t port, address interface_address, std::error_code& ec) noexcept {
        assert(handle != not_initialized);

        // sockaddr doesn't have enough space to store sockaddr_in6 but we don't care
        sockaddr_storage address = endpoint_to_sockaddr({interface_address, port});
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
#    define NO_SIGPIPE MSG_NOSIGNAL
#else
#    define NO_SIGPIPE 0
#endif

    size_t socket::write(const void* input, size_t length_bytes, std::error_code& ec) noexcept {
        assert(handle != not_initialized);

        ssize_t actually_written =
            error_wrapper(ec, ::send, handle, reinterpret_cast<const char*>(input), length_bytes, NO_SIGPIPE);
        return size_t(actually_written);
    }

    size_t socket::read(void* output, size_t length_bytes, std::error_code& ec) noexcept {
        assert(handle != not_initialized);
        if (length_bytes == 0) {
            return 0;
        }

        ssize_t actually_read =
            error_wrapper(ec, recv, handle, reinterpret_cast<char*>(output), length_bytes, NO_SIGPIPE | MSG_WAITALL);

        // Treat short read as a EOF because we use MSG_WAITALL.
        // In this case read() can read less bytes than requested only if we
        // hit end-of-stream.
        if (!state.internal_non_blocking && actually_read < ssize_t(length_bytes) && actually_read != -1) {
            ec = std::error_code(EOF, error::system_category());
        }

        // Short read in non-blocking mode is fine, EOF is hit only if we got 0 bytes.
        if (state.internal_non_blocking && actually_read == 0) {
            ec = std::error_code(EOF, error::system_category());
        }

        if (actually_read == -1) {
            return 0;
        }

        return size_t(actually_read);
    }

    socket::operator bool() const noexcept {
        return handle != not_initialized;
    }

    std::tuple<address, uint16_t> socket::local_endpoint() const noexcept {
        assert(handle != not_initialized);

        sockaddr_storage sock_address{};
        socklen_t length = sizeof(sock_address);
        int status = getsockname(handle, reinterpret_cast<sockaddr*>(&sock_address), &length);
        if (status < 0) return {{0, 0, 0, 0}, 0u};
        return sockaddr_to_endpoint(sock_address);
    }

    std::tuple<address, uint16_t> socket::remote_endpoint() const noexcept {
        assert(handle != not_initialized);

        sockaddr_storage sock_address{};
        socklen_t length = sizeof(sock_address);
        int status = getpeername(handle, reinterpret_cast<sockaddr*>(&sock_address), &length);
        if (status < 0) return {{0, 0, 0, 0}, 0u};
        return sockaddr_to_endpoint(sock_address);
    }

    size_t socket::send_to(const void* input, size_t length_bytes, std::error_code& ec,
                           std::optional<std::tuple<address, uint16_t>> destination) noexcept {
        assert(handle != not_initialized);

        ssize_t actually_written;
        if (destination) {
            sockaddr_storage addr = endpoint_to_sockaddr(*destination);
            actually_written =
                error_wrapper(ec, sendto, handle, reinterpret_cast<const char*>(input), length_bytes, NO_SIGPIPE,
                              reinterpret_cast<sockaddr*>(&addr), uint32_t(sizeof(sockaddr_in6)));
        } else {
            actually_written = error_wrapper(ec, sendto, handle, reinterpret_cast<const char*>(input), length_bytes,
                                             NO_SIGPIPE, nullptr, 0u);
        }
        return actually_written < 0 ? 0 : size_t(actually_written);
    }

    std::tuple<address, uint16_t, size_t> socket::receive_from(void* output, size_t length_bytes,
                                                               std::error_code& ec) noexcept {
        assert(handle != not_initialized);

        sockaddr_storage sock_address;
        socklen_t socklen = sizeof(sock_address);

        ssize_t received_bytes = error_wrapper(ec, ::recvfrom, handle, reinterpret_cast<char*>(output), length_bytes,
                                               NO_SIGPIPE, reinterpret_cast<sockaddr*>(&sock_address), &socklen);

        std::tuple<address, uint16_t> endpoint = sockaddr_to_endpoint(sock_address);
        return {std::get<0>(endpoint), std::get<1>(endpoint), received_bytes};
    }
} // namespace libwire::internal_

#include "libwire/tcp/listener.hpp"

namespace libwire::tcp {
    void listener::listen(address local_address, uint16_t port, std::error_code& ec, unsigned max_backlog) noexcept {
        implementation_ = internal_::socket(local_address.version, transport::tcp, ec);
        if (ec) return;
        implementation_.bind(port, local_address, ec);
        if (ec) return;
        implementation_.listen(int(max_backlog), ec);
    }

    socket listener::accept(std::error_code& ec) noexcept {
        return {implementation_.accept(ec)};
    }

#ifdef __cpp_exceptions
    void listener::listen(address local_address, uint16_t port, unsigned max_backlog) {
        std::error_code ec;
        listen(local_address, port, ec, max_backlog);
        if (ec) throw std::system_error(ec);
    }

    socket listener::accept() {
        std::error_code ec;
        auto sock = accept(ec);
        if (ec) throw std::system_error(ec);
        return sock;
    }

    internal_::socket::native_handle_t listener::native_handle() const noexcept {
        return implementation_.handle;
    }

    const internal_::socket& listener::implementation() const noexcept {
        return implementation_;
    }

    internal_::socket& listener::implementation() noexcept {
        return implementation_;
    }

#endif
} // namespace libwire::tcp

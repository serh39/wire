#include "libwire/tcp/socket.hpp"

namespace libwire::tcp {
    template std::vector<uint8_t>& socket::read(size_t, std::vector<uint8_t>&, std::error_code&);
    template std::string& socket::read(size_t, std::string&, std::error_code&);

    template std::vector<uint8_t> socket::read(size_t, std::error_code&);
    template std::string socket::read(size_t, std::error_code&);

    template size_t socket::write(const std::vector<uint8_t>&, std::error_code&);
    template size_t socket::write(const std::string&, std::error_code&);

    template std::vector<uint8_t> socket::read_until(uint8_t, std::error_code&, size_t);
    template std::string socket::read_until(uint8_t, std::error_code&, size_t);

    template std::vector<uint8_t>& socket::read_until(uint8_t, std::vector<uint8_t>&, std::error_code&, size_t);
    template std::string& socket::read_until(uint8_t, std::string&, std::error_code&, size_t);

    socket::socket(internal_::socket&& i) noexcept : implementation_(std::move(i)) {
        open = (implementation_.handle != internal_::socket::not_initialized);
    }

    socket::~socket() {
        open = false;
        if (is_open()) shutdown();
        close();
    }

    internal_::socket::native_handle_t socket::native_handle() const noexcept {
        return implementation_.handle;
    }

    bool socket::is_open() const {
        return this->open;
    }

    void socket::connect(address target, uint16_t port, std::error_code& ec) noexcept {
        implementation_ = internal_::socket(target.version, transport::tcp, ec);
        if (ec) return;
        implementation_.connect(target, port, ec);
        open = !ec;
    }

    void socket::close() noexcept {
        // Reassignment to null socket will call destructor and
        // close destroyed socket.
        implementation_ = internal_::socket();
        open = false;
    }

    void socket::shutdown(bool read, bool write) noexcept {
        implementation_.shutdown(read, write);
    }

    std::tuple<address, uint16_t> socket::local_endpoint() const noexcept {
        return implementation_.local_endpoint();
    }

    std::tuple<address, uint16_t> socket::remote_endpoint() const noexcept {
        return implementation_.remote_endpoint();
    }

    internal_::socket& socket::implementation() noexcept {
        return implementation_;
    }

    const internal_::socket& socket::implementation() const noexcept {
        return implementation_;
    }

#ifdef __cpp_exceptions
    void socket::connect(address target, uint16_t port) {
        std::error_code ec;
        connect(target, port, ec);
        if (ec) throw std::system_error(ec);
    }

    template std::vector<uint8_t>& socket::read(size_t, std::vector<uint8_t>&);
    template std::string& socket::read(size_t, std::string&);

    template std::vector<uint8_t> socket::read(size_t);
    template std::string socket::read(size_t);

    template size_t socket::write(const std::vector<uint8_t>&);
    template size_t socket::write(const std::string&);

    template std::vector<uint8_t>& socket::read_until(uint8_t, std::vector<uint8_t>&, size_t);
    template std::string& socket::read_until(uint8_t, std::string&, size_t);

    template std::vector<uint8_t> socket::read_until(uint8_t, size_t);
    template std::string socket::read_until(uint8_t, size_t);
#endif // ifdef __cpp_exceptions

} // namespace libwire::tcp

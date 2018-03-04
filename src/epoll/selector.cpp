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

#include "libwire/internal/selector.hpp"
#include <cassert>
#include <unistd.h>
#include <sys/epoll.h>
#include <libwire/internal/socket_utils.hpp>

// FIXME: Lack of correct error handling.

namespace libwire::internal_ {
    selector::selector() noexcept {
        epoll_fd = epoll_create1(EPOLL_CLOEXEC);
        assert(epoll_fd > 0);
    }

    selector::~selector() noexcept {
        if (epoll_fd != -1) close(epoll_fd);
    }

    socket_data& selector::register_socket(socket& socket, flags<event_code> interested_events) noexcept {
        assert(epoll_fd != -1);
        assert(socket.handle != socket::not_initialized);

        auto [ it, inserted ] = sockets.emplace(socket.handle, socket_data{socket});
        assert(inserted); // If socket inserted twice - something went really wrong.

        epoll_event event;
        event.events = interested_events.get(event_code::readable)  * EPOLLIN  |
                       interested_events.get(event_code::writable)  * EPOLLOUT |
                       interested_events.get(event_code::error)     * EPOLLERR |
                       interested_events.get(event_code::eof)       * EPOLLHUP;
        event.data.ptr = &(it->second);
        [[maybe_unused]] int status = epoll_ctl(epoll_fd, EPOLL_CTL_ADD, socket.handle, &event);
        assert(status == 0);

        it->second.last_event_mask = interested_events;

        return it->second;
    }

    void selector::change_event_mask(socket::native_handle_t handle, flags<event_code> interested_events) noexcept {
        assert(epoll_fd != -1);
        assert(handle != socket::not_initialized);

        auto& socket_data = sockets.at(handle);
        // TODO: Can we avoid this lookup (move reactor's optimization to this level)?

        epoll_event event;
        event.events = interested_events.get(event_code::readable)  * EPOLLIN  |
                       interested_events.get(event_code::writable)  * EPOLLOUT |
                       interested_events.get(event_code::error)     * EPOLLERR |
                       interested_events.get(event_code::eof)       * EPOLLHUP;
        event.data.ptr = &socket_data;
        [[maybe_unused]] int status = epoll_ctl(epoll_fd, EPOLL_CTL_MOD, handle, &event);
        assert(status == 0);
        socket_data.last_event_mask = interested_events;
    }

    void selector::remove_socket(socket::native_handle_t handle) noexcept {
        assert(epoll_fd != -1);
        assert(handle != socket::not_initialized);

        [[maybe_unused]] int status = epoll_ctl(epoll_fd, EPOLL_CTL_DEL, handle, nullptr);
        assert(status == 0);

        sockets.erase(handle);
    }

    void selector::poll(memory_view<event_t>& event_buffer, std::chrono::milliseconds timeout) noexcept {
        assert(epoll_fd != -1);
        assert(event_buffer.data());

        std::error_code ec;
        int status = error_wrapper(ec, epoll_wait, epoll_fd, event_buffer.data(), event_buffer.size(), timeout.count());
        assert(!ec);
        if (status != -1) {
            event_buffer.resize(status);
        }
    }

    flags<event_code> selector::event_codes(const event_t& event) const noexcept {
        flags<event_code> result;
        if (event.events & EPOLLIN) result.set(event_code::readable);
        if (event.events & EPOLLOUT) result.set(event_code::writable);
        if (event.events & EPOLLERR) result.set(event_code::error);
        if (event.events & EPOLLHUP) result.set(event_code::eof);
        return result;
    }

    socket_data& selector::user_data(const event_t& event) const noexcept {
        return *reinterpret_cast<socket_data*>(event.data.ptr);
    }

    socket_data& selector::user_data(socket::native_handle_t handle) const noexcept {
        return sockets.at(handle);
    }
} // namespace libwire::internal_

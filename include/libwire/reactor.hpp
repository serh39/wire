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

#include <libwire/internal/selector.hpp>

namespace libwire {
    class reactor {
    public:
        /**
         * Start watching socket for read status.
         */
        void add_socket(internal_::socket&);

        /**
         * Stop watching socket for read status.
         */
        void remove_socket(internal_::socket&);

        /**
         * Add operation to socket's queue.
         */
        void enqueue(internal_::socket&, internal_::operation);

        /**
         * Cancel oldest operation from socket's queue.
         */
        void cancel_oldest_operation(internal_::socket&);

        /**
         * Cancel all operations from socket's queue.
         */
        void cancel_all_operations(internal_::socket&);

        /**
         * Poll once and process pending operations.
         */
        void run_once();

    private:
        internal_::selector selector;

        void process_reads(internal_::socket& socket, std::queue<internal_::operation>& operations);

        void process_writes(internal_::socket& socket, std::queue<internal_::operation>& operations);
    };
} // namespace libwire

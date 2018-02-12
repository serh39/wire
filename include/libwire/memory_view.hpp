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

#include <cstddef>
#include <cstdint>
#include <iterator>

/**
 * Defines memory_view wrapper.
 */

/*
 * If you had to open this file to find answer for your question - we are so
 * sorry. Please open issue with your question so we can update documentation
 * to answer it.
 */

#ifdef __cpp_exceptions
#    define LIBWIRE_EXCEPTIONS_ENABLED_BOOL true
#else
#    define LIBWIRE_EXCEPTIONS_ENABLED_BOOL false
#endif

namespace libwire {
    /**
     * Non-owning STL-like wrapper for raw memory.
     * Mostly mimics std::vector except some things:
     * * Not a template - value type is always uint8_t.
     * * Can't be modified (values itself can).
     * * Can shrink in both directions, but doesn't modifies allocation.
     * * Have no allocator.
     * * Capacity is size of underlying memory size FROM \ref begin() TO BIGGEST POSSIBLE \ref end().
     * * Can't extend more than capacity, hence \ref max_size() == \ref capacity().
     *
     * Implemented to allow to use raw memory in read/write operations without additional
     * overload sets.
     */
    template<typename T>
    class memory_view {
    public:
        using value_type = T;
        using size_type = size_t;
        using difference_type = std::ptrdiff_t;
        using reference = value_type&;
        using const_reference = const value_type&;
        using pointer = value_type*;
        using const_pointer = const value_type*;
        using iterator = pointer;
        using const_iterator = const_pointer;
        using reverse_iterator = std::reverse_iterator<iterator>;
        using const_reverse_iterator = std::reverse_iterator<iterator>;

        memory_view() noexcept = default;
        memory_view(T* memory, size_t size_bytes) noexcept;

#ifdef __cpp_exceptions
        const_reference at(size_t) const;
#endif
        const_reference operator[](size_t) const noexcept;

#ifdef __cpp_exceptions
        reference at(size_t);
#endif
        reference operator[](size_t) noexcept;

        const_reference front() const noexcept;
        const_reference back() const noexcept;

        reference front() noexcept;
        reference back() noexcept;

        const_pointer data() const noexcept;
        pointer data() noexcept;

        const_iterator begin() const noexcept;
        const_iterator end() const noexcept;

        iterator begin() noexcept;
        iterator end() noexcept;

        const_iterator cbegin() const noexcept;
        const_iterator cend() const noexcept;

        /**
         * Currently visible memory size.
         */
        size_type size() const noexcept;

        /**
         * Same as \ref capacity.
         */
        size_type max_size() const noexcept;

        /**
         * Return size of underlying memory size FROM \ref begin() TO BIGGEST
         * POSSIBLE \ref end().
         */
        size_type capacity() const noexcept;

        /**
         * "Hide" X bytes from end of memory.
         *
         * Behavior is undefined if bytes_count > \ref size().
         */
        void shrink_back(size_type bytes_count) noexcept;

        /**
         * "Hide" X bytes from begin of memory.
         *
         * Behavior is undefined if bytes_count > \ref size().
         */
        void shrink_front(size_type bytes_count) noexcept;

        /**
         * Same as \ref resize (0).
         */
        void clear() noexcept;

        /**
         * "Hide" X bytes from end of memory so that new_size is left visible.
         * Can also show previous hidden bytes up to capacity().
         *
         * If new_size > capacity() and exceptions enabled then
         * std::out_of_range is thrown, if exceptions disabled then
         * this function have no effect in this case.
         */
        void resize(size_t new_size) noexcept(!LIBWIRE_EXCEPTIONS_ENABLED_BOOL);

        void swap(memory_view& other) noexcept;

    private:
        T* data_ = nullptr;
        size_t size_ = 0;
        size_t capacity_ = 0;
    };

    template<typename T>
    memory_view<T>::memory_view(T* memory, size_t size_bytes) noexcept
        : data_(memory), size_(size_bytes), capacity_(size_bytes) {
    }

#ifdef __cpp_exceptions
    template<typename T>
    const T& memory_view<T>::at(size_t i) const {
        if (i > size()) {
            throw std::out_of_range("Index is bigger than size");
        }
        return *(data_ + i);
    }
#endif // ifdef __cpp_exceptions

    template<typename T>
    const T& memory_view<T>::operator[](size_t i) const noexcept {
        return *(data_ + i);
    }

#ifdef __cpp_exceptions
    template<typename T>
    T& memory_view<T>::at(size_t i) {
        if (i > size()) {
            throw std::out_of_range("Index is bigger than size");
        }
        return *(data_ + i);
    }
#endif // ifdef __cpp_exceptions

    template<typename T>
    T& memory_view<T>::operator[](size_t i) noexcept {
        return *(data_ + i);
    }

    template<typename T>
    const T& memory_view<T>::front() const noexcept {
        return *data_;
    }

    template<typename T>
    const T& memory_view<T>::back() const noexcept {
        return *(data_ + size() - 1);
    }

    template<typename T>
    T& memory_view<T>::front() noexcept {
        return *data_;
    }

    template<typename T>
    T& memory_view<T>::back() noexcept {
        return *(data_ + size() - 1);
    }

    template<typename T>
    const T* memory_view<T>::data() const noexcept {
        return data_;
    }

    template<typename T>
    T* memory_view<T>::data() noexcept {
        return data_;
    }

    template<typename T>
    const T* memory_view<T>::begin() const noexcept {
        return data_;
    }

    template<typename T>
    const T* memory_view<T>::end() const noexcept {
        return data_ + size();
    }

    template<typename T>
    T* memory_view<T>::begin() noexcept {
        return data_;
    }

    template<typename T>
    T* memory_view<T>::end() noexcept {
        return data_ + size();
    }

    template<typename T>
    const T* memory_view<T>::cbegin() const noexcept {
        return data_;
    }

    template<typename T>
    const T* memory_view<T>::cend() const noexcept {
        return data_ + size();
    }

    template<typename T>
    size_t memory_view<T>::size() const noexcept {
        return size_;
    }

    template<typename T>
    size_t memory_view<T>::max_size() const noexcept {
        return capacity_;
    }

    template<typename T>
    size_t memory_view<T>::capacity() const noexcept {
        return capacity_;
    }

    template<typename T>
    void memory_view<T>::shrink_back(size_t bytes_count) noexcept {
        size_ -= bytes_count;
    }

    template<typename T>
    void memory_view<T>::shrink_front(size_t bytes_count) noexcept {
        capacity_ -= bytes_count;
        size_ -= bytes_count;
        data_ += bytes_count;
    }

    template<typename T>
    void memory_view<T>::clear() noexcept {
        size_ = 0;
    }

    template<typename T>
    void memory_view<T>::resize(size_t new_size) noexcept(!LIBWIRE_EXCEPTIONS_ENABLED_BOOL) {
        if (new_size > capacity_) {
#ifdef __cpp_exceptions
            throw std::out_of_range("Too big new_size");
#else
            return;
#endif
        }
        size_ = new_size;
    }

    template<typename T>
    void memory_view<T>::swap(memory_view<T>& other) noexcept {
        using std::swap;

        std::swap(this->data_, other.data_);
        std::swap(this->size_, other.size_);
        std::swap(this->capacity_, other.capacity_);
    }

    extern template class memory_view<uint8_t>;
    extern template class memory_view<int8_t>;
} // namespace libwire

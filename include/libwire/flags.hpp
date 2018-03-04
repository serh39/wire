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

#include <utility>          // std::declvar
#include <initializer_list> // std::initializer_list

namespace libwire {
    /**
     * Type-safe wrapper for OR-ed flags.
     *
     * This class satisfies requirements of BitmaskType concept.
     *
     * \tparam Flags Usually enum but also can be any type which have
     *               valid bitwise operators that return Storage.
     * \tparam Storage Type to use for OR-ed flags store. Following requirement
     *                 should be met: sizeof(Flag) <= sizeof(Storage).
     */
    template<typename Flag, typename Storage = uint32_t>
    class flags {
    public:
        static_assert((sizeof(Flag) <= sizeof(Storage)),
                      "flags<> uses Storage type not enough to hold values of Flag.");

        flags() = default;

        /**
         * Construct Flags from OR-ed flags.
         *
         * \warning No checks performed thus no type safety.
         */
        explicit constexpr inline flags(Storage f) noexcept : set_(f) {
        }

        /**
         * Construct Flags with a single flag set.
         */
        constexpr flags(Flag flag) noexcept : set_(Storage(flag) | 0x00000000) {
        }

        constexpr flags(std::initializer_list<Flag> il) noexcept {
            for (Flag flag : il) {
                set_ |= Storage(flag);
            }
        }

        constexpr bool get(Flag flag) const noexcept {
            return ((set_ & Storage(flag)) == Storage(flag));
        }

        constexpr void set(Flag flag, bool value = true) noexcept {
            if (value) {
                set_ |= Storage(flag);
            } else {
                set_ &= ~Storage(flag);
            }
        }

        constexpr bool operator!() const noexcept {
            return set_ == 0;
        }

        constexpr explicit operator Storage() const noexcept {
            return set_;
        }

        constexpr flags operator~() const noexcept {
            return flags(~set_);
        }

        constexpr flags operator&(Flag flag) const noexcept {
            return flags(set_ & Storage(flag));
        }

        constexpr flags operator|(Flag flag) const noexcept {
            return flags(set_ | Storage(flag));
        }

        constexpr flags operator^(Flag flag) const noexcept {
            return flags(set_ ^ Storage(flag));
        }

        constexpr flags operator&(const flags& f) const noexcept {
            return flags(set_ & f.set_);
        }

        constexpr flags operator|(const flags& f) const noexcept {
            return flags(set_ | f.set_);
        }

        constexpr flags operator^(const flags& f) const noexcept {
            return flags(set_ ^ f.set_);
        }

        constexpr flags& operator&=(Flag flag) noexcept {
            set_ &= Storage(flag);
            return *this;
        }

        constexpr flags& operator|=(Flag flag) noexcept {
            set_ |= Storage(flag);
            return *this;
        }

        constexpr flags& operator^=(Flag flag) noexcept {
            set_ ^= Storage(flag);
            return *this;
        }

        constexpr flags& operator&=(const flags& f) noexcept {
            set_ &= f.set_;
            return *this;
        }

        constexpr flags& operator|=(const flags& f) noexcept {
            set_ |= f.set_;
            return *this;
        }

        constexpr flags& operator^=(const flags& f) noexcept {
            set_ ^= f.set_;
            return *this;
        }

    private:
        Storage set_ = 0x00000000;
    };

#define LIBWIRE_DECLARE_FLAGS(flagtype, name) using name = Flags<flagtype>

#define LIBWIRE_DECLARE_FLAGS_OPERATORS(flagtype, storage) \
    constexpr flags<flagtype> operator|(flagtype lhs, flagtype rhs) noexcept { \
        return flags<flagtype>(storage(lhs) | storage(rhs)); \
    } \
    constexpr flags<flagtype> operator|(flagtype rhs, flags<flagtype, storage> lhs) noexcept { \
        return flags<flagtype>(storage(rhs) | storage(lhs)); \
    }
} // namespace libwire

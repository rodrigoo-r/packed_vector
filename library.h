//#==|---------------------------------------------------|==#
//   *****              zext::packed_vector             *****
//
// A variation of the STL std::vector, exclusively for
// unsigned integer or enum values.
//
// Version 0.0.1
// https://github.com/rodrigoo-r/packed_vector
//
// Licensed under the Apache License v2.0
// <https://www.apache.org/licenses/LICENSE-2.0>.
// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 Rodrigo R. <rodrigo@zelixlang.dev>
//
// This file is an extension of the Zelix Programming
// Language Backend, called Z; hence, the zext identifier.
//
// NOTE: This file requires C++20 or later.
//#==|---------------------------------------------------|==#

#pragma once

#include <vector>
#include <bitset>
#include <cassert>
#include <type_traits>

namespace zext
{
    namespace __intl__
    {
        template <
            unsigned Bits_Per_Element,
            unsigned Max_Value,

            // The inner container type that holds the integers
            std::unsigned_integral Container_Type,

            // The type of the elements we return when indexing/retrieving
            // NOTE: Not necessarily the elements in the vector, as those are bits
            std::unsigned_integral T
        >
        class __packed_slot_base
        {
            // The maximum amount of bits we can store in a single slot
            // Effectively, we may have some bits unused. We trade throughput
            // for memory footprint reduction
            static constexpr auto max_bits = sizeof(Container_Type) * CHAR_BIT;

            // The maximum number of elements we can store in a single slot
            static constexpr auto max_elements = max_bits / Bits_Per_Element;

            // Assert that the elements are smaller than the number of bits
            // we can store in a single slot
            static_assert(Bits_Per_Element <= max_bits);

            std::bitset<max_bits> bits; // Main storage for bits
            size_t len = 0; // Number of active elements in the slot

        public:
            class __iterator
            {
            public:
                // STL Compatibility
                using iterator_category = std::random_access_iterator_tag;
                using value_type        = T;
                using difference_type   = std::ptrdiff_t;
                using pointer           = void;
                using reference         = value_type;

            private:
                using Slot =
                    __packed_slot_base<Bits_Per_Element, Max_Value, Container_Type, T>;

                Slot *s_ptr;
                size_t el_idx = 0; // Index of the element in the slot

            public:
                __iterator(Slot *slot, size_t el_idx) :
                    s_ptr(slot), el_idx(el_idx)
                {}

                reference operator*() const { return s_ptr->at(el_idx); }
                void operator->() const = delete;

                // Prefix increment
                auto &operator++()
                {
                    ++el_idx;
                    return *this;
                }

                // Postfix increment
                auto operator++(int)
                {
                    auto tmp = *this;
                    ++(*this);

                    return tmp;
                }

                auto operator==(const __iterator& other) const
                {
                    return s_ptr == other.s_ptr && el_idx == other.el_idx;
                }
            };

            class __const_iterator
            {
            public:
                // STL Compatibility
                using iterator_category = std::random_access_iterator_tag;
                using value_type        = const T;
                using difference_type   = std::ptrdiff_t;
                using pointer           = void;
                using reference         = value_type;

            private:
                using Slot =
                    __packed_slot_base<Bits_Per_Element, Max_Value, Container_Type, T>;

                using Const_Slot = const Slot;
                using Const_Slot_Ptr = const Slot *const;

                Const_Slot_Ptr s_ptr;
                size_t el_idx = 0; // Index of the element in the slot

            public:
                __const_iterator(Const_Slot_Ptr slot, size_t el_idx) :
                    s_ptr(slot), el_idx(el_idx)
                {}

                reference operator*() const { return s_ptr->at(el_idx); }
                void operator->() const = delete;

                // Prefix increment
                auto &operator++()
                {
                    ++el_idx;
                    return *this;
                }

                // Postfix increment
                auto operator++(int)
                {
                    auto tmp = *this;
                    ++(*this);

                    return tmp;
                }

                auto operator==(const __const_iterator& other) const
                {
                    return s_ptr == other.s_ptr && el_idx == other.el_idx;
                }
            };

        private:
            friend class __iterator;
            friend class __const_iterator;

        public:
            [[nodiscard]] auto size()           const noexcept { return len; }
            [[nodiscard]] auto empty()          const noexcept { return len == 0; }
            [[nodiscard]] auto capacity()       const noexcept { return max_elements; }
            [[nodiscard]] auto capacity_bits()  const noexcept { return max_bits; }
            [[nodiscard]] auto max_size()       const noexcept { return max_elements; }
            [[nodiscard]] auto full()           const noexcept { return len == max_elements; }

            void insert_bits(size_t index, T value)
            {
                assert(!full() && value <= Max_Value);

                // NOTE: index is in elements, not bits!
                auto begin = index * Bits_Per_Element;
                auto end = begin + Bits_Per_Element;

                for (auto i = begin; i < end; ++i)
                {
                    bits[i] = (value >> (end - i - 1)) & 1;
                }

                ++len;
            }

            [[nodiscard]] auto at(size_t idx) const
            {
                assert(idx < len);
                // NOTE: idx is in elements, not bits!
                auto begin = idx * Bits_Per_Element;
                auto end = begin + Bits_Per_Element;
                T result{};

                for (auto i = begin; i < end; ++i)
                {
                    result |= (bits[i] << (end - i - 1));
                }

                return result;
            }

            template <typename ...Args>
            void emplace_back(Args &&...args)
            {
                auto val = T{ std::forward<Args>(args)... };
                insert_bits(len++, std::move(val));
            }

            void push_back(T value)
            {
                insert_bits(len++, value);
            }

            auto begin()    const noexcept { return __const_iterator(this, 0); }
            auto end()      const noexcept { return __const_iterator(this, len); }

            auto begin()    noexcept { return __iterator(this, 0); }
            auto end()      noexcept { return __iterator(this, len); }
            auto rbegin()   noexcept { return __iterator(this, len - 1); }
            auto rend()     noexcept { return __iterator(this, -1); }

            auto rbegin()   const noexcept { return __const_iterator(this, len - 1); }
            auto rend()     const noexcept { return __const_iterator(this, -1); }

            auto cbegin()   const noexcept { return __const_iterator(this, 0); }
            auto cend()     const noexcept { return __const_iterator(this, len); }
        };

        template<std::unsigned_integral T>
        consteval int __bits_per_element(T n)
        {
            // Mathematically, a given integer T uses at least Ceil(Log2(T + 1)) bits
            // However, we can't make that consteval, so we use C++20 utilities:
            // - Find the number of value bits in a given type T (A)
            // - Find the leading zeroes in a given type T (B)
            // Then A - B = N; N + 1 = Ceil(Log2(T + 1))
            return n <= 1 ? 0 : std::numeric_limits<T>::digits - std::countl_zero(n - 1);
        }
    }

    namespace pmr
    {
        template <
            // The maximum number that a given element T can be, inclusive
            unsigned Max_Value,

            // The type of the elements we return when indexing/retrieving
            // NOTE: Not necessarily the elements in the vector, as those are bits
            std::unsigned_integral T
        >
        class packed_vector
        {
            static constexpr auto bits_per_element = (unsigned)__intl__::__bits_per_element(Max_Value);

            using Inner_Container =
                std::conditional_t<
                    (bits_per_element < CHAR_BIT),
                    // IMPORTANT: uint_fast32_t isn't always exactly 32 bits, but at least 32 bits
                    uint_fast32_t,
                    std::conditional_t<
                        (bits_per_element < (CHAR_BIT + (CHAR_BIT / 2))),
                        // IMPORTANT: same as uint_fast32_t applies for uint_fast64_t
                        uint_fast64_t,
#                       ifdef __SIZEOF_INT128__
                            // Compiler extension, not portable
                            __uint128_t
#                       else
                            // Safe fallback
                           uintmax_t
#                       endif
                    >
                >
            ;

            using Slot =
                __intl__::__packed_slot_base<bits_per_element, Max_Value, Inner_Container, T>;

            std::pmr::vector<Slot> slots;
            size_t len = 0; // Number of active elements across all containers

        public:
            packed_vector(auto *allocator) :
                slots(allocator)
            {}

            [[nodiscard]] auto size()       const noexcept { return len; }
            auto operator[](size_t idx)     const { return at(idx); }

            template <typename... Args>
            void emplace_back(Args &&... value)
            {
                if (slots.empty() || slots.back().full())
                {
                    slots.emplace_back();
                }

                slots.back().emplace_back(std::forward<Args>(value)...);
                ++len;
            }

            [[nodiscard]] auto at(size_t idx) const
            {
                assert(idx < len);
                return slots[idx / bits_per_element][idx % bits_per_element];
            }

            void push_back(T value) { emplace_back(value); }
        };
    }
}
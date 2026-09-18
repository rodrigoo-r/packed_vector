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
#include <memory_resource>

// Magic enum integration, to support packed_vector with enums
#if __has_include(<magic_enum.hpp>)
#   ifndef ZEXT_DISABLE_MAGIC_ENUM
#       include <magic_enum.hpp>
#   endif
#endif

namespace zext
{
    namespace __intl__
    {
        template <typename T>
        concept __maybe_enum = std::is_unsigned_v<T> || std::is_enum_v<T>;

        template <
            unsigned Bits_Per_Element,
            std::unsigned_integral Container_Type,
            __maybe_enum T
        >
        class __bitset_base_inner
        {
        public:
            // The maximum amount of bits we can store in a single slot
            // Effectively, we may have some bits unused. We trade throughput
            // for memory footprint reduction
            static constexpr auto max_bits = sizeof(Container_Type) * CHAR_BIT;

            // The maximum number of elements we can store in a single slot
            static constexpr auto max_elements = max_bits / Bits_Per_Element;

        private:
            // Assert that the elements are smaller than the number of bits
            // we can store in a single slot
            static_assert(Bits_Per_Element <= max_bits);

            std::bitset<max_bits> bits;
            size_t len = 0; // Number of active elements in the slot

        public:
            [[nodiscard]] bool full() const noexcept { return len == max_elements; }

            void insert(T val)
            {
                assert(!full());
                const auto begin = len++ * Bits_Per_Element;

                auto insert_value = (Container_Type)val;
                for (std::size_t bit = 0; bit < Bits_Per_Element; ++bit)
                {
                    bits[begin + bit] = (insert_value >> (Bits_Per_Element - bit - 1)) & 1;
                }
            }

            auto retrieve(size_t idx) const
            {
                auto begin = Bits_Per_Element * idx;
                auto end = begin + Bits_Per_Element;
                unsigned int result = 0;

                for (auto i = begin; i < end; ++i)
                {
                    result |= unsigned{bits[i]} << (end - i - 1);
                }

                return (T)result;
            }

            auto size() const noexcept { return len; }
        };

        template <
            unsigned Bits_Per_Element,

            // The inner container type that holds the integers
            std::unsigned_integral Container_Type,

            // The type of the elements we return when indexing/retrieving
            // NOTE: Not necessarily the elements in the vector, as those are bits
            __maybe_enum T
        >
        class __packed_slot_base
        {
        public:
            using Base = __bitset_base_inner<Bits_Per_Element, Container_Type, T>;

        private:
            Base inner;

        public:
            [[nodiscard]] auto size()           const noexcept { return inner.size(); }
            [[nodiscard]] auto empty()          const noexcept { return size() == 0; }
            [[nodiscard]] auto capacity()       const noexcept { return Base::max_elements; }
            [[nodiscard]] auto capacity_bits()  const noexcept { return Base::max_bits; }
            [[nodiscard]] auto max_size()       const noexcept { return Base::max_elements; }
            [[nodiscard]] auto full()           const noexcept { return size() == Base::max_elements; }

            [[nodiscard]] auto at(size_t idx) const { return inner.retrieve(idx); }

            template <typename ...Args>
            void emplace_back(Args &&...args)
            {
                auto val = T{ std::forward<Args>(args)... };
                inner.insert(std::move(val));
            }

            void push_back(T value) { inner.insert(value); }
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

    namespace config
    {
        enum class storage_selection
        {
            automatic,
            manual
        };
    };

    namespace pmr
    {
        template <
            // The maximum number that a given element T can be, inclusive
            unsigned Max_Value,

            // The type of the elements we return when indexing/retrieving
            // NOTE: Not necessarily the elements in the vector, as those are bits
            __intl__::__maybe_enum T,

            // How the user prefers to select the inner storage that holds elements
            config::storage_selection Storage = config::storage_selection::automatic,
            std::unsigned_integral Word = std::uint32_t // Placeholder if automatic storage is selected
        >
        class packed_vector
        {
            static constexpr auto bits_per_element = (unsigned)__intl__::__bits_per_element(Max_Value);

            using Inner_Container =
                std::conditional_t<
                    Storage == config::storage_selection::manual,
                    Word,
                    std::conditional_t<
                        (bits_per_element < CHAR_BIT),
                        // IMPORTANT: uint_fast32_t isn't always exactly 32 bits, but at least 32 bits
                        std::uint_fast32_t,
                        std::conditional_t<
                            (bits_per_element < (CHAR_BIT + (CHAR_BIT / 2))),
                            // IMPORTANT: same as uint_fast32_t applies for uint_fast64_t
                            std::uint_fast64_t,
    #                       ifdef __SIZEOF_INT128__
                                // Compiler extension, not portable
                                __uint128_t
    #                       else
                                // Safe fallback
                               std::uintmax_t
    #                       endif
                        >
                    >
                >
            ;

            using Slot =
                __intl__::__packed_slot_base<bits_per_element, Inner_Container, T>;

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
                auto slot_idx = idx / Slot::Base::max_elements;
                auto relative_idx = idx % Slot::Base::max_elements;
                return slots[slot_idx].at(relative_idx);
            }

            void push_back(T value) { emplace_back(value); }

            void resize_pool(size_t new_size)
            {
                slots.resize(new_size);
            }

            void resize(size_t new_size)
            {
                if (len >= new_size) return;

                auto elements_per_slot = Slot::max_elements;
                auto old_size = slots.size();
                auto slots_needed = (new_size + elements_per_slot - 1) / elements_per_slot;

                if (slots_needed <= old_size) return;
                auto slots_requested = slots_needed - old_size;

                resize_pool(slots_requested);
                len = new_size;
            }
        };
    }

    template <
        // The maximum number that a given element T can be, inclusive
        unsigned Max_Value,

        // The type of the elements we return when indexing/retrieving
        // NOTE: Not necessarily the elements in the vector, as those are bits
        __intl__::__maybe_enum T,

        // How the user prefers to select the inner storage that holds elements
        config::storage_selection Storage = config::storage_selection::automatic,
        std::unsigned_integral Word = std::uint32_t // Placeholder if automatic storage is selected
    >
    class packed_vector :
        public pmr::packed_vector<Max_Value, T, Storage, Word>
    {
        using Base = pmr::packed_vector<Max_Value, T, Storage, Word>;

    public:
        using Base::Base;

        packed_vector() :
            Base(std::pmr::get_default_resource())
        {}
    };

    // Magic enum integration
#   if __has_include(<magic_enum.hpp>)
#   ifndef ZEXT_DISABLE_MAGIC_ENUM
    template <
        // The type of the elements we return when indexing/retrieving
        // NOTE: Not necessarily the elements in the vector, as those are bits
        typename T,

        // How the user prefers to select the inner storage that holds elements
        config::storage_selection Storage = config::storage_selection::automatic,
        std::unsigned_integral Word = std::uint32_t
    >
    class packed_enum_vector :
        // Integrate magic_enum to get the count of elements
        public packed_vector<
            magic_enum::enum_count<T>,
            T,
            Storage,
            Word
        >
    {
        using Base = packed_vector<magic_enum::enum_count<T>, T>;
    public:
        using Base::Base;
    }
#   endif
#   endif
}
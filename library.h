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
    namespace intl
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
            [[nodiscard]] auto size() const noexcept { return len; }
            [[nodiscard]] auto empty() const noexcept { return len == 0; }
            [[nodiscard]] auto capacity() const noexcept { return max_elements; }
            [[nodiscard]] auto capacity_bits() const noexcept { return max_bits; }
            [[nodiscard]] auto max_size() const noexcept { return max_elements; }
            [[nodiscard]] auto full() const noexcept { return len == max_elements; }

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
        };
    }

    namespace pmr
    {
        template <
            unsigned Max_Element
        >
        class packed_vector
        {

        };
    }
}
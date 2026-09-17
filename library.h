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

namespace zext
{
    namespace __intl__
    {
        template <typename T>
        concept __maybe_enum = std::is_unsigned_v<T> || std::is_enum_v<T>;

        template <
            unsigned Bits_Per_Element,
            unsigned Max_Value,

            // The inner container type that holds the integers
            std::unsigned_integral Container_Type,

            // The type of the elements we return when indexing/retrieving
            // NOTE: Not necessarily the elements in the vector, as those are bits
            __maybe_enum T
        >
        class __packed_slot_base
        {
        public:
            // The maximum amount of bits we can store in a single slot
            // Effectively, we may have some bits unused. We trade throughput
            // for memory footprint reduction
            static constexpr auto max_bits = sizeof(Container_Type) * CHAR_BIT;

            // The maximum number of elements we can store in a single slot
            static constexpr auto max_elements = max_bits / Bits_Per_Element;

            // Assert that the elements are smaller than the number of bits
            // we can store in a single slot
            static_assert(Bits_Per_Element <= max_bits);

        private:
            std::bitset<max_bits> bits; // Main storage for bits
            size_t len = 0; // Number of active elements in the slot

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

                auto operator!=(const __iterator& other) const
                {
                    return !operator==(other);
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

                auto operator!=(const __const_iterator& other) const
                {
                    return !operator==(other);
                }
            };

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
                assert(!full() && (Container_Type)value <= Max_Value);

                // NOTE: index is in elements, not bits!
                auto begin = index * Bits_Per_Element;
                auto end = begin + Bits_Per_Element;

                if constexpr (std::is_enum_v<T>)
                {
                    auto ins_value = (Container_Type)value;
                    for (auto i = begin; i < end; ++i)
                    {
                        bits[i] = (ins_value >> (end - i - 1)) & 1;
                    }
                }
                else
                {
                    for (auto i = begin; i < end; ++i)
                    {
                        bits[i] = (value >> (end - i - 1)) & 1;
                    }
                }

                ++len;
            }

            [[nodiscard]] auto at(size_t idx) const
            {
                assert(idx < len);
                // NOTE: idx is in elements, not bits!
                auto begin = idx * Bits_Per_Element;
                auto end = begin + Bits_Per_Element;
                Container_Type result{};

                for (auto i = begin; i < end; ++i)
                {
                    result |= (bits[i] << (end - i - 1));
                }

                return (T)result;
            }

            template <typename ...Args>
            void emplace_back(Args &&...args)
            {
                auto val = T{ std::forward<Args>(args)... };
                insert_bits(len, std::move(val));
            }

            void push_back(T value)
            {
                insert_bits(len, value);
            }
        };

        template<std::unsigned_integral T>
        consteval int __bits_per_element(T n)
        {
            // Mathematically, a given integer T uses at least Ceil(Log2(T + 1)) bits
            // However, we can't make that consteval, so we use C++20 utilities:
            // - Find the number of value bits in a given type T (A)
            // - Find the leading zeroes in a given type T (B)
            // Then A - B = N; N + 1 = Ceil(Log2(T + 1))
            return (n <= 1 ? 0 : std::numeric_limits<T>::digits - std::countl_zero(n - 1)) + 1;
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
                __intl__::__packed_slot_base<bits_per_element, Max_Value, Inner_Container, T>;

            std::pmr::vector<Slot> slots;
            size_t len = 0; // Number of active elements across all containers

        public:
            class iterator
            {
            public:
                // STL Compatibility
                using iterator_category = std::random_access_iterator_tag;
                using value_type        = T;
                using difference_type   = std::ptrdiff_t;
                using pointer           = void;
                using reference         = value_type;

            private:
                using Vector =
                    packed_vector<Max_Value, T, Storage, Word>;

                Vector *v_ptr;
                size_t slot_idx = 0; // Index of the slot in the vector
                size_t el_idx = 0; // Index of the element in the slot

            public:
                iterator(Vector *slot, size_t slot_idx, size_t el_idx) :
                    v_ptr(slot), slot_idx(slot_idx), el_idx(el_idx)
                {}

                reference operator*() const
                {
                    auto &v_slots = v_ptr->slots;

                    assert(slot_idx < v_slots.size());
                    auto &slot = v_slots[slot_idx];
                    assert(el_idx < slot.size());

                    return slot.at(el_idx);
                }

                void operator->() const = delete;

                // Prefix increment
                auto &operator++()
                {
                    auto &v_slots = v_ptr->slots;
                    auto &slot = v_slots[slot_idx];

                    if (slot_idx == slot.size() - 1)
                    {
                        ++slot_idx;
                        el_idx = 0;
                    }
                    else
                    {
                        ++el_idx;
                    }

                    return *this;
                }

                // Postfix increment
                auto operator++(int)
                {
                    auto tmp = *this;
                    ++(*this);

                    return tmp;
                }

                auto operator==(const iterator& other) const
                {
                    return v_ptr == other.v_ptr &&
                        el_idx == other.el_idx &&
                        slot_idx == other.slot_idx;
                }

                auto operator!=(const iterator& other) const
                {
                    return !operator==(other);
                }
            };

            class const_iterator
            {
            public:
                // STL Compatibility
                using iterator_category = std::random_access_iterator_tag;
                using value_type        = const T;
                using difference_type   = std::ptrdiff_t;
                using pointer           = void;
                using reference         = value_type;

            private:
                using Vector =
                    packed_vector<Max_Value, T, Storage, Word>;

                using Const_Vector = const Vector;
                using Const_Vector_Ptr = const Vector *const;

                Const_Vector_Ptr v_ptr;
                size_t slot_idx = 0; // Index of the slot in the vector
                size_t el_idx = 0; // Index of the element in the slot

            public:
                const_iterator(Const_Vector_Ptr slot, size_t slot_idx, size_t el_idx) :
                    v_ptr(slot), slot_idx(slot_idx), el_idx(el_idx)
                {}

                reference operator*() const
                {
                    auto &v_slots = v_ptr->slots;

                    assert(slot_idx < v_slots.size());
                    auto &slot = v_slots[slot_idx];
                    assert(el_idx < slot.size());

                    return slot.at(el_idx);
                }

                void operator->() const = delete;

                // Prefix increment
                auto &operator++()
                {
                    auto &v_slots = v_ptr->slots;

                    auto &slot = v_slots[slot_idx];
                    if (slot_idx == slot.size() - 1)
                    {
                        ++slot_idx;
                        el_idx = 0;
                    }
                    else
                    {
                        ++el_idx;
                    }

                    return *this;
                }

                // Postfix increment
                auto operator++(int)
                {
                    auto tmp = *this;
                    ++(*this);

                    return tmp;
                }

                auto operator==(const const_iterator& other) const
                {
                    return v_ptr == other.v_ptr &&
                        el_idx == other.el_idx &&
                        slot_idx == other.slot_idx;
                }

                auto operator!=(const const_iterator& other) const
                {
                    return !operator==(other);
                }
            };

        private:
            friend class iterator;

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
                return slots[idx / bits_per_element].at(idx % bits_per_element);
            }

            void push_back(T value) { emplace_back(value); }

            auto begin()    const noexcept { return const_iterator(this, 0, 0); }
            auto end()      const noexcept { return const_iterator(this, slots.size(), slots.back().size()); }

            auto begin()    noexcept { return iterator(this, 0, 0); }
            auto end()      noexcept { return iterator(this, slots.size() - 1, slots.back().size() - 1); }

            auto rbegin()   noexcept { return iterator(this, slots.size() - 1, slots.back().size() - 1); }
            auto rend()     noexcept { return iterator(this, -1, -1); }

            auto rbegin()   const noexcept { return const_iterator(this, slots.size() - 1, slots.back().size() - 1); }
            auto rend()     const noexcept { return const_iterator(this, -1, -1); }

            auto cbegin()   const noexcept { return const_iterator(this, 0, 0); }
            auto cend()     const noexcept { return const_iterator(this, slots.size(), slots.back().size()); }

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
}
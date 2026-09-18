//#==|---------------------------------------------------|==#
//   *****              zext::packed_vector             *****
//
// A variation of the STL std::vector, exclusively for
// unsigned integer or enum values.
//
// Version 1.0.0
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
// #==|---------------------------------------------------|==#

#include <iostream>
#include <algorithm>
#include <zext/packed_vector.h>

using namespace std;

int main()
{
    vector<int> vec2;
    for (int i = 1; i <= 10'000; ++i)
    {
        vec2.push_back(i);
    }

    // Base usage
    zext::packed_vector<3, unsigned, zext::config::storage_selection::manual, __uint128_t> vec;
    vec.push_back(1);
    vec.push_back(2);
    vec.push_back(3);

    cout << *vec.at(0) << endl;

    vec.at(0) = 0;

    cout << "---------------------" << endl;
    // Iteration
    for (auto val : vec)
    {
        cout << *val << endl;
    }

    // Arbitrary index modifying
    vec.set(2, 1);

    cout << "---------------------" << endl;
    // Iteration
    for (auto val : vec)
    {
        cout << *val << endl;
    }

    vec.clear();
    for (int i = 1; i <= 10'000; ++i)
    {
        vec.push_back(1);
    }

    cout << "---------------------" << endl;
    cout << "Memory footprint for 10'000 elements: " << endl;
    cout << "std::vector: " << (vec2.capacity() * sizeof(int)) << " bytes" << endl;
    cout << "zext::packed_vector: " << (vec.capacity() * vec.slot_size()) << " bytes" << endl;

    return 0;
}
# `zext::packed_vector`

A header-only C++20 container for storing small unsigned integers—or enum
values—in a compact bit-packed representation.

`zext::packed_vector` is intended for sequences whose values have a known,
small upper bound. Rather than allocating a full native integer for each
entry, it stores only the number of bits required by that bound. For example,
values in the range `0` through `15` use four bits per element.

> **Status:** Early-stage project (version 0.0.1). The public API is small and
> may evolve.

## Requirements

- A C++20-compatible compiler
- CMake 3.12 or later when integrating with CMake

The library has no required runtime dependencies. If
[`magic_enum`](https://github.com/Neargye/magic_enum) is available on the
include path, support for `zext::packed_enum_vector` is enabled automatically.
Define `ZEXT_DISABLE_MAGIC_ENUM` before including the header to disable that
integration.

## Quick start

Add the repository's `Include` directory to your compiler's include path, then
include the single header:

```cpp
#include <zext/packed_vector.h>

#include <cstdint>
#include <iostream>

int main()
{
    // Each value is expected to be in [0, 15], so four bits are stored per item.
    zext::packed_vector<15, std::uint8_t> values;

    values.push_back(3);
    values.emplace_back(12);

    std::cout << values.size() << " values: "
              << static_cast<unsigned>(values[0]) << ", "
              << static_cast<unsigned>(values.at(1)) << '\n';
}
```

Compile it with a C++20 compiler, for example:

```sh
c++ -std=c++20 -I/path/to/packed_vector/Include example.cpp -o example
```

## CMake integration

Add the project as a subdirectory, then link its interface target. The header
directory uses an uppercase `Include` in the current source tree; add it
explicitly until the project layout and CMake include path are aligned.

```cmake
add_subdirectory(path/to/packed_vector)

target_link_libraries(my_target PRIVATE zext::packed_vector)
target_include_directories(my_target PRIVATE
    path/to/packed_vector/Include
)
```

## Usage

### Choosing the bound and value type

The first template argument, `Max_Value`, is the inclusive maximum value that
will be stored. It determines the number of bits reserved for every element.
The second argument is the unsigned integer or enum type returned by access
operations.

```cpp
// Values 0 through 7: 3 bits per element.
zext::packed_vector<7, std::uint8_t> flags;

// Values 0 through 1,023: 10 bits per element.
zext::packed_vector<1023, std::uint16_t> identifiers;
```

Keep inserted values within the declared range. The container is designed for
unsigned integral types and enums; signed integer types are not supported.

### Appending and reading values

The implemented sequence operations are `push_back`, `emplace_back`, `size`,
`at`, and read-only `operator[]`.

```cpp
zext::packed_vector<31, std::uint8_t> scores;

scores.push_back(18);
scores.emplace_back(27);

auto first = scores.at(0);  // Bounds checked with an assertion.
auto second = scores[1];
```

Elements are returned by value. The current API does not provide mutable index
references, iterators, or erase/pop operations.

### Selecting the storage word

By default, the container selects an unsigned word type automatically. To
choose one yourself, pass `zext::config::storage_selection::manual` followed
by an unsigned integral word type:

```cpp
using compact_values = zext::packed_vector<255,
                                           std::uint8_t,
                                           zext::config::storage_selection::manual,
                                           std::uint32_t>;
```

The storage word must be wide enough to hold one packed element.

### Polymorphic memory resources

`zext::pmr::packed_vector` accepts a `std::pmr::memory_resource`, allowing the
slot storage to use an application-provided allocator. `zext::packed_vector`
uses the default polymorphic resource.

```cpp
#include <array>
#include <memory_resource>

std::array<std::byte, 1024> buffer;
std::pmr::monotonic_buffer_resource resource{buffer.data(), buffer.size()};

zext::pmr::packed_vector<63, std::uint8_t> values{&resource};
values.push_back(42);
```

### Enum values with `magic_enum`

When `magic_enum.hpp` is available, `packed_enum_vector` determines the upper
bound from the number of enumerators:

```cpp
enum class colour : std::uint8_t { red, green, blue };

zext::packed_enum_vector<colour> colours;
colours.push_back(colour::green);
```

## How packing works

Values are appended to fixed-size slots. A slot holds several values in a
single unsigned storage word, with each value occupying the number of bits
derived from `Max_Value`. This reduces storage for bounded values, at the cost
of bit manipulation during reads and writes. If the element width does not
divide the word width exactly, unused bits may remain in each slot.

## License

Copyright © 2026 Rodrigo R.

Distributed under the [Apache License, Version 2.0](LICENSE).

## Contributing

Please follow the repository's [Code of Conduct](CODE_OF_CONDUCT.md) when
participating in the project.


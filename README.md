# `zext::packed_vector`

`zext::packed_vector` is a header-only C++20 container for compactly storing
bounded unsigned integers or enum values. It packs each element into only the
bits required by its declared inclusive maximum, instead of allocating a full
integer-sized object for every value.

For example, values from `0` through `15` require four bits each. Values from
`0` through `3` require two bits each.

> **Version:** 1.0.0

## Requirements

- A C++20-compatible compiler
- CMake 3.12 or later when building the example or adding the project with
  CMake

There are no required runtime dependencies. If
[`magic_enum`](https://github.com/Neargye/magic_enum) is on the include path,
`zext::packed_enum_vector` support is enabled automatically. Define
`ZEXT_DISABLE_MAGIC_ENUM` before including the header to opt out.

## Quick start

Add the repository's `Include` directory to your compiler's include path and
include the single header:

```cpp
#include <zext/packed_vector.h>

#include <cstdint>
#include <iostream>

int main()
{
    // Values are in [0, 15], so each element occupies four bits.
    zext::packed_vector<15, std::uint8_t> values;

    values.push_back(3);
    values.emplace_back(std::uint8_t{12});

    std::cout << "first: " << static_cast<unsigned>(*values[0]) << '\n';
    values[1] = 7;
    std::cout << "second: " << static_cast<unsigned>(*values.at(1)) << '\n';
}
```

Compile it with a C++20 compiler:

```sh
c++ -std=c++20 -I/path/to/packed_vector/Include example.cpp -o example
```

## CMake integration

Add the project as a subdirectory and link its interface target:

```cmake
add_subdirectory(path/to/packed_vector)

target_link_libraries(my_target PRIVATE zext::packed_vector)
```

The target publishes the `Include` directory and requires C++20.

## Usage

### Choose a bound and value type

The first template argument, `Max_Value`, is the inclusive maximum valid
value. It determines the fixed number of bits reserved per element. The
second argument is the unsigned integer or enum type used by the interface.

```cpp
// 0 through 7: three bits per element.
zext::packed_vector<7, std::uint8_t> flags;

// 0 through 1,023: ten bits per element.
zext::packed_vector<1023, std::uint16_t> identifiers;
```

Use an unsigned integral type or an enum, and only insert values represented
by the bound. Signed integer element types are not supported.

### Add, access, and change values

`push_back` and `emplace_back` append elements. `at` and `operator[]` return
an element proxy: dereference it to read the value, or assign to it to update
the packed value in place. `at` and `set` use assertions to check indexes.

```cpp
zext::packed_vector<31, std::uint8_t> scores;

scores.push_back(18);
scores.emplace_back(std::uint8_t{27});

auto first = *scores.at(0);
scores[1] = 24;
scores.set(0, 20);
```

### Iterate

The container supports range-based iteration. Iteration yields the same proxy
type as indexed access; dereference it to obtain the current value.

```cpp
for (auto value : scores)
{
    std::cout << static_cast<unsigned>(*value) << '\n';
}
```

### Size and removal

`size`, `front`, `back`, `pop_back`, and `clear` provide the usual basic
sequence operations. Accessing `front` or `back` on an empty container, or
calling `pop_back` when it is empty, violates an assertion.

```cpp
if (scores.size() != 0)
{
    auto last = *scores.back();
    scores.pop_back();
}

scores.clear();
```

`capacity()` reports the number of allocated packed slots, while `slot_size()`
reports the byte size of one slot. They are useful for inspecting the storage
layout, not for a `std::vector`-style element-capacity guarantee.

### Select the storage word

Storage is selected automatically by default. To specify an unsigned storage
word yourself, pass `zext::config::storage_selection::manual` followed by the
word type:

```cpp
using compact_values = zext::packed_vector<255,
                                           std::uint8_t,
                                           zext::config::storage_selection::manual,
                                           std::uint32_t>;
```

The chosen word must be wide enough for one packed element.

### Use a polymorphic memory resource

`zext::pmr::packed_vector` takes a `std::pmr::memory_resource` for its slot
storage. `zext::packed_vector` uses the default polymorphic resource.

```cpp
#include <array>
#include <memory_resource>

std::array<std::byte, 1024> buffer;
std::pmr::monotonic_buffer_resource resource{buffer.data(), buffer.size()};

zext::pmr::packed_vector<63, std::uint8_t> values{&resource};
values.push_back(42);
```

### Pack enum values with `magic_enum`

When `magic_enum.hpp` is available, `packed_enum_vector` derives its bound
from the enum's number of enumerators:

```cpp
enum class colour : std::uint8_t { red, green, blue };

zext::packed_enum_vector<colour> colours;
colours.push_back(colour::green);
```

## How packing works

Elements are placed in fixed-size slots backed by an unsigned storage word.
Each value occupies the number of bits derived from `Max_Value`; when a slot
is full, the container allocates another slot. This can substantially reduce
memory use for tightly bounded values, with the trade-off of bit manipulation
during reads and writes. When the element width does not evenly divide the
storage-word width, a slot can contain unused bits.

## License

Copyright © 2026 Rodrigo R.

Distributed under the [Apache License, Version 2.0](LICENSE).

## Contributing

Please follow the repository's [Code of Conduct](CODE_OF_CONDUCT.md) when
participating in the project.

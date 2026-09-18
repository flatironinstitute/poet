# POET

[![CI](https://github.com/flatironinstitute/poet/actions/workflows/ci.yml/badge.svg)](https://github.com/flatironinstitute/poet/actions/workflows/ci.yml)
[![License: MIT](https://img.shields.io/badge/License-MIT-blue.svg)](LICENSE)
[![C++ Standard](https://img.shields.io/badge/C%2B%2B-17%2B-lightblue)](https://en.cppreference.com/w/cpp/17)
[![Coverage](https://codecov.io/gh/flatironinstitute/poet/branch/main/graph/badge.svg)](https://codecov.io/gh/flatironinstitute/poet)
[![Docs](https://github.com/flatironinstitute/poet/actions/workflows/docs.yml/badge.svg)](https://flatironinstitute.github.io/poet/docs/)

POET is a header-only C++ library for three related jobs:

- `static_for`: compile-time unrolled loops
- `dynamic_for`: runtime loops emitted as compile-time unrolled blocks of exactly `Unroll` bodies
- `dispatch` / `dispatch_set`: runtime-to-compile-time specialization

It also exposes CPU detection helpers (`poet::available_registers()`, `poet::cache_line()`)
for ISA, vector-width, and cache-line queries. Requires C++17 or later; the
`dynamic_for` range adaptor requires C++20.

## When not to use it

- `static_for` for large ranges: compile time and code size grow linearly with the range.
- `dynamic_for` for trivial index-only bodies: a plain `for` loop has less overhead.
- `dispatch` when the value is known at compile time (call the template directly), or
  when the set of combinations is very large (table size grows with the product of the ranges).

See [docs/guides](docs/guides/) for the full contract of each primitive, with the test
that verifies it.

## Include

```cpp
#include <poet/poet.hpp>
```

## Examples

Each snippet below is copied from its runnable counterpart under
[`examples/`](examples/). Build with `-DPOET_BUILD_EXAMPLES=ON` and
`ctest -R '^example_'` to run them.

`static_for` ([examples/static_for.cpp](examples/static_for.cpp)):

```cpp
std::array<int, 4> a{};
poet::static_for<0, 4>([&](auto I) {
    a[I] = I * I;
});
```

`dynamic_for` ([examples/dynamic_for.cpp](examples/dynamic_for.cpp)):

```cpp
std::vector<int> out(n);
poet::dynamic_for<4>(std::size_t{ 0 }, n, [&](std::size_t i) { out[i] = static_cast<int>(i * i); });
```

`dispatch` ([examples/dispatch.cpp](examples/dispatch.cpp)):

```cpp
struct AddN {
    template<int N> int operator()(int x) const { return N + x; }
};

int y = poet::dispatch(AddN{}, poet::dispatch_param<poet::inclusive_range<0, 4>>{ choice }, 10);
```

`dispatch_set` ([examples/dispatch_set.cpp](examples/dispatch_set.cpp)):

```cpp
struct MatMul {
    template<int R, int C> int operator()(int a, int b) const { return R * a + C * b; }
};

using Shapes = poet::dispatch_set<int, poet::values<2, 2>, poet::values<4, 4>, poet::values<2, 4>>;
int v = poet::dispatch(MatMul{}, Shapes{ rows, cols }, 3, 5);
```

Worked examples: [`examples/polynomial.cpp`](examples/polynomial.cpp) (Horner's method,
degree picked at runtime via `dispatch`, unrolled with `static_for`) and
[`examples/dot_product.cpp`](examples/dot_product.cpp) (lane-aware `dynamic_for`
breaking a serial FMA chain into independent accumulators). All examples also run on
[Compiler Explorer](https://flatironinstitute.github.io/poet/static_for.html).

## Install

POET is header-only. In a local CMake project:

```cmake
add_subdirectory(path/to/poet)
target_link_libraries(my_app PRIVATE poet::poet)
```

As an installed package:

```cmake
find_package(poet CONFIG REQUIRED)
target_link_libraries(my_app PRIVATE poet::poet)
```

Via FetchContent:

```cmake
include(FetchContent)
FetchContent_Declare(poet GIT_REPOSITORY https://github.com/flatironinstitute/poet.git GIT_TAG main)
FetchContent_MakeAvailable(poet)
target_link_libraries(my_app PRIVATE poet::poet)
```

See [docs/install.rst](docs/install.rst) for non-CMake builds and version constants.

## Docs

- Guides and API docs: [flatironinstitute.github.io/poet/docs](https://flatironinstitute.github.io/poet/docs/)
- Guarantees and tests: [docs/guides/static_for.rst](docs/guides/static_for.rst),
  [docs/guides/dynamic_for.rst](docs/guides/dynamic_for.rst),
  [docs/guides/dispatch.rst](docs/guides/dispatch.rst),
  [docs/guides/cpu_info.rst](docs/guides/cpu_info.rst)
- Benchmarks: [docs/guides/benchmarks.rst](docs/guides/benchmarks.rst)
- API entry point: [include/poet/poet.hpp](include/poet/poet.hpp)

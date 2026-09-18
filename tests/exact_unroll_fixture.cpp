/// \file exact_unroll_fixture.cpp
/// \brief Compile-only fixture for exact_unroll_check: one `std::fma` per iteration.
///
/// The serial accumulator keeps the vectorizer out, so the FMAs in a function are
/// the bodies it emits. `opaque_count` hides every trip count, so a constant range
/// runs the same main loop as a runtime one and leaves nothing else to count.

#include <cmath>
#include <cstddef>

#include <poet/poet.hpp>

#define POET_EXACT_UNROLL_CASE(name, U, N)                                                               \
    extern "C" double name(const double *p, double acc) {                                                \
        poet::dynamic_for<(U), 1>(                                                                       \
          std::size_t{ 0 }, std::size_t{ (N) }, [&](std::size_t i) { acc = std::fma(acc, p[i], 1.0); }); \
        return acc;                                                                                      \
    }

// Many blocks, no tail.
POET_EXACT_UNROLL_CASE(u1_blocks, 1, 64)
POET_EXACT_UNROLL_CASE(u2_blocks, 2, 64)
POET_EXACT_UNROLL_CASE(u4_blocks, 4, 64)
POET_EXACT_UNROLL_CASE(u8_blocks, 8, 64)
// Two blocks plus a tail: a constant the complete unroller would peel.
POET_EXACT_UNROLL_CASE(u2_tail, 2, 5)
POET_EXACT_UNROLL_CASE(u4_tail, 4, 9)
// Exactly one block: straight-line code, no loop.
POET_EXACT_UNROLL_CASE(u2_one, 2, 2)
POET_EXACT_UNROLL_CASE(u4_one, 4, 4)
POET_EXACT_UNROLL_CASE(u8_one, 8, 8)

/// Positive control: a constant trip count every compiler unrolls under
/// `-funroll-loops` (apple-clang declines a runtime count there).
extern "C" double naked(const double *p, double acc) {
    for (std::size_t i = 0; i < 64; ++i) { acc = std::fma(acc, p[i], 1.0); }
    return acc;
}

// --- static_for, dispatch and dispatch_set codegen contracts ---

/// static_for<0,8> is a compile-time unroll: reuses the fma/branch counter above
/// to prove it too emits exactly 8 bodies and no loop.
extern "C" double static_for8(const double *p, double acc) {
    poet::static_for<0, 8>([&](auto i) { acc = std::fma(acc, p[static_cast<std::size_t>(decltype(i)::value)], 1.0); });
    return acc;
}

struct AddN {
    template<int N> int operator()(int x) const { return x + N; }
};

struct Add2 {
    template<int A, int B> int operator()(int x) const { return x + A + B; }
};

/// dispatch_param<inclusive_range<1,16>>, one-instruction body: contiguous table.
extern "C" int dispatch_contig16(int choice, int x) {
    return poet::dispatch(AddN{}, poet::dispatch_param<poet::inclusive_range<1, 16>>{ choice }, x);
}

/// Sparse, non-contiguous sequence: seq_lookup finds the slot, then the same table.
extern "C" int dispatch_sparse5(int choice, int x) {
    return poet::dispatch(AddN{}, poet::dispatch_param<std::integer_sequence<int, 1, 2, 4, 8, 16>>{ choice }, x);
}

/// Fixed-gap strided sequence: seq_lookup's exact-stride path, one compile-time-constant
/// reciprocal `imul`, still no `div`.
extern "C" int dispatch_strided8(int choice, int x) {
    return poet::dispatch(
      AddN{}, poet::dispatch_param<std::integer_sequence<int, 0, 3, 6, 9, 12, 15, 18, 21>>{ choice }, x);
}

/// 2D {1,2,4} x {1,2}: fused flat-index table, same contract.
extern "C" int dispatch_2d(int rows, int cols, int x) {
    auto params = std::make_tuple(poet::dispatch_param<std::integer_sequence<int, 1, 2, 4>>{ rows },
      poet::dispatch_param<std::integer_sequence<int, 1, 2>>{ cols });
    return poet::dispatch(Add2{}, params, x);
}

using DispatchSet4 =
  poet::dispatch_set<int, poet::values<1, 1>, poet::values<2, 2>, poet::values<3, 3>, poet::values<4, 4>>;
using DispatchSet12 = poet::dispatch_set<int,
  poet::values<1, 1>,
  poet::values<2, 2>,
  poet::values<3, 3>,
  poet::values<4, 4>,
  poet::values<5, 5>,
  poet::values<6, 6>,
  poet::values<7, 7>,
  poet::values<8, 8>,
  poet::values<9, 9>,
  poet::values<10, 10>,
  poet::values<11, 11>,
  poet::values<12, 12>>;

/// dispatch_set, 4 tuples: at dispatch_set_linear_max, linear fold.
extern "C" int dispatch_set4(int rows, int cols, int x) {
    return poet::dispatch(Add2{}, DispatchSet4{ rows, cols }, x);
}

/// dispatch_set, 12 tuples: above the linear threshold, sorted compare tree.
extern "C" int dispatch_set12(int rows, int cols, int x) {
    return poet::dispatch(Add2{}, DispatchSet12{ rows, cols }, x);
}

/// throw_on_no_match: the hit path must stay call-free besides the dispatch table's
/// own indirect call; a miss throws out of that path instead of inline.
extern "C" int dispatch_throw16(int choice, int x) {
    return poet::dispatch(
      poet::throw_on_no_match, AddN{}, poet::dispatch_param<poet::inclusive_range<1, 16>>{ choice }, x);
}

/// Positive control for the divide/multiply counter: `% 5` is not a power of two, so
/// every compiler lowers it to a magic-number `imul` at -O2/-O3, proving the counter
/// used above actually fires on divide-shaped code.
extern "C" int naive_mod_table(int v) {
    static constexpr int table[5] = { 10, 20, 30, 40, 50 };
    return table[v % 5];
}

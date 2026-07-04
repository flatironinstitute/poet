#include <poet/core/pipelined_for.hpp>

#include <catch2/catch_test_macros.hpp>

#include <array>
#include <cstddef>
#include <numeric>
#include <vector>

// ── compile-time sanity checks ────────────────────────────────────────────────

namespace {

// pipeline_depth always returns >= 1.
static_assert(poet::pipeline_depth(1) >= 1);
static_assert(poet::pipeline_depth(4) >= 1);

// pipeline_depth never exceeds latency * ports.
static_assert(poet::pipeline_depth(1, 4, 2) <= 8);
static_assert(poet::pipeline_depth(4, 4, 2) <= 8);

// live_regs_per_group larger than the register file clamps to 1.
static_assert(poet::pipeline_depth(1024) == 1);

// live_regs_per_group == 0 is handled gracefully.
static_assert(poet::pipeline_depth(0) == 1);

// With live=1, reg_budget == vector_register_count(); result is
// min(vector_register_count(), latency*ports).
static_assert(
  poet::pipeline_depth(1, 4, 2) == (poet::vector_register_count() < 8u ? poet::vector_register_count() : 8u));

}// namespace

// ── helper functor (member template requires namespace scope) ─────────────────

namespace {

/// Accumulates per-group partial sums using the integral_constant group tag
/// directly as an array subscript — the intended usage pattern.
template<std::size_t U> struct group_adder {
    std::array<int, U> sums{};

    template<std::size_t G> void operator()(int i, std::integral_constant<std::size_t, G> /*group*/) noexcept {
        sums[G] += i;
    }
};

}// namespace

// ── runtime tests ─────────────────────────────────────────────────────────────

TEST_CASE("pipelined_for sum equals serial sum — exact multiple of U", "[pipelined_for]") {
    // 20 elements, U=4 → 5 full groups, no tail.
    constexpr int kFirst = 0;
    constexpr int kLast = 20;
    constexpr std::size_t kU = 4;

    std::array<int, kU> partial{};
    partial.fill(0);

    poet::pipelined_for<kU>(kFirst, kLast, [&partial](int i, auto group) {
        partial[group] += i;// group used directly as index
    });

    const int total = std::accumulate(partial.begin(), partial.end(), 0);
    const int expected = (kLast - 1) * kLast / 2;// 0+1+…+19 = 190
    REQUIRE(total == expected);
}

TEST_CASE("pipelined_for tail handling — (last-first) % U != 0", "[pipelined_for]") {
    // 21 elements, U=4 → 5 full groups + 1 tail element.
    constexpr int kFirst = 0;
    constexpr int kLast = 21;
    constexpr std::size_t kU = 4;

    std::array<int, kU> partial{};
    partial.fill(0);

    poet::pipelined_for<kU>(kFirst, kLast, [&partial](int i, auto group) { partial[group] += i; });

    const int total = std::accumulate(partial.begin(), partial.end(), 0);
    const int expected = kLast * (kLast - 1) / 2;// 0+1+…+20 = 210
    REQUIRE(total == expected);
}

TEST_CASE("pipelined_for U=1 degenerate case visits every index in order", "[pipelined_for]") {
    constexpr int kFirst = 3;
    constexpr int kLast = 13;

    std::vector<int> visited;
    visited.reserve(static_cast<std::size_t>(kLast - kFirst));

    poet::pipelined_for<1>(kFirst, kLast, [&visited](int i, auto /*group*/) { visited.push_back(i); });

    REQUIRE(visited.size() == static_cast<std::size_t>(kLast - kFirst));
    for (int i = kFirst; i < kLast; ++i) { REQUIRE(visited[static_cast<std::size_t>(i - kFirst)] == i); }
}

TEST_CASE("pipelined_for preserves lvalue functor state", "[pipelined_for]") {
    // An lvalue functor must be mutated in-place (same contract as static_for).
    group_adder<4> adder;
    poet::pipelined_for<4>(0, 20, adder);

    const int total = std::accumulate(adder.sums.begin(), adder.sums.end(), 0);
    REQUIRE(total == 190);// 0+1+…+19
}

TEST_CASE("pipelined_for non-zero start offset is handled correctly", "[pipelined_for]") {
    // Range [5, 17), U=3 → 4 full groups + 0 tail.
    constexpr int kFirst = 5;
    constexpr int kLast = 17;
    constexpr std::size_t kU = 3;

    std::array<int, kU> partial{};
    partial.fill(0);

    poet::pipelined_for<kU>(kFirst, kLast, [&partial](int i, auto group) { partial[group] += i; });

    const int total = std::accumulate(partial.begin(), partial.end(), 0);
    // 5+6+…+16 = sum(1..16) - sum(1..4) = 136 - 10 = 126
    const int expected = (kFirst + kLast - 1) * (kLast - kFirst) / 2;
    REQUIRE(total == expected);
}

TEST_CASE("pipelined_for empty range performs no iterations", "[pipelined_for]") {
    int calls = 0;
    poet::pipelined_for<4>(7, 7, [&calls](int /*i*/, auto /*group*/) { ++calls; });
    REQUIRE(calls == 0);
}

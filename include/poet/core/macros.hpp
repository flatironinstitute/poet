#ifndef POET_CORE_MACROS_HPP
#define POET_CORE_MACROS_HPP

/// \file macros.hpp
/// \brief Compiler-specific macros for portability and optimization.

// ============================================================================
// POET_UNREACHABLE
// ============================================================================
/// Marks a code path as unreachable. UB if reached at runtime.
#if defined(__GNUC__) || defined(__clang__)
#define POET_UNREACHABLE() __builtin_unreachable()// NOLINT(cppcoreguidelines-macro-usage)
#elif defined(_MSC_VER)
#define POET_UNREACHABLE() __assume(false)// NOLINT(cppcoreguidelines-macro-usage)
#else
#define POET_UNREACHABLE() \
    do {                   \
    } while (false)// NOLINT(cppcoreguidelines-macro-usage)
#endif

// ============================================================================
// POET_FORCEINLINE
// ============================================================================
/// Forces function inlining regardless of compiler heuristics.
#ifdef _MSC_VER
#define POET_FORCEINLINE __forceinline
#elif defined(__GNUC__) || defined(__clang__)
#define POET_FORCEINLINE inline __attribute__((always_inline))
#else
#define POET_FORCEINLINE inline
#endif

// ============================================================================
// POET_ALWAYS_INLINE_LAMBDA
// ============================================================================
/// Forces inlining of lambda call operators. Place after the parameter list:
///
///   auto fn = [&](auto x) POET_ALWAYS_INLINE_LAMBDA { return x; };
///
/// Uses __attribute__((always_inline)) on GCC/Clang (the only syntax that
/// applies to the call operator) and [[msvc::forceinline]] on MSVC.
/// GCC 15+ / Clang 22+: attributed generic lambdas must be assigned to a
/// variable before passing to template functions.
#if defined(_MSC_VER) && !defined(__clang__)
#define POET_ALWAYS_INLINE_LAMBDA [[msvc::forceinline]]
#elif defined(__GNUC__) || defined(__clang__)
#define POET_ALWAYS_INLINE_LAMBDA __attribute__((always_inline))
#else
#define POET_ALWAYS_INLINE_LAMBDA
#endif

// ============================================================================
// POET_NOINLINE_FLATTEN
// ============================================================================
/// Prevents a function from being inlined into its caller (register isolation)
/// while forcing all functions it calls to be inlined into it.
///
/// `flatten` is what makes `noinline` usable on GCC: on its own, GCC's ISRA
/// pass extracts each functor `operator()` instantiation into an out-of-line
/// clone, so every call reloads the body's constants from .rodata. With
/// `flatten` the constants are hoisted into registers once at block entry.
/// Clang already inlines everything inside a noinline block.
#ifdef _MSC_VER
#define POET_NOINLINE_FLATTEN __declspec(noinline)
#elif defined(__GNUC__) || defined(__clang__)
#define POET_NOINLINE_FLATTEN __attribute__((noinline, flatten))
#else
#define POET_NOINLINE_FLATTEN
#endif

// ============================================================================
// POET_LIKELY / POET_UNLIKELY
// ============================================================================
/// Branch prediction hints. Use for conditions true/false >95% of the time.
#if defined(__GNUC__) || defined(__clang__)
#define POET_LIKELY(x) __builtin_expect(!!(x), 1)// NOLINT(cppcoreguidelines-macro-usage)
#define POET_UNLIKELY(x) __builtin_expect(!!(x), 0)// NOLINT(cppcoreguidelines-macro-usage)
#else
#define POET_LIKELY(x) (x)// NOLINT(cppcoreguidelines-macro-usage)
#define POET_UNLIKELY(x) (x)// NOLINT(cppcoreguidelines-macro-usage)
#endif

// ============================================================================
// poet::detail::count_trailing_zeros
// ============================================================================
/// Counts trailing zero bits of a std::size_t. UB if value is 0.
/// Guarded separately so it is defined only once even when macros.hpp is
/// re-included after undef_macros.hpp.
#ifndef POET_COUNT_TRAILING_ZEROS_DEFINED
#define POET_COUNT_TRAILING_ZEROS_DEFINED

#include <cstddef>

#if __cplusplus >= 202002L
#include <bit>
#elif defined(_MSC_VER)
#include <intrin.h>
#endif

namespace poet::detail {

#if __cplusplus >= 202002L

constexpr auto count_trailing_zeros(std::size_t value) noexcept -> unsigned int {
    return static_cast<unsigned int>(std::countr_zero(value));
}

#elif defined(__GNUC__) || defined(__clang__)

constexpr auto count_trailing_zeros(std::size_t value) noexcept -> unsigned int {
    static_assert(sizeof(std::size_t) <= sizeof(unsigned long long), "unsupported std::size_t width");
    if constexpr (sizeof(std::size_t) <= sizeof(unsigned int)) {
        return static_cast<unsigned int>(__builtin_ctz(static_cast<unsigned int>(value)));
    } else {
        return static_cast<unsigned int>(__builtin_ctzll(static_cast<unsigned long long>(value)));
    }
}

#elif defined(_MSC_VER)

inline auto count_trailing_zeros(std::size_t value) noexcept -> unsigned int {
    unsigned long index = 0;
#if defined(_WIN64)
    _BitScanForward64(&index, static_cast<unsigned __int64>(value));
#else
    _BitScanForward(&index, static_cast<unsigned long>(value));
#endif
    return static_cast<unsigned int>(index);
}

#else

/// Portable fallback: width-agnostic, so it is correct for any std::size_t.
/// Only reached on C++17 compilers that are neither GCC/Clang nor MSVC, and
/// only once per dynamic_for call with a non-constant power-of-two stride.
constexpr auto count_trailing_zeros(std::size_t value) noexcept -> unsigned int {
    unsigned int count = 0;
    while ((value & std::size_t{ 1 }) == 0) {
        value >>= 1;
        ++count;
    }
    return count;
}

#endif

}// namespace poet::detail

#endif// POET_COUNT_TRAILING_ZEROS_DEFINED

// ============================================================================
// Optimization level detection
// ============================================================================
#if defined(__OPTIMIZE__) && !defined(__OPTIMIZE_SIZE__)
#define POET_HIGH_OPTIMIZATION 1// NOLINT(cppcoreguidelines-macro-usage)
#elif defined(_MSC_VER) && !defined(_DEBUG) && defined(NDEBUG)
#define POET_HIGH_OPTIMIZATION 1// NOLINT(cppcoreguidelines-macro-usage)
#else
#define POET_HIGH_OPTIMIZATION 0// NOLINT(cppcoreguidelines-macro-usage)
#endif

// ============================================================================
// POET_HOT_LOOP
// ============================================================================
/// Marks hot-path functions for aggressive optimization and inlining.
#if defined(__GNUC__) || defined(__clang__)
#define POET_HOT_LOOP inline __attribute__((hot, always_inline))
#elif defined(_MSC_VER)
#define POET_HOT_LOOP __forceinline
#else
#define POET_HOT_LOOP inline
#endif

// ============================================================================
// POET_PUSH_OPTIMIZE / POET_POP_OPTIMIZE
// ============================================================================
/// GCC register-allocator tuning for hot paths. Wrap performance-critical
/// function groups in POET_PUSH_OPTIMIZE / POET_POP_OPTIMIZE pairs.
///
/// At -O3 (POET_HIGH_OPTIMIZATION=1) on GCC, enables IRA pressure flags
/// (-fira-hoist-pressure, -fno-ira-share-spill-slots, -frename-registers)
/// that improve register allocation in unrolled and isolated blocks.
/// Without -O3, promotes the section to -O3.
/// On MSVC, enables aggressive optimization (/Ogt).
/// On Clang and others: no-op (Clang cannot enable optimizations via pragma).
///
/// Opt-out via -DPOET_DISABLE_PUSH_OPTIMIZE to preserve custom flags.
#ifndef POET_DISABLE_PUSH_OPTIMIZE
#if defined(__GNUC__) && !defined(__clang__)
#if POET_HIGH_OPTIMIZATION
// At -O3: Apply IRA pressure tuning + semantic-interposition removal for hot paths.
// -fno-semantic-interposition: allow inlining/IPO across function boundaries
//   (GCC default assumes exported symbols may be LD_PRELOAD-interposed, which
//    blocks optimizations even within the same TU; safe for header-only POET).
// -fvect-cost-model=cheap: allow vectorization even when GCC's cost model is
//   uncertain (helps SLP-vectorize independent accumulator chains in static_for).
//
// Vector width: prefer the widest SIMD width enabled at compile time.
//   GCC 13/14 sometimes drop to 128-bit even with AVX2; -mprefer-vector-width
//   ensures hot paths use the full register width. On AArch64 SVE, -msve-vector-bits
//   locks the VL so the compiler can unroll without predication overhead.
//   Uses #pragma GCC target (not optimize) since these are machine flags.
//   Scoped to push/pop, so it does not affect user code outside POET internals.
//   On SSE-only x86 and NEON (fixed 128-bit): no target pragma needed.

// -- Internal: optimization flags common to all GCC hot paths
#define POET_PUSH_OPTIMIZE_BASE_                                                                              \
    _Pragma("GCC push_options") _Pragma("GCC optimize(\"-fira-hoist-pressure\")")                             \
      _Pragma("GCC optimize(\"-fno-ira-share-spill-slots\")") _Pragma("GCC optimize(\"-frename-registers\")") \
        _Pragma("GCC optimize(\"-fno-semantic-interposition\")") _Pragma("GCC optimize(\"-fvect-cost-model=cheap\")")

// -- Internal: target pragma for widest available vector width
#if defined(__AVX512F__)
#define POET_PUSH_VECTOR_WIDTH_ _Pragma("GCC target(\"prefer-vector-width=512\")")
#elif defined(__AVX2__) || defined(__AVX__)
#define POET_PUSH_VECTOR_WIDTH_ _Pragma("GCC target(\"prefer-vector-width=256\")")
#elif defined(__ARM_FEATURE_SVE_BITS) && __ARM_FEATURE_SVE_BITS > 0
// SVE with known VL (e.g. -msve-vector-bits=256): lock it for the hot path.
#define POET_PUSH_SVE_BITS_STR_(x) #x
#define POET_PUSH_SVE_BITS_VAL_(x) POET_PUSH_SVE_BITS_STR_(x)
#define POET_PUSH_VECTOR_WIDTH_ \
    _Pragma("GCC target(\"sve-vector-bits=" POET_PUSH_SVE_BITS_VAL_(__ARM_FEATURE_SVE_BITS) "\")")
#else
#define POET_PUSH_VECTOR_WIDTH_
#endif

#define POET_PUSH_OPTIMIZE POET_PUSH_OPTIMIZE_BASE_ POET_PUSH_VECTOR_WIDTH_
#define POET_POP_OPTIMIZE _Pragma("GCC pop_options")
#else
// Without -O3: Enable -O3 for this section
#define POET_PUSH_OPTIMIZE _Pragma("GCC push_options") _Pragma("GCC optimize(\"-O3\")")
#define POET_POP_OPTIMIZE _Pragma("GCC pop_options")
#endif
#elif defined(_MSC_VER)
// In Debug builds, /RTC1 (runtime checks) is incompatible with /O2.
// Only enable optimization pragma in non-debug MSVC builds.
#ifndef _DEBUG
#define POET_PUSH_OPTIMIZE __pragma(optimize("gt", on))
#define POET_POP_OPTIMIZE __pragma(optimize("", on))
#else
#define POET_PUSH_OPTIMIZE
#define POET_POP_OPTIMIZE
#endif
#else
// Clang and others: no-op (Clang can only disable opts, not enable)
#define POET_PUSH_OPTIMIZE
#define POET_POP_OPTIMIZE
#endif
#else
// User opted out: no-op to preserve their custom flags
#define POET_PUSH_OPTIMIZE
#define POET_POP_OPTIMIZE
#endif

// ============================================================================
// C++20 Feature Detection
// ============================================================================
/// Use `consteval` for C++20+, fallback to `constexpr` for C++17.
#if __cplusplus >= 202002L
#define POET_CPP20_CONSTEVAL consteval
#else
#define POET_CPP20_CONSTEVAL constexpr
#endif

#endif// POET_CORE_MACROS_HPP

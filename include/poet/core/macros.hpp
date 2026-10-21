#ifndef POET_CORE_MACROS_HPP
#define POET_CORE_MACROS_HPP

/// \file macros.hpp
/// \brief Compiler-specific macros for portability and optimization.

// --- POET_CPLUSPLUS ---
/// The language standard in effect. MSVC leaves `__cplusplus` at 199711L
/// unless `/Zc:__cplusplus` is passed, so testing it directly hides C++20
/// code paths from MSVC users.
#ifdef _MSVC_LANG
#define POET_CPLUSPLUS _MSVC_LANG// NOLINT(cppcoreguidelines-macro-usage)
#else
#define POET_CPLUSPLUS __cplusplus// NOLINT(cppcoreguidelines-macro-usage)
#endif

// --- POET_UNREACHABLE ---
/// Marks a code path as unreachable. UB if reached at runtime.
/// Prefers std::unreachable when the library probe says it exists (C++23
/// only on this matrix); gcc and clang expand it to __builtin_unreachable.
/// <utility> is included at the definition site so the probe does not
/// depend on transitive includes.
#include <utility>

#if defined(__cpp_lib_unreachable) && __cpp_lib_unreachable >= 202202L
#define POET_UNREACHABLE() std::unreachable()// NOLINT(cppcoreguidelines-macro-usage)
#elif defined(__GNUC__) || defined(__clang__)
#define POET_UNREACHABLE() __builtin_unreachable()// NOLINT(cppcoreguidelines-macro-usage)
#elif defined(_MSC_VER)
#define POET_UNREACHABLE() __assume(false)// NOLINT(cppcoreguidelines-macro-usage)
#else
#define POET_UNREACHABLE() \
    do {                   \
    } while (false)// NOLINT(cppcoreguidelines-macro-usage)
#endif

// --- POET_FORCEINLINE ---
/// Forces function inlining regardless of compiler heuristics.
#ifdef _MSC_VER
#define POET_FORCEINLINE __forceinline
#elif defined(__GNUC__) || defined(__clang__)
#define POET_FORCEINLINE inline __attribute__((always_inline))
#else
#define POET_FORCEINLINE inline
#endif

// --- POET_ALWAYS_INLINE_LAMBDA ---
/// Forces inlining of a lambda's call operator, after the parameter list.
/// Attribute syntax is the only form the call operator accepts.
/// GCC 15+ / Clang 22+: assign an attributed generic lambda to a variable
/// before passing it to a template function.
#if defined(_MSC_VER) && !defined(__clang__)
#define POET_ALWAYS_INLINE_LAMBDA [[msvc::forceinline]]
#elif defined(__GNUC__) || defined(__clang__)
#define POET_ALWAYS_INLINE_LAMBDA __attribute__((always_inline))
#else
#define POET_ALWAYS_INLINE_LAMBDA
#endif

// --- POET_NOINLINE_FLATTEN ---
/// Keeps a function out of its caller (register isolation) while inlining
/// everything it calls. Without `flatten`, GCC's ISRA pass clones each functor
/// `operator()` out of line, reloading its constants per call; clang needs none.
#ifdef _MSC_VER
#define POET_NOINLINE_FLATTEN __declspec(noinline)
#elif defined(__GNUC__) || defined(__clang__)
#define POET_NOINLINE_FLATTEN __attribute__((noinline, flatten))
#else
#define POET_NOINLINE_FLATTEN
#endif

// --- POET_LIKELY / POET_UNLIKELY ---
/// Branch prediction hints. Use for conditions true/false >95% of the time.
#if defined(__GNUC__) || defined(__clang__)
#define POET_LIKELY(x) __builtin_expect(!!(x), 1)// NOLINT(cppcoreguidelines-macro-usage)
#define POET_UNLIKELY(x) __builtin_expect(!!(x), 0)// NOLINT(cppcoreguidelines-macro-usage)
#else
#define POET_LIKELY(x) (x)// NOLINT(cppcoreguidelines-macro-usage)
#define POET_UNLIKELY(x) (x)// NOLINT(cppcoreguidelines-macro-usage)
#endif

// --- POET_IF_LIKELY / POET_IF_UNLIKELY ---
/// Statement-position branch hints as `if` headers: `POET_IF_UNLIKELY(c)`
/// stands for `if (c) [[unlikely]]`. At C++20+ the standard statement
/// attribute hints the branch; below it the condition carries the expression
/// builtin. Gated on the language level only: __has_cpp_attribute(likely)
/// answers 201803L already at C++17 on GCC 13/16 and Clang 23, where Clang
/// under -Wpedantic -Werror rejects the attribute as a C++20 extension.
#if POET_CPLUSPLUS >= 202002L
#define POET_IF_LIKELY(cond) if (cond) [[likely]]// NOLINT(cppcoreguidelines-macro-usage)
#define POET_IF_UNLIKELY(cond) if (cond) [[unlikely]]// NOLINT(cppcoreguidelines-macro-usage)
#else
#define POET_IF_LIKELY(cond) if (POET_LIKELY(cond))// NOLINT(cppcoreguidelines-macro-usage)
#define POET_IF_UNLIKELY(cond) if (POET_UNLIKELY(cond))// NOLINT(cppcoreguidelines-macro-usage)
#endif

// --- poet::detail::count_trailing_zeros ---
/// Counts trailing zero bits of a std::size_t. UB if value is 0.
/// Own guard: re-inclusion after undef_macros.hpp must define it only once.
#ifndef POET_COUNT_TRAILING_ZEROS_DEFINED
#define POET_COUNT_TRAILING_ZEROS_DEFINED

#include <cstddef>

#if POET_CPLUSPLUS >= 202002L
#include <bit>
#elif defined(_MSC_VER)
#include <intrin.h>
#endif

namespace poet::detail {

#if POET_CPLUSPLUS >= 202002L

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

/// Portable fallback, width-agnostic. Reached only on C++17 compilers other
/// than GCC/Clang/MSVC, once per `dynamic_for` call with a non-constant
/// power-of-two stride.
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

// --- Optimization level detection ---
#if defined(__OPTIMIZE__) && !defined(__OPTIMIZE_SIZE__)
#define POET_HIGH_OPTIMIZATION 1// NOLINT(cppcoreguidelines-macro-usage)
#elif defined(_MSC_VER) && !defined(_DEBUG) && defined(NDEBUG)
#define POET_HIGH_OPTIMIZATION 1// NOLINT(cppcoreguidelines-macro-usage)
#else
#define POET_HIGH_OPTIMIZATION 0// NOLINT(cppcoreguidelines-macro-usage)
#endif

// --- POET_HOT_LOOP ---
/// Marks hot-path functions for aggressive optimization and inlining.
#if defined(__GNUC__) || defined(__clang__)
#define POET_HOT_LOOP inline __attribute__((hot, always_inline))
#elif defined(_MSC_VER)
#define POET_HOT_LOOP __forceinline
#else
#define POET_HOT_LOOP inline
#endif

// --- POET_NO_UNROLL ---
/// Keeps the compiler from unrolling the loop that follows, `-funroll-loops`
/// included. Clang first: it also defines `__GNUC__`. MSVC has no such pragma.
#ifdef __clang__
#define POET_NO_UNROLL _Pragma("clang loop unroll(disable)")
#elif defined(__GNUC__)
#define POET_NO_UNROLL _Pragma("GCC unroll 1")
#else
#define POET_NO_UNROLL
#endif

// --- POET_IS_CONSTANT ---
/// True when the optimizer knows `x` after inlining. MSVC has no such query and
/// answers true, which leaves a runtime test in place.
#if defined(__GNUC__) || defined(__clang__)
#define POET_IS_CONSTANT(x) __builtin_constant_p(x)// NOLINT(cppcoreguidelines-macro-usage)
#else
#define POET_IS_CONSTANT(x) true// NOLINT(cppcoreguidelines-macro-usage)
#endif

// --- POET_PUSH_OPTIMIZE / POET_POP_OPTIMIZE ---
/// Register-allocator tuning for hot paths, in push/pop pairs. Active only
/// when the build already optimizes for speed; it never raises the
/// optimization level. MSVC gets /Ogt; clang cannot enable optimizations via
/// pragma. Opt out with -DPOET_DISABLE_PUSH_OPTIMIZE.
#ifndef POET_DISABLE_PUSH_OPTIMIZE
#if defined(__GNUC__) && !defined(__clang__)
#if POET_HIGH_OPTIMIZATION
// Cheap vector cost model lets SLP pack unrolled accumulators; without the width pin GCC 13/14 drop to 128-bit under
// AVX2. Width flags are machine flags: `target`, not `optimize`, scoped to the push/pop.
#define POET_PUSH_OPTIMIZE_BASE_                                                                              \
    _Pragma("GCC push_options") _Pragma("GCC optimize(\"-fira-hoist-pressure\")")                             \
      _Pragma("GCC optimize(\"-fno-ira-share-spill-slots\")") _Pragma("GCC optimize(\"-frename-registers\")") \
        _Pragma("GCC optimize(\"-fvect-cost-model=cheap\")")

#if defined(__AVX512F__)
#define POET_PUSH_VECTOR_WIDTH_ _Pragma("GCC target(\"prefer-vector-width=512\")")
#elif defined(__AVX2__) || defined(__AVX__)
#define POET_PUSH_VECTOR_WIDTH_ _Pragma("GCC target(\"prefer-vector-width=256\")")
#elif defined(__ARM_FEATURE_SVE_BITS) && __ARM_FEATURE_SVE_BITS > 0
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
#define POET_PUSH_OPTIMIZE
#define POET_POP_OPTIMIZE
#endif
#elif defined(_MSC_VER)
// /RTC1 is incompatible with /O2, so the optimize pragma applies to non-debug builds only.
#ifndef _DEBUG
#define POET_PUSH_OPTIMIZE __pragma(optimize("gt", on))
#define POET_POP_OPTIMIZE __pragma(optimize("", on))
#else
#define POET_PUSH_OPTIMIZE
#define POET_POP_OPTIMIZE
#endif
#else
#define POET_PUSH_OPTIMIZE
#define POET_POP_OPTIMIZE
#endif
#else
#define POET_PUSH_OPTIMIZE
#define POET_POP_OPTIMIZE
#endif

// --- C++20 Feature Detection ---
#if POET_CPLUSPLUS >= 202002L
#define POET_CPP20_CONSTEVAL consteval
#else
#define POET_CPP20_CONSTEVAL constexpr
#endif

// POET_DISPATCH_SET_INLINE_ — GCC 13/16 outline the dispatch_set match chain (constprop/ISRA clones);
// clang inlines it on its own, so the hint is GCC-only.
#if defined(__GNUC__) && !defined(__clang__)
#define POET_DISPATCH_SET_INLINE_ POET_FORCEINLINE// NOLINT(cppcoreguidelines-macro-usage)
#else
#define POET_DISPATCH_SET_INLINE_// NOLINT(cppcoreguidelines-macro-usage)
#endif

// POET_DISPATCH_ENTRY_INLINE_ — GCC 13 alone outlines the variadic entry:
// annotated, GCC 16 perturbs loop rotation (+1.2..+2.8% measured), clang 23 leaves the 1D thunks outlined.
#if defined(__GNUC__) && __GNUC__ == 13 && !defined(__clang__)
#define POET_DISPATCH_ENTRY_INLINE_ POET_FORCEINLINE// NOLINT(cppcoreguidelines-macro-usage)
#else
#define POET_DISPATCH_ENTRY_INLINE_// NOLINT(cppcoreguidelines-macro-usage)
#endif

#endif// POET_CORE_MACROS_HPP

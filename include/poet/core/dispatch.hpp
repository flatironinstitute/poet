#pragma once

/// \file dispatch.hpp
/// \brief Runtime-to-compile-time dispatch for integer choices and tuples.

#include <algorithm>
#include <array>
#include <cstddef>
#include <limits>
#include <optional>
#include <stdexcept>
#include <tuple>
#include <type_traits>
#include <utility>

#include <poet/core/macros.hpp>
#include <poet/core/mdspan_utils.hpp>

namespace poet {

/// \brief Concise tuple syntax for `dispatch_set`.
template<auto... Vs> struct values {};

namespace detail {

    /// Payload for void-returning dispatch, so a match is still an engaged optional.
    struct void_result {};

    template<typename T> using result_holder = std::optional<std::conditional_t<std::is_void_v<T>, void_result, T>>;

    template<typename Functor, typename ResultType, typename RuntimeTuple, typename... Args> struct seq_matcher;

    template<typename ValueType,
      ValueType... V,
      typename ResultType,
      typename RuntimeTuple,
      typename Functor,
      typename... Args>
    struct seq_matcher<std::integer_sequence<ValueType, V...>, ResultType, RuntimeTuple, Functor, Args...> {
        template<std::size_t... Idx, typename F>
        static auto
          impl(std::index_sequence<Idx...> /*idx_seq*/, const RuntimeTuple &runtime_tuple, F &&func, Args &&...args)
            -> result_holder<ResultType> {
            result_holder<ResultType> res;
            if (((std::get<Idx>(runtime_tuple) == V) && ...)) {
                // Prefer value form, per can_use_value_form's rule for dispatch_param.
                constexpr bool value_form =
                  std::is_invocable_v<F &, std::integral_constant<ValueType, V>..., Args &&...>;
                if constexpr (std::is_void_v<ResultType>) {
                    if constexpr (value_form) {
                        std::forward<F>(func)(std::integral_constant<ValueType, V>{}..., std::forward<Args>(args)...);
                    } else {
                        std::forward<F>(func).template operator()<V...>(std::forward<Args>(args)...);
                    }
                    res = void_result{};
                } else if constexpr (value_form) {
                    res = std::forward<F>(func)(std::integral_constant<ValueType, V>{}..., std::forward<Args>(args)...);
                } else {
                    res = std::forward<F>(func).template operator()<V...>(std::forward<Args>(args)...);
                }
            }
            return res;
        }

        template<typename F>
        static auto match_and_call(const RuntimeTuple &runtime_tuple, F &&func, Args &&...args)
          -> result_holder<ResultType> {
            return impl(std::make_index_sequence<sizeof...(V)>{},
              runtime_tuple,
              std::forward<F>(func),
              std::forward<Args>(args)...);
        }
    };

    /// Declaration-only: splits an `integer_sequence` into one single-value
    /// `integer_sequence` per element, so `dispatch_result_t` (which already prefers the
    /// value form and falls back to the template form) can compute a `dispatch_set`
    /// tuple's result type the same way it does for `dispatch_param`.
    template<typename ValueType, ValueType... V>
    auto split_seq(std::integer_sequence<ValueType, V...> /*seq*/)
      -> std::tuple<std::integer_sequence<ValueType, V>...>;

    template<typename V, V Start, V... Is>
    auto inclusive_range_impl(std::integer_sequence<V, Is...>)
      -> std::integer_sequence<V, static_cast<V>(Start + Is)...>;

}// namespace detail

/// \brief Inclusive integer sequence `[Start, End]`, in `Start`'s own value type.
template<auto Start, decltype(Start) End>
using inclusive_range = decltype(detail::inclusive_range_impl<decltype(Start), Start>(
  std::make_integer_sequence<decltype(Start), End - Start + 1>{}));

/// \brief Runtime value paired with the compile-time candidates to probe.
template<typename Seq> struct dispatch_param {
    using seq_type = Seq;
    /// The sequence's own value type, so brace-init rejects a narrowing runtime
    /// value instead of silently truncating.
    using value_type = typename Seq::value_type;
    value_type runtime_val;
};

namespace detail {
    template<typename T> struct is_dispatch_param : std::false_type {};
    template<typename Seq> struct is_dispatch_param<dispatch_param<Seq>> : std::true_type {};

    template<typename T> inline constexpr bool is_dispatch_param_v = is_dispatch_param<std::decay_t<T>>::value;

    template<typename T> struct is_dispatch_param_tuple : std::false_type {};

    template<typename... Ts>
    struct is_dispatch_param_tuple<std::tuple<Ts...>> : std::bool_constant<(is_dispatch_param_v<Ts> && ...)> {};

    template<typename T>
    inline constexpr bool is_dispatch_param_tuple_v = is_dispatch_param_tuple<std::decay_t<T>>::value;
}// namespace detail

namespace detail {

    template<typename Sequence> struct sequence_size;

    template<typename T, T... Values>
    struct sequence_size<std::integer_sequence<T, Values...>>
      : std::integral_constant<std::size_t, sizeof...(Values)> {};

    template<typename Sequence> struct sequence_first;

    template<typename V, V First, V... Rest>
    struct sequence_first<std::integer_sequence<V, First, Rest...>> : std::integral_constant<V, First> {};

    /// `high == low + 1`, phrased as a guarded difference so an unsigned value type
    /// cannot wrap a descending pair into a spurious unit step.
    template<typename V> constexpr auto steps_up(V low, V high) noexcept -> bool {
        return high > low && high - low == 1;
    }

    /// True when the values form a unit-stride run, ascending or descending.
    ///
    /// `seq_lookup` resolves runs by `position == distance from First`, so a
    /// span equal to the value count is not enough: a permutation such as
    /// `{2, 0, 1}` matches the span but not the positions.
    template<typename V, V... Values> POET_CPP20_CONSTEVAL auto is_unit_stride() noexcept -> bool {
        constexpr std::size_t count = sizeof...(Values);
        if constexpr (count < 2) {
            return true;
        } else {
            constexpr std::array<V, count> values = { Values... };
            constexpr bool ascending = steps_up(values[0], values[1]);
            if constexpr (!ascending && !steps_up(values[1], values[0])) {
                return false;
            } else {
                for (std::size_t i = 2; i < count; ++i) {
                    const bool unit =
                      ascending ? steps_up(values[i - 1], values[i]) : steps_up(values[i], values[i - 1]);
                    if (!unit) { return false; }
                }
                return true;
            }
        }
    }

    template<typename Seq> struct is_contiguous_sequence : std::false_type {};

    template<typename V, V First, V... Rest>
    struct is_contiguous_sequence<std::integer_sequence<V, First, Rest...>>
      : std::bool_constant<is_unit_stride<V, First, Rest...>()> {};

    template<typename Seq> struct sparse_index;

    template<typename V, V... Values> struct sparse_index<std::integer_sequence<V, Values...>> {
        static constexpr std::size_t value_count = sizeof...(Values);

        struct sorted_data_t {
            std::array<V, value_count> sorted_keys{};
            std::array<std::size_t, value_count> sorted_indices{};
        };

        // Insertion sort that carries original positions alongside keys, so the dispatch table
        // preserves user-declared slot order while lookups can use ordered search (binary/strided).
        static constexpr sorted_data_t sorted_data = []() constexpr -> sorted_data_t {
            sorted_data_t out{};
            out.sorted_keys = std::array<V, value_count>{ Values... };
            for (std::size_t i = 0; i < value_count; ++i) { out.sorted_indices[i] = i; }
            for (std::size_t i = 1; i < value_count; ++i) {
                const V current_key = out.sorted_keys[i];
                const std::size_t current_index = out.sorted_indices[i];
                std::size_t insert_pos = i;
                while (insert_pos > 0 && out.sorted_keys[insert_pos - 1] > current_key) {
                    out.sorted_keys[insert_pos] = out.sorted_keys[insert_pos - 1];
                    out.sorted_indices[insert_pos] = out.sorted_indices[insert_pos - 1];
                    --insert_pos;
                }
                out.sorted_keys[insert_pos] = current_key;
                out.sorted_indices[insert_pos] = current_index;
            }
            return out;
        }();

        static constexpr std::size_t unique_count = []() constexpr -> std::size_t {
            if constexpr (value_count == 0) { return 0; }
            std::size_t count = 1;
            for (std::size_t i = 1; i < value_count; ++i) {
                if (sorted_data.sorted_keys[i] != sorted_data.sorted_keys[i - 1]) { ++count; }
            }
            return count;
        }();

        static constexpr std::array<V, unique_count> keys = []() constexpr -> std::array<V, unique_count> {
            std::array<V, unique_count> out{};
            if constexpr (value_count > 0) {
                std::size_t out_i = 0;
                out[out_i++] = sorted_data.sorted_keys[0];
                for (std::size_t i = 1; i < value_count; ++i) {
                    if (sorted_data.sorted_keys[i] != sorted_data.sorted_keys[i - 1]) {
                        out[out_i++] = sorted_data.sorted_keys[i];
                    }
                }
            }
            return out;
        }();

        static constexpr std::array<std::size_t, unique_count> indices =
          []() constexpr -> std::array<std::size_t, unique_count> {
            std::array<std::size_t, unique_count> out{};
            if constexpr (value_count > 0) {
                std::size_t out_i = 0;
                out[out_i++] = sorted_data.sorted_indices[0];
                for (std::size_t i = 1; i < value_count; ++i) {
                    if (sorted_data.sorted_keys[i] != sorted_data.sorted_keys[i - 1]) {
                        out[out_i++] = sorted_data.sorted_indices[i];
                    }
                }
            }
            return out;
        }();
    };

    inline constexpr std::size_t dispatch_npos = static_cast<std::size_t>(-1);

    // Exact division by a compile-time stride: `Stride = 2^shift * odd` and `odd`'s inverse mod 2^digits recover
    // `diff / Stride` as shift + wrapping multiply. Types only: GCC keys local asm labels to a TU's functions.

    /// Trailing zeros of `M > 0` by splitting the digit count in half per
    /// recursion level: type-recursive, so instantiation always terminates.
    /// (value recursion on `M` would instantiate the discarded ternary arm).
    template<typename Wide, Wide M, unsigned int Digits> struct exact_stride_shift_ {
        static constexpr Wide low = M & static_cast<Wide>((1ULL << (Digits / 2)) - 1);
        static constexpr Wide high = static_cast<Wide>(M >> (Digits / 2));
        static constexpr unsigned int value = low != 0
                                                ? exact_stride_shift_<Wide, low, Digits / 2>::value
                                                : (Digits / 2) + exact_stride_shift_<Wide, high, Digits / 2>::value;
    };

    template<typename Wide, Wide M> struct exact_stride_shift_<Wide, M, 1> {
        static constexpr unsigned int value = (M & Wide{ 1 }) == 0 ? 1 : 0;
    };

    /// Modular inverse of an odd factor mod 2^64: `odd * odd == 1` (mod 8)
    /// seeds 3 correct bits and each Newton step `x * (2 - odd * x)` doubles
    /// them, so 5 steps cover 64 bits. Truncated to narrower widths at the
    /// call; an inverse mod 2^64 stays an inverse mod every smaller width.
    template<unsigned long long Odd, unsigned int Step> struct exact_stride_inverse_ {
        static constexpr unsigned long long prev = exact_stride_inverse_<Odd, Step - 1>::value;
        static constexpr unsigned long long value = prev * (2ULL - (Odd * prev));
    };

    template<unsigned long long Odd> struct exact_stride_inverse_<Odd, 0> {
        static constexpr unsigned long long value = Odd;
    };

    template<typename U, U Stride> struct exact_stride_division_ {
        static_assert(Stride > 0, "strided keys are sorted and unique, so the gap is positive");

        static constexpr unsigned int shift = exact_stride_shift_<U, Stride, std::numeric_limits<U>::digits>::value;

        /// `wide` is promotion-proof (unsigned int or wider): an 8/16-bit
        /// multiply would promote to `int` and could overflow it.
        using wide = std::
          conditional_t<(std::numeric_limits<U>::digits < std::numeric_limits<unsigned int>::digits), unsigned int, U>;

        static constexpr wide odd = static_cast<wide>(Stride) >> shift;
        static constexpr wide inverse =
          static_cast<wide>(exact_stride_inverse_<static_cast<unsigned long long>(odd), 5>::value);

        // Divide: `static_cast<U>((static_cast<wide>(diff) >> shift) * inverse)`.
        // Inlined at the call site so no helper body ever enters a TU.
    };

    /// Compile-time exactness proof at the boundary quotients: 0, 1, the
    /// middle, and both ends of the representable range.
    template<typename U, U Stride> struct exact_stride_proof_ {
        using op = exact_stride_division_<U, Stride>;
        static constexpr U kmax = static_cast<U>(std::numeric_limits<U>::max() / Stride);
        template<U K>
        static constexpr U dividend =
          static_cast<U>(static_cast<typename op::wide>(K) * static_cast<typename op::wide>(Stride));
        template<U K>
        static constexpr U quotient =
          static_cast<U>((static_cast<typename op::wide>(dividend<K>) >> op::shift) * op::inverse);
        // Probes stay `<= kmax` by construction (`Stride > 0` gives `kmax >= 1`): no constant-foldable comparison, so
        // MSVC C4296 cannot fire.
        template<U K> static constexpr bool holds_ = quotient<K> == K;
        static constexpr bool value = holds_<0> && holds_<1> && holds_<static_cast<U>(kmax / 2)>
                                      && holds_<static_cast<U>(2 * (kmax / 2))> && holds_<static_cast<U>(kmax - 1)>
                                      && holds_<kmax>;
    };

    template<typename U, U... Strides>
    inline constexpr bool exact_stride_proven_ = (exact_stride_proof_<U, Strides>::value && ...);

    // Unit, pure power-of-two, odd, mixed, half-range, sign-bit and saturating
    // strides, at every unsigned width the strided find arm supports.
    static_assert(exact_stride_proven_<unsigned char, 1, 2, 3, 5, 6, 10, 127, 128, 255>);
    static_assert(exact_stride_proven_<unsigned short, 1, 2, 3, 6, 10, 255, 0x7FFF, 0x8000, 0xFFFF>);
    static_assert(
      exact_stride_proven_<unsigned int, 1, 2, 3, 5, 6, 10, 0x55555555U, 0x7FFFFFFFU, 0x80000000U, 0xFFFFFFFFU>);
    static_assert(exact_stride_proven_<unsigned long long,
      1,
      2,
      3,
      5,
      10,
      0x5555555555555555ULL,
      0x7FFFFFFFFFFFFFFFULL,
      0x8000000000000000ULL,
      0xFFFFFFFFFFFFFFFFULL>);

    /// Maps a runtime value to its slot in `Seq`.
    ///
    /// `find` returns a slot in `[0, count)` on a hit and some value `>= count`
    /// on a miss, by contract rather than by a fixed sentinel: the contiguous
    /// finder's raw unsigned difference underflows out of range on its own, so
    /// a hit costs one subtraction and no select. Callers test `idx < count`.
    template<typename Seq, bool IsContiguous = is_contiguous_sequence<Seq>::value> struct seq_lookup;

    template<typename V, V... Values> struct seq_lookup<std::integer_sequence<V, Values...>, true> {
        static constexpr V first = sequence_first<std::integer_sequence<V, Values...>>::value;
        static constexpr std::size_t len = sizeof...(Values);
        static constexpr bool ascending = (first == std::min({ Values... }));

        static constexpr std::size_t count = len;

        static POET_FORCEINLINE auto find(V value) -> std::size_t {
            // Unsigned subtraction sends "below first" far above `count`, so the caller's
            // `idx < count` test covers underflow and overflow alike; the width must be the value type's own.
            using U = std::make_unsigned_t<V>;
            const auto lhs = static_cast<U>(ascending ? value : first);
            const auto rhs = static_cast<U>(ascending ? first : value);
            return static_cast<std::size_t>(static_cast<U>(lhs - rhs));
        }
    };

    template<typename V, V... Values> struct seq_lookup<std::integer_sequence<V, Values...>, false> {
        using sparse_data = sparse_index<std::integer_sequence<V, Values...>>;

        static constexpr bool is_strided = []() constexpr -> bool {
            if constexpr (sparse_data::unique_count < 2) {
                return false;
            } else {
                // Reject non-positive strides up front so `find` can use unsigned math.
                // Keys are sorted and unique, so the gap is positive in any value type.
                constexpr V stride0 = static_cast<V>(sparse_data::keys[1] - sparse_data::keys[0]);
                if constexpr (stride0 == 0) { return false; }
                // cppcheck-suppress syntaxError ; cppcheck cannot parse a loop inside if constexpr
                for (std::size_t i = 2; i < sparse_data::unique_count; ++i) {
                    if (static_cast<V>(sparse_data::keys[i] - sparse_data::keys[i - 1]) != stride0) { return false; }
                }
                return true;
            }
        }();

        static constexpr std::size_t count = sparse_data::value_count;

        /// `indices` is a permutation of `[0, count)`, but GCC and Clang cannot
        /// see that through the table load and re-check the bound the caller
        /// already applies. Stating the invariant drops the duplicate compare.
        static POET_FORCEINLINE auto bounded(std::size_t slot) -> std::size_t {
            if (slot >= count) { POET_UNREACHABLE(); }
            return slot;
        }

        static POET_FORCEINLINE auto find(V value) -> std::size_t {
            if constexpr (is_strided) {
                using U = std::make_unsigned_t<V>;
                static constexpr V first = sparse_data::keys[0];
                static constexpr V stride = static_cast<V>(sparse_data::keys[1] - sparse_data::keys[0]);
                // Unsigned wraps "below first" past the bound: both range ends collapse into the `slot >=` test.
                // `% stride` proves divisibility; inverse constants recover the slot as shift+multiply, no divide.
                using divop = exact_stride_division_<U, static_cast<U>(stride)>;
                const auto diff = static_cast<U>(static_cast<U>(value) - static_cast<U>(first));
                if (diff % static_cast<U>(stride) != 0) { return count; }
                const auto slot = static_cast<std::size_t>(
                  static_cast<U>((static_cast<typename divop::wide>(diff) >> divop::shift) * divop::inverse));
                if (slot >= sparse_data::unique_count) { return count; }
                return bounded(sparse_data::indices[slot]);
            } else {
                const auto pos = std::lower_bound(sparse_data::keys.begin(), sparse_data::keys.end(), value);
                if (pos == sparse_data::keys.end() || *pos != value) { return count; }
                return bounded(sparse_data::indices[static_cast<std::size_t>(pos - sparse_data::keys.begin())]);
            }
        }
    };

    template<typename ParamTuple, std::size_t... Idx>
    POET_CPP20_CONSTEVAL auto dimensions_of_impl(std::index_sequence<Idx...> /*idxs*/)
      -> std::array<std::size_t, sizeof...(Idx)> {
        using P = std::decay_t<ParamTuple>;
        return std::array<std::size_t, sizeof...(Idx)>{
            sequence_size<typename std::tuple_element_t<Idx, P>::seq_type>::value...
        };
    }

    template<typename ParamTuple>
    POET_CPP20_CONSTEVAL auto dimensions_of() -> std::array<std::size_t, std::tuple_size_v<std::decay_t<ParamTuple>>> {
        return dimensions_of_impl<ParamTuple>(std::make_index_sequence<std::tuple_size_v<std::decay_t<ParamTuple>>>{});
    }

    /// Row-major flat index of the runtime coordinate, or `dispatch_npos` on a miss.
    ///
    /// `seq_lookup::find` already specializes each per-dimension lookup, so one
    /// flattening path serves every sequence shape.
    template<typename ParamTuple, std::size_t... Idx>
    POET_FORCEINLINE auto flat_index(const ParamTuple &params, std::index_sequence<Idx...> /*idxs*/) -> std::size_t {
        using P = std::decay_t<ParamTuple>;
        constexpr auto strides = compute_strides(dimensions_of<P>());

        using lookup = std::tuple<seq_lookup<typename std::tuple_element_t<Idx, P>::seq_type>...>;

        const std::array<std::size_t, sizeof...(Idx)> found = { std::tuple_element_t<Idx, lookup>::find(
          std::get<Idx>(params).runtime_val)... };

        // Bitwise-AND fold, not logical: no per-dimension branch; the offset sums unconditionally — a miss discards it
        // anyway.
        const unsigned hit = ((static_cast<unsigned>(found[Idx] < std::tuple_element_t<Idx, lookup>::count)) & ...);
        const std::size_t flat = ((found[Idx] * strides[Idx]) + ...);

        return (hit != 0) ? flat : dispatch_npos;
    }

    template<typename ParamTuple> POET_FORCEINLINE auto extract_flat_index(const ParamTuple &params) -> std::size_t {
        return flat_index(params, std::make_index_sequence<std::tuple_size_v<std::decay_t<ParamTuple>>>{});
    }

    template<typename A, typename B> struct seq_equal;
    template<typename T, T... A, T... B>
    struct seq_equal<std::integer_sequence<T, A...>, std::integer_sequence<T, B...>>
      : std::bool_constant<((A == B) && ...)> {};

    template<typename... S> struct unique_helper;
    template<> struct unique_helper<> : std::true_type {};
    template<typename Head, typename... Rest>
    struct unique_helper<Head, Rest...>
      : std::bool_constant<(!(seq_equal<Head, Rest>::value || ...) && unique_helper<Rest...>::value)> {};

    /// Each `dispatch_param`'s `seq_type` (`Seq::value_type` requires `Seq` to
    /// be a sequence, not a tuple), collected into one tuple.
    template<typename Tuple, std::size_t... Indices>
    POET_CPP20_CONSTEVAL auto extract_sequences_impl(std::index_sequence<Indices...> /*idxs*/) {
        using TupleType = std::remove_reference_t<Tuple>;
        return std::make_tuple(typename std::tuple_element_t<Indices, TupleType>::seq_type{}...);
    }

    template<typename Tuple> POET_CPP20_CONSTEVAL auto extract_sequences() {
        using TupleType = std::remove_reference_t<Tuple>;
        return extract_sequences_impl<TupleType>(std::make_index_sequence<std::tuple_size_v<TupleType>>{});
    }

    template<typename Functor, typename... Seq> struct dispatch_result_helper {
        template<typename... Args>
        static auto compute_impl(std::true_type /*use_value_args*/)
          -> decltype(std::declval<Functor &>()(sequence_first<Seq>{}..., std::declval<Args>()...));

        template<typename... Args>
        static auto compute_impl(std::false_type /*use_value_args*/)
          -> decltype(std::declval<Functor &>().template operator()<sequence_first<Seq>::value...>(
            std::declval<Args>()...));

        template<typename... Args>
        static auto compute() -> decltype(compute_impl<Args...>(
          std::integral_constant<bool, std::is_invocable_v<Functor &, sequence_first<Seq>..., Args...>>{}));
    };

    template<typename Functor, typename SequenceTuple, typename... Args> struct dispatch_result;

    template<typename Functor, typename... Seq, typename... Args>
    struct dispatch_result<Functor, std::tuple<Seq...>, Args...> {
        using type = decltype(dispatch_result_helper<Functor, Seq...>::template compute<Args...>());
    };

    template<typename Functor, typename SequenceTuple, typename... Args>
    using dispatch_result_t = typename dispatch_result<Functor, SequenceTuple, Args...>::type;

    template<typename... Args> struct arg_pack {};

    template<typename T>
    inline constexpr bool is_stateless_v = std::is_empty_v<T> && std::is_default_constructible_v<T>;

    template<typename T> struct arg_pass {
        using raw = std::remove_reference_t<T>;
        using raw_unqual = std::remove_cv_t<raw>;
        static constexpr bool is_small_trivial =
          std::is_trivially_copyable_v<raw_unqual> && (sizeof(raw_unqual) <= 2 * sizeof(void *));

        static constexpr bool caller_allows_copy =
          std::is_rvalue_reference_v<T> || (std::is_lvalue_reference_v<T> && std::is_const_v<raw>);

        static constexpr bool by_value = is_small_trivial && caller_allows_copy;

        using type = std::conditional_t<by_value, raw_unqual, T>;
    };

    template<typename T> using pass_t = typename arg_pass<T>::type;

    /// `IC` is the `integral_constant` the functor would receive, value type included.
    template<typename Functor, typename IC, typename ArgPack> struct can_use_value_form : std::false_type {};

    template<typename Functor, typename IC, typename... Args>
    struct can_use_value_form<Functor, IC, arg_pack<Args...>>
      : std::bool_constant<std::is_invocable_v<Functor &, IC, Args &&...>> {};

    template<typename Functor, typename ArgPack, typename R, typename V, V... Values> struct table_builder;

    template<typename Functor, typename... Args, typename R, typename V, V... Values>
    struct table_builder<Functor, arg_pack<Args...>, R, V, Values...> {
        static constexpr V first_value = sequence_first<std::integer_sequence<V, Values...>>::value;

        template<V Value> static POET_FORCEINLINE auto call(Functor &func, pass_t<Args &&>... args) -> R {
            using ic = std::integral_constant<V, Value>;
            if constexpr (can_use_value_form<Functor, ic, arg_pack<Args...>>::value) {
                return func(ic{}, std::forward<Args>(args)...);
            } else {
                return func.template operator()<Value>(std::forward<Args>(args)...);
            }
        }

        // Stateless functors are default-constructed in the thunk, stateful arrive by reference: one signature per
        // entry. Two overloads, not `if constexpr` + two returns: nvcc reports that form as a missing return.
        template<V Value> static POET_CPP20_CONSTEVAL auto make_entry(std::true_type /*stateless*/) {
            return +[](pass_t<Args &&>... args) -> R {
                Functor func{};
                return call<Value>(func, std::forward<Args>(args)...);
            };
        }

        template<V Value> static POET_CPP20_CONSTEVAL auto make_entry(std::false_type /*stateless*/) {
            return +[](Functor &func, pass_t<Args &&>... args) -> R {
                return call<Value>(func, std::forward<Args>(args)...);
            };
        }

        using stateless_tag = std::bool_constant<is_stateless_v<Functor>>;

        template<V Value> static POET_CPP20_CONSTEVAL auto make_entry() { return make_entry<Value>(stateless_tag{}); }

        static POET_CPP20_CONSTEVAL auto make() {
            using fn_type = decltype(make_entry<first_value>());
            return std::array<fn_type, sizeof...(Values)>{ make_entry<Values>()... };
        }
    };

    template<typename Functor, typename ArgPack, typename R, typename V, V... Values>
    POET_CPP20_CONSTEVAL auto make_dispatch_table(std::integer_sequence<V, Values...> /*seq*/) {
        return table_builder<Functor, ArgPack, R, V, Values...>::make();
    }

    template<typename Functor, typename ArgPack, typename SeqTuple, typename IndexSeq> struct nd_table_builder;

    template<typename Functor, typename... Args, typename... Seqs, std::size_t... FlatIndices>
    struct nd_table_builder<Functor, arg_pack<Args...>, std::tuple<Seqs...>, std::index_sequence<FlatIndices...>> {

        static constexpr std::array<std::size_t, sizeof...(Seqs)> dims_ = { sequence_size<Seqs>::value... };
        static constexpr std::array<std::size_t, sizeof...(Seqs)> strides_ = compute_strides(dims_);

        template<std::size_t I, typename Seq> struct get_sequence_value;

        template<std::size_t I, typename V, V... Values>
        struct get_sequence_value<I, std::integer_sequence<V, Values...>> {
            static constexpr std::array<V, sizeof...(Values)> values = { Values... };
            static constexpr V value = values[I];
        };

        template<std::size_t FlatIdx, std::size_t DimIdx>
        static constexpr std::size_t dim_index_v = FlatIdx / strides_[DimIdx] % dims_[DimIdx];

        // Exposes each flat-index dimension as `ic<N>`, which the functor receives; distinct value types per dimension
        // exclude a shared array.
        template<std::size_t FlatIdx, std::size_t... SeqIdx> struct value_extractor {
            template<std::size_t N> using seq_at = std::tuple_element_t<N, std::tuple<Seqs...>>;

            template<std::size_t N>
            using ic = std::integral_constant<typename seq_at<N>::value_type,
              get_sequence_value<dim_index_v<FlatIdx, N>, seq_at<N>>::value>;
        };

        template<std::size_t FlatIdx> struct nd_index_caller {
            template<typename R, std::size_t... SeqIdx>
            static POET_FORCEINLINE auto invoke(Functor &func, std::index_sequence<SeqIdx...> /*idx*/, Args &&...args)
              -> R {
                using VE_local = value_extractor<FlatIdx, SeqIdx...>;
                constexpr bool use_value_form =
                  std::is_invocable_v<Functor &, typename VE_local::template ic<SeqIdx>..., Args &&...>;
                if constexpr (use_value_form) {
                    return func(typename VE_local::template ic<SeqIdx>{}..., std::forward<Args>(args)...);
                } else {
                    return func.template operator()<VE_local::template ic<SeqIdx>::value...>(
                      std::forward<Args>(args)...);
                }
            }

            template<typename R> static POET_FORCEINLINE auto call(Functor &func, pass_t<Args &&>... args) -> R {
                return invoke<R>(func, std::make_index_sequence<sizeof...(Seqs)>{}, std::forward<Args>(args)...);
            }

            template<typename R> static POET_FORCEINLINE auto call_stateless(pass_t<Args &&>... args) -> R {
                Functor func{};
                return invoke<R>(func, std::make_index_sequence<sizeof...(Seqs)>{}, std::forward<Args>(args)...);
            }
        };

        template<typename R> static constexpr auto make_table(std::true_type /*stateless*/) {
            using fn_type = decltype(&nd_index_caller<0>::template call_stateless<R>);
            return std::array<fn_type, sizeof...(FlatIndices)>{
                &nd_index_caller<FlatIndices>::template call_stateless<R>...
            };
        }

        template<typename R> static constexpr auto make_table(std::false_type /*stateless*/) {
            using fn_type = decltype(&nd_index_caller<0>::template call<R>);
            return std::array<fn_type, sizeof...(FlatIndices)>{ &nd_index_caller<FlatIndices>::template call<R>... };
        }

        template<typename R> static constexpr auto make_table() {
            return make_table<R>(std::bool_constant<is_stateless_v<Functor>>{});
        }
    };

    template<typename Functor, typename ArgPack, typename R, typename... Seqs>
    POET_CPP20_CONSTEVAL auto make_nd_dispatch_table(std::tuple<Seqs...> /*seqs*/) {
        constexpr std::size_t total_size = (sequence_size<Seqs>::value * ... * 1);
        return nd_table_builder<Functor, ArgPack, std::tuple<Seqs...>, std::make_index_sequence<total_size>>::
          template make_table<R>();
    }

}// namespace detail

/// \brief Exact set of allowed tuples for sparse dispatch.
///
/// Enumerates only the tuples that exist, unlike the cartesian product of
/// `dispatch_param`s. See docs/guides/dispatch.rst for the value-vs-template
/// form preference and the codegen contract.
///
/// \tparam ValueType Type every tuple element is converted to.
/// \tparam Tuples The allowed combinations, as `values<...>`. All must have the
///   same arity and be distinct.
template<typename ValueType, typename... Tuples> struct dispatch_set {
  private:
    template<typename TupleHelper> struct convert_tuple;

    template<auto... Vs> struct convert_tuple<values<Vs...>> {
        using type = std::integer_sequence<ValueType, static_cast<ValueType>(Vs)...>;
    };

  public:
    /// The allowed tuples as `std::integer_sequence`s; what `dispatch` matches against.
    using seq_type = std::tuple<typename convert_tuple<Tuples>::type...>;

    /// Number of values in every allowed tuple.
    static constexpr std::size_t tuple_arity = detail::sequence_size<std::tuple_element_t<0, seq_type>>::value;

  private:
    template<typename S> struct same_arity : std::bool_constant<detail::sequence_size<S>::value == tuple_arity> {};

    template<std::size_t... Idx> [[nodiscard]] auto runtime_tuple_impl(std::index_sequence<Idx...> /*idxs*/) const {
        return std::make_tuple(runtime_val[Idx]...);
    }

    std::array<ValueType, tuple_arity> runtime_val;

    static_assert(sizeof...(Tuples) >= 1, "dispatch_set requires at least one allowed tuple");

    static_assert((same_arity<typename convert_tuple<Tuples>::type>::value && ...),
      "All tuples in dispatch_set must have the same arity");

    static_assert(detail::unique_helper<typename convert_tuple<Tuples>::type...>::value,
      "dispatch_set contains duplicate allowed tuples");

  public:
    /// \brief Binds the runtime values to probe. Requires exactly `tuple_arity` of them.
    template<typename... Args, typename = std::enable_if_t<sizeof...(Args) == tuple_arity>>
    explicit dispatch_set(Args &&...args) : runtime_val{ static_cast<ValueType>(std::forward<Args>(args))... } {}

    [[nodiscard]] auto runtime_tuple() const { return runtime_tuple_impl(std::make_index_sequence<tuple_arity>{}); }
};

struct throw_on_no_match_t {};
inline constexpr throw_on_no_match_t throw_on_no_match{};

/// \brief Thrown when a `throw_on_no_match` dispatch has no matching specialization.
struct no_match_error : std::runtime_error {
    using std::runtime_error::runtime_error;
};

namespace detail {

    template<typename R, typename EntryFn, typename FunctorFwd, typename... Args>
    POET_FORCEINLINE auto invoke_table_entry(FunctorFwd &functor, EntryFn entry, Args &&...args) -> R {
        using FT = std::decay_t<FunctorFwd>;
        // `return <void expression>` is valid in a void function: no result-type split needed.
        if constexpr (is_stateless_v<FT>) {
            return entry(std::forward<Args>(args)...);
        } else {
            return entry(static_cast<FT &>(functor), std::forward<Args>(args)...);
        }
    }

    template<bool ThrowOnNoMatch, typename R, typename Functor, typename ParamTuple, typename... Args>
    POET_FORCEINLINE auto dispatch_1d(Functor &functor, ParamTuple const &params, Args &&...args) -> R {
        using FirstParam = std::tuple_element_t<0, std::remove_reference_t<ParamTuple>>;
        using Seq = typename FirstParam::seq_type;
        const auto runtime_val = std::get<0>(params).runtime_val;
        const std::size_t idx = seq_lookup<Seq>::find(runtime_val);

        if (idx < seq_lookup<Seq>::count) {
            using FunctorT = std::decay_t<Functor>;
            static constexpr auto table = make_dispatch_table<FunctorT, arg_pack<Args...>, R>(Seq{});
            return invoke_table_entry<R>(functor, table[idx], std::forward<Args>(args)...);
        }
        if constexpr (ThrowOnNoMatch) {
            throw no_match_error("poet::dispatch: no matching compile-time combination for runtime inputs");
        } else if constexpr (!std::is_void_v<R>) {
            return R{};
        }
    }

    template<bool ThrowOnNoMatch, typename R, typename Functor, typename ParamTuple, typename... Args>
    POET_FORCEINLINE auto dispatch_nd(Functor &functor, ParamTuple const &params, Args &&...args) -> R {
        const std::size_t flat_idx = extract_flat_index(params);
        POET_IF_LIKELY(flat_idx != dispatch_npos) {
            using FunctorT = std::decay_t<Functor>;
            static constexpr auto table =
              make_nd_dispatch_table<FunctorT, arg_pack<Args...>, R>(decltype(extract_sequences<ParamTuple>()){});
            return invoke_table_entry<R>(functor, table[flat_idx], std::forward<Args>(args)...);
        }
        if constexpr (ThrowOnNoMatch) {
            throw no_match_error("poet::dispatch: no matching compile-time combination for runtime inputs");
        } else if constexpr (!std::is_void_v<R>) {
            return R{};
        }
    }

    template<bool ThrowOnNoMatch, typename Functor, typename ParamTuple, typename... Args>
    POET_FORCEINLINE auto dispatch_impl(Functor &functor, ParamTuple const &params, Args &&...args) -> decltype(auto) {
        constexpr std::size_t param_count = std::tuple_size_v<std::remove_reference_t<ParamTuple>>;
        static_assert(param_count >= 1, "poet::dispatch requires at least one dispatch_param");
        using sequences_t = decltype(extract_sequences<ParamTuple>());
        using result_type = dispatch_result_t<Functor, sequences_t, Args &&...>;

        if constexpr (param_count == 1) {
            return dispatch_1d<ThrowOnNoMatch, result_type>(functor, params, std::forward<Args>(args)...);
        } else {
            return dispatch_nd<ThrowOnNoMatch, result_type>(functor, params, std::forward<Args>(args)...);
        }
    }

}// namespace detail

namespace detail {
    /// Count of `dispatch_param`s at the front of `Ts`, stopping at the first non-param.
    template<typename... Ts> struct leading_param_count : std::integral_constant<std::size_t, 0> {};

    template<typename First, typename... Rest>
    struct leading_param_count<First, Rest...>
      : std::integral_constant<std::size_t,
          is_dispatch_param_v<First> ? (1 + leading_param_count<Rest...>::value) : 0> {};

    template<bool ThrowOnNoMatch, typename Functor, std::size_t... ParamIdx, std::size_t... ArgIdx, typename... All>
    POET_FORCEINLINE auto dispatch_split_impl(Functor &functor,
      std::index_sequence<ParamIdx...> /*p*/,
      std::index_sequence<ArgIdx...> /*a*/,
      All &&...all) -> decltype(auto) {

        constexpr std::size_t num_params = sizeof...(ParamIdx);
        auto all_refs = std::forward_as_tuple(std::forward<All>(all)...);
        auto params = std::make_tuple(std::get<ParamIdx>(all_refs)...);

        // `std::move(all_refs)` moves only the tuple; the references inside keep
        // the remaining entries' value categories.
        return dispatch_impl<ThrowOnNoMatch>(functor,
          params,
          std::get<num_params + ArgIdx>(
            std::move(all_refs))...);// NOLINT(bugprone-use-after-move,hicpp-invalid-access-moved)
    }

    template<bool ThrowOnNoMatch, typename Functor, typename FirstParam, typename... Rest>
    POET_FORCEINLINE auto dispatch_variadic_impl(Functor &functor, FirstParam &&first_param, Rest &&...rest)
      -> decltype(auto) {
        constexpr std::size_t num_params = 1 + leading_param_count<Rest...>::value;
        constexpr std::size_t num_args = sizeof...(Rest) + 1 - num_params;

        if constexpr (num_args == 0) {
            auto params = std::make_tuple(std::forward<FirstParam>(first_param), std::forward<Rest>(rest)...);
            return dispatch_impl<ThrowOnNoMatch>(functor, params);
        } else {
            return dispatch_split_impl<ThrowOnNoMatch>(functor,
              std::make_index_sequence<num_params>{},
              std::make_index_sequence<num_args>{},
              std::forward<FirstParam>(first_param),
              std::forward<Rest>(rest)...);
        }
    }
}// namespace detail

namespace detail {
    template<typename T> struct is_dispatch_set : std::false_type {};
    template<typename ValueType, typename... Tuples>
    struct is_dispatch_set<dispatch_set<ValueType, Tuples...>> : std::true_type {};
    template<typename T> inline constexpr bool is_dispatch_set_v = is_dispatch_set<std::decay_t<T>>::value;

    /// True for anything `dispatch`'s leading argument accepts: a `dispatch_param`,
    /// a tuple of `dispatch_param`s, or a `dispatch_set`.
    template<typename T>
    inline constexpr bool is_dispatch_arg_v =
      is_dispatch_param_v<T> || is_dispatch_param_tuple_v<T> || is_dispatch_set_v<T>;
}// namespace detail

namespace detail {

    /// At or below this many allowed tuples, `dispatch_tuples_impl` keeps the
    /// declaration-order linear fold (a short merged branch chain small sets
    /// compile to); above it, `tuple_tree_matcher`'s O(log N) probe runs.
    inline constexpr std::size_t dispatch_set_linear_max = 8;

    /// An allowed tuple's values as one `std::array`, for constexpr tables.
    template<typename Seq> struct seq_to_array;
    template<typename V, V... Vs> struct seq_to_array<std::integer_sequence<V, Vs...>> {
        static constexpr std::array<V, sizeof...(Vs)> value = { Vs... };
    };

    /// Sorted balanced compare tree over the allowed tuples of a wide
    /// `dispatch_set`. The linear fold walks one compare chain per declared
    /// tuple; this tree visits ceil(log2 N) lexicographic pivots instead. The
    /// probe order differs from declaration order, which is immaterial
    /// because `dispatch_set` rejects duplicate tuples: exactly one candidate
    /// can match, so the same specialization is selected either way.
    template<typename R, typename TupleList, typename RuntimeTuple, typename Functor, typename... Args>
    struct tuple_tree_matcher {
        using TL = std::decay_t<TupleList>;
        using value_type = typename std::tuple_element_t<0, TL>::value_type;

        static constexpr std::size_t count = std::tuple_size_v<TL>;
        static constexpr std::size_t arity = sequence_size<std::tuple_element_t<0, TL>>::value;
        using key_type = std::array<value_type, arity>;
        using arity_indices = std::make_index_sequence<arity>;

        /// The allowed tuples, sorted lexicographically as plain values.
        static constexpr std::array<key_type, count> keys = []() constexpr -> std::array<key_type, count> {
            // std::array's relationals are not constexpr until C++20, so the
            // element-wise lexicographic compare is spelled out.
            std::array<key_type, count> sorted{};
            std::size_t out_idx = 0;
            std::apply(
              [&](const auto &...seqs) constexpr -> void {
                  ((sorted[out_idx++] = seq_to_array<std::decay_t<decltype(seqs)>>::value), ...);
              },
              TL{});
            for (std::size_t front = 1; front < count; ++front) {
                const key_type key = sorted[front];
                std::size_t pos = front;
                while (pos > 0) {
                    std::size_t dim = 0;
                    while (dim < arity && sorted[pos - 1][dim] == key[dim]) { ++dim; }
                    if (dim == arity || key[dim] > sorted[pos - 1][dim]) { break; }
                    sorted[pos] = sorted[pos - 1];
                    --pos;
                }
                sorted[pos] = key;
            }
            return sorted;
        }();

        /// Lexicographic less between the runtime tuple and `keys[Mid]`;
        /// `KeyLeft` picks the operand order. Short-circuits at the first
        /// component that differs.
        template<std::size_t Mid, bool KeyLeft, std::size_t J = 0>
        static POET_FORCEINLINE auto lex_less(const RuntimeTuple &runtime) -> bool {
            const value_type &lhs = KeyLeft ? keys[Mid][J] : std::get<J>(runtime);
            const value_type &rhs = KeyLeft ? std::get<J>(runtime) : keys[Mid][J];
            if constexpr (J + 1 == arity) {
                return lhs < rhs;
            } else {
                return (lhs != rhs) ? (lhs < rhs) : lex_less<Mid, KeyLeft, J + 1>(runtime);
            }
        }

        template<std::size_t I, std::size_t... J>
        static POET_FORCEINLINE auto call_key(std::index_sequence<J...> /*idxs*/, Functor &functor, Args &&...args)
          -> result_holder<R> {
            result_holder<R> res;
            // Prefer value form, per can_use_value_form's rule for dispatch_param.
            constexpr bool value_form =
              std::is_invocable_v<Functor &, std::integral_constant<value_type, keys[I][J]>..., Args &&...>;
            if constexpr (std::is_void_v<R>) {
                if constexpr (value_form) {
                    functor(std::integral_constant<value_type, keys[I][J]>{}..., std::forward<Args>(args)...);
                } else {
                    functor.template operator()<keys[I][J]...>(std::forward<Args>(args)...);
                }
                res = void_result{};
            } else if constexpr (value_form) {
                res = functor(std::integral_constant<value_type, keys[I][J]>{}..., std::forward<Args>(args)...);
            } else {
                res = functor.template operator()<keys[I][J]...>(std::forward<Args>(args)...);
            }
            return res;
        }

        template<std::size_t I, std::size_t... J>
        static POET_FORCEINLINE auto
          leaf_match(const RuntimeTuple &runtime, Functor &functor, std::index_sequence<J...> idxs, Args &&...args)
            -> result_holder<R> {
            result_holder<R> res;
            if (((std::get<J>(runtime) == keys[I][J]) && ...)) {
                res = call_key<I>(idxs, functor, std::forward<Args>(args)...);
            }
            return res;
        }

        /// Args are re-forwarded down the recursion; like the linear fold,
        /// only the single matched candidate ever consumes them.
        template<std::size_t Lo, std::size_t Hi>
        static POET_FORCEINLINE auto find_and_call([[maybe_unused]] const RuntimeTuple &runtime,
          [[maybe_unused]] Functor &functor,
          [[maybe_unused]] Args &&...args) -> result_holder<R> {
            if constexpr (Hi - Lo == 0) {
                return result_holder<R>{};
            } else if constexpr (Hi - Lo == 1) {
                return leaf_match<Lo>(runtime, functor, arity_indices{}, std::forward<Args>(args)...);
            } else {
                constexpr std::size_t mid = Lo + ((Hi - Lo) / 2);
                if (lex_less<mid, false>(runtime)) {
                    return find_and_call<Lo, mid>(runtime, functor, std::forward<Args>(args)...);
                }
                if (lex_less<mid, true>(runtime)) {
                    return find_and_call<mid + 1, Hi>(runtime, functor, std::forward<Args>(args)...);
                }
                return call_key<mid>(arity_indices{}, functor, std::forward<Args>(args)...);
            }
        }
    };

    template<bool ThrowOnNoMatch, typename Functor, typename TupleList, typename RuntimeTuple, typename... Args>
    auto dispatch_tuples_impl(Functor &functor,
      TupleList const & /*tl*/,
      const RuntimeTuple &runtime_tuple,
      Args &&...args)// NOLINT(cppcoreguidelines-missing-std-forward) forwarded inside short-circuiting fold
      -> decltype(auto) {
        using TL = std::decay_t<TupleList>;
        static_assert(std::tuple_size_v<TL> >= 1, "tuple list must contain at least one allowed tuple");

        using first_seq = std::tuple_element_t<0, TL>;
        using result_type =
          dispatch_result_t<std::decay_t<Functor>, decltype(split_seq(first_seq{})), std::decay_t<Args>...>;

        result_holder<result_type> out;

        // By reference, not by copy: a stateful functor's mutations must be
        // visible to the caller, exactly as on the dispatch_param path.
        using FunctorT = std::decay_t<Functor>;
        FunctorT &functor_ref = functor;

        bool matched = false;
        if constexpr (std::tuple_size_v<TL> <= dispatch_set_linear_max) {
            matched = std::apply(
              [&](auto... seqs) POET_ALWAYS_INLINE_LAMBDA -> bool {
                  return ([&](auto &seq) POET_ALWAYS_INLINE_LAMBDA -> bool {
                      using SeqType = std::decay_t<decltype(seq)>;
                      auto result = seq_matcher<SeqType, result_type, RuntimeTuple, FunctorT, Args...>::match_and_call(
                        runtime_tuple, functor_ref, std::forward<Args>(args)...);

                      if (result.has_value()) {
                          out = std::move(result);
                          return true;
                      }
                      return false;
                  }(seqs) || ...);
              },
              TL{});
        } else {
            out = tuple_tree_matcher<result_type, TL, RuntimeTuple, FunctorT, Args...>::template find_and_call<0,
              std::tuple_size_v<TL>>(runtime_tuple, functor_ref, std::forward<Args>(args)...);
            matched = out.has_value();
        }

        if (matched) {
            if constexpr (std::is_void_v<result_type>) {
                return;
            } else {
                return result_type(std::move(*out));
            }
        }
        if constexpr (ThrowOnNoMatch) {
            throw no_match_error("poet::dispatch: no matching compile-time tuple for runtime inputs");
        } else if constexpr (!std::is_void_v<result_type>) {
            return result_type{};
        }
    }

    /// Routes to the matching `*_impl<ThrowOnNoMatch>` by `First`'s shape: a leading
    /// `dispatch_param` (consecutive ones form a cartesian product), a tuple of
    /// `dispatch_param`s, or a `dispatch_set`.
    template<bool ThrowOnNoMatch, typename Functor, typename First, typename... Rest>
    auto dispatch_any(Functor &functor, First &&first, Rest &&...rest) -> decltype(auto) {
        if constexpr (is_dispatch_param_v<std::decay_t<First>>) {
            return dispatch_variadic_impl<ThrowOnNoMatch>(
              functor, std::forward<First>(first), std::forward<Rest>(rest)...);
        } else if constexpr (is_dispatch_param_tuple_v<std::decay_t<First>>) {
            return dispatch_impl<ThrowOnNoMatch>(functor, first, std::forward<Rest>(rest)...);
        } else {
            static_assert(is_dispatch_set_v<std::decay_t<First>>,
              "poet::dispatch: expected a dispatch_param, a tuple of dispatch_params, or a dispatch_set");
            return dispatch_tuples_impl<ThrowOnNoMatch>(
              functor, typename std::decay_t<First>::seq_type{}, first.runtime_tuple(), std::forward<Rest>(rest)...);
        }
    }

}// namespace detail

/// \brief Dispatches runtime integers to compile-time specializations.
///
/// Accepts a leading `dispatch_param` (consecutive ones form a cartesian product), a
/// tuple of `dispatch_param`s, or a `dispatch_set`, then forwards the rest to
/// `functor` untouched. See docs/guides/dispatch.rst for the value-vs-template form
/// preference and the no-match contract (silent by default; prefix with
/// `poet::throw_on_no_match` for a `no_match_error`).
///
/// \param functor The callable to specialize. Bound by reference, so a stateful
///   functor's mutations stay visible to the caller.
/// \param first First `dispatch_param`, tuple of `dispatch_param`s, or `dispatch_set`.
/// \param rest Further `dispatch_param`s (consecutive ones form a cartesian product;
///   only meaningful after a leading `dispatch_param`), then the arguments to forward.
template<typename Functor,
  typename First,
  typename... Rest,
  std::enable_if_t<detail::is_dispatch_arg_v<std::decay_t<First>>, int> = 0>
auto dispatch(Functor &&functor,// NOLINT(cppcoreguidelines-missing-std-forward): the impl binds the functor as an
                                // lvalue ref, so forwarding is a no-op.
  First &&first,
  Rest &&...rest) -> decltype(auto) {
    return detail::dispatch_any<false>(functor, std::forward<First>(first), std::forward<Rest>(rest)...);
}

/// \brief `poet::throw_on_no_match` entry point: same call syntax as the silent
/// overload above, with the tag prepended.
template<typename Functor, typename First, typename... Rest>
auto dispatch(throw_on_no_match_t /*tag*/,
  Functor &&functor,// NOLINT(cppcoreguidelines-missing-std-forward): the impl binds the functor as an lvalue ref, so
                    // forwarding is a no-op.
  First &&first,
  Rest &&...rest) -> decltype(auto) {
    return detail::dispatch_any<true>(functor, std::forward<First>(first), std::forward<Rest>(rest)...);
}

}// namespace poet

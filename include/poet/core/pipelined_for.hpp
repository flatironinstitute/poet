#pragma once

/// \file pipelined_for.hpp
/// \brief Runtime loop with U architecturally independent body instances for
///        latency hiding via software pipelining.

#include <cstddef>
#include <type_traits>
#include <utility>

#include <poet/core/cpu_info.hpp>
#include <poet/core/macros.hpp>

namespace poet {

namespace detail {

    /// \brief Expands one pipeline slot: invokes `body(base + Gs, IC<Gs>{})` for
    ///        each group index in the parameter pack in left-to-right order.
    template<typename Body, typename Index, std::size_t... Gs>
    POET_FORCEINLINE constexpr void pipelined_step(Body &body, Index base, std::index_sequence<Gs...> /*groups*/) {
        (body(base + static_cast<Index>(Gs), std::integral_constant<std::size_t, Gs>{}), ...);
    }

}// namespace detail

/// \brief Returns the recommended pipeline depth `U` for a kernel that uses
///        `live_regs_per_group` vector registers per independent chain.
///
/// The result is clamped so that:
///  - at least one chain is always produced (`U >= 1`),
///  - the total register demand stays within `vector_register_count()` (`U <=
///    floor(vector_register_count() / live_regs_per_group)`), and
///  - the depth does not exceed what is needed to hide a `latency`-cycle chain
///    on `ports` execution ports (`U <= latency * ports`).
///
/// \param live_regs_per_group  Live vector registers per pipeline group.
///                             Pass 0 to unconditionally receive `U = 1`.
/// \param latency              FMA / multiply chain latency in cycles (default 4).
/// \param ports                Independent execution ports (default 2).
/// \return Recommended unroll depth U.
// NOLINTNEXTLINE(bugprone-easily-swappable-parameters) — API-specified (live, latency, ports) order
[[nodiscard]] POET_CPP20_CONSTEVAL auto pipeline_depth(std::size_t live_regs_per_group,
  std::size_t latency = 4,
  std::size_t ports = 2) noexcept -> std::size_t {
    if (live_regs_per_group == 0) { return 1; }
    const std::size_t reg_budget = vector_register_count() / live_regs_per_group;
    const std::size_t latency_budget = latency * ports;
    if (reg_budget < 1) { return 1; }
    return reg_budget < latency_budget ? reg_budget : latency_budget;
}

/// \brief Iterates `[first, last)` round-robin across `U` architecturally
///        independent body instances.
///
/// Each of the `U` body instances owns disjoint accumulator state.  The
/// interleaving ensures the out-of-order window always sees at least `U`
/// independent dependency chains, hiding FMA / load latency.
///
/// The callable `body` is invoked as:
/// \code
///   body(index, std::integral_constant<std::size_t, group>)
/// \endcode
/// where `index` is the runtime element index and `group ∈ [0, U)` is a
/// compile-time `std::integral_constant`.  Because `std::integral_constant`
/// is implicitly convertible to its value type, the group tag can be used
/// directly as an array subscript without any `::value` cast:
/// \code
///   std::array<xsimd::batch<float>, U> acc{};
///   poet::pipelined_for<U>(0, n, [&](int i, auto group) {
///       acc[group] += data[i];   // 'group' used as index directly
///   });
/// \endcode
///
/// **Tail handling**: the remaining `(last - first) % U` iterations are run
/// sequentially with `group == 0`.
///
/// \tparam U     Pipeline depth (>= 1).  Use `pipeline_depth()` for a
///               consteval estimate derived from register count and FMA latency.
/// \tparam Index Integer-like range type (deduced).
/// \tparam Body  Callable type (deduced).
/// \param first  Inclusive start of the iteration range.
/// \param last   Exclusive end of the iteration range.
/// \param body   Callable as described above.
template<std::size_t U, typename Index, typename Body>
POET_FORCEINLINE constexpr void pipelined_for(Index first, Index last, Body &&body) {
    static_assert(U > 0, "pipelined_for requires U >= 1");

    using body_t = std::remove_reference_t<Body>;

    auto do_pipeline = [&](auto &ref) POET_ALWAYS_INLINE_LAMBDA -> void {
        const auto count = last - first;
        const auto full_groups = count / static_cast<decltype(count)>(U);
        const auto tail = count % static_cast<decltype(count)>(U);

        for (auto grp = decltype(count){}; grp < full_groups; ++grp) {
            const Index base = first + (static_cast<Index>(grp) * static_cast<Index>(U));
            detail::pipelined_step(ref, base, std::make_index_sequence<U>{});
        }

        // Tail: remaining iterations run single-instance (group = 0).
        const Index tail_base = first + (static_cast<Index>(full_groups) * static_cast<Index>(U));
        for (auto rem = decltype(tail){}; rem < tail; ++rem) {
            ref(tail_base + static_cast<Index>(rem), std::integral_constant<std::size_t, 0>{});
        }
    };

    if constexpr (std::is_lvalue_reference_v<Body>) {
        do_pipeline(body);
    } else {
        body_t callable(std::forward<Body>(body));
        do_pipeline(callable);
    }
}

}// namespace poet

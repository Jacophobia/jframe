// bestow/entt_compat.hpp
// EnTT compatibility header for MSVC C++23 modules
//
// When using 'import std;' with MSVC, ADL (Argument-Dependent Lookup) fails to find
// EnTT's iterator comparison operators. This header provides workarounds.
//
// Include this header instead of <entt/entt.hpp> in module global fragments.

#ifndef BESTOW_ENTT_COMPAT_HPP
#define BESTOW_ENTT_COMPAT_HPP

// Include standard headers first (before EnTT)
#include <vector>
#include <cstdint>

#include <entt/entt.hpp>

// MSVC with C++23 modules has ADL issues finding EnTT's iterator operators.
// The operators are template functions in entt::internal namespace, but when
// using 'import std;', ADL doesn't find them across module boundaries.
//
// Solution: Define explicit inline forwarding operators in the entt::internal
// namespace for our specific Entity type. This makes them visible to ADL.
#if defined(_MSC_VER)

// Define type aliases outside entt namespace to avoid conflicts
// CRITICAL: The container type must match EXACTLY what EnTT uses internally.
// EnTT uses std::vector<entt::entity, std::allocator<entt::entity>>
namespace bestow::detail {
    using BestowEntityContainer = std::vector<::entt::entity, std::allocator<::entt::entity>>;
    using BestowEntityIterator = ::entt::internal::sparse_set_iterator<BestowEntityContainer>;
}

// Define inline comparison operators in entt::internal namespace
// These explicitly forward to the template operators for our Entity type
namespace entt::internal {

    // Forward declare the iterator template
    template<typename Container>
    class sparse_set_iterator;

    // Forward declare the template operators (they exist in EnTT)
    template<typename Container>
    [[nodiscard]] constexpr bool operator==(const sparse_set_iterator<Container>&, const sparse_set_iterator<Container>&) noexcept;

    template<typename Container>
    [[nodiscard]] constexpr bool operator!=(const sparse_set_iterator<Container>&, const sparse_set_iterator<Container>&) noexcept;

    template<typename Container>
    [[nodiscard]] constexpr bool operator<(const sparse_set_iterator<Container>&, const sparse_set_iterator<Container>&) noexcept;

    template<typename Container>
    [[nodiscard]] constexpr bool operator>(const sparse_set_iterator<Container>&, const sparse_set_iterator<Container>&) noexcept;

    template<typename Container>
    [[nodiscard]] constexpr bool operator<=(const sparse_set_iterator<Container>&, const sparse_set_iterator<Container>&) noexcept;

    template<typename Container>
    [[nodiscard]] constexpr bool operator>=(const sparse_set_iterator<Container>&, const sparse_set_iterator<Container>&) noexcept;

    // Explicit overloads for Entity type that call the templates
    // Use the type aliases from bestow::detail to avoid naming conflicts
    inline bool operator==(const bestow::detail::BestowEntityIterator& lhs, const bestow::detail::BestowEntityIterator& rhs) noexcept {
        return operator==<bestow::detail::BestowEntityContainer>(lhs, rhs);
    }

    inline bool operator!=(const bestow::detail::BestowEntityIterator& lhs, const bestow::detail::BestowEntityIterator& rhs) noexcept {
        return operator!=<bestow::detail::BestowEntityContainer>(lhs, rhs);
    }

    inline bool operator<(const bestow::detail::BestowEntityIterator& lhs, const bestow::detail::BestowEntityIterator& rhs) noexcept {
        return operator< <bestow::detail::BestowEntityContainer>(lhs, rhs);
    }

    inline bool operator>(const bestow::detail::BestowEntityIterator& lhs, const bestow::detail::BestowEntityIterator& rhs) noexcept {
        return operator><bestow::detail::BestowEntityContainer>(lhs, rhs);
    }

    inline bool operator<=(const bestow::detail::BestowEntityIterator& lhs, const bestow::detail::BestowEntityIterator& rhs) noexcept {
        return operator<=<bestow::detail::BestowEntityContainer>(lhs, rhs);
    }

    inline bool operator>=(const bestow::detail::BestowEntityIterator& lhs, const bestow::detail::BestowEntityIterator& rhs) noexcept {
        return operator>=<bestow::detail::BestowEntityContainer>(lhs, rhs);
    }

} // namespace entt::internal

#endif // _MSC_VER

#endif // BESTOW_ENTT_COMPAT_HPP

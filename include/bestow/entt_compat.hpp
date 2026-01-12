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

// Define inline comparison operators in entt::internal namespace
// These explicitly forward to the template operators for our Entity type
namespace entt::internal {

    // Forward declare the iterator
    template<typename Container>
    class sparse_set_iterator;

    // Define explicit comparison operators for Entity iterators
    // These are non-template, concrete function overloads that forward to
    // EnTT's template operators. ADL will find these in the entt::internal namespace.
    using EntityContainer = std::vector<std::uint32_t>;
    using EntityIterator = sparse_set_iterator<EntityContainer>;

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
    inline bool operator==(const EntityIterator& lhs, const EntityIterator& rhs) noexcept {
        return operator==<EntityContainer>(lhs, rhs);
    }

    inline bool operator!=(const EntityIterator& lhs, const EntityIterator& rhs) noexcept {
        return operator!=<EntityContainer>(lhs, rhs);
    }

    inline bool operator<(const EntityIterator& lhs, const EntityIterator& rhs) noexcept {
        return operator< <EntityContainer>(lhs, rhs);
    }

    inline bool operator>(const EntityIterator& lhs, const EntityIterator& rhs) noexcept {
        return operator><EntityContainer>(lhs, rhs);
    }

    inline bool operator<=(const EntityIterator& lhs, const EntityIterator& rhs) noexcept {
        return operator<=<EntityContainer>(lhs, rhs);
    }

    inline bool operator>=(const EntityIterator& lhs, const EntityIterator& rhs) noexcept {
        return operator>=<EntityContainer>(lhs, rhs);
    }

} // namespace entt::internal

#endif // _MSC_VER

#endif // BESTOW_ENTT_COMPAT_HPP

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

    // Explicit overloads for Entity type
    // These concrete (non-template) overloads will be found by ADL when comparing
    // sparse_set_iterator<std::vector<entt::entity>> objects.
    //
    // CRITICAL: Don't explicitly specify template parameters in the operator calls!
    // Let template argument deduction figure it out from the parameter types.
    // Otherwise MSVC's template machinery will try to instantiate things with our type aliases.
    using BestowEntityIter = sparse_set_iterator<std::vector<::entt::entity, std::allocator<::entt::entity>>>;

    inline bool operator==(const BestowEntityIter& lhs, const BestowEntityIter& rhs) noexcept {
        // Call index() member function to compare iterator positions
        return lhs.index() == rhs.index();
    }

    inline bool operator!=(const BestowEntityIter& lhs, const BestowEntityIter& rhs) noexcept {
        return !(lhs == rhs);
    }

    inline bool operator<(const BestowEntityIter& lhs, const BestowEntityIter& rhs) noexcept {
        return lhs.index() > rhs.index(); // sparse_set is reverse iteration
    }

    inline bool operator>(const BestowEntityIter& lhs, const BestowEntityIter& rhs) noexcept {
        return rhs < lhs;
    }

    inline bool operator<=(const BestowEntityIter& lhs, const BestowEntityIter& rhs) noexcept {
        return !(rhs < lhs);
    }

    inline bool operator>=(const BestowEntityIter& lhs, const BestowEntityIter& rhs) noexcept {
        return !(lhs < rhs);
    }

} // namespace entt::internal

#endif // _MSC_VER

#endif // BESTOW_ENTT_COMPAT_HPP

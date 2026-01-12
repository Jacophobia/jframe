// bestow/entt_compat.hpp
// EnTT compatibility header for MSVC C++23 modules
//
// When using 'import std;' with MSVC, ADL (Argument-Dependent Lookup) fails to find
// EnTT's iterator comparison operators. This header provides workarounds.
//
// Include this header instead of <entt/entt.hpp> in module global fragments.

#ifndef BESTOW_ENTT_COMPAT_HPP
#define BESTOW_ENTT_COMPAT_HPP

#include <entt/entt.hpp>

// MSVC with C++23 modules has ADL issues finding EnTT's iterator operators.
// The operators are template functions in entt::internal namespace, but when
// using 'import std;', ADL doesn't find them across module boundaries.
//
// Additionally, template operators may not be instantiated properly across
// module boundaries, causing LNK2019 unresolved external symbol errors.
#if defined(_MSC_VER)

// Bring EnTT internal iterator operators into scope for ADL
// This fixes the "no operator != matches" error with sparse_set_iterator
namespace bestow::entt_compat {

// Import comparison operators from EnTT's internal namespace
using entt::internal::operator==;
using entt::internal::operator!=;
using entt::internal::operator<;
using entt::internal::operator>;
using entt::internal::operator<=;
using entt::internal::operator>=;

} // namespace bestow::entt_compat

// Bring operators to global scope for ADL to find them
using bestow::entt_compat::operator==;
using bestow::entt_compat::operator!=;
using bestow::entt_compat::operator<;
using bestow::entt_compat::operator>;
using bestow::entt_compat::operator<=;
using bestow::entt_compat::operator>=;

// Define comparison operators explicitly for sparse_set_iterator to fix MSVC ADL issues
// When using 'import std;', MSVC's ADL fails to find operators in entt::internal namespace
namespace entt::internal {
    // sparse_set_iterator comparison operators for MSVC
    template<typename Container>
    inline bool operator==(const sparse_set_iterator<Container>& lhs,
                          const sparse_set_iterator<Container>& rhs) noexcept {
        return lhs.index() == rhs.index();
    }

    template<typename Container>
    inline bool operator!=(const sparse_set_iterator<Container>& lhs,
                          const sparse_set_iterator<Container>& rhs) noexcept {
        return !(lhs == rhs);
    }
} // namespace entt::internal

#endif // _MSC_VER

#endif // BESTOW_ENTT_COMPAT_HPP

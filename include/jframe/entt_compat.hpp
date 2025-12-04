// jframe/entt_compat.hpp
// EnTT compatibility header for MSVC C++23 modules
//
// When using 'import std;' with MSVC, ADL (Argument-Dependent Lookup) fails to find
// EnTT's iterator comparison operators. This header provides workarounds.
//
// Include this header instead of <entt/entt.hpp> in module global fragments.

#ifndef JFRAME_ENTT_COMPAT_HPP
#define JFRAME_ENTT_COMPAT_HPP

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
namespace jframe::entt_compat {

// Import comparison operators from EnTT's internal namespace
using entt::internal::operator==;
using entt::internal::operator!=;
using entt::internal::operator<;
using entt::internal::operator>;
using entt::internal::operator<=;
using entt::internal::operator>=;

} // namespace jframe::entt_compat

// Bring operators to global scope for ADL to find them
using jframe::entt_compat::operator==;
using jframe::entt_compat::operator!=;
using jframe::entt_compat::operator<;
using jframe::entt_compat::operator>;
using jframe::entt_compat::operator<=;
using jframe::entt_compat::operator>=;

// Explicit template instantiation declarations/definitions for EnTT iterator operators.
// MSVC C++23 modules don't properly instantiate these template operators across
// module boundaries, causing LNK2019 errors. We force explicit instantiation here.
namespace entt::internal {

// Explicit instantiation for view_iterator operators used by basic_view
// The template parameters are: Type, Checked, Get, Exclude
// For single-component views: basic_sparse_set<entity>, false, 1, 0
using ViewIteratorType = entt::basic_sparse_set<entt::entity, std::allocator<entt::entity>>;

// Force instantiation by providing inline wrapper functions that use the operators
// This ensures the template code is compiled in every translation unit that includes this header
template<typename T, bool C, std::size_t G, std::size_t E>
[[maybe_unused]] inline bool force_view_iterator_eq(
    const view_iterator<T, C, G, E>& a,
    const view_iterator<T, C, G, E>& b) noexcept {
    return a == b;
}

template<typename T, bool C, std::size_t G, std::size_t E>
[[maybe_unused]] inline bool force_view_iterator_ne(
    const view_iterator<T, C, G, E>& a,
    const view_iterator<T, C, G, E>& b) noexcept {
    return a != b;
}

// Instantiate for common view configurations
// Single component view: <basic_sparse_set<entity>, false, 1, 0>
template bool force_view_iterator_eq<ViewIteratorType, false, 1, 0>(
    const view_iterator<ViewIteratorType, false, 1, 0>&,
    const view_iterator<ViewIteratorType, false, 1, 0>&) noexcept;
template bool force_view_iterator_ne<ViewIteratorType, false, 1, 0>(
    const view_iterator<ViewIteratorType, false, 1, 0>&,
    const view_iterator<ViewIteratorType, false, 1, 0>&) noexcept;

// Multi-component view: <basic_sparse_set<entity>, false, 2, 0>
template bool force_view_iterator_eq<ViewIteratorType, false, 2, 0>(
    const view_iterator<ViewIteratorType, false, 2, 0>&,
    const view_iterator<ViewIteratorType, false, 2, 0>&) noexcept;
template bool force_view_iterator_ne<ViewIteratorType, false, 2, 0>(
    const view_iterator<ViewIteratorType, false, 2, 0>&,
    const view_iterator<ViewIteratorType, false, 2, 0>&) noexcept;

// Three-component view: <basic_sparse_set<entity>, false, 3, 0>
template bool force_view_iterator_eq<ViewIteratorType, false, 3, 0>(
    const view_iterator<ViewIteratorType, false, 3, 0>&,
    const view_iterator<ViewIteratorType, false, 3, 0>&) noexcept;
template bool force_view_iterator_ne<ViewIteratorType, false, 3, 0>(
    const view_iterator<ViewIteratorType, false, 3, 0>&,
    const view_iterator<ViewIteratorType, false, 3, 0>&) noexcept;

// Force instantiation for sparse_set_iterator operators used when iterating sparse sets
// The template parameter is the container type, typically std::vector<entity>
template<typename Container>
[[maybe_unused]] inline bool force_sparse_set_iterator_eq(
    const sparse_set_iterator<Container>& a,
    const sparse_set_iterator<Container>& b) noexcept {
    return a == b;
}

template<typename Container>
[[maybe_unused]] inline bool force_sparse_set_iterator_ne(
    const sparse_set_iterator<Container>& a,
    const sparse_set_iterator<Container>& b) noexcept {
    return a != b;
}

// Instantiate for common sparse_set_iterator configurations
// sparse_set_iterator uses a reference container (std::vector<entity>&) wrapper
using SparseSetContainer = std::vector<entt::entity, std::allocator<entt::entity>>;
template bool force_sparse_set_iterator_eq<SparseSetContainer>(
    const sparse_set_iterator<SparseSetContainer>&,
    const sparse_set_iterator<SparseSetContainer>&) noexcept;
template bool force_sparse_set_iterator_ne<SparseSetContainer>(
    const sparse_set_iterator<SparseSetContainer>&,
    const sparse_set_iterator<SparseSetContainer>&) noexcept;

} // namespace entt::internal

#endif // _MSC_VER

#endif // JFRAME_ENTT_COMPAT_HPP

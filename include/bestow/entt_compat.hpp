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
// Solution: Explicitly instantiate the comparison operators for the types we use.
// This forces MSVC to generate the template instantiations so ADL can find them.
#if defined(_MSC_VER)

#include <vector>
#include <cstdint>

// Forward declare the Entity type that EnTT uses
namespace bestow {
    using Entity = std::uint32_t;
}

// Explicitly instantiate EnTT's iterator comparison operators for our Entity type
// This ensures MSVC generates these template instantiations, making them visible to ADL
namespace entt::internal {

    // Forward declare the iterator template
    template<typename Container>
    class sparse_set_iterator;

    // Explicitly instantiate comparison operators for Entity container
    template class sparse_set_iterator<std::vector<bestow::Entity>>;

    // Explicitly force instantiation of comparison operators
    template bool operator==(const sparse_set_iterator<std::vector<bestow::Entity>>&,
                            const sparse_set_iterator<std::vector<bestow::Entity>>&);
    template bool operator!=(const sparse_set_iterator<std::vector<bestow::Entity>>&,
                            const sparse_set_iterator<std::vector<bestow::Entity>>&);
    template bool operator<(const sparse_set_iterator<std::vector<bestow::Entity>>&,
                           const sparse_set_iterator<std::vector<bestow::Entity>>&);
    template bool operator>(const sparse_set_iterator<std::vector<bestow::Entity>>&,
                           const sparse_set_iterator<std::vector<bestow::Entity>>&);
    template bool operator<=(const sparse_set_iterator<std::vector<bestow::Entity>>&,
                            const sparse_set_iterator<std::vector<bestow::Entity>>&);
    template bool operator>=(const sparse_set_iterator<std::vector<bestow::Entity>>&,
                            const sparse_set_iterator<std::vector<bestow::Entity>>&);

} // namespace entt::internal

#endif // _MSC_VER

#endif // BESTOW_ENTT_COMPAT_HPP

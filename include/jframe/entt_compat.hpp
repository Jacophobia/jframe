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
// We provide explicit using declarations to bring them into scope.
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
using entt::internal::operator+;
using entt::internal::operator-;

} // namespace jframe::entt_compat

// Bring operators to global scope for ADL to find them
using jframe::entt_compat::operator==;
using jframe::entt_compat::operator!=;
using jframe::entt_compat::operator<;
using jframe::entt_compat::operator>;
using jframe::entt_compat::operator<=;
using jframe::entt_compat::operator>=;
using jframe::entt_compat::operator+;
using jframe::entt_compat::operator-;

#endif // _MSC_VER

#endif // JFRAME_ENTT_COMPAT_HPP

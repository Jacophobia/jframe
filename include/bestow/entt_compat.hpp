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
// Note: This is a known MSVC bug with C++23 modules and ADL. Workaround is to
// use /std:c++20 instead of /std:c++latest to avoid the stricter C++23 ADL rules.
#if defined(_MSC_VER)

// Bring EnTT internal iterator operators into scope for ADL
// This should work with /std:c++20 flag
namespace bestow::entt_compat {

using entt::internal::operator==;
using entt::internal::operator!=;
using entt::internal::operator<;
using entt::internal::operator>;
using entt::internal::operator<=;
using entt::internal::operator>=;

} // namespace bestow::entt_compat

// Bring operators to global scope
using bestow::entt_compat::operator==;
using bestow::entt_compat::operator!=;
using bestow::entt_compat::operator<;
using bestow::entt_compat::operator>;
using bestow::entt_compat::operator<=;
using bestow::entt_compat::operator>=;

#endif // _MSC_VER

#endif // BESTOW_ENTT_COMPAT_HPP

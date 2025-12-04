// jframe/sol2_compat.hpp
// sol2 compatibility header for MSVC C++23 modules
//
// When using 'import std;' with MSVC and C++23 modules, sol2's global constants
// (sol::in_place, sol::detail::not_enough_stack_space_*, etc.) may not be
// instantiated properly. Including this header in executables that use sol2
// through JFrame modules ensures these symbols are defined.
//
// Include this header in the global module fragment of main.cpp files that
// import modules using sol2 (jframe.config, jframe.level, jframe.blueprints, etc.)

#ifndef JFRAME_SOL2_COMPAT_HPP
#define JFRAME_SOL2_COMPAT_HPP

// Force inclusion of sol2 to instantiate global constants
#define SOL_ALL_SAFETIES_ON 1
#include <sol/sol.hpp>

// On MSVC with C++23 modules, ensure sol2 globals are properly instantiated
#if defined(_MSC_VER)

// Force instantiation of sol2 global constants that may not be exported
// properly through C++23 modules. These are inline variables in sol2 but
// the module system may not propagate them correctly.
namespace jframe::sol2_compat {

// Reference sol2 globals to ensure they're instantiated in this TU
[[maybe_unused]] static const auto& ensure_in_place = sol::in_place;
[[maybe_unused]] static const auto& ensure_not_enough_stack_space_generic =
    sol::detail::not_enough_stack_space_generic;
[[maybe_unused]] static const auto& ensure_not_enough_stack_space_string =
    sol::detail::not_enough_stack_space_string;

} // namespace jframe::sol2_compat

#endif // _MSC_VER

#endif // JFRAME_SOL2_COMPAT_HPP

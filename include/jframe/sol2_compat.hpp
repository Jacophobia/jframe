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

// MSVC C++23 modules have issues with constexpr variables from headers.
// The sol2 library defines these as constexpr which gives them internal linkage,
// but when modules reference them, MSVC generates extern references that don't
// link properly.
//
// Solution: Provide inline constexpr definitions. The 'inline' keyword in C++17
// gives variables external linkage and ensures there's exactly one definition
// across all translation units.

namespace sol {
    // Provide inline constexpr definitions for sol2 globals
    // These match the declarations in sol/in_place.hpp but with inline for external linkage
    inline constexpr std::in_place_t in_place {};

    namespace detail {
        // These match the declarations in sol/error_handler.hpp
        inline constexpr const char* not_enough_stack_space_generic =
            "not enough space left on Lua stack to push valuees";
        inline constexpr const char* not_enough_stack_space_string =
            "not enough space left on Lua stack for a string";
    }
}

#endif // _MSC_VER

#endif // JFRAME_SOL2_COMPAT_HPP

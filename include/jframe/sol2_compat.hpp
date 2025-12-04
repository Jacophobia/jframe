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

// On MSVC with C++23 modules, we need to ensure sol2 globals have external linkage.
// The sol2 library defines constexpr variables which have internal linkage by default,
// but when modules reference them, MSVC generates extern references that don't link.
//
// Solution: Define these variables with external linkage BEFORE including sol2,
// then skip sol2's own definitions by defining guards.
#if defined(_MSC_VER)

// Provide external linkage definitions for sol2 globals BEFORE sol2 defines them
#include <utility>  // for std::in_place_t

// Pre-define the sol2 namespace with inline constexpr versions
namespace sol {
    // Provide external linkage version of in_place
    // The inline keyword gives external linkage in C++17+
    inline constexpr std::in_place_t in_place {};
    inline constexpr std::in_place_t in_place_of {};

    namespace detail {
        // Error message strings from sol2's error_handler.hpp
        inline constexpr const char* not_enough_stack_space = "not enough space left on Lua stack";
        inline constexpr const char* not_enough_stack_space_floating = "not enough space left on Lua stack for a floating point number";
        inline constexpr const char* not_enough_stack_space_integral = "not enough space left on Lua stack for an integral number";
        inline constexpr const char* not_enough_stack_space_string = "not enough space left on Lua stack for a string";
        inline constexpr const char* not_enough_stack_space_meta_function_name = "not enough space left on Lua stack for the name of a meta_function";
        inline constexpr const char* not_enough_stack_space_userdata = "not enough space left on Lua stack to create a sol2 userdata";
        inline constexpr const char* not_enough_stack_space_generic = "not enough space left on Lua stack to push valuees";
        inline constexpr const char* not_enough_stack_space_environment = "not enough space left on Lua stack to retrieve environment";
        inline constexpr const char* protected_function_error = "caught (...) unknown error during protected_function call";
    }
}

// Tell sol2 to skip its own in_place definition since we've provided one
#define SOL_IN_PLACE_HPP

#endif // _MSC_VER

// Now include sol2
#define SOL_ALL_SAFETIES_ON 1
#include <sol/sol.hpp>

#endif // JFRAME_SOL2_COMPAT_HPP

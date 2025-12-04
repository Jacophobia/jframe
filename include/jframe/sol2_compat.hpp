// jframe/sol2_compat.hpp
// sol2 compatibility header for MSVC C++23 modules
//
// When using 'import std;' with MSVC and C++23 modules, sol2's global constants
// (sol::in_place, sol::detail::not_enough_stack_space_*, etc.) have internal
// linkage (constexpr), but MSVC modules incorrectly generate external references.
//
// KNOWN MSVC BUG: C++23 modules with 'import std;' cause linker errors for
// constexpr variables from third-party headers included in the global fragment.
//
// WORKAROUND: Use sol2_compat.hpp instead of <sol/sol.hpp> in ALL files that
// need sol2, including module global fragments. This header provides workarounds
// for MSVC while being transparent on other compilers.

#ifndef JFRAME_SOL2_COMPAT_HPP
#define JFRAME_SOL2_COMPAT_HPP

#define SOL_ALL_SAFETIES_ON 1

// On MSVC, we redefine the problematic constexpr variables with inline constexpr
// for external linkage BEFORE sol2 includes them, using include guards to skip
// sol2's own definitions.
#if defined(_MSC_VER)

// Include required standard headers
#include <cstddef>
#include <utility>

// Pre-declare sol namespace with inline constexpr definitions for external linkage
namespace sol {
    // in_place variables from sol/in_place.hpp
    using in_place_t = std::in_place_t;
    inline constexpr std::in_place_t in_place {};
    inline constexpr std::in_place_t in_place_of {};

    template <typename T>
    using in_place_type_t = std::in_place_type_t<T>;
    template <typename T>
    inline constexpr std::in_place_type_t<T> in_place_type {};

    template <size_t I>
    using in_place_index_t = std::in_place_index_t<I>;
    template <size_t I>
    inline constexpr in_place_index_t<I> in_place_index {};

    // Error message strings from sol/error_handler.hpp
    // These need inline constexpr for external linkage with MSVC modules
    namespace detail {
        inline constexpr const char* not_a_number = "not a numeric type";
        inline constexpr const char* not_a_number_or_number_string = "not a numeric type or numeric string";
        inline constexpr const char* not_a_number_integral = "not a numeric type that fits exactly an integer (number maybe has significant decimals)";
        inline constexpr const char* not_a_number_or_number_string_integral
             = "not a numeric type or a numeric string that fits exactly an integer (e.g. number maybe has significant decimals)";

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

// Define sol2's include guard so it doesn't redefine in_place variables
#define SOL_IN_PLACE_HPP

#endif // _MSC_VER

// Now include sol2 - on MSVC it will skip sol/in_place.hpp
#include <sol/sol.hpp>

#endif // JFRAME_SOL2_COMPAT_HPP

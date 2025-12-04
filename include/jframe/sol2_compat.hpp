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
#include <cstdio>
#include <string>

// Define sol2's include guards BEFORE including any sol headers
// This prevents sol from defining its constexpr variables with internal linkage
#define SOL_IN_PLACE_HPP
#define SOL_ERROR_HANDLER_HPP

// Include sol2's dependencies for error_handler.hpp
#include <sol/config.hpp>
#include <sol/types.hpp>
#include <sol/demangle.hpp>

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

        inline void accumulate_and_mark(const std::string& n, std::string& aux_message, int& marker) {
            if (marker > 0) {
                aux_message += ", ";
            }
            aux_message += n;
            ++marker;
        }
    } // namespace detail

    inline std::string associated_type_name(lua_State* L, int index, type t) {
        switch (t) {
        case type::poly:
            return "anything";
        case type::userdata: {
#if SOL_IS_ON(SOL_SAFE_STACK_CHECK)
            luaL_checkstack(L, 2, "not enough space to push get the type name");
#endif
            if (lua_getmetatable(L, index) == 0) {
                break;
            }
            lua_pushlstring(L, "__name", 6);
            lua_rawget(L, -2);
            size_t sz;
            const char* name = lua_tolstring(L, -1, &sz);
            std::string tn(name, static_cast<std::string::size_type>(sz));
            lua_pop(L, 2);
            return tn;
        }
        default:
            break;
        }
        return lua_typename(L, static_cast<int>(t));
    }

    inline int push_type_panic_string(lua_State* L, int index, type expected, type actual, string_view message, string_view aux_message) noexcept {
        const char* err = message.size() == 0
             ? (aux_message.size() == 0 ? "stack index %d, expected %s, received %s" : "stack index %d, expected %s, received %s: %s%s")
             : "stack index %d, expected %s, received %s: %s %s";
        const char* type_name = expected == type::poly ? "anything" : lua_typename(L, static_cast<int>(expected));
        {
            std::string actual_name = associated_type_name(L, index, actual);
            lua_pushfstring(L, err, index, type_name, actual_name.c_str(), message.data(), aux_message.data());
        }
        return 1;
    }

    inline int type_panic_string(lua_State* L, int index, type expected, type actual, string_view message = "") noexcept(false) {
        push_type_panic_string(L, index, expected, actual, message, "");
        size_t str_size = 0;
        const char* str = lua_tolstring(L, -1, &str_size);
        return luaL_error(L, str);
    }

    inline int type_panic_c_str(lua_State* L, int index, type expected, type actual, const char* message = nullptr) noexcept(false) {
        push_type_panic_string(L, index, expected, actual, message == nullptr ? "" : message, "");
        size_t str_size = 0;
        const char* str = lua_tolstring(L, -1, &str_size);
        return luaL_error(L, str);
    }

    struct type_panic_t {
        int operator()(lua_State* L, int index, type expected, type actual) const noexcept(false) {
            return type_panic_c_str(L, index, expected, actual, nullptr);
        }
        int operator()(lua_State* L, int index, type expected, type actual, string_view message) const noexcept(false) {
            return type_panic_c_str(L, index, expected, actual, message.data());
        }
    };

    inline const type_panic_t type_panic = {};

    struct constructor_handler {
        int operator()(lua_State* L, int index, type expected, type actual, string_view message) const noexcept(false) {
            push_type_panic_string(L, index, expected, actual, message, "(type check failed in constructor)");
            size_t str_size = 0;
            const char* str = lua_tolstring(L, -1, &str_size);
            return luaL_error(L, str);
        }
    };

    template <typename F = void>
    struct argument_handler {
        int operator()(lua_State* L, int index, type expected, type actual, string_view message) const noexcept(false) {
            push_type_panic_string(L, index, expected, actual, message, "(bad argument to variable or function call)");
            size_t str_size = 0;
            const char* str = lua_tolstring(L, -1, &str_size);
            return luaL_error(L, str);
        }
    };

    template <typename R, typename... Args>
    struct argument_handler<types<R, Args...>> {
        int operator()(lua_State* L, int index, type expected, type actual, string_view message) const noexcept(false) {
            {
                std::string aux_message = "(bad argument into '";
                aux_message += detail::demangle<R>();
                aux_message += "(";
                int marker = 0;
                (void)detail::swallow { int(), (detail::accumulate_and_mark(detail::demangle<Args>(), aux_message, marker), int())... };
                aux_message += ")')";
                push_type_panic_string(L, index, expected, actual, message, aux_message);
            }
            size_t str_size = 0;
            const char* str = lua_tolstring(L, -1, &str_size);
            return luaL_error(L, str);
        }
    };

    inline int no_panic(lua_State*, int, type, type, const char* = nullptr) noexcept {
        return 0;
    }

    inline void type_error(lua_State* L, int expected, int actual) noexcept(false) {
        luaL_error(L, "expected %s, received %s", lua_typename(L, expected), lua_typename(L, actual));
    }

    inline void type_error(lua_State* L, type expected, type actual) noexcept(false) {
        type_error(L, static_cast<int>(expected), static_cast<int>(actual));
    }

    inline void type_assert(lua_State* L, int index, type expected, type actual) noexcept(false) {
        if (expected != type::poly && expected != actual) {
            type_panic_c_str(L, index, expected, actual, nullptr);
        }
    }

    inline void type_assert(lua_State* L, int index, type expected) {
        type actual = type_of(L, index);
        type_assert(L, index, expected, actual);
    }
} // namespace sol

#endif // _MSC_VER

// Now include sol2 - on MSVC it will skip sol/in_place.hpp and sol/error_handler.hpp
#include <sol/sol.hpp>

#endif // JFRAME_SOL2_COMPAT_HPP

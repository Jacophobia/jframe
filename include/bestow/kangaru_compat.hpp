// bestow/kangaru_compat.hpp
// Kangaru compatibility header for MSVC C++23 modules
//
// When using 'import std;' with MSVC, inline constexpr values from kangaru
// are not properly exported across module boundaries. This header forces
// the definition of these symbols to be available.
//
// Include this header in executables that use bestow modules with kangaru.

#ifndef BESTOW_KANGARU_COMPAT_HPP
#define BESTOW_KANGARU_COMPAT_HPP

#include <kangaru/kangaru.hpp>

// MSVC with C++23 modules has issues with inline constexpr values across
// module boundaries. The kgr::detail::in_place and kgr::detail::override_index
// values are not properly visible when imported through modules.
//
// Force the symbols to be defined in every translation unit that includes this header.
#if defined(_MSC_VER)

namespace bestow::kangaru_compat {

// Reference the inline constexpr values to force their instantiation
// These functions are never called, but their mere existence forces the
// compiler to emit the symbols.
[[maybe_unused]] inline const void* force_kangaru_symbols() {
    // Reference both symbols to ensure they are instantiated
    static const void* ptrs[] = {
        static_cast<const void*>(&kgr::detail::in_place),
        static_cast<const void*>(&kgr::detail::override_index)
    };
    return ptrs;
}

// Force instantiation by creating a static initializer
namespace {
    [[maybe_unused]] const void* kangaru_symbol_forcer = force_kangaru_symbols();
}

} // namespace bestow::kangaru_compat

#endif // _MSC_VER

#endif // BESTOW_KANGARU_COMPAT_HPP

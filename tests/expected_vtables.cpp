// tests/expected_vtables.cpp
// Workaround for missing std::expected vtables when using LLVM Clang 20 with import std;
//
// This file deliberately uses #include <expected> instead of import std; to ensure
// that vtable symbols for std::bad_expected_access are emitted by the compiler.
//
// When using import std;, the std.pcm precompiled module contains declarations but
// doesn't always emit vtable symbols for template instantiations. By using traditional
// #include in this translation unit, we force the compiler to generate the needed symbols.

#include <expected>
#include <system_error>

// Reference the types we need to ensure their vtables are emitted
// The mere instantiation in this translation unit with traditional headers
// should cause the linker to have access to the vtable symbols.

namespace {
    // Force instantiation by creating unused pointers
    // This ensures vtables and typeinfo are emitted without violating ODR
    [[maybe_unused]] std::bad_expected_access<void>* unused_ptr_void = nullptr;
    [[maybe_unused]] std::bad_expected_access<std::error_code>* unused_ptr_error = nullptr;
}

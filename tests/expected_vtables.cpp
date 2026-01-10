// tests/expected_vtables.cpp
// Minimal placeholder to ensure std::expected types are available
//
// This compilation unit uses traditional #include to access std::expected
// types. While it doesn't directly provide vtables, linking with c++abi
// should provide the necessary symbols.

#include <expected>
#include <system_error>

// Empty translation unit - the mere inclusion with c++abi linking should suffice

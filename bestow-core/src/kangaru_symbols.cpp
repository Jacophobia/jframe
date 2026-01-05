// kangaru_symbols.cpp
// Provides explicit symbol definitions for kangaru's constexpr variables
// that MSVC doesn't handle correctly without 'inline' keyword

// Define the symbols with external linkage that MSVC linker expects
namespace kgr {
namespace detail {
    struct in_place_t {};
    struct override_index_t {};

    // These definitions provide the actual symbols
    extern const in_place_t in_place{};
    extern const override_index_t override_index{};
}
}

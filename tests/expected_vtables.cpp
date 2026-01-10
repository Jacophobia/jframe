// tests/expected_vtables.cpp
// Workaround for missing std::expected exception vtables on macOS with libc++experimental
//
// On macOS with LLVM Clang 20+ and libc++experimental, the vtable and typeinfo for
// std::bad_expected_access<T> are not provided by the library. We explicitly instantiate
// them here to provide the missing symbols at link time.

#if defined(__APPLE__) && defined(__clang__)

#include <expected>
#include <system_error>

// Forward declare AnimationError from bestow.animation module
namespace bestow {
    enum class AnimationError;
}

namespace std {

// Explicitly instantiate bad_expected_access for the types we use in tests
// This provides the vtable and typeinfo symbols that libc++experimental is missing

template class bad_expected_access<bestow::AnimationError>;
template class bad_expected_access<error_code>;

}  // namespace std

#endif  // __APPLE__ && __clang__

// tests/expected_vtables.cpp
// Provides explicit template instantiations for std::bad_expected_access
//
// When using LLVM Clang 20 with import std;, the std.pcm module contains declarations
// but may not emit vtables for template types. We need to explicitly instantiate
// the templates used in our codebase to force vtable emission.
//
// This is NOT needed when using Apple Clang (Xcode), which has its own libc++ implementation.

import std;
import bestow.animation;

// Force instantiation of std::bad_expected_access vtables and typeinfo
// These templates are used in our Result<T, E> types throughout the codebase

// Base class instantiation - required for polymorphism
template class std::bad_expected_access<void>;

// Derived class instantiations - used in our systems
template class std::bad_expected_access<bestow::AnimationError>;
template class std::bad_expected_access<std::error_code>;

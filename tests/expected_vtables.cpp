// tests/expected_vtables.cpp
// Empty file - kept to ensure consistent build configuration
//
// Note: macOS CI with Xcode 15.4 SDK doesn't have std::expected support in system libc++.
// We link c++abi as a workaround but the symbols still may not be available.
// This is a known limitation of using std::expected with older macOS SDKs.

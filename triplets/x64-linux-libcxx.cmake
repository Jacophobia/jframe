# x64-linux-libcxx.cmake
# Custom triplet for Linux with libc++ (required for C++23 'import std;')

set(VCPKG_TARGET_ARCHITECTURE x64)
set(VCPKG_CRT_LINKAGE dynamic)
set(VCPKG_LIBRARY_LINKAGE static)

set(VCPKG_CMAKE_SYSTEM_NAME Linux)

# Use Clang with libc++ for ABI compatibility with 'import std;'
set(VCPKG_CXX_FLAGS "-stdlib=libc++")
set(VCPKG_C_FLAGS "")
set(VCPKG_LINKER_FLAGS "-stdlib=libc++ -lc++abi")

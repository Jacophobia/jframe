# x64-linux-libcxx.cmake
# Custom triplet for Linux with libc++ (required for C++23 'import std;')

set(VCPKG_TARGET_ARCHITECTURE x64)
set(VCPKG_CRT_LINKAGE dynamic)
set(VCPKG_LIBRARY_LINKAGE static)

set(VCPKG_CMAKE_SYSTEM_NAME Linux)

# Use Clang with libc++ for ABI compatibility with 'import std;'
# The compiler must be set via environment or passed to vcpkg
set(VCPKG_CXX_FLAGS "-stdlib=libc++")
set(VCPKG_C_FLAGS "")
set(VCPKG_LINKER_FLAGS "-stdlib=libc++ -lc++abi")

# Force use of Clang - check common LLVM installation paths in order of preference
# Prefer Clang 20+ for best C++23 module support
if(EXISTS "/usr/bin/clang-20")
    set(VCPKG_CMAKE_CONFIGURE_OPTIONS
        "-DCMAKE_C_COMPILER=/usr/bin/clang-20"
        "-DCMAKE_CXX_COMPILER=/usr/bin/clang++-20"
    )
elseif(EXISTS "/usr/bin/clang-19")
    set(VCPKG_CMAKE_CONFIGURE_OPTIONS
        "-DCMAKE_C_COMPILER=/usr/bin/clang-19"
        "-DCMAKE_CXX_COMPILER=/usr/bin/clang++-19"
    )
elseif(EXISTS "/usr/bin/clang-18")
    set(VCPKG_CMAKE_CONFIGURE_OPTIONS
        "-DCMAKE_C_COMPILER=/usr/bin/clang-18"
        "-DCMAKE_CXX_COMPILER=/usr/bin/clang++-18"
    )
elseif(EXISTS "/usr/bin/clang")
    set(VCPKG_CMAKE_CONFIGURE_OPTIONS
        "-DCMAKE_C_COMPILER=/usr/bin/clang"
        "-DCMAKE_CXX_COMPILER=/usr/bin/clang++"
    )
endif()

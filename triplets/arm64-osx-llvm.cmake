# arm64-osx-llvm.cmake
# Custom triplet for macOS ARM64 with Homebrew LLVM
# Required for ABI compatibility when using LLVM Clang instead of Apple Clang

set(VCPKG_TARGET_ARCHITECTURE arm64)
set(VCPKG_CRT_LINKAGE dynamic)
set(VCPKG_LIBRARY_LINKAGE static)

set(VCPKG_CMAKE_SYSTEM_NAME Darwin)
set(VCPKG_OSX_ARCHITECTURES arm64)

# Use LLVM Clang from Homebrew for ABI compatibility with 'import std;'
# Also use LLVM's ar and ranlib to avoid archive format incompatibilities
# Check common Homebrew LLVM 20 paths
if(EXISTS "/opt/homebrew/opt/llvm@20/bin/clang")
    set(VCPKG_CMAKE_CONFIGURE_OPTIONS
        "-DCMAKE_C_COMPILER=/opt/homebrew/opt/llvm@20/bin/clang"
        "-DCMAKE_CXX_COMPILER=/opt/homebrew/opt/llvm@20/bin/clang++"
        "-DCMAKE_AR=/opt/homebrew/opt/llvm@20/bin/llvm-ar"
        "-DCMAKE_RANLIB=/opt/homebrew/opt/llvm@20/bin/llvm-ranlib"
    )
elseif(EXISTS "/opt/homebrew/opt/llvm/bin/clang")
    set(VCPKG_CMAKE_CONFIGURE_OPTIONS
        "-DCMAKE_C_COMPILER=/opt/homebrew/opt/llvm/bin/clang"
        "-DCMAKE_CXX_COMPILER=/opt/homebrew/opt/llvm/bin/clang++"
        "-DCMAKE_AR=/opt/homebrew/opt/llvm/bin/llvm-ar"
        "-DCMAKE_RANLIB=/opt/homebrew/opt/llvm/bin/llvm-ranlib"
    )
endif()

# arm64-osx-llvm.cmake
# Custom triplet for macOS ARM64 with Homebrew LLVM
# Required for ABI compatibility when using LLVM Clang instead of Apple Clang

set(VCPKG_TARGET_ARCHITECTURE arm64)
set(VCPKG_CRT_LINKAGE dynamic)
set(VCPKG_LIBRARY_LINKAGE static)

set(VCPKG_CMAKE_SYSTEM_NAME Darwin)
set(VCPKG_OSX_ARCHITECTURES arm64)

# Find the macOS SDK path - required for LLVM to find system headers like <sys/types.h>
# LLVM Clang (unlike Apple Clang) doesn't automatically know the SDK location
execute_process(
    COMMAND xcrun --show-sdk-path
    OUTPUT_VARIABLE MACOS_SDK_PATH
    OUTPUT_STRIP_TRAILING_WHITESPACE
    ERROR_QUIET
)

if(NOT MACOS_SDK_PATH)
    # Fallback to common SDK locations
    if(EXISTS "/Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX.sdk")
        set(MACOS_SDK_PATH "/Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX.sdk")
    elseif(EXISTS "/Library/Developer/CommandLineTools/SDKs/MacOSX.sdk")
        set(MACOS_SDK_PATH "/Library/Developer/CommandLineTools/SDKs/MacOSX.sdk")
    endif()
endif()

# Use LLVM Clang from Homebrew for ABI compatibility with 'import std;'
# Also use LLVM's ar and ranlib to avoid archive format incompatibilities
# Check common Homebrew LLVM 20 paths
if(EXISTS "/opt/homebrew/opt/llvm@20/bin/clang")
    set(LLVM_PATH "/opt/homebrew/opt/llvm@20")
elseif(EXISTS "/opt/homebrew/opt/llvm/bin/clang")
    set(LLVM_PATH "/opt/homebrew/opt/llvm")
elseif(EXISTS "/usr/local/opt/llvm@20/bin/clang")
    set(LLVM_PATH "/usr/local/opt/llvm@20")
endif()

if(LLVM_PATH)
    set(VCPKG_CMAKE_CONFIGURE_OPTIONS
        "-DCMAKE_C_COMPILER=${LLVM_PATH}/bin/clang"
        "-DCMAKE_CXX_COMPILER=${LLVM_PATH}/bin/clang++"
        "-DCMAKE_AR=${LLVM_PATH}/bin/llvm-ar"
        "-DCMAKE_RANLIB=${LLVM_PATH}/bin/llvm-ranlib"
    )

    # Add SDK path if found - this is critical for LLVM to find system headers
    if(MACOS_SDK_PATH)
        list(APPEND VCPKG_CMAKE_CONFIGURE_OPTIONS
            "-DCMAKE_OSX_SYSROOT=${MACOS_SDK_PATH}"
        )
        # Also set C/CXX flags to include isysroot for headers
        set(VCPKG_C_FLAGS "-isysroot ${MACOS_SDK_PATH}")
        set(VCPKG_CXX_FLAGS "-isysroot ${MACOS_SDK_PATH}")
    endif()
endif()

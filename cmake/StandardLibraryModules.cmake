# cmake/StandardLibraryModules.cmake
# Configures C++ standard library module support for different compilers
#
# Supported compilers:
#   - MSVC 19.38+ (Visual Studio 2022 17.8+): Native support via /std:c++latest
#   - LLVM Clang 20+: Pre-compiled std.pcm from libc++
#   - GCC 14+: Experimental (not yet production-ready)

# =============================================================================
# MSVC Configuration (Windows)
# =============================================================================
if(MSVC)
    message(STATUS "Configuring MSVC for C++23 modules with 'import std;' support")

    # Check MSVC version - need 19.38+ (VS 2022 17.8+)
    if(MSVC_VERSION LESS 1938)
        message(FATAL_ERROR
            "MSVC ${MSVC_VERSION} does not support 'import std;'.\n"
            "Please upgrade to Visual Studio 2022 version 17.8 or later (MSVC 19.38+).\n"
            "Download from: https://visualstudio.microsoft.com/downloads/"
        )
    endif()

    # Function to configure MSVC targets for std module support
    function(target_use_std_module TARGET_NAME)
        # Use /std:c++20 instead of /std:c++latest to avoid C++23 ADL issues with EnTT
        # MSVC C++23 modules have stricter ADL rules that break EnTT iterator comparisons
        # C++20 standard still supports 'import std;' and has better ADL compatibility
        # /Zc:preprocessor enables the conforming C++20 preprocessor (required for __VA_OPT__)
        target_compile_options(${TARGET_NAME} PRIVATE
            /std:c++20
            /experimental:module
            /Zc:preprocessor
        )

        # Enable standard library modules
        # This tells MSVC to build and use the std module
        set_target_properties(${TARGET_NAME} PROPERTIES
            CXX_STANDARD 23
            CXX_STANDARD_REQUIRED ON
            CXX_EXTENSIONS OFF
        )

        message(STATUS "  Configured ${TARGET_NAME} for MSVC std module support")
    endfunction()

    return()
endif()

# =============================================================================
# Clang Configuration (macOS / Linux / Windows with MSYS2)
# =============================================================================
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    message(STATUS "Configuring Clang for C++23 modules with 'import std;' support")

    # Use libc++ for all Clang builds (required for import std)
    # This must be set globally before any targets are defined
    add_compile_options(-stdlib=libc++)
    add_link_options(-stdlib=libc++)

    # Add pthread support globally - required for std module consistency
    # The std.pcm must be compiled with the same pthread setting as all targets
    add_compile_options(-pthread)
    add_link_options(-pthread)

    # On macOS, use system libc++ (Homebrew LLVM doesn't ship libc++ libraries)
    # macOS 15+ has std::expected support in system libc++.dylib
    # On Linux, we need to add LLVM's libc++ library path
    if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
        # Find libc++ library path
        get_filename_component(COMPILER_BIN_DIR "${CMAKE_CXX_COMPILER}" DIRECTORY)
        get_filename_component(COMPILER_ROOT "${COMPILER_BIN_DIR}" DIRECTORY)

        # Check common libc++ library locations
        if(EXISTS "${COMPILER_ROOT}/lib/x86_64-unknown-linux-gnu")
            link_directories("${COMPILER_ROOT}/lib/x86_64-unknown-linux-gnu")
        elseif(EXISTS "${COMPILER_ROOT}/lib")
            link_directories("${COMPILER_ROOT}/lib")
        endif()

        # Also try /usr/lib/llvm-20 for apt-installed LLVM
        if(EXISTS "/usr/lib/llvm-20/lib")
            link_directories("/usr/lib/llvm-20/lib")
        endif()
    endif()

    # First, try to derive the libc++ path from the compiler location
    # This handles cases where LLVM is installed in non-standard locations (e.g., CI runners)
    get_filename_component(COMPILER_DIR "${CMAKE_CXX_COMPILER}" DIRECTORY)
    get_filename_component(LLVM_ROOT "${COMPILER_DIR}" DIRECTORY)

    # Try to find std.cppm - first check relative to compiler, then common locations
    set(POSSIBLE_STD_MODULE_PATHS
        # Derived from compiler location (most reliable)
        "${LLVM_ROOT}/share/libc++/v1/std.cppm"
        # macOS Homebrew LLVM 20
        "/opt/homebrew/opt/llvm@20/share/libc++/v1/std.cppm"
        "/opt/homebrew/opt/llvm/share/libc++/v1/std.cppm"
        # macOS Intel Homebrew
        "/usr/local/opt/llvm@20/share/libc++/v1/std.cppm"
        "/usr/local/opt/llvm/share/libc++/v1/std.cppm"
        # Linux system paths
        "/usr/lib/llvm-20/share/libc++/v1/std.cppm"
        "/usr/lib/llvm-18/share/libc++/v1/std.cppm"
        "/usr/lib/llvm-17/share/libc++/v1/std.cppm"
        # Generic paths
        "/usr/share/libc++/v1/std.cppm"
        # Windows MSYS2 CLANG64 paths (CI uses D:\a\_temp\msys64)
        "D:/a/_temp/msys64/clang64/share/libc++/v1/std.cppm"
        "C:/msys64/clang64/share/libc++/v1/std.cppm"
        # MSYS2 standard installation
        "/clang64/share/libc++/v1/std.cppm"
    )

    message(STATUS "  Compiler: ${CMAKE_CXX_COMPILER}")
    message(STATUS "  LLVM root (derived): ${LLVM_ROOT}")

    set(LIBC++_STD_MODULE "")
    foreach(PATH ${POSSIBLE_STD_MODULE_PATHS})
        if(EXISTS "${PATH}")
            set(LIBC++_STD_MODULE "${PATH}")
            break()
        endif()
    endforeach()

    if(NOT LIBC++_STD_MODULE)
        message(FATAL_ERROR
            "Could not find std.cppm for libc++.\n"
            "Searched paths:\n"
            "  ${POSSIBLE_STD_MODULE_PATHS}\n\n"
            "On macOS: brew install llvm@20\n"
            "On Linux: Install libc++ with module support from LLVM 17+"
        )
    endif()

    message(STATUS "  Found std.cppm at: ${LIBC++_STD_MODULE}")

    # Directory for pre-compiled modules
    set(BESTOW_PCM_DIR "${CMAKE_BINARY_DIR}/pcm")
    file(MAKE_DIRECTORY "${BESTOW_PCM_DIR}")

    # Path to pre-compiled std module
    set(BESTOW_STD_PCM "${BESTOW_PCM_DIR}/std.pcm")

    # Get macOS sysroot and architecture flags for LLVM
    set(BESTOW_ARCH_FLAGS "")
    if(APPLE)
        if(CMAKE_OSX_SYSROOT)
            set(BESTOW_SYSROOT_FLAG "-isysroot" "${CMAKE_OSX_SYSROOT}")
        else()
            set(BESTOW_SYSROOT_FLAG "")
        endif()
        # Pass target architecture for cross-compilation (e.g., x86_64 on arm64)
        if(CMAKE_OSX_ARCHITECTURES)
            set(BESTOW_ARCH_FLAGS "-arch" "${CMAKE_OSX_ARCHITECTURES}")
        endif()
    else()
        set(BESTOW_SYSROOT_FLAG "")
    endif()

    # Pre-compile std.cppm to std.pcm at configure time
    if(NOT EXISTS "${BESTOW_STD_PCM}")
        message(STATUS "  Pre-compiling C++ standard library module...")

        execute_process(
            COMMAND "${CMAKE_CXX_COMPILER}"
                -std=c++23
                -stdlib=libc++
                -pthread
                ${BESTOW_ARCH_FLAGS}
                ${BESTOW_SYSROOT_FLAG}
                --precompile
                "${LIBC++_STD_MODULE}"
                -o "${BESTOW_STD_PCM}"
            RESULT_VARIABLE STD_PCM_RESULT
            OUTPUT_VARIABLE STD_PCM_OUTPUT
            ERROR_VARIABLE STD_PCM_ERROR
        )

        if(NOT STD_PCM_RESULT EQUAL 0)
            message(FATAL_ERROR "Failed to pre-compile std.cppm:\n${STD_PCM_ERROR}")
        endif()

        message(STATUS "  Pre-compiled std.pcm to ${BESTOW_STD_PCM}")
    endif()

    # Function to configure Clang targets for std module support
    function(target_use_std_module TARGET_NAME)
        # Add the pre-built module path
        target_compile_options(${TARGET_NAME} PRIVATE
            -fprebuilt-module-path=${BESTOW_PCM_DIR}
        )

        # Make sure std.pcm is built before the target
        if(NOT TARGET bestow-std-module)
            add_custom_command(
                OUTPUT "${BESTOW_STD_PCM}"
                COMMAND "${CMAKE_CXX_COMPILER}"
                    -std=c++23
                    -stdlib=libc++
                    -pthread
                    ${BESTOW_ARCH_FLAGS}
                    ${BESTOW_SYSROOT_FLAG}
                    --precompile
                    "${LIBC++_STD_MODULE}"
                    -o "${BESTOW_STD_PCM}"
                DEPENDS "${LIBC++_STD_MODULE}"
                COMMENT "Pre-compiling C++ standard library module"
                VERBATIM
            )

            add_custom_target(bestow-std-module
                DEPENDS "${BESTOW_STD_PCM}"
            )
        endif()

        add_dependencies(${TARGET_NAME} bestow-std-module)
    endfunction()

    return()
endif()

# =============================================================================
# GCC Configuration (Linux - Experimental)
# =============================================================================
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    # Check GCC version
    if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS "14.0")
        message(FATAL_ERROR
            "GCC ${CMAKE_CXX_COMPILER_VERSION} has limited 'import std;' support.\n"
            "Please upgrade to GCC 14+ or use Clang 17+ instead.\n"
            "On Ubuntu/Debian: sudo apt install g++-14\n"
            "Alternatively, install LLVM Clang for better module support."
        )
    endif()

    message(STATUS "Configuring GCC for C++23 modules (experimental)")
    message(WARNING
        "GCC's 'import std;' support is experimental and may have issues.\n"
        "Consider using Clang 17+ or MSVC 19.38+ for better stability."
    )

    # Function for GCC - minimal configuration
    function(target_use_std_module TARGET_NAME)
        target_compile_options(${TARGET_NAME} PRIVATE
            -fmodules-ts
        )
        set_target_properties(${TARGET_NAME} PROPERTIES
            CXX_STANDARD 23
            CXX_STANDARD_REQUIRED ON
        )
    endfunction()

    return()
endif()

# =============================================================================
# Unsupported Compiler
# =============================================================================
message(FATAL_ERROR
    "Unsupported compiler: ${CMAKE_CXX_COMPILER_ID}\n\n"
    "Bestow requires a C++23 compiler with 'import std;' support:\n"
    "  - Windows: MSVC 19.38+ (Visual Studio 2022 17.8+)\n"
    "  - macOS: LLVM Clang 20+ (brew install llvm@20)\n"
    "  - Linux: Clang 17+ with libc++, or GCC 14+ (experimental)\n\n"
    "Apple Clang (Xcode) does NOT support 'import std;'."
)

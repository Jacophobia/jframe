# cmake/StandardLibraryModules.cmake
# Pre-compiles the C++ standard library module for LLVM 20

# Only run this for LLVM/Clang compilers
if(NOT CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    message(WARNING "Standard library module pre-compilation is only supported with Clang")
    return()
endif()

# Check if we're on macOS with Homebrew LLVM 20
set(LLVM20_ROOT "/opt/homebrew/opt/llvm@20")
set(LIBC++_STD_MODULE "${LLVM20_ROOT}/share/libc++/v1/std.cppm")

if(NOT EXISTS "${LIBC++_STD_MODULE}")
    message(WARNING "Could not find std.cppm at ${LIBC++_STD_MODULE}")
    return()
endif()

# Directory for pre-compiled modules
set(JFRAME_PCM_DIR "${CMAKE_BINARY_DIR}/pcm")
file(MAKE_DIRECTORY "${JFRAME_PCM_DIR}")

# Path to pre-compiled std module
set(JFRAME_STD_PCM "${JFRAME_PCM_DIR}/std.pcm")

# Get macOS sysroot for LLVM
if(APPLE AND CMAKE_OSX_SYSROOT)
    set(JFRAME_SYSROOT_FLAG "-isysroot" "${CMAKE_OSX_SYSROOT}")
else()
    set(JFRAME_SYSROOT_FLAG "")
endif()

# Pre-compile std.cppm to std.pcm
if(NOT EXISTS "${JFRAME_STD_PCM}")
    message(STATUS "Pre-compiling C++ standard library module...")

    execute_process(
        COMMAND "${CMAKE_CXX_COMPILER}"
            -std=c++23
            -stdlib=libc++
            ${JFRAME_SYSROOT_FLAG}
            --precompile
            "${LIBC++_STD_MODULE}"
            -o "${JFRAME_STD_PCM}"
        RESULT_VARIABLE STD_PCM_RESULT
        OUTPUT_VARIABLE STD_PCM_OUTPUT
        ERROR_VARIABLE STD_PCM_ERROR
    )

    if(NOT STD_PCM_RESULT EQUAL 0)
        message(FATAL_ERROR "Failed to pre-compile std.cppm:\n${STD_PCM_ERROR}")
    endif()

    message(STATUS "Pre-compiled std.pcm to ${JFRAME_STD_PCM}")
endif()

# Function to configure a target to use the pre-compiled std module
function(target_use_std_module TARGET_NAME)
    # Add the pre-built module path
    target_compile_options(${TARGET_NAME} PRIVATE
        -fprebuilt-module-path=${JFRAME_PCM_DIR}
    )

    # Make sure std.pcm is built before the target
    # We do this by creating a custom target that depends on the std.pcm file
    if(NOT TARGET jframe-std-module)
        add_custom_command(
            OUTPUT "${JFRAME_STD_PCM}"
            COMMAND "${CMAKE_CXX_COMPILER}"
                -std=c++23
                -stdlib=libc++
                ${JFRAME_SYSROOT_FLAG}
                --precompile
                "${LIBC++_STD_MODULE}"
                -o "${JFRAME_STD_PCM}"
            DEPENDS "${LIBC++_STD_MODULE}"
            COMMENT "Pre-compiling C++ standard library module"
            VERBATIM
        )

        add_custom_target(jframe-std-module
            DEPENDS "${JFRAME_STD_PCM}"
        )
    endif()

    add_dependencies(${TARGET_NAME} jframe-std-module)
endfunction()

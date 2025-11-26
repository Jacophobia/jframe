cmake_minimum_required(VERSION 3.28)

project(jframe
    VERSION 1.0.0
    LANGUAGES CXX
    DESCRIPTION "JFrame Game Framework"
)

# C++23 settings
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Options
option(JFRAME_BUILD_TESTS "Build tests" ON)
option(JFRAME_BUILD_EXAMPLES "Build examples" ON)
option(JFRAME_ENABLE_TRACY "Enable Tracy profiler" ON)
option(JFRAME_ENABLE_ASAN "Enable AddressSanitizer" OFF)
option(JFRAME_DEV_TOOLS "Enable development tools (hot reload, inspector)" ON)

# vcpkg toolchain (if using vcpkg)
if(DEFINED ENV{VCPKG_ROOT} AND NOT DEFINED CMAKE_TOOLCHAIN_FILE)
    set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake")
endif()

# Include cmake modules
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(CompilerWarnings)
include(StaticAnalysis)
include(Dependencies)
include(StandardLibraryModules)

# Find dependencies
find_dependencies()

# Enable C++23 module scanning after dependencies are found
# (to avoid interference with CMake's internal try_compile checks)
set(CMAKE_CXX_SCAN_FOR_MODULES ON)

# Add subdirectories
add_subdirectory(jframe-contract)
add_subdirectory(jframe-core)
add_subdirectory(jframe-entity)
add_subdirectory(jframe-graphics)
add_subdirectory(jframe-audio)
add_subdirectory(jframe-input)
add_subdirectory(jframe-assets)
add_subdirectory(jframe-save)
add_subdirectory(jframe-level)
add_subdirectory(jframe-events)
add_subdirectory(jframe-physics)
add_subdirectory(jframe-ai)

# Development tools (only in debug builds)
if(JFRAME_DEV_TOOLS)
    add_subdirectory(jframe-dev)
endif()

if(JFRAME_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

if(JFRAME_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

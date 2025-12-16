cmake_minimum_required(VERSION 3.28)

project(bestow
    VERSION 1.0.0
    LANGUAGES CXX
    DESCRIPTION "Bestow Game Framework"
)

# C++23 settings
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Options
option(BESTOW_BUILD_TESTS "Build tests" ON)
option(BESTOW_ENABLE_TRACY "Enable Tracy profiler" ON)
option(BESTOW_ENABLE_ASAN "Enable AddressSanitizer" OFF)
option(BESTOW_DEV_TOOLS "Enable development tools (hot reload, inspector)" ON)
option(BESTOW_BUILD_VULKAN "Build Vulkan rendering backend (requires vcpkg feature: vulkan)" OFF)

# vcpkg toolchain (if using vcpkg)
if(DEFINED ENV{VCPKG_ROOT} AND NOT DEFINED CMAKE_TOOLCHAIN_FILE)
    set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake")
endif()

# Include cmake modules
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(CompilerWarnings)
include(StaticAnalysis)
include(Dependencies)

# Add project-wide include directory (for compatibility headers)
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/include")

# Find dependencies BEFORE StandardLibraryModules to avoid
# global compile options interfering with vcpkg try_compile checks
find_dependencies()

# Now include StandardLibraryModules - this adds -stdlib=libc++ globally
# but vcpkg has already finished its try_compile checks
include(StandardLibraryModules)

# Enable C++23 module scanning after dependencies are found
# (to avoid interference with CMake's internal try_compile checks)
set(CMAKE_CXX_SCAN_FOR_MODULES ON)

# Add subdirectories
add_subdirectory(bestow-contract)
add_subdirectory(bestow-metrics)
add_subdirectory(bestow-components)
add_subdirectory(bestow-core)
add_subdirectory(bestow-entity)
add_subdirectory(bestow-opengl-helpers)
add_subdirectory(bestow-opengl)
add_subdirectory(bestow-audio)
add_subdirectory(bestow-input)
add_subdirectory(bestow-assets)
add_subdirectory(bestow-save)
add_subdirectory(bestow-level)
add_subdirectory(bestow-events)
add_subdirectory(bestow-physics-common)
add_subdirectory(bestow-physics)
add_subdirectory(bestow-physics3d)
add_subdirectory(bestow-ai)
add_subdirectory(bestow-camera)
add_subdirectory(bestow-config)
add_subdirectory(bestow-gas)
add_subdirectory(bestow-blueprints)
add_subdirectory(bestow-ui)
add_subdirectory(bestow-gamestate)
add_subdirectory(bestow-shader)

# Vulkan rendering backend (optional)
if(BESTOW_BUILD_VULKAN)
    find_package(Vulkan QUIET)
    if(Vulkan_FOUND)
        add_subdirectory(bestow-vulkan)
    else()
        message(STATUS "Vulkan not found - skipping bestow-vulkan")
    endif()
endif()

# Development tools (only in debug builds)
if(BESTOW_DEV_TOOLS)
    add_subdirectory(bestow-dev)
endif()

if(BESTOW_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

# Example games (require Vulkan)
if(BESTOW_BUILD_VULKAN AND TARGET bestow-vulkan)
    add_subdirectory(games/game1)
endif()

# Template uses proper contract-based DI architecture
# See template/README.md for usage

# =============================================================================
# Installation Configuration
# =============================================================================

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

# List of all Bestow library targets
set(BESTOW_LIBRARY_TARGETS
    bestow-contract
    bestow-metrics
    bestow-components
    bestow-core
    bestow-entity
    bestow-opengl-helpers
    bestow-opengl
    bestow-audio
    bestow-input
    bestow-assets
    bestow-save
    bestow-level
    bestow-events
    bestow-physics-common
    bestow-physics
    bestow-ai
    bestow-camera
    bestow-config
    bestow-gas
    bestow-blueprints
    bestow-ui
    bestow-gamestate
    bestow-shader
)

# Add optional 3D physics system (requires Jolt Physics)
if(TARGET bestow-physics3d)
    list(APPEND BESTOW_LIBRARY_TARGETS bestow-physics3d)
endif()

if(BESTOW_DEV_TOOLS)
    list(APPEND BESTOW_LIBRARY_TARGETS bestow-dev)
endif()

# Note: Tracy is NOT exported - it's statically linked into bestow-metrics
# and its include paths point to the build directory (FetchContent)

# Install library targets
install(TARGETS ${BESTOW_LIBRARY_TARGETS}
    EXPORT bestow-targets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    FILE_SET CXX_MODULES DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/bestow/modules
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

# Export targets
install(EXPORT bestow-targets
    FILE bestow-targets.cmake
    NAMESPACE bestow::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/bestow
)

# Create package config file
configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/bestow-config.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/bestow-config.cmake"
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/bestow
)

# Create version file
write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/bestow-config-version.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

# Install config files
install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/bestow-config.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/bestow-config-version.cmake"
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/bestow
)

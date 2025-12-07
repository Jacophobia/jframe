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
option(BESTOW_BUILD_EXAMPLES "Build examples" ON)
option(BESTOW_ENABLE_TRACY "Enable Tracy profiler" ON)
option(BESTOW_ENABLE_ASAN "Enable AddressSanitizer" OFF)
option(BESTOW_DEV_TOOLS "Enable development tools (hot reload, inspector)" ON)

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

# Add project-wide include directory (for compatibility headers)
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/include")

# Find dependencies
find_dependencies()

# Enable C++23 module scanning after dependencies are found
# (to avoid interference with CMake's internal try_compile checks)
set(CMAKE_CXX_SCAN_FOR_MODULES ON)

# Add subdirectories
add_subdirectory(bestow-contract)
add_subdirectory(bestow-metrics)
add_subdirectory(bestow-components)
add_subdirectory(bestow-core)
add_subdirectory(bestow-entity)
add_subdirectory(bestow-graphics)
add_subdirectory(bestow-graphics3d)
add_subdirectory(bestow-audio)
add_subdirectory(bestow-input)
add_subdirectory(bestow-assets)
add_subdirectory(bestow-save)
add_subdirectory(bestow-level)
add_subdirectory(bestow-events)
add_subdirectory(bestow-physics)
add_subdirectory(bestow-physics3d)
add_subdirectory(bestow-ai)
add_subdirectory(bestow-camera)
add_subdirectory(bestow-config)
add_subdirectory(bestow-gas)
add_subdirectory(bestow-blueprints)
add_subdirectory(bestow-ui)
add_subdirectory(bestow-gamestate)

# Development tools (only in debug builds)
if(BESTOW_DEV_TOOLS)
    add_subdirectory(bestow-dev)
endif()

if(BESTOW_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

if(BESTOW_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

# Template game project (always built as a reference)
add_subdirectory(template)

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
    bestow-graphics
    bestow-graphics3d
    bestow-audio
    bestow-input
    bestow-assets
    bestow-save
    bestow-level
    bestow-events
    bestow-physics
    bestow-ai
    bestow-camera
    bestow-config
    bestow-gas
    bestow-blueprints
    bestow-ui
    bestow-gamestate
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

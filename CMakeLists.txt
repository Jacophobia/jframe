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

# Add project-wide include directory (for compatibility headers)
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/include")

# Find dependencies
find_dependencies()

# Enable C++23 module scanning after dependencies are found
# (to avoid interference with CMake's internal try_compile checks)
set(CMAKE_CXX_SCAN_FOR_MODULES ON)

# Add subdirectories
add_subdirectory(jframe-contract)
add_subdirectory(jframe-components)
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
add_subdirectory(jframe-camera)
add_subdirectory(jframe-config)
add_subdirectory(jframe-gas)
add_subdirectory(jframe-blueprints)

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

# Template game project (always built as a reference)
add_subdirectory(template)

# =============================================================================
# Installation Configuration
# =============================================================================

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

# List of all JFrame library targets
set(JFRAME_LIBRARY_TARGETS
    jframe-contract
    jframe-components
    jframe-core
    jframe-entity
    jframe-graphics
    jframe-audio
    jframe-input
    jframe-assets
    jframe-save
    jframe-level
    jframe-events
    jframe-physics
    jframe-ai
    jframe-camera
    jframe-config
    jframe-gas
    jframe-blueprints
)

if(JFRAME_DEV_TOOLS)
    list(APPEND JFRAME_LIBRARY_TARGETS jframe-dev)
endif()

# Install library targets
install(TARGETS ${JFRAME_LIBRARY_TARGETS}
    EXPORT jframe-targets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    FILE_SET CXX_MODULES DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/jframe/modules
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

# Export targets
install(EXPORT jframe-targets
    FILE jframe-targets.cmake
    NAMESPACE jframe::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/jframe
)

# Create package config file
configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/jframe-config.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/jframe-config.cmake"
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/jframe
)

# Create version file
write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/jframe-config-version.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

# Install config files
install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/jframe-config.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/jframe-config-version.cmake"
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/jframe
)

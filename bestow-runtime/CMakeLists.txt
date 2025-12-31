# bestow-runtime/CMakeLists.txt
# Lua-first game runtime executable
#
# This creates the 'bestow' executable that runs Lua games.
# Usage: bestow [options] <main.lua>
#
# Integrates with full Bestow engine via DI:
# - VulkanGraphics3DSystem for 3D rendering
# - InputSystem for keyboard/mouse/gamepad
# - AudioSystem for sound playback
# - AssetSystem for asset loading
# - ConfigSystem for Lua config parsing

# Only build if Vulkan is available
if(NOT TARGET bestow-vulkan)
    message(STATUS "Skipping bestow-runtime - bestow-vulkan not available (enable BESTOW_BUILD_VULKAN)")
    return()
endif()

add_executable(bestow-runtime
    src/main.cpp
)

# Module files
target_sources(bestow-runtime
    PUBLIC FILE_SET CXX_MODULES FILES
        src/bestow.lua.runtime.cppm
)

# Link against Bestow core and implementation libraries
target_link_libraries(bestow-runtime
    PRIVATE
        # Core (includes Engine class and contracts)
        bestow-core

        # Implementation libraries
        bestow-vulkan      # Vulkan 3D graphics
        bestow-input       # Input system
        bestow-events      # Event system
        bestow-audio       # Audio system
        bestow-assets      # Asset system
        bestow-config      # Config system

        # Lua bindings
        lua
        sol2::sol2

        # Utilities
        spdlog::spdlog
        glm::glm

        # Required for MSVC C++23 modules
        kangaru::kangaru
)

target_compile_features(bestow-runtime PRIVATE cxx_std_23)

# MSVC workaround
if(MSVC)
    target_compile_options(bestow-runtime PRIVATE /FI"bestow/kangaru_compat.hpp")
endif()

# Enable import std; support
target_use_std_module(bestow-runtime)

# Output as 'bestow' executable
set_target_properties(bestow-runtime PROPERTIES
    OUTPUT_NAME "bestow"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

# Copy asset-library to output directory (includes shaders in library/shaders/)
if(EXISTS "${CMAKE_SOURCE_DIR}/asset-library")
    add_custom_command(TARGET bestow-runtime POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_SOURCE_DIR}/asset-library"
            "$<TARGET_FILE_DIR:bestow-runtime>/library"
        COMMENT "Copying asset-library to output directory"
    )
endif()

# Install the runtime executable
install(TARGETS bestow-runtime
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

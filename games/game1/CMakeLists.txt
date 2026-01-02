# games/game1 - 3D Isometric Snake Game
# Demonstrates the Bestow contract-based DI architecture

# Only build if Vulkan is available
if(NOT TARGET bestow-vulkan)
    message(STATUS "Skipping game1 - bestow-vulkan not available (enable BESTOW_BUILD_VULKAN)")
    return()
endif()

add_executable(snake-game
    src/main.cpp
)

# The game modules (partitions must come before primary interface)
target_sources(snake-game
    PUBLIC FILE_SET CXX_MODULES FILES
        src/snake.game-types.cppm    # Types partition (must be first)
        src/snake.game.cppm          # Primary module interface
)

# Link against Bestow core and implementation libraries
target_link_libraries(snake-game
    PRIVATE
        # Core (includes Engine class and contracts)
        bestow-core

        # Implementation libraries used by this game
        bestow-vulkan      # Vulkan 3D graphics
        bestow-input       # Input system
        bestow-events      # Event system
        bestow-audio       # Audio system
        bestow-assets      # Asset system
        bestow-config      # Config system

        # Required for MSVC C++23 modules - kangaru inline constexpr values
        kangaru::kangaru
)

target_compile_features(snake-game PRIVATE cxx_std_23)

# MSVC workaround: force include kangaru_compat.hpp to fix inline constexpr issues with C++23 modules
if(MSVC)
    target_compile_options(snake-game PRIVATE /FI"bestow/kangaru_compat.hpp")
endif()

# Enable import std; support
target_use_std_module(snake-game)

# Copy data directory if it exists
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/data")
    add_custom_command(TARGET snake-game POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_CURRENT_SOURCE_DIR}/data"
            "$<TARGET_FILE_DIR:snake-game>/data"
    )
endif()

# Note: Shaders are now included in asset-library/shaders and copied via the library copy below

# Copy asset-library to output directory as "library" (for :library:/ path resolution)
if(EXISTS "${CMAKE_SOURCE_DIR}/asset-library")
    add_custom_command(TARGET snake-game POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_SOURCE_DIR}/asset-library"
            "$<TARGET_FILE_DIR:snake-game>/library"
        COMMENT "Copying asset-library to output directory"
    )
endif()

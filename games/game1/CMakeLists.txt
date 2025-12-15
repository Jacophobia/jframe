# games/game1 - 3D Isometric Snake Game
# Demonstrates the simple bestow::Game API

# Only build if bestow-runtime is available (requires Vulkan)
if(NOT TARGET bestow-runtime)
    message(STATUS "Skipping game1 - bestow-runtime not available (enable BESTOW_BUILD_VULKAN)")
    return()
endif()

add_executable(snake-game
    src/main.cpp
)

# The game module
target_sources(snake-game
    PUBLIC FILE_SET CXX_MODULES FILES
        src/snake.game.cppm
)

target_link_libraries(snake-game
    PRIVATE
        bestow-runtime
)

target_compile_features(snake-game PRIVATE cxx_std_23)

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

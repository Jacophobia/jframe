# JFrame Template Game Project
# Copy this template to start your own game!

cmake_minimum_required(VERSION 3.28)

add_executable(template-game
    src/main.cpp
    src/Game.cpp
    src/Game.h
)

# Enable C++23 modules support
target_use_std_module(template-game)

# Link all JFrame systems
# Note: lua and sol2 linked explicitly for Windows - module imports require the symbols
target_link_libraries(template-game
    PRIVATE
        jframe-contract
        jframe-core
        jframe-entity
        jframe-graphics
        jframe-physics
        jframe-audio
        jframe-input
        jframe-events
        jframe-assets
        jframe-level
        jframe-config
        jframe-gas
        jframe-camera
        jframe-blueprints
        jframe-components
        lua
        sol2::sol2
)

target_compile_features(template-game PRIVATE cxx_std_23)

# Copy data directory to build output
add_custom_command(TARGET template-game POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_CURRENT_SOURCE_DIR}/data
    $<TARGET_FILE_DIR:template-game>/data
)

# Copy FMOD dylib to build output (macOS)
if(APPLE AND TARGET fmod)
    add_custom_command(TARGET template-game POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_SOURCE_DIR}/external/fmod/lib/macos/libfmod.dylib
        $<TARGET_FILE_DIR:template-game>/libfmod.dylib
    )
endif()

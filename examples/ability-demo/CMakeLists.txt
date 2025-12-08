add_executable(ability-demo
    src/main.cpp
    src/Game.cpp
    src/Game.h
    src/Components.h
)
target_use_std_module(ability-demo)

# Note: lua and sol2 linked explicitly for Windows - module imports require the symbols
target_link_libraries(ability-demo
    PRIVATE
        bestow-contract
        bestow-core
        bestow-entity
        bestow-opengl
        bestow-physics
        bestow-audio
        bestow-input
        bestow-events
        bestow-assets
        bestow-level
        bestow-config
        bestow-gas
        bestow-camera
        bestow-blueprints
        bestow-components
        lua
        sol2::sol2
)

target_compile_features(ability-demo PRIVATE cxx_std_23)

# Copy data directory to build output
add_custom_command(TARGET ability-demo POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_CURRENT_SOURCE_DIR}/data
    $<TARGET_FILE_DIR:ability-demo>/data
)

# Copy FMOD dylib to build output (macOS)
if(APPLE AND TARGET fmod)
    add_custom_command(TARGET ability-demo POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_SOURCE_DIR}/external/fmod/lib/macos/libfmod.dylib
        $<TARGET_FILE_DIR:ability-demo>/libfmod.dylib
    )
endif()

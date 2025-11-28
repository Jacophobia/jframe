add_executable(ability-demo
    src/main.cpp
    src/Game.cpp
    src/Game.h
    src/Components.h
)
target_use_std_module(ability-demo)

target_link_libraries(ability-demo
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

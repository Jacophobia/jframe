add_executable(platformer
    src/main.cpp
    src/Game.cpp
    src/systems/PlayerMovementSystem.cpp
)
target_use_std_module(platformer)

target_link_libraries(platformer
    PRIVATE
        jframe-contract
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
)

if(JFRAME_DEV_TOOLS)
    target_link_libraries(platformer PRIVATE jframe-dev)
endif()

target_compile_features(platformer PRIVATE cxx_std_23)

# Copy data directory to build output
add_custom_command(TARGET platformer POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_CURRENT_SOURCE_DIR}/data
    $<TARGET_FILE_DIR:platformer>/data
)

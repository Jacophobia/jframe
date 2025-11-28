add_executable(platformer-demo
    src/main.cpp
    src/Game.cpp
)
target_use_std_module(platformer-demo)

target_link_libraries(platformer-demo
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
        jframe-camera
        jframe-components
        jframe-config
)

if(JFRAME_DEV_TOOLS)
    target_link_libraries(platformer-demo PRIVATE jframe-dev)
endif()

target_compile_features(platformer-demo PRIVATE cxx_std_23)

# Copy data directory to build output
add_custom_command(TARGET platformer-demo POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_CURRENT_SOURCE_DIR}/data
    $<TARGET_FILE_DIR:platformer-demo>/data
)

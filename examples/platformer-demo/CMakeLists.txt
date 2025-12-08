add_executable(platformer-demo
    src/main.cpp
    src/Game.cpp
)
target_use_std_module(platformer-demo)

# Note: lua and sol2 linked explicitly for Windows - module imports require the symbols
target_link_libraries(platformer-demo
    PRIVATE
        bestow-contract
        bestow-core
        bestow-entity
        bestow-opengl
        bestow-audio
        bestow-input
        bestow-assets
        bestow-save
        bestow-level
        bestow-events
        bestow-physics
        bestow-ai
        bestow-camera
        bestow-components
        bestow-config
        lua
        sol2::sol2
)

if(BESTOW_DEV_TOOLS)
    target_link_libraries(platformer-demo PRIVATE bestow-dev)
endif()

target_compile_features(platformer-demo PRIVATE cxx_std_23)

# Copy data directory to build output
add_custom_command(TARGET platformer-demo POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_CURRENT_SOURCE_DIR}/data
    $<TARGET_FILE_DIR:platformer-demo>/data
)

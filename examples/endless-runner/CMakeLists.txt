add_executable(endless-runner
    src/main.cpp
    src/Game.cpp
)
target_use_std_module(endless-runner)

# Note: lua and sol2 linked explicitly for Windows - module imports require the symbols
target_link_libraries(endless-runner
    PRIVATE
        bestow-contract
        bestow-core
        bestow-entity
        bestow-graphics
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
    target_link_libraries(endless-runner PRIVATE bestow-dev)
endif()

target_compile_features(endless-runner PRIVATE cxx_std_23)

# Copy data directory to build output
add_custom_command(TARGET endless-runner POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_CURRENT_SOURCE_DIR}/data
    $<TARGET_FILE_DIR:endless-runner>/data
)

# Copy shared assets from platformer-demo
add_custom_command(TARGET endless-runner POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_SOURCE_DIR}/examples/platformer-demo/data/textures
    $<TARGET_FILE_DIR:endless-runner>/data/textures
)
add_custom_command(TARGET endless-runner POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_SOURCE_DIR}/examples/platformer-demo/data/fonts
    $<TARGET_FILE_DIR:endless-runner>/data/fonts
)
add_custom_command(TARGET endless-runner POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_SOURCE_DIR}/examples/platformer-demo/data/audio
    $<TARGET_FILE_DIR:endless-runner>/data/audio
)

add_executable(platformer
    src/main.cpp
    src/Game.cpp
)
target_use_std_module(platformer)

# Note: lua and sol2 linked explicitly for Windows - module imports require the symbols
target_link_libraries(platformer
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
        lua
        sol2::sol2
)

if(BESTOW_DEV_TOOLS)
    target_link_libraries(platformer PRIVATE bestow-dev)
endif()

target_compile_features(platformer PRIVATE cxx_std_23)

# Copy data directory to build output
add_custom_command(TARGET platformer POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_CURRENT_SOURCE_DIR}/data
    $<TARGET_FILE_DIR:platformer>/data
)

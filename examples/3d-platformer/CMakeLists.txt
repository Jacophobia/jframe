# 3D Platformer Demo - Demonstrates Physics3D and Graphics3D systems

add_executable(3d-platformer
    src/main.cpp
)
target_use_std_module(3d-platformer)

target_link_libraries(3d-platformer
    PRIVATE
        bestow-contract
        bestow-graphics3d
        bestow-physics3d
        bestow-input
        glfw
        glm::glm
)

target_compile_features(3d-platformer PRIVATE cxx_std_23)

# Copy assets to build directory so the executable can find them
add_custom_command(TARGET 3d-platformer POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_CURRENT_SOURCE_DIR}/assets"
        "$<TARGET_FILE_DIR:3d-platformer>/assets"
    COMMENT "Copying 3d-platformer assets to build directory"
)

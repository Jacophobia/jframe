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

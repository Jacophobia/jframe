# 3D Platformer Demo - Demonstrates Physics3D and Graphics3D systems
# Builds two executables: 3d-platformer_gl (OpenGL) and 3d-platformer_vk (Vulkan)

# OpenGL version
add_executable(3d-platformer_gl
    src/main.cpp
)
target_use_std_module(3d-platformer_gl)

target_link_libraries(3d-platformer_gl
    PRIVATE
        bestow-contract
        bestow-opengl
        bestow-shader
        bestow-physics3d
        bestow-input
        bestow-dev
        glfw
        glm::glm
)

target_compile_features(3d-platformer_gl PRIVATE cxx_std_23)

# Copy assets to build directory so the executable can find them
add_custom_command(TARGET 3d-platformer_gl POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_CURRENT_SOURCE_DIR}/assets"
        "$<TARGET_FILE_DIR:3d-platformer_gl>/assets"
    COMMENT "Copying 3d-platformer_gl assets to build directory"
)

# Vulkan version
add_executable(3d-platformer_vk
    src/main.cpp
)
target_use_std_module(3d-platformer_vk)

target_compile_definitions(3d-platformer_vk PRIVATE USE_VULKAN_RENDERER)

target_link_libraries(3d-platformer_vk
    PRIVATE
        bestow-contract
        bestow-vulkan
        bestow-shader
        bestow-physics3d
        bestow-input
        bestow-dev
        glfw
        glm::glm
)

target_compile_features(3d-platformer_vk PRIVATE cxx_std_23)

add_custom_command(TARGET 3d-platformer_vk POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_CURRENT_SOURCE_DIR}/assets"
        "$<TARGET_FILE_DIR:3d-platformer_vk>/assets"
    # Copy GLSL sources alongside SPV files for hot reload
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_SOURCE_DIR}/bestow-vulkan/shaders"
        "$<TARGET_FILE_DIR:3d-platformer_vk>/shaders"
    COMMENT "Copying 3d-platformer_vk assets to build directory"
)

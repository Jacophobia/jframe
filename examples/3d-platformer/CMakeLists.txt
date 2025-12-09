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
        bestow-assets
        bestow-config
        glfw
        glm::glm
)

target_compile_features(3d-platformer_gl PRIVATE cxx_std_23)

# Create symlink to assets for hot reload support (edits to source files are immediately visible)
# On Windows, use copy_directory as fallback since symlinks require admin privileges
add_custom_command(TARGET 3d-platformer_gl POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E rm -rf "$<TARGET_FILE_DIR:3d-platformer_gl>/assets"
    COMMAND ${CMAKE_COMMAND} -E create_symlink
        "${CMAKE_CURRENT_SOURCE_DIR}/assets"
        "$<TARGET_FILE_DIR:3d-platformer_gl>/assets"
    COMMENT "Creating symlink to 3d-platformer assets for hot reload"
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
        bestow-assets
        bestow-config
        glfw
        glm::glm
)

target_compile_features(3d-platformer_vk PRIVATE cxx_std_23)

# Create symlinks to assets for hot reload support
add_custom_command(TARGET 3d-platformer_vk POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E rm -rf "$<TARGET_FILE_DIR:3d-platformer_vk>/assets"
    COMMAND ${CMAKE_COMMAND} -E create_symlink
        "${CMAKE_CURRENT_SOURCE_DIR}/assets"
        "$<TARGET_FILE_DIR:3d-platformer_vk>/assets"
    # Also symlink Vulkan shaders for hot reload
    COMMAND ${CMAKE_COMMAND} -E rm -rf "$<TARGET_FILE_DIR:3d-platformer_vk>/shaders"
    COMMAND ${CMAKE_COMMAND} -E create_symlink
        "${CMAKE_SOURCE_DIR}/bestow-vulkan/shaders"
        "$<TARGET_FILE_DIR:3d-platformer_vk>/shaders"
    COMMENT "Creating symlinks to 3d-platformer assets for hot reload"
)

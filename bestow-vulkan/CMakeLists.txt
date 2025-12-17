# bestow-vulkan/CMakeLists.txt
# Vulkan rendering backend

find_package(Vulkan REQUIRED)
find_package(VulkanMemoryAllocator CONFIG REQUIRED)
find_package(vk-bootstrap CONFIG REQUIRED)

#=============================================================================
# Shader Sources (GLSL)
#=============================================================================
# NOTE: Shaders are NO LONGER pre-compiled to SPIR-V at build time.
# Instead, they are compiled at runtime via AssetSystem using the shaderc library.
# This enables:
# - Hot reload of shaders during development
# - Single source of truth (GLSL files only)
# - No external compiler dependency for end users
#
# The GLSL sources are copied to the build directory for runtime access.

# Shader source files
set(VULKAN_SHADER_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/shaders/basic3d.vert
    ${CMAKE_CURRENT_SOURCE_DIR}/shaders/basic3d.frag
    ${CMAKE_CURRENT_SOURCE_DIR}/shaders/debug.vert
    ${CMAKE_CURRENT_SOURCE_DIR}/shaders/debug.frag
    ${CMAKE_CURRENT_SOURCE_DIR}/shaders/sprite2d.vert
    ${CMAKE_CURRENT_SOURCE_DIR}/shaders/sprite2d.frag
)

# Output directory for shader sources (runtime access)
set(SHADER_OUTPUT_DIR ${CMAKE_CURRENT_BINARY_DIR}/shaders)
file(MAKE_DIRECTORY ${SHADER_OUTPUT_DIR})

# Copy GLSL sources to build directory (for runtime loading via AssetSystem)
set(COPIED_SHADERS "")
foreach(SHADER_SOURCE ${VULKAN_SHADER_SOURCES})
    get_filename_component(SHADER_NAME ${SHADER_SOURCE} NAME)
    set(SHADER_OUTPUT ${SHADER_OUTPUT_DIR}/${SHADER_NAME})
    list(APPEND COPIED_SHADERS ${SHADER_OUTPUT})

    add_custom_command(
        OUTPUT ${SHADER_OUTPUT}
        COMMAND ${CMAKE_COMMAND} -E copy_if_different ${SHADER_SOURCE} ${SHADER_OUTPUT}
        DEPENDS ${SHADER_SOURCE}
        COMMENT "Copying shader source ${SHADER_NAME}"
    )
endforeach()

# Custom target for all shader sources
add_custom_target(bestow-vulkan-shaders ALL DEPENDS ${COPIED_SHADERS})

#=============================================================================
# Library
#=============================================================================

add_library(bestow-vulkan)
target_use_std_module(bestow-vulkan)

# Add dependency on shader compilation
if(TARGET bestow-vulkan-shaders)
    add_dependencies(bestow-vulkan bestow-vulkan-shaders)
endif()

# Define shader path for runtime loading
target_compile_definitions(bestow-vulkan PRIVATE
    BESTOW_VULKAN_SHADER_PATH="${SHADER_OUTPUT_DIR}"
)

target_sources(bestow-vulkan
    PUBLIC FILE_SET CXX_MODULES FILES
        src/bestow.vulkan.cppm
        src/bestow.vulkan.impl.cppm
    PRIVATE
        src/VmaImpl.cpp
        src/VulkanContext.cpp
        src/VulkanGraphicsSystem.cpp
        src/VulkanGraphics3DSystem.cpp
        src/VulkanPipeline.cpp
        src/VulkanBuffer.cpp
        src/VulkanTexture.cpp
)

target_link_libraries(bestow-vulkan
    PUBLIC
        bestow-contract
        kangaru::kangaru
        Vulkan::Vulkan
        GPUOpen::VulkanMemoryAllocator
        vk-bootstrap::vk-bootstrap
        glfw
        glm::glm
    PRIVATE
        lua
        sol2::sol2
)

target_compile_features(bestow-vulkan PUBLIC cxx_std_23)

# VMA_IMPLEMENTATION is defined only in VmaImpl.cpp to avoid duplicate symbols

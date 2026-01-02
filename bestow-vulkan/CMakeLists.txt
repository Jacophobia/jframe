# bestow-vulkan/CMakeLists.txt
# Vulkan rendering backend

find_package(Vulkan REQUIRED)
find_package(VulkanMemoryAllocator CONFIG REQUIRED)
find_package(vk-bootstrap CONFIG REQUIRED)

#=============================================================================
# Shader Loading
#=============================================================================
# NOTE: Shaders are loaded at runtime from the asset-library via PathResolver.
# The :library:/shaders/ path prefix is resolved by PathResolver to the
# asset-library/shaders directory set by the application.
#
# Shaders are compiled to SPIR-V at runtime via AssetSystem using shaderc.
# This enables:
# - Hot reload of shaders during development
# - Single source of truth (GLSL files in asset-library/shaders)
# - No external compiler dependency for end users

#=============================================================================
# Library
#=============================================================================

add_library(bestow-vulkan)
target_use_std_module(bestow-vulkan)

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
        unofficial::luajit::luajit
        sol2::sol2
)

target_compile_features(bestow-vulkan PUBLIC cxx_std_23)

# VMA_IMPLEMENTATION is defined only in VmaImpl.cpp to avoid duplicate symbols

# bestow-vulkan/CMakeLists.txt
# Vulkan rendering backend

find_package(Vulkan REQUIRED)
find_package(VulkanMemoryAllocator CONFIG REQUIRED)
find_package(vk-bootstrap CONFIG REQUIRED)

# Find glslc for runtime shader compilation
find_program(GLSLC_EXECUTABLE glslc HINTS ${Vulkan_SDK}/bin $ENV{VULKAN_SDK}/bin /usr/local/bin)
if(GLSLC_EXECUTABLE)
    message(STATUS "glslc found: ${GLSLC_EXECUTABLE} - runtime GLSL compilation enabled")
    add_compile_definitions(BESTOW_HAS_GLSLC="${GLSLC_EXECUTABLE}")
else()
    message(WARNING "glslc not found - runtime GLSL compilation disabled")
endif()

#=============================================================================
# Shader Compilation
#=============================================================================

# Find glslc or glslangValidator for shader compilation
find_program(GLSLC glslc HINTS ${Vulkan_SDK}/bin $ENV{VULKAN_SDK}/bin)
find_program(GLSLANG_VALIDATOR glslangValidator HINTS ${Vulkan_SDK}/bin $ENV{VULKAN_SDK}/bin)

set(SHADER_COMPILER "")
if(GLSLC)
    set(SHADER_COMPILER ${GLSLC})
    set(SHADER_COMPILE_FLAGS -c)
elseif(GLSLANG_VALIDATOR)
    set(SHADER_COMPILER ${GLSLANG_VALIDATOR})
    set(SHADER_COMPILE_FLAGS -V)
endif()

# Shader source files
set(VULKAN_SHADER_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/shaders/basic3d.vert
    ${CMAKE_CURRENT_SOURCE_DIR}/shaders/basic3d.frag
    ${CMAKE_CURRENT_SOURCE_DIR}/shaders/debug.vert
    ${CMAKE_CURRENT_SOURCE_DIR}/shaders/debug.frag
    ${CMAKE_CURRENT_SOURCE_DIR}/shaders/sprite2d.vert
    ${CMAKE_CURRENT_SOURCE_DIR}/shaders/sprite2d.frag
)

# Output directory for compiled shaders
set(SHADER_OUTPUT_DIR ${CMAKE_CURRENT_BINARY_DIR}/shaders)
file(MAKE_DIRECTORY ${SHADER_OUTPUT_DIR})

# Compile each shader to SPIR-V
set(COMPILED_SHADERS "")
if(SHADER_COMPILER)
    foreach(SHADER_SOURCE ${VULKAN_SHADER_SOURCES})
        get_filename_component(SHADER_NAME ${SHADER_SOURCE} NAME)
        set(SHADER_OUTPUT ${SHADER_OUTPUT_DIR}/${SHADER_NAME}.spv)
        list(APPEND COMPILED_SHADERS ${SHADER_OUTPUT})

        add_custom_command(
            OUTPUT ${SHADER_OUTPUT}
            COMMAND ${SHADER_COMPILER} ${SHADER_COMPILE_FLAGS} ${SHADER_SOURCE} -o ${SHADER_OUTPUT}
            DEPENDS ${SHADER_SOURCE}
            COMMENT "Compiling shader ${SHADER_NAME}"
        )
    endforeach()

    # Custom target for all shaders
    add_custom_target(bestow-vulkan-shaders ALL DEPENDS ${COMPILED_SHADERS})
else()
    message(WARNING "No SPIR-V compiler found. Vulkan shaders will not be compiled.")
endif()

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
        bestow-assets
        bestow-shader
        lua
        sol2::sol2
)

target_compile_features(bestow-vulkan PUBLIC cxx_std_23)

# VMA_IMPLEMENTATION is defined only in VmaImpl.cpp to avoid duplicate symbols

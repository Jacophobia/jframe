// bestow-vulkan/src/bestow.vulkan.cppm
// Vulkan backend types and interfaces

module;

#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>

export module bestow.vulkan;

import std;
import bestow.types;

export namespace bestow::vulkan {

//==========================================================================
// Vulkan Configuration
//==========================================================================

struct VulkanConfig {
    std::string applicationName = "Bestow Application";
    std::uint32_t applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    bool enableValidation = true;  // Enable validation layers for debugging
    bool preferDiscreteGPU = true;
    int windowWidth = 800;
    int windowHeight = 600;
    std::string windowTitle = "Bestow";
    bool vsync = true;
    bool headless = false;  // Enable headless rendering (no window)
    int headlessWidth = 1920;
    int headlessHeight = 1080;
    void* window = nullptr;  // Optional: use existing GLFW window instead of creating one
};

//==========================================================================
// Error Types
//==========================================================================

enum class VulkanError {
    Success,
    InstanceCreationFailed,
    SurfaceCreationFailed,
    NoSuitableGPU,
    DeviceCreationFailed,
    SwapchainCreationFailed,
    RenderPassCreationFailed,
    FramebufferCreationFailed,
    CommandPoolCreationFailed,
    CommandBufferAllocationFailed,
    SyncObjectCreationFailed,
    MemoryAllocationFailed,
    BufferCreationFailed,
    ImageCreationFailed,
    ShaderModuleCreationFailed,
    PipelineCreationFailed,
    DescriptorPoolCreationFailed,
    SamplerCreationFailed,
    ValidationLayersNotAvailable,
    OutOfMemory,
    DeviceLost,
    SurfaceLost,
    Unknown
};

inline std::string_view vulkanErrorToString(VulkanError error) {
    switch (error) {
        case VulkanError::Success: return "Success";
        case VulkanError::InstanceCreationFailed: return "Failed to create Vulkan instance";
        case VulkanError::SurfaceCreationFailed: return "Failed to create window surface";
        case VulkanError::NoSuitableGPU: return "No suitable GPU found";
        case VulkanError::DeviceCreationFailed: return "Failed to create logical device";
        case VulkanError::SwapchainCreationFailed: return "Failed to create swapchain";
        case VulkanError::RenderPassCreationFailed: return "Failed to create render pass";
        case VulkanError::FramebufferCreationFailed: return "Failed to create framebuffer";
        case VulkanError::CommandPoolCreationFailed: return "Failed to create command pool";
        case VulkanError::CommandBufferAllocationFailed: return "Failed to allocate command buffers";
        case VulkanError::SyncObjectCreationFailed: return "Failed to create synchronization objects";
        case VulkanError::MemoryAllocationFailed: return "Failed to allocate memory";
        case VulkanError::BufferCreationFailed: return "Failed to create buffer";
        case VulkanError::ImageCreationFailed: return "Failed to create image";
        case VulkanError::ShaderModuleCreationFailed: return "Failed to create shader module";
        case VulkanError::PipelineCreationFailed: return "Failed to create pipeline";
        case VulkanError::DescriptorPoolCreationFailed: return "Failed to create descriptor pool";
        case VulkanError::SamplerCreationFailed: return "Failed to create sampler";
        case VulkanError::ValidationLayersNotAvailable: return "Validation layers not available";
        case VulkanError::OutOfMemory: return "Out of memory";
        case VulkanError::DeviceLost: return "Device lost";
        case VulkanError::SurfaceLost: return "Surface lost";
        case VulkanError::Unknown: return "Unknown error";
    }
    return "Unknown error";
}

//==========================================================================
// Resource Handles
//==========================================================================

using VulkanBufferHandle = std::uint64_t;
using VulkanImageHandle = std::uint64_t;
using VulkanPipelineHandle = std::uint64_t;
using VulkanDescriptorSetHandle = std::uint64_t;

//==========================================================================
// Buffer Types
//==========================================================================

enum class VulkanBufferUsage : std::uint32_t {
    Vertex = 0x00000001,
    Index = 0x00000002,
    Uniform = 0x00000004,
    Storage = 0x00000008,
    Staging = 0x00000010,
    TransferSrc = 0x00000020,
    TransferDst = 0x00000040
};

inline VulkanBufferUsage operator|(VulkanBufferUsage a, VulkanBufferUsage b) {
    return static_cast<VulkanBufferUsage>(static_cast<std::uint32_t>(a) | static_cast<std::uint32_t>(b));
}

inline bool operator&(VulkanBufferUsage a, VulkanBufferUsage b) {
    return (static_cast<std::uint32_t>(a) & static_cast<std::uint32_t>(b)) != 0;
}

struct VulkanBufferDef {
    std::size_t size = 0;
    VulkanBufferUsage usage = VulkanBufferUsage::Vertex;
    bool hostVisible = false;  // CPU-accessible memory
    bool persistentlyMapped = false;  // Keep buffer mapped
    std::string_view debugName;
};

//==========================================================================
// Image Types
//==========================================================================

enum class VulkanImageUsage : std::uint32_t {
    Sampled = 0x00000001,        // Can be sampled in shader
    Storage = 0x00000002,        // Can be used as storage image
    ColorAttachment = 0x00000004,
    DepthStencilAttachment = 0x00000008,
    TransferSrc = 0x00000010,
    TransferDst = 0x00000020
};

inline VulkanImageUsage operator|(VulkanImageUsage a, VulkanImageUsage b) {
    return static_cast<VulkanImageUsage>(static_cast<std::uint32_t>(a) | static_cast<std::uint32_t>(b));
}

struct VulkanImageDef {
    std::uint32_t width = 1;
    std::uint32_t height = 1;
    std::uint32_t depth = 1;
    std::uint32_t mipLevels = 1;
    std::uint32_t arrayLayers = 1;
    VkFormat format = VK_FORMAT_R8G8B8A8_UNORM;
    VkImageType type = VK_IMAGE_TYPE_2D;
    VulkanImageUsage usage = VulkanImageUsage::Sampled | VulkanImageUsage::TransferDst;
    std::string_view debugName;
};

//==========================================================================
// Pipeline Types
//==========================================================================

struct VulkanShaderStage {
    VkShaderStageFlagBits stage;
    std::span<const std::uint32_t> spirv;  // SPIR-V bytecode
    std::string entryPoint = "main";
};

struct VulkanVertexBinding {
    std::uint32_t binding = 0;
    std::uint32_t stride = 0;
    VkVertexInputRate inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
};

struct VulkanVertexAttribute {
    std::uint32_t location = 0;
    std::uint32_t binding = 0;
    VkFormat format = VK_FORMAT_R32G32B32_SFLOAT;
    std::uint32_t offset = 0;
};

struct VulkanPushConstantRange {
    VkShaderStageFlags stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    std::uint32_t offset = 0;
    std::uint32_t size = 0;
};

struct VulkanPipelineDef {
    std::vector<VulkanShaderStage> shaderStages;
    std::vector<VulkanVertexBinding> vertexBindings;
    std::vector<VulkanVertexAttribute> vertexAttributes;
    std::vector<VulkanPushConstantRange> pushConstantRanges;
    VkPrimitiveTopology topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    VkPolygonMode polygonMode = VK_POLYGON_MODE_FILL;
    VkCullModeFlags cullMode = VK_CULL_MODE_BACK_BIT;
    VkFrontFace frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    bool depthTestEnable = true;
    bool depthWriteEnable = true;
    VkCompareOp depthCompareOp = VK_COMPARE_OP_LESS;
    bool blendEnable = false;
    VkBlendFactor srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    VkBlendFactor dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    VkBlendOp colorBlendOp = VK_BLEND_OP_ADD;
    std::string_view debugName;
};

//==========================================================================
// Statistics
//==========================================================================

struct VulkanStats {
    std::uint64_t totalAllocatedMemory = 0;
    std::uint64_t usedMemory = 0;
    std::uint32_t allocationCount = 0;
    std::uint32_t bufferCount = 0;
    std::uint32_t imageCount = 0;
    std::uint32_t descriptorSetCount = 0;
    std::uint32_t pipelineCount = 0;
    double gpuFrameTimeMs = 0.0;
};

}  // namespace bestow::vulkan

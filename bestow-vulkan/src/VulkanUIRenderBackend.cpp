// bestow-vulkan/src/VulkanUIRenderBackend.cpp
// Vulkan implementation of IUIRenderBackend for 2D UI rendering
//
// Uses AssetSystem for runtime GLSL-to-SPIR-V shader compilation with hot reload.
// Follows the same pattern as VulkanGraphics3DSystem for pipeline/shader management.

module;

#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <spdlog/spdlog.h>

#include <stb_image.h>

module bestow.vulkan.impl;

import std;
import bestow.vulkan;
import bestow.services;

namespace bestow::vulkan {

//==========================================================================
// Push constant layout (must match ui.vert / ui.frag)
//==========================================================================

struct UIPushConstants {
    glm::mat4 projection;     // 64 bytes
    glm::vec2 translation;    //  8 bytes
    int hasTexture;            //  4 bytes
    int _pad;                  //  4 bytes (alignment)
};  // Total: 80 bytes

//==========================================================================
// Constructor / Destructor
//==========================================================================

VulkanUIRenderBackend::VulkanUIRenderBackend(VulkanContext* context, IAssetSystem* assetSystem)
    : context_(context), assetSystem_(assetSystem) {}

VulkanUIRenderBackend::~VulkanUIRenderBackend() {
    shutdown();
}

//==========================================================================
// Lifecycle
//==========================================================================

bool VulkanUIRenderBackend::initialize() {
    if (initialized_) {
        return true;
    }

    if (!context_ || !context_->isInitialized()) {
        spdlog::error("VulkanUIRenderBackend: VulkanContext not initialized");
        return false;
    }

    if (!assetSystem_) {
        spdlog::error("VulkanUIRenderBackend: AssetSystem not provided");
        return false;
    }

    viewportWidth_ = static_cast<int>(context_->getSwapchainExtent().width);
    viewportHeight_ = static_cast<int>(context_->getSwapchainExtent().height);

    // Create descriptor set layout and pool for texture binding
    if (!createDescriptorResources()) {
        spdlog::error("VulkanUIRenderBackend: Failed to create descriptor resources");
        return false;
    }

    // Create 1x1 white texture (fallback when no texture is bound)
    if (!createWhiteTexture()) {
        spdlog::error("VulkanUIRenderBackend: Failed to create white texture");
        return false;
    }

    // Create the UI pipeline (loads shaders via AssetSystem)
    if (!createUIPipeline()) {
        spdlog::error("VulkanUIRenderBackend: Failed to create UI pipeline");
        return false;
    }

    // Subscribe to shader hot reload
    if (assetSystem_) {
        shaderSubscriptionId_ = assetSystem_->subscribeToType(
            AssetType::Shader,
            [this](AssetHandle handle, AssetType type) {
                onShaderAssetChanged(handle, type);
            });
        spdlog::info("[Vulkan] Subscribed to shader asset changes for UI hot reload");
    }

    initialized_ = true;
    spdlog::info("[Vulkan] UI render backend initialized ({}x{})", viewportWidth_, viewportHeight_);
    return true;
}

void VulkanUIRenderBackend::shutdown() {
    if (!initialized_) {
        return;
    }

    if (context_ && context_->isInitialized()) {
        auto device = context_->getDevice();
        vkDeviceWaitIdle(device);

        // Unsubscribe from hot reload
        if (assetSystem_ && shaderSubscriptionId_ != 0) {
            assetSystem_->unsubscribe(shaderSubscriptionId_);
            shaderSubscriptionId_ = 0;
        }

        // Release all geometry
        for (auto& [handle, geom] : geometryCache_) {
            context_->destroyBuffer(geom.vertexBufferHandle);
            context_->destroyBuffer(geom.indexBufferHandle);
        }
        geometryCache_.clear();

        // Release all textures (descriptor sets freed when pool is destroyed)
        for (auto& [handle, tex] : textureCache_) {
            context_->destroyImage(tex.imageHandle);
        }
        textureCache_.clear();

        // Release white texture
        if (whiteTextureImage_ != 0) {
            context_->destroyImage(whiteTextureImage_);
            whiteTextureImage_ = 0;
        }
        whiteTextureDescSet_ = VK_NULL_HANDLE;

        // Destroy pipeline
        if (uiPipeline_ != 0) {
            context_->destroyPipeline(uiPipeline_);
            uiPipeline_ = 0;
        }

        // Destroy descriptor pool (frees all descriptor sets)
        if (textureDescPool_ != VK_NULL_HANDLE) {
            vkDestroyDescriptorPool(device, textureDescPool_, nullptr);
            textureDescPool_ = VK_NULL_HANDLE;
        }

        // Destroy descriptor set layout
        if (textureDescSetLayout_ != VK_NULL_HANDLE) {
            vkDestroyDescriptorSetLayout(device, textureDescSetLayout_, nullptr);
            textureDescSetLayout_ = VK_NULL_HANDLE;
        }
    }

    initialized_ = false;
    spdlog::info("[Vulkan] UI render backend shut down");
}

bool VulkanUIRenderBackend::isInitialized() const {
    return initialized_;
}

//==========================================================================
// Descriptor Resources
//==========================================================================

bool VulkanUIRenderBackend::createDescriptorResources() {
    auto device = context_->getDevice();

    // Layout: one combined image sampler at binding 0
    VkDescriptorSetLayoutBinding samplerBinding{};
    samplerBinding.binding = 0;
    samplerBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    samplerBinding.descriptorCount = 1;
    samplerBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = 1;
    layoutInfo.pBindings = &samplerBinding;

    if (vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &textureDescSetLayout_) != VK_SUCCESS) {
        spdlog::error("VulkanUIRenderBackend: Failed to create descriptor set layout");
        return false;
    }

    // Pool: enough for many texture descriptor sets
    VkDescriptorPoolSize poolSize{};
    poolSize.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    poolSize.descriptorCount = 256;

    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    poolInfo.maxSets = 256;
    poolInfo.poolSizeCount = 1;
    poolInfo.pPoolSizes = &poolSize;

    if (vkCreateDescriptorPool(device, &poolInfo, nullptr, &textureDescPool_) != VK_SUCCESS) {
        spdlog::error("VulkanUIRenderBackend: Failed to create descriptor pool");
        return false;
    }

    return true;
}

VkDescriptorSet VulkanUIRenderBackend::allocateTextureDescriptorSet(VulkanImageHandle imageHandle) {
    auto device = context_->getDevice();

    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = textureDescPool_;
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts = &textureDescSetLayout_;

    VkDescriptorSet descSet = VK_NULL_HANDLE;
    if (vkAllocateDescriptorSets(device, &allocInfo, &descSet) != VK_SUCCESS) {
        spdlog::error("VulkanUIRenderBackend: Failed to allocate texture descriptor set");
        return VK_NULL_HANDLE;
    }

    // Update with image view and sampler
    VkDescriptorImageInfo imageInfo{};
    imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    imageInfo.imageView = context_->getImageView(imageHandle);
    imageInfo.sampler = context_->getImageSampler(imageHandle);

    VkWriteDescriptorSet write{};
    write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write.dstSet = descSet;
    write.dstBinding = 0;
    write.dstArrayElement = 0;
    write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    write.descriptorCount = 1;
    write.pImageInfo = &imageInfo;

    vkUpdateDescriptorSets(device, 1, &write, 0, nullptr);
    return descSet;
}

//==========================================================================
// White Texture
//==========================================================================

bool VulkanUIRenderBackend::createWhiteTexture() {
    // 1x1 white pixel (RGBA)
    std::uint8_t whitePixel[] = {255, 255, 255, 255};

    VulkanImageDef imageDef{
        .width = 1,
        .height = 1,
        .depth = 1,
        .mipLevels = 1,
        .arrayLayers = 1,
        .format = VK_FORMAT_R8G8B8A8_UNORM,
        .type = VK_IMAGE_TYPE_2D,
        .usage = VulkanImageUsage::Sampled | VulkanImageUsage::TransferDst,
        .isCubemap = false,
        .debugName = "UI White Texture"
    };

    auto result = context_->createImage(imageDef);
    if (!result) {
        return false;
    }

    whiteTextureImage_ = *result;
    context_->uploadToImage(whiteTextureImage_, whitePixel, sizeof(whitePixel));

    whiteTextureDescSet_ = allocateTextureDescriptorSet(whiteTextureImage_);
    return whiteTextureDescSet_ != VK_NULL_HANDLE;
}

//==========================================================================
// Pipeline Creation
//==========================================================================

bool VulkanUIRenderBackend::createUIPipeline() {
    // Load shaders via AssetSystem (runtime GLSL -> SPIR-V compilation)
    auto vertHandle = assetSystem_->loadShaderCompiled(":library:/shaders/ui.vert");
    auto fragHandle = assetSystem_->loadShaderCompiled(":library:/shaders/ui.frag");

    if (!assetSystem_->isLoaded(vertHandle) || !assetSystem_->isLoaded(fragHandle)) {
        spdlog::error("VulkanUIRenderBackend: Failed to load UI shaders");
        return false;
    }

    const ShaderData* vertShader = assetSystem_->getShaderData(vertHandle);
    const ShaderData* fragShader = assetSystem_->getShaderData(fragHandle);

    if (!vertShader || !fragShader ||
        vertShader->spirvBytecode.empty() || fragShader->spirvBytecode.empty()) {
        spdlog::error("VulkanUIRenderBackend: UI shaders have no SPIR-V bytecode");
        return false;
    }

    // Store asset handles for hot reload
    pipelineShaders_.vertShaderAsset = vertHandle;
    pipelineShaders_.fragShaderAsset = fragHandle;

    // UIVertex layout: vec2 position (8b) + Color RGBA8 (4b) + vec2 texCoord (8b) = 20 bytes
    VulkanPipelineDef def;
    def.shaderStages = {
        {VK_SHADER_STAGE_VERTEX_BIT, vertShader->spirvBytecode, "main"},
        {VK_SHADER_STAGE_FRAGMENT_BIT, fragShader->spirvBytecode, "main"}
    };

    def.vertexBindings = {
        {0, sizeof(UIVertex), VK_VERTEX_INPUT_RATE_VERTEX}
    };

    def.vertexAttributes = {
        {0, 0, VK_FORMAT_R32G32_SFLOAT, 0},                              // position (vec2)
        {1, 0, VK_FORMAT_R8G8B8A8_UNORM, static_cast<std::uint32_t>(offsetof(UIVertex, color))},  // color (RGBA8)
        {2, 0, VK_FORMAT_R32G32_SFLOAT, static_cast<std::uint32_t>(offsetof(UIVertex, texCoord))}  // texCoord (vec2)
    };

    def.pushConstantRanges = {
        {VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(UIPushConstants)}
    };

    def.descriptorSetLayouts = {textureDescSetLayout_};

    def.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    def.cullMode = VK_CULL_MODE_NONE;      // UI can have any winding
    def.depthTestEnable = false;            // UI always on top
    def.depthWriteEnable = false;
    def.blendEnable = true;
    def.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;                    // Premultiplied alpha
    def.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    def.colorBlendOp = VK_BLEND_OP_ADD;
    def.debugName = "UI Pipeline";

    auto result = context_->createPipeline(def);
    if (!result) {
        spdlog::error("VulkanUIRenderBackend: Failed to create UI pipeline");
        return false;
    }

    uiPipeline_ = *result;
    spdlog::info("[Vulkan] Created UI pipeline");
    return true;
}

//==========================================================================
// Shader Hot Reload
//==========================================================================

void VulkanUIRenderBackend::onShaderAssetChanged(AssetHandle handle, AssetType type) {
    // Check if the changed asset is one of our UI shaders
    if (handle.uuid != pipelineShaders_.vertShaderAsset.uuid &&
        handle.uuid != pipelineShaders_.fragShaderAsset.uuid) {
        return;  // Not our shader
    }

    spdlog::info("[Vulkan] UI shader changed, reloading pipeline...");
    reloadPipelineShaders();
}

void VulkanUIRenderBackend::reloadPipelineShaders() {
    if (!assetSystem_ || !context_) return;

    auto device = context_->getDevice();
    vkDeviceWaitIdle(device);

    // Reload both shaders
    assetSystem_->reloadAsset(pipelineShaders_.vertShaderAsset);
    assetSystem_->reloadAsset(pipelineShaders_.fragShaderAsset);

    const ShaderData* vertShader = assetSystem_->getShaderData(pipelineShaders_.vertShaderAsset);
    const ShaderData* fragShader = assetSystem_->getShaderData(pipelineShaders_.fragShaderAsset);

    if (!vertShader || !fragShader ||
        vertShader->spirvBytecode.empty() || fragShader->spirvBytecode.empty()) {
        spdlog::error("VulkanUIRenderBackend: Hot reload failed - shader compilation error");
        return;
    }

    // Destroy old pipeline
    if (uiPipeline_ != 0) {
        context_->destroyPipeline(uiPipeline_);
        uiPipeline_ = 0;
    }

    // Recreate pipeline with new shaders
    VulkanPipelineDef def;
    def.shaderStages = {
        {VK_SHADER_STAGE_VERTEX_BIT, vertShader->spirvBytecode, "main"},
        {VK_SHADER_STAGE_FRAGMENT_BIT, fragShader->spirvBytecode, "main"}
    };
    def.vertexBindings = {{0, sizeof(UIVertex), VK_VERTEX_INPUT_RATE_VERTEX}};
    def.vertexAttributes = {
        {0, 0, VK_FORMAT_R32G32_SFLOAT, 0},
        {1, 0, VK_FORMAT_R8G8B8A8_UNORM, static_cast<std::uint32_t>(offsetof(UIVertex, color))},
        {2, 0, VK_FORMAT_R32G32_SFLOAT, static_cast<std::uint32_t>(offsetof(UIVertex, texCoord))}
    };
    def.pushConstantRanges = {
        {VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(UIPushConstants)}
    };
    def.descriptorSetLayouts = {textureDescSetLayout_};
    def.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    def.cullMode = VK_CULL_MODE_NONE;
    def.depthTestEnable = false;
    def.depthWriteEnable = false;
    def.blendEnable = true;
    def.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
    def.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    def.colorBlendOp = VK_BLEND_OP_ADD;
    def.debugName = "UI Pipeline (hot-reloaded)";

    auto result = context_->createPipeline(def);
    if (result) {
        uiPipeline_ = *result;
        spdlog::info("[Vulkan] UI pipeline hot-reloaded successfully");
    } else {
        spdlog::error("VulkanUIRenderBackend: Failed to recreate pipeline during hot reload");
    }
}

//==========================================================================
// Geometry Management
//==========================================================================

UIGeometryHandle VulkanUIRenderBackend::compileGeometry(
    std::span<const UIVertex> vertices,
    std::span<const std::uint32_t> indices) {

    if (!initialized_ || vertices.empty() || indices.empty()) {
        return InvalidUIGeometry;
    }

    GeometryResource geom{};
    geom.indexCount = static_cast<std::uint32_t>(indices.size());

    // Create vertex buffer
    VulkanBufferDef vertexBufferDef{
        .size = vertices.size() * sizeof(UIVertex),
        .usage = VulkanBufferUsage::Vertex | VulkanBufferUsage::TransferDst,
        .hostVisible = false,
        .persistentlyMapped = false,
        .debugName = "UI Vertex Buffer"
    };

    auto vertexResult = context_->createBuffer(vertexBufferDef);
    if (!vertexResult) {
        spdlog::error("VulkanUIRenderBackend: Failed to create vertex buffer");
        return InvalidUIGeometry;
    }

    geom.vertexBufferHandle = *vertexResult;
    geom.vertexBuffer = context_->getBuffer(geom.vertexBufferHandle);

    // Create index buffer
    VulkanBufferDef indexBufferDef{
        .size = indices.size() * sizeof(std::uint32_t),
        .usage = VulkanBufferUsage::Index | VulkanBufferUsage::TransferDst,
        .hostVisible = false,
        .persistentlyMapped = false,
        .debugName = "UI Index Buffer"
    };

    auto indexResult = context_->createBuffer(indexBufferDef);
    if (!indexResult) {
        spdlog::error("VulkanUIRenderBackend: Failed to create index buffer");
        context_->destroyBuffer(geom.vertexBufferHandle);
        return InvalidUIGeometry;
    }

    geom.indexBufferHandle = *indexResult;
    geom.indexBuffer = context_->getBuffer(geom.indexBufferHandle);

    // Upload data
    context_->uploadToBuffer(geom.vertexBufferHandle, vertices.data(),
                             vertices.size() * sizeof(UIVertex));
    context_->uploadToBuffer(geom.indexBufferHandle, indices.data(),
                             indices.size() * sizeof(std::uint32_t));

    UIGeometryHandle handle = nextGeometryHandle_++;
    geometryCache_[handle] = geom;
    return handle;
}

void VulkanUIRenderBackend::releaseGeometry(UIGeometryHandle geometry) {
    auto it = geometryCache_.find(geometry);
    if (it != geometryCache_.end()) {
        context_->destroyBuffer(it->second.vertexBufferHandle);
        context_->destroyBuffer(it->second.indexBufferHandle);
        geometryCache_.erase(it);
    }
}

//==========================================================================
// Rendering
//==========================================================================

void VulkanUIRenderBackend::beginUIPass() {
    if (!initialized_ || uiPipeline_ == 0) {
        return;
    }

    auto cmd = context_->getCurrentCommandBuffer();
    if (cmd == VK_NULL_HANDLE) {
        return;
    }

    inUIPass_ = true;
    drawCallCount_ = 0;
    triangleCount_ = 0;

    // Bind the UI pipeline
    context_->bindPipeline(uiPipeline_);

    // Set up orthographic projection (top-left origin, pixel coordinates)
    UIPushConstants pc{};
    pc.projection = glm::ortho(
        0.0f, static_cast<float>(viewportWidth_),
        static_cast<float>(viewportHeight_), 0.0f,   // flipped Y: top=0, bottom=height
        -1.0f, 1.0f);
    pc.translation = {0.0f, 0.0f};
    pc.hasTexture = 0;

    VkPipelineLayout layout = context_->getPipelineLayout(uiPipeline_);
    vkCmdPushConstants(cmd, layout,
                       VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                       0, sizeof(UIPushConstants), &pc);

    // Reset scissor to full viewport
    VkRect2D fullScissor{};
    fullScissor.offset = {0, 0};
    fullScissor.extent = {static_cast<uint32_t>(viewportWidth_),
                          static_cast<uint32_t>(viewportHeight_)};
    vkCmdSetScissor(cmd, 0, 1, &fullScissor);
}

void VulkanUIRenderBackend::renderGeometry(
    UIGeometryHandle geometry,
    Vec2 translation,
    UITextureHandle texture) {

    if (!initialized_ || !inUIPass_ || uiPipeline_ == 0) {
        return;
    }

    auto it = geometryCache_.find(geometry);
    if (it == geometryCache_.end()) {
        return;
    }

    auto cmd = context_->getCurrentCommandBuffer();
    if (cmd == VK_NULL_HANDLE) {
        return;
    }

    const auto& geom = it->second;
    VkPipelineLayout layout = context_->getPipelineLayout(uiPipeline_);

    // Update push constants for this draw: translation + hasTexture
    UIPushConstants pc{};
    pc.projection = glm::ortho(
        0.0f, static_cast<float>(viewportWidth_),
        static_cast<float>(viewportHeight_), 0.0f,
        -1.0f, 1.0f);
    pc.translation = {translation.x, translation.y};
    pc.hasTexture = (texture != InvalidUITexture) ? 1 : 0;

    vkCmdPushConstants(cmd, layout,
                       VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                       0, sizeof(UIPushConstants), &pc);

    // Bind texture descriptor set
    VkDescriptorSet texDescSet = whiteTextureDescSet_;
    if (texture != InvalidUITexture) {
        auto texIt = textureCache_.find(texture);
        if (texIt != textureCache_.end() && texIt->second.descriptorSet != VK_NULL_HANDLE) {
            texDescSet = texIt->second.descriptorSet;
        }
    }

    if (texDescSet != VK_NULL_HANDLE) {
        vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
                                layout, 0, 1, &texDescSet, 0, nullptr);
    }

    // Bind vertex buffer
    VkDeviceSize offset = 0;
    vkCmdBindVertexBuffers(cmd, 0, 1, &geom.vertexBuffer, &offset);

    // Bind index buffer and draw
    vkCmdBindIndexBuffer(cmd, geom.indexBuffer, 0, VK_INDEX_TYPE_UINT32);
    vkCmdDrawIndexed(cmd, geom.indexCount, 1, 0, 0, 0);

    drawCallCount_++;
    triangleCount_ += geom.indexCount / 3;
}

void VulkanUIRenderBackend::endUIPass() {
    inUIPass_ = false;
}

//==========================================================================
// Texture Management
//==========================================================================

UITextureHandle VulkanUIRenderBackend::loadTexture(
    const std::filesystem::path& path,
    int& outWidth,
    int& outHeight) {

    if (!initialized_) {
        return InvalidUITexture;
    }

    // Load image data with stb_image
    int width = 0, height = 0, channels = 0;
    stbi_uc* pixels = stbi_load(path.string().c_str(), &width, &height, &channels, 4);
    if (!pixels) {
        spdlog::error("VulkanUIRenderBackend: Failed to load texture: {}", path.string());
        outWidth = 0;
        outHeight = 0;
        return InvalidUITexture;
    }

    std::span<const std::uint8_t> data(pixels, width * height * 4);
    auto handle = createTexture(data, width, height);

    stbi_image_free(pixels);

    outWidth = width;
    outHeight = height;
    return handle;
}

UITextureHandle VulkanUIRenderBackend::createTexture(
    std::span<const std::uint8_t> data,
    int width,
    int height) {

    if (!initialized_ || data.empty() || width <= 0 || height <= 0) {
        return InvalidUITexture;
    }

    // Create Vulkan image
    VulkanImageDef imageDef{
        .width = static_cast<std::uint32_t>(width),
        .height = static_cast<std::uint32_t>(height),
        .depth = 1,
        .mipLevels = 1,
        .arrayLayers = 1,
        .format = VK_FORMAT_R8G8B8A8_UNORM,
        .type = VK_IMAGE_TYPE_2D,
        .usage = VulkanImageUsage::Sampled | VulkanImageUsage::TransferDst,
        .isCubemap = false,
        .debugName = "UI Texture"
    };

    auto result = context_->createImage(imageDef);
    if (!result) {
        spdlog::error("VulkanUIRenderBackend: Failed to create texture image");
        return InvalidUITexture;
    }

    TextureResource tex{};
    tex.imageHandle = *result;
    tex.width = width;
    tex.height = height;

    // Upload texture data
    context_->uploadToImage(tex.imageHandle, data.data(), data.size());

    // Allocate descriptor set for this texture
    tex.descriptorSet = allocateTextureDescriptorSet(tex.imageHandle);
    if (tex.descriptorSet == VK_NULL_HANDLE) {
        context_->destroyImage(tex.imageHandle);
        return InvalidUITexture;
    }

    UITextureHandle handle = nextTextureHandle_++;
    textureCache_[handle] = tex;
    return handle;
}

void VulkanUIRenderBackend::releaseTexture(UITextureHandle texture) {
    auto it = textureCache_.find(texture);
    if (it != textureCache_.end()) {
        // Free descriptor set back to pool
        if (it->second.descriptorSet != VK_NULL_HANDLE) {
            vkFreeDescriptorSets(context_->getDevice(), textureDescPool_,
                                 1, &it->second.descriptorSet);
        }
        context_->destroyImage(it->second.imageHandle);
        textureCache_.erase(it);
    }
}

//==========================================================================
// Scissor (Clipping)
//==========================================================================

void VulkanUIRenderBackend::enableScissor(bool enable) {
    scissorEnabled_ = enable;

    if (!initialized_ || !inUIPass_) return;

    // When disabling scissor, reset to full viewport
    if (!enable) {
        auto cmd = context_->getCurrentCommandBuffer();
        if (cmd != VK_NULL_HANDLE) {
            VkRect2D fullScissor{};
            fullScissor.offset = {0, 0};
            fullScissor.extent = {static_cast<uint32_t>(viewportWidth_),
                                  static_cast<uint32_t>(viewportHeight_)};
            vkCmdSetScissor(cmd, 0, 1, &fullScissor);
        }
    }
}

void VulkanUIRenderBackend::setScissorRegion(const UIScissorRect& region) {
    if (!initialized_ || !inUIPass_) {
        return;
    }

    auto cmd = context_->getCurrentCommandBuffer();
    if (cmd == VK_NULL_HANDLE) {
        return;
    }

    VkRect2D scissor{};
    scissor.offset.x = std::max(0, region.x);
    scissor.offset.y = std::max(0, region.y);
    scissor.extent.width = static_cast<uint32_t>(std::max(0, region.width));
    scissor.extent.height = static_cast<uint32_t>(std::max(0, region.height));

    vkCmdSetScissor(cmd, 0, 1, &scissor);
}

//==========================================================================
// Viewport
//==========================================================================

void VulkanUIRenderBackend::setViewportSize(int width, int height) {
    viewportWidth_ = width;
    viewportHeight_ = height;
}

Size VulkanUIRenderBackend::getViewportSize() const {
    return Size{viewportWidth_, viewportHeight_};
}

//==========================================================================
// Statistics
//==========================================================================

std::uint32_t VulkanUIRenderBackend::getDrawCallCount() const {
    return drawCallCount_;
}

std::uint32_t VulkanUIRenderBackend::getTriangleCount() const {
    return triangleCount_;
}

}  // namespace bestow::vulkan

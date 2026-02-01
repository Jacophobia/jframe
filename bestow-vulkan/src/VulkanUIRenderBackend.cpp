// bestow-vulkan/src/VulkanUIRenderBackend.cpp
// Vulkan implementation of IUIRenderBackend for 2D UI rendering.
// Uses AssetSystem for shader loading/compilation and hot reload support.

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
// VulkanUIRenderBackend Implementation
//==========================================================================

VulkanUIRenderBackend::VulkanUIRenderBackend(VulkanContext* context, IAssetSystem* assetSystem)
    : context_(context), assetSystem_(assetSystem) {}

VulkanUIRenderBackend::~VulkanUIRenderBackend() {
    shutdown();
}

bool VulkanUIRenderBackend::initialize() {
    if (initialized_) {
        return true;
    }

    if (!context_ || !context_->isInitialized()) {
        spdlog::error("VulkanUIRenderBackend: VulkanContext not initialized");
        return false;
    }

    if (!assetSystem_) {
        spdlog::error("VulkanUIRenderBackend: AssetSystem is required for shader loading");
        return false;
    }

    viewportWidth_ = static_cast<int>(context_->getSwapchainExtent().width);
    viewportHeight_ = static_cast<int>(context_->getSwapchainExtent().height);

    // Create descriptor set layout and pool for texture binding
    if (!createDescriptorResources()) {
        spdlog::error("VulkanUIRenderBackend: Failed to create descriptor resources");
        return false;
    }

    // Create 1x1 white texture for solid color rendering
    if (!createWhiteTexture()) {
        spdlog::error("VulkanUIRenderBackend: Failed to create white texture");
        return false;
    }

    // Load shaders and create pipeline via AssetSystem
    if (!createUIPipeline()) {
        spdlog::error("VulkanUIRenderBackend: Failed to create UI pipeline");
        return false;
    }

    // Subscribe to shader asset changes for hot reload
    if (hotReloadEnabled_ && assetSystem_) {
        shaderSubscriptionId_ = assetSystem_->subscribeToType(
            AssetType::Shader,
            [this](AssetHandle handle, AssetType type) {
                onShaderAssetChanged(handle, type);
            }
        );
        assetSystem_->enableHotReload(true);
        spdlog::info("VulkanUIRenderBackend: Subscribed to shader hot reload");
    }

    initialized_ = true;
    spdlog::info("VulkanUIRenderBackend: Initialized successfully");
    return true;
}

void VulkanUIRenderBackend::shutdown() {
    if (!initialized_) {
        return;
    }

    if (context_ && context_->isInitialized()) {
        auto device = context_->getDevice();

        // Unsubscribe from hot reload
        if (shaderSubscriptionId_ != InvalidSubscriptionId && assetSystem_) {
            assetSystem_->unsubscribe(shaderSubscriptionId_);
            shaderSubscriptionId_ = InvalidSubscriptionId;
        }

        // Release all geometry
        for (auto& [handle, geom] : geometryCache_) {
            if (geom.vertexBuffer != VK_NULL_HANDLE) {
                context_->destroyBuffer(geom.vertexBufferHandle);
            }
            if (geom.indexBuffer != VK_NULL_HANDLE) {
                context_->destroyBuffer(geom.indexBufferHandle);
            }
        }
        geometryCache_.clear();

        // Release all textures
        for (auto& [handle, tex] : textureCache_) {
            context_->destroyImage(tex.imageHandle);
        }
        textureCache_.clear();
        textureDescriptorSets_.clear();

        // Destroy white texture
        if (whiteTexture_ != 0) {
            context_->destroyImage(whiteTexture_);
            whiteTexture_ = 0;
        }

        // Destroy pipeline
        if (uiPipeline_ != 0) {
            context_->destroyPipeline(uiPipeline_);
            uiPipeline_ = 0;
        }
        pipelineShaderFiles_.clear();
        shaderAssetToPipelines_.clear();

        // Destroy descriptor pool (frees all descriptor sets allocated from it)
        if (textureDescriptorPool_ != VK_NULL_HANDLE) {
            vkDestroyDescriptorPool(device, textureDescriptorPool_, nullptr);
            textureDescriptorPool_ = VK_NULL_HANDLE;
        }
        whiteTextureDescriptorSet_ = VK_NULL_HANDLE;

        // Destroy descriptor set layout
        if (textureDescriptorSetLayout_ != VK_NULL_HANDLE) {
            vkDestroyDescriptorSetLayout(device, textureDescriptorSetLayout_, nullptr);
            textureDescriptorSetLayout_ = VK_NULL_HANDLE;
        }
    }

    initialized_ = false;
    spdlog::info("VulkanUIRenderBackend: Shutdown complete");
}

bool VulkanUIRenderBackend::isInitialized() const {
    return initialized_;
}

//==========================================================================
// Descriptor Resources
//==========================================================================

bool VulkanUIRenderBackend::createDescriptorResources() {
    auto device = context_->getDevice();

    // Create descriptor set layout: one combined image sampler at binding 0
    VkDescriptorSetLayoutBinding texBinding{};
    texBinding.binding = 0;
    texBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    texBinding.descriptorCount = 1;
    texBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    texBinding.pImmutableSamplers = nullptr;

    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = 1;
    layoutInfo.pBindings = &texBinding;

    if (vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &textureDescriptorSetLayout_) != VK_SUCCESS) {
        spdlog::error("VulkanUIRenderBackend: Failed to create descriptor set layout");
        return false;
    }

    // Create descriptor pool (enough for many UI textures + white texture)
    VkDescriptorPoolSize poolSize{};
    poolSize.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    poolSize.descriptorCount = 128;

    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = 1;
    poolInfo.pPoolSizes = &poolSize;
    poolInfo.maxSets = 128;
    poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;

    if (vkCreateDescriptorPool(device, &poolInfo, nullptr, &textureDescriptorPool_) != VK_SUCCESS) {
        spdlog::error("VulkanUIRenderBackend: Failed to create descriptor pool");
        return false;
    }

    return true;
}

//==========================================================================
// White Texture
//==========================================================================

bool VulkanUIRenderBackend::createWhiteTexture() {
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

    whiteTexture_ = *result;

    // Upload white pixel
    std::uint8_t whitePixel[4] = {255, 255, 255, 255};
    context_->uploadToImage(whiteTexture_, whitePixel, 4);

    // Allocate descriptor set for white texture
    whiteTextureDescriptorSet_ = allocateTextureDescriptorSet(whiteTexture_);
    if (whiteTextureDescriptorSet_ == VK_NULL_HANDLE) {
        return false;
    }

    return true;
}

VkDescriptorSet VulkanUIRenderBackend::allocateTextureDescriptorSet(VulkanImageHandle imageHandle) {
    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = textureDescriptorPool_;
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts = &textureDescriptorSetLayout_;

    VkDescriptorSet descriptorSet = VK_NULL_HANDLE;
    if (vkAllocateDescriptorSets(context_->getDevice(), &allocInfo, &descriptorSet) != VK_SUCCESS) {
        spdlog::error("VulkanUIRenderBackend: Failed to allocate texture descriptor set");
        return VK_NULL_HANDLE;
    }

    // Update descriptor set with image view and sampler
    VkDescriptorImageInfo imageInfo{};
    imageInfo.sampler = context_->getImageSampler(imageHandle);
    imageInfo.imageView = context_->getImageView(imageHandle);
    imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    VkWriteDescriptorSet write{};
    write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write.dstSet = descriptorSet;
    write.dstBinding = 0;
    write.dstArrayElement = 0;
    write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    write.descriptorCount = 1;
    write.pImageInfo = &imageInfo;

    vkUpdateDescriptorSets(context_->getDevice(), 1, &write, 0, nullptr);

    return descriptorSet;
}

//==========================================================================
// Pipeline Creation (via AssetSystem)
//==========================================================================

bool VulkanUIRenderBackend::createUIPipeline() {
    // Load UI shaders through AssetSystem with SPIR-V compilation
    auto vertHandle = assetSystem_->loadShaderCompiled(":library:/shaders/ui.vert");
    auto fragHandle = assetSystem_->loadShaderCompiled(":library:/shaders/ui.frag");

    if (!assetSystem_->isLoaded(vertHandle) || !assetSystem_->isLoaded(fragHandle)) {
        spdlog::error("VulkanUIRenderBackend: Failed to load UI shaders");
        return false;
    }

    const ShaderData* vertShader = assetSystem_->getShaderData(vertHandle);
    const ShaderData* fragShader = assetSystem_->getShaderData(fragHandle);

    if (!vertShader || !fragShader) {
        spdlog::error("VulkanUIRenderBackend: Failed to get shader data");
        return false;
    }

    if (vertShader->spirvBytecode.empty() || fragShader->spirvBytecode.empty()) {
        spdlog::error("VulkanUIRenderBackend: Shader compilation failed");
        if (!vertShader->compileError.empty()) {
            spdlog::error("  Vertex: {}", vertShader->compileError);
        }
        if (!fragShader->compileError.empty()) {
            spdlog::error("  Fragment: {}", fragShader->compileError);
        }
        return false;
    }

    VulkanPipelineDef def;
    def.shaderStages = {
        {VK_SHADER_STAGE_VERTEX_BIT, vertShader->spirvBytecode, "main"},
        {VK_SHADER_STAGE_FRAGMENT_BIT, fragShader->spirvBytecode, "main"}
    };

    // UI vertex layout: position (vec2) + color (RGBA8 unorm) + texcoord (vec2)
    def.vertexBindings = {
        {0, sizeof(UIVertex), VK_VERTEX_INPUT_RATE_VERTEX}
    };
    def.vertexAttributes = {
        {0, 0, VK_FORMAT_R32G32_SFLOAT, static_cast<std::uint32_t>(offsetof(UIVertex, position))},
        {1, 0, VK_FORMAT_R8G8B8A8_UNORM, static_cast<std::uint32_t>(offsetof(UIVertex, color))},
        {2, 0, VK_FORMAT_R32G32_SFLOAT, static_cast<std::uint32_t>(offsetof(UIVertex, texCoord))}
    };

    // Push constants: mat4 projection (64) + vec2 translation (8) + int hasTexture (4) = 76 bytes
    def.pushConstantRanges = {
        {VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, 76}
    };

    // Descriptor set layout for texture binding
    def.descriptorSetLayouts = {textureDescriptorSetLayout_};

    // UI rendering state: no depth, premultiplied alpha blending, no culling
    def.depthTestEnable = false;
    def.depthWriteEnable = false;
    def.blendEnable = true;
    def.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;  // Premultiplied alpha
    def.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    def.colorBlendOp = VK_BLEND_OP_ADD;
    def.cullMode = VK_CULL_MODE_NONE;

    auto result = context_->createPipeline(def);
    if (!result) {
        spdlog::error("VulkanUIRenderBackend: Failed to create UI pipeline");
        return false;
    }

    uiPipeline_ = *result;

    // Track shaders for hot reload (follows VulkanGraphics3DSystem pattern)
    if (hotReloadEnabled_ && assetSystem_) {
        ShaderFileInfo info;
        info.vertGlslPath = ":library:/shaders/ui.vert";
        info.fragGlslPath = ":library:/shaders/ui.frag";

        // Register vertex shader for hot reload tracking
        info.vertShaderAsset = assetSystem_->loadShaderCompiled(info.vertGlslPath);
        if (info.vertShaderAsset.uuid != 0) {
            shaderAssetToPipelines_[info.vertShaderAsset.uuid].push_back(uiPipeline_);
        }

        // Register fragment shader for hot reload tracking
        info.fragShaderAsset = assetSystem_->loadShaderCompiled(info.fragGlslPath);
        if (info.fragShaderAsset.uuid != 0) {
            shaderAssetToPipelines_[info.fragShaderAsset.uuid].push_back(uiPipeline_);
        }

        if (info.vertShaderAsset.uuid != 0 || info.fragShaderAsset.uuid != 0) {
            pipelineShaderFiles_[uiPipeline_] = std::move(info);
            spdlog::info("VulkanUIRenderBackend: Registered UI shaders for hot reload");
        }
    }

    spdlog::info("VulkanUIRenderBackend: UI pipeline created successfully");
    return true;
}

//==========================================================================
// Shader Hot Reload
//==========================================================================

void VulkanUIRenderBackend::onShaderAssetChanged(AssetHandle handle, AssetType type) {
    if (type != AssetType::Shader) return;

    // Find pipelines that use this shader asset
    auto it = shaderAssetToPipelines_.find(handle.uuid);
    if (it == shaderAssetToPipelines_.end()) {
        return;  // Not a shader we're tracking
    }

    spdlog::info("VulkanUIRenderBackend: Shader asset changed, reloading pipeline");

    // Wait for GPU to be idle before modifying pipelines
    vkDeviceWaitIdle(context_->getDevice());

    for (VulkanPipelineHandle pipeline : it->second) {
        auto infoIt = pipelineShaderFiles_.find(pipeline);
        if (infoIt != pipelineShaderFiles_.end()) {
            reloadPipelineShaders(pipeline, infoIt->second);
        }
    }
}

void VulkanUIRenderBackend::reloadPipelineShaders(
    VulkanPipelineHandle oldPipeline, const ShaderFileInfo& info) {

    spdlog::info("VulkanUIRenderBackend: Reloading pipeline from: {} + {}",
                info.vertGlslPath, info.fragGlslPath);

    // Force reload to recompile shaders
    const ShaderData* vertShader = nullptr;
    const ShaderData* fragShader = nullptr;

    if (info.vertShaderAsset.uuid != 0 && assetSystem_) {
        assetSystem_->reloadAsset(info.vertShaderAsset);
        vertShader = assetSystem_->getShaderData(info.vertShaderAsset);
    }
    if (info.fragShaderAsset.uuid != 0 && assetSystem_) {
        assetSystem_->reloadAsset(info.fragShaderAsset);
        fragShader = assetSystem_->getShaderData(info.fragShaderAsset);
    }

    if (!vertShader || !fragShader || !vertShader->compiled || !fragShader->compiled) {
        spdlog::error("VulkanUIRenderBackend: Shader reload failed - compilation error");
        if (vertShader && !vertShader->compileError.empty()) {
            spdlog::error("  Vertex: {}", vertShader->compileError);
        }
        if (fragShader && !fragShader->compileError.empty()) {
            spdlog::error("  Fragment: {}", fragShader->compileError);
        }
        return;
    }

    // Recreate pipeline with new shader bytecode
    VulkanPipelineDef def;
    def.shaderStages = {
        {VK_SHADER_STAGE_VERTEX_BIT, vertShader->spirvBytecode, "main"},
        {VK_SHADER_STAGE_FRAGMENT_BIT, fragShader->spirvBytecode, "main"}
    };
    def.vertexBindings = {
        {0, sizeof(UIVertex), VK_VERTEX_INPUT_RATE_VERTEX}
    };
    def.vertexAttributes = {
        {0, 0, VK_FORMAT_R32G32_SFLOAT, static_cast<std::uint32_t>(offsetof(UIVertex, position))},
        {1, 0, VK_FORMAT_R8G8B8A8_UNORM, static_cast<std::uint32_t>(offsetof(UIVertex, color))},
        {2, 0, VK_FORMAT_R32G32_SFLOAT, static_cast<std::uint32_t>(offsetof(UIVertex, texCoord))}
    };
    def.pushConstantRanges = {
        {VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, 76}
    };
    def.descriptorSetLayouts = {textureDescriptorSetLayout_};
    def.depthTestEnable = false;
    def.depthWriteEnable = false;
    def.blendEnable = true;
    def.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
    def.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    def.colorBlendOp = VK_BLEND_OP_ADD;
    def.cullMode = VK_CULL_MODE_NONE;

    auto result = context_->createPipeline(def);
    if (!result) {
        spdlog::error("VulkanUIRenderBackend: Shader reload failed - could not create new pipeline");
        return;
    }

    VulkanPipelineHandle newPipeline = *result;

    // Update tracking to point to the new pipeline
    ShaderFileInfo newInfo = info;
    pipelineShaderFiles_.erase(oldPipeline);
    pipelineShaderFiles_[newPipeline] = newInfo;

    // Update shader asset to pipeline mapping
    if (info.vertShaderAsset.uuid != 0) {
        auto& pipelines = shaderAssetToPipelines_[info.vertShaderAsset.uuid];
        pipelines.erase(std::remove(pipelines.begin(), pipelines.end(), oldPipeline), pipelines.end());
        pipelines.push_back(newPipeline);
    }
    if (info.fragShaderAsset.uuid != 0) {
        auto& pipelines = shaderAssetToPipelines_[info.fragShaderAsset.uuid];
        pipelines.erase(std::remove(pipelines.begin(), pipelines.end(), oldPipeline), pipelines.end());
        pipelines.push_back(newPipeline);
    }

    // Swap the active pipeline and destroy the old one
    uiPipeline_ = newPipeline;
    context_->destroyPipeline(oldPipeline);

    spdlog::info("VulkanUIRenderBackend: Shader reload successful (compiled via AssetSystem shaderc)");
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

    // Upload data via staging buffer
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

    auto cmdBuffer = context_->getCurrentCommandBuffer();
    if (cmdBuffer == VK_NULL_HANDLE) {
        return;
    }

    // Bind UI pipeline
    context_->bindPipeline(uiPipeline_);

    // Set orthographic projection: origin at top-left, Y-down (matches UI coordinate system)
    glm::mat4 projection = glm::ortho(
        0.0f, static_cast<float>(viewportWidth_),
        static_cast<float>(viewportHeight_), 0.0f,
        -1.0f, 1.0f
    );

    VkPipelineLayout layout = context_->getPipelineLayout(uiPipeline_);

    // Push projection matrix
    vkCmdPushConstants(cmdBuffer, layout,
                       VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                       0, sizeof(glm::mat4), &projection);

    // Reset scissor to full viewport
    VkRect2D scissor{};
    scissor.offset = {0, 0};
    scissor.extent.width = static_cast<std::uint32_t>(viewportWidth_);
    scissor.extent.height = static_cast<std::uint32_t>(viewportHeight_);
    vkCmdSetScissor(cmdBuffer, 0, 1, &scissor);

    // Reset statistics
    drawCallCount_ = 0;
    triangleCount_ = 0;

    inUIPass_ = true;
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

    auto cmdBuffer = context_->getCurrentCommandBuffer();
    if (cmdBuffer == VK_NULL_HANDLE) {
        return;
    }

    const auto& geom = it->second;
    VkPipelineLayout layout = context_->getPipelineLayout(uiPipeline_);

    // Push translation
    float translationData[2] = {translation.x, translation.y};
    vkCmdPushConstants(cmdBuffer, layout,
                       VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                       64, 8, translationData);

    // Determine texture and push hasTexture flag
    VkDescriptorSet texDescSet = whiteTextureDescriptorSet_;
    std::int32_t hasTexture = 0;

    if (texture != InvalidUITexture) {
        auto descIt = textureDescriptorSets_.find(texture);
        if (descIt != textureDescriptorSets_.end()) {
            texDescSet = descIt->second;
            hasTexture = 1;
        }
    }

    vkCmdPushConstants(cmdBuffer, layout,
                       VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                       72, sizeof(std::int32_t), &hasTexture);

    // Bind texture descriptor set
    vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                            layout, 0, 1, &texDescSet, 0, nullptr);

    // Bind vertex buffer
    VkDeviceSize vertexOffset = 0;
    vkCmdBindVertexBuffers(cmdBuffer, 0, 1, &geom.vertexBuffer, &vertexOffset);

    // Bind index buffer
    vkCmdBindIndexBuffer(cmdBuffer, geom.indexBuffer, 0, VK_INDEX_TYPE_UINT32);

    // Draw indexed
    vkCmdDrawIndexed(cmdBuffer, geom.indexCount, 1, 0, 0, 0);

    // Update statistics
    drawCallCount_++;
    triangleCount_ += geom.indexCount / 3;
}

void VulkanUIRenderBackend::endUIPass() {
    if (!inUIPass_) {
        return;
    }

    // Reset scissor to full viewport in case it was modified
    if (scissorEnabled_) {
        auto cmdBuffer = context_->getCurrentCommandBuffer();
        if (cmdBuffer != VK_NULL_HANDLE) {
            VkRect2D scissor{};
            scissor.offset = {0, 0};
            scissor.extent.width = static_cast<std::uint32_t>(viewportWidth_);
            scissor.extent.height = static_cast<std::uint32_t>(viewportHeight_);
            vkCmdSetScissor(cmdBuffer, 0, 1, &scissor);
        }
    }

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

    // Load image using stb_image
    int width, height, channels;
    stbi_set_flip_vertically_on_load(false);  // UI textures use top-left origin
    unsigned char* data = stbi_load(path.string().c_str(), &width, &height, &channels, 4);
    if (!data) {
        spdlog::error("VulkanUIRenderBackend: Failed to load texture: {}", path.string());
        outWidth = 0;
        outHeight = 0;
        return InvalidUITexture;
    }

    outWidth = width;
    outHeight = height;

    // Create texture from raw data
    auto dataSpan = std::span<const std::uint8_t>(data, static_cast<std::size_t>(width * height * 4));
    UITextureHandle handle = createTexture(dataSpan, width, height);

    stbi_image_free(data);

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
        spdlog::error("VulkanUIRenderBackend: Failed to create texture");
        return InvalidUITexture;
    }

    TextureResource tex{};
    tex.imageHandle = *result;
    tex.width = width;
    tex.height = height;

    // Upload texture data
    context_->uploadToImage(tex.imageHandle, data.data(), data.size());

    // Allocate descriptor set for this texture
    VkDescriptorSet descSet = allocateTextureDescriptorSet(tex.imageHandle);
    if (descSet == VK_NULL_HANDLE) {
        context_->destroyImage(tex.imageHandle);
        return InvalidUITexture;
    }

    UITextureHandle handle = nextTextureHandle_++;
    textureCache_[handle] = tex;
    textureDescriptorSets_[handle] = descSet;
    return handle;
}

void VulkanUIRenderBackend::releaseTexture(UITextureHandle texture) {
    auto it = textureCache_.find(texture);
    if (it != textureCache_.end()) {
        // Free descriptor set
        auto descIt = textureDescriptorSets_.find(texture);
        if (descIt != textureDescriptorSets_.end()) {
            vkFreeDescriptorSets(context_->getDevice(), textureDescriptorPool_,
                                 1, &descIt->second);
            textureDescriptorSets_.erase(descIt);
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

    // In Vulkan, scissor is always active with dynamic state.
    // When "disabling" scissor, reset to full viewport.
    if (!enable && inUIPass_) {
        auto cmdBuffer = context_->getCurrentCommandBuffer();
        if (cmdBuffer != VK_NULL_HANDLE) {
            VkRect2D scissor{};
            scissor.offset = {0, 0};
            scissor.extent.width = static_cast<std::uint32_t>(viewportWidth_);
            scissor.extent.height = static_cast<std::uint32_t>(viewportHeight_);
            vkCmdSetScissor(cmdBuffer, 0, 1, &scissor);
        }
    }
}

void VulkanUIRenderBackend::setScissorRegion(const UIScissorRect& region) {
    if (!initialized_ || !inUIPass_) {
        return;
    }

    auto cmdBuffer = context_->getCurrentCommandBuffer();
    if (cmdBuffer == VK_NULL_HANDLE) {
        return;
    }

    // Vulkan scissor origin is top-left (same as UI coordinate system)
    // Clamp to valid values
    VkRect2D scissor{};
    scissor.offset.x = std::max(0, region.x);
    scissor.offset.y = std::max(0, region.y);
    scissor.extent.width = static_cast<uint32_t>(std::max(0, region.width));
    scissor.extent.height = static_cast<uint32_t>(std::max(0, region.height));

    vkCmdSetScissor(cmdBuffer, 0, 1, &scissor);
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

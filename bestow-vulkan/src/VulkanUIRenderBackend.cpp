// bestow-vulkan/src/VulkanUIRenderBackend.cpp
// Vulkan implementation of IUIRenderBackend for 2D UI rendering

module;

#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <spdlog/spdlog.h>

module bestow.vulkan.impl;

import std;
import bestow.vulkan;
import bestow.services;

namespace bestow::vulkan {

//==========================================================================
// VulkanUIRenderBackend Implementation
//
// NOTE: This is a skeleton implementation. The full Vulkan pipeline setup
// for UI rendering requires:
// - Creating UI vertex/fragment shaders
// - Setting up descriptor sets for textures
// - Creating a graphics pipeline for 2D UI
// - Managing push constants or uniform buffers for transforms
//
// For now, this provides the interface structure so the architecture compiles.
//==========================================================================

VulkanUIRenderBackend::VulkanUIRenderBackend(VulkanContext* context)
    : context_(context) {}

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

    // TODO: Create UI pipeline, shaders, descriptor sets
    // For now, mark as initialized but log a warning
    spdlog::warn("VulkanUIRenderBackend: UI rendering not yet implemented - will be added in a future update");

    viewportWidth_ = static_cast<int>(context_->getSwapchainExtent().width);
    viewportHeight_ = static_cast<int>(context_->getSwapchainExtent().height);

    initialized_ = true;
    return true;
}

void VulkanUIRenderBackend::shutdown() {
    if (!initialized_) {
        return;
    }

    if (context_ && context_->isInitialized()) {
        auto device = context_->getDevice();

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

        // TODO: Destroy pipeline, shaders, descriptor sets
    }

    initialized_ = false;
}

bool VulkanUIRenderBackend::isInitialized() const {
    return initialized_;
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
    if (!initialized_) {
        return;
    }

    // TODO: Bind UI pipeline, set up descriptor sets
    inUIPass_ = true;
    drawCallCount_ = 0;
    triangleCount_ = 0;
}

void VulkanUIRenderBackend::renderGeometry(
    UIGeometryHandle geometry,
    Vec2 translation,
    UITextureHandle texture) {

    if (!initialized_ || !inUIPass_) {
        return;
    }

    auto it = geometryCache_.find(geometry);
    if (it == geometryCache_.end()) {
        return;
    }

    // TODO: Implement actual rendering
    // - Bind vertex/index buffers
    // - Set push constants for translation and texture flag
    // - Bind texture descriptor if texture != InvalidUITexture
    // - Issue draw call

    const auto& geom = it->second;
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

    // TODO: Load texture from file using stb_image or similar
    // For now, return invalid
    spdlog::warn("VulkanUIRenderBackend::loadTexture not yet implemented");
    outWidth = 0;
    outHeight = 0;
    return InvalidUITexture;
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
    tex.image = VK_NULL_HANDLE;  // Will be set when we retrieve it
    tex.width = width;
    tex.height = height;

    // Upload texture data
    context_->uploadToImage(tex.imageHandle, data.data(), data.size());

    UITextureHandle handle = nextTextureHandle_++;
    textureCache_[handle] = tex;
    return handle;
}

void VulkanUIRenderBackend::releaseTexture(UITextureHandle texture) {
    auto it = textureCache_.find(texture);
    if (it != textureCache_.end()) {
        context_->destroyImage(it->second.imageHandle);
        textureCache_.erase(it);
    }
}

//==========================================================================
// Scissor (Clipping)
//==========================================================================

void VulkanUIRenderBackend::enableScissor(bool enable) {
    scissorEnabled_ = enable;
    // In Vulkan, scissor is always enabled when using dynamic scissor state
    // The actual clipping is controlled by vkCmdSetScissor
}

void VulkanUIRenderBackend::setScissorRegion(const UIScissorRect& region) {
    if (!initialized_ || !inUIPass_) {
        return;
    }

    auto cmdBuffer = context_->getCurrentCommandBuffer();
    if (cmdBuffer == VK_NULL_HANDLE) {
        return;
    }

    // Vulkan scissor origin is top-left (same as UI), but we need to clamp
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

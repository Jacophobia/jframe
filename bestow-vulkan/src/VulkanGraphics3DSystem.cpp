// bestow-vulkan/src/VulkanGraphics3DSystem.cpp
// Vulkan 3D graphics system implementation

module;

#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <bestow/sol2_compat.hpp>
#include <spdlog/spdlog.h>
#include <stb_image.h>  // For decoding embedded compressed textures

module bestow.vulkan.impl;

import std;
import bestow.services;  // Re-exports all contracts
import bestow.utils;

namespace bestow::vulkan {

VulkanGraphics3DSystem::~VulkanGraphics3DSystem() {
    shutdown();
}

void VulkanGraphics3DSystem::shutdown() {
    if (!initialized_) return;

    // Wait for GPU to finish all work before destroying resources
    if (context_.getDevice() != VK_NULL_HANDLE) {
        vkDeviceWaitIdle(context_.getDevice());
    }

    // Cleanup meshes
    for (auto& [handle, mesh] : meshes_) {
        if (mesh.vertexBuffer != 0) context_.destroyBuffer(mesh.vertexBuffer);
        if (mesh.indexBuffer != 0) context_.destroyBuffer(mesh.indexBuffer);
    }
    meshes_.clear();

    // Cleanup uniform buffers
    if (cameraUBO_ != 0) {
        context_.destroyBuffer(cameraUBO_);
        cameraUBO_ = 0;
    }
    if (lightUBO_ != 0) {
        context_.destroyBuffer(lightUBO_);
        lightUBO_ = 0;
    }

    // Cleanup skybox cubemap texture
    if (skyboxCubemapImage_ != 0) {
        context_.destroyImage(skyboxCubemapImage_);
        skyboxCubemapImage_ = 0;
    }
    hasSkybox_ = false;

    // Cleanup pipelines
    if (pbrPipeline_ != 0) {
        context_.destroyPipeline(pbrPipeline_);
        pbrPipeline_ = 0;
    }
    if (unlitPipeline_ != 0) {
        context_.destroyPipeline(unlitPipeline_);
        unlitPipeline_ = 0;
    }
    if (debugPipeline_ != 0) {
        context_.destroyPipeline(debugPipeline_);
        debugPipeline_ = 0;
    }
    if (skyboxPipeline_ != 0) {
        context_.destroyPipeline(skyboxPipeline_);
        skyboxPipeline_ = 0;
    }

    context_.shutdown();
    initialized_ = false;
}

bool VulkanGraphics3DSystem::initialize(const Graphics3DConfig& config) {
    if (initialized_) return true;

    // Convert Graphics3DConfig to VulkanConfig
    VulkanConfig vulkanConfig;
    vulkanConfig.windowWidth = config.windowWidth;
    vulkanConfig.windowHeight = config.windowHeight;
    vulkanConfig.windowTitle = config.windowTitle;
    vulkanConfig.vsync = config.vsync;
    vulkanConfig.enableValidation = config.enableValidation;
    vulkanConfig.headless = false;

    // If a native window handle is provided, use it
    if (config.nativeWindowHandle) {
        vulkanConfig.window = static_cast<GLFWwindow*>(config.nativeWindowHandle);
    }

    auto result = context_.initialize(vulkanConfig);
    if (!result) {
        return false;
    }

    // Create uniform buffers
    VulkanBufferDef uboDesc{};
    uboDesc.size = sizeof(CameraUBO);
    uboDesc.usage = VulkanBufferUsage::Uniform;
    uboDesc.hostVisible = true;
    uboDesc.persistentlyMapped = true;

    auto bufferResult = context_.createBuffer(uboDesc);
    if (!bufferResult) {
        context_.shutdown();
        return false;
    }
    cameraUBO_ = *bufferResult;

    uboDesc.size = sizeof(LightUBO) + 16 * sizeof(glm::vec4) * 2;  // Room for lights
    bufferResult = context_.createBuffer(uboDesc);
    if (!bufferResult) {
        context_.destroyBuffer(cameraUBO_);
        context_.shutdown();
        return false;
    }
    lightUBO_ = *bufferResult;

    // Create bone matrix UBO for skeletal animation
    uboDesc.size = MAX_BONES * sizeof(glm::mat4);  // 100 bones * 64 bytes = 6400 bytes
    bufferResult = context_.createBuffer(uboDesc);
    if (!bufferResult) {
        context_.destroyBuffer(lightUBO_);
        context_.destroyBuffer(cameraUBO_);
        context_.shutdown();
        return false;
    }
    boneUBO_ = *bufferResult;

    // Create descriptor set layout for bone matrices
    VkDescriptorSetLayoutBinding boneBinding{};
    boneBinding.binding = 0;
    boneBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    boneBinding.descriptorCount = 1;
    boneBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    boneBinding.pImmutableSamplers = nullptr;

    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = 1;
    layoutInfo.pBindings = &boneBinding;

    if (vkCreateDescriptorSetLayout(context_.getDevice(), &layoutInfo, nullptr, &boneDescriptorSetLayout_) != VK_SUCCESS) {
        std::fprintf(stderr, "[Vulkan] Failed to create bone descriptor set layout\n");
    }

    // Create descriptor pool for bone descriptors
    VkDescriptorPoolSize poolSize{};
    poolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    poolSize.descriptorCount = 1;

    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = 1;
    poolInfo.pPoolSizes = &poolSize;
    poolInfo.maxSets = 1;

    if (vkCreateDescriptorPool(context_.getDevice(), &poolInfo, nullptr, &boneDescriptorPool_) != VK_SUCCESS) {
        std::fprintf(stderr, "[Vulkan] Failed to create bone descriptor pool\n");
    }

    // Allocate bone descriptor set
    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = boneDescriptorPool_;
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts = &boneDescriptorSetLayout_;

    if (vkAllocateDescriptorSets(context_.getDevice(), &allocInfo, &boneDescriptorSet_) != VK_SUCCESS) {
        std::fprintf(stderr, "[Vulkan] Failed to allocate bone descriptor set\n");
    }

    // Update descriptor set with bone UBO
    VkDescriptorBufferInfo bufferInfo{};
    bufferInfo.buffer = context_.getBuffer(boneUBO_);
    bufferInfo.offset = 0;
    bufferInfo.range = MAX_BONES * sizeof(glm::mat4);

    VkWriteDescriptorSet descriptorWrite{};
    descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrite.dstSet = boneDescriptorSet_;
    descriptorWrite.dstBinding = 0;
    descriptorWrite.dstArrayElement = 0;
    descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    descriptorWrite.descriptorCount = 1;
    descriptorWrite.pBufferInfo = &bufferInfo;

    vkUpdateDescriptorSets(context_.getDevice(), 1, &descriptorWrite, 0, nullptr);

    //==========================================================================
    // Create texture descriptor set layout (set 1 for material textures)
    //==========================================================================
    {
        VkDescriptorSetLayoutBinding texBinding{};
        texBinding.binding = 0;
        texBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        texBinding.descriptorCount = 1;
        texBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
        texBinding.pImmutableSamplers = nullptr;

        VkDescriptorSetLayoutCreateInfo texLayoutInfo{};
        texLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        texLayoutInfo.bindingCount = 1;
        texLayoutInfo.pBindings = &texBinding;

        if (vkCreateDescriptorSetLayout(context_.getDevice(), &texLayoutInfo, nullptr, &textureDescriptorSetLayout_) != VK_SUCCESS) {
            std::fprintf(stderr, "[Vulkan] Failed to create texture descriptor set layout\n");
        }
    }

    //==========================================================================
    // Create texture descriptor pool (enough for many materials)
    //==========================================================================
    {
        VkDescriptorPoolSize texPoolSize{};
        texPoolSize.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        texPoolSize.descriptorCount = 100;  // Support up to 100 materials with textures

        VkDescriptorPoolCreateInfo texPoolInfo{};
        texPoolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        texPoolInfo.poolSizeCount = 1;
        texPoolInfo.pPoolSizes = &texPoolSize;
        texPoolInfo.maxSets = 100;
        texPoolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;

        if (vkCreateDescriptorPool(context_.getDevice(), &texPoolInfo, nullptr, &textureDescriptorPool_) != VK_SUCCESS) {
            std::fprintf(stderr, "[Vulkan] Failed to create texture descriptor pool\n");
        }
    }

    //==========================================================================
    // Create default 1x1 white texture
    //==========================================================================
    {
        VulkanImageDef imageDef;
        imageDef.type = VK_IMAGE_TYPE_2D;
        imageDef.format = VK_FORMAT_R8G8B8A8_UNORM;
        imageDef.width = 1;
        imageDef.height = 1;
        imageDef.depth = 1;
        imageDef.mipLevels = 1;
        imageDef.arrayLayers = 1;
        imageDef.usage = VulkanImageUsage::Sampled | VulkanImageUsage::TransferDst;
        imageDef.isCubemap = false;

        auto imageResult = context_.createImage(imageDef);
        if (imageResult) {
            defaultWhiteTexture_ = *imageResult;

            // Upload white pixel
            unsigned char whitePixel[4] = {255, 255, 255, 255};
            context_.uploadToImage(defaultWhiteTexture_, whitePixel, 4);

            std::fprintf(stderr, "[Vulkan] Created default white texture\n");
        } else {
            std::fprintf(stderr, "[Vulkan] Failed to create default white texture\n");
        }
    }

    //==========================================================================
    // Allocate default texture descriptor set
    //==========================================================================
    if (defaultWhiteTexture_ != 0 && textureDescriptorSetLayout_ != VK_NULL_HANDLE && textureDescriptorPool_ != VK_NULL_HANDLE) {
        VkDescriptorSetAllocateInfo texAllocInfo{};
        texAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        texAllocInfo.descriptorPool = textureDescriptorPool_;
        texAllocInfo.descriptorSetCount = 1;
        texAllocInfo.pSetLayouts = &textureDescriptorSetLayout_;

        if (vkAllocateDescriptorSets(context_.getDevice(), &texAllocInfo, &defaultTextureDescriptorSet_) == VK_SUCCESS) {
            // Update descriptor set with default white texture
            VkDescriptorImageInfo imageInfo{};
            imageInfo.sampler = context_.getImageSampler(defaultWhiteTexture_);
            imageInfo.imageView = context_.getImageView(defaultWhiteTexture_);
            imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

            VkWriteDescriptorSet texWrite{};
            texWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            texWrite.dstSet = defaultTextureDescriptorSet_;
            texWrite.dstBinding = 0;
            texWrite.dstArrayElement = 0;
            texWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            texWrite.descriptorCount = 1;
            texWrite.pImageInfo = &imageInfo;

            vkUpdateDescriptorSets(context_.getDevice(), 1, &texWrite, 0, nullptr);
            std::fprintf(stderr, "[Vulkan] Created default texture descriptor set\n");
        } else {
            std::fprintf(stderr, "[Vulkan] Failed to allocate default texture descriptor set\n");
        }
    }

    createDefaultMaterials();
    createPipelines();

    // Subscribe to shader type changes for hot reload if asset system is available
    if (pIAssetSystem_ && useAssetSystemHotReload_) {
        shaderSubscriptionId_ = pIAssetSystem_->subscribeToType(
            AssetType::Shader,
            [this](AssetHandle handle, AssetType type) {
                onShaderAssetChanged(handle, type);
            }
        );
        // Enable hot reload on the asset system
        pIAssetSystem_->enableHotReload(true);
        std::fprintf(stderr, "[Vulkan] Subscribed to shader asset changes for hot reload\n");
    }

    initialized_ = true;
    return true;
}

void VulkanGraphics3DSystem::beginFrame() {
    renderQueue_.clear();
    context_.beginFrame();
    updateCameraUBO();
    updateLightUBO();
    checkShaderHotReload();
}

void VulkanGraphics3DSystem::endFrame() {
    flushRenderQueue();
    renderDebugLines();
    context_.endFrame();

    // Update timed debug lines
    for (auto& line : debugLines_) {
        line.timeRemaining -= 1.0f / 60.0f;
    }
    std::erase_if(debugLines_, [](const DebugLine& l) { return l.timeRemaining <= 0; });
}

Result<MeshHandle, Graphics3DError> VulkanGraphics3DSystem::createMesh(const MeshDef& def) {
    VulkanMesh mesh;
    mesh.vertexCount = static_cast<std::uint32_t>(def.vertices.size());
    mesh.indexCount = static_cast<std::uint32_t>(def.indices.size());
    mesh.bounds = def.bounds;
    mesh.isDynamic = def.isDynamic;

    // Create vertex buffer
    VulkanBufferDef vertexDef{};
    vertexDef.size = def.vertices.size() * sizeof(Vertex3D);
    vertexDef.usage = VulkanBufferUsage::Vertex | VulkanBufferUsage::TransferDst;
    vertexDef.hostVisible = def.isDynamic;

    auto vertexResult = context_.createBuffer(vertexDef);
    if (!vertexResult) {
        return std::unexpected(Graphics3DError::OutOfMemory);
    }
    mesh.vertexBuffer = *vertexResult;

    // Upload vertex data
    context_.uploadToBuffer(mesh.vertexBuffer, def.vertices.data(),
                            def.vertices.size() * sizeof(Vertex3D));

    // Create index buffer
    if (!def.indices.empty()) {
        VulkanBufferDef indexDef{};
        indexDef.size = def.indices.size() * sizeof(std::uint32_t);
        indexDef.usage = VulkanBufferUsage::Index | VulkanBufferUsage::TransferDst;
        indexDef.hostVisible = false;

        auto indexResult = context_.createBuffer(indexDef);
        if (!indexResult) {
            context_.destroyBuffer(mesh.vertexBuffer);
            return std::unexpected(Graphics3DError::OutOfMemory);
        }
        mesh.indexBuffer = *indexResult;

        context_.uploadToBuffer(mesh.indexBuffer, def.indices.data(),
                                def.indices.size() * sizeof(std::uint32_t));
    }

    MeshHandle handle = nextMeshHandle_++;
    meshes_[handle] = mesh;
    return handle;
}

void VulkanGraphics3DSystem::destroyMesh(MeshHandle handle) {
    auto it = meshes_.find(handle);
    if (it != meshes_.end()) {
        context_.destroyBuffer(it->second.vertexBuffer);
        if (it->second.indexBuffer != 0) {
            context_.destroyBuffer(it->second.indexBuffer);
        }
        meshes_.erase(it);
    }
}

bool VulkanGraphics3DSystem::hasMesh(MeshHandle handle) const {
    return meshes_.contains(handle);
}

AABB3D VulkanGraphics3DSystem::getMeshBounds(MeshHandle handle) const {
    auto it = meshes_.find(handle);
    if (it != meshes_.end()) {
        return it->second.bounds;
    }
    return AABB3D{};
}

Result<void, Graphics3DError> VulkanGraphics3DSystem::updateMeshVertices(
    MeshHandle handle, std::span<const Vertex3D> vertices, std::uint32_t offset) {
    auto it = meshes_.find(handle);
    if (it == meshes_.end()) {
        return std::unexpected(Graphics3DError::InvalidMesh);
    }

    if (!it->second.isDynamic) {
        return std::unexpected(Graphics3DError::InvalidMesh);
    }

    auto result = context_.uploadToBuffer(it->second.vertexBuffer,
                                          vertices.data(),
                                          vertices.size() * sizeof(Vertex3D),
                                          offset * sizeof(Vertex3D));
    if (!result) {
        return std::unexpected(Graphics3DError::OutOfMemory);
    }

    return {};
}

// Primitive mesh generation helper
namespace {
    std::vector<Vertex3D> generateCubeVertices(float size) {
        float h = size / 2.0f;
        std::vector<Vertex3D> vertices;

        // Each face has 4 vertices
        // Front face
        vertices.push_back({{-h, -h, h}, {0, 0, 1}, {0, 0}});
        vertices.push_back({{h, -h, h}, {0, 0, 1}, {1, 0}});
        vertices.push_back({{h, h, h}, {0, 0, 1}, {1, 1}});
        vertices.push_back({{-h, h, h}, {0, 0, 1}, {0, 1}});

        // Back face
        vertices.push_back({{h, -h, -h}, {0, 0, -1}, {0, 0}});
        vertices.push_back({{-h, -h, -h}, {0, 0, -1}, {1, 0}});
        vertices.push_back({{-h, h, -h}, {0, 0, -1}, {1, 1}});
        vertices.push_back({{h, h, -h}, {0, 0, -1}, {0, 1}});

        // Top face
        vertices.push_back({{-h, h, h}, {0, 1, 0}, {0, 0}});
        vertices.push_back({{h, h, h}, {0, 1, 0}, {1, 0}});
        vertices.push_back({{h, h, -h}, {0, 1, 0}, {1, 1}});
        vertices.push_back({{-h, h, -h}, {0, 1, 0}, {0, 1}});

        // Bottom face
        vertices.push_back({{-h, -h, -h}, {0, -1, 0}, {0, 0}});
        vertices.push_back({{h, -h, -h}, {0, -1, 0}, {1, 0}});
        vertices.push_back({{h, -h, h}, {0, -1, 0}, {1, 1}});
        vertices.push_back({{-h, -h, h}, {0, -1, 0}, {0, 1}});

        // Right face
        vertices.push_back({{h, -h, h}, {1, 0, 0}, {0, 0}});
        vertices.push_back({{h, -h, -h}, {1, 0, 0}, {1, 0}});
        vertices.push_back({{h, h, -h}, {1, 0, 0}, {1, 1}});
        vertices.push_back({{h, h, h}, {1, 0, 0}, {0, 1}});

        // Left face
        vertices.push_back({{-h, -h, -h}, {-1, 0, 0}, {0, 0}});
        vertices.push_back({{-h, -h, h}, {-1, 0, 0}, {1, 0}});
        vertices.push_back({{-h, h, h}, {-1, 0, 0}, {1, 1}});
        vertices.push_back({{-h, h, -h}, {-1, 0, 0}, {0, 1}});

        return vertices;
    }

    std::vector<std::uint32_t> generateCubeIndices() {
        std::vector<std::uint32_t> indices;
        for (std::uint32_t face = 0; face < 6; ++face) {
            std::uint32_t base = face * 4;
            indices.push_back(base);
            indices.push_back(base + 1);
            indices.push_back(base + 2);
            indices.push_back(base);
            indices.push_back(base + 2);
            indices.push_back(base + 3);
        }
        return indices;
    }
}

Result<MeshHandle, Graphics3DError> VulkanGraphics3DSystem::createCubeMesh(float size) {
    auto vertices = generateCubeVertices(size);
    auto indices = generateCubeIndices();

    float h = size / 2.0f;
    MeshDef def;
    def.vertices = vertices;
    def.indices = indices;
    def.bounds = AABB3D{Vec3{-h, -h, -h}, Vec3{h, h, h}};

    return createMesh(def);
}

Result<MeshHandle, Graphics3DError> VulkanGraphics3DSystem::createSphereMesh(
    float radius, std::uint32_t segments, std::uint32_t rings) {
    std::vector<Vertex3D> vertices;
    std::vector<std::uint32_t> indices;

    for (std::uint32_t r = 0; r <= rings; ++r) {
        float phi = 3.14159f * r / rings;
        for (std::uint32_t s = 0; s <= segments; ++s) {
            float theta = 2.0f * 3.14159f * s / segments;

            Vec3 pos{
                radius * std::sin(phi) * std::cos(theta),
                radius * std::cos(phi),
                radius * std::sin(phi) * std::sin(theta)
            };
            Vec3 normal = pos;
            float len = std::sqrt(normal.x * normal.x + normal.y * normal.y + normal.z * normal.z);
            normal = Vec3{normal.x / len, normal.y / len, normal.z / len};
            Vec2 uv{static_cast<float>(s) / segments, static_cast<float>(r) / rings};

            vertices.push_back({pos, normal, uv});
        }
    }

    for (std::uint32_t r = 0; r < rings; ++r) {
        for (std::uint32_t s = 0; s < segments; ++s) {
            std::uint32_t i0 = r * (segments + 1) + s;
            std::uint32_t i1 = i0 + 1;
            std::uint32_t i2 = i0 + (segments + 1);
            std::uint32_t i3 = i2 + 1;

            indices.push_back(i0);
            indices.push_back(i2);
            indices.push_back(i1);

            indices.push_back(i1);
            indices.push_back(i2);
            indices.push_back(i3);
        }
    }

    MeshDef def;
    def.vertices = vertices;
    def.indices = indices;
    def.bounds = AABB3D{Vec3{-radius, -radius, -radius}, Vec3{radius, radius, radius}};

    return createMesh(def);
}

Result<MeshHandle, Graphics3DError> VulkanGraphics3DSystem::createCylinderMesh(
    float radius, float height, std::uint32_t segments) {
    // Simplified cylinder generation
    std::vector<Vertex3D> vertices;
    std::vector<std::uint32_t> indices;

    // Top and bottom circles plus sides
    for (std::uint32_t i = 0; i <= segments; ++i) {
        float angle = 2.0f * 3.14159f * i / segments;
        float x = radius * std::cos(angle);
        float z = radius * std::sin(angle);

        // Top circle
        vertices.push_back({{x, height, z}, {0, 1, 0}, {static_cast<float>(i) / segments, 0}});
        // Bottom circle
        vertices.push_back({{x, 0, z}, {0, -1, 0}, {static_cast<float>(i) / segments, 1}});
        // Side top
        Vec3 normal{std::cos(angle), 0, std::sin(angle)};
        vertices.push_back({{x, height, z}, normal, {static_cast<float>(i) / segments, 0}});
        // Side bottom
        vertices.push_back({{x, 0, z}, normal, {static_cast<float>(i) / segments, 1}});
    }

    // Generate indices for sides
    for (std::uint32_t i = 0; i < segments; ++i) {
        std::uint32_t base = i * 4;
        // Side quad
        indices.push_back(base + 2);
        indices.push_back(base + 6);
        indices.push_back(base + 3);

        indices.push_back(base + 3);
        indices.push_back(base + 6);
        indices.push_back(base + 7);
    }

    MeshDef def;
    def.vertices = vertices;
    def.indices = indices;
    def.bounds = AABB3D{Vec3{-radius, 0, -radius}, Vec3{radius, height, radius}};

    return createMesh(def);
}

Result<MeshHandle, Graphics3DError> VulkanGraphics3DSystem::createCapsuleMesh(
    float radius, float height, std::uint32_t segments, std::uint32_t rings) {
    std::vector<Vertex3D> vertices;
    std::vector<std::uint32_t> indices;

    using bestow::core::Math::PI;

    // Capsule consists of: top hemisphere + cylinder + bottom hemisphere
    // Match OpenGL behavior: height parameter is the cylinder portion height
    // Total capsule height = height + 2*radius
    float halfHeight = height * 0.5f;

    // Top hemisphere (centered at +halfHeight)
    for (std::uint32_t r = 0; r <= rings; ++r) {
        float v = static_cast<float>(r) / rings;
        float phi = v * PI * 0.5f;  // 0 to PI/2 (top half of sphere)

        for (std::uint32_t s = 0; s <= segments; ++s) {
            float u = static_cast<float>(s) / segments;
            float theta = u * 2.0f * PI;

            float sinPhi = std::sin(phi);
            float cosPhi = std::cos(phi);
            float sinTheta = std::sin(theta);
            float cosTheta = std::cos(theta);

            Vec3 normal{sinPhi * cosTheta, cosPhi, sinPhi * sinTheta};
            Vec3 pos = normal * radius + Vec3{0.0f, halfHeight, 0.0f};
            Vec2 uv{u, v * 0.25f};  // Top quarter of texture

            vertices.push_back({pos, normal, uv});
        }
    }

    std::uint32_t topHemisphereVertices = (rings + 1) * (segments + 1);

    // Cylinder section (two rings connecting hemispheres)
    for (std::uint32_t s = 0; s <= segments; ++s) {
        float u = static_cast<float>(s) / segments;
        float theta = u * 2.0f * PI;
        float x = std::cos(theta);
        float z = std::sin(theta);

        // Top of cylinder at y = halfHeight
        Vec3 topPos{x * radius, halfHeight, z * radius};
        Vec3 normal = glm::normalize(Vec3{x, 0.0f, z});
        Vec2 topUv{u, 0.25f};
        vertices.push_back({topPos, normal, topUv});

        // Bottom of cylinder at y = -halfHeight
        Vec3 botPos{x * radius, -halfHeight, z * radius};
        Vec2 botUv{u, 0.75f};
        vertices.push_back({botPos, normal, botUv});
    }

    std::uint32_t cylinderVertices = 2 * (segments + 1);

    // Bottom hemisphere (centered at -halfHeight)
    for (std::uint32_t r = 0; r <= rings; ++r) {
        float v = static_cast<float>(r) / rings;
        float phi = PI * 0.5f + v * PI * 0.5f;  // PI/2 to PI (bottom half)

        for (std::uint32_t s = 0; s <= segments; ++s) {
            float u = static_cast<float>(s) / segments;
            float theta = u * 2.0f * PI;

            float sinPhi = std::sin(phi);
            float cosPhi = std::cos(phi);
            float sinTheta = std::sin(theta);
            float cosTheta = std::cos(theta);

            Vec3 normal{sinPhi * cosTheta, cosPhi, sinPhi * sinTheta};
            Vec3 pos = normal * radius + Vec3{0.0f, -halfHeight, 0.0f};
            Vec2 uv{u, 0.75f + v * 0.25f};  // Bottom quarter of texture

            vertices.push_back({pos, normal, uv});
        }
    }

    // Generate indices for top hemisphere (CCW winding for outward-facing normals)
    for (std::uint32_t r = 0; r < rings; ++r) {
        for (std::uint32_t s = 0; s < segments; ++s) {
            std::uint32_t i0 = r * (segments + 1) + s;
            std::uint32_t i1 = i0 + 1;
            std::uint32_t i2 = i0 + (segments + 1);
            std::uint32_t i3 = i2 + 1;

            // CCW winding: i0 → i1 → i2, i1 → i3 → i2
            indices.push_back(i0);
            indices.push_back(i1);
            indices.push_back(i2);

            indices.push_back(i1);
            indices.push_back(i3);
            indices.push_back(i2);
        }
    }

    // Generate indices for cylinder (interleaved top/bottom pairs, CCW winding)
    std::uint32_t cylBase = topHemisphereVertices;
    for (std::uint32_t s = 0; s < segments; ++s) {
        // Each segment has 2 vertices: top and bottom
        std::uint32_t topCurr = cylBase + s * 2;
        std::uint32_t botCurr = cylBase + s * 2 + 1;
        std::uint32_t topNext = cylBase + (s + 1) * 2;
        std::uint32_t botNext = cylBase + (s + 1) * 2 + 1;

        // CCW winding for outward-facing cylinder
        indices.push_back(topCurr);
        indices.push_back(topNext);
        indices.push_back(botCurr);

        indices.push_back(topNext);
        indices.push_back(botNext);
        indices.push_back(botCurr);
    }

    // Generate indices for bottom hemisphere (CCW winding for outward-facing normals)
    std::uint32_t botBase = topHemisphereVertices + cylinderVertices;
    for (std::uint32_t r = 0; r < rings; ++r) {
        for (std::uint32_t s = 0; s < segments; ++s) {
            std::uint32_t i0 = botBase + r * (segments + 1) + s;
            std::uint32_t i1 = i0 + 1;
            std::uint32_t i2 = i0 + (segments + 1);
            std::uint32_t i3 = i2 + 1;

            // CCW winding: i0 → i1 → i2, i1 → i3 → i2
            indices.push_back(i0);
            indices.push_back(i1);
            indices.push_back(i2);

            indices.push_back(i1);
            indices.push_back(i3);
            indices.push_back(i2);
        }
    }

    MeshDef def;
    def.vertices = vertices;
    def.indices = indices;
    // Bounds: total height is height (cylinder) + 2*radius (hemispheres)
    def.bounds = AABB3D{
        Vec3{-radius, -halfHeight - radius, -radius},
        Vec3{radius, halfHeight + radius, radius}
    };

    return createMesh(def);
}

Result<MeshHandle, Graphics3DError> VulkanGraphics3DSystem::createPlaneMesh(
    float width, float height, std::uint32_t widthSegments, std::uint32_t heightSegments) {
    std::vector<Vertex3D> vertices;
    std::vector<std::uint32_t> indices;

    float halfW = width / 2.0f;
    float halfH = height / 2.0f;

    for (std::uint32_t y = 0; y <= heightSegments; ++y) {
        for (std::uint32_t x = 0; x <= widthSegments; ++x) {
            float u = static_cast<float>(x) / widthSegments;
            float v = static_cast<float>(y) / heightSegments;

            vertices.push_back({
                {-halfW + u * width, 0, -halfH + v * height},
                {0, 1, 0},
                {u, v}
            });
        }
    }

    for (std::uint32_t y = 0; y < heightSegments; ++y) {
        for (std::uint32_t x = 0; x < widthSegments; ++x) {
            std::uint32_t i0 = y * (widthSegments + 1) + x;
            std::uint32_t i1 = i0 + 1;
            std::uint32_t i2 = i0 + (widthSegments + 1);
            std::uint32_t i3 = i2 + 1;

            indices.push_back(i0);
            indices.push_back(i2);
            indices.push_back(i1);

            indices.push_back(i1);
            indices.push_back(i2);
            indices.push_back(i3);
        }
    }

    MeshDef def;
    def.vertices = vertices;
    def.indices = indices;
    def.bounds = AABB3D{Vec3{-halfW, 0, -halfH}, Vec3{halfW, 0, halfH}};

    return createMesh(def);
}

Result<MaterialHandle, Graphics3DError> VulkanGraphics3DSystem::createMaterial(const PBRMaterial& mat) {
    VulkanMaterial material;
    material.pbrData = mat;
    material.isPBR = true;

    MaterialHandle handle = nextMaterialHandle_++;
    materials_[handle] = material;
    return handle;
}

Result<MaterialHandle, Graphics3DError> VulkanGraphics3DSystem::createUnlitMaterial(const UnlitMaterial& mat) {
    VulkanMaterial material;
    material.pbrData.baseColorFactor = mat.color;
    material.pbrData.blendMode = mat.blendMode;
    material.pbrData.cullMode = mat.cullMode;
    material.isPBR = false;

    MaterialHandle handle = nextMaterialHandle_++;
    materials_[handle] = material;
    return handle;
}

void VulkanGraphics3DSystem::destroyMaterial(MaterialHandle handle) {
    materials_.erase(handle);
}

bool VulkanGraphics3DSystem::hasMaterial(MaterialHandle handle) const {
    return materials_.contains(handle);
}

Result<void, Graphics3DError> VulkanGraphics3DSystem::setMaterialTexture(
    MaterialHandle handle, std::uint32_t slot, AssetHandle texture) {
    auto it = materials_.find(handle);
    if (it == materials_.end()) {
        return std::unexpected(Graphics3DError::InvalidMaterial);
    }

    // Update the appropriate texture slot in the material
    // Texture slots:
    // 0 = baseColorTexture
    // 1 = metallicRoughnessTexture
    // 2 = normalTexture
    // 3 = occlusionTexture
    // 4 = emissiveTexture
    switch (slot) {
        case 0:
            it->second.pbrData.baseColorTexture = texture;
            break;
        case 1:
            it->second.pbrData.metallicRoughnessTexture = texture;
            break;
        case 2:
            it->second.pbrData.normalTexture = texture;
            break;
        case 3:
            it->second.pbrData.occlusionTexture = texture;
            break;
        case 4:
            it->second.pbrData.emissiveTexture = texture;
            break;
        default:
            return std::unexpected(Graphics3DError::InvalidTexture);
    }

    // NOTE: In a full implementation, this would also update the Vulkan
    // descriptor set to bind the texture for GPU access. Currently, the
    // texture handle is stored and can be used during rendering to look up
    // the actual texture data via the AssetSystem.

    return {};
}

MaterialHandle VulkanGraphics3DSystem::getDefaultPBRMaterial() const {
    return defaultPBRMaterial_;
}

MaterialHandle VulkanGraphics3DSystem::getDefaultUnlitMaterial() const {
    return defaultUnlitMaterial_;
}

MaterialHandle VulkanGraphics3DSystem::getErrorMaterial() const {
    return errorMaterial_;
}

void VulkanGraphics3DSystem::drawMesh(
    MeshHandle mesh, MaterialHandle material, const Mat4& worldMatrix,
    bool castShadow, bool receiveShadow) {
    queueRenderItem(RenderItem{
        .mesh = mesh,
        .material = material,
        .worldMatrix = worldMatrix,
        .castShadow = castShadow,
        .receiveShadow = receiveShadow
    });
}

void VulkanGraphics3DSystem::drawMesh(
    MeshHandle mesh, MaterialHandle material, const Transform3D& transform,
    bool castShadow, bool receiveShadow) {
    // Convert transform to matrix
    glm::mat4 worldMatrix = glm::mat4(1.0f);
    worldMatrix = glm::translate(worldMatrix, glm::vec3{transform.position.x, transform.position.y, transform.position.z});
    worldMatrix = worldMatrix * glm::mat4_cast(glm::quat{transform.rotation.w, transform.rotation.x, transform.rotation.y, transform.rotation.z});
    worldMatrix = glm::scale(worldMatrix, glm::vec3{transform.scale.x, transform.scale.y, transform.scale.z});

    Mat4 mat;
    std::memcpy(&mat, &worldMatrix, sizeof(Mat4));

    drawMesh(mesh, material, mat, castShadow, receiveShadow);
}

void VulkanGraphics3DSystem::queueRenderItem(const RenderItem& item) {
    renderQueue_.push_back(item);
}

void VulkanGraphics3DSystem::queueRenderItems(std::span<const RenderItem> items) {
    for (const auto& item : items) {
        renderQueue_.push_back(item);
    }
}

void VulkanGraphics3DSystem::flushRenderQueue() {
    static int frameCount = 0;
    frameCount++;

    if (renderQueue_.empty()) {
        if (frameCount <= 3) std::fprintf(stderr, "[Vulkan] Frame %d: Render queue is empty\n", frameCount);
        return;
    }

    VkCommandBuffer cmd = context_.getCurrentCommandBuffer();
    if (cmd == VK_NULL_HANDLE || pbrPipeline_ == 0) {
        if (frameCount <= 3) std::fprintf(stderr, "[Vulkan] Frame %d: cmd=%p, pbrPipeline=%u\n",
                                          frameCount, (void*)cmd, pbrPipeline_);
        renderQueue_.clear();
        return;
    }

    if (frameCount <= 3) std::fprintf(stderr, "[Vulkan] Frame %d: Rendering %zu items\n",
                                      frameCount, renderQueue_.size());

    // Compute camera position for distance sorting
    Vec3 cameraWorldPos = camera_.transform.position;

    // Sort render queue for optimal rendering:
    // 1. By render layer (lower layers rendered first)
    // 2. Within layer: opaque objects before transparent
    // 3. Opaque: front-to-back (early-Z optimization)
    // 4. Transparent: back-to-front (correct blending)
    // 5. Within same category: batch by material
    std::sort(renderQueue_.begin(), renderQueue_.end(),
              [this, &cameraWorldPos](const RenderItem& a, const RenderItem& b) {
                  // First, sort by render layer
                  if (a.layer != b.layer) {
                      return a.layer < b.layer;
                  }

                  // Check if materials are transparent
                  bool aTransparent = false;
                  bool bTransparent = false;

                  auto aMatIt = materials_.find(a.material);
                  auto bMatIt = materials_.find(b.material);

                  if (aMatIt != materials_.end()) {
                      aTransparent = (aMatIt->second.pbrData.blendMode == BlendMode::AlphaBlend ||
                                      aMatIt->second.pbrData.blendMode == BlendMode::Additive);
                  }
                  if (bMatIt != materials_.end()) {
                      bTransparent = (bMatIt->second.pbrData.blendMode == BlendMode::AlphaBlend ||
                                      bMatIt->second.pbrData.blendMode == BlendMode::Additive);
                  }

                  // Opaque objects before transparent
                  if (aTransparent != bTransparent) {
                      return !aTransparent;  // false (opaque) comes before true (transparent)
                  }

                  // Calculate squared distances (avoid sqrt for performance)
                  float aDistSq = (a.worldBounds.min.x + a.worldBounds.max.x) * 0.5f - cameraWorldPos.x;
                  aDistSq *= aDistSq;
                  float tempA = (a.worldBounds.min.y + a.worldBounds.max.y) * 0.5f - cameraWorldPos.y;
                  aDistSq += tempA * tempA;
                  tempA = (a.worldBounds.min.z + a.worldBounds.max.z) * 0.5f - cameraWorldPos.z;
                  aDistSq += tempA * tempA;

                  float bDistSq = (b.worldBounds.min.x + b.worldBounds.max.x) * 0.5f - cameraWorldPos.x;
                  bDistSq *= bDistSq;
                  float tempB = (b.worldBounds.min.y + b.worldBounds.max.y) * 0.5f - cameraWorldPos.y;
                  bDistSq += tempB * tempB;
                  tempB = (b.worldBounds.min.z + b.worldBounds.max.z) * 0.5f - cameraWorldPos.z;
                  bDistSq += tempB * tempB;

                  if (aTransparent) {
                      // Transparent: back-to-front (farther objects first)
                      if (std::abs(aDistSq - bDistSq) > 0.001f) {
                          return aDistSq > bDistSq;
                      }
                  } else {
                      // Opaque: front-to-back (closer objects first for early-Z)
                      if (std::abs(aDistSq - bDistSq) > 0.001f) {
                          return aDistSq < bDistSq;
                      }
                  }

                  // Finally, batch by material
                  return a.material < b.material;
              });

    // Compute view-projection matrix
    Size windowSize = getWindowSize();
    auto& pos = camera_.transform.position;
    glm::quat rot{camera_.transform.rotation.w, camera_.transform.rotation.x,
                  camera_.transform.rotation.y, camera_.transform.rotation.z};
    glm::vec3 cameraPos{pos.x, pos.y, pos.z};
    glm::vec3 forward = rot * glm::vec3{0.0f, 0.0f, -1.0f};
    glm::vec3 up = rot * glm::vec3{0.0f, 1.0f, 0.0f};
    glm::mat4 view = glm::lookAt(cameraPos, cameraPos + forward, up);
    float aspect = windowSize.height > 0 ?
        static_cast<float>(windowSize.width) / static_cast<float>(windowSize.height) : 1.0f;
    glm::mat4 projection = glm::perspective(glm::radians(camera_.fovY), aspect, camera_.nearPlane, camera_.farPlane);
    projection[1][1] *= -1;  // Flip Y for Vulkan
    glm::mat4 viewProjection = projection * view;

    // Bind PBR pipeline
    context_.bindPipeline(pbrPipeline_);
    VkPipelineLayout layout = context_.getPipelineLayout(pbrPipeline_);

    // Get light direction from directional light or use default
    glm::vec4 lightDir{0.5f, -1.0f, 0.3f, 0.0f};
    glm::vec4 lightColor{1.0f, 1.0f, 1.0f, 1.0f};
    if (hasDirectionalLight_) {
        lightDir = glm::vec4{directionalLight_.direction.x, directionalLight_.direction.y,
                            directionalLight_.direction.z, 0.0f};
        lightColor = glm::vec4{directionalLight_.color.r, directionalLight_.color.g,
                              directionalLight_.color.b, directionalLight_.intensity};
    }
    glm::vec4 ambientColor{ambientColor_.x, ambientColor_.y, ambientColor_.z, ambientIntensity_};

    // Track current pipeline to minimize rebinds
    VulkanPipelineHandle currentPipeline = pbrPipeline_;

    // Draw each item
    for (const auto& item : renderQueue_) {
        auto meshIt = meshes_.find(item.mesh);
        if (meshIt == meshes_.end()) continue;

        const auto& mesh = meshIt->second;
        if (mesh.vertexBuffer == 0) continue;

        // Use custom pipeline if specified, otherwise use PBR pipeline
        VulkanPipelineHandle itemPipeline = (item.customPipeline != 0) ?
            static_cast<VulkanPipelineHandle>(item.customPipeline) : pbrPipeline_;

        // Rebind pipeline if changed
        if (itemPipeline != currentPipeline) {
            context_.bindPipeline(itemPipeline);
            layout = context_.getPipelineLayout(itemPipeline);
            currentPipeline = itemPipeline;
        }

        // Get model matrix from RenderItem worldMatrix
        glm::mat4 model;
        std::memcpy(&model, &item.worldMatrix, sizeof(glm::mat4));

        // Get material color, then apply color override
        glm::vec4 baseColor{1.0f, 1.0f, 1.0f, 1.0f};
        auto matIt = materials_.find(item.material);
        if (matIt != materials_.end()) {
            const auto& bc = matIt->second.pbrData.baseColorFactor;
            baseColor = glm::vec4{bc.x, bc.y, bc.z, bc.w};
        }

        // Apply color override from RenderItem
        baseColor.r *= item.colorOverride.x;
        baseColor.g *= item.colorOverride.y;
        baseColor.b *= item.colorOverride.z;
        baseColor.a *= item.colorOverride.w;

        // Push constants: model + viewProjection + baseColor + lightDir + lightColor + ambientColor + cameraPos = 208 bytes
        glm::vec4 camPos{cameraPos.x, cameraPos.y, cameraPos.z, 1.0f};
        struct PushData {
            glm::mat4 model;
            glm::mat4 viewProjection;
            glm::vec4 baseColor;
            glm::vec4 lightDir;
            glm::vec4 lightColor;
            glm::vec4 ambientColor;
            glm::vec4 cameraPos;
        } pushData = {model, viewProjection, baseColor, lightDir, lightColor, ambientColor, camPos};

        vkCmdPushConstants(cmd, layout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0,
                          sizeof(pushData), &pushData);

        // Bind vertex buffer
        VkBuffer vertexBuffer = context_.getBuffer(mesh.vertexBuffer);
        if (vertexBuffer == VK_NULL_HANDLE) continue;

        VkDeviceSize offset = 0;
        vkCmdBindVertexBuffers(cmd, 0, 1, &vertexBuffer, &offset);

        // Draw
        if (mesh.indexBuffer != 0 && mesh.indexCount > 0) {
            VkBuffer indexBuffer = context_.getBuffer(mesh.indexBuffer);
            if (indexBuffer != VK_NULL_HANDLE) {
                vkCmdBindIndexBuffer(cmd, indexBuffer, 0, VK_INDEX_TYPE_UINT32);
                vkCmdDrawIndexed(cmd, mesh.indexCount, 1, 0, 0, 0);
            }
        } else if (mesh.vertexCount > 0) {
            vkCmdDraw(cmd, mesh.vertexCount, 1, 0, 0);
        }
    }

    renderQueue_.clear();

    // Draw skinned meshes
    if (!skinnedRenderQueue_.empty() && skinnedPipeline_ != 0) {
        // Bind skinned pipeline
        context_.bindPipeline(skinnedPipeline_);
        VkPipelineLayout skinnedLayout = context_.getPipelineLayout(skinnedPipeline_);

        // Bind bone descriptor set (set 0)
        if (boneDescriptorSet_ != VK_NULL_HANDLE) {
            vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
                                    skinnedLayout, 0, 1, &boneDescriptorSet_, 0, nullptr);
        }

        for (const auto& item : skinnedRenderQueue_) {
            auto meshIt = meshes_.find(item.mesh);
            if (meshIt == meshes_.end()) continue;

            const auto& mesh = meshIt->second;
            if (mesh.vertexBuffer == 0) continue;

            // Upload bone transforms to UBO
            std::size_t numBones = std::min(item.boneTransforms.size(), MAX_BONES);
            if (numBones > 0) {
                context_.uploadToBuffer(boneUBO_, item.boneTransforms.data(),
                                        numBones * sizeof(Mat4));
            }

            // Get model matrix
            glm::mat4 model;
            std::memcpy(&model, &item.worldMatrix, sizeof(glm::mat4));

            // Get material color
            glm::vec4 baseColor{1.0f, 1.0f, 1.0f, 1.0f};
            auto matIt = materials_.find(item.material);
            if (matIt != materials_.end()) {
                const auto& bc = matIt->second.pbrData.baseColorFactor;
                baseColor = glm::vec4{bc.x, bc.y, bc.z, bc.w};
            }

            // Bind texture descriptor set (set 1)
            // Look for material-specific texture, otherwise use default white
            VkDescriptorSet texDescSet = defaultTextureDescriptorSet_;
            auto texSetIt = materialTextureDescriptorSets_.find(item.material);
            if (texSetIt != materialTextureDescriptorSets_.end()) {
                texDescSet = texSetIt->second;
            }
            if (texDescSet != VK_NULL_HANDLE) {
                vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
                                        skinnedLayout, 1, 1, &texDescSet, 0, nullptr);
            }

            // Push constants (same as regular meshes)
            glm::vec4 camPos{cameraPos.x, cameraPos.y, cameraPos.z, 1.0f};
            struct PushData {
                glm::mat4 model;
                glm::mat4 viewProjection;
                glm::vec4 baseColor;
                glm::vec4 lightDir;
                glm::vec4 lightColor;
                glm::vec4 ambientColor;
                glm::vec4 cameraPos;
            } pushData = {model, viewProjection, baseColor, lightDir, lightColor, ambientColor, camPos};

            vkCmdPushConstants(cmd, skinnedLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0,
                              sizeof(pushData), &pushData);

            // Bind vertex buffer
            VkBuffer vertexBuffer = context_.getBuffer(mesh.vertexBuffer);
            if (vertexBuffer == VK_NULL_HANDLE) continue;

            VkDeviceSize offset = 0;
            vkCmdBindVertexBuffers(cmd, 0, 1, &vertexBuffer, &offset);

            // Draw
            if (mesh.indexBuffer != 0 && mesh.indexCount > 0) {
                VkBuffer indexBuffer = context_.getBuffer(mesh.indexBuffer);
                if (indexBuffer != VK_NULL_HANDLE) {
                    vkCmdBindIndexBuffer(cmd, indexBuffer, 0, VK_INDEX_TYPE_UINT32);
                    vkCmdDrawIndexed(cmd, mesh.indexCount, 1, 0, 0, 0);
                }
            } else if (mesh.vertexCount > 0) {
                vkCmdDraw(cmd, mesh.vertexCount, 1, 0, 0);
            }
        }

        skinnedRenderQueue_.clear();
    }
}

void VulkanGraphics3DSystem::renderEntities(IEntitySystem& entities) {
    // Iterate through entities with Mesh3DComponent and Transform3D components
    auto meshView = entities.view<Mesh3DComponent, Transform3D>();

    for (auto entity : meshView) {
        const auto& meshComp = entities.get<Mesh3DComponent>(entity);
        const auto& transform = entities.get<Transform3D>(entity);

        if (!meshComp.visible) continue;

        // Convert transform to matrix
        glm::mat4 worldMatrix = glm::mat4(1.0f);
        worldMatrix = glm::translate(worldMatrix, glm::vec3{transform.position.x, transform.position.y, transform.position.z});
        worldMatrix = worldMatrix * glm::mat4_cast(glm::quat{transform.rotation.w, transform.rotation.x, transform.rotation.y, transform.rotation.z});
        worldMatrix = glm::scale(worldMatrix, glm::vec3{transform.scale.x, transform.scale.y, transform.scale.z});

        Mat4 mat;
        std::memcpy(&mat, &worldMatrix, sizeof(Mat4));

        queueRenderItem(RenderItem{
            .mesh = meshComp.mesh,
            .material = meshComp.material,
            .worldMatrix = mat,
            .layer = meshComp.layer,
            .castShadow = meshComp.castShadow,
            .receiveShadow = meshComp.receiveShadow
        });
    }
}

void VulkanGraphics3DSystem::renderEntities(IEntitySystem& entities, const Frustum& frustum) {
    // Render with frustum culling
    auto meshView = entities.view<Mesh3DComponent, Transform3D>();

    for (auto entity : meshView) {
        const auto& meshComp = entities.get<Mesh3DComponent>(entity);
        const auto& transform = entities.get<Transform3D>(entity);

        if (!meshComp.visible) continue;

        // Frustum culling - check if entity bounds are in frustum
        if (frustumCullingEnabled_ && meshComp.mesh != 0) {
            AABB3D bounds = getMeshBounds(meshComp.mesh);
            // Transform AABB by entity transform (simplified - center point test)
            Vec3 center{
                (bounds.min.x + bounds.max.x) * 0.5f + transform.position.x,
                (bounds.min.y + bounds.max.y) * 0.5f + transform.position.y,
                (bounds.min.z + bounds.max.z) * 0.5f + transform.position.z
            };

            // Simple point-in-frustum test using all 6 planes
            bool inFrustum = true;
            for (int i = 0; i < 6 && inFrustum; ++i) {
                const auto& plane = frustum.planes[i];
                float dist = plane.normal.x * center.x + plane.normal.y * center.y +
                            plane.normal.z * center.z + plane.distance;
                if (dist < -bounds.max.x) {  // Use max extent as radius approximation
                    inFrustum = false;
                }
            }

            if (!inFrustum) continue;
        }

        // Convert transform to matrix
        glm::mat4 worldMatrix = glm::mat4(1.0f);
        worldMatrix = glm::translate(worldMatrix, glm::vec3{transform.position.x, transform.position.y, transform.position.z});
        worldMatrix = worldMatrix * glm::mat4_cast(glm::quat{transform.rotation.w, transform.rotation.x, transform.rotation.y, transform.rotation.z});
        worldMatrix = glm::scale(worldMatrix, glm::vec3{transform.scale.x, transform.scale.y, transform.scale.z});

        Mat4 mat;
        std::memcpy(&mat, &worldMatrix, sizeof(Mat4));

        queueRenderItem(RenderItem{
            .mesh = meshComp.mesh,
            .material = meshComp.material,
            .worldMatrix = mat,
            .layer = meshComp.layer,
            .castShadow = meshComp.castShadow,
            .receiveShadow = meshComp.receiveShadow
        });
    }
}

void VulkanGraphics3DSystem::renderEntities(
    IEntitySystem& entities, RenderLayer minLayer, RenderLayer maxLayer) {
    // Render layer range
    auto meshView = entities.view<Mesh3DComponent, Transform3D>();

    for (auto entity : meshView) {
        const auto& meshComp = entities.get<Mesh3DComponent>(entity);
        const auto& transform = entities.get<Transform3D>(entity);

        if (!meshComp.visible) continue;
        if (meshComp.layer < minLayer || meshComp.layer > maxLayer) continue;

        // Convert transform to matrix
        glm::mat4 worldMatrix = glm::mat4(1.0f);
        worldMatrix = glm::translate(worldMatrix, glm::vec3{transform.position.x, transform.position.y, transform.position.z});
        worldMatrix = worldMatrix * glm::mat4_cast(glm::quat{transform.rotation.w, transform.rotation.x, transform.rotation.y, transform.rotation.z});
        worldMatrix = glm::scale(worldMatrix, glm::vec3{transform.scale.x, transform.scale.y, transform.scale.z});

        Mat4 mat;
        std::memcpy(&mat, &worldMatrix, sizeof(Mat4));

        queueRenderItem(RenderItem{
            .mesh = meshComp.mesh,
            .material = meshComp.material,
            .worldMatrix = mat,
            .layer = meshComp.layer,
            .castShadow = meshComp.castShadow,
            .receiveShadow = meshComp.receiveShadow
        });
    }
}

void VulkanGraphics3DSystem::setCamera(const Camera3D& camera) {
    camera_ = camera;
}

Camera3D VulkanGraphics3DSystem::getCamera() const {
    return camera_;
}

Ray3D VulkanGraphics3DSystem::screenToWorldRay(Vec2 screenPos) const {
    // Convert screen pos to normalized device coordinates
    Size windowSize = context_.getWindowSize();
    float x = (2.0f * screenPos.x) / windowSize.width - 1.0f;
    float y = 1.0f - (2.0f * screenPos.y) / windowSize.height;

    // Create ray from camera
    return Ray3D{camera_.transform.position, Vec3{x, y, -1.0f}};  // Simplified
}

std::optional<Vec2> VulkanGraphics3DSystem::worldToScreen(const Vec3& worldPos) const {
    // Would project world position using view-projection matrix
    Size windowSize = context_.getWindowSize();
    return Vec2{windowSize.width / 2.0f, windowSize.height / 2.0f};  // Placeholder
}

void VulkanGraphics3DSystem::setDirectionalLight(const DirectionalLight& light) {
    directionalLight_ = light;
    hasDirectionalLight_ = true;
}

void VulkanGraphics3DSystem::clearDirectionalLight() {
    hasDirectionalLight_ = false;
}

std::uint32_t VulkanGraphics3DSystem::addPointLight(const PointLight& light, const Vec3& position) {
    std::uint32_t id = nextLightId_++;
    pointLights_[id] = {light, position};
    return id;
}

std::uint32_t VulkanGraphics3DSystem::addSpotLight(const SpotLight& light, const Vec3& position) {
    std::uint32_t id = nextLightId_++;
    spotLights_[id] = {light, position};
    return id;
}

void VulkanGraphics3DSystem::setLightPosition(std::uint32_t lightId, const Vec3& position) {
    if (auto it = pointLights_.find(lightId); it != pointLights_.end()) {
        it->second.second = position;
    }
    if (auto it = spotLights_.find(lightId); it != spotLights_.end()) {
        it->second.second = position;
    }
}

void VulkanGraphics3DSystem::removeLight(std::uint32_t lightId) {
    pointLights_.erase(lightId);
    spotLights_.erase(lightId);
}

void VulkanGraphics3DSystem::clearLights() {
    pointLights_.clear();
    spotLights_.clear();
}

void VulkanGraphics3DSystem::setAmbientLight(const Vec3& color, float intensity) {
    ambientColor_ = color;
    ambientIntensity_ = intensity;
}

void VulkanGraphics3DSystem::updateEntityLights(IEntitySystem& entities) {
    // Clear existing dynamic lights (keep manually added lights)
    // NOTE: This assumes all lights should come from entities when this method is called
    clearLights();

    // Get the view of entities with both Light3DComponent and Transform3D
    auto lightView = entities.view<Light3DComponent, Transform3D>();

    for (auto entity : lightView) {
        const auto& lightComp = lightView.get<Light3DComponent>(entity);
        const auto& transform = lightView.get<Transform3D>(entity);

        // Skip disabled lights
        if (!lightComp.enabled) continue;

        const Light3D& light = lightComp.light;
        Vec3 position = transform.position;

        switch (light.type) {
            case LightType::Directional: {
                // Directional lights use transform rotation to determine direction
                // Forward vector is typically (0, 0, -1) in local space, rotated by quaternion
                Vec3 forward{0.0f, 0.0f, -1.0f};
                // Apply quaternion rotation: q * v * q^(-1)
                // Simplified rotation formula for a vector
                Quat q = transform.rotation;
                float qw = q.w, qx = q.x, qy = q.y, qz = q.z;

                float vx = forward.x, vy = forward.y, vz = forward.z;

                // Compute rotated vector using quaternion rotation formula
                float ix = qw * vx + qy * vz - qz * vy;
                float iy = qw * vy + qz * vx - qx * vz;
                float iz = qw * vz + qx * vy - qy * vx;
                float iw = -qx * vx - qy * vy - qz * vz;

                Vec3 direction{
                    ix * qw + iw * -qx + iy * -qz - iz * -qy,
                    iy * qw + iw * -qy + iz * -qx - ix * -qz,
                    iz * qw + iw * -qz + ix * -qy - iy * -qx
                };

                DirectionalLight dirLight;
                dirLight.direction = direction;
                dirLight.color = light.color;
                dirLight.intensity = light.intensity;
                dirLight.castShadows = light.castShadows;
                setDirectionalLight(dirLight);
                break;
            }

            case LightType::Point: {
                PointLight pointLight;
                pointLight.color = light.color;
                pointLight.intensity = light.intensity;
                pointLight.range = light.range;
                pointLight.castShadows = light.castShadows;
                addPointLight(pointLight, position);
                break;
            }

            case LightType::Spot: {
                // Spot lights also need direction from rotation
                Vec3 forward{0.0f, 0.0f, -1.0f};
                Quat q = transform.rotation;
                float qw = q.w, qx = q.x, qy = q.y, qz = q.z;

                float vx = forward.x, vy = forward.y, vz = forward.z;

                float ix = qw * vx + qy * vz - qz * vy;
                float iy = qw * vy + qz * vx - qx * vz;
                float iz = qw * vz + qx * vy - qy * vx;
                float iw = -qx * vx - qy * vy - qz * vz;

                Vec3 direction{
                    ix * qw + iw * -qx + iy * -qz - iz * -qy,
                    iy * qw + iw * -qy + iz * -qx - ix * -qz,
                    iz * qw + iw * -qz + ix * -qy - iy * -qx
                };

                SpotLight spotLight;
                spotLight.direction = direction;
                spotLight.color = light.color;
                spotLight.intensity = light.intensity;
                spotLight.range = light.range;
                spotLight.innerConeAngle = light.innerConeAngle;
                spotLight.outerConeAngle = light.outerConeAngle;
                spotLight.castShadows = light.castShadows;
                addSpotLight(spotLight, position);
                break;
            }
        }
    }
}

void VulkanGraphics3DSystem::setSkybox(const Skybox& skybox) {
    skybox_ = skybox;
    hasSkybox_ = true;
}

void VulkanGraphics3DSystem::clearSkybox() {
    if (skyboxCubemapImage_ != 0) {
        context_.destroyImage(skyboxCubemapImage_);
        skyboxCubemapImage_ = 0;
    }
    skyboxCubemapData_ = CubemapData{};
    hasSkybox_ = false;
}

void VulkanGraphics3DSystem::setEnvironmentMap(const EnvironmentMap& envMap) {
    environmentMap_ = envMap;
    hasEnvironmentMap_ = true;
}

void VulkanGraphics3DSystem::clearEnvironmentMap() {
    hasEnvironmentMap_ = false;
}

void VulkanGraphics3DSystem::setFog(const Fog& fog) {
    fog_ = fog;
}

void VulkanGraphics3DSystem::setShadowsEnabled(bool enabled) {
    shadowsEnabled_ = enabled;
}

bool VulkanGraphics3DSystem::areShadowsEnabled() const {
    return shadowsEnabled_;
}

void VulkanGraphics3DSystem::setDirectionalShadowResolution(int resolution) {
    shadowResolution_ = resolution;
}

void VulkanGraphics3DSystem::setShadowDistance(float distance) {
    shadowDistance_ = distance;
}

void VulkanGraphics3DSystem::debugDrawLine(
    const Vec3& start, const Vec3& end, const Color& color,
    float duration, bool depthTest) {
    if (!debugRenderingEnabled_) return;
    debugLines_.push_back({start, end, color, duration, depthTest, duration});
}

void VulkanGraphics3DSystem::debugDrawBox(
    const Vec3& center, const Vec3& halfExtents, const Quat& rotation,
    const Color& color, float duration, bool depthTest) {
    if (!debugRenderingEnabled_) return;
    // Draw 12 edges of the box
    Vec3 corners[8];
    for (int i = 0; i < 8; ++i) {
        corners[i] = Vec3{
            center.x + halfExtents.x * ((i & 1) ? 1 : -1),
            center.y + halfExtents.y * ((i & 2) ? 1 : -1),
            center.z + halfExtents.z * ((i & 4) ? 1 : -1)
        };
    }

    // Bottom face
    debugDrawLine(corners[0], corners[1], color, duration, depthTest);
    debugDrawLine(corners[1], corners[3], color, duration, depthTest);
    debugDrawLine(corners[3], corners[2], color, duration, depthTest);
    debugDrawLine(corners[2], corners[0], color, duration, depthTest);

    // Top face
    debugDrawLine(corners[4], corners[5], color, duration, depthTest);
    debugDrawLine(corners[5], corners[7], color, duration, depthTest);
    debugDrawLine(corners[7], corners[6], color, duration, depthTest);
    debugDrawLine(corners[6], corners[4], color, duration, depthTest);

    // Vertical edges
    debugDrawLine(corners[0], corners[4], color, duration, depthTest);
    debugDrawLine(corners[1], corners[5], color, duration, depthTest);
    debugDrawLine(corners[2], corners[6], color, duration, depthTest);
    debugDrawLine(corners[3], corners[7], color, duration, depthTest);
}

void VulkanGraphics3DSystem::debugDrawSphere(
    const Vec3& center, float radius, const Color& color,
    float duration, bool depthTest) {
    if (!debugRenderingEnabled_) return;
    // Draw circles in 3 planes
    const int segments = 16;
    for (int i = 0; i < segments; ++i) {
        float a1 = 2.0f * 3.14159f * i / segments;
        float a2 = 2.0f * 3.14159f * (i + 1) / segments;

        // XY plane
        debugDrawLine(
            Vec3{center.x + radius * std::cos(a1), center.y + radius * std::sin(a1), center.z},
            Vec3{center.x + radius * std::cos(a2), center.y + radius * std::sin(a2), center.z},
            color, duration, depthTest);

        // XZ plane
        debugDrawLine(
            Vec3{center.x + radius * std::cos(a1), center.y, center.z + radius * std::sin(a1)},
            Vec3{center.x + radius * std::cos(a2), center.y, center.z + radius * std::sin(a2)},
            color, duration, depthTest);

        // YZ plane
        debugDrawLine(
            Vec3{center.x, center.y + radius * std::cos(a1), center.z + radius * std::sin(a1)},
            Vec3{center.x, center.y + radius * std::cos(a2), center.z + radius * std::sin(a2)},
            color, duration, depthTest);
    }
}

void VulkanGraphics3DSystem::debugDrawCapsule(
    const Vec3& start, const Vec3& end, float radius,
    const Color& color, float duration, bool depthTest) {
    if (!debugRenderingEnabled_) return;

    constexpr int segments = 16;
    constexpr int hemisphereRings = 4;

    // Calculate the capsule axis
    Vec3 axis{end.x - start.x, end.y - start.y, end.z - start.z};
    float length = std::sqrt(axis.x * axis.x + axis.y * axis.y + axis.z * axis.z);

    if (length < 0.0001f) {
        // Degenerate capsule - just draw a sphere
        debugDrawSphere(start, radius, color, duration, depthTest);
        return;
    }

    // Normalize axis
    Vec3 axisNorm{axis.x / length, axis.y / length, axis.z / length};

    // Find perpendicular vectors using Gram-Schmidt
    Vec3 perp1, perp2;
    if (std::abs(axisNorm.x) < 0.9f) {
        perp1 = Vec3{1.0f, 0.0f, 0.0f};
    } else {
        perp1 = Vec3{0.0f, 1.0f, 0.0f};
    }
    // perp1 = perp1 - (perp1 dot axisNorm) * axisNorm
    float dot1 = perp1.x * axisNorm.x + perp1.y * axisNorm.y + perp1.z * axisNorm.z;
    perp1.x -= dot1 * axisNorm.x;
    perp1.y -= dot1 * axisNorm.y;
    perp1.z -= dot1 * axisNorm.z;
    float perp1Len = std::sqrt(perp1.x * perp1.x + perp1.y * perp1.y + perp1.z * perp1.z);
    perp1.x /= perp1Len;
    perp1.y /= perp1Len;
    perp1.z /= perp1Len;

    // perp2 = axisNorm cross perp1
    perp2.x = axisNorm.y * perp1.z - axisNorm.z * perp1.y;
    perp2.y = axisNorm.z * perp1.x - axisNorm.x * perp1.z;
    perp2.z = axisNorm.x * perp1.y - axisNorm.y * perp1.x;

    // Draw cylinder rings at start and end
    for (int i = 0; i < segments; ++i) {
        float a1 = 2.0f * 3.14159f * i / segments;
        float a2 = 2.0f * 3.14159f * (i + 1) / segments;

        float c1 = std::cos(a1), s1 = std::sin(a1);
        float c2 = std::cos(a2), s2 = std::sin(a2);

        // Points on the cylinder at start
        Vec3 p1s{start.x + radius * (c1 * perp1.x + s1 * perp2.x),
                 start.y + radius * (c1 * perp1.y + s1 * perp2.y),
                 start.z + radius * (c1 * perp1.z + s1 * perp2.z)};
        Vec3 p2s{start.x + radius * (c2 * perp1.x + s2 * perp2.x),
                 start.y + radius * (c2 * perp1.y + s2 * perp2.y),
                 start.z + radius * (c2 * perp1.z + s2 * perp2.z)};

        // Points on the cylinder at end
        Vec3 p1e{end.x + radius * (c1 * perp1.x + s1 * perp2.x),
                 end.y + radius * (c1 * perp1.y + s1 * perp2.y),
                 end.z + radius * (c1 * perp1.z + s1 * perp2.z)};
        Vec3 p2e{end.x + radius * (c2 * perp1.x + s2 * perp2.x),
                 end.y + radius * (c2 * perp1.y + s2 * perp2.y),
                 end.z + radius * (c2 * perp1.z + s2 * perp2.z)};

        // Draw ring segments at start and end
        debugDrawLine(p1s, p2s, color, duration, depthTest);
        debugDrawLine(p1e, p2e, color, duration, depthTest);

        // Draw longitudinal lines along cylinder (4 lines evenly spaced)
        if (i % (segments / 4) == 0) {
            debugDrawLine(p1s, p1e, color, duration, depthTest);
        }
    }

    // Draw hemispherical caps
    for (int ring = 1; ring <= hemisphereRings; ++ring) {
        float phi = (3.14159f / 2.0f) * ring / hemisphereRings;  // 0 to pi/2
        float ringRadius = radius * std::cos(phi);
        float ringOffset = radius * std::sin(phi);

        for (int i = 0; i < segments; ++i) {
            float a1 = 2.0f * 3.14159f * i / segments;
            float a2 = 2.0f * 3.14159f * (i + 1) / segments;

            float c1 = std::cos(a1), s1 = std::sin(a1);
            float c2 = std::cos(a2), s2 = std::sin(a2);

            // Start hemisphere (pointing in -axis direction)
            Vec3 p1{start.x - axisNorm.x * ringOffset + ringRadius * (c1 * perp1.x + s1 * perp2.x),
                    start.y - axisNorm.y * ringOffset + ringRadius * (c1 * perp1.y + s1 * perp2.y),
                    start.z - axisNorm.z * ringOffset + ringRadius * (c1 * perp1.z + s1 * perp2.z)};
            Vec3 p2{start.x - axisNorm.x * ringOffset + ringRadius * (c2 * perp1.x + s2 * perp2.x),
                    start.y - axisNorm.y * ringOffset + ringRadius * (c2 * perp1.y + s2 * perp2.y),
                    start.z - axisNorm.z * ringOffset + ringRadius * (c2 * perp1.z + s2 * perp2.z)};

            debugDrawLine(p1, p2, color, duration, depthTest);

            // End hemisphere (pointing in +axis direction)
            Vec3 p3{end.x + axisNorm.x * ringOffset + ringRadius * (c1 * perp1.x + s1 * perp2.x),
                    end.y + axisNorm.y * ringOffset + ringRadius * (c1 * perp1.y + s1 * perp2.y),
                    end.z + axisNorm.z * ringOffset + ringRadius * (c1 * perp1.z + s1 * perp2.z)};
            Vec3 p4{end.x + axisNorm.x * ringOffset + ringRadius * (c2 * perp1.x + s2 * perp2.x),
                    end.y + axisNorm.y * ringOffset + ringRadius * (c2 * perp1.y + s2 * perp2.y),
                    end.z + axisNorm.z * ringOffset + ringRadius * (c2 * perp1.z + s2 * perp2.z)};

            debugDrawLine(p3, p4, color, duration, depthTest);
        }
    }

    // Draw meridian lines on hemispheres (4 evenly spaced)
    for (int i = 0; i < 4; ++i) {
        float angle = 2.0f * 3.14159f * i / 4;
        float c = std::cos(angle), s = std::sin(angle);

        Vec3 dir{c * perp1.x + s * perp2.x, c * perp1.y + s * perp2.y, c * perp1.z + s * perp2.z};

        // Draw arc from cylinder edge to pole at start
        for (int j = 0; j < hemisphereRings; ++j) {
            float phi1 = (3.14159f / 2.0f) * j / hemisphereRings;
            float phi2 = (3.14159f / 2.0f) * (j + 1) / hemisphereRings;

            Vec3 p1{start.x + radius * std::cos(phi1) * dir.x - radius * std::sin(phi1) * axisNorm.x,
                    start.y + radius * std::cos(phi1) * dir.y - radius * std::sin(phi1) * axisNorm.y,
                    start.z + radius * std::cos(phi1) * dir.z - radius * std::sin(phi1) * axisNorm.z};
            Vec3 p2{start.x + radius * std::cos(phi2) * dir.x - radius * std::sin(phi2) * axisNorm.x,
                    start.y + radius * std::cos(phi2) * dir.y - radius * std::sin(phi2) * axisNorm.y,
                    start.z + radius * std::cos(phi2) * dir.z - radius * std::sin(phi2) * axisNorm.z};

            debugDrawLine(p1, p2, color, duration, depthTest);
        }

        // Draw arc from cylinder edge to pole at end
        for (int j = 0; j < hemisphereRings; ++j) {
            float phi1 = (3.14159f / 2.0f) * j / hemisphereRings;
            float phi2 = (3.14159f / 2.0f) * (j + 1) / hemisphereRings;

            Vec3 p1{end.x + radius * std::cos(phi1) * dir.x + radius * std::sin(phi1) * axisNorm.x,
                    end.y + radius * std::cos(phi1) * dir.y + radius * std::sin(phi1) * axisNorm.y,
                    end.z + radius * std::cos(phi1) * dir.z + radius * std::sin(phi1) * axisNorm.z};
            Vec3 p2{end.x + radius * std::cos(phi2) * dir.x + radius * std::sin(phi2) * axisNorm.x,
                    end.y + radius * std::cos(phi2) * dir.y + radius * std::sin(phi2) * axisNorm.y,
                    end.z + radius * std::cos(phi2) * dir.z + radius * std::sin(phi2) * axisNorm.z};

            debugDrawLine(p1, p2, color, duration, depthTest);
        }
    }
}

void VulkanGraphics3DSystem::debugDrawFrustum(
    const Frustum& frustum, const Color& color,
    float duration, bool depthTest) {
    if (!debugRenderingEnabled_) return;

    // Helper lambda to intersect 3 planes and get a point
    // Planes: p1*x + d1 = 0 where p1 is normal
    // Solve: [n1; n2; n3] * point = -[d1; d2; d3]
    auto intersectPlanes = [](const Plane& p1, const Plane& p2, const Plane& p3) -> std::optional<Vec3> {
        // Build matrix from normals (rows)
        // Using Cramer's rule for 3x3 system
        float a11 = p1.normal.x, a12 = p1.normal.y, a13 = p1.normal.z;
        float a21 = p2.normal.x, a22 = p2.normal.y, a23 = p2.normal.z;
        float a31 = p3.normal.x, a32 = p3.normal.y, a33 = p3.normal.z;

        // Determinant of coefficient matrix
        float det = a11 * (a22 * a33 - a23 * a32)
                  - a12 * (a21 * a33 - a23 * a31)
                  + a13 * (a21 * a32 - a22 * a31);

        if (std::abs(det) < 1e-6f) {
            return std::nullopt;  // Planes don't intersect at a point
        }

        // Right-hand side (negative distances)
        float b1 = -p1.distance;
        float b2 = -p2.distance;
        float b3 = -p3.distance;

        // Solve using Cramer's rule
        float x = (b1 * (a22 * a33 - a23 * a32)
                 - a12 * (b2 * a33 - a23 * b3)
                 + a13 * (b2 * a32 - a22 * b3)) / det;

        float y = (a11 * (b2 * a33 - a23 * b3)
                 - b1 * (a21 * a33 - a23 * a31)
                 + a13 * (a21 * b3 - b2 * a31)) / det;

        float z = (a11 * (a22 * b3 - b2 * a32)
                 - a12 * (a21 * b3 - b2 * a31)
                 + b1 * (a21 * a32 - a22 * a31)) / det;

        return Vec3{x, y, z};
    };

    // Frustum planes order: Near (0), Far (1), Left (2), Right (3), Top (4), Bottom (5)
    // Calculate 8 corners by intersecting appropriate planes
    std::array<std::optional<Vec3>, 8> corners;

    // Near plane corners
    corners[0] = intersectPlanes(frustum.planes[0], frustum.planes[2], frustum.planes[5]);  // Near-Left-Bottom
    corners[1] = intersectPlanes(frustum.planes[0], frustum.planes[3], frustum.planes[5]);  // Near-Right-Bottom
    corners[2] = intersectPlanes(frustum.planes[0], frustum.planes[2], frustum.planes[4]);  // Near-Left-Top
    corners[3] = intersectPlanes(frustum.planes[0], frustum.planes[3], frustum.planes[4]);  // Near-Right-Top

    // Far plane corners
    corners[4] = intersectPlanes(frustum.planes[1], frustum.planes[2], frustum.planes[5]);  // Far-Left-Bottom
    corners[5] = intersectPlanes(frustum.planes[1], frustum.planes[3], frustum.planes[5]);  // Far-Right-Bottom
    corners[6] = intersectPlanes(frustum.planes[1], frustum.planes[2], frustum.planes[4]);  // Far-Left-Top
    corners[7] = intersectPlanes(frustum.planes[1], frustum.planes[3], frustum.planes[4]);  // Far-Right-Top

    // Verify all corners are valid
    for (const auto& corner : corners) {
        if (!corner.has_value()) {
            return;  // Invalid frustum, can't draw
        }
    }

    // Draw near plane quad
    debugDrawLine(*corners[0], *corners[1], color, duration, depthTest);  // Bottom edge
    debugDrawLine(*corners[1], *corners[3], color, duration, depthTest);  // Right edge
    debugDrawLine(*corners[3], *corners[2], color, duration, depthTest);  // Top edge
    debugDrawLine(*corners[2], *corners[0], color, duration, depthTest);  // Left edge

    // Draw far plane quad
    debugDrawLine(*corners[4], *corners[5], color, duration, depthTest);  // Bottom edge
    debugDrawLine(*corners[5], *corners[7], color, duration, depthTest);  // Right edge
    debugDrawLine(*corners[7], *corners[6], color, duration, depthTest);  // Top edge
    debugDrawLine(*corners[6], *corners[4], color, duration, depthTest);  // Left edge

    // Draw edges connecting near to far
    debugDrawLine(*corners[0], *corners[4], color, duration, depthTest);  // Left-Bottom
    debugDrawLine(*corners[1], *corners[5], color, duration, depthTest);  // Right-Bottom
    debugDrawLine(*corners[2], *corners[6], color, duration, depthTest);  // Left-Top
    debugDrawLine(*corners[3], *corners[7], color, duration, depthTest);  // Right-Top
}

void VulkanGraphics3DSystem::debugDrawRay(
    const Vec3& origin, const Vec3& direction, float length,
    const Color& color, float duration, bool depthTest) {
    if (!debugRenderingEnabled_) return;
    Vec3 end{origin.x + direction.x * length, origin.y + direction.y * length, origin.z + direction.z * length};
    debugDrawLine(origin, end, color, duration, depthTest);
}

void VulkanGraphics3DSystem::debugDrawAxes(
    const Transform3D& transform, float size, float duration, bool depthTest) {
    if (!debugRenderingEnabled_) return;
    debugDrawLine(transform.position,
                  Vec3{transform.position.x + size, transform.position.y, transform.position.z},
                  Color::red(), duration, depthTest);
    debugDrawLine(transform.position,
                  Vec3{transform.position.x, transform.position.y + size, transform.position.z},
                  Color::green(), duration, depthTest);
    debugDrawLine(transform.position,
                  Vec3{transform.position.x, transform.position.y, transform.position.z + size},
                  Color::blue(), duration, depthTest);
}

void VulkanGraphics3DSystem::debugDrawAABB(
    const AABB3D& aabb, const Color& color, float duration, bool depthTest) {
    Vec3 center{(aabb.min.x + aabb.max.x) / 2, (aabb.min.y + aabb.max.y) / 2, (aabb.min.z + aabb.max.z) / 2};
    Vec3 halfExtents{(aabb.max.x - aabb.min.x) / 2, (aabb.max.y - aabb.min.y) / 2, (aabb.max.z - aabb.min.z) / 2};
    debugDrawBox(center, halfExtents, Quat{1, 0, 0, 0}, color, duration, depthTest);
}

void VulkanGraphics3DSystem::debugClear() {
    debugLines_.clear();
}

void VulkanGraphics3DSystem::setDebugRenderingEnabled(bool enabled) {
    debugRenderingEnabled_ = enabled;
}

bool VulkanGraphics3DSystem::isDebugRenderingEnabled() const {
    return debugRenderingEnabled_;
}

Size VulkanGraphics3DSystem::getWindowSize() const {
    return context_.getWindowSize();
}

void VulkanGraphics3DSystem::setWindowSize(Size size) {
    context_.setWindowSize(size);
}

bool VulkanGraphics3DSystem::isFullscreen() const {
    return isFullscreen_;
}

void VulkanGraphics3DSystem::setFullscreen(bool fullscreen) {
    if (isFullscreen_ == fullscreen) return;

    GLFWwindow* window = context_.getWindow();
    if (!window) return;

    if (fullscreen) {
        // Save current windowed position and size for later restoration
        glfwGetWindowPos(window, &windowedPosX_, &windowedPosY_);
        glfwGetWindowSize(window, &windowedWidth_, &windowedHeight_);

        // Switch to fullscreen on primary monitor
        GLFWmonitor* monitor = glfwGetPrimaryMonitor();
        const GLFWvidmode* mode = glfwGetVideoMode(monitor);
        glfwSetWindowMonitor(window, monitor, 0, 0, mode->width, mode->height, mode->refreshRate);
    } else {
        // Restore windowed mode with saved position and size
        glfwSetWindowMonitor(window, nullptr, windowedPosX_, windowedPosY_,
                             windowedWidth_, windowedHeight_, 0);
    }

    isFullscreen_ = fullscreen;
}

bool VulkanGraphics3DSystem::shouldClose() const {
    GLFWwindow* window = context_.getWindow();
    return window ? glfwWindowShouldClose(window) : false;
}

void* VulkanGraphics3DSystem::getNativeWindowHandle() const {
    return context_.getWindow();
}

void VulkanGraphics3DSystem::setClearColor(const Color& color) {
    clearColor_ = color;
    // Forward to context (convert from 0-255 to 0.0-1.0)
    context_.setClearColor(
        static_cast<float>(color.r) / 255.0f,
        static_cast<float>(color.g) / 255.0f,
        static_cast<float>(color.b) / 255.0f,
        static_cast<float>(color.a) / 255.0f
    );
}

void VulkanGraphics3DSystem::setVSync(bool enabled) {
    // Would recreate swapchain
}

void VulkanGraphics3DSystem::setRenderScale(float scale) {
    renderScale_ = scale;
}

float VulkanGraphics3DSystem::getRenderScale() const {
    return renderScale_;
}

//==========================================================================
// Runtime Configuration
//==========================================================================

bool VulkanGraphics3DSystem::loadRuntimeConfig(const std::filesystem::path& configPath) {
    configPath_ = configPath;

    // Load config file through AssetSystem (or fall back to direct I/O if no AssetSystem)
    std::string luaContent;

    if (pIAssetSystem_) {
        // Use AssetSystem as the sole gateway to the file system
        AssetHandle configHandle = pIAssetSystem_->registerAsset(AssetType::Data, configPath);
        pIAssetSystem_->loadAsset(configHandle);

        if (!pIAssetSystem_->isLoaded(configHandle)) {
            std::fprintf(stderr, "[Vulkan] Config file not found or failed to load: %s\n", configPath.string().c_str());
            return false;
        }

        // Get the DataAsset and extract raw text (Lua content)
        const DataAsset* dataAsset = pIAssetSystem_->getAsset<DataAsset>(configHandle);
        if (!dataAsset) {
            std::fprintf(stderr, "[Vulkan] Failed to get config data: %s\n", configPath.string().c_str());
            return false;
        }

        luaContent = dataAsset->rawText;
    } else {
        std::fprintf(stderr, "[Vulkan] ERROR: AssetSystem is required for loading config\n");
        return false;
    }

    try {
        // Use ConfigSystem's unified Lua parsing instead of creating our own sol::state
        if (!pIConfigSystem_) {
            std::fprintf(stderr, "[Vulkan] ERROR: ConfigSystem is required for loading config\n");
            return false;
        }

        auto result = pIConfigSystem_->parseLuaString(luaContent, configPath.string());
        if (!result) {
            std::fprintf(stderr, "[Vulkan] Failed to load config: %s\n", configPath.string().c_str());
            return false;
        }

        sol::table config = result->as<sol::table>();
        Graphics3DRuntimeConfig newConfig;

        //======================================================================
        // Unified Rendering Settings
        //======================================================================

        // Gamma correction (unified setting)
        if (auto gamma = config["gammaCorrection"]; gamma.valid()) {
            newConfig.gammaCorrection = gamma.get<bool>();
        }

        // MSAA samples (unified setting)
        if (auto msaa = config["msaaSamples"]; msaa.valid()) {
            newConfig.msaaSamples = msaa.get<std::uint32_t>();
        }

        // V-Sync (unified setting)
        if (auto vsync = config["vsync"]; vsync.valid()) {
            std::string mode = vsync.get<std::string>();
            if (mode == "off") {
                newConfig.vsync = PresentMode::Immediate;
            } else if (mode == "adaptive") {
                newConfig.vsync = PresentMode::Mailbox;
            } else {
                newConfig.vsync = PresentMode::FIFO;  // "on" or default
            }
        }

        // Shader paths
        if (auto shaderPaths = config["shaderPaths"]; shaderPaths.valid()) {
            sol::table paths = shaderPaths;
            for (auto& kv : paths) {
                newConfig.shaderPaths.push_back(kv.second.as<std::string>());
            }
        }

        //======================================================================
        // Backend-Specific Settings (Vulkan-only features)
        //======================================================================

        if (auto vulkan = config["vulkan"]; vulkan.valid()) {
            sol::table vk = vulkan;

            if (auto val = vk["validationLayers"]; val.valid()) {
                newConfig.vulkanValidationLayers = val.get<bool>();
            }
        }

        //======================================================================
        // Lighting
        if (auto lighting = config["lighting"]; lighting.valid()) {
            sol::table lt = lighting;

            if (auto dir = lt["lightDirection"]; dir.valid()) {
                sol::table d = dir;
                newConfig.lightDirection = Vec3{
                    d[1].get_or(0.5f),
                    d[2].get_or(-1.0f),
                    d[3].get_or(0.3f)
                };
            }

            if (auto col = lt["lightColor"]; col.valid()) {
                sol::table c = col;
                newConfig.lightColor = Vec3{
                    c[1].get_or(1.0f),
                    c[2].get_or(0.98f),
                    c[3].get_or(0.95f)
                };
            }

            if (auto amb = lt["ambientColor"]; amb.valid()) {
                sol::table a = amb;
                newConfig.ambientColor = Vec3{
                    a[1].get_or(0.15f),
                    a[2].get_or(0.18f),
                    a[3].get_or(0.22f)
                };
            }

            if (auto intensity = lt["ambientIntensity"]; intensity.valid()) {
                newConfig.ambientIntensity = intensity.get<float>();
            }
        }

        // Clear color
        if (auto clearColor = config["clearColor"]; clearColor.valid()) {
            sol::table cc = clearColor;
            newConfig.clearColor = Color::fromFloat(
                cc[1].get_or(0.529f),
                cc[2].get_or(0.808f),
                cc[3].get_or(0.922f),
                cc[4].get_or(1.0f)
            );
        }

        // Debug
        if (auto debug = config["debug"]; debug.valid()) {
            sol::table d = debug;

            if (auto wireframe = d["wireframe"]; wireframe.valid()) {
                newConfig.debugWireframe = wireframe.get<bool>();
            }
            if (auto normals = d["showNormals"]; normals.valid()) {
                newConfig.debugShowNormals = normals.get<bool>();
            }
            if (auto fps = d["showFps"]; fps.valid()) {
                newConfig.debugShowFps = fps.get<bool>();
            }
            if (auto hr = d["hotReload"]; hr.valid()) {
                newConfig.hotReload = hr.get<bool>();
            }
        }

        applyRuntimeConfig(newConfig);
        std::printf("[Vulkan] Loaded graphics config: %s\n", configPath.string().c_str());
        return true;

    } catch (const std::exception& e) {
        std::fprintf(stderr, "[Vulkan] Exception loading config: %s\n", e.what());
        return false;
    }
}

void VulkanGraphics3DSystem::applyRuntimeConfig(const Graphics3DRuntimeConfig& config) {
    runtimeConfig_ = config;

    // Apply clear color
    setClearColor(config.clearColor);

    // Apply lighting defaults
    if (!hasDirectionalLight_) {
        DirectionalLight light;
        light.direction = config.lightDirection;
        light.color = config.lightColor;
        setDirectionalLight(light);
    }

    setAmbientLight(config.ambientColor, config.ambientIntensity);

    // Note: Swapchain format changes require full swapchain recreation
    // which is more involved - for now we apply what we can at runtime
}

const Graphics3DRuntimeConfig& VulkanGraphics3DSystem::getRuntimeConfig() const {
    return runtimeConfig_;
}

bool VulkanGraphics3DSystem::reloadRuntimeConfig() {
    if (configPath_.empty()) {
        return false;
    }
    return loadRuntimeConfig(configPath_);
}

void VulkanGraphics3DSystem::drawMeshWithShaderMaterial(
    MeshHandle mesh, ShaderProgramHandle shader, const Mat4& worldMatrix,
    bool castShadow, bool receiveShadow) {
    // Would bind shader and draw
}

VulkanPipelineHandle VulkanGraphics3DSystem::loadShaderPipeline(
    std::string_view vertPath, std::string_view fragPath) {
    if (!pIAssetSystem_) {
        std::fprintf(stderr, "[Vulkan] ERROR: AssetSystem is required for loading shaders\n");
        return 0;
    }

    // Load shaders through AssetSystem with SPIR-V compilation
    auto vertHandle = pIAssetSystem_->loadShaderCompiled(std::string(vertPath));
    auto fragHandle = pIAssetSystem_->loadShaderCompiled(std::string(fragPath));

    if (!pIAssetSystem_->isLoaded(vertHandle) || !pIAssetSystem_->isLoaded(fragHandle)) {
        std::fprintf(stderr, "[Vulkan] Failed to load shaders: %.*s, %.*s\n",
            static_cast<int>(vertPath.size()), vertPath.data(),
            static_cast<int>(fragPath.size()), fragPath.data());
        return 0;
    }

    const ShaderData* vertShader = pIAssetSystem_->getShaderData(vertHandle);
    const ShaderData* fragShader = pIAssetSystem_->getShaderData(fragHandle);

    if (!vertShader || !fragShader) {
        std::fprintf(stderr, "[Vulkan] Failed to get shader data: %.*s, %.*s\n",
            static_cast<int>(vertPath.size()), vertPath.data(),
            static_cast<int>(fragPath.size()), fragPath.data());
        return 0;
    }

    auto vertSpirv = vertShader->spirvBytecode;
    auto fragSpirv = fragShader->spirvBytecode;

    if (vertSpirv.empty() || fragSpirv.empty()) {
        return 0;
    }

    VulkanPipelineDef def;
    def.shaderStages = {
        {VK_SHADER_STAGE_VERTEX_BIT, vertSpirv, "main"},
        {VK_SHADER_STAGE_FRAGMENT_BIT, fragSpirv, "main"}
    };

    // 3D vertex layout: position (vec3) + normal (vec3) + texcoord (vec2)
    def.vertexBindings = {
        {0, sizeof(Vertex3D), VK_VERTEX_INPUT_RATE_VERTEX}
    };
    def.vertexAttributes = {
        {0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex3D, position)},
        {1, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex3D, normal)},
        {2, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(Vertex3D, texCoord)}
    };

    // Push constants: model + viewProjection + baseColor + lightDir + lightColor + ambientColor + cameraPos = 208 bytes
    def.pushConstantRanges = {
        {VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, 208}
    };

    def.depthTestEnable = true;
    def.depthWriteEnable = true;
    def.cullMode = VK_CULL_MODE_BACK_BIT;
    def.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;

    auto result = context_.createPipeline(def);
    VulkanPipelineHandle pipeline = result ? *result : 0;

    // Track GLSL source files for hot reload (not SPV files)
    if (pipeline != 0 && hotReloadEnabled_) {
        ShaderFileInfo info;
        // Convert SPV paths to GLSL paths for hot reload tracking
        std::string vertGlsl = std::string(vertPath);
        std::string fragGlsl = std::string(fragPath);
        if (vertGlsl.ends_with(".spv")) {
            vertGlsl = vertGlsl.substr(0, vertGlsl.size() - 4);
        }
        if (fragGlsl.ends_with(".spv")) {
            fragGlsl = fragGlsl.substr(0, fragGlsl.size() - 4);
        }

        // Register shaders with asset system for subscription-based hot reload
        // AssetSystem will handle file existence checks internally
        if (pIAssetSystem_ && useAssetSystemHotReload_) {
            info.vertGlslPath = vertGlsl;
            info.fragGlslPath = fragGlsl;

            // Register vertex shader - AssetSystem handles existence checking
            info.vertShaderAsset = pIAssetSystem_->loadShaderCompiled(vertGlsl);
            if (info.vertShaderAsset.uuid != 0) {
                shaderAssetToPipelines_[info.vertShaderAsset.uuid].push_back(pipeline);
            }

            // Register fragment shader
            info.fragShaderAsset = pIAssetSystem_->loadShaderCompiled(fragGlsl);
            if (info.fragShaderAsset.uuid != 0) {
                shaderAssetToPipelines_[info.fragShaderAsset.uuid].push_back(pipeline);
            }

            // Only track if at least one shader was successfully registered
            if (info.vertShaderAsset.uuid != 0 || info.fragShaderAsset.uuid != 0) {
                pipelineShaderFiles_[pipeline] = std::move(info);
                std::fprintf(stderr, "[Vulkan] Registered shaders with asset system: vert=%llu, frag=%llu\n",
                            static_cast<unsigned long long>(info.vertShaderAsset.uuid),
                            static_cast<unsigned long long>(info.fragShaderAsset.uuid));
                std::fprintf(stderr, "[Vulkan] Tracking GLSL for hot reload: %s, %s\n",
                    vertGlsl.c_str(), fragGlsl.c_str());
            }
        }
    }

    return pipeline;
}

void VulkanGraphics3DSystem::checkShaderHotReload() {
    if (!hotReloadEnabled_) return;

    // Event-driven hot reload: AssetSystem uses efsw file watcher to detect changes.
    // The asset system will call our onShaderAssetChanged callback when files change.
    // We just need to pump the asset system's update to process any queued events.
    if (pIAssetSystem_) {
        // This processes queued efsw file change events (no polling!)
        pIAssetSystem_->update();
    }
    // Note: No polling fallback - event-driven only via asset system subscriptions
}

void VulkanGraphics3DSystem::onShaderAssetChanged(AssetHandle handle, AssetType type) {
    if (type != AssetType::Shader) return;

    std::fprintf(stderr, "[Vulkan] Asset system notified shader change: UUID %llu\n",
                static_cast<unsigned long long>(handle.uuid));

    // Find all pipelines that use this shader asset
    auto it = shaderAssetToPipelines_.find(handle.uuid);
    if (it == shaderAssetToPipelines_.end()) {
        // Not a shader we're tracking
        return;
    }

    // Wait for GPU to be idle before modifying pipelines
    vkDeviceWaitIdle(context_.getDevice());

    for (VulkanPipelineHandle pipeline : it->second) {
        auto infoIt = pipelineShaderFiles_.find(pipeline);
        if (infoIt != pipelineShaderFiles_.end()) {
            reloadPipelineShaders(pipeline, infoIt->second);
        }
    }
}

void VulkanGraphics3DSystem::reloadPipelineShaders(VulkanPipelineHandle oldPipeline, const ShaderFileInfo& info) {
    std::fprintf(stderr, "[Vulkan] Reloading pipeline from asset system: %s + %s\n",
                info.vertGlslPath.c_str(), info.fragGlslPath.c_str());

    // Get SPIR-V from AssetSystem (already recompiled on hot reload)
    const ShaderData* vertShader = nullptr;
    const ShaderData* fragShader = nullptr;

    if (info.vertShaderAsset.uuid != 0 && pIAssetSystem_) {
        pIAssetSystem_->reloadAsset(info.vertShaderAsset);  // Force reload to recompile
        vertShader = pIAssetSystem_->getShaderData(info.vertShaderAsset);
    }
    if (info.fragShaderAsset.uuid != 0 && pIAssetSystem_) {
        pIAssetSystem_->reloadAsset(info.fragShaderAsset);  // Force reload to recompile
        fragShader = pIAssetSystem_->getShaderData(info.fragShaderAsset);
    }

    if (!vertShader || !fragShader || !vertShader->compiled || !fragShader->compiled) {
        std::fprintf(stderr, "[Vulkan] Shader reload failed: compilation error\n");
        if (vertShader && !vertShader->compileError.empty()) {
            std::fprintf(stderr, "  Vertex: %s\n", vertShader->compileError.c_str());
        }
        if (fragShader && !fragShader->compileError.empty()) {
            std::fprintf(stderr, "  Fragment: %s\n", fragShader->compileError.c_str());
        }
        return;
    }

    const auto& vertSpirv = vertShader->spirvBytecode;
    const auto& fragSpirv = fragShader->spirvBytecode;

    VulkanPipelineDef def;
    def.shaderStages = {
        {VK_SHADER_STAGE_VERTEX_BIT, vertSpirv, "main"},
        {VK_SHADER_STAGE_FRAGMENT_BIT, fragSpirv, "main"}
    };
    def.vertexBindings = {{0, sizeof(Vertex3D), VK_VERTEX_INPUT_RATE_VERTEX}};
    def.vertexAttributes = {
        {0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex3D, position)},
        {1, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex3D, normal)},
        {2, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(Vertex3D, texCoord)}
    };
    def.pushConstantRanges = {
        {VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, 208}
    };
    def.depthTestEnable = true;
    def.depthWriteEnable = true;
    def.cullMode = VK_CULL_MODE_BACK_BIT;
    def.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;

    auto result = context_.createPipeline(def);
    if (!result) {
        std::fprintf(stderr, "[Vulkan] Shader reload failed: could not create new pipeline\n");
        return;
    }

    VulkanPipelineHandle newPipeline = *result;

    // Update material pipeline cache to use new pipeline
    for (auto& [name, cachedPipeline] : materialPipelineCache_) {
        if (cachedPipeline == oldPipeline) {
            cachedPipeline = newPipeline;
        }
    }

    // Update shader file tracking
    // NOTE: last_write_time is only used for legacy hot reload (when AssetSystem not available)
    // When using AssetSystem hot reload, these timestamps are not used
    ShaderFileInfo newInfo = info;
    std::error_code ec;
    newInfo.vertLastModified = std::filesystem::last_write_time(info.vertGlslPath, ec);
    newInfo.fragLastModified = std::filesystem::last_write_time(info.fragGlslPath, ec);
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

    // Destroy old pipeline
    context_.destroyPipeline(oldPipeline);

    std::fprintf(stderr, "[Vulkan] Shader reload successful (compiled via AssetSystem shaderc)\n");
}

VulkanPipelineHandle VulkanGraphics3DSystem::getOrCreateMaterialPipeline(std::string_view materialPath) {
    std::string key{materialPath};

    // Check cache first
    auto it = materialPipelineCache_.find(key);
    if (it != materialPipelineCache_.end()) {
        return it->second;
    }

    // Resolve material path (handles :assets:/ prefix)
    auto resolvedMaterialPath = bestow::PathResolver::resolve(":assets:/materials/" + key);

    // Default shader paths (fallback)
    std::string vertPath = "shaders/basic3d.vert";
    std::string fragPath = "shaders/basic3d.frag";

    // Load material Lua file through AssetSystem (or fall back to direct I/O)
    std::string luaContent;
    bool materialLoaded = false;

    if (pIAssetSystem_) {
        // Use AssetSystem as the sole gateway to the file system
        AssetHandle materialHandle = pIAssetSystem_->registerAsset(AssetType::Material, resolvedMaterialPath);
        pIAssetSystem_->loadAsset(materialHandle);

        if (pIAssetSystem_->isLoaded(materialHandle)) {
            const DataAsset* dataAsset = pIAssetSystem_->getAsset<DataAsset>(materialHandle);
            if (dataAsset) {
                luaContent = dataAsset->rawText;
                materialLoaded = true;
            }
        }
    } else {
        std::fprintf(stderr, "[Vulkan] ERROR: AssetSystem is required for loading materials\n");
        return 0;
    }

    // Parse the Lua material file to get shader paths
    if (materialLoaded) {
        try {
            // Use ConfigSystem's unified Lua parsing instead of creating our own sol::state
            if (!pIConfigSystem_) {
                std::fprintf(stderr, "[Vulkan] ERROR: ConfigSystem is required for parsing materials\n");
            } else {
                auto result = pIConfigSystem_->parseLuaString(luaContent, resolvedMaterialPath.string());
                if (result) {
                    sol::table mat = result->as<sol::table>();

                    // Extract shader paths from material definition
                    if (mat["shader"].valid()) {
                        sol::table shader = mat["shader"];
                        sol::optional<std::string> vertOpt = shader["vertex"];
                        sol::optional<std::string> fragOpt = shader["fragment"];

                        if (vertOpt) vertPath = *vertOpt;
                        if (fragOpt) fragPath = *fragOpt;
                    }
                } else {
                    std::fprintf(stderr, "[Vulkan] Failed to parse material '%s'\n", key.c_str());
                }
            }
        } catch (const std::exception& e) {
            std::fprintf(stderr, "[Vulkan] Exception parsing material '%s': %s\n",
                         key.c_str(), e.what());
        }
    } else {
        std::fprintf(stderr, "[Vulkan] Material file not found: %s\n",
                     resolvedMaterialPath.string().c_str());
    }

    // Resolve shader paths (handles :library:/ and :assets:/ prefixes)
    auto resolvedVertPath = bestow::PathResolver::resolve(vertPath);
    auto resolvedFragPath = bestow::PathResolver::resolve(fragPath);

    // If paths don't have a scheme, try searching in configured shader paths
    // Shader paths are now resolved by AssetSystem during loading
    // No need for manual filesystem checks here

    VulkanPipelineHandle pipeline = loadShaderPipeline(resolvedVertPath.string(), resolvedFragPath.string());

    // Cache even if failed (as 0) to avoid repeated attempts
    materialPipelineCache_[key] = pipeline;

    if (pipeline != 0) {
        std::fprintf(stderr, "[Vulkan] Loaded material pipeline for '%s': %s + %s\n",
                     key.c_str(), vertPath.c_str(), fragPath.c_str());
    }

    return pipeline;
}

Result<void, Graphics3DError> VulkanGraphics3DSystem::drawMeshWithLuaMaterial(
    MeshHandle mesh, std::string_view materialPath, const Mat4& worldMatrix) {
    return drawMeshWithLuaMaterial(mesh, materialPath, worldMatrix, Vec4{1.0f, 1.0f, 1.0f, 1.0f});
}

Result<void, Graphics3DError> VulkanGraphics3DSystem::drawMeshWithLuaMaterial(
    MeshHandle mesh, std::string_view materialPath, const Mat4& worldMatrix,
    const Vec4& colorOverride) {
    // Get or create pipeline for this material
    VulkanPipelineHandle pipeline = getOrCreateMaterialPipeline(materialPath);

    if (pipeline == 0) {
        // Fall back to default PBR pipeline
        return std::unexpected(Graphics3DError::InvalidShader);
    }

    // For now, use the PBR material system with the custom pipeline
    // We'll queue a special render item that uses the material pipeline
    auto matIt = materials_.find(defaultPBRMaterial_);
    MaterialHandle material = defaultPBRMaterial_;

    // Queue render item - we need to track the pipeline to use
    // For now, we store the pipeline handle in a separate map keyed by mesh+material
    // Actually, we'll just render directly using the pipeline

    // Queue the item (will be rendered with this pipeline by modifying flushRenderQueue)
    queueRenderItem(RenderItem{
        .mesh = mesh,
        .material = material,
        .worldMatrix = worldMatrix,
        .layer = 0,
        .castShadow = true,
        .receiveShadow = true,
        .colorOverride = colorOverride,
        .customPipeline = static_cast<std::uint32_t>(pipeline)
    });

    return {};
}

Result<MeshHandle, Graphics3DError> VulkanGraphics3DSystem::createMeshFromData(const MeshData& data) {
    // Convert Vertex3DData to Vertex3D (including bone data for skinning)
    std::vector<Vertex3D> vertices;
    vertices.reserve(data.vertices.size());
    for (const auto& v : data.vertices) {
        Vertex3D vert;
        vert.position = Vec3{v.position[0], v.position[1], v.position[2]};
        vert.normal = Vec3{v.normal[0], v.normal[1], v.normal[2]};
        vert.texCoord = Vec2{v.texCoord[0], v.texCoord[1]};
        vert.color = Vec4{v.color[0], v.color[1], v.color[2], v.color[3]};
        // Copy bone data for skeletal animation
        for (int i = 0; i < 4; ++i) {
            vert.boneIndices[i] = v.boneIndices[i];
            vert.boneWeights[i] = v.boneWeights[i];
        }
        vertices.push_back(vert);
    }

    MeshDef def;
    def.vertices = vertices;
    def.indices = data.indices;
    def.bounds = AABB3D{
        Vec3{data.boundsMin[0], data.boundsMin[1], data.boundsMin[2]},
        Vec3{data.boundsMax[0], data.boundsMax[1], data.boundsMax[2]}
    };
    return createMesh(def);
}

Result<MaterialHandle, Graphics3DError> VulkanGraphics3DSystem::createMaterialFromData(const MaterialData& data) {
    PBRMaterial mat;

    // Copy PBR factors
    mat.baseColorFactor = Vec4{data.baseColorFactor[0], data.baseColorFactor[1],
                               data.baseColorFactor[2], data.baseColorFactor[3]};
    mat.metallicFactor = data.metallicFactor;
    mat.roughnessFactor = data.roughnessFactor;
    mat.normalScale = data.normalScale;
    mat.occlusionStrength = data.occlusionStrength;
    mat.emissiveFactor = Vec3{data.emissiveFactor[0], data.emissiveFactor[1], data.emissiveFactor[2]};
    mat.alphaCutoff = data.alphaCutoff;

    // Copy texture handles from MaterialData
    // These can be loaded via AssetSystem and passed in the MaterialTextureRef
    mat.baseColorTexture = data.baseColorTexture.handle;
    mat.metallicRoughnessTexture = data.metallicRoughnessTexture.handle;
    mat.normalTexture = data.normalTexture.handle;
    mat.occlusionTexture = data.occlusionTexture.handle;
    mat.emissiveTexture = data.emissiveTexture.handle;

    // Copy render state
    mat.doubleSided = data.doubleSided;
    if (data.transparent) {
        mat.blendMode = BlendMode::AlphaBlend;
    } else if (mat.alphaCutoff > 0.0f && mat.alphaCutoff < 1.0f) {
        mat.blendMode = BlendMode::AlphaTest;  // Use alpha test in shader
    }

    // Handle unlit materials differently
    if (data.unlit) {
        UnlitMaterial unlitMat;
        unlitMat.color = mat.baseColorFactor;
        unlitMat.texture = mat.baseColorTexture;
        unlitMat.blendMode = mat.blendMode;
        unlitMat.cullMode = data.doubleSided ? CullMode::None : CullMode::Back;
        return createUnlitMaterial(unlitMat);
    }

    auto matResult = createMaterial(mat);
    if (!matResult) return matResult;
    MaterialHandle matHandle = *matResult;

    // Handle embedded textures - create GPU texture and descriptor set
    if (!data.baseColorTexture.embeddedData.empty()) {
        const auto& texRef = data.baseColorTexture;

        // Embedded texture data needs to be decoded if it's compressed (width = -1)
        std::vector<unsigned char> decodedData;
        int width = texRef.embeddedWidth;
        int height = texRef.embeddedHeight;
        int channels = texRef.embeddedChannels;

        if (width < 0) {
            // Compressed format (PNG, JPG) - decode using stb_image
            int decWidth, decHeight, decChannels;
            unsigned char* decoded = stbi_load_from_memory(
                texRef.embeddedData.data(),
                static_cast<int>(texRef.embeddedData.size()),
                &decWidth, &decHeight, &decChannels, 4  // Force RGBA
            );

            if (decoded) {
                width = decWidth;
                height = decHeight;
                channels = 4;
                decodedData.assign(decoded, decoded + (width * height * 4));
                stbi_image_free(decoded);
                spdlog::info("[Vulkan] Decoded compressed embedded texture: {}x{} (source had {} channels)",
                             width, height, decChannels);
            } else {
                spdlog::warn("[Vulkan] Failed to decode compressed embedded texture: {}", stbi_failure_reason());
                return matResult;
            }
        }

        // Use decoded data if available (from compressed texture), otherwise use raw embedded data
        const unsigned char* pixelData = decodedData.empty() ? texRef.embeddedData.data() : decodedData.data();
        std::size_t pixelSize = decodedData.empty() ? texRef.embeddedData.size() : decodedData.size();

        if (width > 0 && height > 0 && pixelData != nullptr && pixelSize > 0) {
            // Create GPU texture
            VulkanImageDef imageDef;
            imageDef.type = VK_IMAGE_TYPE_2D;
            imageDef.format = VK_FORMAT_R8G8B8A8_UNORM;
            imageDef.width = static_cast<std::uint32_t>(width);
            imageDef.height = static_cast<std::uint32_t>(height);
            imageDef.depth = 1;
            imageDef.mipLevels = 1;
            imageDef.arrayLayers = 1;
            imageDef.usage = VulkanImageUsage::Sampled | VulkanImageUsage::TransferDst;
            imageDef.isCubemap = false;

            auto imageResult = context_.createImage(imageDef);
            if (imageResult) {
                VulkanImageHandle texImage = *imageResult;

                // Upload texture data
                context_.uploadToImage(texImage, pixelData, pixelSize);

                materialTextures_[matHandle] = texImage;

                // Create descriptor set for this material's texture
                if (textureDescriptorSetLayout_ != VK_NULL_HANDLE && textureDescriptorPool_ != VK_NULL_HANDLE) {
                    VkDescriptorSetAllocateInfo allocInfo{};
                    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
                    allocInfo.descriptorPool = textureDescriptorPool_;
                    allocInfo.descriptorSetCount = 1;
                    allocInfo.pSetLayouts = &textureDescriptorSetLayout_;

                    VkDescriptorSet descSet = VK_NULL_HANDLE;
                    if (vkAllocateDescriptorSets(context_.getDevice(), &allocInfo, &descSet) == VK_SUCCESS) {
                        VkDescriptorImageInfo imageInfo{};
                        imageInfo.sampler = context_.getImageSampler(texImage);
                        imageInfo.imageView = context_.getImageView(texImage);
                        imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

                        VkWriteDescriptorSet texWrite{};
                        texWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                        texWrite.dstSet = descSet;
                        texWrite.dstBinding = 0;
                        texWrite.dstArrayElement = 0;
                        texWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
                        texWrite.descriptorCount = 1;
                        texWrite.pImageInfo = &imageInfo;

                        vkUpdateDescriptorSets(context_.getDevice(), 1, &texWrite, 0, nullptr);

                        materialTextureDescriptorSets_[matHandle] = descSet;
                        spdlog::info("[Vulkan] Created texture descriptor for material ({}x{})", width, height);
                    }
                }
            }
        }
    }

    return matResult;
}

Result<std::vector<MaterialHandle>, Graphics3DError> VulkanGraphics3DSystem::createMaterialsFromModel(const ModelData& data) {
    std::vector<MaterialHandle> handles;
    for (const auto& matData : data.materials) {
        auto result = createMaterialFromData(matData);
        if (result) {
            handles.push_back(*result);
        }
    }
    return handles;
}

Result<void, Graphics3DError> VulkanGraphics3DSystem::createSkyboxFromData(const CubemapData& data) {
    // Validate cubemap data
    if (data.facePixels.size() != 6) {
        return std::unexpected(Graphics3DError::InvalidTexture);
    }

    if (data.faceWidth <= 0 || data.faceHeight <= 0 || data.channels <= 0) {
        return std::unexpected(Graphics3DError::InvalidTexture);
    }

    // Verify all faces have the expected size
    std::size_t expectedSize = static_cast<std::size_t>(data.faceWidth * data.faceHeight * data.channels);
    for (const auto& facePixels : data.facePixels) {
        if (facePixels.size() != expectedSize) {
            return std::unexpected(Graphics3DError::InvalidTexture);
        }
    }

    // Destroy existing skybox cubemap if present
    if (skyboxCubemapImage_ != 0) {
        context_.destroyImage(skyboxCubemapImage_);
        skyboxCubemapImage_ = 0;
    }

    // Determine Vulkan format based on channels
    VkFormat format;
    switch (data.channels) {
        case 1:
            format = VK_FORMAT_R8_UNORM;
            break;
        case 2:
            format = VK_FORMAT_R8G8_UNORM;
            break;
        case 3:
            // Vulkan doesn't support RGB8, convert to RGBA or use BGR
            // Most cubemap loaders provide RGBA, but fallback to RGBA8
            format = VK_FORMAT_R8G8B8A8_UNORM;
            break;
        case 4:
            format = VK_FORMAT_R8G8B8A8_UNORM;
            break;
        default:
            return std::unexpected(Graphics3DError::InvalidTexture);
    }

    // For RGB data, we need to convert to RGBA
    std::vector<std::vector<unsigned char>> rgbaFaces;
    const std::vector<std::vector<unsigned char>>* facesToUpload = &data.facePixels;

    if (data.channels == 3) {
        rgbaFaces.resize(6);
        for (std::size_t face = 0; face < 6; ++face) {
            const auto& srcFace = data.facePixels[face];
            auto& dstFace = rgbaFaces[face];
            std::size_t pixelCount = static_cast<std::size_t>(data.faceWidth * data.faceHeight);
            dstFace.resize(pixelCount * 4);

            for (std::size_t i = 0; i < pixelCount; ++i) {
                dstFace[i * 4 + 0] = srcFace[i * 3 + 0];  // R
                dstFace[i * 4 + 1] = srcFace[i * 3 + 1];  // G
                dstFace[i * 4 + 2] = srcFace[i * 3 + 2];  // B
                dstFace[i * 4 + 3] = 255;                  // A
            }
        }
        facesToUpload = &rgbaFaces;
        expectedSize = static_cast<std::size_t>(data.faceWidth * data.faceHeight * 4);
    }

    // Create cubemap image
    vulkan::VulkanImageDef imageDef{};
    imageDef.width = static_cast<std::uint32_t>(data.faceWidth);
    imageDef.height = static_cast<std::uint32_t>(data.faceHeight);
    imageDef.depth = 1;
    imageDef.mipLevels = 1;
    imageDef.arrayLayers = 6;  // Will be set automatically due to isCubemap
    imageDef.format = format;
    imageDef.type = VK_IMAGE_TYPE_2D;
    imageDef.usage = vulkan::VulkanImageUsage::Sampled | vulkan::VulkanImageUsage::TransferDst;
    imageDef.isCubemap = true;
    imageDef.debugName = "skybox_cubemap";

    auto imageResult = context_.createImage(imageDef);
    if (!imageResult) {
        return std::unexpected(Graphics3DError::InternalError);
    }

    skyboxCubemapImage_ = *imageResult;

    // Upload each face to the cubemap
    // Face order: +X (0), -X (1), +Y (2), -Y (3), +Z (4), -Z (5)
    for (std::uint32_t face = 0; face < 6; ++face) {
        const auto& faceData = (*facesToUpload)[face];
        auto uploadResult = context_.uploadToImageLayer(
            skyboxCubemapImage_,
            faceData.data(),
            faceData.size(),
            face
        );

        if (!uploadResult) {
            // Clean up on failure
            context_.destroyImage(skyboxCubemapImage_);
            skyboxCubemapImage_ = 0;
            return std::unexpected(Graphics3DError::InternalError);
        }
    }

    // Store cubemap data for potential re-creation
    skyboxCubemapData_ = data;
    hasSkybox_ = true;

    // Set default skybox parameters
    skybox_.rotation = 0.0f;
    skybox_.exposure = 1.0f;

    return {};
}

void VulkanGraphics3DSystem::setFrustumCulling(bool enabled) {
    frustumCullingEnabled_ = enabled;
}

bool VulkanGraphics3DSystem::isFrustumCullingEnabled() const {
    return frustumCullingEnabled_;
}

void VulkanGraphics3DSystem::setToneMapping(bool enabled) {
    toneMappingEnabled_ = enabled;
}

void VulkanGraphics3DSystem::setExposure(float exposure) {
    exposure_ = exposure;
}

void VulkanGraphics3DSystem::setBloom(bool enabled, float threshold, float intensity) {
    bloomEnabled_ = enabled;
    bloomThreshold_ = threshold;
    bloomIntensity_ = intensity;
}

void VulkanGraphics3DSystem::setSSAO(bool enabled, float radius, float intensity) {
    ssaoEnabled_ = enabled;
    ssaoRadius_ = radius;
    ssaoIntensity_ = intensity;
}

RenderStats VulkanGraphics3DSystem::getStats() const {
    auto vulkanStats = context_.getStats();
    return RenderStats{
        .drawCalls = static_cast<std::uint32_t>(renderQueue_.size()),
        .triangles = 0,  // Would track
        .vertices = 0,
        .meshes = static_cast<std::uint32_t>(meshes_.size()),
        .materials = static_cast<std::uint32_t>(materials_.size()),
        .textures = 0,
        .lights = static_cast<std::uint32_t>(pointLights_.size() + spotLights_.size()),
        .visibleObjects = static_cast<std::uint32_t>(renderQueue_.size()),
        .culledObjects = 0,
        .frameTimeMs = 16.67f,
        .gpuTimeMs = static_cast<float>(vulkanStats.gpuFrameTimeMs)
    };
}

// Remaining stub implementations for less common features
Result<InstanceBufferHandle, Graphics3DError> VulkanGraphics3DSystem::createInstanceBuffer(
    std::uint32_t maxInstances, bool dynamic) {
    return nextInstanceBufferHandle_++;
}

Result<void, Graphics3DError> VulkanGraphics3DSystem::updateInstanceBuffer(
    InstanceBufferHandle buffer, std::span<const InstanceData> data, std::uint32_t offset) {
    return {};
}

void VulkanGraphics3DSystem::destroyInstanceBuffer(InstanceBufferHandle buffer) {
    instanceBuffers_.erase(buffer);
}

void VulkanGraphics3DSystem::drawInstanced(const InstancedRenderItem& item) {}
void VulkanGraphics3DSystem::queueInstancedRenderItem(const InstancedRenderItem& item) {}

Result<SkeletonHandle, Graphics3DError> VulkanGraphics3DSystem::createSkeleton(const ModelData& modelData) {
    return nextSkeletonHandle_++;
}

Result<AnimationClipHandle, Graphics3DError> VulkanGraphics3DSystem::createAnimationClip(
    SkeletonHandle skeleton, const std::string& clipName, const ModelData& modelData) {
    AnimationClipHandle handle = nextAnimationClipHandle_++;
    animationClips_[handle] = AnimationClip{clipName, 1.0f, true, 30.0f};
    return handle;
}

void VulkanGraphics3DSystem::destroySkeleton(SkeletonHandle skeleton) {
    skeletons_.erase(skeleton);
}

void VulkanGraphics3DSystem::destroyAnimationClip(AnimationClipHandle clip) {
    animationClips_.erase(clip);
}

std::vector<std::string> VulkanGraphics3DSystem::getAnimationClipNames(SkeletonHandle skeleton) const {
    return {};
}

AnimationClip VulkanGraphics3DSystem::getAnimationClipInfo(AnimationClipHandle clip) const {
    auto it = animationClips_.find(clip);
    if (it != animationClips_.end()) return it->second;
    return AnimationClip{};
}

std::vector<Mat4> VulkanGraphics3DSystem::sampleAnimation(AnimationClipHandle clip, float time, bool loop) {
    return std::vector<Mat4>(64);
}

std::vector<Mat4> VulkanGraphics3DSystem::blendAnimations(const BlendedAnimation& blend) {
    return std::vector<Mat4>(64);
}

void VulkanGraphics3DSystem::drawSkinnedMesh(
    MeshHandle mesh, MaterialHandle material, const Mat4& worldMatrix,
    std::span<const Mat4> boneTransforms) {
    if (skinnedPipeline_ == 0) {
        // Fallback to regular mesh drawing if skinned pipeline not available
        spdlog::warn("[Vulkan] drawSkinnedMesh: skinned pipeline not available, using fallback");
        drawMesh(mesh, material, worldMatrix, true, true);
        return;
    }

    static int debugCount = 0;
    if (++debugCount % 120 == 1) {
        spdlog::info("[Vulkan] drawSkinnedMesh: {} bone transforms, first bone pos: {}, {}, {}",
            boneTransforms.size(),
            boneTransforms.empty() ? 0.0f : boneTransforms[0][3][0],
            boneTransforms.empty() ? 0.0f : boneTransforms[0][3][1],
            boneTransforms.empty() ? 0.0f : boneTransforms[0][3][2]);
    }

    // Queue skinned render item
    SkinnedRenderItem item;
    item.mesh = mesh;
    item.material = material;
    item.worldMatrix = worldMatrix;
    item.boneTransforms.assign(boneTransforms.begin(), boneTransforms.end());
    skinnedRenderQueue_.push_back(std::move(item));
}

Result<Font3DHandle, Graphics3DError> VulkanGraphics3DSystem::loadFont3D(AssetHandle fontAsset) {
    return nextFont3DHandle_++;
}

void VulkanGraphics3DSystem::destroyFont3D(Font3DHandle font) {
    fonts3D_.erase(font);
}

void VulkanGraphics3DSystem::drawText3D(const Text3DItem& item) {}
void VulkanGraphics3DSystem::drawText3D(const std::string& text, const Vec3& position,
                                         Font3DHandle font, float fontSize, const Color& color) {}

AABB3D VulkanGraphics3DSystem::measureText3D(const std::string& text, Font3DHandle font, float fontSize) {
    return AABB3D{};
}

Result<void, Graphics3DError> VulkanGraphics3DSystem::setMaterialBaseColor(MaterialHandle handle, const Vec4& color) {
    auto it = materials_.find(handle);
    if (it == materials_.end()) return std::unexpected(Graphics3DError::InvalidMaterial);
    it->second.pbrData.baseColorFactor = color;
    return {};
}

Result<void, Graphics3DError> VulkanGraphics3DSystem::setMaterialMetallicRoughness(
    MaterialHandle handle, float metallic, float roughness) {
    auto it = materials_.find(handle);
    if (it == materials_.end()) return std::unexpected(Graphics3DError::InvalidMaterial);
    it->second.pbrData.metallicFactor = metallic;
    it->second.pbrData.roughnessFactor = roughness;
    return {};
}

Result<void, Graphics3DError> VulkanGraphics3DSystem::setMaterialEmissive(MaterialHandle handle, const Vec3& emissive) {
    auto it = materials_.find(handle);
    if (it == materials_.end()) return std::unexpected(Graphics3DError::InvalidMaterial);
    it->second.pbrData.emissiveFactor = emissive;
    return {};
}

std::optional<PBRMaterial> VulkanGraphics3DSystem::getMaterialProperties(MaterialHandle handle) const {
    auto it = materials_.find(handle);
    if (it != materials_.end()) return it->second.pbrData;
    return std::nullopt;
}

void VulkanGraphics3DSystem::setLODDistances(std::span<const float> distances) {
    lodDistances_.assign(distances.begin(), distances.end());
}

void VulkanGraphics3DSystem::registerLODMeshes(MeshHandle primaryMesh, std::span<const MeshHandle> lodMeshes) {
    lodMeshRegistry_[primaryMesh].assign(lodMeshes.begin(), lodMeshes.end());
}

void VulkanGraphics3DSystem::setLODBias(float bias) {
    lodBias_ = bias;
}

void VulkanGraphics3DSystem::createDefaultMaterials() {
    auto pbrResult = createMaterial(PBRMaterial{});
    if (pbrResult) defaultPBRMaterial_ = *pbrResult;

    auto unlitResult = createUnlitMaterial(UnlitMaterial{});
    if (unlitResult) defaultUnlitMaterial_ = *unlitResult;

    // Error material - magenta checkerboard
    PBRMaterial errorMat;
    errorMat.baseColorFactor = Vec4{1.0f, 0.0f, 1.0f, 1.0f};
    auto errorResult = createMaterial(errorMat);
    if (errorResult) errorMaterial_ = *errorResult;
}

void VulkanGraphics3DSystem::createPipelines() {
    if (!pIAssetSystem_) {
        std::fprintf(stderr, "[Vulkan] ERROR: AssetSystem is required for loading shaders\n");
        return;
    }

    // Load debug pipeline shaders (for debug line rendering)
    // Use :library:/shaders/ prefix for proper path resolution
    auto debugVertHandle = pIAssetSystem_->loadShaderCompiled(":library:/shaders/debug.vert");
    auto debugFragHandle = pIAssetSystem_->loadShaderCompiled(":library:/shaders/debug.frag");

    if (pIAssetSystem_->isLoaded(debugVertHandle) && pIAssetSystem_->isLoaded(debugFragHandle)) {
        const ShaderData* debugVert = pIAssetSystem_->getShaderData(debugVertHandle);
        const ShaderData* debugFrag = pIAssetSystem_->getShaderData(debugFragHandle);

        if (debugVert && debugFrag && !debugVert->spirvBytecode.empty() && !debugFrag->spirvBytecode.empty()) {
            VulkanPipelineDef debugDef;
            debugDef.shaderStages = {
                {VK_SHADER_STAGE_VERTEX_BIT, debugVert->spirvBytecode, "main"},
                {VK_SHADER_STAGE_FRAGMENT_BIT, debugFrag->spirvBytecode, "main"}
            };

            // Debug vertex layout: position (vec3) + color (vec4)
            debugDef.vertexBindings = {
                {0, sizeof(float) * 7, VK_VERTEX_INPUT_RATE_VERTEX}  // pos(3) + color(4)
            };
            debugDef.vertexAttributes = {
                {0, 0, VK_FORMAT_R32G32B32_SFLOAT, 0},                       // position
                {1, 0, VK_FORMAT_R32G32B32A32_SFLOAT, sizeof(float) * 3}     // color
            };

            // Push constants: mat4 viewProjection (64 bytes)
            debugDef.pushConstantRanges = {
                {VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::mat4)}
            };

            debugDef.topology = VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
            debugDef.depthTestEnable = true;
            debugDef.depthWriteEnable = false;
            debugDef.cullMode = VK_CULL_MODE_NONE;

            auto result = context_.createPipeline(debugDef);
            if (result) {
                debugPipeline_ = *result;
            } else {
                std::fprintf(stderr, "[Vulkan] Failed to create debug pipeline\n");
            }
        }
    }

    // Load basic 3D pipeline shaders (for mesh rendering)
    // Use :library:/shaders/ prefix for proper path resolution
    auto basic3dVertHandle = pIAssetSystem_->loadShaderCompiled(":library:/shaders/basic3d.vert");
    auto basic3dFragHandle = pIAssetSystem_->loadShaderCompiled(":library:/shaders/basic3d.frag");

    if (pIAssetSystem_->isLoaded(basic3dVertHandle) && pIAssetSystem_->isLoaded(basic3dFragHandle)) {
        const ShaderData* basic3dVert = pIAssetSystem_->getShaderData(basic3dVertHandle);
        const ShaderData* basic3dFrag = pIAssetSystem_->getShaderData(basic3dFragHandle);

        if (basic3dVert && basic3dFrag && !basic3dVert->spirvBytecode.empty() && !basic3dFrag->spirvBytecode.empty()) {
            VulkanPipelineDef pbrDef;
            pbrDef.shaderStages = {
                {VK_SHADER_STAGE_VERTEX_BIT, basic3dVert->spirvBytecode, "main"},
                {VK_SHADER_STAGE_FRAGMENT_BIT, basic3dFrag->spirvBytecode, "main"}
            };

            // 3D vertex layout: position (vec3) + normal (vec3) + texcoord (vec2)
            pbrDef.vertexBindings = {
                {0, sizeof(Vertex3D), VK_VERTEX_INPUT_RATE_VERTEX}
            };
            pbrDef.vertexAttributes = {
                {0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex3D, position)},
                {1, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex3D, normal)},
                {2, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(Vertex3D, texCoord)}
            };

            // Push constants: model + viewProjection + baseColor + lightDir + lightColor + ambientColor + cameraPos = 208 bytes
            pbrDef.pushConstantRanges = {
                {VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, 208}
            };

            pbrDef.depthTestEnable = true;
            pbrDef.depthWriteEnable = true;
            pbrDef.cullMode = VK_CULL_MODE_BACK_BIT;
            pbrDef.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;

            auto result = context_.createPipeline(pbrDef);
            if (result) {
                pbrPipeline_ = *result;
                unlitPipeline_ = *result;  // Use same pipeline for now
            } else {
                std::fprintf(stderr, "[Vulkan] Failed to create PBR pipeline\n");
            }
        }
    }

    // Create skinned mesh pipeline for skeletal animation (with texture support)
    auto skinnedVertHandle = pIAssetSystem_->loadShaderCompiled(":library:/shaders/skinned3d.vert");
    auto skinnedFragHandle = pIAssetSystem_->loadShaderCompiled(":library:/shaders/skinned3d.frag");

    if (pIAssetSystem_->isLoaded(skinnedVertHandle) && pIAssetSystem_->isLoaded(skinnedFragHandle)) {
        const ShaderData* skinnedVert = pIAssetSystem_->getShaderData(skinnedVertHandle);
        const ShaderData* skinnedFrag = pIAssetSystem_->getShaderData(skinnedFragHandle);

        if (skinnedVert && skinnedFrag && !skinnedVert->spirvBytecode.empty() && !skinnedFrag->spirvBytecode.empty()) {
            VulkanPipelineDef skinnedDef;
            skinnedDef.shaderStages = {
                {VK_SHADER_STAGE_VERTEX_BIT, skinnedVert->spirvBytecode, "main"},
                {VK_SHADER_STAGE_FRAGMENT_BIT, skinnedFrag->spirvBytecode, "main"}
            };

            // Skinned vertex layout: position + normal + texcoord + color + boneIndices + boneWeights
            skinnedDef.vertexBindings = {
                {0, sizeof(Vertex3D), VK_VERTEX_INPUT_RATE_VERTEX}
            };
            skinnedDef.vertexAttributes = {
                {0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex3D, position)},
                {1, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex3D, normal)},
                {2, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(Vertex3D, texCoord)},
                {3, 0, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(Vertex3D, color)},
                {4, 0, VK_FORMAT_R8G8B8A8_UINT, offsetof(Vertex3D, boneIndices)},
                {5, 0, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(Vertex3D, boneWeights)}
            };

            // Push constants same as PBR pipeline
            skinnedDef.pushConstantRanges = {
                {VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, 208}
            };

            // Add descriptor set layouts: set 0 = bone matrices, set 1 = material texture
            if (boneDescriptorSetLayout_ != VK_NULL_HANDLE && textureDescriptorSetLayout_ != VK_NULL_HANDLE) {
                skinnedDef.descriptorSetLayouts = {boneDescriptorSetLayout_, textureDescriptorSetLayout_};
            } else if (boneDescriptorSetLayout_ != VK_NULL_HANDLE) {
                skinnedDef.descriptorSetLayouts = {boneDescriptorSetLayout_};
            }

            skinnedDef.depthTestEnable = true;
            skinnedDef.depthWriteEnable = true;
            skinnedDef.cullMode = VK_CULL_MODE_BACK_BIT;
            skinnedDef.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;

            auto skinnedResult = context_.createPipeline(skinnedDef);
            if (skinnedResult) {
                skinnedPipeline_ = *skinnedResult;
                std::fprintf(stderr, "[Vulkan] Created skinned mesh pipeline with texture support\n");
            } else {
                std::fprintf(stderr, "[Vulkan] Failed to create skinned pipeline\n");
            }
        } else {
            std::fprintf(stderr, "[Vulkan] Skinned shader data is empty or null\n");
        }
    } else {
        std::fprintf(stderr, "[Vulkan] Failed to load skinned shaders\n");
    }
}

void VulkanGraphics3DSystem::updateCameraUBO() {
    // Compute forward and up vectors from quaternion rotation
    glm::quat rot{camera_.transform.rotation.w, camera_.transform.rotation.x,
                  camera_.transform.rotation.y, camera_.transform.rotation.z};
    glm::vec3 forward = rot * glm::vec3{0.0f, 0.0f, -1.0f};  // Default forward is -Z
    glm::vec3 up = rot * glm::vec3{0.0f, 1.0f, 0.0f};        // Default up is +Y

    glm::vec3 pos{camera_.transform.position.x, camera_.transform.position.y, camera_.transform.position.z};
    glm::mat4 view = glm::lookAt(pos, pos + forward, up);

    Size windowSize = context_.getWindowSize();
    float aspect = static_cast<float>(windowSize.width) / static_cast<float>(windowSize.height);
    glm::mat4 projection = glm::perspective(glm::radians(camera_.fovY), aspect, camera_.nearPlane, camera_.farPlane);
    projection[1][1] *= -1;  // Flip Y for Vulkan

    CameraUBO ubo;
    std::memcpy(&ubo.view, &view, sizeof(glm::mat4));
    std::memcpy(&ubo.projection, &projection, sizeof(glm::mat4));
    glm::mat4 viewProjection = projection * view;
    std::memcpy(&ubo.viewProjection, &viewProjection, sizeof(glm::mat4));
    ubo.cameraPosition = glm::vec4{pos.x, pos.y, pos.z, 1.0f};

    context_.uploadToBuffer(cameraUBO_, &ubo, sizeof(CameraUBO));
}

void VulkanGraphics3DSystem::updateLightUBO() {
    LightUBO ubo;
    if (hasDirectionalLight_) {
        ubo.directionalDir = glm::vec4{directionalLight_.direction.x,
                                       directionalLight_.direction.y,
                                       directionalLight_.direction.z, 0};
        ubo.directionalColor = glm::vec4{directionalLight_.color.x,
                                         directionalLight_.color.y,
                                         directionalLight_.color.z,
                                         directionalLight_.intensity};
    }
    ubo.ambientColor = glm::vec4{ambientColor_.x, ambientColor_.y, ambientColor_.z, ambientIntensity_};
    ubo.lightCounts = glm::ivec4{static_cast<int>(pointLights_.size()),
                                 static_cast<int>(spotLights_.size()), 0, 0};

    context_.uploadToBuffer(lightUBO_, &ubo, sizeof(LightUBO));
}

void VulkanGraphics3DSystem::renderDebugLines() {
    if (debugLines_.empty() || !debugRenderingEnabled_) return;
    if (debugPipeline_ == 0) return;

    // TODO: Debug line rendering needs proper buffer management (deferred destruction)
    // For now, just clear the debug lines to prevent memory buildup
    debugLines_.clear();
}

}  // namespace bestow::vulkan

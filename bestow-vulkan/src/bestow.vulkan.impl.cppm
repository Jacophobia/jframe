// bestow-vulkan/src/bestow.vulkan.impl.cppm
// Vulkan implementation module

module;

#include <kangaru/kangaru.hpp>
#include <bestow/kangaru_macros.hpp>
#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>
#include <VkBootstrap.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

export module bestow.vulkan.impl;

import std;
import bestow.vulkan;
import bestow.services;  // Re-exports all contracts including bestow.graphics, bestow.assets, etc.

export namespace bestow::vulkan {

//==========================================================================
// VulkanContext - Core Vulkan Resource Management
//==========================================================================

class VulkanContext {
public:
    VulkanContext() = default;
    ~VulkanContext();

    // Non-copyable, movable
    VulkanContext(const VulkanContext&) = delete;
    VulkanContext& operator=(const VulkanContext&) = delete;
    VulkanContext(VulkanContext&&) noexcept;
    VulkanContext& operator=(VulkanContext&&) noexcept;

    //======================================================================
    // Initialization
    //======================================================================

    Result<void, VulkanError> initialize(const VulkanConfig& config);
    void shutdown();
    bool isInitialized() const { return initialized_; }

    //======================================================================
    // Frame Management
    //======================================================================

    Result<void, VulkanError> beginFrame();
    Result<void, VulkanError> endFrame();
    void setClearColor(float r, float g, float b, float a);

    //======================================================================
    // Resource Management
    //======================================================================

    Result<VulkanBufferHandle, VulkanError> createBuffer(const VulkanBufferDef& def);
    void destroyBuffer(VulkanBufferHandle handle);

    Result<VulkanImageHandle, VulkanError> createImage(const VulkanImageDef& def);
    void destroyImage(VulkanImageHandle handle);

    Result<VulkanPipelineHandle, VulkanError> createPipeline(const VulkanPipelineDef& def);
    void destroyPipeline(VulkanPipelineHandle handle);

    //======================================================================
    // Buffer Operations
    //======================================================================

    void* mapBuffer(VulkanBufferHandle handle);
    void unmapBuffer(VulkanBufferHandle handle);
    Result<void, VulkanError> uploadToBuffer(VulkanBufferHandle handle, const void* data, std::size_t size, std::size_t offset = 0);

    //======================================================================
    // Image Operations
    //======================================================================

    // Upload 2D texture data to an image (layer 0)
    Result<void, VulkanError> uploadToImage(VulkanImageHandle handle, const void* data, std::size_t size);

    // Upload data to a specific layer of an image (for cubemaps: layer 0-5 = +X, -X, +Y, -Y, +Z, -Z)
    Result<void, VulkanError> uploadToImageLayer(VulkanImageHandle handle, const void* data, std::size_t size, std::uint32_t layer);

    // Get image view for binding to descriptor sets
    VkImageView getImageView(VulkanImageHandle handle) const;

    // Get sampler for binding to descriptor sets
    VkSampler getImageSampler(VulkanImageHandle handle) const;

    //======================================================================
    // Command Recording
    //======================================================================

    VkCommandBuffer getCurrentCommandBuffer() const { return currentCommandBuffer_; }
    VkRenderPass getRenderPass() const { return renderPass_; }

    //======================================================================
    // Pipeline Operations
    //======================================================================

    void bindPipeline(VulkanPipelineHandle handle);
    VkPipeline getPipeline(VulkanPipelineHandle handle) const;
    VkPipelineLayout getPipelineLayout(VulkanPipelineHandle handle) const;

    //======================================================================
    // Buffer Operations (additional)
    //======================================================================

    VkBuffer getBuffer(VulkanBufferHandle handle) const;

    //======================================================================
    // Accessors
    //======================================================================

    VkDevice getDevice() const { return device_; }
    VkPhysicalDevice getPhysicalDevice() const { return physicalDevice_; }
    VkQueue getGraphicsQueue() const { return graphicsQueue_; }
    VmaAllocator getAllocator() const { return allocator_; }
    VkExtent2D getSwapchainExtent() const { return swapchainExtent_; }
    std::uint32_t getCurrentFrameIndex() const { return currentFrame_; }
    GLFWwindow* getWindow() const { return window_; }

    VulkanStats getStats() const;

    bool isHeadless() const { return config_.headless; }
    Size getWindowSize() const;
    void setWindowSize(Size size);

private:
    // Configuration
    VulkanConfig config_;
    bool initialized_ = false;
    std::array<float, 4> clearColor_ = {0.0f, 0.0f, 0.0f, 1.0f};

    // Window (null for headless)
    GLFWwindow* window_ = nullptr;

    // Vulkan Core
    VkInstance instance_ = VK_NULL_HANDLE;
    VkDebugUtilsMessengerEXT debugMessenger_ = VK_NULL_HANDLE;
    VkSurfaceKHR surface_ = VK_NULL_HANDLE;
    VkPhysicalDevice physicalDevice_ = VK_NULL_HANDLE;
    VkDevice device_ = VK_NULL_HANDLE;
    VkQueue graphicsQueue_ = VK_NULL_HANDLE;
    VkQueue presentQueue_ = VK_NULL_HANDLE;
    std::uint32_t graphicsQueueFamily_ = 0;
    std::uint32_t presentQueueFamily_ = 0;

    // Swapchain
    VkSwapchainKHR swapchain_ = VK_NULL_HANDLE;
    std::vector<VkImage> swapchainImages_;
    std::vector<VkImageView> swapchainImageViews_;
    VkFormat swapchainImageFormat_ = VK_FORMAT_B8G8R8A8_UNORM;
    VkExtent2D swapchainExtent_{};

    // Depth buffer
    VkImage depthImage_ = VK_NULL_HANDLE;
    VmaAllocation depthImageAllocation_ = VK_NULL_HANDLE;
    VkImageView depthImageView_ = VK_NULL_HANDLE;
    VkFormat depthFormat_ = VK_FORMAT_D32_SFLOAT;

    // Render pass and framebuffers
    VkRenderPass renderPass_ = VK_NULL_HANDLE;
    std::vector<VkFramebuffer> framebuffers_;

    // Command pools and buffers
    VkCommandPool commandPool_ = VK_NULL_HANDLE;
    std::vector<VkCommandBuffer> commandBuffers_;
    VkCommandBuffer currentCommandBuffer_ = VK_NULL_HANDLE;

    // Synchronization
    static constexpr std::uint32_t MAX_FRAMES_IN_FLIGHT = 2;
    std::vector<VkSemaphore> imageAvailableSemaphores_;
    std::vector<VkSemaphore> renderFinishedSemaphores_;
    std::vector<VkFence> inFlightFences_;
    std::uint32_t currentFrame_ = 0;
    std::uint32_t currentImageIndex_ = 0;

    // VMA Allocator
    VmaAllocator allocator_ = VK_NULL_HANDLE;

    // vk-bootstrap objects (stored to avoid reconstructing from raw handles)
    std::optional<vkb::Instance> vkbInstance_;
    std::optional<vkb::PhysicalDevice> vkbPhysicalDevice_;

    // Resource tracking
    struct BufferResource {
        VkBuffer buffer = VK_NULL_HANDLE;
        VmaAllocation allocation = VK_NULL_HANDLE;
        std::size_t size = 0;
        void* mappedPtr = nullptr;
        bool hostVisible = false;
        bool persistentlyMapped = false;  // true if created with VMA_ALLOCATION_CREATE_MAPPED_BIT
    };
    std::map<VulkanBufferHandle, BufferResource> buffers_;
    VulkanBufferHandle nextBufferHandle_ = 1;

    struct ImageResource {
        VkImage image = VK_NULL_HANDLE;
        VmaAllocation allocation = VK_NULL_HANDLE;
        VkImageView view = VK_NULL_HANDLE;
        VkSampler sampler = VK_NULL_HANDLE;
        VulkanImageDef def;
    };
    std::map<VulkanImageHandle, ImageResource> images_;
    VulkanImageHandle nextImageHandle_ = 1;

    struct PipelineResource {
        VkPipeline pipeline = VK_NULL_HANDLE;
        VkPipelineLayout layout = VK_NULL_HANDLE;
    };
    std::map<VulkanPipelineHandle, PipelineResource> pipelines_;
    VulkanPipelineHandle nextPipelineHandle_ = 1;

    // Headless rendering resources
    VkImage headlessColorImage_ = VK_NULL_HANDLE;
    VmaAllocation headlessColorAllocation_ = VK_NULL_HANDLE;
    VkImageView headlessColorView_ = VK_NULL_HANDLE;
    VkFramebuffer headlessFramebuffer_ = VK_NULL_HANDLE;

    // Internal helpers
    Result<void, VulkanError> createInstance();
    Result<void, VulkanError> createSurface();
    Result<void, VulkanError> selectPhysicalDevice();
    Result<void, VulkanError> createLogicalDevice();
    Result<void, VulkanError> createAllocator();
    Result<void, VulkanError> createSwapchain();
    Result<void, VulkanError> createDepthResources();
    Result<void, VulkanError> createRenderPass();
    Result<void, VulkanError> createFramebuffers();
    Result<void, VulkanError> createCommandPool();
    Result<void, VulkanError> createCommandBuffers();
    Result<void, VulkanError> createSyncObjects();
    Result<void, VulkanError> createHeadlessResources();

    void cleanupSwapchain();
    Result<void, VulkanError> recreateSwapchain();

    VkFormat findDepthFormat();
    VkFormat findSupportedFormat(const std::vector<VkFormat>& candidates,
                                  VkImageTiling tiling,
                                  VkFormatFeatureFlags features);
};

//==========================================================================
// VulkanGraphicsSystem - 2D Vulkan Renderer
//==========================================================================

class VulkanGraphicsSystem : public IGraphicsSystem {
public:
    explicit VulkanGraphicsSystem(IAssetSystem* pIAssetSystem = nullptr)
        : pIAssetSystem_(pIAssetSystem) {}
    ~VulkanGraphicsSystem() override;

    Result<void, VulkanError> initialize(const VulkanConfig& config);

    //======================================================================
    // Frame Lifecycle
    //======================================================================

    void beginFrame() override;
    void endFrame() override;

    //======================================================================
    // Sprite Rendering
    //======================================================================

    void draw(const Sprite& sprite) override;
    void drawBatch(std::span<const Sprite> sprites) override;

    void drawSprite(const SpriteSheet& sheet, int frameIndex,
                   const Transform2D& transform, Color tint = Color::white()) override;
    void drawAnimatedSprite(AnimatedSprite& sprite,
                           const Transform2D& transform, Color tint = Color::white()) override;

    //======================================================================
    // Primitive Rendering
    //======================================================================

    void drawRect(const Canvas& rect, const Color& color, bool filled = true) override;
    void drawLine(Vec2 from, Vec2 to, const Color& color, float thickness = 1.0f) override;
    void drawCircle(Vec2 center, float radius, const Color& color,
                    bool filled = true, int segments = 32) override;
    void drawPolygon(std::span<const Vec2> vertices, const Color& color,
                     bool filled = true) override;

    //======================================================================
    // Text Rendering
    //======================================================================

    void drawText(const std::string& text, Vec2 position,
                  AssetHandle fontHandle, float size,
                  const Color& color = Color::white()) override;
    void drawTextCentered(const std::string& text, Vec2 position,
                          AssetHandle fontHandle, float size,
                          const Color& color = Color::white()) override;
    Vec2 measureText(const std::string& text, AssetHandle fontHandle,
                     float size) const override;

    //======================================================================
    // Camera
    //======================================================================

    void setCamera(const Camera& camera) override;
    Camera getCamera() const override;

    Vec2 worldToScreen(Vec2 worldPos) const override;
    Vec2 screenToWorld(Vec2 screenPos) const override;

    //======================================================================
    // Window Management
    //======================================================================

    Size getWindowSize() const override;
    void setWindowSize(Size size) override;
    bool isFullscreen() const override;
    void setFullscreen(bool fullscreen) override;
    bool shouldClose() const override;
    void* getNativeWindowHandle() const override;

    //======================================================================
    // Render State
    //======================================================================

    void setClearColor(const Color& color) override;
    void setVSync(bool enabled) override;

    //======================================================================
    // Automatic Entity Rendering
    //======================================================================

    void renderEntities(IEntitySystem& entities) override;
    void renderEntities(IEntitySystem& entities,
                        RenderLayer minLayer, RenderLayer maxLayer) override;
    void setViewportCulling(bool enabled) override;
    bool isViewportCullingEnabled() const override;

    //======================================================================
    // Vulkan-specific
    //======================================================================

    VulkanContext& getContext() { return context_; }
    const VulkanContext& getContext() const { return context_; }

private:
    VulkanContext context_;
    Camera camera_;
    Color clearColor_ = Color::black();
    bool viewportCullingEnabled_ = false;
    bool isFullscreen_ = false;

    // Windowed state (for restoring after exiting fullscreen)
    int windowedPosX_ = 100;
    int windowedPosY_ = 100;
    int windowedWidth_ = 800;
    int windowedHeight_ = 600;

    // Sprite batching
    struct SpriteVertex {
        glm::vec2 position;
        glm::vec2 texCoord;
        glm::vec4 color;
    };
    std::vector<SpriteVertex> spriteVertices_;
    VulkanBufferHandle spriteVertexBuffer_ = 0;
    VulkanPipelineHandle spritePipeline_ = 0;

    // Primitive rendering
    std::vector<glm::vec2> primitiveVertices_;
    std::vector<glm::vec4> primitiveColors_;
    VulkanBufferHandle primitiveVertexBuffer_ = 0;
    VulkanPipelineHandle primitivePipeline_ = 0;

    void flushSpriteBatch();
    void flushPrimitives();
    void createPipelines();

    // Injected dependencies
    IAssetSystem* pIAssetSystem_ = nullptr;
};

// Service definition - must be after class is complete
BESTOW_SERVICE(VulkanGraphicsSystem, GraphicsSystem, AssetSystem);

//==========================================================================
// VulkanGraphics3DSystem - 3D Vulkan Renderer
//==========================================================================

class VulkanGraphics3DSystem : public IGraphics3DSystem {
public:
    explicit VulkanGraphics3DSystem(IAssetSystem* pIAssetSystem = nullptr,
                                     IConfigSystem* pIConfigSystem = nullptr)
        : pIAssetSystem_(pIAssetSystem)
        , pIConfigSystem_(pIConfigSystem) {}
    ~VulkanGraphics3DSystem() override;

    //======================================================================
    // Lifecycle (from interface)
    //======================================================================

    bool initialize(const Graphics3DConfig& config) override;
    void shutdown() override;
    bool isInitialized() const override { return initialized_; }

    //======================================================================
    // Frame Lifecycle
    //======================================================================

    void beginFrame() override;
    void endFrame() override;

    //======================================================================
    // Mesh Management
    //======================================================================

    Result<MeshHandle, Graphics3DError> createMesh(const MeshDef& def) override;
    void destroyMesh(MeshHandle handle) override;
    bool hasMesh(MeshHandle handle) const override;
    AABB3D getMeshBounds(MeshHandle handle) const override;

    Result<void, Graphics3DError> updateMeshVertices(
        MeshHandle handle,
        std::span<const Vertex3D> vertices,
        std::uint32_t offset = 0) override;

    //======================================================================
    // Primitive Mesh Generation
    //======================================================================

    Result<MeshHandle, Graphics3DError> createCubeMesh(float size = 1.0f) override;
    Result<MeshHandle, Graphics3DError> createSphereMesh(
        float radius = 0.5f,
        std::uint32_t segments = 32,
        std::uint32_t rings = 16) override;
    Result<MeshHandle, Graphics3DError> createCylinderMesh(
        float radius = 0.5f,
        float height = 1.0f,
        std::uint32_t segments = 32) override;
    Result<MeshHandle, Graphics3DError> createCapsuleMesh(
        float radius = 0.5f,
        float height = 1.0f,
        std::uint32_t segments = 32,
        std::uint32_t rings = 8) override;
    Result<MeshHandle, Graphics3DError> createPlaneMesh(
        float width = 1.0f,
        float height = 1.0f,
        std::uint32_t widthSegments = 1,
        std::uint32_t heightSegments = 1) override;

    //======================================================================
    // Material Management
    //======================================================================

    Result<MaterialHandle, Graphics3DError> createMaterial(const PBRMaterial& mat) override;
    Result<MaterialHandle, Graphics3DError> createUnlitMaterial(const UnlitMaterial& mat) override;
    void destroyMaterial(MaterialHandle handle) override;
    bool hasMaterial(MaterialHandle handle) const override;

    Result<void, Graphics3DError> setMaterialTexture(
        MaterialHandle handle,
        std::uint32_t slot,
        AssetHandle texture) override;

    MaterialHandle getDefaultPBRMaterial() const override;
    MaterialHandle getDefaultUnlitMaterial() const override;
    MaterialHandle getErrorMaterial() const override;

    //======================================================================
    // Immediate Mode Rendering
    //======================================================================

    void drawMesh(
        MeshHandle mesh,
        MaterialHandle material,
        const Mat4& worldMatrix,
        bool castShadow = true,
        bool receiveShadow = true) override;

    void drawMesh(
        MeshHandle mesh,
        MaterialHandle material,
        const Transform3D& transform,
        bool castShadow = true,
        bool receiveShadow = true) override;

    //======================================================================
    // Batch Rendering
    //======================================================================

    void queueRenderItem(const RenderItem& item) override;
    void queueRenderItems(std::span<const RenderItem> items) override;
    void flushRenderQueue() override;

    //======================================================================
    // Entity Rendering (ECS)
    //======================================================================

    void renderEntities(IEntitySystem& entities) override;
    void renderEntities(IEntitySystem& entities, const Frustum& frustum) override;
    void renderEntities(IEntitySystem& entities,
                        RenderLayer minLayer, RenderLayer maxLayer) override;

    //======================================================================
    // Camera
    //======================================================================

    void setCamera(const Camera3D& camera) override;
    Camera3D getCamera() const override;
    void setCameraTarget(const Vec3& target) override;

    Ray3D screenToWorldRay(Vec2 screenPos) const override;
    std::optional<Vec2> worldToScreen(const Vec3& worldPos) const override;

    //======================================================================
    // Lighting
    //======================================================================

    void setDirectionalLight(const DirectionalLight& light) override;
    void clearDirectionalLight() override;

    std::uint32_t addPointLight(const PointLight& light, const Vec3& position) override;
    std::uint32_t addSpotLight(const SpotLight& light, const Vec3& position) override;

    void setLightPosition(std::uint32_t lightId, const Vec3& position) override;
    void removeLight(std::uint32_t lightId) override;
    void clearLights() override;

    void setAmbientLight(const Vec3& color, float intensity = 1.0f) override;
    void updateEntityLights(IEntitySystem& entities) override;

    //======================================================================
    // Environment
    //======================================================================

    void setSkybox(const Skybox& skybox) override;
    void clearSkybox() override;

    void setEnvironmentMap(const EnvironmentMap& envMap) override;
    void clearEnvironmentMap() override;

    void setFog(const Fog& fog) override;

    //======================================================================
    // Shadows
    //======================================================================

    void setShadowsEnabled(bool enabled) override;
    bool areShadowsEnabled() const override;
    void setDirectionalShadowResolution(int resolution) override;
    void setShadowDistance(float distance) override;

    //======================================================================
    // Debug Rendering
    //======================================================================

    void debugDrawLine(const Vec3& start, const Vec3& end, const Color& color = Color::white(),
                       float duration = 0.0f, bool depthTest = true) override;
    void debugDrawBox(const Vec3& center, const Vec3& halfExtents,
                      const Quat& rotation = Quat{1.0f, 0.0f, 0.0f, 0.0f},
                      const Color& color = Color::white(),
                      float duration = 0.0f, bool depthTest = true) override;
    void debugDrawSphere(const Vec3& center, float radius, const Color& color = Color::white(),
                         float duration = 0.0f, bool depthTest = true) override;
    void debugDrawCapsule(const Vec3& start, const Vec3& end, float radius,
                          const Color& color = Color::white(),
                          float duration = 0.0f, bool depthTest = true) override;
    void debugDrawFrustum(const Frustum& frustum, const Color& color = Color::white(),
                          float duration = 0.0f, bool depthTest = true) override;
    void debugDrawRay(const Vec3& origin, const Vec3& direction, float length,
                      const Color& color = Color::white(),
                      float duration = 0.0f, bool depthTest = true) override;
    void debugDrawAxes(const Transform3D& transform, float size = 1.0f,
                       float duration = 0.0f, bool depthTest = true) override;
    void debugDrawAABB(const AABB3D& aabb, const Color& color = Color::white(),
                       float duration = 0.0f, bool depthTest = true) override;

    void debugClear() override;
    void setDebugRenderingEnabled(bool enabled) override;
    bool isDebugRenderingEnabled() const override;

    //======================================================================
    // Window Management
    //======================================================================

    Size getWindowSize() const override;
    void setWindowSize(Size size) override;
    bool isFullscreen() const override;
    void setFullscreen(bool fullscreen) override;
    bool shouldClose() const override;
    void* getNativeWindowHandle() const override;

    //======================================================================
    // Render State
    //======================================================================

    void setClearColor(const Color& color) override;
    void setVSync(bool enabled) override;
    void setRenderScale(float scale) override;
    float getRenderScale() const override;

    //======================================================================
    // Runtime Configuration (Hot-Reloadable)
    //======================================================================

    bool loadRuntimeConfig(const std::filesystem::path& configPath) override;
    void applyRuntimeConfig(const Graphics3DRuntimeConfig& config) override;
    const Graphics3DRuntimeConfig& getRuntimeConfig() const override;
    bool reloadRuntimeConfig() override;

    //======================================================================
    // Shader System Integration
    //======================================================================

    void drawMeshWithShaderMaterial(
        MeshHandle mesh,
        ShaderProgramHandle shader,
        const Mat4& worldMatrix,
        bool castShadow = true,
        bool receiveShadow = true) override;

    Result<void, Graphics3DError> drawMeshWithLuaMaterial(
        MeshHandle mesh,
        std::string_view materialPath,
        const Mat4& worldMatrix) override;

    Result<void, Graphics3DError> drawMeshWithLuaMaterial(
        MeshHandle mesh,
        std::string_view materialPath,
        const Mat4& worldMatrix,
        const Vec4& colorOverride) override;

    //======================================================================
    // Asset System Integration
    //======================================================================

    Result<MeshHandle, Graphics3DError> createMeshFromData(const MeshData& data) override;
    Result<MaterialHandle, Graphics3DError> createMaterialFromData(const MaterialData& data) override;
    Result<std::vector<MaterialHandle>, Graphics3DError> createMaterialsFromModel(const ModelData& data) override;
    Result<void, Graphics3DError> createSkyboxFromData(const CubemapData& data) override;

    //======================================================================
    // Culling
    //======================================================================

    void setFrustumCulling(bool enabled) override;
    bool isFrustumCullingEnabled() const override;

    //======================================================================
    // Post-Processing
    //======================================================================

    void setToneMapping(bool enabled) override;
    void setExposure(float exposure) override;
    void setBloom(bool enabled, float threshold = 1.0f, float intensity = 1.0f) override;
    void setSSAO(bool enabled, float radius = 0.5f, float intensity = 1.0f) override;

    //======================================================================
    // Statistics
    //======================================================================

    RenderStats getStats() const override;

    //======================================================================
    // Instanced Rendering
    //======================================================================

    Result<InstanceBufferHandle, Graphics3DError> createInstanceBuffer(
        std::uint32_t maxInstances, bool dynamic = true) override;
    Result<void, Graphics3DError> updateInstanceBuffer(
        InstanceBufferHandle buffer,
        std::span<const InstanceData> data,
        std::uint32_t offset = 0) override;
    void destroyInstanceBuffer(InstanceBufferHandle buffer) override;
    void drawInstanced(const InstancedRenderItem& item) override;
    void queueInstancedRenderItem(const InstancedRenderItem& item) override;

    //======================================================================
    // Skeletal Animation
    //======================================================================

    Result<SkeletonHandle, Graphics3DError> createSkeleton(const ModelData& modelData) override;
    Result<AnimationClipHandle, Graphics3DError> createAnimationClip(
        SkeletonHandle skeleton,
        const std::string& clipName,
        const ModelData& modelData) override;
    void destroySkeleton(SkeletonHandle skeleton) override;
    void destroyAnimationClip(AnimationClipHandle clip) override;

    std::vector<std::string> getAnimationClipNames(SkeletonHandle skeleton) const override;
    AnimationClip getAnimationClipInfo(AnimationClipHandle clip) const override;

    std::vector<Mat4> sampleAnimation(AnimationClipHandle clip, float time, bool loop = true) override;
    std::vector<Mat4> blendAnimations(const BlendedAnimation& blend) override;

    void drawSkinnedMesh(
        MeshHandle mesh,
        MaterialHandle material,
        const Mat4& worldMatrix,
        std::span<const Mat4> boneTransforms) override;

    //======================================================================
    // 3D Text Rendering
    //======================================================================

    Result<Font3DHandle, Graphics3DError> loadFont3D(AssetHandle fontAsset) override;
    void destroyFont3D(Font3DHandle font) override;
    void drawText3D(const Text3DItem& item) override;
    void drawText3D(const std::string& text, const Vec3& position, Font3DHandle font,
                    float fontSize = 1.0f, const Color& color = Color::white()) override;
    AABB3D measureText3D(const std::string& text, Font3DHandle font, float fontSize = 1.0f) override;

    //======================================================================
    // Material Property Updates
    //======================================================================

    Result<void, Graphics3DError> setMaterialBaseColor(MaterialHandle handle, const Vec4& color) override;
    Result<void, Graphics3DError> setMaterialMetallicRoughness(
        MaterialHandle handle, float metallic, float roughness) override;
    Result<void, Graphics3DError> setMaterialEmissive(MaterialHandle handle, const Vec3& emissive) override;
    std::optional<PBRMaterial> getMaterialProperties(MaterialHandle handle) const override;

    //======================================================================
    // LOD (Level of Detail)
    //======================================================================

    void setLODDistances(std::span<const float> distances) override;
    void registerLODMeshes(MeshHandle primaryMesh, std::span<const MeshHandle> lodMeshes) override;
    void setLODBias(float bias) override;

    //======================================================================
    // Vulkan-specific
    //======================================================================

    VulkanContext& getContext() { return context_; }
    const VulkanContext& getContext() const { return context_; }

private:
    VulkanContext context_;
    Camera3D camera_;
    Vec3 cameraTarget_{0.0f, 0.0f, 0.0f};
    bool useCameraTarget_ = false;
    Color clearColor_ = Color::black();
    bool isFullscreen_ = false;
    float renderScale_ = 1.0f;

    // Windowed state (for restoring after exiting fullscreen)
    int windowedPosX_ = 100;
    int windowedPosY_ = 100;
    int windowedWidth_ = 800;
    int windowedHeight_ = 600;

    // Lighting state
    DirectionalLight directionalLight_;
    bool hasDirectionalLight_ = false;
    std::map<std::uint32_t, std::pair<PointLight, Vec3>> pointLights_;
    std::map<std::uint32_t, std::pair<SpotLight, Vec3>> spotLights_;
    std::uint32_t nextLightId_ = 1;
    Vec3 ambientColor_{0.1f, 0.1f, 0.1f};
    float ambientIntensity_ = 1.0f;

    // Environment
    Skybox skybox_;
    bool hasSkybox_ = false;
    CubemapData skyboxCubemapData_;  // Store cubemap pixel data
    VulkanImageHandle skyboxCubemapImage_ = 0;  // GPU cubemap texture
    EnvironmentMap environmentMap_;
    bool hasEnvironmentMap_ = false;
    Fog fog_;

    // Shadows
    bool shadowsEnabled_ = true;
    int shadowResolution_ = 2048;
    float shadowDistance_ = 100.0f;

    // Culling
    bool frustumCullingEnabled_ = true;

    // Post-processing
    bool toneMappingEnabled_ = true;
    float exposure_ = 1.0f;
    bool bloomEnabled_ = false;
    float bloomThreshold_ = 1.0f;
    float bloomIntensity_ = 1.0f;
    bool ssaoEnabled_ = false;
    float ssaoRadius_ = 0.5f;
    float ssaoIntensity_ = 1.0f;

    // Debug rendering
    bool debugRenderingEnabled_ = true;
    struct DebugLine {
        Vec3 start;
        Vec3 end;
        Color color;
        float duration;
        bool depthTest;
        float timeRemaining;
    };
    std::vector<DebugLine> debugLines_;
    VulkanBufferHandle debugLineBuffer_ = 0;
    static constexpr std::size_t MAX_DEBUG_LINES = 10000;

    // Initialization state
    bool initialized_ = false;

    // Runtime config
    Graphics3DRuntimeConfig runtimeConfig_;
    std::filesystem::path configPath_;

    // LOD
    std::vector<float> lodDistances_;
    std::map<MeshHandle, std::vector<MeshHandle>> lodMeshRegistry_;
    float lodBias_ = 1.0f;

    // Mesh storage
    struct VulkanMesh {
        VulkanBufferHandle vertexBuffer = 0;
        VulkanBufferHandle indexBuffer = 0;
        std::uint32_t vertexCount = 0;
        std::uint32_t indexCount = 0;
        AABB3D bounds;
        bool isDynamic = false;
    };
    std::map<MeshHandle, VulkanMesh> meshes_;
    MeshHandle nextMeshHandle_ = 1;

    // Material storage
    struct VulkanMaterial {
        PBRMaterial pbrData;
        VulkanDescriptorSetHandle descriptorSet = 0;
        bool isPBR = true;
    };
    std::map<MaterialHandle, VulkanMaterial> materials_;
    MaterialHandle nextMaterialHandle_ = 1;
    MaterialHandle defaultPBRMaterial_ = 0;
    MaterialHandle defaultUnlitMaterial_ = 0;
    MaterialHandle errorMaterial_ = 0;

    // Instance buffers
    std::set<InstanceBufferHandle> instanceBuffers_;
    InstanceBufferHandle nextInstanceBufferHandle_ = 1;

    // Skeletal animation
    std::set<SkeletonHandle> skeletons_;
    std::map<AnimationClipHandle, AnimationClip> animationClips_;
    SkeletonHandle nextSkeletonHandle_ = 1;
    AnimationClipHandle nextAnimationClipHandle_ = 1;

    // 3D fonts
    std::set<Font3DHandle> fonts3D_;
    Font3DHandle nextFont3DHandle_ = 1;

    // Render queue
    std::vector<RenderItem> renderQueue_;

    // Skinned mesh render queue (separate because it needs bone transforms)
    struct SkinnedRenderItem {
        MeshHandle mesh;
        MaterialHandle material;
        Mat4 worldMatrix;
        std::vector<Mat4> boneTransforms;
    };
    std::vector<SkinnedRenderItem> skinnedRenderQueue_;

    // Pipelines
    VulkanPipelineHandle pbrPipeline_ = 0;
    VulkanPipelineHandle unlitPipeline_ = 0;
    VulkanPipelineHandle debugPipeline_ = 0;
    VulkanPipelineHandle skyboxPipeline_ = 0;
    VulkanPipelineHandle skinnedPipeline_ = 0;  // For skeletal animation

    // Bone matrix buffer for skinned mesh rendering
    static constexpr std::size_t MAX_BONES = 100;
    VulkanBufferHandle boneUBO_ = 0;
    VkDescriptorSetLayout boneDescriptorSetLayout_ = VK_NULL_HANDLE;
    VkDescriptorSet boneDescriptorSet_ = VK_NULL_HANDLE;
    VkDescriptorPool boneDescriptorPool_ = VK_NULL_HANDLE;

    // Material texture descriptor set (set 1 for skinned pipeline)
    VkDescriptorSetLayout textureDescriptorSetLayout_ = VK_NULL_HANDLE;
    VkDescriptorPool textureDescriptorPool_ = VK_NULL_HANDLE;
    VulkanImageHandle defaultWhiteTexture_ = 0;
    VkDescriptorSet defaultTextureDescriptorSet_ = VK_NULL_HANDLE;

    // Per-material texture descriptor sets
    std::map<MaterialHandle, VkDescriptorSet> materialTextureDescriptorSets_;
    std::map<MaterialHandle, VulkanImageHandle> materialTextures_;  // GPU textures for materials

    // Lua material pipeline cache (material name -> pipeline handle)
    std::unordered_map<std::string, VulkanPipelineHandle> materialPipelineCache_;
    std::string shaderBasePath_;

    // Shader hot reload tracking (watches GLSL sources, compiles to SPIR-V at runtime)
    struct ShaderFileInfo {
        std::string vertGlslPath;  // Path to .vert GLSL source
        std::string fragGlslPath;  // Path to .frag GLSL source
        std::filesystem::file_time_type vertLastModified;
        std::filesystem::file_time_type fragLastModified;
        AssetHandle vertShaderAsset;  // Asset handle for vertex shader (for subscription)
        AssetHandle fragShaderAsset;  // Asset handle for fragment shader (for subscription)
    };
    std::unordered_map<VulkanPipelineHandle, ShaderFileInfo> pipelineShaderFiles_;
    bool hotReloadEnabled_ = true;
    std::chrono::milliseconds hotReloadCheckInterval_{500};  // Check every 500ms
    std::chrono::steady_clock::time_point lastHotReloadCheck_{};
    std::chrono::steady_clock::time_point lastSuccessfulReload_{};  // Cooldown after reload
    std::chrono::milliseconds reloadCooldown_{1000};  // Wait 1s after reload before checking again

    // Asset system subscription for shader hot reload
    SubscriptionId shaderSubscriptionId_ = InvalidSubscriptionId;
    std::unordered_map<UUID, std::vector<VulkanPipelineHandle>> shaderAssetToPipelines_;  // Track which pipelines use which shader assets
    bool useAssetSystemHotReload_ = true;  // Use asset system subscriptions instead of polling
    void onShaderAssetChanged(AssetHandle handle, AssetType type);
    void reloadPipelineShaders(VulkanPipelineHandle pipeline, const ShaderFileInfo& info);

    // Helper to load/create pipeline for a Lua material
    VulkanPipelineHandle getOrCreateMaterialPipeline(std::string_view materialPath);
    VulkanPipelineHandle loadShaderPipeline(std::string_view vertPath, std::string_view fragPath);
    void checkShaderHotReload();

    // Uniform buffers
    struct CameraUBO {
        glm::mat4 view;
        glm::mat4 projection;
        glm::mat4 viewProjection;
        glm::vec4 cameraPosition;
    };
    VulkanBufferHandle cameraUBO_ = 0;

    struct LightUBO {
        glm::vec4 directionalDir;
        glm::vec4 directionalColor;
        glm::vec4 ambientColor;
        glm::ivec4 lightCounts;  // x = point, y = spot
        // Point lights and spot lights would follow in arrays
    };
    VulkanBufferHandle lightUBO_ = 0;

    void createDefaultMaterials();
    void createPipelines();
    void updateCameraUBO();
    void updateLightUBO();
    void renderDebugLines();

    // Injected dependencies
    IAssetSystem* pIAssetSystem_ = nullptr;
    IConfigSystem* pIConfigSystem_ = nullptr;

public:
    // Forward declaration - defined after class is complete
    struct Service;
};

// Service type for Engine::use<IGraphics3DSystem, VulkanGraphics3DSystem>()
struct VulkanGraphics3DSystem::Service : kgr::single_service<VulkanGraphics3DSystem>, kgr::overrides<IGraphics3DSystemService> {
    static auto construct(
        kgr::inject_t<IAssetSystemService> d1,
        kgr::inject_t<IConfigSystemService> d2)
        -> kgr::inject_result<IAssetSystem*, IConfigSystem*> {
        return kgr::inject(&d1.forward(), &d2.forward());
    }
};

// Backwards compatibility alias
using VulkanGraphics3DSystemService = VulkanGraphics3DSystem::Service;

}  // namespace bestow::vulkan

// Export to bestow namespace for cleaner API
export namespace bestow {
    using VulkanGraphics3DSystem = vulkan::VulkanGraphics3DSystem;
}

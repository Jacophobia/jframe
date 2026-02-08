// tests/mocks/MockGraphics3DSystem.hpp
// Mock 3D graphics system for headless testing

#pragma once

import std;
import bestow.graphics3d;
import bestow.uirender;
import bestow.types;
import bestow.assets;
import bestow.entity;
import bestow.animation;

namespace bestow::tests {

// Track 3D rendering operations for test verification
struct DrawMeshCall {
    MeshHandle mesh;
    MaterialHandle material;
    Mat4 worldMatrix;
    bool castShadow;
    bool receiveShadow;
};

struct DebugLineCall {
    Vec3 start;
    Vec3 end;
    Color color;
    float duration;
    bool depthTest;
};

class MockGraphics3DSystem : public IGraphics3DSystem {
public:
    MockGraphics3DSystem() = default;
    ~MockGraphics3DSystem() override = default;

    //==========================================================================
    // Test Utilities
    //==========================================================================

    void clearRecordedCalls() {
        meshDrawCalls_.clear();
        debugLineCalls_.clear();
        renderItemQueue_.clear();
    }

    const std::vector<DrawMeshCall>& getMeshDrawCalls() const { return meshDrawCalls_; }
    const std::vector<DebugLineCall>& getDebugLineCalls() const { return debugLineCalls_; }
    std::size_t getMeshDrawCallCount() const { return meshDrawCalls_.size(); }
    int getFrameCount() const { return frameCount_; }
    bool wasBeginFrameCalled() const { return beginFrameCalled_; }
    bool wasEndFrameCalled() const { return endFrameCalled_; }

    //==========================================================================
    // Initialization
    //==========================================================================

    bool initialize(const Graphics3DConfig& config) override {
        initialized_ = true;
        return true;
    }

    void shutdown() override {
        initialized_ = false;
    }

    bool isInitialized() const override {
        return initialized_;
    }

    //==========================================================================
    // IGraphicsContext Interface
    //==========================================================================

    IUIRenderBackend* getUIRenderBackend() override { return nullptr; }
    bool isInFrame() const override { return beginFrameCalled_ && !endFrameCalled_; }
    void* getRenderContext() const override { return nullptr; }
    void* getCurrentCommandBuffer() const override { return nullptr; }

    //==========================================================================
    // Frame Lifecycle
    //==========================================================================

    void beginFrame() override {
        beginFrameCalled_ = true;
        frameCount_++;
        clearRecordedCalls();
    }

    void endFrame() override {
        endFrameCalled_ = true;
    }

    //==========================================================================
    // Mesh Management
    //==========================================================================

    Result<MeshHandle, Graphics3DError> createMesh(const MeshDef& def) override {
        MeshHandle handle = nextMeshHandle_++;
        meshes_.insert(handle);
        meshBounds_[handle] = def.bounds;
        return handle;
    }

    void destroyMesh(MeshHandle handle) override {
        meshes_.erase(handle);
        meshBounds_.erase(handle);
    }

    bool hasMesh(MeshHandle handle) const override {
        return meshes_.contains(handle);
    }

    AABB3D getMeshBounds(MeshHandle handle) const override {
        auto it = meshBounds_.find(handle);
        if (it != meshBounds_.end()) {
            return it->second;
        }
        return AABB3D{};
    }

    Result<void, Graphics3DError> updateMeshVertices(
        MeshHandle handle,
        std::span<const Vertex3D> vertices,
        std::uint32_t offset) override {
        if (!meshes_.contains(handle)) {
            return std::unexpected(Graphics3DError::InvalidMesh);
        }
        return {};
    }

    //==========================================================================
    // Primitive Mesh Generation
    //==========================================================================

    Result<MeshHandle, Graphics3DError> createCubeMesh(float size) override {
        MeshHandle handle = nextMeshHandle_++;
        meshes_.insert(handle);
        float half = size / 2.0f;
        meshBounds_[handle] = AABB3D{Vec3{-half}, Vec3{half}};
        return handle;
    }

    Result<MeshHandle, Graphics3DError> createSphereMesh(
        float radius, std::uint32_t segments, std::uint32_t rings) override {
        MeshHandle handle = nextMeshHandle_++;
        meshes_.insert(handle);
        meshBounds_[handle] = AABB3D{Vec3{-radius}, Vec3{radius}};
        return handle;
    }

    Result<MeshHandle, Graphics3DError> createCylinderMesh(
        float radius, float height, std::uint32_t segments) override {
        MeshHandle handle = nextMeshHandle_++;
        meshes_.insert(handle);
        meshBounds_[handle] = AABB3D{Vec3{-radius, 0, -radius}, Vec3{radius, height, radius}};
        return handle;
    }

    Result<MeshHandle, Graphics3DError> createCapsuleMesh(
        float radius, float height, std::uint32_t segments, std::uint32_t rings) override {
        MeshHandle handle = nextMeshHandle_++;
        meshes_.insert(handle);
        float totalHeight = height + 2 * radius;
        meshBounds_[handle] = AABB3D{Vec3{-radius, 0, -radius}, Vec3{radius, totalHeight, radius}};
        return handle;
    }

    Result<MeshHandle, Graphics3DError> createPlaneMesh(
        float width, float height, std::uint32_t widthSegments, std::uint32_t heightSegments) override {
        MeshHandle handle = nextMeshHandle_++;
        meshes_.insert(handle);
        meshBounds_[handle] = AABB3D{Vec3{-width/2, 0, -height/2}, Vec3{width/2, 0, height/2}};
        return handle;
    }

    //==========================================================================
    // Material Management
    //==========================================================================

    Result<MaterialHandle, Graphics3DError> createMaterial(const PBRMaterial& mat) override {
        MaterialHandle handle = nextMaterialHandle_++;
        materials_.insert(handle);
        pbrMaterials_[handle] = mat;
        return handle;
    }

    Result<MaterialHandle, Graphics3DError> createUnlitMaterial(const UnlitMaterial& mat) override {
        MaterialHandle handle = nextMaterialHandle_++;
        materials_.insert(handle);
        return handle;
    }

    void destroyMaterial(MaterialHandle handle) override {
        materials_.erase(handle);
        pbrMaterials_.erase(handle);
    }

    bool hasMaterial(MaterialHandle handle) const override {
        return materials_.contains(handle);
    }

    Result<void, Graphics3DError> setMaterialTexture(
        MaterialHandle handle, std::uint32_t slot, AssetHandle texture) override {
        if (!materials_.contains(handle)) {
            return std::unexpected(Graphics3DError::InvalidMaterial);
        }
        return {};
    }

    MaterialHandle getDefaultPBRMaterial() const override {
        return defaultPBRMaterial_;
    }

    MaterialHandle getDefaultUnlitMaterial() const override {
        return defaultUnlitMaterial_;
    }

    MaterialHandle getErrorMaterial() const override {
        return errorMaterial_;
    }

    //==========================================================================
    // Immediate Mode Rendering
    //==========================================================================

    void drawMesh(
        MeshHandle mesh,
        MaterialHandle material,
        const Mat4& worldMatrix,
        bool castShadow,
        bool receiveShadow) override {
        meshDrawCalls_.push_back({mesh, material, worldMatrix, castShadow, receiveShadow});
    }

    void drawMesh(
        MeshHandle mesh,
        MaterialHandle material,
        const Transform3D& transform,
        bool castShadow,
        bool receiveShadow) override {
        // Convert transform to matrix and record
        Mat4 worldMatrix{};  // Identity for mock
        meshDrawCalls_.push_back({mesh, material, worldMatrix, castShadow, receiveShadow});
    }

    //==========================================================================
    // Batch Rendering
    //==========================================================================

    void queueRenderItem(const RenderItem& item) override {
        renderItemQueue_.push_back(item);
    }

    void queueRenderItems(std::span<const RenderItem> items) override {
        for (const auto& item : items) {
            renderItemQueue_.push_back(item);
        }
    }

    void flushRenderQueue() override {
        for (const auto& item : renderItemQueue_) {
            meshDrawCalls_.push_back({item.mesh, item.material, item.worldMatrix,
                                      item.castShadow, item.receiveShadow});
        }
        renderItemQueue_.clear();
    }

    //==========================================================================
    // Entity Rendering (ECS)
    //==========================================================================

    void renderEntities(IEntitySystem& entities) override {
        entitiesRenderedCount_++;
    }

    void renderEntities(IEntitySystem& entities, const Frustum& frustum) override {
        entitiesRenderedCount_++;
    }

    void renderEntities(
        IEntitySystem& entities,
        RenderLayer minLayer,
        RenderLayer maxLayer) override {
        entitiesRenderedCount_++;
        lastMinLayer_ = minLayer;
        lastMaxLayer_ = maxLayer;
    }

    //==========================================================================
    // Camera
    //==========================================================================

    void setCamera(const Camera3D& camera) override {
        camera_ = camera;
    }

    Camera3D getCamera() const override {
        return camera_;
    }

    void setCameraTarget(const Vec3& target) override {
        cameraTarget_ = target;
    }

    Ray3D screenToWorldRay(Vec2 screenPos) const override {
        // Simple mock ray pointing forward
        return Ray3D{camera_.transform.position, Vec3{0, 0, -1}};
    }

    std::optional<Vec2> worldToScreen(const Vec3& worldPos) const override {
        // Simple mock: just project to screen center
        return Vec2{windowSize_.width / 2.0f, windowSize_.height / 2.0f};
    }

    //==========================================================================
    // Lighting
    //==========================================================================

    void setDirectionalLight(const DirectionalLight& light) override {
        directionalLight_ = light;
        hasDirectionalLight_ = true;
    }

    void clearDirectionalLight() override {
        hasDirectionalLight_ = false;
    }

    std::uint32_t addPointLight(const PointLight& light, const Vec3& position) override {
        std::uint32_t id = nextLightId_++;
        pointLights_[id] = {light, position};
        return id;
    }

    std::uint32_t addSpotLight(const SpotLight& light, const Vec3& position) override {
        std::uint32_t id = nextLightId_++;
        spotLights_[id] = {light, position};
        return id;
    }

    void setLightPosition(std::uint32_t lightId, const Vec3& position) override {
        if (auto it = pointLights_.find(lightId); it != pointLights_.end()) {
            it->second.second = position;
        }
        if (auto it = spotLights_.find(lightId); it != spotLights_.end()) {
            it->second.second = position;
        }
    }

    void removeLight(std::uint32_t lightId) override {
        pointLights_.erase(lightId);
        spotLights_.erase(lightId);
    }

    void clearLights() override {
        pointLights_.clear();
        spotLights_.clear();
    }

    void setAmbientLight(const Vec3& color, float intensity) override {
        ambientColor_ = color;
        ambientIntensity_ = intensity;
    }

    void updateEntityLights(IEntitySystem& entities) override {
        // Mock: just track that it was called
    }

    //==========================================================================
    // Environment
    //==========================================================================

    void setSkybox(const Skybox& skybox) override {
        skybox_ = skybox;
        hasSkybox_ = true;
    }

    void clearSkybox() override {
        hasSkybox_ = false;
    }

    void setEnvironmentMap(const EnvironmentMap& envMap) override {
        environmentMap_ = envMap;
        hasEnvironmentMap_ = true;
    }

    void clearEnvironmentMap() override {
        hasEnvironmentMap_ = false;
    }

    void setFog(const Fog& fog) override {
        fog_ = fog;
    }

    //==========================================================================
    // Shadows
    //==========================================================================

    void setShadowsEnabled(bool enabled) override {
        shadowsEnabled_ = enabled;
    }

    bool areShadowsEnabled() const override {
        return shadowsEnabled_;
    }

    void setDirectionalShadowResolution(int resolution) override {
        shadowResolution_ = resolution;
    }

    void setShadowDistance(float distance) override {
        shadowDistance_ = distance;
    }

    //==========================================================================
    // Debug Rendering
    //==========================================================================

    void debugDrawLine(
        const Vec3& start, const Vec3& end, const Color& color,
        float duration, bool depthTest) override {
        debugLineCalls_.push_back({start, end, color, duration, depthTest});
    }

    void debugDrawBox(
        const Vec3& center, const Vec3& halfExtents, const Quat& rotation,
        const Color& color, float duration, bool depthTest) override {
        // Mock: just track that debug drawing was requested
    }

    void debugDrawSphere(
        const Vec3& center, float radius, const Color& color,
        float duration, bool depthTest) override {}

    void debugDrawCapsule(
        const Vec3& start, const Vec3& end, float radius,
        const Color& color, float duration, bool depthTest) override {}

    void debugDrawFrustum(
        const Frustum& frustum, const Color& color,
        float duration, bool depthTest) override {}

    void debugDrawRay(
        const Vec3& origin, const Vec3& direction, float length,
        const Color& color, float duration, bool depthTest) override {}

    void debugDrawAxes(
        const Transform3D& transform, float size,
        float duration, bool depthTest) override {}

    void debugDrawAABB(
        const AABB3D& aabb, const Color& color,
        float duration, bool depthTest) override {}

    void debugClear() override {
        debugLineCalls_.clear();
    }

    void setDebugRenderingEnabled(bool enabled) override {
        debugRenderingEnabled_ = enabled;
    }

    bool isDebugRenderingEnabled() const override {
        return debugRenderingEnabled_;
    }

    //==========================================================================
    // Window Management
    //==========================================================================

    Size getWindowSize() const override {
        return windowSize_;
    }

    void setWindowSize(Size size) override {
        windowSize_ = size;
    }

    bool isFullscreen() const override {
        return isFullscreen_;
    }

    void setFullscreen(bool fullscreen) override {
        isFullscreen_ = fullscreen;
    }

    bool shouldClose() const override {
        return shouldClose_;
    }

    void setShouldClose(bool close) {
        shouldClose_ = close;
    }

    void* getNativeWindowHandle() const override {
        return nullptr;
    }

    //==========================================================================
    // Render State
    //==========================================================================

    void setClearColor(const Color& color) override {
        clearColor_ = color;
    }

    void setVSync(bool enabled) override {
        vsyncEnabled_ = enabled;
    }

    void setRenderScale(float scale) override {
        renderScale_ = scale;
    }

    float getRenderScale() const override {
        return renderScale_;
    }

    //==========================================================================
    // Shader System Integration
    //==========================================================================

    void drawMeshWithShaderMaterial(
        MeshHandle mesh,
        ShaderProgramHandle shader,
        const Mat4& worldMatrix,
        bool castShadow,
        bool receiveShadow) override {
        meshDrawCalls_.push_back({mesh, 0, worldMatrix, castShadow, receiveShadow});
    }

    Result<void, Graphics3DError> drawMeshWithLuaMaterial(
        MeshHandle mesh,
        std::string_view materialPath,
        const Mat4& worldMatrix) override {
        meshDrawCalls_.push_back({mesh, 0, worldMatrix, true, true});
        return {};
    }

    Result<void, Graphics3DError> drawMeshWithLuaMaterial(
        MeshHandle mesh,
        std::string_view materialPath,
        const Mat4& worldMatrix,
        const Vec4& colorOverride) override {
        meshDrawCalls_.push_back({mesh, 0, worldMatrix, true, true});
        return {};
    }

    //==========================================================================
    // Asset System Integration
    //==========================================================================

    Result<MeshHandle, Graphics3DError> createMeshFromData(const MeshData& data) override {
        return createMesh(MeshDef{});
    }

    Result<MaterialHandle, Graphics3DError> createMaterialFromData(const MaterialData& data) override {
        return createMaterial(PBRMaterial{});
    }

    Result<std::vector<MaterialHandle>, Graphics3DError> createMaterialsFromModel(const ModelData& data) override {
        std::vector<MaterialHandle> handles;
        for (std::size_t i = 0; i < data.materials.size(); ++i) {
            auto result = createMaterial(PBRMaterial{});
            if (result) {
                handles.push_back(*result);
            }
        }
        return handles;
    }

    Result<void, Graphics3DError> createSkyboxFromData(const CubemapData& data) override {
        return {};
    }

    //==========================================================================
    // Culling
    //==========================================================================

    void setFrustumCulling(bool enabled) override {
        frustumCullingEnabled_ = enabled;
    }

    bool isFrustumCullingEnabled() const override {
        return frustumCullingEnabled_;
    }

    //==========================================================================
    // Post-Processing
    //==========================================================================

    void setToneMapping(bool enabled) override {
        toneMappingEnabled_ = enabled;
    }

    void setExposure(float exposure) override {
        exposure_ = exposure;
    }

    void setBloom(bool enabled, float threshold, float intensity) override {
        bloomEnabled_ = enabled;
        bloomThreshold_ = threshold;
        bloomIntensity_ = intensity;
    }

    void setSSAO(bool enabled, float radius, float intensity) override {
        ssaoEnabled_ = enabled;
        ssaoRadius_ = radius;
        ssaoIntensity_ = intensity;
    }

    //==========================================================================
    // Statistics
    //==========================================================================

    RenderStats getStats() const override {
        return RenderStats{
            .drawCalls = static_cast<std::uint32_t>(meshDrawCalls_.size()),
            .triangles = 0,
            .vertices = 0,
            .meshes = static_cast<std::uint32_t>(meshes_.size()),
            .materials = static_cast<std::uint32_t>(materials_.size()),
            .textures = 0,
            .lights = static_cast<std::uint32_t>(pointLights_.size() + spotLights_.size()),
            .visibleObjects = static_cast<std::uint32_t>(meshDrawCalls_.size()),
            .culledObjects = 0,
            .frameTimeMs = 16.67f,  // Simulate 60fps
            .gpuTimeMs = 8.0f
        };
    }

    //==========================================================================
    // Instanced Rendering
    //==========================================================================

    Result<InstanceBufferHandle, Graphics3DError> createInstanceBuffer(
        std::uint32_t maxInstances, bool dynamic) override {
        InstanceBufferHandle handle = nextInstanceBufferHandle_++;
        instanceBuffers_.insert(handle);
        return handle;
    }

    Result<void, Graphics3DError> updateInstanceBuffer(
        InstanceBufferHandle buffer,
        std::span<const InstanceData> data,
        std::uint32_t offset) override {
        if (!instanceBuffers_.contains(buffer)) {
            return std::unexpected(Graphics3DError::InvalidMesh);
        }
        return {};
    }

    void destroyInstanceBuffer(InstanceBufferHandle buffer) override {
        instanceBuffers_.erase(buffer);
    }

    void drawInstanced(const InstancedRenderItem& item) override {
        // Track instanced draws
    }

    void queueInstancedRenderItem(const InstancedRenderItem& item) override {
        // Queue for batched instanced rendering
    }

    //==========================================================================
    // Skeletal Animation
    //==========================================================================

    Result<SkeletonHandle, Graphics3DError> createSkeleton(const ModelData& modelData) override {
        SkeletonHandle handle = nextSkeletonHandle_++;
        skeletons_.insert(handle);
        return handle;
    }

    Result<AnimationClipHandle, Graphics3DError> createAnimationClip(
        SkeletonHandle skeleton,
        const std::string& clipName,
        const ModelData& modelData) override {
        AnimationClipHandle handle = nextAnimationClipHandle_++;
        animationClips_[handle] = AnimationClip{clipName, 1.0f, true, 30.0f};
        return handle;
    }

    void destroySkeleton(SkeletonHandle skeleton) override {
        skeletons_.erase(skeleton);
    }

    void destroyAnimationClip(AnimationClipHandle clip) override {
        animationClips_.erase(clip);
    }

    std::vector<std::string> getAnimationClipNames(SkeletonHandle skeleton) const override {
        return {"idle", "walk", "run"};  // Mock clip names
    }

    AnimationClip getAnimationClipInfo(AnimationClipHandle clip) const override {
        auto it = animationClips_.find(clip);
        if (it != animationClips_.end()) {
            return it->second;
        }
        return AnimationClip{};
    }

    std::vector<Mat4> sampleAnimation(
        AnimationClipHandle clip, float time, bool loop) override {
        return std::vector<Mat4>(64, Mat4{});  // Return identity matrices
    }

    std::vector<Mat4> blendAnimations(const BlendedAnimation& blend) override {
        return std::vector<Mat4>(64, Mat4{});
    }

    void drawSkinnedMesh(
        MeshHandle mesh,
        MaterialHandle material,
        const Mat4& worldMatrix,
        std::span<const Mat4> boneTransforms) override {
        meshDrawCalls_.push_back({mesh, material, worldMatrix, true, true});
    }

    //==========================================================================
    // 3D Text Rendering
    //==========================================================================

    Result<Font3DHandle, Graphics3DError> loadFont3D(AssetHandle fontAsset) override {
        Font3DHandle handle = nextFont3DHandle_++;
        fonts3D_.insert(handle);
        return handle;
    }

    void destroyFont3D(Font3DHandle font) override {
        fonts3D_.erase(font);
    }

    void drawText3D(const Text3DItem& item) override {
        text3DDrawCount_++;
    }

    void drawText3D(
        const std::string& text,
        const Vec3& position,
        Font3DHandle font,
        float fontSize,
        const Color& color) override {
        text3DDrawCount_++;
    }

    AABB3D measureText3D(
        const std::string& text,
        Font3DHandle font,
        float fontSize) override {
        float width = static_cast<float>(text.length()) * fontSize * 0.5f;
        return AABB3D{Vec3{0}, Vec3{width, fontSize, 0.1f}};
    }

    //==========================================================================
    // Material Property Updates
    //==========================================================================

    Result<void, Graphics3DError> setMaterialBaseColor(
        MaterialHandle handle, const Vec4& color) override {
        if (!materials_.contains(handle)) {
            return std::unexpected(Graphics3DError::InvalidMaterial);
        }
        if (auto it = pbrMaterials_.find(handle); it != pbrMaterials_.end()) {
            it->second.baseColorFactor = color;
        }
        return {};
    }

    Result<void, Graphics3DError> setMaterialMetallicRoughness(
        MaterialHandle handle, float metallic, float roughness) override {
        if (!materials_.contains(handle)) {
            return std::unexpected(Graphics3DError::InvalidMaterial);
        }
        if (auto it = pbrMaterials_.find(handle); it != pbrMaterials_.end()) {
            it->second.metallicFactor = metallic;
            it->second.roughnessFactor = roughness;
        }
        return {};
    }

    Result<void, Graphics3DError> setMaterialEmissive(
        MaterialHandle handle, const Vec3& emissive) override {
        if (!materials_.contains(handle)) {
            return std::unexpected(Graphics3DError::InvalidMaterial);
        }
        if (auto it = pbrMaterials_.find(handle); it != pbrMaterials_.end()) {
            it->second.emissiveFactor = emissive;
        }
        return {};
    }

    std::optional<PBRMaterial> getMaterialProperties(MaterialHandle handle) const override {
        auto it = pbrMaterials_.find(handle);
        if (it != pbrMaterials_.end()) {
            return it->second;
        }
        return std::nullopt;
    }

    //==========================================================================
    // LOD (Level of Detail)
    //==========================================================================

    void setLODDistances(std::span<const float> distances) override {
        lodDistances_.assign(distances.begin(), distances.end());
    }

    void registerLODMeshes(
        MeshHandle primaryMesh,
        std::span<const MeshHandle> lodMeshes) override {
        lodMeshRegistry_[primaryMesh].assign(lodMeshes.begin(), lodMeshes.end());
    }

    void setLODBias(float bias) override {
        lodBias_ = bias;
    }

    //==========================================================================
    // Runtime Config
    //==========================================================================

    void applyRuntimeConfig(const Graphics3DRuntimeConfig& config) override {
        runtimeConfig_ = config;
    }

    const Graphics3DRuntimeConfig& getRuntimeConfig() const override {
        return runtimeConfig_;
    }

    bool reloadRuntimeConfig() override {
        return true;
    }

    bool loadRuntimeConfig(const std::filesystem::path& configPath) override {
        return true;  // Mock: always succeeds
    }

    //==========================================================================
    // Lock-On Targeting System
    //==========================================================================

    void setLockOnConfig(const LockOnConfig& config) override {
        lockOnConfig_ = config;
    }

    LockOnConfig getLockOnConfig() const override {
        return lockOnConfig_;
    }

    LockOnResult lockOn(IEntitySystem& entities, IAnimationSystem* animation) override {
        lockOnCalled_ = true;
        auto targets = getPotentialTargets(entities, animation);
        if (!targets.empty()) {
            // Select the best target (highest score)
            currentLockTarget_ = targets.front();
            isLocked_ = true;
            return currentLockTarget_;
        }
        return LockOnResult{};
    }

    std::optional<LockOnResult> getLockTarget() const override {
        if (isLocked_) {
            return currentLockTarget_;
        }
        return std::nullopt;
    }

    std::optional<Vec3> pollLockPosition(IEntitySystem& entities, IAnimationSystem* animation) override {
        pollLockPositionCalled_ = true;
        if (isLocked_) {
            // In mock, just return the stored position (real impl would query entity)
            return currentLockTarget_.worldPosition;
        }
        return std::nullopt;
    }

    LockOnResult shiftLockTarget(const Vec2& screenDirection, IEntitySystem& entities, IAnimationSystem* animation) override {
        shiftLockTargetCalled_ = true;
        lastShiftDirection_ = screenDirection;

        auto targets = getPotentialTargets(entities, animation);
        if (targets.size() > 1 && isLocked_) {
            // Simple mock: just cycle to next target
            for (std::size_t i = 0; i < targets.size(); ++i) {
                if (targets[i].entity == currentLockTarget_.entity) {
                    std::size_t nextIdx = (i + 1) % targets.size();
                    currentLockTarget_ = targets[nextIdx];
                    return currentLockTarget_;
                }
            }
        }
        return currentLockTarget_;
    }

    void unlock() override {
        unlockCalled_ = true;
        isLocked_ = false;
        currentLockTarget_ = LockOnResult{};
    }

    bool isLocked() const override {
        return isLocked_;
    }

    std::vector<LockOnResult> getPotentialTargets(IEntitySystem& entities, IAnimationSystem* animation) const override {
        getPotentialTargetsCalled_ = true;

        std::vector<LockOnResult> results;

        // Use the mock potential targets if set
        for (const auto& target : mockPotentialTargets_) {
            results.push_back(target);
        }

        return results;
    }

    // Test helpers for lock-on
    void setMockPotentialTargets(std::vector<LockOnResult> targets) {
        mockPotentialTargets_ = std::move(targets);
    }

    bool wasLockOnCalled() const { return lockOnCalled_; }
    bool wasPollLockPositionCalled() const { return pollLockPositionCalled_; }
    bool wasShiftLockTargetCalled() const { return shiftLockTargetCalled_; }
    bool wasUnlockCalled() const { return unlockCalled_; }
    mutable bool getPotentialTargetsCalled_ = false;
    bool wasGetPotentialTargetsCalled() const { return getPotentialTargetsCalled_; }
    Vec2 getLastShiftDirection() const { return lastShiftDirection_; }

    void resetLockOnTestState() {
        lockOnCalled_ = false;
        pollLockPositionCalled_ = false;
        shiftLockTargetCalled_ = false;
        unlockCalled_ = false;
        getPotentialTargetsCalled_ = false;
        isLocked_ = false;
        currentLockTarget_ = LockOnResult{};
        mockPotentialTargets_.clear();
        lastShiftDirection_ = Vec2{0, 0};
    }

    //==========================================================================
    // Additional Test Accessors
    //==========================================================================

    int getEntitiesRenderedCount() const { return entitiesRenderedCount_; }
    RenderLayer getLastMinLayer() const { return lastMinLayer_; }
    RenderLayer getLastMaxLayer() const { return lastMaxLayer_; }
    int getText3DDrawCount() const { return text3DDrawCount_; }

    std::size_t getMeshCount() const { return meshes_.size(); }
    std::size_t getMaterialCount() const { return materials_.size(); }
    std::size_t getPointLightCount() const { return pointLights_.size(); }
    std::size_t getSpotLightCount() const { return spotLights_.size(); }

private:
    // Resource tracking
    std::set<MeshHandle> meshes_;
    std::map<MeshHandle, AABB3D> meshBounds_;
    std::set<MaterialHandle> materials_;
    std::map<MaterialHandle, PBRMaterial> pbrMaterials_;
    std::set<InstanceBufferHandle> instanceBuffers_;
    std::set<SkeletonHandle> skeletons_;
    std::map<AnimationClipHandle, AnimationClip> animationClips_;
    std::set<Font3DHandle> fonts3D_;

    // Handle generators
    MeshHandle nextMeshHandle_ = 1;
    MaterialHandle nextMaterialHandle_ = 1;
    InstanceBufferHandle nextInstanceBufferHandle_ = 1;
    SkeletonHandle nextSkeletonHandle_ = 1;
    AnimationClipHandle nextAnimationClipHandle_ = 1;
    Font3DHandle nextFont3DHandle_ = 1;
    std::uint32_t nextLightId_ = 1;

    // Default materials
    MaterialHandle defaultPBRMaterial_ = 0;
    MaterialHandle defaultUnlitMaterial_ = 0;
    MaterialHandle errorMaterial_ = 0;

    // Camera and window state
    Camera3D camera_;
    Vec3 cameraTarget_{0.0f, 0.0f, 0.0f};
    Size windowSize_{800, 600};
    bool isFullscreen_ = false;
    bool shouldClose_ = false;
    Color clearColor_ = Color::black();
    bool vsyncEnabled_ = true;
    float renderScale_ = 1.0f;

    // Lighting
    DirectionalLight directionalLight_;
    bool hasDirectionalLight_ = false;
    std::map<std::uint32_t, std::pair<PointLight, Vec3>> pointLights_;
    std::map<std::uint32_t, std::pair<SpotLight, Vec3>> spotLights_;
    Vec3 ambientColor_{0.1f, 0.1f, 0.1f};
    float ambientIntensity_ = 1.0f;

    // Environment
    Skybox skybox_;
    bool hasSkybox_ = false;
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

    // LOD
    std::vector<float> lodDistances_;
    std::map<MeshHandle, std::vector<MeshHandle>> lodMeshRegistry_;
    float lodBias_ = 1.0f;

    // Runtime Config
    Graphics3DRuntimeConfig runtimeConfig_;

    // Lock-On System
    LockOnConfig lockOnConfig_;
    LockOnResult currentLockTarget_;
    bool isLocked_ = false;
    std::vector<LockOnResult> mockPotentialTargets_;
    bool lockOnCalled_ = false;
    bool pollLockPositionCalled_ = false;
    bool shiftLockTargetCalled_ = false;
    bool unlockCalled_ = false;
    Vec2 lastShiftDirection_{0, 0};

    // Systems
    IAssetSystem* assetSystem_ = nullptr;

    // Initialization state
    bool initialized_ = false;

    // Frame tracking
    int frameCount_ = 0;
    bool beginFrameCalled_ = false;
    bool endFrameCalled_ = false;

    // Entity rendering tracking
    int entitiesRenderedCount_ = 0;
    RenderLayer lastMinLayer_ = 0;
    RenderLayer lastMaxLayer_ = 0;

    // Text rendering tracking
    int text3DDrawCount_ = 0;

    // Draw call recording
    std::vector<DrawMeshCall> meshDrawCalls_;
    std::vector<DebugLineCall> debugLineCalls_;
    std::vector<RenderItem> renderItemQueue_;
};

}  // namespace bestow::tests

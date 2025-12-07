// bestow-contract/src/bestow.graphics3d.cppm
// 3D Graphics system interface

module;

#include <span>
#include <string>
#include <functional>
#include <optional>
#include <any>

export module bestow.graphics3d;

import bestow.types;
import bestow.assets;
import bestow.entity;
import bestow.shader;

export namespace bestow {

// All 3D math types (Vec4, Mat4, Quat, Transform3D, AABB3D, Ray3D, Frustum, etc.)
// are imported from bestow.types
// All graphics types (Vertex3D, Camera3D, BlendMode, CullMode, Fog, etc.)
// are imported from bestow.types

//==========================================================================
// Graphics-Specific Types (not in bestow.types)
//==========================================================================

using MeshHandle = std::uint64_t;
using MaterialHandle = std::uint64_t;

struct SubMesh {
    std::uint32_t indexOffset = 0;
    std::uint32_t indexCount = 0;
    std::uint32_t materialIndex = 0;
    AABB3D bounds;
};

struct MeshDef {
    std::span<const Vertex3D> vertices;
    std::span<const std::uint32_t> indices;
    std::vector<SubMesh> subMeshes;
    AABB3D bounds;
    bool isDynamic = false;  // Can vertices be updated after creation?
};

//==========================================================================
// Material Types
//==========================================================================

struct PBRMaterial {
    Vec4 baseColorFactor{1.0f, 1.0f, 1.0f, 1.0f};
    AssetHandle baseColorTexture;

    float metallicFactor = 0.0f;
    float roughnessFactor = 1.0f;
    AssetHandle metallicRoughnessTexture;

    AssetHandle normalTexture;
    float normalScale = 1.0f;

    AssetHandle occlusionTexture;
    float occlusionStrength = 1.0f;

    Vec3 emissiveFactor{0.0f};
    AssetHandle emissiveTexture;

    BlendMode blendMode = BlendMode::Opaque;
    CullMode cullMode = CullMode::Back;
    float alphaCutoff = 0.5f;
    bool doubleSided = false;
    bool receiveShadows = true;
    bool castShadows = true;
};

struct UnlitMaterial {
    Vec4 color{1.0f, 1.0f, 1.0f, 1.0f};
    AssetHandle texture;
    BlendMode blendMode = BlendMode::Opaque;
    CullMode cullMode = CullMode::Back;
};

//==========================================================================
// Lighting Types
//==========================================================================

struct DirectionalLight {
    Vec3 direction{0.0f, -1.0f, 0.0f};
    Vec3 color{1.0f, 1.0f, 1.0f};
    float intensity = 1.0f;
    bool castShadows = true;
    int shadowMapResolution = 2048;
};

struct PointLight {
    Vec3 color{1.0f, 1.0f, 1.0f};
    float intensity = 1.0f;
    float range = 10.0f;
    bool castShadows = false;
};

struct SpotLight {
    Vec3 direction{0.0f, -1.0f, 0.0f};
    Vec3 color{1.0f, 1.0f, 1.0f};
    float intensity = 1.0f;
    float range = 10.0f;
    float innerConeAngle = 0.4f;  // Radians
    float outerConeAngle = 0.5f;  // Radians
    bool castShadows = false;
};

//==========================================================================
// Environment Types
//==========================================================================

struct Skybox {
    AssetHandle cubemapTexture;
    float rotation = 0.0f;  // Radians
    float exposure = 1.0f;
};

struct EnvironmentMap {
    AssetHandle irradianceMap;      // For diffuse IBL
    AssetHandle prefilteredMap;     // For specular IBL
    AssetHandle brdfLUT;            // BRDF lookup texture
    float intensity = 1.0f;
};

//==========================================================================
// Render Item (for batching)
//==========================================================================

struct RenderItem {
    MeshHandle mesh;
    std::uint32_t subMeshIndex = 0;
    MaterialHandle material;
    Mat4 worldMatrix;
    AABB3D worldBounds;
    RenderLayer layer = 0;
    bool castShadow = true;
    bool receiveShadow = true;
};

//==========================================================================
// Instanced Rendering
//==========================================================================

using InstanceBufferHandle = std::uint64_t;

struct InstanceData {
    Mat4 worldMatrix;
    Vec4 customData{0.0f};  // User-defined per-instance data (e.g., color, ID)
};

struct InstancedRenderItem {
    MeshHandle mesh;
    MaterialHandle material;
    InstanceBufferHandle instances;
    std::uint32_t instanceCount = 0;
    RenderLayer layer = 0;
    bool castShadow = true;
};

//==========================================================================
// Skeletal Animation
//==========================================================================

using SkeletonHandle = std::uint64_t;
using AnimationHandle = std::uint64_t;
using AnimationClipHandle = std::uint64_t;

struct BoneTransform {
    Vec3 translation{0.0f};
    Quat rotation{1.0f, 0.0f, 0.0f, 0.0f};
    Vec3 scale{1.0f};
};

struct AnimationClip {
    std::string name;
    float duration = 0.0f;
    bool looping = true;
    float ticksPerSecond = 30.0f;
};

struct AnimationState {
    AnimationClipHandle clip;
    float time = 0.0f;
    float speed = 1.0f;
    float weight = 1.0f;
    bool playing = false;
    bool looping = true;
};

struct BlendedAnimation {
    std::vector<AnimationState> layers;
    float crossfadeDuration = 0.25f;
};

//==========================================================================
// 3D Text Rendering
//==========================================================================

using Font3DHandle = std::uint64_t;

enum class TextAlignment3D : std::uint8_t {
    Left,
    Center,
    Right
};

enum class TextVerticalAlign3D : std::uint8_t {
    Top,
    Middle,
    Bottom
};

struct Text3DStyle {
    Font3DHandle font;
    float fontSize = 1.0f;
    Color color = Color::white();
    TextAlignment3D alignment = TextAlignment3D::Center;
    TextVerticalAlign3D verticalAlign = TextVerticalAlign3D::Middle;
    float lineSpacing = 1.2f;
    float letterSpacing = 0.0f;
    bool billboard = false;  // Always face camera
    float outlineWidth = 0.0f;
    Color outlineColor = Color::black();
};

struct Text3DItem {
    std::string text;
    Text3DStyle style;
    Transform3D transform;
    RenderLayer layer = 0;
};

//==========================================================================
// Error Types
//==========================================================================

enum class Graphics3DError {
    Success,
    InvalidMesh,
    InvalidMaterial,
    InvalidTexture,
    InvalidShader,
    ShaderCompilationFailed,
    OutOfMemory,
    ContextLost,
    InternalError
};

//==========================================================================
// Statistics
//==========================================================================

struct RenderStats {
    std::uint32_t drawCalls = 0;
    std::uint32_t triangles = 0;
    std::uint32_t vertices = 0;
    std::uint32_t meshes = 0;
    std::uint32_t materials = 0;
    std::uint32_t textures = 0;
    std::uint32_t lights = 0;
    std::uint32_t visibleObjects = 0;
    std::uint32_t culledObjects = 0;
    float frameTimeMs = 0.0f;
    float gpuTimeMs = 0.0f;
};

//==========================================================================
// IGraphics3DSystem Interface
//==========================================================================

class IGraphics3DSystem {
public:
    virtual ~IGraphics3DSystem() = default;

    //======================================================================
    // Frame Lifecycle
    //======================================================================

    virtual void beginFrame() = 0;
    virtual void endFrame() = 0;

    //======================================================================
    // Mesh Management
    //======================================================================

    virtual Result<MeshHandle, Graphics3DError> createMesh(const MeshDef& def) = 0;
    virtual void destroyMesh(MeshHandle handle) = 0;
    virtual bool hasMesh(MeshHandle handle) const = 0;
    virtual AABB3D getMeshBounds(MeshHandle handle) const = 0;

    /// Update dynamic mesh vertices (mesh must be created with isDynamic = true)
    virtual Result<void, Graphics3DError> updateMeshVertices(
        MeshHandle handle,
        std::span<const Vertex3D> vertices,
        std::uint32_t offset = 0) = 0;

    //======================================================================
    // Primitive Mesh Generation
    //======================================================================

    virtual Result<MeshHandle, Graphics3DError> createCubeMesh(float size = 1.0f) = 0;

    virtual Result<MeshHandle, Graphics3DError> createSphereMesh(
        float radius = 0.5f,
        std::uint32_t segments = 32,
        std::uint32_t rings = 16) = 0;

    virtual Result<MeshHandle, Graphics3DError> createCylinderMesh(
        float radius = 0.5f,
        float height = 1.0f,
        std::uint32_t segments = 32) = 0;

    virtual Result<MeshHandle, Graphics3DError> createCapsuleMesh(
        float radius = 0.5f,
        float height = 1.0f,
        std::uint32_t segments = 32,
        std::uint32_t rings = 8) = 0;

    virtual Result<MeshHandle, Graphics3DError> createPlaneMesh(
        float width = 1.0f,
        float height = 1.0f,
        std::uint32_t widthSegments = 1,
        std::uint32_t heightSegments = 1) = 0;

    //======================================================================
    // Material Management
    //======================================================================

    virtual Result<MaterialHandle, Graphics3DError> createMaterial(const PBRMaterial& mat) = 0;
    virtual Result<MaterialHandle, Graphics3DError> createUnlitMaterial(const UnlitMaterial& mat) = 0;
    virtual void destroyMaterial(MaterialHandle handle) = 0;
    virtual bool hasMaterial(MaterialHandle handle) const = 0;

    virtual Result<void, Graphics3DError> setMaterialTexture(
        MaterialHandle handle,
        std::uint32_t slot,
        AssetHandle texture) = 0;

    /// Get pre-created default materials
    virtual MaterialHandle getDefaultPBRMaterial() const = 0;
    virtual MaterialHandle getDefaultUnlitMaterial() const = 0;
    virtual MaterialHandle getErrorMaterial() const = 0;  // Checkerboard for missing textures

    //======================================================================
    // Immediate Mode Rendering
    //======================================================================

    virtual void drawMesh(
        MeshHandle mesh,
        MaterialHandle material,
        const Mat4& worldMatrix,
        bool castShadow = true,
        bool receiveShadow = true) = 0;

    virtual void drawMesh(
        MeshHandle mesh,
        MaterialHandle material,
        const Transform3D& transform,
        bool castShadow = true,
        bool receiveShadow = true) = 0;

    //======================================================================
    // Batch Rendering
    //======================================================================

    virtual void queueRenderItem(const RenderItem& item) = 0;
    virtual void queueRenderItems(std::span<const RenderItem> items) = 0;
    virtual void flushRenderQueue() = 0;

    //======================================================================
    // Entity Rendering (ECS)
    //======================================================================

    /// Render all entities with 3D visual components (Mesh3D, Transform3D)
    virtual void renderEntities(IEntitySystem& entities) = 0;

    /// Render entities visible within the camera frustum
    virtual void renderEntities(IEntitySystem& entities, const Frustum& frustum) = 0;

    /// Render entities within a specific layer range
    virtual void renderEntities(
        IEntitySystem& entities,
        RenderLayer minLayer,
        RenderLayer maxLayer) = 0;

    //======================================================================
    // Camera
    //======================================================================

    virtual void setCamera(const Camera3D& camera) = 0;
    virtual Camera3D getCamera() const = 0;

    /// Convert screen position (pixels) to world ray
    virtual Ray3D screenToWorldRay(Vec2 screenPos) const = 0;

    /// Convert world position to screen position (pixels)
    /// Returns nullopt if point is behind camera
    virtual std::optional<Vec2> worldToScreen(const Vec3& worldPos) const = 0;

    //======================================================================
    // Lighting
    //======================================================================

    virtual void setDirectionalLight(const DirectionalLight& light) = 0;
    virtual void clearDirectionalLight() = 0;

    virtual std::uint32_t addPointLight(const PointLight& light, const Vec3& position) = 0;
    virtual std::uint32_t addSpotLight(const SpotLight& light, const Vec3& position) = 0;

    virtual void setLightPosition(std::uint32_t lightId, const Vec3& position) = 0;
    virtual void removeLight(std::uint32_t lightId) = 0;
    virtual void clearLights() = 0;

    virtual void setAmbientLight(const Vec3& color, float intensity = 1.0f) = 0;

    /// Automatically update lights from entities with light components
    virtual void updateEntityLights(IEntitySystem& entities) = 0;

    //======================================================================
    // Environment
    //======================================================================

    virtual void setSkybox(const Skybox& skybox) = 0;
    virtual void clearSkybox() = 0;

    virtual void setEnvironmentMap(const EnvironmentMap& envMap) = 0;
    virtual void clearEnvironmentMap() = 0;

    virtual void setFog(const Fog& fog) = 0;

    //======================================================================
    // Shadows
    //======================================================================

    virtual void setShadowsEnabled(bool enabled) = 0;
    virtual bool areShadowsEnabled() const = 0;
    virtual void setDirectionalShadowResolution(int resolution) = 0;
    virtual void setShadowDistance(float distance) = 0;

    //======================================================================
    // Debug Rendering
    //======================================================================

    virtual void debugDrawLine(
        const Vec3& start,
        const Vec3& end,
        const Color& color = Color::white(),
        float duration = 0.0f,
        bool depthTest = true) = 0;

    virtual void debugDrawBox(
        const Vec3& center,
        const Vec3& halfExtents,
        const Quat& rotation = Quat{1.0f, 0.0f, 0.0f, 0.0f},
        const Color& color = Color::white(),
        float duration = 0.0f,
        bool depthTest = true) = 0;

    virtual void debugDrawSphere(
        const Vec3& center,
        float radius,
        const Color& color = Color::white(),
        float duration = 0.0f,
        bool depthTest = true) = 0;

    virtual void debugDrawCapsule(
        const Vec3& start,
        const Vec3& end,
        float radius,
        const Color& color = Color::white(),
        float duration = 0.0f,
        bool depthTest = true) = 0;

    virtual void debugDrawFrustum(
        const Frustum& frustum,
        const Color& color = Color::white(),
        float duration = 0.0f,
        bool depthTest = true) = 0;

    virtual void debugDrawRay(
        const Vec3& origin,
        const Vec3& direction,
        float length,
        const Color& color = Color::white(),
        float duration = 0.0f,
        bool depthTest = true) = 0;

    virtual void debugDrawAxes(
        const Transform3D& transform,
        float size = 1.0f,
        float duration = 0.0f,
        bool depthTest = true) = 0;

    virtual void debugDrawAABB(
        const AABB3D& aabb,
        const Color& color = Color::white(),
        float duration = 0.0f,
        bool depthTest = true) = 0;

    virtual void debugClear() = 0;
    virtual void setDebugRenderingEnabled(bool enabled) = 0;
    virtual bool isDebugRenderingEnabled() const = 0;

    //======================================================================
    // Window Management
    //======================================================================

    virtual Size getWindowSize() const = 0;
    virtual void setWindowSize(Size size) = 0;
    virtual bool isFullscreen() const = 0;
    virtual void setFullscreen(bool fullscreen) = 0;
    virtual bool shouldClose() const = 0;
    virtual void* getNativeWindowHandle() const = 0;

    //======================================================================
    // Render State
    //======================================================================

    virtual void setClearColor(const Color& color) = 0;
    virtual void setVSync(bool enabled) = 0;
    virtual void setRenderScale(float scale) = 0;
    virtual float getRenderScale() const = 0;

    //======================================================================
    // Shader System Integration
    //======================================================================

    /// Set shader system for custom shader/material support
    virtual void setShaderSystem(IShaderSystem* shaders) = 0;

    /// Get the shader system (for direct access to shader features)
    virtual IShaderSystem* getShaderSystem() const = 0;

    /// Draw mesh with a shader system material (from Lua or custom shaders)
    virtual void drawMeshWithShaderMaterial(
        MeshHandle mesh,
        ShaderProgramHandle shader,
        const Mat4& worldMatrix,
        bool castShadow = true,
        bool receiveShadow = true) = 0;

    /// Convenience: Draw mesh with Lua material file
    virtual Result<void, Graphics3DError> drawMeshWithLuaMaterial(
        MeshHandle mesh,
        std::string_view materialPath,
        const Mat4& worldMatrix) = 0;

    /// Draw mesh with Lua material and per-object color override
    virtual Result<void, Graphics3DError> drawMeshWithLuaMaterial(
        MeshHandle mesh,
        std::string_view materialPath,
        const Mat4& worldMatrix,
        const Vec4& colorOverride) = 0;

    /// Update shader system (call each frame for hot reload)
    virtual void updateShaders() = 0;

    //======================================================================
    // Asset System Integration
    //======================================================================

    virtual void setAssetSystem(IAssetSystem* assets) = 0;

    /// Create GPU mesh from asset system's MeshData
    /// Use: assets->getMeshData(handle) to get MeshData, then call this
    virtual Result<MeshHandle, Graphics3DError> createMeshFromData(const MeshData& data) = 0;

    /// Create GPU material from asset system's MaterialData
    virtual Result<MaterialHandle, Graphics3DError> createMaterialFromData(const MaterialData& data) = 0;

    /// Create multiple materials from ModelData (one per material in the model)
    virtual Result<std::vector<MaterialHandle>, Graphics3DError> createMaterialsFromModel(const ModelData& data) = 0;

    /// Create skybox from asset system's CubemapData
    virtual Result<void, Graphics3DError> createSkyboxFromData(const CubemapData& data) = 0;

    //======================================================================
    // Culling
    //======================================================================

    virtual void setFrustumCulling(bool enabled) = 0;
    virtual bool isFrustumCullingEnabled() const = 0;

    //======================================================================
    // Post-Processing
    //======================================================================

    virtual void setToneMapping(bool enabled) = 0;
    virtual void setExposure(float exposure) = 0;
    virtual void setBloom(bool enabled, float threshold = 1.0f, float intensity = 1.0f) = 0;
    virtual void setSSAO(bool enabled, float radius = 0.5f, float intensity = 1.0f) = 0;

    //======================================================================
    // Statistics
    //======================================================================

    virtual RenderStats getStats() const = 0;

    //======================================================================
    // Instanced Rendering
    //======================================================================

    /// Create a buffer for instance data (transforms, custom data)
    virtual Result<InstanceBufferHandle, Graphics3DError> createInstanceBuffer(
        std::uint32_t maxInstances,
        bool dynamic = true) = 0;

    /// Update instance buffer data
    virtual Result<void, Graphics3DError> updateInstanceBuffer(
        InstanceBufferHandle buffer,
        std::span<const InstanceData> data,
        std::uint32_t offset = 0) = 0;

    /// Destroy instance buffer
    virtual void destroyInstanceBuffer(InstanceBufferHandle buffer) = 0;

    /// Draw mesh instanced
    virtual void drawInstanced(const InstancedRenderItem& item) = 0;

    /// Queue instanced render items for batched rendering
    virtual void queueInstancedRenderItem(const InstancedRenderItem& item) = 0;

    //======================================================================
    // Skeletal Animation
    //======================================================================

    /// Create skeleton from model data
    virtual Result<SkeletonHandle, Graphics3DError> createSkeleton(const ModelData& modelData) = 0;

    /// Create animation clip from model data
    virtual Result<AnimationClipHandle, Graphics3DError> createAnimationClip(
        SkeletonHandle skeleton,
        const std::string& clipName,
        const ModelData& modelData) = 0;

    /// Destroy skeleton and associated data
    virtual void destroySkeleton(SkeletonHandle skeleton) = 0;

    /// Destroy animation clip
    virtual void destroyAnimationClip(AnimationClipHandle clip) = 0;

    /// Get available animation clips for a skeleton
    virtual std::vector<std::string> getAnimationClipNames(SkeletonHandle skeleton) const = 0;

    /// Get animation clip info
    virtual AnimationClip getAnimationClipInfo(AnimationClipHandle clip) const = 0;

    /// Sample animation at specific time, get bone transforms
    virtual std::vector<Mat4> sampleAnimation(
        AnimationClipHandle clip,
        float time,
        bool loop = true) = 0;

    /// Blend multiple animations together
    virtual std::vector<Mat4> blendAnimations(
        const BlendedAnimation& blend) = 0;

    /// Draw skinned mesh with bone transforms
    virtual void drawSkinnedMesh(
        MeshHandle mesh,
        MaterialHandle material,
        const Mat4& worldMatrix,
        std::span<const Mat4> boneTransforms) = 0;

    //======================================================================
    // 3D Text Rendering
    //======================================================================

    /// Load font for 3D text rendering (SDF font atlas)
    virtual Result<Font3DHandle, Graphics3DError> loadFont3D(AssetHandle fontAsset) = 0;

    /// Destroy 3D font
    virtual void destroyFont3D(Font3DHandle font) = 0;

    /// Draw 3D text at world position
    virtual void drawText3D(const Text3DItem& item) = 0;

    /// Draw 3D text (simplified version)
    virtual void drawText3D(
        const std::string& text,
        const Vec3& position,
        Font3DHandle font,
        float fontSize = 1.0f,
        const Color& color = Color::white()) = 0;

    /// Measure text bounds in world units
    virtual AABB3D measureText3D(
        const std::string& text,
        Font3DHandle font,
        float fontSize = 1.0f) = 0;

    //======================================================================
    // Material Property Updates
    //======================================================================

    /// Update PBR material properties at runtime
    virtual Result<void, Graphics3DError> setMaterialBaseColor(
        MaterialHandle handle,
        const Vec4& color) = 0;

    virtual Result<void, Graphics3DError> setMaterialMetallicRoughness(
        MaterialHandle handle,
        float metallic,
        float roughness) = 0;

    virtual Result<void, Graphics3DError> setMaterialEmissive(
        MaterialHandle handle,
        const Vec3& emissive) = 0;

    /// Get current material properties
    virtual std::optional<PBRMaterial> getMaterialProperties(MaterialHandle handle) const = 0;

    //======================================================================
    // LOD (Level of Detail)
    //======================================================================

    /// Set LOD distances for automatic mesh switching
    virtual void setLODDistances(std::span<const float> distances) = 0;

    /// Register LOD meshes for a primary mesh
    virtual void registerLODMeshes(
        MeshHandle primaryMesh,
        std::span<const MeshHandle> lodMeshes) = 0;

    /// Set current LOD bias (< 1.0 = lower detail, > 1.0 = higher detail)
    virtual void setLODBias(float bias) = 0;
};

}  // namespace bestow

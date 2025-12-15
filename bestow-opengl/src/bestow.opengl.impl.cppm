// bestow-opengl/src/bestow.opengl.impl.cppm
// OpenGL rendering backend - 2D and 3D Graphics system implementations

module;

#include <kangaru/kangaru.hpp>
#include <bestow/kangaru_macros.hpp>

// OpenGL and windowing headers - MUST be in global module fragment
#include <glad/glad.h>
#include <GLFW/glfw3.h>

// Enable GLM experimental extensions before including them
#define GLM_ENABLE_EXPERIMENTAL

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/transform.hpp>

// Lua/sol2 for runtime configuration
#include <bestow/sol2_compat.hpp>

export module bestow.opengl.impl;

import std;
import bestow.services;  // Re-exports all contracts including bestow.graphics, bestow.assets, etc.

export namespace bestow {

//==========================================================================
// 2D Graphics - Font and Text Support Structures
//==========================================================================

// Font glyph metrics
struct GlyphInfo {
    float advanceX;      // Horizontal advance
    float bearingX;      // Horizontal bearing (offset from cursor)
    float bearingY;      // Vertical bearing (offset from baseline)
    float width;         // Glyph width
    float height;        // Glyph height
    float texCoordX;     // Texture coordinate X (normalized)
    float texCoordY;     // Texture coordinate Y (normalized)
    float texCoordW;     // Texture coordinate width (normalized)
    float texCoordH;     // Texture coordinate height (normalized)
};

// Font atlas data
struct FontAtlas {
    GLuint textureID = 0;
    int atlasWidth = 0;
    int atlasHeight = 0;
    float fontSize = 0.0f;
    GlyphInfo glyphs[256] = {};  // ASCII characters
    float lineHeight = 0.0f;
    AssetHandle fontHandle;
};

BESTOW_SYSTEM(OpenGLGraphicsSystem, IGraphicsSystem, IAssetSystem) {
public:
    ~OpenGLGraphicsSystem() override;

    bool initialize(int width, int height, const std::string& title);

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

private:
    GLFWwindow* window_ = nullptr;
    Camera camera_;
    Color clearColor_ = Color::black();
    bool isFullscreen_ = false;
    bool viewportCullingEnabled_ = false;
    int windowedWidth_ = 0;
    int windowedHeight_ = 0;
    int windowedX_ = 0;
    int windowedY_ = 0;
    std::vector<Sprite> spriteBatch_;

    // OpenGL rendering resources
    GLuint spriteShaderProgram_ = 0;
    GLuint spriteVAO_ = 0;
    GLuint spriteVBO_ = 0;
    GLuint whiteTexture_ = 0;

    // Debug primitive rendering resources
    GLuint primitiveShaderProgram_ = 0;
    GLuint primitiveVAO_ = 0;
    GLuint primitiveVBO_ = 0;

    // Text rendering resources
    GLuint textShaderProgram_ = 0;
    GLuint textVAO_ = 0;
    GLuint textVBO_ = 0;
    mutable std::unordered_map<AssetHandle, FontAtlas, AssetHandleHash> fontAtlases_;
    FontAtlas defaultFontAtlas_;

    // Texture cache (AssetHandle -> OpenGL texture ID)
    std::unordered_map<AssetHandle, GLuint, AssetHandleHash> textureCache_;

    // Shader compilation helpers
    bool compileShader(GLuint shader, const char* source);
    bool linkProgram(GLuint program);
    GLuint createShaderProgram(const char* vertSource, const char* fragSource);
    void createWhiteTexture();
    void createPrimitiveResources();

    // Text rendering helpers
    void createDefaultFont();
    void createTextResources();
    const FontAtlas& getFontAtlas(AssetHandle fontHandle, float size) const;

    // Texture helpers
    GLuint getOrUploadTexture(AssetHandle handle);
};


//==========================================================================
// 3D Graphics - Resource Structures and Implementation
//==========================================================================

//==========================================================================
// Internal Resource Structures
//==========================================================================

struct MeshResource {
    GLuint vao = 0;
    GLuint vbo = 0;
    GLuint ebo = 0;
    std::vector<SubMesh> subMeshes;
    AABB3D bounds;
    uint32_t vertexCount = 0;
    uint32_t indexCount = 0;
    bool isDynamic = false;
};

struct MaterialResource {
    // PBR parameters
    Vec4 baseColor{1.0f, 1.0f, 1.0f, 1.0f};
    float metallic = 0.0f;
    float roughness = 1.0f;
    Vec3 emissive{0.0f};

    // Textures (OpenGL texture IDs)
    GLuint baseColorTex = 0;
    GLuint normalTex = 0;
    GLuint metallicRoughnessTex = 0;
    GLuint occlusionTex = 0;
    GLuint emissiveTex = 0;

    // Render state
    BlendMode blendMode = BlendMode::Opaque;
    CullMode cullMode = CullMode::Back;
    float alphaCutoff = 0.5f;
    bool doubleSided = false;
    bool receiveShadows = true;
    bool castShadows = true;

    // Material type
    bool isUnlit = false;
};

struct ShaderProgram {
    GLuint program = 0;
    std::unordered_map<std::string, GLint> uniformLocations;

    void use() const {
        glUseProgram(program);
    }

    GLint getUniformLocation(const std::string& name) {
        auto it = uniformLocations.find(name);
        if (it != uniformLocations.end()) {
            return it->second;
        }
        GLint loc = glGetUniformLocation(program, name.c_str());
        uniformLocations[name] = loc;
        return loc;
    }

    void setMat4(const std::string& name, const glm::mat4& mat) {
        GLint loc = getUniformLocation(name);
        if (loc != -1) {
            glUniformMatrix4fv(loc, 1, GL_FALSE, glm::value_ptr(mat));
        }
    }

    void setMat3(const std::string& name, const glm::mat3& mat) {
        GLint loc = getUniformLocation(name);
        if (loc != -1) {
            glUniformMatrix3fv(loc, 1, GL_FALSE, glm::value_ptr(mat));
        }
    }

    void setVec3(const std::string& name, const glm::vec3& v) {
        GLint loc = getUniformLocation(name);
        if (loc != -1) {
            glUniform3fv(loc, 1, glm::value_ptr(v));
        }
    }

    void setVec4(const std::string& name, const glm::vec4& v) {
        GLint loc = getUniformLocation(name);
        if (loc != -1) {
            glUniform4fv(loc, 1, glm::value_ptr(v));
        }
    }

    void setFloat(const std::string& name, float v) {
        GLint loc = getUniformLocation(name);
        if (loc != -1) {
            glUniform1f(loc, v);
        }
    }

    void setInt(const std::string& name, int v) {
        GLint loc = getUniformLocation(name);
        if (loc != -1) {
            glUniform1i(loc, v);
        }
    }
};

// Internal debug line with duration/depthTest for persistent lines
struct DebugLineInternal {
    Vec3 start;
    Vec3 end;
    Color color;
    float duration;
    bool depthTest;
};

struct LightData {
    uint32_t id;
    enum class Type { Point, Spot } type;
    Vec3 position;
    Vec3 direction;  // For spotlights
    Vec3 color;
    float intensity;
    float range;
    float innerCone;  // For spotlights
    float outerCone;  // For spotlights
    bool castShadows;
};

struct InstanceBufferResource {
    GLuint vbo = 0;
    uint32_t maxInstances = 0;
    uint32_t currentCount = 0;
    bool dynamic = true;
};

struct BoneData {
    std::string name;
    int32_t parentIndex = -1;
    Mat4 offsetMatrix{1.0f};  // Bind pose inverse
    Mat4 localTransform{1.0f};
};

struct SkeletonResource {
    std::vector<BoneData> bones;
    std::unordered_map<std::string, int32_t> boneNameToIndex;
    std::vector<AnimationClipHandle> clips;
};

struct AnimationKeyframe {
    float time;
    Vec3 translation;
    Quat rotation;
    Vec3 scale;
};

struct BoneAnimationChannel {
    int32_t boneIndex;
    std::vector<AnimationKeyframe> keyframes;
};

struct AnimationClipResource {
    std::string name;
    float duration = 0.0f;
    float ticksPerSecond = 30.0f;
    bool looping = true;
    SkeletonHandle skeleton;
    std::vector<BoneAnimationChannel> channels;
};

struct Font3DGlyph {
    Vec2 atlasOffset;    // UV offset in atlas
    Vec2 atlasSize;      // UV size in atlas
    Vec2 size;           // Size in world units at fontSize=1
    Vec2 bearing;        // Offset from baseline
    float advance;       // Horizontal advance
};

struct Font3DResource {
    GLuint atlasTexture = 0;
    uint32_t atlasWidth = 0;
    uint32_t atlasHeight = 0;
    std::unordered_map<uint32_t, Font3DGlyph> glyphs;  // Codepoint -> glyph
    float lineHeight = 1.0f;
    float ascender = 0.0f;
    float descender = 0.0f;
};

struct LODConfig {
    std::vector<float> distances;
    float bias = 0.0f;
};

struct LODMeshGroup {
    MeshHandle primaryMesh;
    std::vector<MeshHandle> lodMeshes;  // Index 0 = LOD1, etc.
};

//==========================================================================
// Embedded Shaders
//==========================================================================

const char* PBR_VERTEX_SHADER = R"(
#version 410 core
layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aTexCoord;
layout(location = 3) in vec4 aColor;

uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uProjection;
uniform mat3 uNormalMatrix;

out vec3 vWorldPos;
out vec3 vNormal;
out vec2 vTexCoord;
out vec4 vColor;

void main() {
    vec4 worldPos = uModel * vec4(aPosition, 1.0);
    vWorldPos = worldPos.xyz;
    vNormal = uNormalMatrix * aNormal;
    vTexCoord = aTexCoord;
    vColor = aColor;

    gl_Position = uProjection * uView * worldPos;
}
)";

const char* PBR_FRAGMENT_SHADER = R"(
#version 410 core
in vec3 vWorldPos;
in vec3 vNormal;
in vec2 vTexCoord;
in vec4 vColor;

uniform vec4 uBaseColor;
uniform float uMetallic;
uniform float uRoughness;
uniform vec3 uEmissive;
uniform vec3 uCameraPos;
uniform vec3 uLightDir;
uniform vec3 uLightColor;
uniform vec3 uAmbient;

uniform bool uHasBaseColorTex;
uniform sampler2D uBaseColorTex;

out vec4 FragColor;

const float PI = 3.14159265359;

void main() {
    // Sample base color
    vec4 baseColor = uBaseColor * vColor;
    if (uHasBaseColorTex) {
        baseColor *= texture(uBaseColorTex, vTexCoord);
    }

    vec3 N = normalize(vNormal);
    vec3 L = normalize(-uLightDir);
    vec3 V = normalize(uCameraPos - vWorldPos);
    vec3 H = normalize(L + V);

    float NdotL = max(dot(N, L), 0.0);
    float NdotV = max(dot(N, V), 0.0);
    float NdotH = max(dot(N, H), 0.0);
    float VdotH = max(dot(V, H), 0.0);

    // Simplified PBR (Schlick approximation for Fresnel)
    vec3 F0 = mix(vec3(0.04), baseColor.rgb, uMetallic);
    vec3 F = F0 + (1.0 - F0) * pow(1.0 - VdotH, 5.0);

    // GGX normal distribution
    float roughness = max(uRoughness, 0.04);
    float a = roughness * roughness;
    float a2 = a * a;
    float denom = NdotH * NdotH * (a2 - 1.0) + 1.0;
    float D = a2 / (PI * denom * denom);

    // Schlick-GGX geometry term
    float k = (roughness + 1.0) * (roughness + 1.0) / 8.0;
    float GL = NdotL / (NdotL * (1.0 - k) + k);
    float GV = NdotV / (NdotV * (1.0 - k) + k);
    float G = GL * GV;

    // Cook-Torrance BRDF
    vec3 specular = (D * F * G) / max(4.0 * NdotL * NdotV, 0.001);

    // Diffuse (Lambertian)
    vec3 kD = (1.0 - F) * (1.0 - uMetallic);
    vec3 diffuse = kD * baseColor.rgb / PI;

    // Final color
    vec3 color = (diffuse + specular) * uLightColor * NdotL + uAmbient * baseColor.rgb + uEmissive;

    FragColor = vec4(color, baseColor.a);
}
)";

const char* UNLIT_VERTEX_SHADER = R"(
#version 410 core
layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aTexCoord;
layout(location = 3) in vec4 aColor;

uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uProjection;

out vec2 vTexCoord;
out vec4 vColor;

void main() {
    vTexCoord = aTexCoord;
    vColor = aColor;
    gl_Position = uProjection * uView * uModel * vec4(aPosition, 1.0);
}
)";

const char* UNLIT_FRAGMENT_SHADER = R"(
#version 410 core
in vec2 vTexCoord;
in vec4 vColor;

uniform vec4 uColor;
uniform bool uHasTexture;
uniform sampler2D uTexture;

out vec4 FragColor;

void main() {
    vec4 color = uColor * vColor;
    if (uHasTexture) {
        color *= texture(uTexture, vTexCoord);
    }
    FragColor = color;
}
)";

const char* DEBUG_LINE_VERTEX_SHADER = R"(
#version 410 core
layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec4 aColor;

uniform mat4 uViewProjection;

out vec4 vColor;

void main() {
    vColor = aColor;
    gl_Position = uViewProjection * vec4(aPosition, 1.0);
}
)";

const char* DEBUG_LINE_FRAGMENT_SHADER = R"(
#version 410 core
in vec4 vColor;

out vec4 FragColor;

void main() {
    FragColor = vColor;
}
)";

//==========================================================================
// OpenGL Graphics3D System Implementation
//==========================================================================

BESTOW_SYSTEM(OpenGLGraphics3DSystem, IGraphics3DSystem, IAssetSystem, IShaderSystem, IConfigSystem) {
public:
    ~OpenGLGraphics3DSystem() override;

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
        uint32_t offset = 0) override;

    //======================================================================
    // Primitive Mesh Generation
    //======================================================================

    Result<MeshHandle, Graphics3DError> createCubeMesh(float size = 1.0f) override;

    Result<MeshHandle, Graphics3DError> createSphereMesh(
        float radius = 0.5f,
        uint32_t segments = 32,
        uint32_t rings = 16) override;

    Result<MeshHandle, Graphics3DError> createCylinderMesh(
        float radius = 0.5f,
        float height = 1.0f,
        uint32_t segments = 32) override;

    Result<MeshHandle, Graphics3DError> createCapsuleMesh(
        float radius = 0.5f,
        float height = 1.0f,
        uint32_t segments = 32,
        uint32_t rings = 8) override;

    Result<MeshHandle, Graphics3DError> createPlaneMesh(
        float width = 1.0f,
        float height = 1.0f,
        uint32_t widthSegments = 1,
        uint32_t heightSegments = 1) override;

    //======================================================================
    // Material Management
    //======================================================================

    Result<MaterialHandle, Graphics3DError> createMaterial(const PBRMaterial& mat) override;
    Result<MaterialHandle, Graphics3DError> createUnlitMaterial(const UnlitMaterial& mat) override;
    void destroyMaterial(MaterialHandle handle) override;
    bool hasMaterial(MaterialHandle handle) const override;

    Result<void, Graphics3DError> setMaterialTexture(
        MaterialHandle handle,
        uint32_t slot,
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
    void renderEntities(
        IEntitySystem& entities,
        RenderLayer minLayer,
        RenderLayer maxLayer) override;

    //======================================================================
    // Camera
    //======================================================================

    void setCamera(const Camera3D& camera) override;
    Camera3D getCamera() const override;

    Ray3D screenToWorldRay(Vec2 screenPos) const override;
    std::optional<Vec2> worldToScreen(const Vec3& worldPos) const override;

    //======================================================================
    // Lighting
    //======================================================================

    void setDirectionalLight(const DirectionalLight& light) override;
    void clearDirectionalLight() override;

    uint32_t addPointLight(const PointLight& light, const Vec3& position) override;
    uint32_t addSpotLight(const SpotLight& light, const Vec3& position) override;

    void setLightPosition(uint32_t lightId, const Vec3& position) override;
    void removeLight(uint32_t lightId) override;
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

    void debugDrawLine(
        const Vec3& start,
        const Vec3& end,
        const Color& color = Color::white(),
        float duration = 0.0f,
        bool depthTest = true) override;

    void debugDrawBox(
        const Vec3& center,
        const Vec3& halfExtents,
        const Quat& rotation = Quat{1.0f, 0.0f, 0.0f, 0.0f},
        const Color& color = Color::white(),
        float duration = 0.0f,
        bool depthTest = true) override;

    void debugDrawSphere(
        const Vec3& center,
        float radius,
        const Color& color = Color::white(),
        float duration = 0.0f,
        bool depthTest = true) override;

    void debugDrawCapsule(
        const Vec3& start,
        const Vec3& end,
        float radius,
        const Color& color = Color::white(),
        float duration = 0.0f,
        bool depthTest = true) override;

    void debugDrawFrustum(
        const Frustum& frustum,
        const Color& color = Color::white(),
        float duration = 0.0f,
        bool depthTest = true) override;

    void debugDrawRay(
        const Vec3& origin,
        const Vec3& direction,
        float length,
        const Color& color = Color::white(),
        float duration = 0.0f,
        bool depthTest = true) override;

    void debugDrawAxes(
        const Transform3D& transform,
        float size = 1.0f,
        float duration = 0.0f,
        bool depthTest = true) override;

    void debugDrawAABB(
        const AABB3D& aabb,
        const Color& color = Color::white(),
        float duration = 0.0f,
        bool depthTest = true) override;

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

    void updateShaders() override;

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
        uint32_t maxInstances,
        bool dynamic = true) override;

    Result<void, Graphics3DError> updateInstanceBuffer(
        InstanceBufferHandle buffer,
        std::span<const InstanceData> data,
        uint32_t offset = 0) override;

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

    std::vector<Mat4> sampleAnimation(
        AnimationClipHandle clip,
        float time,
        bool loop = true) override;

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
    void drawText3D(
        const std::string& text,
        const Vec3& position,
        Font3DHandle font,
        float fontSize = 1.0f,
        const Color& color = Color::white()) override;
    AABB3D measureText3D(
        const std::string& text,
        Font3DHandle font,
        float fontSize = 1.0f) override;

    //======================================================================
    // Material Property Updates
    //======================================================================

    Result<void, Graphics3DError> setMaterialBaseColor(
        MaterialHandle handle,
        const Vec4& color) override;

    Result<void, Graphics3DError> setMaterialMetallicRoughness(
        MaterialHandle handle,
        float metallic,
        float roughness) override;

    Result<void, Graphics3DError> setMaterialEmissive(
        MaterialHandle handle,
        const Vec3& emissive) override;

    std::optional<PBRMaterial> getMaterialProperties(MaterialHandle handle) const override;

    //======================================================================
    // LOD (Level of Detail)
    //======================================================================

    void setLODDistances(std::span<const float> distances) override;
    void registerLODMeshes(MeshHandle primaryMesh, std::span<const MeshHandle> lodMeshes) override;
    void setLODBias(float bias) override;

    //======================================================================
    // Runtime Configuration
    //======================================================================

    bool loadRuntimeConfig(const std::filesystem::path& configPath) override;
    void applyRuntimeConfig(const Graphics3DRuntimeConfig& config) override;
    const Graphics3DRuntimeConfig& getRuntimeConfig() const override;
    bool reloadRuntimeConfig() override;

private:
    // Lifecycle
    bool initialized_ = false;

    // Window
    GLFWwindow* window_ = nullptr;
    bool ownsWindow_ = false;
    Size windowSize_{800, 600};
    bool isFullscreen_ = false;

    // Camera
    Camera3D camera_;
    glm::mat4 viewMatrix_{1.0f};
    glm::mat4 projectionMatrix_{1.0f};
    glm::mat4 viewProjectionMatrix_{1.0f};
    Frustum viewFrustum_;

    // Render state
    Color clearColor_ = Color::black();
    float renderScale_ = 1.0f;
    bool frustumCullingEnabled_ = true;
    bool debugRenderingEnabled_ = true;

    // Lighting
    std::optional<DirectionalLight> directionalLight_;
    Vec3 ambientColor_{0.1f, 0.1f, 0.1f};
    float ambientIntensity_ = 1.0f;
    std::vector<LightData> lights_;
    uint32_t nextLightId_ = 1;

    // Shadows
    bool shadowsEnabled_ = true;
    int shadowResolution_ = 2048;
    float shadowDistance_ = 100.0f;

    // Environment
    std::optional<Skybox> skybox_;
    std::optional<EnvironmentMap> environmentMap_;
    Fog fog_;

    // Post-processing
    bool toneMappingEnabled_ = true;
    float exposure_ = 1.0f;
    bool bloomEnabled_ = false;
    float bloomThreshold_ = 1.0f;
    float bloomIntensity_ = 1.0f;
    bool ssaoEnabled_ = false;
    float ssaoRadius_ = 0.5f;
    float ssaoIntensity_ = 1.0f;

    // Resource management
    MeshHandle nextMeshHandle_ = 1;
    std::unordered_map<MeshHandle, MeshResource> meshes_;

    MaterialHandle nextMaterialHandle_ = 1;
    std::unordered_map<MaterialHandle, MaterialResource> materials_;
    MaterialHandle defaultPBRMaterial_ = 0;
    MaterialHandle defaultUnlitMaterial_ = 0;
    MaterialHandle errorMaterial_ = 0;

    // Instance buffers
    InstanceBufferHandle nextInstanceBufferHandle_ = 1;
    std::unordered_map<InstanceBufferHandle, InstanceBufferResource> instanceBuffers_;
    std::vector<InstancedRenderItem> instancedRenderQueue_;

    // Skeletal animation
    SkeletonHandle nextSkeletonHandle_ = 1;
    std::unordered_map<SkeletonHandle, SkeletonResource> skeletons_;
    AnimationClipHandle nextAnimationClipHandle_ = 1;
    std::unordered_map<AnimationClipHandle, AnimationClipResource> animationClips_;

    // 3D Text
    Font3DHandle nextFont3DHandle_ = 1;
    std::unordered_map<Font3DHandle, Font3DResource> fonts3D_;
    ShaderProgram textShader_;
    GLuint textQuadVAO_ = 0;
    GLuint textQuadVBO_ = 0;

    // LOD
    LODConfig lodConfig_;
    std::unordered_map<MeshHandle, LODMeshGroup> lodGroups_;

    // Shaders
    ShaderProgram pbrShader_;
    ShaderProgram unlitShader_;
    ShaderProgram debugLineShader_;

    // Debug rendering
    std::vector<DebugLineInternal> debugLines_;
    std::vector<DebugLineInternal> persistentDebugLines_;
    GLuint debugVAO_ = 0;
    GLuint debugVBO_ = 0;

    // Render queue
    std::vector<RenderItem> renderQueue_;

    // Statistics
    mutable RenderStats stats_;

    // Lua material caching
    std::unordered_map<std::string, MaterialHandle> luaMaterialCache_;

    // Runtime configuration
    Graphics3DRuntimeConfig runtimeConfig_;
    std::filesystem::path configPath_;

    // Helper functions
    bool compileShader(GLuint shader, const char* source);
    bool linkProgram(GLuint program);
    GLuint createShaderProgram(const char* vertSource, const char* fragSource);
    void createShaders();
    void createDefaultMaterials();
    void createDebugResources();

    void updateViewFrustum();
    glm::mat4 transformToMatrix(const Transform3D& transform) const;
    void renderMeshInternal(
        const MeshResource& mesh,
        const MaterialResource& material,
        const glm::mat4& worldMatrix);
    void renderDebugLines();
    void updatePersistentDebugLines(float deltaTime);
    void applyBlendMode(BlendMode mode);
    void applyCullMode(CullMode mode);

    // LOD selection helper
    MeshHandle selectLODMesh(MeshHandle primaryMesh, float distance) const;

    // Frustum culling helper - returns true if AABB is visible
    bool isAABBInFrustum(const AABB3D& aabb, const Frustum& frustum) const;

    // Transform AABB by world matrix to get world-space AABB
    AABB3D transformAABB(const AABB3D& aabb, const glm::mat4& worldMatrix) const;
};

//==========================================================================
// Implementation
//==========================================================================

OpenGLGraphics3DSystem::~OpenGLGraphics3DSystem() {
    // Clean up meshes
    for (auto& [handle, mesh] : meshes_) {
        if (mesh.vao) glDeleteVertexArrays(1, &mesh.vao);
        if (mesh.vbo) glDeleteBuffers(1, &mesh.vbo);
        if (mesh.ebo) glDeleteBuffers(1, &mesh.ebo);
    }

    // Clean up shaders
    if (pbrShader_.program) glDeleteProgram(pbrShader_.program);
    if (unlitShader_.program) glDeleteProgram(unlitShader_.program);
    if (debugLineShader_.program) glDeleteProgram(debugLineShader_.program);

    // Clean up debug resources
    if (debugVAO_) glDeleteVertexArrays(1, &debugVAO_);
    if (debugVBO_) glDeleteBuffers(1, &debugVBO_);

    // Clean up window if we own it
    if (ownsWindow_ && window_) {
        glfwDestroyWindow(window_);
    }
}

bool OpenGLGraphics3DSystem::initialize(const Graphics3DConfig& config) {
    if (initialized_) return true;

    if (config.nativeWindowHandle) {
        // Use provided window
        window_ = static_cast<GLFWwindow*>(config.nativeWindowHandle);
        ownsWindow_ = false;
    } else {
        // Create our own window
        if (!glfwInit()) {
            return false;
        }

        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
        glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
        if (config.enableValidation) {
            glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GL_TRUE);
        }

        window_ = glfwCreateWindow(config.windowWidth, config.windowHeight,
                                    config.windowTitle.c_str(), nullptr, nullptr);
        if (!window_) {
            glfwTerminate();
            return false;
        }
        ownsWindow_ = true;
        glfwMakeContextCurrent(window_);
        glfwSwapInterval(config.vsync ? 1 : 0);
    }

    // Initialize glad if not already done
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        if (ownsWindow_) {
            glfwDestroyWindow(window_);
            glfwTerminate();
        }
        return false;
    }

    // Get window size
    int width, height;
    glfwGetFramebufferSize(window_, &width, &height);
    windowSize_ = {width, height};

    // Set up OpenGL state
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);

    // Create shaders and resources
    createShaders();
    createDefaultMaterials();
    createDebugResources();

    // Set default camera
    camera_.transform.position = Vec3{0.0f, 0.0f, 5.0f};
    camera_.aspectRatio = static_cast<float>(width) / static_cast<float>(height);
    setCamera(camera_);

    initialized_ = true;
    return true;
}

void OpenGLGraphics3DSystem::shutdown() {
    if (!initialized_) return;
    // Cleanup OpenGL resources handled by destructor
    initialized_ = false;
}

void OpenGLGraphics3DSystem::beginFrame() {
    stats_ = RenderStats{};

    // Normalize Color (uint8_t 0-255) to float (0.0-1.0) for glClearColor
    glClearColor(
        static_cast<float>(clearColor_.r) / 255.0f,
        static_cast<float>(clearColor_.g) / 255.0f,
        static_cast<float>(clearColor_.b) / 255.0f,
        static_cast<float>(clearColor_.a) / 255.0f
    );
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Update view frustum for culling
    updateViewFrustum();
}

void OpenGLGraphics3DSystem::endFrame() {
    // Render debug lines last
    if (debugRenderingEnabled_) {
        renderDebugLines();
    }

    glfwSwapBuffers(window_);
    glfwPollEvents();

    // Clear one-frame debug lines
    debugLines_.clear();
}

//==========================================================================
// Mesh Management
//==========================================================================

Result<MeshHandle, Graphics3DError> OpenGLGraphics3DSystem::createMesh(const MeshDef& def) {
    if (def.vertices.empty()) {
        return std::unexpected(Graphics3DError::InvalidMesh);
    }

    MeshResource mesh;
    mesh.bounds = def.bounds;
    mesh.subMeshes = def.subMeshes;
    mesh.vertexCount = static_cast<uint32_t>(def.vertices.size());
    mesh.indexCount = static_cast<uint32_t>(def.indices.size());
    mesh.isDynamic = def.isDynamic;

    // Create VAO
    glGenVertexArrays(1, &mesh.vao);
    glBindVertexArray(mesh.vao);

    // Create VBO
    glGenBuffers(1, &mesh.vbo);
    glBindBuffer(GL_ARRAY_BUFFER, mesh.vbo);
    glBufferData(GL_ARRAY_BUFFER,
                 def.vertices.size() * sizeof(Vertex3D),
                 def.vertices.data(),
                 def.isDynamic ? GL_DYNAMIC_DRAW : GL_STATIC_DRAW);

    // Set up vertex attributes
    // Position
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex3D),
                          (void*)offsetof(Vertex3D, position));

    // Normal
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex3D),
                          (void*)offsetof(Vertex3D, normal));

    // TexCoord
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex3D),
                          (void*)offsetof(Vertex3D, texCoord));

    // Color
    glEnableVertexAttribArray(3);
    glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex3D),
                          (void*)offsetof(Vertex3D, color));

    // Create EBO if indices provided
    if (!def.indices.empty()) {
        glGenBuffers(1, &mesh.ebo);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.ebo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                     def.indices.size() * sizeof(uint32_t),
                     def.indices.data(),
                     GL_STATIC_DRAW);
    }

    glBindVertexArray(0);

    MeshHandle handle = nextMeshHandle_++;
    meshes_[handle] = mesh;

    stats_.meshes++;

    return handle;
}

void OpenGLGraphics3DSystem::destroyMesh(MeshHandle handle) {
    auto it = meshes_.find(handle);
    if (it == meshes_.end()) return;

    auto& mesh = it->second;
    if (mesh.vao) glDeleteVertexArrays(1, &mesh.vao);
    if (mesh.vbo) glDeleteBuffers(1, &mesh.vbo);
    if (mesh.ebo) glDeleteBuffers(1, &mesh.ebo);

    meshes_.erase(it);
    stats_.meshes--;
}

bool OpenGLGraphics3DSystem::hasMesh(MeshHandle handle) const {
    return meshes_.find(handle) != meshes_.end();
}

AABB3D OpenGLGraphics3DSystem::getMeshBounds(MeshHandle handle) const {
    auto it = meshes_.find(handle);
    if (it == meshes_.end()) return AABB3D{};
    return it->second.bounds;
}

Result<void, Graphics3DError> OpenGLGraphics3DSystem::updateMeshVertices(
    MeshHandle handle,
    std::span<const Vertex3D> vertices,
    uint32_t offset) {

    auto it = meshes_.find(handle);
    if (it == meshes_.end()) {
        return std::unexpected(Graphics3DError::InvalidMesh);
    }

    auto& mesh = it->second;
    if (!mesh.isDynamic) {
        return std::unexpected(Graphics3DError::InvalidMesh);
    }

    glBindBuffer(GL_ARRAY_BUFFER, mesh.vbo);
    glBufferSubData(GL_ARRAY_BUFFER,
                    offset * sizeof(Vertex3D),
                    vertices.size() * sizeof(Vertex3D),
                    vertices.data());
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    return {};
}

//==========================================================================
// Primitive Mesh Generation
//==========================================================================

Result<MeshHandle, Graphics3DError> OpenGLGraphics3DSystem::createCubeMesh(float size) {
    float s = size * 0.5f;

    std::vector<Vertex3D> vertices = {
        // Front face (+Z)
        {{-s, -s,  s}, {0, 0, 1}, {0, 0}},
        {{ s, -s,  s}, {0, 0, 1}, {1, 0}},
        {{ s,  s,  s}, {0, 0, 1}, {1, 1}},
        {{-s,  s,  s}, {0, 0, 1}, {0, 1}},

        // Back face (-Z)
        {{ s, -s, -s}, {0, 0, -1}, {0, 0}},
        {{-s, -s, -s}, {0, 0, -1}, {1, 0}},
        {{-s,  s, -s}, {0, 0, -1}, {1, 1}},
        {{ s,  s, -s}, {0, 0, -1}, {0, 1}},

        // Right face (+X)
        {{ s, -s,  s}, {1, 0, 0}, {0, 0}},
        {{ s, -s, -s}, {1, 0, 0}, {1, 0}},
        {{ s,  s, -s}, {1, 0, 0}, {1, 1}},
        {{ s,  s,  s}, {1, 0, 0}, {0, 1}},

        // Left face (-X)
        {{-s, -s, -s}, {-1, 0, 0}, {0, 0}},
        {{-s, -s,  s}, {-1, 0, 0}, {1, 0}},
        {{-s,  s,  s}, {-1, 0, 0}, {1, 1}},
        {{-s,  s, -s}, {-1, 0, 0}, {0, 1}},

        // Top face (+Y)
        {{-s,  s,  s}, {0, 1, 0}, {0, 0}},
        {{ s,  s,  s}, {0, 1, 0}, {1, 0}},
        {{ s,  s, -s}, {0, 1, 0}, {1, 1}},
        {{-s,  s, -s}, {0, 1, 0}, {0, 1}},

        // Bottom face (-Y)
        {{-s, -s, -s}, {0, -1, 0}, {0, 0}},
        {{ s, -s, -s}, {0, -1, 0}, {1, 0}},
        {{ s, -s,  s}, {0, -1, 0}, {1, 1}},
        {{-s, -s,  s}, {0, -1, 0}, {0, 1}},
    };

    std::vector<uint32_t> indices = {
        0, 1, 2, 2, 3, 0,       // Front
        4, 5, 6, 6, 7, 4,       // Back
        8, 9, 10, 10, 11, 8,    // Right
        12, 13, 14, 14, 15, 12, // Left
        16, 17, 18, 18, 19, 16, // Top
        20, 21, 22, 22, 23, 20  // Bottom
    };

    AABB3D bounds;
    bounds.min = Vec3{-s, -s, -s};
    bounds.max = Vec3{s, s, s};

    MeshDef def;
    def.vertices = vertices;
    def.indices = indices;
    def.bounds = bounds;

    return createMesh(def);
}

Result<MeshHandle, Graphics3DError> OpenGLGraphics3DSystem::createSphereMesh(
    float radius,
    uint32_t segments,
    uint32_t rings) {

    std::vector<Vertex3D> vertices;
    std::vector<uint32_t> indices;

    // Generate vertices
    for (uint32_t ring = 0; ring <= rings; ++ring) {
        float v = static_cast<float>(ring) / rings;
        float phi = v * glm::pi<float>();

        for (uint32_t segment = 0; segment <= segments; ++segment) {
            float u = static_cast<float>(segment) / segments;
            float theta = u * 2.0f * glm::pi<float>();

            Vertex3D vertex;
            vertex.position.x = radius * std::sin(phi) * std::cos(theta);
            vertex.position.y = radius * std::cos(phi);
            vertex.position.z = radius * std::sin(phi) * std::sin(theta);

            vertex.normal = glm::normalize(vertex.position);
            vertex.texCoord = Vec2{u, v};

            vertices.push_back(vertex);
        }
    }

    // Generate indices
    for (uint32_t ring = 0; ring < rings; ++ring) {
        for (uint32_t segment = 0; segment < segments; ++segment) {
            uint32_t current = ring * (segments + 1) + segment;
            uint32_t next = current + segments + 1;

            indices.push_back(current);
            indices.push_back(next);
            indices.push_back(current + 1);

            indices.push_back(current + 1);
            indices.push_back(next);
            indices.push_back(next + 1);
        }
    }

    AABB3D bounds;
    bounds.min = Vec3{-radius, -radius, -radius};
    bounds.max = Vec3{radius, radius, radius};

    MeshDef def;
    def.vertices = vertices;
    def.indices = indices;
    def.bounds = bounds;

    return createMesh(def);
}

Result<MeshHandle, Graphics3DError> OpenGLGraphics3DSystem::createCylinderMesh(
    float radius,
    float height,
    uint32_t segments) {

    std::vector<Vertex3D> vertices;
    std::vector<uint32_t> indices;

    float halfHeight = height * 0.5f;

    // Generate side vertices
    for (uint32_t i = 0; i <= segments; ++i) {
        float angle = (static_cast<float>(i) / segments) * 2.0f * glm::pi<float>();
        float x = std::cos(angle) * radius;
        float z = std::sin(angle) * radius;
        float u = static_cast<float>(i) / segments;

        // Bottom vertex
        Vertex3D bottom;
        bottom.position = Vec3{x, -halfHeight, z};
        bottom.normal = glm::normalize(Vec3{x, 0.0f, z});
        bottom.texCoord = Vec2{u, 0.0f};
        vertices.push_back(bottom);

        // Top vertex
        Vertex3D top;
        top.position = Vec3{x, halfHeight, z};
        top.normal = glm::normalize(Vec3{x, 0.0f, z});
        top.texCoord = Vec2{u, 1.0f};
        vertices.push_back(top);
    }

    // Generate side indices
    for (uint32_t i = 0; i < segments; ++i) {
        uint32_t b0 = i * 2;
        uint32_t t0 = i * 2 + 1;
        uint32_t b1 = (i + 1) * 2;
        uint32_t t1 = (i + 1) * 2 + 1;

        indices.push_back(b0);
        indices.push_back(b1);
        indices.push_back(t1);

        indices.push_back(b0);
        indices.push_back(t1);
        indices.push_back(t0);
    }

    uint32_t sideVertexCount = static_cast<uint32_t>(vertices.size());

    // Top cap center
    uint32_t topCenter = static_cast<uint32_t>(vertices.size());
    Vertex3D topCenterV;
    topCenterV.position = Vec3{0.0f, halfHeight, 0.0f};
    topCenterV.normal = Vec3{0.0f, 1.0f, 0.0f};
    topCenterV.texCoord = Vec2{0.5f, 0.5f};
    vertices.push_back(topCenterV);

    // Top cap ring
    for (uint32_t i = 0; i <= segments; ++i) {
        float angle = (static_cast<float>(i) / segments) * 2.0f * glm::pi<float>();
        float x = std::cos(angle) * radius;
        float z = std::sin(angle) * radius;

        Vertex3D v;
        v.position = Vec3{x, halfHeight, z};
        v.normal = Vec3{0.0f, 1.0f, 0.0f};
        v.texCoord = Vec2{0.5f + 0.5f * std::cos(angle), 0.5f + 0.5f * std::sin(angle)};
        vertices.push_back(v);
    }

    // Top cap indices
    for (uint32_t i = 0; i < segments; ++i) {
        indices.push_back(topCenter);
        indices.push_back(topCenter + 1 + i);
        indices.push_back(topCenter + 2 + i);
    }

    // Bottom cap center
    uint32_t bottomCenter = static_cast<uint32_t>(vertices.size());
    Vertex3D bottomCenterV;
    bottomCenterV.position = Vec3{0.0f, -halfHeight, 0.0f};
    bottomCenterV.normal = Vec3{0.0f, -1.0f, 0.0f};
    bottomCenterV.texCoord = Vec2{0.5f, 0.5f};
    vertices.push_back(bottomCenterV);

    // Bottom cap ring
    for (uint32_t i = 0; i <= segments; ++i) {
        float angle = (static_cast<float>(i) / segments) * 2.0f * glm::pi<float>();
        float x = std::cos(angle) * radius;
        float z = std::sin(angle) * radius;

        Vertex3D v;
        v.position = Vec3{x, -halfHeight, z};
        v.normal = Vec3{0.0f, -1.0f, 0.0f};
        v.texCoord = Vec2{0.5f + 0.5f * std::cos(angle), 0.5f + 0.5f * std::sin(angle)};
        vertices.push_back(v);
    }

    // Bottom cap indices (reverse winding)
    for (uint32_t i = 0; i < segments; ++i) {
        indices.push_back(bottomCenter);
        indices.push_back(bottomCenter + 2 + i);
        indices.push_back(bottomCenter + 1 + i);
    }

    AABB3D bounds;
    bounds.min = Vec3{-radius, -halfHeight, -radius};
    bounds.max = Vec3{radius, halfHeight, radius};

    MeshDef def;
    def.vertices = vertices;
    def.indices = indices;
    def.bounds = bounds;

    return createMesh(def);
}

Result<MeshHandle, Graphics3DError> OpenGLGraphics3DSystem::createCapsuleMesh(
    float radius,
    float height,
    uint32_t segments,
    uint32_t rings) {

    std::vector<Vertex3D> vertices;
    std::vector<uint32_t> indices;

    float halfHeight = height * 0.5f;

    // Top hemisphere
    for (uint32_t ring = 0; ring <= rings; ++ring) {
        float v = static_cast<float>(ring) / rings;
        float phi = v * glm::pi<float>() * 0.5f;  // 0 to pi/2 (top half)

        for (uint32_t seg = 0; seg <= segments; ++seg) {
            float u = static_cast<float>(seg) / segments;
            float theta = u * 2.0f * glm::pi<float>();

            Vertex3D vertex;
            float sinPhi = std::sin(phi);
            float cosPhi = std::cos(phi);
            float sinTheta = std::sin(theta);
            float cosTheta = std::cos(theta);

            vertex.normal = Vec3{sinPhi * cosTheta, cosPhi, sinPhi * sinTheta};
            vertex.position = vertex.normal * radius + Vec3{0.0f, halfHeight, 0.0f};
            vertex.texCoord = Vec2{u, v * 0.25f};  // Top quarter of texture

            vertices.push_back(vertex);
        }
    }

    // Generate top hemisphere indices
    for (uint32_t ring = 0; ring < rings; ++ring) {
        for (uint32_t seg = 0; seg < segments; ++seg) {
            uint32_t current = ring * (segments + 1) + seg;
            uint32_t next = current + segments + 1;

            indices.push_back(current);
            indices.push_back(next);
            indices.push_back(current + 1);

            indices.push_back(current + 1);
            indices.push_back(next);
            indices.push_back(next + 1);
        }
    }

    uint32_t cylinderStart = static_cast<uint32_t>(vertices.size());

    // Cylinder section
    for (uint32_t seg = 0; seg <= segments; ++seg) {
        float u = static_cast<float>(seg) / segments;
        float theta = u * 2.0f * glm::pi<float>();
        float x = std::cos(theta) * radius;
        float z = std::sin(theta) * radius;

        // Top of cylinder
        Vertex3D top;
        top.position = Vec3{x, halfHeight, z};
        top.normal = glm::normalize(Vec3{x, 0.0f, z});
        top.texCoord = Vec2{u, 0.25f};
        vertices.push_back(top);

        // Bottom of cylinder
        Vertex3D bottom;
        bottom.position = Vec3{x, -halfHeight, z};
        bottom.normal = glm::normalize(Vec3{x, 0.0f, z});
        bottom.texCoord = Vec2{u, 0.75f};
        vertices.push_back(bottom);
    }

    // Cylinder indices
    for (uint32_t seg = 0; seg < segments; ++seg) {
        uint32_t t0 = cylinderStart + seg * 2;
        uint32_t b0 = t0 + 1;
        uint32_t t1 = cylinderStart + (seg + 1) * 2;
        uint32_t b1 = t1 + 1;

        indices.push_back(t0);
        indices.push_back(b0);
        indices.push_back(t1);

        indices.push_back(t1);
        indices.push_back(b0);
        indices.push_back(b1);
    }

    uint32_t bottomHemiStart = static_cast<uint32_t>(vertices.size());

    // Bottom hemisphere
    for (uint32_t ring = 0; ring <= rings; ++ring) {
        float v = static_cast<float>(ring) / rings;
        float phi = glm::pi<float>() * 0.5f + v * glm::pi<float>() * 0.5f;  // pi/2 to pi

        for (uint32_t seg = 0; seg <= segments; ++seg) {
            float u = static_cast<float>(seg) / segments;
            float theta = u * 2.0f * glm::pi<float>();

            Vertex3D vertex;
            float sinPhi = std::sin(phi);
            float cosPhi = std::cos(phi);
            float sinTheta = std::sin(theta);
            float cosTheta = std::cos(theta);

            vertex.normal = Vec3{sinPhi * cosTheta, cosPhi, sinPhi * sinTheta};
            vertex.position = vertex.normal * radius + Vec3{0.0f, -halfHeight, 0.0f};
            vertex.texCoord = Vec2{u, 0.75f + v * 0.25f};  // Bottom quarter

            vertices.push_back(vertex);
        }
    }

    // Generate bottom hemisphere indices
    for (uint32_t ring = 0; ring < rings; ++ring) {
        for (uint32_t seg = 0; seg < segments; ++seg) {
            uint32_t current = bottomHemiStart + ring * (segments + 1) + seg;
            uint32_t next = current + segments + 1;

            indices.push_back(current);
            indices.push_back(next);
            indices.push_back(current + 1);

            indices.push_back(current + 1);
            indices.push_back(next);
            indices.push_back(next + 1);
        }
    }

    AABB3D bounds;
    bounds.min = Vec3{-radius, -halfHeight - radius, -radius};
    bounds.max = Vec3{radius, halfHeight + radius, radius};

    MeshDef def;
    def.vertices = vertices;
    def.indices = indices;
    def.bounds = bounds;

    return createMesh(def);
}

Result<MeshHandle, Graphics3DError> OpenGLGraphics3DSystem::createPlaneMesh(
    float width,
    float height,
    uint32_t widthSegments,
    uint32_t heightSegments) {

    std::vector<Vertex3D> vertices;
    std::vector<uint32_t> indices;

    float w = width * 0.5f;
    float h = height * 0.5f;

    // Generate vertices
    for (uint32_t y = 0; y <= heightSegments; ++y) {
        float v = static_cast<float>(y) / heightSegments;
        for (uint32_t x = 0; x <= widthSegments; ++x) {
            float u = static_cast<float>(x) / widthSegments;

            Vertex3D vertex;
            vertex.position = Vec3{
                -w + u * width,
                0.0f,
                -h + v * height
            };
            vertex.normal = Vec3{0.0f, 1.0f, 0.0f};
            vertex.texCoord = Vec2{u, v};

            vertices.push_back(vertex);
        }
    }

    // Generate indices
    for (uint32_t y = 0; y < heightSegments; ++y) {
        for (uint32_t x = 0; x < widthSegments; ++x) {
            uint32_t i0 = y * (widthSegments + 1) + x;
            uint32_t i1 = i0 + 1;
            uint32_t i2 = i0 + (widthSegments + 1);
            uint32_t i3 = i2 + 1;

            indices.push_back(i0);
            indices.push_back(i2);
            indices.push_back(i1);

            indices.push_back(i1);
            indices.push_back(i2);
            indices.push_back(i3);
        }
    }

    AABB3D bounds;
    bounds.min = Vec3{-w, 0.0f, -h};
    bounds.max = Vec3{w, 0.0f, h};

    MeshDef def;
    def.vertices = vertices;
    def.indices = indices;
    def.bounds = bounds;

    return createMesh(def);
}

//==========================================================================
// Material Management
//==========================================================================

Result<MaterialHandle, Graphics3DError> OpenGLGraphics3DSystem::createMaterial(const PBRMaterial& mat) {
    MaterialResource material;
    material.baseColor = mat.baseColorFactor;
    material.metallic = mat.metallicFactor;
    material.roughness = mat.roughnessFactor;
    material.emissive = mat.emissiveFactor;
    material.blendMode = mat.blendMode;
    material.cullMode = mat.cullMode;
    material.alphaCutoff = mat.alphaCutoff;
    material.doubleSided = mat.doubleSided;
    material.receiveShadows = mat.receiveShadows;
    material.castShadows = mat.castShadows;
    material.isUnlit = false;

    // TODO: Load textures from asset handles when asset system is integrated

    MaterialHandle handle = nextMaterialHandle_++;
    materials_[handle] = material;
    stats_.materials++;

    return handle;
}

Result<MaterialHandle, Graphics3DError> OpenGLGraphics3DSystem::createUnlitMaterial(const UnlitMaterial& mat) {
    MaterialResource material;
    material.baseColor = mat.color;
    material.blendMode = mat.blendMode;
    material.cullMode = mat.cullMode;
    material.isUnlit = true;

    // TODO: Load texture from asset handle

    MaterialHandle handle = nextMaterialHandle_++;
    materials_[handle] = material;
    stats_.materials++;

    return handle;
}

void OpenGLGraphics3DSystem::destroyMaterial(MaterialHandle handle) {
    auto it = materials_.find(handle);
    if (it != materials_.end()) {
        materials_.erase(it);
        stats_.materials--;
    }
}

bool OpenGLGraphics3DSystem::hasMaterial(MaterialHandle handle) const {
    return materials_.find(handle) != materials_.end();
}

Result<void, Graphics3DError> OpenGLGraphics3DSystem::setMaterialTexture(
    MaterialHandle handle,
    uint32_t slot,
    AssetHandle texture) {

    auto it = materials_.find(handle);
    if (it == materials_.end()) {
        return std::unexpected(Graphics3DError::InvalidMaterial);
    }

    // TODO: Load texture from asset system and set appropriate texture slot

    return {};
}

MaterialHandle OpenGLGraphics3DSystem::getDefaultPBRMaterial() const {
    return defaultPBRMaterial_;
}

MaterialHandle OpenGLGraphics3DSystem::getDefaultUnlitMaterial() const {
    return defaultUnlitMaterial_;
}

MaterialHandle OpenGLGraphics3DSystem::getErrorMaterial() const {
    return errorMaterial_;
}

//==========================================================================
// Immediate Mode Rendering
//==========================================================================

void OpenGLGraphics3DSystem::drawMesh(
    MeshHandle meshHandle,
    MaterialHandle materialHandle,
    const Mat4& worldMatrix,
    bool castShadow,
    bool receiveShadow) {

    auto meshIt = meshes_.find(meshHandle);
    auto matIt = materials_.find(materialHandle);

    if (meshIt == meshes_.end() || matIt == materials_.end()) {
        return;
    }

    renderMeshInternal(meshIt->second, matIt->second, worldMatrix);
}

void OpenGLGraphics3DSystem::drawMesh(
    MeshHandle meshHandle,
    MaterialHandle materialHandle,
    const Transform3D& transform,
    bool castShadow,
    bool receiveShadow) {

    glm::mat4 worldMatrix = transformToMatrix(transform);
    drawMesh(meshHandle, materialHandle, worldMatrix, castShadow, receiveShadow);
}

//==========================================================================
// Batch Rendering
//==========================================================================

void OpenGLGraphics3DSystem::queueRenderItem(const RenderItem& item) {
    renderQueue_.push_back(item);
}

void OpenGLGraphics3DSystem::queueRenderItems(std::span<const RenderItem> items) {
    renderQueue_.insert(renderQueue_.end(), items.begin(), items.end());
}

void OpenGLGraphics3DSystem::flushRenderQueue() {
    // TODO: Sort render queue by material/depth
    // TODO: Batch rendering

    for (const auto& item : renderQueue_) {
        drawMesh(item.mesh, item.material, item.worldMatrix,
                item.castShadow, item.receiveShadow);
    }

    renderQueue_.clear();
}

//==========================================================================
// Entity Rendering
//==========================================================================

void OpenGLGraphics3DSystem::renderEntities(IEntitySystem& entities) {
    // Query entities with Mesh3DComponent and Transform3D components
    auto view = entities.view<Mesh3DComponent, Transform3D>();

    for (auto entity : view) {
        const auto& meshComp = view.get<Mesh3DComponent>(entity);
        const auto& transform = view.get<Transform3D>(entity);

        // Skip invisible entities
        if (!meshComp.visible) continue;

        // Convert transform to matrix
        glm::mat4 worldMatrix = transformToMatrix(transform);

        // Frustum culling if enabled
        if (frustumCullingEnabled_) {
            auto meshIt = meshes_.find(meshComp.mesh);
            if (meshIt != meshes_.end()) {
                AABB3D worldBounds = transformAABB(meshIt->second.bounds, worldMatrix);
                if (!isAABBInFrustum(worldBounds, viewFrustum_)) {
                    stats_.culledObjects++;
                    continue;
                }
            }
        }

        stats_.visibleObjects++;

        // Draw the mesh
        drawMesh(meshComp.mesh, meshComp.material, worldMatrix,
                meshComp.castShadow, meshComp.receiveShadow);
    }
}

void OpenGLGraphics3DSystem::renderEntities(IEntitySystem& entities, const Frustum& frustum) {
    // Query entities with Mesh3DComponent and Transform3D components
    auto view = entities.view<Mesh3DComponent, Transform3D>();

    for (auto entity : view) {
        const auto& meshComp = view.get<Mesh3DComponent>(entity);
        const auto& transform = view.get<Transform3D>(entity);

        // Skip invisible entities
        if (!meshComp.visible) continue;

        // Convert transform to matrix
        glm::mat4 worldMatrix = transformToMatrix(transform);

        // Frustum culling with provided frustum
        auto meshIt = meshes_.find(meshComp.mesh);
        if (meshIt != meshes_.end()) {
            AABB3D worldBounds = transformAABB(meshIt->second.bounds, worldMatrix);
            if (!isAABBInFrustum(worldBounds, frustum)) {
                stats_.culledObjects++;
                continue;
            }
        }

        stats_.visibleObjects++;

        // Draw the mesh
        drawMesh(meshComp.mesh, meshComp.material, worldMatrix,
                meshComp.castShadow, meshComp.receiveShadow);
    }
}

void OpenGLGraphics3DSystem::renderEntities(
    IEntitySystem& entities,
    RenderLayer minLayer,
    RenderLayer maxLayer) {

    // Query entities with Mesh3DComponent and Transform3D components
    auto view = entities.view<Mesh3DComponent, Transform3D>();

    for (auto entity : view) {
        const auto& meshComp = view.get<Mesh3DComponent>(entity);
        const auto& transform = view.get<Transform3D>(entity);

        // Skip invisible entities
        if (!meshComp.visible) continue;

        // Skip entities outside the layer range
        if (meshComp.layer < minLayer || meshComp.layer > maxLayer) continue;

        // Convert transform to matrix
        glm::mat4 worldMatrix = transformToMatrix(transform);

        // Frustum culling if enabled
        if (frustumCullingEnabled_) {
            auto meshIt = meshes_.find(meshComp.mesh);
            if (meshIt != meshes_.end()) {
                AABB3D worldBounds = transformAABB(meshIt->second.bounds, worldMatrix);
                if (!isAABBInFrustum(worldBounds, viewFrustum_)) {
                    stats_.culledObjects++;
                    continue;
                }
            }
        }

        stats_.visibleObjects++;

        // Draw the mesh
        drawMesh(meshComp.mesh, meshComp.material, worldMatrix,
                meshComp.castShadow, meshComp.receiveShadow);
    }
}

//==========================================================================
// Camera
//==========================================================================

void OpenGLGraphics3DSystem::setCamera(const Camera3D& camera) {
    camera_ = camera;

    // Compute view matrix
    glm::vec3 position = camera.transform.position;
    glm::quat rotation = camera.transform.rotation;

    glm::mat4 rotationMatrix = glm::mat4_cast(rotation);
    glm::vec3 forward = glm::vec3(rotationMatrix * glm::vec4(0, 0, -1, 0));
    glm::vec3 up = glm::vec3(rotationMatrix * glm::vec4(0, 1, 0, 0));

    viewMatrix_ = glm::lookAt(position, position + forward, up);

    // Compute projection matrix
    if (camera.projection == ProjectionType::Perspective) {
        float fovRadians = glm::radians(camera.fovY);
        projectionMatrix_ = glm::perspective(fovRadians, camera.aspectRatio,
                                            camera.nearPlane, camera.farPlane);
    } else {
        float halfWidth = camera.orthoWidth * 0.5f;
        float halfHeight = camera.orthoHeight * 0.5f;
        projectionMatrix_ = glm::ortho(-halfWidth, halfWidth, -halfHeight, halfHeight,
                                      camera.nearPlane, camera.farPlane);
    }

    viewProjectionMatrix_ = projectionMatrix_ * viewMatrix_;
    updateViewFrustum();
}

Camera3D OpenGLGraphics3DSystem::getCamera() const {
    return camera_;
}

Ray3D OpenGLGraphics3DSystem::screenToWorldRay(Vec2 screenPos) const {
    // Convert screen coordinates to NDC
    Vec2 ndc;
    ndc.x = (2.0f * screenPos.x) / static_cast<float>(windowSize_.width) - 1.0f;
    ndc.y = 1.0f - (2.0f * screenPos.y) / static_cast<float>(windowSize_.height);

    // Inverse projection and view
    glm::mat4 invViewProj = glm::inverse(viewProjectionMatrix_);

    glm::vec4 rayClip(ndc.x, ndc.y, -1.0f, 1.0f);
    glm::vec4 rayEye = glm::inverse(projectionMatrix_) * rayClip;
    rayEye = glm::vec4(rayEye.x, rayEye.y, -1.0f, 0.0f);

    glm::vec3 rayWorld = glm::vec3(glm::inverse(viewMatrix_) * rayEye);
    rayWorld = glm::normalize(rayWorld);

    Ray3D ray;
    ray.origin = camera_.transform.position;
    ray.direction = rayWorld;

    return ray;
}

std::optional<Vec2> OpenGLGraphics3DSystem::worldToScreen(const Vec3& worldPos) const {
    glm::vec4 clipSpace = viewProjectionMatrix_ * glm::vec4(worldPos, 1.0f);

    // Check if behind camera
    if (clipSpace.w <= 0.0f) {
        return std::nullopt;
    }

    // Perspective divide
    glm::vec3 ndc = glm::vec3(clipSpace) / clipSpace.w;

    // Check if outside view frustum
    if (ndc.x < -1.0f || ndc.x > 1.0f || ndc.y < -1.0f || ndc.y > 1.0f) {
        return std::nullopt;
    }

    // Convert to screen coordinates
    Vec2 screen;
    screen.x = (ndc.x + 1.0f) * 0.5f * static_cast<float>(windowSize_.width);
    screen.y = (1.0f - ndc.y) * 0.5f * static_cast<float>(windowSize_.height);

    return screen;
}

//==========================================================================
// Lighting
//==========================================================================

void OpenGLGraphics3DSystem::setDirectionalLight(const DirectionalLight& light) {
    directionalLight_ = light;
}

void OpenGLGraphics3DSystem::clearDirectionalLight() {
    directionalLight_.reset();
}

uint32_t OpenGLGraphics3DSystem::addPointLight(const PointLight& light, const Vec3& position) {
    LightData data;
    data.id = nextLightId_++;
    data.type = LightData::Type::Point;
    data.position = position;
    data.color = light.color;
    data.intensity = light.intensity;
    data.range = light.range;
    data.castShadows = light.castShadows;

    lights_.push_back(data);
    stats_.lights++;

    return data.id;
}

uint32_t OpenGLGraphics3DSystem::addSpotLight(const SpotLight& light, const Vec3& position) {
    LightData data;
    data.id = nextLightId_++;
    data.type = LightData::Type::Spot;
    data.position = position;
    data.direction = light.direction;
    data.color = light.color;
    data.intensity = light.intensity;
    data.range = light.range;
    data.innerCone = light.innerConeAngle;
    data.outerCone = light.outerConeAngle;
    data.castShadows = light.castShadows;

    lights_.push_back(data);
    stats_.lights++;

    return data.id;
}

void OpenGLGraphics3DSystem::setLightPosition(uint32_t lightId, const Vec3& position) {
    for (auto& light : lights_) {
        if (light.id == lightId) {
            light.position = position;
            return;
        }
    }
}

void OpenGLGraphics3DSystem::removeLight(uint32_t lightId) {
    auto it = std::remove_if(lights_.begin(), lights_.end(),
        [lightId](const LightData& light) { return light.id == lightId; });

    if (it != lights_.end()) {
        lights_.erase(it, lights_.end());
        stats_.lights--;
    }
}

void OpenGLGraphics3DSystem::clearLights() {
    lights_.clear();
    stats_.lights = 0;
}

void OpenGLGraphics3DSystem::setAmbientLight(const Vec3& color, float intensity) {
    ambientColor_ = color;
    ambientIntensity_ = intensity;
}

void OpenGLGraphics3DSystem::updateEntityLights(IEntitySystem& entities) {
    // Clear existing dynamic lights (keep only manually added ones)
    // For simplicity, we rebuild the lights_ vector each frame from entities
    lights_.clear();
    stats_.lights = 0;

    // Query entities with Light3DComponent and Transform3D
    auto view = entities.view<Light3DComponent, Transform3D>();

    for (auto entity : view) {
        const auto& lightComp = view.get<Light3DComponent>(entity);
        const auto& transform = view.get<Transform3D>(entity);

        // Skip disabled lights
        if (!lightComp.enabled) continue;

        const auto& light = lightComp.light;

        LightData data;
        data.id = static_cast<uint32_t>(entity);
        data.position = transform.position;
        data.color = light.color;
        data.intensity = light.intensity;
        data.range = light.range;
        data.castShadows = light.castShadows;

        // Convert from rotation quaternion to direction vector
        glm::mat4 rotMatrix = glm::mat4_cast(transform.rotation);
        glm::vec3 forward = glm::vec3(rotMatrix * glm::vec4(0, 0, -1, 0));

        switch (light.type) {
            case LightType::Point:
                data.type = LightData::Type::Point;
                data.direction = Vec3{0, -1, 0};  // Not used for point lights
                data.innerCone = 0.0f;
                data.outerCone = 0.0f;
                break;

            case LightType::Spot:
                data.type = LightData::Type::Spot;
                data.direction = forward;
                data.innerCone = light.innerConeAngle;
                data.outerCone = light.outerConeAngle;
                break;

            case LightType::Directional:
                // Directional lights are handled separately via setDirectionalLight
                // Skip them here - they should be set up separately
                continue;
        }

        lights_.push_back(data);
        stats_.lights++;
    }
}

//==========================================================================
// Environment
//==========================================================================

void OpenGLGraphics3DSystem::setSkybox(const Skybox& skybox) {
    skybox_ = skybox;
}

void OpenGLGraphics3DSystem::clearSkybox() {
    skybox_.reset();
}

void OpenGLGraphics3DSystem::setEnvironmentMap(const EnvironmentMap& envMap) {
    environmentMap_ = envMap;
}

void OpenGLGraphics3DSystem::clearEnvironmentMap() {
    environmentMap_.reset();
}

void OpenGLGraphics3DSystem::setFog(const Fog& fog) {
    fog_ = fog;
}

//==========================================================================
// Shadows
//==========================================================================

void OpenGLGraphics3DSystem::setShadowsEnabled(bool enabled) {
    shadowsEnabled_ = enabled;
}

bool OpenGLGraphics3DSystem::areShadowsEnabled() const {
    return shadowsEnabled_;
}

void OpenGLGraphics3DSystem::setDirectionalShadowResolution(int resolution) {
    shadowResolution_ = resolution;
}

void OpenGLGraphics3DSystem::setShadowDistance(float distance) {
    shadowDistance_ = distance;
}

//==========================================================================
// Debug Rendering
//==========================================================================

void OpenGLGraphics3DSystem::debugDrawLine(
    const Vec3& start,
    const Vec3& end,
    const Color& color,
    float duration,
    bool depthTest) {

    DebugLineInternal line{start, end, color, duration, depthTest};

    if (duration > 0.0f) {
        persistentDebugLines_.push_back(line);
    } else {
        debugLines_.push_back(line);
    }
}

void OpenGLGraphics3DSystem::debugDrawBox(
    const Vec3& center,
    const Vec3& halfExtents,
    const Quat& rotation,
    const Color& color,
    float duration,
    bool depthTest) {

    // Generate 12 edges of the box
    Vec3 corners[8];
    for (int i = 0; i < 8; ++i) {
        Vec3 corner{
            (i & 1) ? halfExtents.x : -halfExtents.x,
            (i & 2) ? halfExtents.y : -halfExtents.y,
            (i & 4) ? halfExtents.z : -halfExtents.z
        };

        // Rotate and translate
        glm::vec3 rotated = rotation * corner;
        corners[i] = center + rotated;
    }

    // Draw 12 edges
    int edges[12][2] = {
        {0, 1}, {1, 3}, {3, 2}, {2, 0}, // Bottom face
        {4, 5}, {5, 7}, {7, 6}, {6, 4}, // Top face
        {0, 4}, {1, 5}, {2, 6}, {3, 7}  // Vertical edges
    };

    for (auto& edge : edges) {
        debugDrawLine(corners[edge[0]], corners[edge[1]], color, duration, depthTest);
    }
}

void OpenGLGraphics3DSystem::debugDrawSphere(
    const Vec3& center,
    float radius,
    const Color& color,
    float duration,
    bool depthTest) {

    // Draw 3 circle wireframes (XY, XZ, YZ planes)
    const int segments = 24;

    // XY plane
    for (int i = 0; i < segments; ++i) {
        float angle1 = (i / float(segments)) * 2.0f * glm::pi<float>();
        float angle2 = ((i + 1) / float(segments)) * 2.0f * glm::pi<float>();

        Vec3 p1 = center + Vec3{radius * std::cos(angle1), radius * std::sin(angle1), 0.0f};
        Vec3 p2 = center + Vec3{radius * std::cos(angle2), radius * std::sin(angle2), 0.0f};
        debugDrawLine(p1, p2, color, duration, depthTest);
    }

    // XZ plane
    for (int i = 0; i < segments; ++i) {
        float angle1 = (i / float(segments)) * 2.0f * glm::pi<float>();
        float angle2 = ((i + 1) / float(segments)) * 2.0f * glm::pi<float>();

        Vec3 p1 = center + Vec3{radius * std::cos(angle1), 0.0f, radius * std::sin(angle1)};
        Vec3 p2 = center + Vec3{radius * std::cos(angle2), 0.0f, radius * std::sin(angle2)};
        debugDrawLine(p1, p2, color, duration, depthTest);
    }

    // YZ plane
    for (int i = 0; i < segments; ++i) {
        float angle1 = (i / float(segments)) * 2.0f * glm::pi<float>();
        float angle2 = ((i + 1) / float(segments)) * 2.0f * glm::pi<float>();

        Vec3 p1 = center + Vec3{0.0f, radius * std::cos(angle1), radius * std::sin(angle1)};
        Vec3 p2 = center + Vec3{0.0f, radius * std::cos(angle2), radius * std::sin(angle2)};
        debugDrawLine(p1, p2, color, duration, depthTest);
    }
}

void OpenGLGraphics3DSystem::debugDrawCapsule(
    const Vec3& start,
    const Vec3& end,
    float radius,
    const Color& color,
    float duration,
    bool depthTest) {

    // TODO: Implement capsule debug drawing
}

void OpenGLGraphics3DSystem::debugDrawFrustum(
    const Frustum& frustum,
    const Color& color,
    float duration,
    bool depthTest) {

    // TODO: Implement frustum debug drawing
}

void OpenGLGraphics3DSystem::debugDrawRay(
    const Vec3& origin,
    const Vec3& direction,
    float length,
    const Color& color,
    float duration,
    bool depthTest) {

    Vec3 end = origin + direction * length;
    debugDrawLine(origin, end, color, duration, depthTest);
}

void OpenGLGraphics3DSystem::debugDrawAxes(
    const Transform3D& transform,
    float size,
    float duration,
    bool depthTest) {

    Vec3 origin = transform.position;
    glm::mat4 rotMat = glm::mat4_cast(transform.rotation);

    Vec3 right = glm::vec3(rotMat * glm::vec4(size, 0, 0, 0));
    Vec3 up = glm::vec3(rotMat * glm::vec4(0, size, 0, 0));
    Vec3 forward = glm::vec3(rotMat * glm::vec4(0, 0, size, 0));

    debugDrawLine(origin, origin + right, Color::red(), duration, depthTest);
    debugDrawLine(origin, origin + up, Color::green(), duration, depthTest);
    debugDrawLine(origin, origin + forward, Color::blue(), duration, depthTest);
}

void OpenGLGraphics3DSystem::debugDrawAABB(
    const AABB3D& aabb,
    const Color& color,
    float duration,
    bool depthTest) {

    Vec3 center = aabb.center();
    Vec3 extents = aabb.extents();
    debugDrawBox(center, extents, Quat{1, 0, 0, 0}, color, duration, depthTest);
}

void OpenGLGraphics3DSystem::debugClear() {
    debugLines_.clear();
    persistentDebugLines_.clear();
}

void OpenGLGraphics3DSystem::setDebugRenderingEnabled(bool enabled) {
    debugRenderingEnabled_ = enabled;
}

bool OpenGLGraphics3DSystem::isDebugRenderingEnabled() const {
    return debugRenderingEnabled_;
}

//==========================================================================
// Window Management
//==========================================================================

Size OpenGLGraphics3DSystem::getWindowSize() const {
    return windowSize_;
}

void OpenGLGraphics3DSystem::setWindowSize(Size size) {
    windowSize_ = size;
    if (window_) {
        glfwSetWindowSize(window_, size.width, size.height);
    }
}

bool OpenGLGraphics3DSystem::isFullscreen() const {
    return isFullscreen_;
}

void OpenGLGraphics3DSystem::setFullscreen(bool fullscreen) {
    // TODO: Implement fullscreen toggle
    isFullscreen_ = fullscreen;
}

bool OpenGLGraphics3DSystem::shouldClose() const {
    return window_ ? glfwWindowShouldClose(window_) : false;
}

void* OpenGLGraphics3DSystem::getNativeWindowHandle() const {
    return window_;
}

//==========================================================================
// Render State
//==========================================================================

void OpenGLGraphics3DSystem::setClearColor(const Color& color) {
    clearColor_ = color;
}

void OpenGLGraphics3DSystem::setVSync(bool enabled) {
    glfwSwapInterval(enabled ? 1 : 0);
}

void OpenGLGraphics3DSystem::setRenderScale(float scale) {
    renderScale_ = scale;
}

float OpenGLGraphics3DSystem::getRenderScale() const {
    return renderScale_;
}

//==========================================================================
// Shader System Integration
//==========================================================================

void OpenGLGraphics3DSystem::drawMeshWithShaderMaterial(
    MeshHandle mesh,
    ShaderProgramHandle shader,
    const Mat4& worldMatrix,
    bool /*castShadow*/,
    bool /*receiveShadow*/)
{
    if (!pIShaderSystem_) return;

    auto meshIt = meshes_.find(mesh);
    if (meshIt == meshes_.end()) return;

    const auto& meshRes = meshIt->second;

    // Bind the shader
    pIShaderSystem_->bindShader(shader);

    // Set standard uniforms that all shaders expect
    pIShaderSystem_->setUniform("uModel", worldMatrix);
    pIShaderSystem_->setUniform("uView", viewMatrix_);
    pIShaderSystem_->setUniform("uProjection", projectionMatrix_);

    // Calculate normal matrix
    Mat3 normalMatrix = glm::transpose(glm::inverse(glm::mat3(worldMatrix)));
    pIShaderSystem_->setUniform("uNormalMatrix", normalMatrix);

    // Set camera and lighting uniforms
    pIShaderSystem_->setUniform("uCameraPos", camera_.transform.position);
    if (directionalLight_) {
        pIShaderSystem_->setUniform("uLightDir", directionalLight_->direction);
        pIShaderSystem_->setUniform("uLightColor", directionalLight_->color * directionalLight_->intensity);
    } else {
        pIShaderSystem_->setUniform("uLightDir", Vec3(0.0f, -1.0f, 0.0f));
        pIShaderSystem_->setUniform("uLightColor", Vec3(1.0f, 1.0f, 1.0f));
    }
    pIShaderSystem_->setUniform("uAmbientColor", ambientColor_);
    pIShaderSystem_->setUniform("uTime", static_cast<float>(glfwGetTime()));

    // Draw the mesh
    glBindVertexArray(meshRes.vao);
    glDrawElements(GL_TRIANGLES, meshRes.indexCount, GL_UNSIGNED_INT, nullptr);
    glBindVertexArray(0);

    stats_.drawCalls++;
    stats_.triangles += meshRes.indexCount / 3;
    stats_.vertices += meshRes.vertexCount;
}

Result<void, Graphics3DError> OpenGLGraphics3DSystem::drawMeshWithLuaMaterial(
    MeshHandle mesh,
    std::string_view materialPath,
    const Mat4& worldMatrix)
{
    if (!pIShaderSystem_) {
        return std::unexpected(Graphics3DError::InvalidShader);
    }

    // Check cache first
    std::string pathStr(materialPath);
    auto cacheIt = luaMaterialCache_.find(pathStr);
    MaterialHandle material = 0;

    if (cacheIt != luaMaterialCache_.end()) {
        material = cacheIt->second;
    } else {
        // Load the material
        auto result = pIShaderSystem_->loadMaterial(materialPath);
        if (!result) {
            return std::unexpected(Graphics3DError::InvalidShader);
        }
        material = *result;
        luaMaterialCache_[pathStr] = material;
    }

    // Bind the material (sets shader and all uniforms)
    pIShaderSystem_->bindMaterial(material);

    // Now draw with the material's shader
    auto meshIt = meshes_.find(mesh);
    if (meshIt == meshes_.end()) {
        return std::unexpected(Graphics3DError::InvalidMesh);
    }

    const auto& meshRes = meshIt->second;

    // Set standard uniforms
    pIShaderSystem_->setUniform("uModel", worldMatrix);
    pIShaderSystem_->setUniform("uView", viewMatrix_);
    pIShaderSystem_->setUniform("uProjection", projectionMatrix_);

    Mat3 normalMatrix = glm::transpose(glm::inverse(glm::mat3(worldMatrix)));
    pIShaderSystem_->setUniform("uNormalMatrix", normalMatrix);
    pIShaderSystem_->setUniform("uCameraPos", camera_.transform.position);
    if (directionalLight_) {
        pIShaderSystem_->setUniform("uLightDir", directionalLight_->direction);
        pIShaderSystem_->setUniform("uLightColor", directionalLight_->color * directionalLight_->intensity);
    } else {
        pIShaderSystem_->setUniform("uLightDir", Vec3(0.0f, -1.0f, 0.0f));
        pIShaderSystem_->setUniform("uLightColor", Vec3(1.0f, 1.0f, 1.0f));
    }
    pIShaderSystem_->setUniform("uAmbientColor", ambientColor_);
    pIShaderSystem_->setUniform("uTime", static_cast<float>(glfwGetTime()));

    // Draw
    glBindVertexArray(meshRes.vao);
    glDrawElements(GL_TRIANGLES, meshRes.indexCount, GL_UNSIGNED_INT, nullptr);
    glBindVertexArray(0);

    stats_.drawCalls++;
    stats_.triangles += meshRes.indexCount / 3;
    stats_.vertices += meshRes.vertexCount;

    return {};
}

Result<void, Graphics3DError> OpenGLGraphics3DSystem::drawMeshWithLuaMaterial(
    MeshHandle mesh,
    std::string_view materialPath,
    const Mat4& worldMatrix,
    const Vec4& colorOverride)
{
    if (!pIShaderSystem_) {
        return std::unexpected(Graphics3DError::InvalidShader);
    }

    // Check cache first
    std::string pathStr(materialPath);
    auto cacheIt = luaMaterialCache_.find(pathStr);
    MaterialHandle material = 0;

    if (cacheIt != luaMaterialCache_.end()) {
        material = cacheIt->second;
    } else {
        // Load the material
        auto result = pIShaderSystem_->loadMaterial(materialPath);
        if (!result) {
            return std::unexpected(Graphics3DError::InvalidShader);
        }
        material = *result;
        luaMaterialCache_[pathStr] = material;
    }

    // Bind the material (sets shader and all uniforms from Lua)
    pIShaderSystem_->bindMaterial(material);

    // Override the base color with per-object color
    pIShaderSystem_->setUniform("uBaseColor", colorOverride);

    // Now draw with the material's shader
    auto meshIt = meshes_.find(mesh);
    if (meshIt == meshes_.end()) {
        return std::unexpected(Graphics3DError::InvalidMesh);
    }

    const auto& meshRes = meshIt->second;

    // Set standard uniforms
    pIShaderSystem_->setUniform("uModel", worldMatrix);
    pIShaderSystem_->setUniform("uView", viewMatrix_);
    pIShaderSystem_->setUniform("uProjection", projectionMatrix_);

    Mat3 normalMatrix = glm::transpose(glm::inverse(glm::mat3(worldMatrix)));
    pIShaderSystem_->setUniform("uNormalMatrix", normalMatrix);
    pIShaderSystem_->setUniform("uCameraPos", camera_.transform.position);
    if (directionalLight_) {
        pIShaderSystem_->setUniform("uLightDir", directionalLight_->direction);
        pIShaderSystem_->setUniform("uLightColor", directionalLight_->color * directionalLight_->intensity);
    } else {
        pIShaderSystem_->setUniform("uLightDir", Vec3(0.0f, -1.0f, 0.0f));
        pIShaderSystem_->setUniform("uLightColor", Vec3(1.0f, 1.0f, 1.0f));
    }
    pIShaderSystem_->setUniform("uAmbientColor", ambientColor_);
    pIShaderSystem_->setUniform("uTime", static_cast<float>(glfwGetTime()));

    // Draw
    glBindVertexArray(meshRes.vao);
    glDrawElements(GL_TRIANGLES, meshRes.indexCount, GL_UNSIGNED_INT, nullptr);
    glBindVertexArray(0);

    stats_.drawCalls++;
    stats_.triangles += meshRes.indexCount / 3;
    stats_.vertices += meshRes.vertexCount;

    return {};
}

void OpenGLGraphics3DSystem::updateShaders() {
    if (pIShaderSystem_) {
        pIShaderSystem_->update();  // Check for hot reload
    }
}

Result<MeshHandle, Graphics3DError> OpenGLGraphics3DSystem::createMeshFromData(const MeshData& data) {
    if (data.vertices.empty()) {
        return std::unexpected(Graphics3DError::InvalidMesh);
    }

    // Convert MeshData vertices to Vertex3D
    std::vector<Vertex3D> vertices;
    vertices.reserve(data.vertices.size());

    for (const auto& v : data.vertices) {
        Vertex3D vertex;
        vertex.position = Vec3{v.position[0], v.position[1], v.position[2]};
        vertex.normal = Vec3{v.normal[0], v.normal[1], v.normal[2]};
        vertex.texCoord = Vec2{v.texCoord[0], v.texCoord[1]};
        vertex.color = Vec4{v.color[0], v.color[1], v.color[2], v.color[3]};
        vertices.push_back(vertex);
    }

    // Convert submeshes
    std::vector<SubMesh> subMeshes;
    for (const auto& sm : data.subMeshes) {
        SubMesh subMesh;
        subMesh.indexOffset = sm.indexOffset;
        subMesh.indexCount = sm.indexCount;
        subMesh.materialIndex = sm.materialIndex;
        subMesh.bounds.min = Vec3{sm.boundsMin[0], sm.boundsMin[1], sm.boundsMin[2]};
        subMesh.bounds.max = Vec3{sm.boundsMax[0], sm.boundsMax[1], sm.boundsMax[2]};
        subMeshes.push_back(subMesh);
    }

    AABB3D bounds;
    bounds.min = Vec3{data.boundsMin[0], data.boundsMin[1], data.boundsMin[2]};
    bounds.max = Vec3{data.boundsMax[0], data.boundsMax[1], data.boundsMax[2]};

    MeshDef def;
    def.vertices = vertices;
    def.indices = data.indices;
    def.subMeshes = std::move(subMeshes);
    def.bounds = bounds;
    def.isDynamic = false;

    return createMesh(def);
}

Result<MaterialHandle, Graphics3DError> OpenGLGraphics3DSystem::createMaterialFromData(const MaterialData& data) {
    if (data.unlit) {
        UnlitMaterial mat;
        mat.color = Vec4{data.baseColorFactor[0], data.baseColorFactor[1],
                         data.baseColorFactor[2], data.baseColorFactor[3]};
        mat.blendMode = data.transparent ? BlendMode::AlphaBlend : BlendMode::Opaque;
        mat.cullMode = data.doubleSided ? CullMode::None : CullMode::Back;
        return createUnlitMaterial(mat);
    }

    PBRMaterial mat;
    mat.baseColorFactor = Vec4{data.baseColorFactor[0], data.baseColorFactor[1],
                               data.baseColorFactor[2], data.baseColorFactor[3]};
    mat.metallicFactor = data.metallicFactor;
    mat.roughnessFactor = data.roughnessFactor;
    mat.normalScale = data.normalScale;
    mat.occlusionStrength = data.occlusionStrength;
    mat.emissiveFactor = Vec3{data.emissiveFactor[0], data.emissiveFactor[1], data.emissiveFactor[2]};
    mat.alphaCutoff = data.alphaCutoff;
    mat.doubleSided = data.doubleSided;
    mat.blendMode = data.transparent ? BlendMode::AlphaBlend : BlendMode::Opaque;
    mat.cullMode = data.doubleSided ? CullMode::None : CullMode::Back;

    return createMaterial(mat);
}

Result<std::vector<MaterialHandle>, Graphics3DError> OpenGLGraphics3DSystem::createMaterialsFromModel(const ModelData& data) {
    std::vector<MaterialHandle> handles;
    handles.reserve(data.materials.size());

    for (const auto& matData : data.materials) {
        auto result = createMaterialFromData(matData);
        if (!result) {
            return std::unexpected(result.error());
        }
        handles.push_back(*result);
    }

    return handles;
}

Result<void, Graphics3DError> OpenGLGraphics3DSystem::createSkyboxFromData(const CubemapData& data) {
    if (data.facePixels.size() != 6) {
        return std::unexpected(Graphics3DError::InvalidTexture);
    }

    // TODO: Create cubemap texture from data
    // For now, just store the skybox reference
    return {};
}

//==========================================================================
// Culling
//==========================================================================

void OpenGLGraphics3DSystem::setFrustumCulling(bool enabled) {
    frustumCullingEnabled_ = enabled;
}

bool OpenGLGraphics3DSystem::isFrustumCullingEnabled() const {
    return frustumCullingEnabled_;
}

//==========================================================================
// Post-Processing
//==========================================================================

void OpenGLGraphics3DSystem::setToneMapping(bool enabled) {
    toneMappingEnabled_ = enabled;
}

void OpenGLGraphics3DSystem::setExposure(float exposure) {
    exposure_ = exposure;
}

void OpenGLGraphics3DSystem::setBloom(bool enabled, float threshold, float intensity) {
    bloomEnabled_ = enabled;
    bloomThreshold_ = threshold;
    bloomIntensity_ = intensity;
}

void OpenGLGraphics3DSystem::setSSAO(bool enabled, float radius, float intensity) {
    ssaoEnabled_ = enabled;
    ssaoRadius_ = radius;
    ssaoIntensity_ = intensity;
}

//==========================================================================
// Statistics
//==========================================================================

RenderStats OpenGLGraphics3DSystem::getStats() const {
    return stats_;
}

//==========================================================================
// Helper Functions
//==========================================================================

bool OpenGLGraphics3DSystem::compileShader(GLuint shader, const char* source) {
    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);

    GLint success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);

    if (!success) {
        char infoLog[512];
        glGetShaderInfoLog(shader, 512, nullptr, infoLog);
        // TODO: Log error
        return false;
    }

    return true;
}

bool OpenGLGraphics3DSystem::linkProgram(GLuint program) {
    glLinkProgram(program);

    GLint success;
    glGetProgramiv(program, GL_LINK_STATUS, &success);

    if (!success) {
        char infoLog[512];
        glGetProgramInfoLog(program, 512, nullptr, infoLog);
        // TODO: Log error
        return false;
    }

    return true;
}

GLuint OpenGLGraphics3DSystem::createShaderProgram(const char* vertSource, const char* fragSource) {
    GLuint vertShader = glCreateShader(GL_VERTEX_SHADER);
    GLuint fragShader = glCreateShader(GL_FRAGMENT_SHADER);

    if (!compileShader(vertShader, vertSource)) {
        glDeleteShader(vertShader);
        glDeleteShader(fragShader);
        return 0;
    }

    if (!compileShader(fragShader, fragSource)) {
        glDeleteShader(vertShader);
        glDeleteShader(fragShader);
        return 0;
    }

    GLuint program = glCreateProgram();
    glAttachShader(program, vertShader);
    glAttachShader(program, fragShader);

    if (!linkProgram(program)) {
        glDeleteProgram(program);
        glDeleteShader(vertShader);
        glDeleteShader(fragShader);
        return 0;
    }

    glDeleteShader(vertShader);
    glDeleteShader(fragShader);

    return program;
}

void OpenGLGraphics3DSystem::createShaders() {
    pbrShader_.program = createShaderProgram(PBR_VERTEX_SHADER, PBR_FRAGMENT_SHADER);
    unlitShader_.program = createShaderProgram(UNLIT_VERTEX_SHADER, UNLIT_FRAGMENT_SHADER);
    debugLineShader_.program = createShaderProgram(DEBUG_LINE_VERTEX_SHADER, DEBUG_LINE_FRAGMENT_SHADER);
}

void OpenGLGraphics3DSystem::createDefaultMaterials() {
    // Default PBR material (white, non-metallic)
    PBRMaterial defaultPBR;
    defaultPBR.baseColorFactor = Vec4{1.0f, 1.0f, 1.0f, 1.0f};
    defaultPBR.metallicFactor = 0.0f;
    defaultPBR.roughnessFactor = 0.5f;
    auto result = createMaterial(defaultPBR);
    if (result) {
        defaultPBRMaterial_ = *result;
    }

    // Default unlit material (white)
    UnlitMaterial defaultUnlit;
    defaultUnlit.color = Vec4{1.0f, 1.0f, 1.0f, 1.0f};
    auto unlitResult = createUnlitMaterial(defaultUnlit);
    if (unlitResult) {
        defaultUnlitMaterial_ = *unlitResult;
    }

    // Error material (magenta, for missing textures/materials)
    PBRMaterial errorMat;
    errorMat.baseColorFactor = Vec4{1.0f, 0.0f, 1.0f, 1.0f};
    auto errorResult = createMaterial(errorMat);
    if (errorResult) {
        errorMaterial_ = *errorResult;
    }
}

void OpenGLGraphics3DSystem::createDebugResources() {
    glGenVertexArrays(1, &debugVAO_);
    glGenBuffers(1, &debugVBO_);
}

void OpenGLGraphics3DSystem::updateViewFrustum() {
    // Extract frustum planes from view-projection matrix using Gribb-Hartmann method
    // Each row of the combined matrix gives us the plane coefficients
    const glm::mat4& m = viewProjectionMatrix_;

    // Left plane: row 4 + row 1
    viewFrustum_.planes[2].normal.x = m[0][3] + m[0][0];
    viewFrustum_.planes[2].normal.y = m[1][3] + m[1][0];
    viewFrustum_.planes[2].normal.z = m[2][3] + m[2][0];
    viewFrustum_.planes[2].distance = m[3][3] + m[3][0];

    // Right plane: row 4 - row 1
    viewFrustum_.planes[3].normal.x = m[0][3] - m[0][0];
    viewFrustum_.planes[3].normal.y = m[1][3] - m[1][0];
    viewFrustum_.planes[3].normal.z = m[2][3] - m[2][0];
    viewFrustum_.planes[3].distance = m[3][3] - m[3][0];

    // Bottom plane: row 4 + row 2
    viewFrustum_.planes[5].normal.x = m[0][3] + m[0][1];
    viewFrustum_.planes[5].normal.y = m[1][3] + m[1][1];
    viewFrustum_.planes[5].normal.z = m[2][3] + m[2][1];
    viewFrustum_.planes[5].distance = m[3][3] + m[3][1];

    // Top plane: row 4 - row 2
    viewFrustum_.planes[4].normal.x = m[0][3] - m[0][1];
    viewFrustum_.planes[4].normal.y = m[1][3] - m[1][1];
    viewFrustum_.planes[4].normal.z = m[2][3] - m[2][1];
    viewFrustum_.planes[4].distance = m[3][3] - m[3][1];

    // Near plane: row 4 + row 3
    viewFrustum_.planes[0].normal.x = m[0][3] + m[0][2];
    viewFrustum_.planes[0].normal.y = m[1][3] + m[1][2];
    viewFrustum_.planes[0].normal.z = m[2][3] + m[2][2];
    viewFrustum_.planes[0].distance = m[3][3] + m[3][2];

    // Far plane: row 4 - row 3
    viewFrustum_.planes[1].normal.x = m[0][3] - m[0][2];
    viewFrustum_.planes[1].normal.y = m[1][3] - m[1][2];
    viewFrustum_.planes[1].normal.z = m[2][3] - m[2][2];
    viewFrustum_.planes[1].distance = m[3][3] - m[3][2];

    // Normalize all planes
    for (int i = 0; i < 6; ++i) {
        float len = std::sqrt(
            viewFrustum_.planes[i].normal.x * viewFrustum_.planes[i].normal.x +
            viewFrustum_.planes[i].normal.y * viewFrustum_.planes[i].normal.y +
            viewFrustum_.planes[i].normal.z * viewFrustum_.planes[i].normal.z
        );
        if (len > 0.0001f) {
            viewFrustum_.planes[i].normal.x /= len;
            viewFrustum_.planes[i].normal.y /= len;
            viewFrustum_.planes[i].normal.z /= len;
            viewFrustum_.planes[i].distance /= len;
        }
    }
}

glm::mat4 OpenGLGraphics3DSystem::transformToMatrix(const Transform3D& transform) const {
    glm::mat4 translation = glm::translate(glm::mat4(1.0f), transform.position);
    glm::mat4 rotation = glm::mat4_cast(transform.rotation);
    glm::mat4 scale = glm::scale(glm::mat4(1.0f), transform.scale);

    return translation * rotation * scale;
}

void OpenGLGraphics3DSystem::renderMeshInternal(
    const MeshResource& mesh,
    const MaterialResource& material,
    const glm::mat4& worldMatrix) {

    // Select shader
    ShaderProgram* shader = material.isUnlit ? &unlitShader_ : &pbrShader_;
    shader->use();

    // Set transforms
    shader->setMat4("uModel", worldMatrix);
    shader->setMat4("uView", viewMatrix_);
    shader->setMat4("uProjection", projectionMatrix_);

    if (!material.isUnlit) {
        glm::mat3 normalMatrix = glm::transpose(glm::inverse(glm::mat3(worldMatrix)));
        shader->setMat3("uNormalMatrix", normalMatrix);

        // Set material properties
        shader->setVec4("uBaseColor", material.baseColor);
        shader->setFloat("uMetallic", material.metallic);
        shader->setFloat("uRoughness", material.roughness);
        shader->setVec3("uEmissive", material.emissive);

        // Set lighting
        shader->setVec3("uCameraPos", camera_.transform.position);

        if (directionalLight_) {
            shader->setVec3("uLightDir", directionalLight_->direction);
            shader->setVec3("uLightColor", directionalLight_->color * directionalLight_->intensity);
        } else {
            shader->setVec3("uLightDir", Vec3{0, -1, 0});
            shader->setVec3("uLightColor", Vec3{1, 1, 1});
        }

        shader->setVec3("uAmbient", ambientColor_ * ambientIntensity_);

        // Set textures
        shader->setInt("uHasBaseColorTex", material.baseColorTex != 0 ? 1 : 0);
        if (material.baseColorTex) {
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, material.baseColorTex);
            shader->setInt("uBaseColorTex", 0);
        }
    } else {
        shader->setVec4("uColor", material.baseColor);
        shader->setInt("uHasTexture", material.baseColorTex != 0 ? 1 : 0);
        if (material.baseColorTex) {
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, material.baseColorTex);
            shader->setInt("uTexture", 0);
        }
    }

    // Apply blend mode
    applyBlendMode(material.blendMode);
    applyCullMode(material.cullMode);

    // Draw mesh
    glBindVertexArray(mesh.vao);

    if (mesh.ebo) {
        glDrawElements(GL_TRIANGLES, mesh.indexCount, GL_UNSIGNED_INT, nullptr);
    } else {
        glDrawArrays(GL_TRIANGLES, 0, mesh.vertexCount);
    }

    glBindVertexArray(0);

    stats_.drawCalls++;
    stats_.triangles += mesh.indexCount / 3;
    stats_.vertices += mesh.vertexCount;
}

void OpenGLGraphics3DSystem::renderDebugLines() {
    if (debugLines_.empty() && persistentDebugLines_.empty()) {
        return;
    }

    // Combine all debug lines
    std::vector<DebugLineInternal> allLines = debugLines_;
    allLines.insert(allLines.end(), persistentDebugLines_.begin(), persistentDebugLines_.end());

    // Build vertex data (position + color)
    std::vector<float> vertices;
    vertices.reserve(allLines.size() * 14); // 2 vertices * 7 floats per vertex

    for (const auto& line : allLines) {
        // Start vertex
        vertices.push_back(line.start.x);
        vertices.push_back(line.start.y);
        vertices.push_back(line.start.z);
        vertices.push_back(line.color.r);
        vertices.push_back(line.color.g);
        vertices.push_back(line.color.b);
        vertices.push_back(line.color.a);

        // End vertex
        vertices.push_back(line.end.x);
        vertices.push_back(line.end.y);
        vertices.push_back(line.end.z);
        vertices.push_back(line.color.r);
        vertices.push_back(line.color.g);
        vertices.push_back(line.color.b);
        vertices.push_back(line.color.a);
    }

    // Upload to GPU
    glBindVertexArray(debugVAO_);
    glBindBuffer(GL_ARRAY_BUFFER, debugVBO_);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_DYNAMIC_DRAW);

    // Position attribute
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 7 * sizeof(float), (void*)0);

    // Color attribute
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 7 * sizeof(float), (void*)(3 * sizeof(float)));

    // Render
    debugLineShader_.use();
    debugLineShader_.setMat4("uViewProjection", viewProjectionMatrix_);

    glDrawArrays(GL_LINES, 0, static_cast<GLsizei>(allLines.size() * 2));

    glBindVertexArray(0);
}

void OpenGLGraphics3DSystem::updatePersistentDebugLines(float deltaTime) {
    // Update durations and remove expired lines
    auto it = std::remove_if(persistentDebugLines_.begin(), persistentDebugLines_.end(),
        [deltaTime](DebugLineInternal& line) {
            line.duration -= deltaTime;
            return line.duration <= 0.0f;
        });

    persistentDebugLines_.erase(it, persistentDebugLines_.end());
}

void OpenGLGraphics3DSystem::applyBlendMode(BlendMode mode) {
    switch (mode) {
        case BlendMode::Opaque:
            glDisable(GL_BLEND);
            break;
        case BlendMode::AlphaTest:
            // Treat as opaque for now (alpha testing would require shader support)
            glDisable(GL_BLEND);
            break;
        case BlendMode::AlphaBlend:
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            break;
        case BlendMode::Additive:
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE);
            break;
        case BlendMode::Multiply:
            glEnable(GL_BLEND);
            glBlendFunc(GL_DST_COLOR, GL_ZERO);
            break;
    }
}

void OpenGLGraphics3DSystem::applyCullMode(CullMode mode) {
    switch (mode) {
        case CullMode::None:
            glDisable(GL_CULL_FACE);
            break;
        case CullMode::Front:
            glEnable(GL_CULL_FACE);
            glCullFace(GL_FRONT);
            break;
        case CullMode::Back:
            glEnable(GL_CULL_FACE);
            glCullFace(GL_BACK);
            break;
    }
}

//==========================================================================
// Instanced Rendering
//==========================================================================

Result<InstanceBufferHandle, Graphics3DError> OpenGLGraphics3DSystem::createInstanceBuffer(
    uint32_t maxInstances,
    bool dynamic) {

    if (maxInstances == 0) {
        return std::unexpected(Graphics3DError::InternalError);
    }

    InstanceBufferResource resource;
    resource.maxInstances = maxInstances;
    resource.currentCount = 0;
    resource.dynamic = dynamic;

    // Create VBO for instance data
    glGenBuffers(1, &resource.vbo);
    glBindBuffer(GL_ARRAY_BUFFER, resource.vbo);

    // InstanceData contains: Mat4 worldMatrix (16 floats) + Vec4 customData (4 floats) = 20 floats
    const size_t instanceDataSize = sizeof(InstanceData);
    glBufferData(GL_ARRAY_BUFFER,
                 static_cast<GLsizeiptr>(maxInstances * instanceDataSize),
                 nullptr,
                 dynamic ? GL_DYNAMIC_DRAW : GL_STATIC_DRAW);

    glBindBuffer(GL_ARRAY_BUFFER, 0);

    InstanceBufferHandle handle = nextInstanceBufferHandle_++;
    instanceBuffers_[handle] = resource;

    return handle;
}

Result<void, Graphics3DError> OpenGLGraphics3DSystem::updateInstanceBuffer(
    InstanceBufferHandle buffer,
    std::span<const InstanceData> data,
    uint32_t offset) {

    auto it = instanceBuffers_.find(buffer);
    if (it == instanceBuffers_.end()) {
        return std::unexpected(Graphics3DError::InternalError);
    }

    auto& resource = it->second;

    if (offset + data.size() > resource.maxInstances) {
        return std::unexpected(Graphics3DError::InternalError);
    }

    glBindBuffer(GL_ARRAY_BUFFER, resource.vbo);
    glBufferSubData(GL_ARRAY_BUFFER,
                    static_cast<GLintptr>(offset * sizeof(InstanceData)),
                    static_cast<GLsizeiptr>(data.size() * sizeof(InstanceData)),
                    data.data());
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    resource.currentCount = static_cast<uint32_t>(offset + data.size());

    return {};
}

void OpenGLGraphics3DSystem::destroyInstanceBuffer(InstanceBufferHandle buffer) {
    auto it = instanceBuffers_.find(buffer);
    if (it != instanceBuffers_.end()) {
        if (it->second.vbo) {
            glDeleteBuffers(1, &it->second.vbo);
        }
        instanceBuffers_.erase(it);
    }
}

void OpenGLGraphics3DSystem::drawInstanced(const InstancedRenderItem& item) {
    // Find mesh
    auto meshIt = meshes_.find(item.mesh);
    if (meshIt == meshes_.end()) return;

    // Find material
    auto matIt = materials_.find(item.material);
    if (matIt == materials_.end()) {
        matIt = materials_.find(defaultPBRMaterial_);
        if (matIt == materials_.end()) return;
    }

    // Find instance buffer
    auto instIt = instanceBuffers_.find(item.instances);
    if (instIt == instanceBuffers_.end()) return;

    const auto& mesh = meshIt->second;
    const auto& material = matIt->second;
    const auto& instanceBuffer = instIt->second;

    // Select shader
    ShaderProgram& shader = material.isUnlit ? unlitShader_ : pbrShader_;
    shader.use();

    // Set view/projection uniforms
    shader.setMat4("uView", viewMatrix_);
    shader.setMat4("uProjection", projectionMatrix_);

    // Set material uniforms
    shader.setVec4("uBaseColor", glm::vec4(material.baseColor.x, material.baseColor.y,
                                            material.baseColor.z, material.baseColor.w));
    if (!material.isUnlit) {
        shader.setFloat("uMetallic", material.metallic);
        shader.setFloat("uRoughness", material.roughness);
        shader.setVec3("uEmissive", glm::vec3(material.emissive.x, material.emissive.y,
                                               material.emissive.z));
    }

    applyBlendMode(material.blendMode);
    applyCullMode(material.cullMode);

    // Bind mesh VAO
    glBindVertexArray(mesh.vao);

    // Bind instance buffer and set up instanced attributes
    glBindBuffer(GL_ARRAY_BUFFER, instanceBuffer.vbo);

    // Instance attributes start at location 4 (after position, normal, texcoord, color)
    // Mat4 takes 4 vec4 attribute slots (locations 4-7)
    for (int i = 0; i < 4; ++i) {
        GLuint loc = 4 + i;
        glEnableVertexAttribArray(loc);
        glVertexAttribPointer(loc, 4, GL_FLOAT, GL_FALSE,
                              sizeof(InstanceData),
                              reinterpret_cast<void*>(i * sizeof(glm::vec4)));
        glVertexAttribDivisor(loc, 1);  // One per instance
    }

    // Custom data at location 8
    glEnableVertexAttribArray(8);
    glVertexAttribPointer(8, 4, GL_FLOAT, GL_FALSE,
                          sizeof(InstanceData),
                          reinterpret_cast<void*>(sizeof(Mat4)));
    glVertexAttribDivisor(8, 1);

    // Draw instanced
    glDrawElementsInstanced(GL_TRIANGLES,
                            static_cast<GLsizei>(mesh.indexCount),
                            GL_UNSIGNED_INT,
                            nullptr,
                            static_cast<GLsizei>(item.instanceCount));

    // Reset vertex attrib divisors
    for (int i = 4; i <= 8; ++i) {
        glVertexAttribDivisor(i, 0);
        glDisableVertexAttribArray(i);
    }

    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    stats_.drawCalls++;
    stats_.triangles += (mesh.indexCount / 3) * item.instanceCount;
}

void OpenGLGraphics3DSystem::queueInstancedRenderItem(const InstancedRenderItem& item) {
    instancedRenderQueue_.push_back(item);
}

//==========================================================================
// Skeletal Animation
//==========================================================================

Result<SkeletonHandle, Graphics3DError> OpenGLGraphics3DSystem::createSkeleton(const ModelData& modelData) {
    if (modelData.bones.empty()) {
        return std::unexpected(Graphics3DError::InvalidMesh);
    }

    SkeletonResource skeleton;
    skeleton.bones.reserve(modelData.bones.size());

    for (size_t i = 0; i < modelData.bones.size(); ++i) {
        const auto& srcBone = modelData.bones[i];
        BoneData bone;
        bone.name = srcBone.name;
        bone.parentIndex = srcBone.parentIndex;

        // Convert offset matrix from array to Mat4
        for (int j = 0; j < 16; ++j) {
            bone.offsetMatrix[j / 4][j % 4] = srcBone.offsetMatrix[j];
        }

        skeleton.boneNameToIndex[bone.name] = static_cast<int32_t>(i);
        skeleton.bones.push_back(bone);
    }

    SkeletonHandle handle = nextSkeletonHandle_++;
    skeletons_[handle] = std::move(skeleton);

    return handle;
}

Result<AnimationClipHandle, Graphics3DError> OpenGLGraphics3DSystem::createAnimationClip(
    SkeletonHandle skeleton,
    const std::string& clipName,
    const ModelData& modelData) {

    auto skelIt = skeletons_.find(skeleton);
    if (skelIt == skeletons_.end()) {
        return std::unexpected(Graphics3DError::InternalError);
    }

    // Find the animation in modelData
    const ModelData::Animation* foundAnim = nullptr;
    for (const auto& anim : modelData.animations) {
        if (anim.name == clipName) {
            foundAnim = &anim;
            break;
        }
    }

    if (!foundAnim) {
        return std::unexpected(Graphics3DError::InternalError);
    }

    AnimationClipResource clip;
    clip.name = foundAnim->name;
    clip.duration = foundAnim->duration;
    clip.ticksPerSecond = foundAnim->ticksPerSecond;
    clip.skeleton = skeleton;
    clip.channels.reserve(foundAnim->channels.size());

    for (const auto& srcChannel : foundAnim->channels) {
        BoneAnimationChannel channel;
        channel.boneIndex = srcChannel.boneIndex;
        channel.keyframes.reserve(srcChannel.keyframes.size());

        for (const auto& srcKey : srcChannel.keyframes) {
            AnimationKeyframe key;
            key.time = srcKey.time;
            key.translation = Vec3{srcKey.translation[0], srcKey.translation[1], srcKey.translation[2]};
            key.rotation = Quat{srcKey.rotation[3], srcKey.rotation[0], srcKey.rotation[1], srcKey.rotation[2]};
            key.scale = Vec3{srcKey.scale[0], srcKey.scale[1], srcKey.scale[2]};
            channel.keyframes.push_back(key);
        }

        clip.channels.push_back(std::move(channel));
    }

    AnimationClipHandle handle = nextAnimationClipHandle_++;
    animationClips_[handle] = std::move(clip);

    // Associate clip with skeleton
    skelIt->second.clips.push_back(handle);

    return handle;
}

void OpenGLGraphics3DSystem::destroySkeleton(SkeletonHandle skeleton) {
    auto it = skeletons_.find(skeleton);
    if (it != skeletons_.end()) {
        // Destroy associated animation clips
        for (auto clipHandle : it->second.clips) {
            animationClips_.erase(clipHandle);
        }
        skeletons_.erase(it);
    }
}

void OpenGLGraphics3DSystem::destroyAnimationClip(AnimationClipHandle clip) {
    auto it = animationClips_.find(clip);
    if (it != animationClips_.end()) {
        // Remove from skeleton's clip list
        auto skelIt = skeletons_.find(it->second.skeleton);
        if (skelIt != skeletons_.end()) {
            auto& clips = skelIt->second.clips;
            clips.erase(std::remove(clips.begin(), clips.end(), clip), clips.end());
        }
        animationClips_.erase(it);
    }
}

std::vector<std::string> OpenGLGraphics3DSystem::getAnimationClipNames(SkeletonHandle skeleton) const {
    std::vector<std::string> names;
    auto skelIt = skeletons_.find(skeleton);
    if (skelIt != skeletons_.end()) {
        for (auto clipHandle : skelIt->second.clips) {
            auto clipIt = animationClips_.find(clipHandle);
            if (clipIt != animationClips_.end()) {
                names.push_back(clipIt->second.name);
            }
        }
    }
    return names;
}

AnimationClip OpenGLGraphics3DSystem::getAnimationClipInfo(AnimationClipHandle clip) const {
    auto it = animationClips_.find(clip);
    if (it != animationClips_.end()) {
        AnimationClip info;
        info.name = it->second.name;
        info.duration = it->second.duration;
        info.ticksPerSecond = it->second.ticksPerSecond;
        info.looping = true;  // Default
        return info;
    }
    return AnimationClip{};
}

std::vector<Mat4> OpenGLGraphics3DSystem::sampleAnimation(
    AnimationClipHandle clip,
    float time,
    bool loop) {

    auto clipIt = animationClips_.find(clip);
    if (clipIt == animationClips_.end()) {
        return {};
    }

    const auto& clipRes = clipIt->second;
    auto skelIt = skeletons_.find(clipRes.skeleton);
    if (skelIt == skeletons_.end()) {
        return {};
    }

    const auto& skeleton = skelIt->second;
    std::vector<Mat4> boneTransforms(skeleton.bones.size());

    // Normalize time
    float animTime = time;
    if (clipRes.duration > 0.0f) {
        if (loop) {
            animTime = std::fmod(time, clipRes.duration);
        } else {
            animTime = std::min(time, clipRes.duration);
        }
    }

    // Initialize with identity transforms
    for (size_t i = 0; i < skeleton.bones.size(); ++i) {
        boneTransforms[i] = Mat4{1.0f};
    }

    // Sample each channel
    for (const auto& channel : clipRes.channels) {
        if (channel.boneIndex < 0 || channel.boneIndex >= static_cast<int32_t>(skeleton.bones.size())) {
            continue;
        }

        if (channel.keyframes.empty()) {
            continue;
        }

        // Find keyframes to interpolate between
        size_t keyIndex = 0;
        for (size_t i = 0; i < channel.keyframes.size() - 1; ++i) {
            if (animTime < channel.keyframes[i + 1].time) {
                keyIndex = i;
                break;
            }
            keyIndex = i;
        }

        const auto& key0 = channel.keyframes[keyIndex];
        const auto& key1 = channel.keyframes[std::min(keyIndex + 1, channel.keyframes.size() - 1)];

        // Calculate interpolation factor
        float t = 0.0f;
        float deltaTime = key1.time - key0.time;
        if (deltaTime > 0.0001f) {
            t = (animTime - key0.time) / deltaTime;
            t = std::clamp(t, 0.0f, 1.0f);
        }

        // Interpolate translation
        Vec3 translation = {
            key0.translation.x + (key1.translation.x - key0.translation.x) * t,
            key0.translation.y + (key1.translation.y - key0.translation.y) * t,
            key0.translation.z + (key1.translation.z - key0.translation.z) * t
        };

        // Interpolate rotation (slerp)
        glm::quat q0(key0.rotation.w, key0.rotation.x, key0.rotation.y, key0.rotation.z);
        glm::quat q1(key1.rotation.w, key1.rotation.x, key1.rotation.y, key1.rotation.z);
        glm::quat qInterp = glm::slerp(q0, q1, t);

        // Interpolate scale
        Vec3 scale = {
            key0.scale.x + (key1.scale.x - key0.scale.x) * t,
            key0.scale.y + (key1.scale.y - key0.scale.y) * t,
            key0.scale.z + (key1.scale.z - key0.scale.z) * t
        };

        // Build local transform matrix
        glm::mat4 localTransform = glm::translate(glm::mat4(1.0f),
            glm::vec3(translation.x, translation.y, translation.z));
        localTransform *= glm::mat4_cast(qInterp);
        localTransform = glm::scale(localTransform, glm::vec3(scale.x, scale.y, scale.z));

        // Store in boneTransforms (will be combined with parent later)
        for (int i = 0; i < 4; ++i) {
            for (int j = 0; j < 4; ++j) {
                boneTransforms[channel.boneIndex][i][j] = localTransform[i][j];
            }
        }
    }

    // Apply parent transforms (from root to leaves)
    for (size_t i = 0; i < skeleton.bones.size(); ++i) {
        int32_t parentIdx = skeleton.bones[i].parentIndex;
        if (parentIdx >= 0 && parentIdx < static_cast<int32_t>(skeleton.bones.size())) {
            // Convert Mat4 to glm::mat4 for multiplication
            glm::mat4 parentMat, localMat;
            for (int r = 0; r < 4; ++r) {
                for (int c = 0; c < 4; ++c) {
                    parentMat[r][c] = boneTransforms[parentIdx][r][c];
                    localMat[r][c] = boneTransforms[i][r][c];
                }
            }
            glm::mat4 result = parentMat * localMat;
            for (int r = 0; r < 4; ++r) {
                for (int c = 0; c < 4; ++c) {
                    boneTransforms[i][r][c] = result[r][c];
                }
            }
        }
    }

    // Apply offset matrices (inverse bind pose)
    for (size_t i = 0; i < skeleton.bones.size(); ++i) {
        glm::mat4 boneMat, offsetMat;
        for (int r = 0; r < 4; ++r) {
            for (int c = 0; c < 4; ++c) {
                boneMat[r][c] = boneTransforms[i][r][c];
                offsetMat[r][c] = skeleton.bones[i].offsetMatrix[r][c];
            }
        }
        glm::mat4 result = boneMat * offsetMat;
        for (int r = 0; r < 4; ++r) {
            for (int c = 0; c < 4; ++c) {
                boneTransforms[i][r][c] = result[r][c];
            }
        }
    }

    return boneTransforms;
}

std::vector<Mat4> OpenGLGraphics3DSystem::blendAnimations(const BlendedAnimation& blend) {
    if (blend.layers.empty()) {
        return {};
    }

    // Get bone count from first valid clip
    size_t boneCount = 0;
    for (const auto& layer : blend.layers) {
        auto clipIt = animationClips_.find(layer.clip);
        if (clipIt != animationClips_.end()) {
            auto skelIt = skeletons_.find(clipIt->second.skeleton);
            if (skelIt != skeletons_.end()) {
                boneCount = skelIt->second.bones.size();
                break;
            }
        }
    }

    if (boneCount == 0) {
        return {};
    }

    // Initialize result with identity matrices
    std::vector<Mat4> result(boneCount);
    for (auto& m : result) {
        m = Mat4{1.0f};
    }

    float totalWeight = 0.0f;

    // Blend each layer
    for (const auto& layer : blend.layers) {
        if (!layer.playing || layer.weight <= 0.0f) {
            continue;
        }

        auto transforms = sampleAnimation(layer.clip, layer.time, layer.looping);
        if (transforms.size() != boneCount) {
            continue;
        }

        // Weighted blend (additive for now - proper blending would use quaternion slerp)
        for (size_t i = 0; i < boneCount; ++i) {
            if (totalWeight == 0.0f) {
                // First layer - just copy
                result[i] = transforms[i];
            } else {
                // Blend with existing
                float w = layer.weight / (totalWeight + layer.weight);
                for (int r = 0; r < 4; ++r) {
                    for (int c = 0; c < 4; ++c) {
                        result[i][r][c] = result[i][r][c] * (1.0f - w) + transforms[i][r][c] * w;
                    }
                }
            }
        }
        totalWeight += layer.weight;
    }

    return result;
}

void OpenGLGraphics3DSystem::drawSkinnedMesh(
    MeshHandle mesh,
    MaterialHandle material,
    const Mat4& worldMatrix,
    std::span<const Mat4> boneTransforms) {

    // Find mesh
    auto meshIt = meshes_.find(mesh);
    if (meshIt == meshes_.end()) return;

    // Find material
    auto matIt = materials_.find(material);
    if (matIt == materials_.end()) {
        matIt = materials_.find(defaultPBRMaterial_);
        if (matIt == materials_.end()) return;
    }

    const auto& meshRes = meshIt->second;
    const auto& materialRes = matIt->second;

    // Use PBR shader (would need a skinned variant for proper GPU skinning)
    ShaderProgram& shader = materialRes.isUnlit ? unlitShader_ : pbrShader_;
    shader.use();

    // Convert world matrix to glm
    glm::mat4 model;
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            model[i][j] = worldMatrix[i][j];
        }
    }

    // Set transforms
    shader.setMat4("uModel", model);
    shader.setMat4("uView", viewMatrix_);
    shader.setMat4("uProjection", projectionMatrix_);
    shader.setMat3("uNormalMatrix", glm::mat3(glm::transpose(glm::inverse(model))));

    // Set bone matrices (limited to 64 bones for now)
    constexpr int MAX_BONES = 64;
    std::vector<glm::mat4> boneMats(std::min(boneTransforms.size(), static_cast<size_t>(MAX_BONES)));
    for (size_t i = 0; i < boneMats.size(); ++i) {
        for (int r = 0; r < 4; ++r) {
            for (int c = 0; c < 4; ++c) {
                boneMats[i][r][c] = boneTransforms[i][r][c];
            }
        }
    }

    GLint boneLoc = shader.getUniformLocation("uBones[0]");
    if (boneLoc != -1 && !boneMats.empty()) {
        glUniformMatrix4fv(boneLoc, static_cast<GLsizei>(boneMats.size()), GL_FALSE,
                           glm::value_ptr(boneMats[0]));
    }

    // Set material uniforms
    shader.setVec4("uBaseColor", glm::vec4(materialRes.baseColor.x, materialRes.baseColor.y,
                                            materialRes.baseColor.z, materialRes.baseColor.w));
    if (!materialRes.isUnlit) {
        shader.setFloat("uMetallic", materialRes.metallic);
        shader.setFloat("uRoughness", materialRes.roughness);
        shader.setVec3("uEmissive", glm::vec3(materialRes.emissive.x, materialRes.emissive.y,
                                               materialRes.emissive.z));
    }

    applyBlendMode(materialRes.blendMode);
    applyCullMode(materialRes.cullMode);

    // Draw
    glBindVertexArray(meshRes.vao);
    glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(meshRes.indexCount), GL_UNSIGNED_INT, nullptr);
    glBindVertexArray(0);

    stats_.drawCalls++;
    stats_.triangles += meshRes.indexCount / 3;
}

//==========================================================================
// 3D Text Rendering
//==========================================================================

Result<Font3DHandle, Graphics3DError> OpenGLGraphics3DSystem::loadFont3D(AssetHandle fontAsset) {
    // For now, create a simple ASCII bitmap font as a placeholder
    // Full SDF font generation would require FreeType + msdf-atlas-gen
    // This creates a basic 16x16 grid font atlas for ASCII 32-127

    Font3DResource font;

    // Create a simple placeholder atlas texture (16x16 grid of characters)
    const int cellSize = 32;
    const int gridSize = 16;
    font.atlasWidth = cellSize * gridSize;
    font.atlasHeight = cellSize * gridSize;

    // Generate a simple placeholder texture (white with black text placeholders)
    std::vector<unsigned char> atlasData(font.atlasWidth * font.atlasHeight * 4, 0);

    // Fill with basic glyph patterns (simple boxes for each character)
    for (int charY = 0; charY < gridSize; ++charY) {
        for (int charX = 0; charX < gridSize; ++charX) {
            int charCode = charY * gridSize + charX;
            if (charCode >= 32 && charCode < 128) {
                // Draw a simple rectangle for each character
                int baseX = charX * cellSize;
                int baseY = charY * cellSize;

                for (int py = 4; py < cellSize - 4; ++py) {
                    for (int px = 4; px < cellSize - 4; ++px) {
                        int idx = ((baseY + py) * font.atlasWidth + (baseX + px)) * 4;
                        atlasData[idx + 0] = 255;  // R
                        atlasData[idx + 1] = 255;  // G
                        atlasData[idx + 2] = 255;  // B
                        atlasData[idx + 3] = 255;  // A
                    }
                }
            }
        }
    }

    // Create OpenGL texture
    glGenTextures(1, &font.atlasTexture);
    glBindTexture(GL_TEXTURE_2D, font.atlasTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, font.atlasWidth, font.atlasHeight,
                 0, GL_RGBA, GL_UNSIGNED_BYTE, atlasData.data());
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_2D, 0);

    // Set up glyph metrics for ASCII 32-127
    float cellUV = 1.0f / static_cast<float>(gridSize);
    for (int c = 32; c < 128; ++c) {
        Font3DGlyph glyph;
        int charX = (c - 32) % gridSize;
        int charY = (c - 32) / gridSize;

        glyph.atlasOffset = Vec2{charX * cellUV, charY * cellUV};
        glyph.atlasSize = Vec2{cellUV, cellUV};
        glyph.size = Vec2{0.5f, 1.0f};  // Width/height at fontSize=1
        glyph.bearing = Vec2{0.0f, 0.8f};  // Baseline offset
        glyph.advance = 0.6f;  // Horizontal advance

        font.glyphs[static_cast<uint32_t>(c)] = glyph;
    }

    font.lineHeight = 1.2f;
    font.ascender = 0.8f;
    font.descender = -0.2f;

    Font3DHandle handle = nextFont3DHandle_++;
    fonts3D_[handle] = std::move(font);

    return handle;
}

void OpenGLGraphics3DSystem::destroyFont3D(Font3DHandle font) {
    auto it = fonts3D_.find(font);
    if (it != fonts3D_.end()) {
        if (it->second.atlasTexture) {
            glDeleteTextures(1, &it->second.atlasTexture);
        }
        fonts3D_.erase(it);
    }
}

void OpenGLGraphics3DSystem::drawText3D(const Text3DItem& item) {
    if (item.text.empty()) return;

    auto fontIt = fonts3D_.find(item.style.font);
    if (fontIt == fonts3D_.end()) return;

    const auto& font = fontIt->second;

    // Use unlit shader for text
    unlitShader_.use();

    // Build model matrix from transform
    glm::mat4 model = glm::translate(glm::mat4(1.0f),
        glm::vec3(item.transform.position.x, item.transform.position.y, item.transform.position.z));

    if (item.style.billboard) {
        // Make text face the camera
        glm::mat4 viewInv = glm::inverse(viewMatrix_);
        glm::vec3 right = glm::vec3(viewInv[0]);
        glm::vec3 up = glm::vec3(viewInv[1]);
        model[0] = glm::vec4(right, 0.0f);
        model[1] = glm::vec4(up, 0.0f);
        model[2] = glm::vec4(glm::cross(right, up), 0.0f);
    } else {
        // Apply rotation from transform
        glm::quat quat(item.transform.rotation.w, item.transform.rotation.x,
                       item.transform.rotation.y, item.transform.rotation.z);
        model *= glm::mat4_cast(quat);
    }

    model = glm::scale(model, glm::vec3(item.style.fontSize));

    unlitShader_.setMat4("uModel", model);
    unlitShader_.setMat4("uView", viewMatrix_);
    unlitShader_.setMat4("uProjection", projectionMatrix_);

    // Set text color
    float r = item.style.color.r / 255.0f;
    float g = item.style.color.g / 255.0f;
    float b = item.style.color.b / 255.0f;
    float a = item.style.color.a / 255.0f;
    unlitShader_.setVec4("uBaseColor", glm::vec4(r, g, b, a));

    // Enable blending for text
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Bind font texture
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, font.atlasTexture);
    unlitShader_.setInt("uTexture", 0);

    // Calculate text width for alignment
    float textWidth = 0.0f;
    for (char c : item.text) {
        uint32_t codepoint = static_cast<uint32_t>(c);
        auto glyphIt = font.glyphs.find(codepoint);
        if (glyphIt != font.glyphs.end()) {
            textWidth += glyphIt->second.advance * item.style.letterSpacing;
        }
    }

    // Apply alignment offset
    float xOffset = 0.0f;
    switch (item.style.alignment) {
        case TextAlignment3D::Left:
            xOffset = 0.0f;
            break;
        case TextAlignment3D::Center:
            xOffset = -textWidth * 0.5f;
            break;
        case TextAlignment3D::Right:
            xOffset = -textWidth;
            break;
    }

    // Create dynamic VBO for text quads
    std::vector<float> vertices;
    vertices.reserve(item.text.size() * 6 * 9);  // 6 vertices per char, 9 floats each

    float cursorX = xOffset;
    float cursorY = 0.0f;

    for (char c : item.text) {
        if (c == '\n') {
            cursorX = xOffset;
            cursorY -= font.lineHeight * item.style.lineSpacing;
            continue;
        }

        uint32_t codepoint = static_cast<uint32_t>(c);
        auto glyphIt = font.glyphs.find(codepoint);
        if (glyphIt == font.glyphs.end()) continue;

        const auto& glyph = glyphIt->second;

        float x0 = cursorX + glyph.bearing.x;
        float y0 = cursorY + glyph.bearing.y - glyph.size.y;
        float x1 = x0 + glyph.size.x;
        float y1 = y0 + glyph.size.y;

        float u0 = glyph.atlasOffset.x;
        float v0 = glyph.atlasOffset.y + glyph.atlasSize.y;
        float u1 = glyph.atlasOffset.x + glyph.atlasSize.x;
        float v1 = glyph.atlasOffset.y;

        // Triangle 1
        vertices.insert(vertices.end(), {x0, y0, 0, 0, 0, 1, u0, v0, 1});
        vertices.insert(vertices.end(), {x1, y0, 0, 0, 0, 1, u1, v0, 1});
        vertices.insert(vertices.end(), {x1, y1, 0, 0, 0, 1, u1, v1, 1});

        // Triangle 2
        vertices.insert(vertices.end(), {x0, y0, 0, 0, 0, 1, u0, v0, 1});
        vertices.insert(vertices.end(), {x1, y1, 0, 0, 0, 1, u1, v1, 1});
        vertices.insert(vertices.end(), {x0, y1, 0, 0, 0, 1, u0, v1, 1});

        cursorX += glyph.advance + item.style.letterSpacing;
    }

    if (vertices.empty()) return;

    // Create temporary VAO/VBO for text
    GLuint textVAO, textVBO;
    glGenVertexArrays(1, &textVAO);
    glGenBuffers(1, &textVBO);

    glBindVertexArray(textVAO);
    glBindBuffer(GL_ARRAY_BUFFER, textVBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_DYNAMIC_DRAW);

    // Position
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 9 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    // Normal
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 9 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    // TexCoord
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 9 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);

    glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(vertices.size() / 9));

    glBindVertexArray(0);
    glDeleteBuffers(1, &textVBO);
    glDeleteVertexArrays(1, &textVAO);

    glBindTexture(GL_TEXTURE_2D, 0);

    stats_.drawCalls++;
}

void OpenGLGraphics3DSystem::drawText3D(
    const std::string& text,
    const Vec3& position,
    Font3DHandle font,
    float fontSize,
    const Color& color) {

    Text3DItem item;
    item.text = text;
    item.transform.position = position;
    item.style.font = font;
    item.style.fontSize = fontSize;
    item.style.color = color;
    drawText3D(item);
}

AABB3D OpenGLGraphics3DSystem::measureText3D(
    const std::string& text,
    Font3DHandle font,
    float fontSize) {

    auto fontIt = fonts3D_.find(font);
    if (fontIt == fonts3D_.end()) {
        return AABB3D{};
    }

    const auto& fontRes = fontIt->second;

    float maxWidth = 0.0f;
    float curWidth = 0.0f;
    int lineCount = 1;

    for (char c : text) {
        if (c == '\n') {
            maxWidth = std::max(maxWidth, curWidth);
            curWidth = 0.0f;
            lineCount++;
            continue;
        }

        uint32_t codepoint = static_cast<uint32_t>(c);
        auto glyphIt = fontRes.glyphs.find(codepoint);
        if (glyphIt != fontRes.glyphs.end()) {
            curWidth += glyphIt->second.advance;
        }
    }
    maxWidth = std::max(maxWidth, curWidth);

    float width = maxWidth * fontSize;
    float height = lineCount * fontRes.lineHeight * fontSize;

    return AABB3D{
        .min = Vec3{0.0f, -height * 0.5f, 0.0f},
        .max = Vec3{width, height * 0.5f, 0.01f}
    };
}

//==========================================================================
// Material Property Updates
//==========================================================================

Result<void, Graphics3DError> OpenGLGraphics3DSystem::setMaterialBaseColor(
    MaterialHandle handle,
    const Vec4& color) {

    auto it = materials_.find(handle);
    if (it == materials_.end()) {
        return std::unexpected(Graphics3DError::InvalidMaterial);
    }

    it->second.baseColor = color;
    return {};
}

Result<void, Graphics3DError> OpenGLGraphics3DSystem::setMaterialMetallicRoughness(
    MaterialHandle handle,
    float metallic,
    float roughness) {

    auto it = materials_.find(handle);
    if (it == materials_.end()) {
        return std::unexpected(Graphics3DError::InvalidMaterial);
    }

    it->second.metallic = metallic;
    it->second.roughness = roughness;
    return {};
}

Result<void, Graphics3DError> OpenGLGraphics3DSystem::setMaterialEmissive(
    MaterialHandle handle,
    const Vec3& emissive) {

    auto it = materials_.find(handle);
    if (it == materials_.end()) {
        return std::unexpected(Graphics3DError::InvalidMaterial);
    }

    it->second.emissive = emissive;
    return {};
}

std::optional<PBRMaterial> OpenGLGraphics3DSystem::getMaterialProperties(MaterialHandle handle) const {
    auto it = materials_.find(handle);
    if (it == materials_.end()) {
        return std::nullopt;
    }

    const auto& res = it->second;
    PBRMaterial mat;
    mat.baseColorFactor = res.baseColor;
    mat.metallicFactor = res.metallic;
    mat.roughnessFactor = res.roughness;
    mat.emissiveFactor = res.emissive;
    mat.blendMode = res.blendMode;
    mat.cullMode = res.cullMode;
    mat.alphaCutoff = res.alphaCutoff;
    mat.doubleSided = res.doubleSided;
    mat.receiveShadows = res.receiveShadows;
    mat.castShadows = res.castShadows;

    return mat;
}

//==========================================================================
// LOD (Level of Detail)
//==========================================================================

void OpenGLGraphics3DSystem::setLODDistances(std::span<const float> distances) {
    lodConfig_.distances.assign(distances.begin(), distances.end());
    // Ensure distances are sorted in ascending order
    std::sort(lodConfig_.distances.begin(), lodConfig_.distances.end());
}

void OpenGLGraphics3DSystem::registerLODMeshes(MeshHandle primaryMesh, std::span<const MeshHandle> lodMeshes) {
    LODMeshGroup group;
    group.primaryMesh = primaryMesh;
    group.lodMeshes.assign(lodMeshes.begin(), lodMeshes.end());
    lodGroups_[primaryMesh] = std::move(group);
}

void OpenGLGraphics3DSystem::setLODBias(float bias) {
    lodConfig_.bias = bias;
}

MeshHandle OpenGLGraphics3DSystem::selectLODMesh(MeshHandle primaryMesh, float distance) const {
    // Check if this mesh has LOD variants
    auto it = lodGroups_.find(primaryMesh);
    if (it == lodGroups_.end()) {
        return primaryMesh;  // No LOD registered, use primary
    }

    const auto& group = it->second;
    if (group.lodMeshes.empty() || lodConfig_.distances.empty()) {
        return primaryMesh;
    }

    // Apply bias to distance
    float adjustedDistance = distance * (1.0f + lodConfig_.bias);

    // Find appropriate LOD level
    size_t lodLevel = 0;
    for (size_t i = 0; i < lodConfig_.distances.size(); ++i) {
        if (adjustedDistance > lodConfig_.distances[i]) {
            lodLevel = i + 1;
        } else {
            break;
        }
    }

    // Clamp to available LOD meshes
    if (lodLevel == 0) {
        return primaryMesh;  // Use highest detail
    }

    lodLevel = std::min(lodLevel - 1, group.lodMeshes.size() - 1);
    return group.lodMeshes[lodLevel];
}

//==========================================================================
// Frustum Culling Helpers
//==========================================================================

bool OpenGLGraphics3DSystem::isAABBInFrustum(const AABB3D& aabb, const Frustum& frustum) const {
    // Test AABB against all 6 frustum planes
    // If the AABB is completely outside any plane, it's not visible
    for (int i = 0; i < 6; ++i) {
        const auto& plane = frustum.planes[i];

        // Find the corner of the AABB most aligned with the plane normal (positive vertex)
        Vec3 positiveVertex{
            (plane.normal.x >= 0) ? aabb.max.x : aabb.min.x,
            (plane.normal.y >= 0) ? aabb.max.y : aabb.min.y,
            (plane.normal.z >= 0) ? aabb.max.z : aabb.min.z
        };

        // If the positive vertex is behind the plane, the AABB is outside
        float dist = plane.normal.x * positiveVertex.x +
                     plane.normal.y * positiveVertex.y +
                     plane.normal.z * positiveVertex.z + plane.distance;

        if (dist < 0) {
            return false;  // AABB is completely outside this plane
        }
    }

    return true;  // AABB is at least partially inside all planes
}

AABB3D OpenGLGraphics3DSystem::transformAABB(const AABB3D& aabb, const glm::mat4& worldMatrix) const {
    // Transform all 8 corners of the AABB and find the new bounds
    Vec3 corners[8] = {
        {aabb.min.x, aabb.min.y, aabb.min.z},
        {aabb.max.x, aabb.min.y, aabb.min.z},
        {aabb.min.x, aabb.max.y, aabb.min.z},
        {aabb.max.x, aabb.max.y, aabb.min.z},
        {aabb.min.x, aabb.min.y, aabb.max.z},
        {aabb.max.x, aabb.min.y, aabb.max.z},
        {aabb.min.x, aabb.max.y, aabb.max.z},
        {aabb.max.x, aabb.max.y, aabb.max.z}
    };

    AABB3D result;
    result.min = Vec3{std::numeric_limits<float>::max()};
    result.max = Vec3{std::numeric_limits<float>::lowest()};

    for (const auto& corner : corners) {
        glm::vec4 transformed = worldMatrix * glm::vec4(corner.x, corner.y, corner.z, 1.0f);
        Vec3 p{transformed.x, transformed.y, transformed.z};

        result.min.x = std::min(result.min.x, p.x);
        result.min.y = std::min(result.min.y, p.y);
        result.min.z = std::min(result.min.z, p.z);

        result.max.x = std::max(result.max.x, p.x);
        result.max.y = std::max(result.max.y, p.y);
        result.max.z = std::max(result.max.z, p.z);
    }

    return result;
}

//==========================================================================
// Runtime Configuration
//==========================================================================

bool OpenGLGraphics3DSystem::loadRuntimeConfig(const std::filesystem::path& configPath) {
    configPath_ = configPath;

    // Load config file content
    std::ifstream file(configPath);
    if (!file) {
        std::cerr << "[OpenGL Graphics3D] Cannot open config file: " << configPath << std::endl;
        return false;
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string luaContent = buffer.str();

    try {
        // Use ConfigSystem's unified Lua parsing instead of creating our own sol::state
        if (!pIConfigSystem_) {
            std::cerr << "[OpenGL Graphics3D] ERROR: ConfigSystem is required for loading config" << std::endl;
            return false;
        }

        auto result = pIConfigSystem_->parseLuaString(luaContent, configPath.string());
        if (!result) {
            std::cerr << "[OpenGL Graphics3D] Failed to load config: " << configPath << std::endl;
            return false;
        }

        sol::table config = result->as<sol::table>();
        Graphics3DRuntimeConfig runtimeConfig;

        //======================================================================
        // Unified Rendering Settings
        //======================================================================

        // Gamma correction (unified setting)
        if (auto gamma = config["gammaCorrection"]; gamma.valid()) {
            runtimeConfig.gammaCorrection = gamma.get<bool>();
        }

        // MSAA samples (unified setting)
        if (auto msaa = config["msaaSamples"]; msaa.valid()) {
            runtimeConfig.msaaSamples = msaa.get<std::uint32_t>();
        }

        // V-Sync (unified setting)
        if (auto vsync = config["vsync"]; vsync.valid()) {
            std::string mode = vsync.get<std::string>();
            if (mode == "off") {
                runtimeConfig.vsync = PresentMode::Immediate;
            } else if (mode == "adaptive") {
                runtimeConfig.vsync = PresentMode::Mailbox;
            } else {
                runtimeConfig.vsync = PresentMode::FIFO;  // "on" or default
            }
        }

        // Shader paths
        if (auto shaderPaths = config["shaderPaths"]; shaderPaths.valid() && shaderPaths.get_type() == sol::type::table) {
            sol::table paths = shaderPaths;
            for (auto& kv : paths) {
                if (kv.second.is<std::string>()) {
                    runtimeConfig.shaderPaths.push_back(kv.second.as<std::string>());
                }
            }
        }

        //======================================================================
        // Backend-Specific Settings (Vulkan-only features)
        //======================================================================

        if (auto vulkan = config["vulkan"]; vulkan.valid() && vulkan.get_type() == sol::type::table) {
            sol::table vk = vulkan;

            if (auto validation = vk["validationLayers"]; validation.valid()) {
                runtimeConfig.vulkanValidationLayers = validation.get<bool>();
            }
        }

        //======================================================================
        // Parse lighting
        if (auto lighting = config["lighting"]; lighting.valid() && lighting.get_type() == sol::type::table) {
            sol::table light = lighting;

            if (auto dir = light["lightDirection"]; dir.valid() && dir.get_type() == sol::type::table) {
                sol::table d = dir;
                runtimeConfig.lightDirection = Vec3{
                    d[1].get_or(0.5f),
                    d[2].get_or(-1.0f),
                    d[3].get_or(0.3f)
                };
            }
            if (auto col = light["lightColor"]; col.valid() && col.get_type() == sol::type::table) {
                sol::table c = col;
                runtimeConfig.lightColor = Vec3{
                    c[1].get_or(1.0f),
                    c[2].get_or(0.98f),
                    c[3].get_or(0.95f)
                };
            }
            if (auto amb = light["ambientColor"]; amb.valid() && amb.get_type() == sol::type::table) {
                sol::table a = amb;
                runtimeConfig.ambientColor = Vec3{
                    a[1].get_or(0.15f),
                    a[2].get_or(0.18f),
                    a[3].get_or(0.22f)
                };
            }
            if (auto intensity = light["ambientIntensity"]; intensity.valid()) {
                runtimeConfig.ambientIntensity = intensity.get<float>();
            }
        }

        // Parse clear color (convert from 0-1 float to 0-255 uint8_t)
        if (auto clearColor = config["clearColor"]; clearColor.valid() && clearColor.get_type() == sol::type::table) {
            sol::table c = clearColor;
            runtimeConfig.clearColor = Color::fromFloat(
                c[1].get_or(0.529f),
                c[2].get_or(0.808f),
                c[3].get_or(0.922f),
                c[4].get_or(1.0f)
            );
        }

        // Parse debug options
        if (auto debug = config["debug"]; debug.valid() && debug.get_type() == sol::type::table) {
            sol::table d = debug;

            if (auto wireframe = d["wireframe"]; wireframe.valid()) {
                runtimeConfig.debugWireframe = wireframe.get<bool>();
            }
            if (auto normals = d["showNormals"]; normals.valid()) {
                runtimeConfig.debugShowNormals = normals.get<bool>();
            }
            if (auto fps = d["showFps"]; fps.valid()) {
                runtimeConfig.debugShowFps = fps.get<bool>();
            }
            if (auto hotReload = d["hotReload"]; hotReload.valid()) {
                runtimeConfig.hotReload = hotReload.get<bool>();
            }
        }

        // Apply the loaded config
        applyRuntimeConfig(runtimeConfig);

        std::cout << "[OpenGL Graphics3D] Loaded runtime config from " << configPath << std::endl;
        return true;

    } catch (const std::exception& e) {
        std::cerr << "[OpenGL Graphics3D] Exception loading config: " << e.what() << std::endl;
        return false;
    }
}

void OpenGLGraphics3DSystem::applyRuntimeConfig(const Graphics3DRuntimeConfig& config) {
    runtimeConfig_ = config;

    // Apply clear color
    setClearColor(config.clearColor);

    // Apply directional light from config if not already set
    if (!directionalLight_.has_value()) {
        DirectionalLight light;
        light.direction = config.lightDirection;
        light.color = config.lightColor;
        setDirectionalLight(light);
    }

    // Apply ambient light
    setAmbientLight(config.ambientColor, config.ambientIntensity);

    // Apply V-Sync setting (unified)
    if (window_) {
        int vsyncValue = 1;  // Default: on
        switch (config.vsync) {
            case PresentMode::Immediate: vsyncValue = 0; break;
            case PresentMode::FIFO: vsyncValue = 1; break;
            case PresentMode::Mailbox: vsyncValue = 1; break;  // OpenGL doesn't have true triple buffering
        }
        glfwSwapInterval(vsyncValue);
    }

    // Apply gamma correction setting (unified)
    // When gammaCorrection is true, enable sRGB framebuffer for hardware gamma
    if (config.gammaCorrection) {
        glEnable(GL_FRAMEBUFFER_SRGB);
    } else {
        glDisable(GL_FRAMEBUFFER_SRGB);
    }

    // Apply wireframe mode
    if (config.debugWireframe) {
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    } else {
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    }

    std::cout << "[OpenGL Graphics3D] Applied runtime config" << std::endl;
}

const Graphics3DRuntimeConfig& OpenGLGraphics3DSystem::getRuntimeConfig() const {
    return runtimeConfig_;
}

bool OpenGLGraphics3DSystem::reloadRuntimeConfig() {
    if (configPath_.empty()) {
        return false;
    }
    return loadRuntimeConfig(configPath_);
}

//==========================================================================
// Kangaru Service Definitions
}  // namespace bestow

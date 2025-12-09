// bestow-contract/src/bestow.assets.cppm
// Asset system interface

module;

#include <cstddef>
#include <filesystem>
#include <functional>
#include <optional>
#include <string>
#include <vector>

export module bestow.assets;

import bestow.types;

export namespace bestow {

// Texture data structure - exported so consumers can access loaded texture data
struct TextureData {
    std::vector<unsigned char> pixels;
    int width = 0;
    int height = 0;
    int channels = 0;
};

// Font data structure - stores raw font file bytes for stb_truetype to process
struct FontData {
    std::vector<unsigned char> fileData;  // Raw TTF/OTF bytes
    std::string path;
    std::size_t fileSize = 0;
};

// Sound data structure - stores raw audio file bytes for FMOD to process
struct SoundData {
    std::vector<unsigned char> fileData;  // Raw WAV/OGG/MP3 bytes
    std::string path;
    std::size_t fileSize = 0;
};

// Shader data structure - stores GLSL source and compiled SPIR-V bytecode
struct ShaderData {
    std::string glslSource;                      // Original GLSL source code
    std::vector<std::uint32_t> spirvBytecode;    // Compiled SPIR-V (empty if not compiled)
    std::string path;
    std::string entryPoint = "main";

    enum class Stage { Vertex, Fragment, Geometry, Compute, TessControl, TessEval };
    Stage stage = Stage::Vertex;

    // Compilation status
    bool compiled = false;
    std::string compileError;                    // Error message if compilation failed
};

//==========================================================================
// 3D Asset Types
//==========================================================================

// Forward declarations for 3D types (defined in bestow.types)
// Using inline definitions here to avoid circular imports

struct Vertex3DData {
    float position[3]{0.0f};
    float normal[3]{0.0f, 1.0f, 0.0f};
    float texCoord[2]{0.0f};
    float color[4]{1.0f, 1.0f, 1.0f, 1.0f};
    float tangent[4]{1.0f, 0.0f, 0.0f, 1.0f};      // w = handedness
    unsigned char boneIndices[4]{0};               // For skeletal animation
    float boneWeights[4]{0.0f, 0.0f, 0.0f, 0.0f};  // For skeletal animation
};

struct SubMeshData {
    std::uint32_t indexOffset = 0;
    std::uint32_t indexCount = 0;
    std::uint32_t materialIndex = 0;
    float boundsMin[3]{0.0f};
    float boundsMax[3]{0.0f};
    std::string name;
};

struct MeshData {
    std::vector<Vertex3DData> vertices;
    std::vector<std::uint32_t> indices;
    std::vector<SubMeshData> subMeshes;
    float boundsMin[3]{0.0f};
    float boundsMax[3]{0.0f};
    std::string name;
    bool hasTangents = false;
    bool hasBoneData = false;
};

struct MaterialTextureRef {
    std::string path;
    AssetHandle handle;
};

struct MaterialData {
    std::string name;

    // PBR properties
    float baseColorFactor[4]{1.0f, 1.0f, 1.0f, 1.0f};
    float metallicFactor = 0.0f;
    float roughnessFactor = 1.0f;
    float normalScale = 1.0f;
    float occlusionStrength = 1.0f;
    float emissiveFactor[3]{0.0f};
    float alphaCutoff = 0.5f;

    // Texture references
    MaterialTextureRef baseColorTexture;
    MaterialTextureRef metallicRoughnessTexture;
    MaterialTextureRef normalTexture;
    MaterialTextureRef occlusionTexture;
    MaterialTextureRef emissiveTexture;

    // Render state
    bool doubleSided = false;
    bool transparent = false;
    bool unlit = false;
};

struct ModelData {
    std::vector<MeshData> meshes;
    std::vector<MaterialData> materials;
    std::string name;

    // Scene hierarchy (for multi-mesh models)
    struct Node {
        std::string name;
        int meshIndex = -1;
        int parentIndex = -1;
        float localTransform[16]{1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1};  // Identity
        std::vector<int> children;
    };
    std::vector<Node> nodes;
    int rootNodeIndex = 0;

    // Skeletal animation data
    struct Bone {
        std::string name;
        int parentIndex = -1;
        float offsetMatrix[16]{1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1};  // Inverse bind pose
    };
    std::vector<Bone> bones;

    struct AnimationKeyframe {
        float time = 0.0f;
        float translation[3]{0, 0, 0};
        float rotation[4]{0, 0, 0, 1};  // Quaternion (x, y, z, w)
        float scale[3]{1, 1, 1};
    };

    struct AnimationChannel {
        int boneIndex = -1;
        std::vector<AnimationKeyframe> keyframes;
    };

    struct Animation {
        std::string name;
        float duration = 0.0f;
        float ticksPerSecond = 30.0f;
        std::vector<AnimationChannel> channels;
    };
    std::vector<Animation> animations;
};

struct CubemapData {
    // 6 faces: +X, -X, +Y, -Y, +Z, -Z
    std::vector<std::vector<unsigned char>> facePixels;
    int faceWidth = 0;
    int faceHeight = 0;
    int channels = 0;
    std::string name;
};

struct AssetMetadata {
    AssetHandle handle;
    std::filesystem::path sourcePath;
    AssetState state = AssetState::Unloaded;
    std::size_t sizeBytes = 0;
    std::optional<std::string> errorMessage;
};

using AssetLoadCallback = std::function<void(AssetHandle, AssetState)>;

// Subscription callback for asset change notifications
// Called when an asset is reloaded (e.g., due to hot reload)
using AssetChangeCallback = std::function<void(AssetHandle handle, AssetType type)>;

// Unique identifier for asset subscriptions
using SubscriptionId = std::uint64_t;
constexpr SubscriptionId InvalidSubscriptionId = 0;

class IAssetSystem {
public:
    virtual ~IAssetSystem() = default;

    //======================================================================
    // Lifecycle
    //======================================================================

    virtual void update() = 0;

    //======================================================================
    // Registration
    //======================================================================

    virtual AssetHandle registerAsset(AssetType type,
                                       const std::filesystem::path& path) = 0;
    virtual void unregisterAsset(AssetHandle handle) = 0;

    //======================================================================
    // Loading
    //======================================================================

    virtual void loadAsset(AssetHandle handle) = 0;
    virtual void loadAssetAsync(AssetHandle handle,
                                AssetLoadCallback callback = nullptr) = 0;
    virtual void unloadAsset(AssetHandle handle) = 0;

    //======================================================================
    // State Queries
    //======================================================================

    virtual AssetState getAssetState(AssetHandle handle) const = 0;
    virtual AssetMetadata getAssetMetadata(AssetHandle handle) const = 0;
    virtual bool isLoaded(AssetHandle handle) const = 0;

    //======================================================================
    // Raw Data Access
    //======================================================================

    virtual void* getRawAsset(AssetHandle handle) = 0;
    virtual const void* getRawAsset(AssetHandle handle) const = 0;

    template<typename T>
    T* getAsset(AssetHandle handle) {
        return static_cast<T*>(getRawAsset(handle));
    }

    template<typename T>
    const T* getAsset(AssetHandle handle) const {
        return static_cast<const T*>(getRawAsset(handle));
    }

    //======================================================================
    // Bulk Operations
    //======================================================================

    virtual void loadAll() = 0;
    virtual void unloadAll() = 0;
    virtual std::vector<AssetHandle> getAssetsOfType(AssetType type) const = 0;

    //======================================================================
    // Hot Reload (Development)
    //======================================================================

    virtual void enableHotReload(bool enable) = 0;
    virtual void checkForReloads() = 0;
    virtual void reloadAsset(AssetHandle handle) = 0;

    //======================================================================
    // Asset Change Subscriptions
    //======================================================================

    /// Subscribe to asset change notifications for a specific asset
    /// Callback is invoked when the asset is reloaded (e.g., via hot reload)
    /// Returns a subscription ID that can be used to unsubscribe
    virtual SubscriptionId subscribe(AssetHandle handle,
                                     AssetChangeCallback callback) = 0;

    /// Subscribe to changes for ALL assets of a specific type
    /// Useful for systems that need to know when any shader/texture/etc changes
    virtual SubscriptionId subscribeToType(AssetType type,
                                           AssetChangeCallback callback) = 0;

    /// Unsubscribe from asset change notifications
    virtual void unsubscribe(SubscriptionId id) = 0;

    //======================================================================
    // Shader Loading and Compilation
    //======================================================================

    /// Load a GLSL shader file (reads source, optionally compiles to SPIR-V)
    /// Shader stage is inferred from file extension (.vert, .frag, .geom, .comp)
    virtual AssetHandle loadShader(const std::filesystem::path& path) = 0;

    /// Get compiled shader data (GLSL source + SPIR-V bytecode)
    virtual const ShaderData* getShaderData(AssetHandle handle) const = 0;

    /// Compile a GLSL shader to SPIR-V asynchronously
    /// The callback is invoked when compilation completes (success or failure)
    virtual void compileShaderAsync(AssetHandle handle,
                                    AssetLoadCallback callback = nullptr) = 0;

    /// Check if shader compilation is supported (shaderc available)
    virtual bool isShaderCompilationSupported() const = 0;

    //======================================================================
    // 3D Asset Loading
    //======================================================================

    /// Load mesh data from an asset (OBJ, glTF, FBX)
    /// The mesh data is parsed and stored in CPU memory
    virtual const MeshData* getMeshData(AssetHandle handle) const = 0;

    /// Load model data from an asset (glTF, FBX with materials)
    /// Includes meshes, materials, and scene hierarchy
    virtual const ModelData* getModelData(AssetHandle handle) const = 0;

    /// Load material data from an asset
    virtual const MaterialData* getMaterialData(AssetHandle handle) const = 0;

    /// Load cubemap data from an asset (6 separate images or single equirectangular)
    virtual const CubemapData* getCubemapData(AssetHandle handle) const = 0;

    /// Register and load a mesh asset in one call
    virtual AssetHandle loadMesh(const std::filesystem::path& path) = 0;

    /// Register and load a model asset in one call
    virtual AssetHandle loadModel(const std::filesystem::path& path) = 0;

    /// Register and load a cubemap asset (6 faces or single HDR)
    virtual AssetHandle loadCubemap(const std::filesystem::path& path) = 0;
    virtual AssetHandle loadCubemap(
        const std::filesystem::path& posX,
        const std::filesystem::path& negX,
        const std::filesystem::path& posY,
        const std::filesystem::path& negY,
        const std::filesystem::path& posZ,
        const std::filesystem::path& negZ) = 0;
};

}  // namespace bestow

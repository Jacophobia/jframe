// bestow-contract/src/bestow.assets.cppm
// Asset system interface

module;

#include <any>
#include <cstddef>
#include <filesystem>
#include <functional>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

// Platform-specific includes for executable path detection
#if defined(__APPLE__)
    #include <mach-o/dyld.h>
    #include <climits>
#elif defined(__linux__)
    #include <unistd.h>
    #include <linux/limits.h>
#elif defined(_WIN32)
    #define WIN32_LEAN_AND_MEAN
    #include <windows.h>
#endif

export module bestow.assets;

import std;
import bestow.types;

export namespace bestow {

//==========================================================================
// Asset Library - Auto-detection and Discovery
//==========================================================================

/// Error codes for AssetLibrary operations
enum class AssetLibraryError {
    None = 0,
    LibraryNotFound,
    AssetNotFound,
    InvalidPath,
    IOError,
};

/// Convert AssetLibraryError to string
[[nodiscard]] inline const char* assetLibraryErrorString(AssetLibraryError err) noexcept {
    switch (err) {
        case AssetLibraryError::None:            return "No error";
        case AssetLibraryError::LibraryNotFound: return "Asset library directory not found";
        case AssetLibraryError::AssetNotFound:   return "Asset not found";
        case AssetLibraryError::InvalidPath:     return "Invalid path";
        case AssetLibraryError::IOError:         return "I/O error";
    }
    return "Unknown error";
}

/// Configuration for AssetLibrary auto-detection
struct AssetLibraryConfig {
    /// Environment variable name to check for library path override
    std::string envOverride = "BESTOW_LIBRARY_PATH";

    /// Folder names to search for (in order of preference)
    std::vector<std::string> folderNames = {"library", "asset-library"};

    /// Additional search paths to check
    std::vector<std::filesystem::path> additionalSearchPaths = {};

    /// Whether to search relative to current working directory
    bool allowCwdFallback = true;

    /// Maximum depth to search upward from executable directory
    int maxUpwardSearchDepth = 5;
};

/// Represents a discovered asset in the library
struct LibraryAssetInfo {
    std::string relativePath;           // Path relative to library root (e.g., "shaders/debug3d.frag")
    std::string libraryPath;            // Full :library:/ path (e.g., ":library:/shaders/debug3d.frag")
    std::string name;                   // File name without path (e.g., "debug3d.frag")
    std::string stem;                   // Name without extension (e.g., "debug3d")
    std::string extension;              // Extension including dot (e.g., ".frag")
    std::string category;               // Top-level directory (e.g., "shaders")
    bool isDirectory = false;
};

/// Automatic asset library path resolution and discovery
///
/// AssetLibrary provides robust auto-detection of the engine's asset library
/// directory without requiring a CLI argument. It searches in the following order:
///
/// 1. Environment variable override (BESTOW_LIBRARY_PATH by default)
/// 2. Paths relative to the executable directory (library/, ../library/, etc.)
/// 3. Additional search paths configured by the user
/// 4. Paths relative to current working directory (as fallback)
///
/// Usage:
///   auto lib = AssetLibrary::create();
///   if (lib) {
///       auto shaderPath = lib->get("shaders/default.frag");
///       if (shaderPath) {
///           // Use *shaderPath
///       }
///
///       // List all shaders for autocomplete
///       auto shaders = lib->listAssets("shaders");
///   }
class AssetLibrary {
public:
    /// Factory function - returns nullopt if library root cannot be found
    [[nodiscard]] static std::optional<AssetLibrary> create(AssetLibraryConfig config = {}) {
        AssetLibrary lib(std::move(config));
        if (lib.initError_ != AssetLibraryError::None) {
            return std::nullopt;
        }
        return lib;
    }

    /// Factory with error output
    [[nodiscard]] static std::optional<AssetLibrary> create(AssetLibraryConfig config,
                                                             AssetLibraryError& outError) {
        AssetLibrary lib(std::move(config));
        outError = lib.initError_;
        if (lib.initError_ != AssetLibraryError::None) {
            return std::nullopt;
        }
        return lib;
    }

    /// Get absolute path to an asset, returns nullopt if not found
    [[nodiscard]] std::optional<std::filesystem::path> get(std::string_view relativePath) const noexcept {
        std::error_code ec;
        std::filesystem::path assetPath = root_ / relativePath;

        if (!std::filesystem::exists(assetPath, ec) || ec) {
            return std::nullopt;
        }

        std::filesystem::path canonical = std::filesystem::canonical(assetPath, ec);
        if (ec) {
            return std::nullopt;
        }

        return canonical;
    }

    /// Get with explicit error reporting
    [[nodiscard]] std::optional<std::filesystem::path> get(std::string_view relativePath,
                                                            AssetLibraryError& outError) const noexcept {
        std::error_code ec;
        std::filesystem::path assetPath = root_ / relativePath;

        if (!std::filesystem::exists(assetPath, ec)) {
            outError = ec ? AssetLibraryError::IOError : AssetLibraryError::AssetNotFound;
            return std::nullopt;
        }

        std::filesystem::path canonical = std::filesystem::canonical(assetPath, ec);
        if (ec) {
            outError = AssetLibraryError::IOError;
            return std::nullopt;
        }

        outError = AssetLibraryError::None;
        return canonical;
    }

    /// Check if an asset exists
    [[nodiscard]] bool exists(std::string_view relativePath) const noexcept {
        std::error_code ec;
        return std::filesystem::exists(root_ / relativePath, ec) && !ec;
    }

    /// Get the root directory of the asset library
    [[nodiscard]] const std::filesystem::path& root() const noexcept {
        return root_;
    }

    /// Get absolute path without checking existence (useful for write targets)
    [[nodiscard]] std::optional<std::filesystem::path> resolve(std::string_view relativePath) const noexcept {
        std::error_code ec;
        std::filesystem::path result = std::filesystem::weakly_canonical(root_ / relativePath, ec);
        if (ec) {
            return std::nullopt;
        }
        return result;
    }

    /// List all assets in a subdirectory (non-recursive)
    /// Returns vector of LibraryAssetInfo for each file/directory found
    [[nodiscard]] std::vector<LibraryAssetInfo> listAssets(std::string_view relativeDir = "") const {
        std::vector<LibraryAssetInfo> results;
        std::error_code ec;
        std::filesystem::path dir = relativeDir.empty() ? root_ : root_ / relativeDir;

        if (!std::filesystem::is_directory(dir, ec) || ec) {
            return results;
        }

        std::string category = relativeDir.empty() ? "" : std::string(relativeDir);
        // Extract top-level category
        if (!category.empty()) {
            auto slashPos = category.find('/');
            if (slashPos != std::string::npos) {
                category = category.substr(0, slashPos);
            }
        }

        for (auto it = std::filesystem::directory_iterator(dir, ec);
             it != std::filesystem::directory_iterator() && !ec;
             it.increment(ec)) {

            LibraryAssetInfo info;
            auto relPath = std::filesystem::relative(it->path(), root_, ec);
            if (ec) continue;

            info.relativePath = relPath.string();
            info.libraryPath = ":library:/" + info.relativePath;
            info.name = it->path().filename().string();
            info.stem = it->path().stem().string();
            info.extension = it->path().extension().string();
            info.category = category.empty() ? info.name : category;
            info.isDirectory = it->is_directory();

            results.push_back(std::move(info));
        }

        return results;
    }

    /// List all assets in a subdirectory (recursive)
    /// Returns vector of LibraryAssetInfo for each file found (not directories)
    [[nodiscard]] std::vector<LibraryAssetInfo> listAssetsRecursive(std::string_view relativeDir = "") const {
        std::vector<LibraryAssetInfo> results;
        std::error_code ec;
        std::filesystem::path dir = relativeDir.empty() ? root_ : root_ / relativeDir;

        if (!std::filesystem::is_directory(dir, ec) || ec) {
            return results;
        }

        for (auto it = std::filesystem::recursive_directory_iterator(dir, ec);
             it != std::filesystem::recursive_directory_iterator() && !ec;
             it.increment(ec)) {

            if (it->is_directory()) continue;  // Only return files

            LibraryAssetInfo info;
            auto relPath = std::filesystem::relative(it->path(), root_, ec);
            if (ec) continue;

            info.relativePath = relPath.string();
            info.libraryPath = ":library:/" + info.relativePath;
            info.name = it->path().filename().string();
            info.stem = it->path().stem().string();
            info.extension = it->path().extension().string();
            info.isDirectory = false;

            // Extract category from first path component
            std::string relStr = info.relativePath;
            auto slashPos = relStr.find('/');
            info.category = (slashPos != std::string::npos) ? relStr.substr(0, slashPos) : "";

            results.push_back(std::move(info));
        }

        return results;
    }

    /// Get all top-level categories (directories) in the library
    [[nodiscard]] std::vector<std::string> listCategories() const {
        std::vector<std::string> categories;
        std::error_code ec;

        for (auto it = std::filesystem::directory_iterator(root_, ec);
             it != std::filesystem::directory_iterator() && !ec;
             it.increment(ec)) {

            if (it->is_directory()) {
                categories.push_back(it->path().filename().string());
            }
        }

        return categories;
    }

    /// Iterate over files in a subdirectory (non-recursive)
    template<typename Callback>
    bool forEach(std::string_view relativeDir, Callback&& callback) const noexcept {
        std::error_code ec;
        std::filesystem::path dir = root_ / relativeDir;

        if (!std::filesystem::is_directory(dir, ec) || ec) {
            return false;
        }

        for (auto it = std::filesystem::directory_iterator(dir, ec);
             it != std::filesystem::directory_iterator() && !ec;
             it.increment(ec)) {
            callback(it->path());
        }

        return !ec;
    }

    /// Iterate over files in a subdirectory (recursive)
    template<typename Callback>
    bool forEachRecursive(std::string_view relativeDir, Callback&& callback) const noexcept {
        std::error_code ec;
        std::filesystem::path dir = root_ / relativeDir;

        if (!std::filesystem::is_directory(dir, ec) || ec) {
            return false;
        }

        for (auto it = std::filesystem::recursive_directory_iterator(dir, ec);
             it != std::filesystem::recursive_directory_iterator() && !ec;
             it.increment(ec)) {
            callback(it->path());
        }

        return !ec;
    }

private:
    AssetLibraryConfig config_;
    std::filesystem::path root_;
    AssetLibraryError initError_ = AssetLibraryError::None;

    explicit AssetLibrary(AssetLibraryConfig config)
        : config_(std::move(config))
    {
        auto root = resolveLibraryRoot();
        if (root) {
            root_ = std::move(*root);
            initError_ = AssetLibraryError::None;
        } else {
            initError_ = AssetLibraryError::LibraryNotFound;
        }
    }

    [[nodiscard]] std::optional<std::filesystem::path> resolveLibraryRoot() const noexcept {
        // 1. Check environment variable override
        if (auto path = tryEnvOverride()) {
            return path;
        }

        // 2. Check paths relative to executable
        if (auto path = tryExeRelative()) {
            return path;
        }

        // 3. Check additional search paths
        for (const auto& searchPath : config_.additionalSearchPaths) {
            if (isValidAssetRoot(searchPath)) {
                std::error_code ec;
                auto canonical = std::filesystem::canonical(searchPath, ec);
                if (!ec) {
                    return canonical;
                }
            }
        }

        // 4. Check current working directory fallback
        if (config_.allowCwdFallback) {
            if (auto path = tryCwdRelative()) {
                return path;
            }
        }

        return std::nullopt;
    }

    [[nodiscard]] std::optional<std::filesystem::path> tryEnvOverride() const noexcept {
        const char* envVal = std::getenv(config_.envOverride.c_str());
        if (envVal && *envVal) {
            std::filesystem::path envPath(envVal);
            if (isValidAssetRoot(envPath)) {
                std::error_code ec;
                auto canonical = std::filesystem::canonical(envPath, ec);
                if (!ec) {
                    return canonical;
                }
            }
        }
        return std::nullopt;
    }

    [[nodiscard]] std::optional<std::filesystem::path> tryExeRelative() const noexcept {
        auto exeDir = getExecutableDir();
        if (!exeDir) {
            return std::nullopt;
        }

        // Build candidate paths relative to executable
        std::vector<std::filesystem::path> candidates;

        // Check each folder name at various relative locations
        for (const auto& folderName : config_.folderNames) {
            // Same directory as executable
            candidates.push_back(*exeDir / folderName);
            // One level up (common for build directories)
            candidates.push_back(*exeDir / ".." / folderName);
            // Standard install location (Unix)
            candidates.push_back(*exeDir / ".." / "share" / folderName);
            // macOS bundle Resources
            candidates.push_back(*exeDir / ".." / "Resources" / folderName);
        }

        // Search upward from executable directory
        std::filesystem::path current = *exeDir;
        for (int depth = 0; depth < config_.maxUpwardSearchDepth; ++depth) {
            for (const auto& folderName : config_.folderNames) {
                candidates.push_back(current / folderName);
            }
            std::error_code ec;
            auto parent = current.parent_path();
            if (parent == current) break;  // Reached root
            current = parent;
        }

        // Find the first valid candidate
        for (const auto& candidate : candidates) {
            if (isValidAssetRoot(candidate)) {
                std::error_code ec;
                auto canonical = std::filesystem::canonical(candidate, ec);
                if (!ec) {
                    return canonical;
                }
            }
        }

        return std::nullopt;
    }

    [[nodiscard]] std::optional<std::filesystem::path> tryCwdRelative() const noexcept {
        std::error_code ec;
        std::filesystem::path cwd = std::filesystem::current_path(ec);
        if (ec) {
            return std::nullopt;
        }

        std::vector<std::filesystem::path> candidates;
        for (const auto& folderName : config_.folderNames) {
            candidates.push_back(cwd / folderName);
            candidates.push_back(cwd / ".." / folderName);
        }

        for (const auto& candidate : candidates) {
            if (isValidAssetRoot(candidate)) {
                auto canonical = std::filesystem::canonical(candidate, ec);
                if (!ec) {
                    return canonical;
                }
            }
        }

        return std::nullopt;
    }

    [[nodiscard]] static bool isValidAssetRoot(const std::filesystem::path& path) noexcept {
        std::error_code ec;
        return std::filesystem::is_directory(path, ec) && !ec;
    }

    [[nodiscard]] static std::optional<std::filesystem::path> getExecutableDir() noexcept {
        std::error_code ec;

        #if defined(__APPLE__)
            char buf[PATH_MAX];
            uint32_t size = sizeof(buf);
            if (_NSGetExecutablePath(buf, &size) == 0) {
                auto canonical = std::filesystem::canonical(buf, ec);
                if (!ec) {
                    return canonical.parent_path();
                }
            } else {
                std::vector<char> dynBuf(size);
                if (_NSGetExecutablePath(dynBuf.data(), &size) == 0) {
                    auto canonical = std::filesystem::canonical(dynBuf.data(), ec);
                    if (!ec) {
                        return canonical.parent_path();
                    }
                }
            }

        #elif defined(__linux__)
            char buf[PATH_MAX];
            ssize_t len = readlink("/proc/self/exe", buf, sizeof(buf) - 1);
            if (len > 0) {
                buf[len] = '\0';
                return std::filesystem::path(buf).parent_path();
            }

        #elif defined(_WIN32)
            char buf[MAX_PATH];
            DWORD len = GetModuleFileNameA(NULL, buf, MAX_PATH);
            if (len > 0 && len < MAX_PATH) {
                return std::filesystem::path(buf).parent_path();
            }
            std::vector<char> dynBuf(32768);
            len = GetModuleFileNameA(NULL, dynBuf.data(), static_cast<DWORD>(dynBuf.size()));
            if (len > 0 && len < dynBuf.size()) {
                return std::filesystem::path(dynBuf.data()).parent_path();
            }
        #endif

        return std::nullopt;
    }
};

//==========================================================================
// Asset Data Structures
//==========================================================================

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

    // Compilation status (internal use - clients should not rely on these)
    bool compiled = false;
    std::string compileError;                    // Error message if compilation failed
    std::uint64_t sourceHash = 0;                // Hash of GLSL source for cache invalidation
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

    // Embedded texture data (for FBX, glTF with embedded textures)
    // If embeddedData is non-empty, use it instead of loading from path
    std::vector<unsigned char> embeddedData;
    int embeddedWidth = 0;
    int embeddedHeight = 0;
    int embeddedChannels = 0;
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
        std::string boneName;  // Name of the bone this channel animates
        int boneIndex = -1;    // Index into skeleton (may be -1 for animation-only files)
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

// NavMesh data structure - stores raw navmesh binary for AI system to process
struct NavMeshData {
    std::vector<unsigned char> fileData;  // Raw navmesh binary
    std::string path;
    std::size_t fileSize = 0;
};

// Data asset structure for JSON, Lua, and other text files
// Note: JSON data is stored as std::any for type-erased access
struct DataAsset {
    std::any jsonData;        // Holds parsed JSON when isJson=true
    std::string rawText;      // Original text content (for Lua, configs, etc)
    bool isJson = false;
};

// Lua material data structure - parsed material definition from Lua files
// Used by Graphics3DSystem to create GPU materials
struct LuaMaterialData {
    std::string name;
    std::string vertexShaderPath;
    std::string fragmentShaderPath;

    // Uniforms stored as std::any - graphics system casts to expected types
    // Common types: float, int, bool, Vec2, Vec3, Vec4, Mat3, Mat4
    std::unordered_map<std::string, std::any> uniforms;

    // Texture slot name -> texture file path
    std::unordered_map<std::string, std::string> texturePaths;

    // Render state
    BlendMode blendMode = BlendMode::Opaque;
    CullMode cullMode = CullMode::Back;
    bool depthWrite = true;
    bool depthTest = true;
    bool hotReload = true;

    std::string path;  // Source file path for hot reload
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

}  // namespace bestow

export namespace bestow {

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
    // Shader Loading
    //======================================================================
    //
    // IMPORTANT: Only GLSL source files are supported (.vert, .frag, .geom, .comp, .tesc, .tese)
    // Direct .spv (SPIR-V) file loading is NOT supported - provide GLSL source instead.
    // The engine compiles to SPIR-V internally when needed (for Vulkan backends).
    //
    // For OpenGL backends: use loadShader() - OpenGL compiles GLSL on the GPU
    // For Vulkan backends: use loadShaderCompiled() - returns GLSL + pre-compiled SPIR-V
    //

    /// Load a GLSL shader file (source only - for OpenGL backends)
    /// Shader stage is inferred from file extension (.vert, .frag, .geom, .comp, .tesc, .tese)
    /// Returns ShaderData with glslSource populated, spirvBytecode empty
    virtual AssetHandle loadShader(const std::filesystem::path& glslPath) = 0;

    /// Load a GLSL shader file with SPIR-V compilation (for Vulkan backends)
    /// Shader stage is inferred from file extension (.vert, .frag, .geom, .comp, .tesc, .tese)
    /// Returns ShaderData with both glslSource and spirvBytecode populated
    /// Compilation is cached - only recompiles if GLSL source hash changes
    virtual AssetHandle loadShaderCompiled(const std::filesystem::path& glslPath) = 0;

    /// Get shader data (source always present, SPIR-V only if loadShaderCompiled was used)
    virtual const ShaderData* getShaderData(AssetHandle handle) const = 0;

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

    //======================================================================
    // Audio Asset Loading
    //======================================================================

    /// Get loaded sound data (WAV, OGG, etc.) for a Sound/Music asset
    /// Returns nullptr if handle is invalid, asset not loaded, or wrong type
    virtual const SoundData* getSoundData(AssetHandle handle) const = 0;

    //======================================================================
    // Lua Material Loading
    //======================================================================
    //
    // Lua materials are material definitions written in Lua files.
    // They specify shader paths, uniforms, textures, and render state.
    // The AssetSystem parses the Lua and returns LuaMaterialData.
    // Graphics systems use this data to create GPU resources.
    //

    /// Load a Lua material file (.lua) and parse it into LuaMaterialData
    /// Returns an AssetHandle that can be used with getLuaMaterialData()
    /// Hot reload is supported via AssetSystem subscriptions
    virtual AssetHandle loadMaterial(const std::filesystem::path& luaPath) = 0;

    /// Get parsed Lua material data from a handle returned by loadMaterial()
    /// Returns nullptr if handle is invalid or asset not loaded
    virtual const LuaMaterialData* getLuaMaterialData(AssetHandle handle) const = 0;

    //======================================================================
    // Library Discovery (for IDE Autocomplete)
    //======================================================================
    //
    // These methods enable discovery of available assets in the library
    // for IDE autocomplete and runtime exploration. Useful for Lua bindings
    // that expose library contents as nested tables for autocomplete.
    //

    /// Get the asset library instance (for path resolution and listing)
    /// Returns nullopt if library was not found during initialization
    virtual std::optional<AssetLibrary> getAssetLibrary() const = 0;

    /// List all assets in a library subdirectory (non-recursive)
    /// Example: listLibraryAssets("shaders") returns all files in :library:/shaders/
    virtual std::vector<LibraryAssetInfo> listLibraryAssets(std::string_view relativeDir = "") const = 0;

    /// List all assets in a library subdirectory (recursive)
    /// Returns all files (not directories) in the subtree
    virtual std::vector<LibraryAssetInfo> listLibraryAssetsRecursive(std::string_view relativeDir = "") const = 0;

    /// Get all top-level categories (directories) in the library
    /// Example: returns ["shaders", "textures", "fonts", "materials"]
    virtual std::vector<std::string> listLibraryCategories() const = 0;
};

}  // namespace bestow

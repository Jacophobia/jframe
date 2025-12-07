// bestow-assets/src/bestow.assets.impl.cppm
// Asset system implementation

module;

// Note: nlohmann/json is NOT included here due to MSVC C++23 module compatibility issues.
// JSON data is stored as std::any and accessed via getJson()/setJson() helper functions.
// The actual nlohmann/json include is in AssetSystem.cpp.

#include <cstddef>  // For size_t

export module bestow.assets.impl;

import std;
import bestow.assets;
import bestow.types;

export namespace bestow {

// Note: TextureData is now in bestow.assets contract module

// Data asset structure for JSON and text files
// Note: JSON data is stored as std::any for MSVC C++23 module compatibility.
// Use getJson() and setJson() helper functions to access the parsed JSON.
struct DataAsset {
    std::any jsonData;        // Holds nlohmann::json when isJson=true (type-erased for MSVC compatibility)
    std::string rawText;      // Original text (for non-JSON or Lua)
    bool isJson = false;
};

// Helper function declarations for JSON access (defined in AssetSystem.cpp)
// These provide type-safe access to the type-erased JSON data.
void setDataAssetJson(DataAsset& asset, const std::string& jsonText);
bool hasDataAssetJson(const DataAsset& asset);

// Template to get JSON value - returns default if not JSON or key doesn't exist
// Usage: auto name = getJsonValue<std::string>(dataAsset, "name", "default");
template<typename T>
T getJsonValue(const DataAsset& asset, const std::string& key, const T& defaultValue);

// Get raw JSON object - only call if hasDataAssetJson returns true
// Returns a reference to the underlying nlohmann::json stored in std::any
const std::any& getDataAssetJsonAny(const DataAsset& asset);

// Note: SoundData is now defined in bestow.assets contract module

// Note: FontData is now defined in bestow.assets contract module

// Shader data structure - stores shader source code
struct ShaderData {
    std::string vertexSource;    // Vertex shader source (if .vert file)
    std::string fragmentSource;  // Fragment shader source (if .frag file)
    std::string source;          // Combined source (if single file)
    std::string path;
};

// NavMesh data structure - stores raw navmesh binary for AI system to process
struct NavMeshData {
    std::vector<unsigned char> fileData;  // Raw navmesh binary
    std::string path;
    size_t fileSize = 0;
};

// BehaviorTree data structure - stores tree definition for AI system
// Note: JSON data is stored as std::any for MSVC C++23 module compatibility.
struct BehaviorTreeData {
    std::any treeData;        // Holds nlohmann::json when isJson=true (type-erased for MSVC compatibility)
    std::string rawText;      // Original file content
    std::string path;
    bool isJson = false;
};

// Helper function declarations for BehaviorTreeData JSON access
void setBehaviorTreeJson(BehaviorTreeData& data, const std::string& jsonText);
bool hasBehaviorTreeJson(const BehaviorTreeData& data);
const std::any& getBehaviorTreeJsonAny(const BehaviorTreeData& data);

class AssetSystem : public IAssetSystem {
public:
    AssetSystem() = default;
    ~AssetSystem() override = default;

    void update() override;

    // Registration
    AssetHandle registerAsset(AssetType type, const std::filesystem::path& path) override;
    void unregisterAsset(AssetHandle handle) override;

    // Loading
    void loadAsset(AssetHandle handle) override;
    void loadAssetAsync(AssetHandle handle, AssetLoadCallback callback = nullptr) override;
    void unloadAsset(AssetHandle handle) override;

    // State queries
    AssetState getAssetState(AssetHandle handle) const override;
    AssetMetadata getAssetMetadata(AssetHandle handle) const override;
    bool isLoaded(AssetHandle handle) const override;

    // Raw data access
    void* getRawAsset(AssetHandle handle) override;
    const void* getRawAsset(AssetHandle handle) const override;

    // Bulk operations
    void loadAll() override;
    void unloadAll() override;
    std::vector<AssetHandle> getAssetsOfType(AssetType type) const override;

    // Hot reload
    void enableHotReload(bool enable) override;
    void checkForReloads() override;
    void reloadAsset(AssetHandle handle) override;

    // 3D Asset loading and access
    const MeshData* getMeshData(AssetHandle handle) const override;
    const ModelData* getModelData(AssetHandle handle) const override;
    const MaterialData* getMaterialData(AssetHandle handle) const override;
    const CubemapData* getCubemapData(AssetHandle handle) const override;
    AssetHandle loadMesh(const std::filesystem::path& path) override;
    AssetHandle loadModel(const std::filesystem::path& path) override;
    AssetHandle loadCubemap(const std::filesystem::path& path) override;
    AssetHandle loadCubemap(
        const std::filesystem::path& right,
        const std::filesystem::path& left,
        const std::filesystem::path& top,
        const std::filesystem::path& bottom,
        const std::filesystem::path& front,
        const std::filesystem::path& back) override;

private:
    struct AssetEntry {
        AssetMetadata metadata;
        std::any data;  // Can hold DataAsset, or other asset types
        std::size_t dataSize = 0;
        std::optional<std::filesystem::file_time_type> lastWriteTime;
    };

    struct PendingLoad {
        AssetHandle handle;
        AssetLoadCallback callback;
        std::future<void> future;
    };

    UUID generateUUID();
    void loadAssetImpl(AssetHandle handle);  // Thread-safe loading implementation

    std::unordered_map<UUID, AssetEntry> assets_;
    std::vector<PendingLoad> pendingLoads_;
    mutable std::mutex assetsMutex_;  // Protects assets_ during async loads
    bool hotReloadEnabled_ = false;
    UUID nextUUID_ = 1;
};

// Factory function (exported via namespace)
inline std::unique_ptr<IAssetSystem> createAssetSystem() {
    return std::make_unique<AssetSystem>();
}

}  // namespace bestow

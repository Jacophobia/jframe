// jframe-assets/src/jframe.assets.impl.cppm
// Asset system implementation

module;

#include <nlohmann/json.hpp>

export module jframe.assets.impl;

import std;
import jframe.assets;
import jframe.types;

export namespace jframe {

// Texture data structure for loaded textures
struct TextureData {
    std::vector<unsigned char> pixels;
    int width = 0;
    int height = 0;
    int channels = 0;
};

// Data asset structure for JSON and text files
struct DataAsset {
    nlohmann::json jsonData;  // Parsed JSON
    std::string rawText;      // Original text (for non-JSON or Lua)
    bool isJson = false;
};

// Sound/Music data structure - stores raw file bytes for FMOD to consume
struct SoundData {
    std::vector<unsigned char> fileData;  // Raw file bytes
    std::string path;
    size_t fileSize = 0;
};

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

private:
    struct AssetEntry {
        AssetMetadata metadata;
        std::any data;  // Can hold DataAsset, or other asset types
        std::size_t dataSize = 0;
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

}  // namespace jframe

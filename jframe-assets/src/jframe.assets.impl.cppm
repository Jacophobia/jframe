// jframe-assets/src/jframe.assets.impl.cppm
// Asset system implementation

module;

export module jframe.assets.impl;

import std;
import jframe.assets;
import jframe.types;

export namespace jframe {

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
        std::unique_ptr<std::byte[]> data;
        std::size_t dataSize = 0;
    };

    UUID generateUUID();

    std::unordered_map<UUID, AssetEntry> assets_;
    std::vector<std::pair<AssetHandle, AssetLoadCallback>> pendingLoads_;
    bool hotReloadEnabled_ = false;
    UUID nextUUID_ = 1;
};

// Factory function (exported via namespace)
inline std::unique_ptr<IAssetSystem> createAssetSystem() {
    return std::make_unique<AssetSystem>();
}

}  // namespace jframe

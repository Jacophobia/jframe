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

struct AssetMetadata {
    AssetHandle handle;
    std::filesystem::path sourcePath;
    AssetState state = AssetState::Unloaded;
    std::size_t sizeBytes = 0;
    std::optional<std::string> errorMessage;
};

using AssetLoadCallback = std::function<void(AssetHandle, AssetState)>;

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
};

}  // namespace bestow

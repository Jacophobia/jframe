// tests/mocks/MockAssetSystem.hpp
// Shared mock asset system for testing

#pragma once

#include <kangaru/kangaru.hpp>

import std;
import bestow;
import bestow.types;

namespace bestow::tests {

class MockAssetSystem : public IAssetSystem {
public:
    // Tracking
    int loadCount = 0;

    // Delegates
    std::function<void()> onUpdate = [] {};
    std::function<AssetHandle(AssetType, const std::filesystem::path&)> onRegisterAsset =
        [this](AssetType type, const std::filesystem::path&) {
            return AssetHandle{nextId_++, type};
        };
    std::function<void(AssetHandle)> onUnregisterAsset = [](AssetHandle) {};
    std::function<void(AssetHandle)> onLoadAsset = [this](AssetHandle) { loadCount++; };
    std::function<void(AssetHandle, AssetLoadCallback)> onLoadAssetAsync =
        [this](AssetHandle h, AssetLoadCallback cb) {
            loadCount++;
            if (cb) cb(h, AssetState::Loaded);
        };
    std::function<void(AssetHandle)> onUnloadAsset = [](AssetHandle) {};
    std::function<AssetState(AssetHandle)> onGetAssetState =
        [](AssetHandle) { return AssetState::Loaded; };
    std::function<AssetMetadata(AssetHandle)> onGetAssetMetadata =
        [](AssetHandle) { return AssetMetadata{}; };
    std::function<bool(AssetHandle)> onIsLoaded = [](AssetHandle) { return true; };
    std::function<void*(AssetHandle)> onGetRawAsset = [](AssetHandle) -> void* { return nullptr; };
    std::function<const void*(AssetHandle)> onGetRawAssetConst =
        [](AssetHandle) -> const void* { return nullptr; };
    std::function<void()> onLoadAll = [] {};
    std::function<void()> onUnloadAll = [] {};
    std::function<std::vector<AssetHandle>(AssetType)> onGetAssetsOfType =
        [](AssetType) { return std::vector<AssetHandle>{}; };
    std::function<void(bool)> onEnableHotReload = [](bool) {};
    std::function<void()> onCheckForReloads = [] {};
    std::function<void(AssetHandle)> onReloadAsset = [](AssetHandle) {};
    std::function<SubscriptionId(AssetHandle, AssetChangeCallback)> onSubscribe =
        [this](AssetHandle, AssetChangeCallback) { return nextSubId_++; };
    std::function<SubscriptionId(AssetType, AssetChangeCallback)> onSubscribeToType =
        [this](AssetType, AssetChangeCallback) { return nextSubId_++; };
    std::function<void(SubscriptionId)> onUnsubscribe = [](SubscriptionId) {};
    std::function<AssetHandle(const std::filesystem::path&)> onLoadShader =
        [](const std::filesystem::path&) { return AssetHandle{}; };
    std::function<AssetHandle(const std::filesystem::path&)> onLoadShaderCompiled =
        [](const std::filesystem::path&) { return AssetHandle{}; };
    std::function<const ShaderData*(AssetHandle)> onGetShaderData =
        [](AssetHandle) -> const ShaderData* { return nullptr; };
    std::function<const MeshData*(AssetHandle)> onGetMeshData =
        [](AssetHandle) -> const MeshData* { return nullptr; };
    std::function<const ModelData*(AssetHandle)> onGetModelData =
        [](AssetHandle) -> const ModelData* { return nullptr; };
    std::function<const MaterialData*(AssetHandle)> onGetMaterialData =
        [](AssetHandle) -> const MaterialData* { return nullptr; };
    std::function<const CubemapData*(AssetHandle)> onGetCubemapData =
        [](AssetHandle) -> const CubemapData* { return nullptr; };
    std::function<AssetHandle(const std::filesystem::path&)> onLoadMesh =
        [](const std::filesystem::path&) { return AssetHandle{}; };
    std::function<AssetHandle(const std::filesystem::path&)> onLoadModel =
        [](const std::filesystem::path&) { return AssetHandle{}; };
    std::function<AssetHandle(const std::filesystem::path&)> onLoadCubemap =
        [](const std::filesystem::path&) { return AssetHandle{}; };
    std::function<AssetHandle(const std::filesystem::path&, const std::filesystem::path&,
                              const std::filesystem::path&, const std::filesystem::path&,
                              const std::filesystem::path&, const std::filesystem::path&)>
        onLoadCubemap6 =
            [](const std::filesystem::path&, const std::filesystem::path&,
               const std::filesystem::path&, const std::filesystem::path&,
               const std::filesystem::path&, const std::filesystem::path&) { return AssetHandle{}; };
    std::function<const SoundData*(AssetHandle)> onGetSoundData =
        [](AssetHandle) -> const SoundData* { return nullptr; };
    std::function<AssetHandle(const std::filesystem::path&)> onLoadMaterial =
        [](const std::filesystem::path&) { return AssetHandle{}; };
    std::function<const LuaMaterialData*(AssetHandle)> onGetLuaMaterialData =
        [](AssetHandle) -> const LuaMaterialData* { return nullptr; };
    std::function<std::optional<AssetLibrary>()> onGetAssetLibrary =
        [] { return std::nullopt; };
    std::function<std::vector<LibraryAssetInfo>(std::string_view)> onListLibraryAssets =
        [](std::string_view) { return std::vector<LibraryAssetInfo>{}; };
    std::function<std::vector<LibraryAssetInfo>(std::string_view)> onListLibraryAssetsRecursive =
        [](std::string_view) { return std::vector<LibraryAssetInfo>{}; };
    std::function<std::vector<std::string>()> onListLibraryCategories =
        [] { return std::vector<std::string>{}; };

    // IAssetSystem overrides
    void update() override { onUpdate(); }
    AssetHandle registerAsset(AssetType type, const std::filesystem::path& path) override {
        return onRegisterAsset(type, path);
    }
    void unregisterAsset(AssetHandle handle) override { onUnregisterAsset(handle); }
    void loadAsset(AssetHandle handle) override { onLoadAsset(handle); }
    void loadAssetAsync(AssetHandle handle, AssetLoadCallback callback) override {
        onLoadAssetAsync(handle, callback);
    }
    void unloadAsset(AssetHandle handle) override { onUnloadAsset(handle); }
    AssetState getAssetState(AssetHandle handle) const override { return onGetAssetState(handle); }
    AssetMetadata getAssetMetadata(AssetHandle handle) const override {
        return onGetAssetMetadata(handle);
    }
    bool isLoaded(AssetHandle handle) const override { return onIsLoaded(handle); }
    void* getRawAsset(AssetHandle handle) override { return onGetRawAsset(handle); }
    const void* getRawAsset(AssetHandle handle) const override {
        return onGetRawAssetConst(handle);
    }
    void loadAll() override { onLoadAll(); }
    void unloadAll() override { onUnloadAll(); }
    std::vector<AssetHandle> getAssetsOfType(AssetType type) const override {
        return onGetAssetsOfType(type);
    }
    void enableHotReload(bool enable) override { onEnableHotReload(enable); }
    void checkForReloads() override { onCheckForReloads(); }
    void reloadAsset(AssetHandle handle) override { onReloadAsset(handle); }
    SubscriptionId subscribe(AssetHandle handle, AssetChangeCallback callback) override {
        return onSubscribe(handle, callback);
    }
    SubscriptionId subscribeToType(AssetType type, AssetChangeCallback callback) override {
        return onSubscribeToType(type, callback);
    }
    void unsubscribe(SubscriptionId id) override { onUnsubscribe(id); }
    AssetHandle loadShader(const std::filesystem::path& path) override {
        return onLoadShader(path);
    }
    AssetHandle loadShaderCompiled(const std::filesystem::path& path) override {
        return onLoadShaderCompiled(path);
    }
    const ShaderData* getShaderData(AssetHandle handle) const override {
        return onGetShaderData(handle);
    }
    const MeshData* getMeshData(AssetHandle handle) const override {
        return onGetMeshData(handle);
    }
    const ModelData* getModelData(AssetHandle handle) const override {
        return onGetModelData(handle);
    }
    const MaterialData* getMaterialData(AssetHandle handle) const override {
        return onGetMaterialData(handle);
    }
    const CubemapData* getCubemapData(AssetHandle handle) const override {
        return onGetCubemapData(handle);
    }
    AssetHandle loadMesh(const std::filesystem::path& path) override {
        return onLoadMesh(path);
    }
    AssetHandle loadModel(const std::filesystem::path& path) override {
        return onLoadModel(path);
    }
    AssetHandle loadCubemap(const std::filesystem::path& path) override {
        return onLoadCubemap(path);
    }
    AssetHandle loadCubemap(const std::filesystem::path& posX, const std::filesystem::path& negX,
                            const std::filesystem::path& posY, const std::filesystem::path& negY,
                            const std::filesystem::path& posZ,
                            const std::filesystem::path& negZ) override {
        return onLoadCubemap6(posX, negX, posY, negY, posZ, negZ);
    }
    const SoundData* getSoundData(AssetHandle handle) const override {
        return onGetSoundData(handle);
    }
    AssetHandle loadMaterial(const std::filesystem::path& path) override {
        return onLoadMaterial(path);
    }
    const LuaMaterialData* getLuaMaterialData(AssetHandle handle) const override {
        return onGetLuaMaterialData(handle);
    }
    std::optional<AssetLibrary> getAssetLibrary() const override {
        return onGetAssetLibrary();
    }
    std::vector<LibraryAssetInfo> listLibraryAssets(std::string_view dir) const override {
        return onListLibraryAssets(dir);
    }
    std::vector<LibraryAssetInfo> listLibraryAssetsRecursive(std::string_view dir) const override {
        return onListLibraryAssetsRecursive(dir);
    }
    std::vector<std::string> listLibraryCategories() const override {
        return onListLibraryCategories();
    }

private:
    UUID nextId_ = 1;
    SubscriptionId nextSubId_ = 1;
};

}  // namespace bestow::tests

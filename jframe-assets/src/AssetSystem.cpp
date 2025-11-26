// jframe-assets/src/AssetSystem.cpp
// Asset system implementation

module;

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

module jframe.assets.impl;

namespace jframe {

UUID AssetSystem::generateUUID() {
    return nextUUID_++;
}

void AssetSystem::update() {
    // Process pending async loads
    for (auto it = pendingLoads_.begin(); it != pendingLoads_.end();) {
        auto& [handle, callback] = *it;
        auto state = getAssetState(handle);
        if (state != AssetState::Loading) {
            if (callback) {
                callback(handle, state);
            }
            it = pendingLoads_.erase(it);
        } else {
            ++it;
        }
    }
}

AssetHandle AssetSystem::registerAsset(AssetType type, const std::filesystem::path& path) {
    AssetHandle handle{generateUUID(), type};

    AssetEntry entry;
    entry.metadata.handle = handle;
    entry.metadata.sourcePath = path;
    entry.metadata.state = AssetState::Unloaded;

    assets_[handle.uuid] = std::move(entry);
    return handle;
}

void AssetSystem::unregisterAsset(AssetHandle handle) {
    assets_.erase(handle.uuid);
}

void AssetSystem::loadAsset(AssetHandle handle) {
    auto it = assets_.find(handle.uuid);
    if (it == assets_.end()) return;

    auto& entry = it->second;
    entry.metadata.state = AssetState::Loading;

    // Load based on type
    try {
        // Actual loading would happen here based on AssetType
        entry.metadata.state = AssetState::Loaded;
    } catch (...) {
        entry.metadata.state = AssetState::Failed;
        entry.metadata.errorMessage = "Failed to load asset";
    }
}

void AssetSystem::loadAssetAsync(AssetHandle handle, AssetLoadCallback callback) {
    auto it = assets_.find(handle.uuid);
    if (it == assets_.end()) return;

    it->second.metadata.state = AssetState::Loading;
    pendingLoads_.emplace_back(handle, std::move(callback));

    // In a real implementation, this would spawn a task
    loadAsset(handle);
}

void AssetSystem::unloadAsset(AssetHandle handle) {
    auto it = assets_.find(handle.uuid);
    if (it == assets_.end()) return;

    it->second.data.reset();
    it->second.dataSize = 0;
    it->second.metadata.state = AssetState::Unloaded;
}

AssetState AssetSystem::getAssetState(AssetHandle handle) const {
    auto it = assets_.find(handle.uuid);
    return it != assets_.end() ? it->second.metadata.state : AssetState::Unloaded;
}

AssetMetadata AssetSystem::getAssetMetadata(AssetHandle handle) const {
    auto it = assets_.find(handle.uuid);
    return it != assets_.end() ? it->second.metadata : AssetMetadata{};
}

bool AssetSystem::isLoaded(AssetHandle handle) const {
    return getAssetState(handle) == AssetState::Loaded;
}

void* AssetSystem::getRawAsset(AssetHandle handle) {
    auto it = assets_.find(handle.uuid);
    return it != assets_.end() ? it->second.data.get() : nullptr;
}

const void* AssetSystem::getRawAsset(AssetHandle handle) const {
    auto it = assets_.find(handle.uuid);
    return it != assets_.end() ? it->second.data.get() : nullptr;
}

void AssetSystem::loadAll() {
    for (auto& [uuid, entry] : assets_) {
        if (entry.metadata.state == AssetState::Unloaded) {
            loadAsset(entry.metadata.handle);
        }
    }
}

void AssetSystem::unloadAll() {
    for (auto& [uuid, entry] : assets_) {
        unloadAsset(entry.metadata.handle);
    }
}

std::vector<AssetHandle> AssetSystem::getAssetsOfType(AssetType type) const {
    std::vector<AssetHandle> result;
    for (const auto& [uuid, entry] : assets_) {
        if (entry.metadata.handle.type == type) {
            result.push_back(entry.metadata.handle);
        }
    }
    return result;
}

void AssetSystem::enableHotReload(bool enable) {
    hotReloadEnabled_ = enable;
}

void AssetSystem::checkForReloads() {
    if (!hotReloadEnabled_) return;
    // Check file modification times and reload changed assets
}

void AssetSystem::reloadAsset(AssetHandle handle) {
    unloadAsset(handle);
    loadAsset(handle);
}

}  // namespace jframe

// jframe-assets/src/AssetSystem.cpp
// Asset system implementation

module;

#include <any>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <functional>
#include <memory>
#include <nlohmann/json.hpp>
#include <optional>
#include <sstream>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

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
        switch (handle.type) {
            case AssetType::Data: {
                // Load data file (JSON or other text format)
                std::ifstream file(entry.metadata.sourcePath);
                if (!file.is_open()) {
                    throw std::runtime_error("Failed to open file: " + entry.metadata.sourcePath.string());
                }

                std::stringstream buffer;
                buffer << file.rdbuf();
                std::string fileContents = buffer.str();

                DataAsset dataAsset;
                dataAsset.rawText = fileContents;

                // Try to parse as JSON
                try {
                    dataAsset.jsonData = nlohmann::json::parse(fileContents);
                    dataAsset.isJson = true;
                } catch (const nlohmann::json::parse_error&) {
                    // Not JSON, keep as raw text
                    dataAsset.isJson = false;
                }

                entry.data = std::move(dataAsset);
                entry.dataSize = fileContents.size();
                break;
            }

            case AssetType::Level: {
                // Load Lua level file
                std::ifstream file(entry.metadata.sourcePath);
                if (!file.is_open()) {
                    throw std::runtime_error("Failed to open file: " + entry.metadata.sourcePath.string());
                }

                std::stringstream buffer;
                buffer << file.rdbuf();
                std::string luaContents = buffer.str();

                DataAsset levelAsset;
                levelAsset.rawText = luaContents;
                levelAsset.isJson = false;  // Lua files are not JSON

                entry.data = std::move(levelAsset);
                entry.dataSize = luaContents.size();
                break;
            }

            case AssetType::Texture: {
                // Load texture using stb_image
                std::string pathStr = entry.metadata.sourcePath.string();

                int width, height, channels;
                unsigned char* pixels = stbi_load(pathStr.c_str(), &width, &height, &channels, 0);

                if (!pixels) {
                    std::string error = "Failed to load texture: ";
                    error += pathStr;
                    const char* stbiError = stbi_failure_reason();
                    if (stbiError) {
                        error += " - ";
                        error += stbiError;
                    }
                    throw std::runtime_error(error);
                }

                // Create TextureData and copy pixels
                TextureData textureData;
                textureData.width = width;
                textureData.height = height;
                textureData.channels = channels;

                // Copy pixel data into vector
                std::size_t pixelDataSize = width * height * channels;
                textureData.pixels.resize(pixelDataSize);
                std::memcpy(textureData.pixels.data(), pixels, pixelDataSize);

                // Free stb_image memory
                stbi_image_free(pixels);

                entry.data = std::move(textureData);
                entry.dataSize = pixelDataSize;
                break;
            }

            case AssetType::Sound:
            case AssetType::Music: {
                // Load audio file as raw bytes for FMOD to consume
                // Open file in binary mode
                std::ifstream file(entry.metadata.sourcePath, std::ios::binary | std::ios::ate);
                if (!file.is_open()) {
                    throw std::runtime_error("Failed to open audio file: " + entry.metadata.sourcePath.string());
                }

                // Get file size
                auto fileSize = file.tellg();
                file.seekg(0, std::ios::beg);

                // Read entire file into memory
                SoundData soundData;
                soundData.path = entry.metadata.sourcePath.string();
                soundData.fileSize = static_cast<size_t>(fileSize);
                soundData.fileData.resize(soundData.fileSize);

                if (!file.read(reinterpret_cast<char*>(soundData.fileData.data()), fileSize)) {
                    throw std::runtime_error("Failed to read audio file: " + entry.metadata.sourcePath.string());
                }

                // Store size before moving
                std::size_t dataSize = soundData.fileSize;
                entry.data = std::move(soundData);
                entry.dataSize = dataSize;
                break;
            }

            default:
                // Other asset types not yet implemented
                throw std::runtime_error("Asset type not yet implemented");
        }

        entry.metadata.state = AssetState::Loaded;
    } catch (const std::exception& e) {
        entry.metadata.state = AssetState::Failed;
        entry.metadata.errorMessage = e.what();
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

    it->second.data.reset();  // std::any::reset() clears the held value
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
    if (it == assets_.end() || !it->second.data.has_value()) {
        return nullptr;
    }
    return &it->second.data;
}

const void* AssetSystem::getRawAsset(AssetHandle handle) const {
    auto it = assets_.find(handle.uuid);
    if (it == assets_.end() || !it->second.data.has_value()) {
        return nullptr;
    }
    return &it->second.data;
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

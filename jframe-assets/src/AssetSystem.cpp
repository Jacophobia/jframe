// jframe-assets/src/AssetSystem.cpp
// Asset system implementation

module;

#include <any>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <functional>
#include <future>
#include <memory>
#include <mutex>
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
        // Check if the future is ready (non-blocking)
        if (it->future.wait_for(std::chrono::seconds(0)) == std::future_status::ready) {
            // Future is complete, get any exception that may have occurred
            try {
                it->future.get();  // This will rethrow any exception from the worker thread
            } catch (...) {
                // Exception already caught and stored in asset state by loadAssetImpl
            }

            // Get the final state and invoke callback on main thread
            auto state = getAssetState(it->handle);
            if (it->callback) {
                it->callback(it->handle, state);
            }

            // Remove completed load from pending list
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
    // Synchronous load - just call the implementation directly
    loadAssetImpl(handle);
}

void AssetSystem::loadAssetImpl(AssetHandle handle) {
    // Thread-safe asset loading implementation
    // Get the source path while holding the lock
    std::filesystem::path sourcePath;
    {
        std::lock_guard<std::mutex> lock(assetsMutex_);
        auto it = assets_.find(handle.uuid);
        if (it == assets_.end()) return;
        sourcePath = it->second.metadata.sourcePath;
        it->second.metadata.state = AssetState::Loading;
    }

    // Load the file WITHOUT holding the lock (this is the slow I/O part)
    std::any loadedData;
    std::size_t loadedSize = 0;
    std::optional<std::string> errorMessage;

    try {
        switch (handle.type) {
            case AssetType::Data: {
                // Load data file (JSON or other text format)
                std::ifstream file(sourcePath);
                if (!file.is_open()) {
                    throw std::runtime_error("Failed to open file: " + sourcePath.string());
                }

                std::stringstream buffer;
                buffer << file.rdbuf();
                std::string fileContents = buffer.str();

                DataAsset dataAsset;
                dataAsset.rawText = fileContents;

                // Try to parse as JSON - use accept() first to avoid exceptions
                if (nlohmann::json::accept(fileContents)) {
                    // Store parsed JSON in std::any for MSVC module compatibility
                    dataAsset.jsonData = nlohmann::json::parse(fileContents);
                    dataAsset.isJson = true;
                } else {
                    // Not valid JSON, keep as raw text only
                    dataAsset.isJson = false;
                }

                loadedData = std::move(dataAsset);
                loadedSize = fileContents.size();
                break;
            }

            case AssetType::Level: {
                // Load Lua level file
                std::ifstream file(sourcePath);
                if (!file.is_open()) {
                    throw std::runtime_error("Failed to open file: " + sourcePath.string());
                }

                std::stringstream buffer;
                buffer << file.rdbuf();
                std::string luaContents = buffer.str();

                DataAsset levelAsset;
                levelAsset.rawText = luaContents;
                levelAsset.isJson = false;  // Lua files are not JSON

                loadedData = std::move(levelAsset);
                loadedSize = luaContents.size();
                break;
            }

            case AssetType::Texture: {
                // Load texture using stb_image
                std::string pathStr = sourcePath.string();

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

                loadedData = std::move(textureData);
                loadedSize = pixelDataSize;
                break;
            }

            case AssetType::Sound:
            case AssetType::Music: {
                // Load audio file as raw bytes for FMOD to consume
                // Open file in binary mode
                std::ifstream file(sourcePath, std::ios::binary | std::ios::ate);
                if (!file.is_open()) {
                    throw std::runtime_error("Failed to open audio file: " + sourcePath.string());
                }

                // Get file size
                auto fileSize = file.tellg();
                file.seekg(0, std::ios::beg);

                // Read entire file into memory
                SoundData soundData;
                soundData.path = sourcePath.string();
                soundData.fileSize = static_cast<size_t>(fileSize);
                soundData.fileData.resize(soundData.fileSize);

                if (!file.read(reinterpret_cast<char*>(soundData.fileData.data()), fileSize)) {
                    throw std::runtime_error("Failed to read audio file: " + sourcePath.string());
                }

                // Store size before moving
                loadedSize = soundData.fileSize;
                loadedData = std::move(soundData);
                break;
            }

            case AssetType::Font: {
                // Load font file as raw bytes for FreeType to process later
                std::ifstream file(sourcePath, std::ios::binary | std::ios::ate);
                if (!file.is_open()) {
                    throw std::runtime_error("Failed to open font file: " + sourcePath.string());
                }

                // Get file size
                auto fileSize = file.tellg();
                file.seekg(0, std::ios::beg);

                // Read entire file into memory
                FontData fontData;
                fontData.path = sourcePath.string();
                fontData.fileSize = static_cast<size_t>(fileSize);
                fontData.fileData.resize(fontData.fileSize);

                if (!file.read(reinterpret_cast<char*>(fontData.fileData.data()), fileSize)) {
                    throw std::runtime_error("Failed to read font file: " + sourcePath.string());
                }

                // Store size before moving
                loadedSize = fontData.fileSize;
                loadedData = std::move(fontData);
                break;
            }

            case AssetType::Shader: {
                // Load shader source as text file
                std::ifstream file(sourcePath);
                if (!file.is_open()) {
                    throw std::runtime_error("Failed to open shader file: " + sourcePath.string());
                }

                std::stringstream buffer;
                buffer << file.rdbuf();
                std::string shaderSource = buffer.str();

                ShaderData shaderData;
                shaderData.path = sourcePath.string();
                shaderData.source = shaderSource;

                // For now, just load into 'source' field
                // The Graphics system can split vertex/fragment later if needed
                // Could also check extension (.vert, .frag) here if desired

                loadedData = std::move(shaderData);
                loadedSize = shaderSource.size();
                break;
            }

            case AssetType::NavMesh: {
                // Load navmesh file as raw binary data for AI system to process
                std::ifstream file(sourcePath, std::ios::binary | std::ios::ate);
                if (!file.is_open()) {
                    throw std::runtime_error("Failed to open navmesh file: " + sourcePath.string());
                }

                // Get file size
                auto fileSize = file.tellg();
                file.seekg(0, std::ios::beg);

                // Read entire file into memory
                NavMeshData navMeshData;
                navMeshData.path = sourcePath.string();
                navMeshData.fileSize = static_cast<size_t>(fileSize);
                navMeshData.fileData.resize(navMeshData.fileSize);

                if (!file.read(reinterpret_cast<char*>(navMeshData.fileData.data()), fileSize)) {
                    throw std::runtime_error("Failed to read navmesh file: " + sourcePath.string());
                }

                // Store size before moving
                loadedSize = navMeshData.fileSize;
                loadedData = std::move(navMeshData);
                break;
            }

            case AssetType::BehaviorTree: {
                // Load behavior tree definition (try JSON, fallback to raw text)
                std::ifstream file(sourcePath);
                if (!file.is_open()) {
                    throw std::runtime_error("Failed to open behavior tree file: " + sourcePath.string());
                }

                std::stringstream buffer;
                buffer << file.rdbuf();
                std::string fileContents = buffer.str();

                BehaviorTreeData btData;
                btData.path = sourcePath.string();
                btData.rawText = fileContents;

                // Try to parse as JSON - use accept() first to avoid exceptions
                if (nlohmann::json::accept(fileContents)) {
                    // Store parsed JSON in std::any for MSVC module compatibility
                    btData.treeData = nlohmann::json::parse(fileContents);
                    btData.isJson = true;
                } else {
                    // Not valid JSON, keep as raw text only
                    btData.isJson = false;
                }

                loadedData = std::move(btData);
                loadedSize = fileContents.size();
                break;
            }

            default:
                // Other asset types not yet implemented
                throw std::runtime_error("Asset type not yet implemented");
        }
    } catch (const std::exception& e) {
        errorMessage = e.what();
    } catch (...) {
        errorMessage = "Failed to load asset";
    }

    // Update the asset entry with loaded data (quick lock)
    {
        std::lock_guard<std::mutex> lock(assetsMutex_);
        auto it = assets_.find(handle.uuid);
        if (it == assets_.end()) return;

        auto& entry = it->second;
        if (errorMessage) {
            entry.metadata.state = AssetState::Failed;
            entry.metadata.errorMessage = *errorMessage;
        } else {
            entry.data = std::move(loadedData);
            entry.dataSize = loadedSize;
            entry.metadata.state = AssetState::Loaded;

            // Store file modification time for hot reload
            std::error_code ec;
            auto writeTime = std::filesystem::last_write_time(sourcePath, ec);
            if (!ec) {
                entry.lastWriteTime = writeTime;
            }
        }
    }
}

void AssetSystem::loadAssetAsync(AssetHandle handle, AssetLoadCallback callback) {
    // Verify asset exists before launching async task
    {
        std::lock_guard<std::mutex> lock(assetsMutex_);
        auto it = assets_.find(handle.uuid);
        if (it == assets_.end()) return;
        it->second.metadata.state = AssetState::Loading;
    }

    // Launch async loading task
    std::future<void> future = std::async(std::launch::async, [this, handle]() {
        loadAssetImpl(handle);
    });

    // Store pending load for callback processing in update()
    pendingLoads_.push_back(PendingLoad{
        .handle = handle,
        .callback = std::move(callback),
        .future = std::move(future)
    });
}

void AssetSystem::unloadAsset(AssetHandle handle) {
    auto it = assets_.find(handle.uuid);
    if (it == assets_.end()) return;

    it->second.data.reset();  // std::any::reset() clears the held value
    it->second.dataSize = 0;
    it->second.metadata.state = AssetState::Unloaded;
}

AssetState AssetSystem::getAssetState(AssetHandle handle) const {
    std::lock_guard<std::mutex> lock(assetsMutex_);
    auto it = assets_.find(handle.uuid);
    return it != assets_.end() ? it->second.metadata.state : AssetState::Unloaded;
}

AssetMetadata AssetSystem::getAssetMetadata(AssetHandle handle) const {
    std::lock_guard<std::mutex> lock(assetsMutex_);
    auto it = assets_.find(handle.uuid);
    return it != assets_.end() ? it->second.metadata : AssetMetadata{};
}

bool AssetSystem::isLoaded(AssetHandle handle) const {
    return getAssetState(handle) == AssetState::Loaded;
}

void* AssetSystem::getRawAsset(AssetHandle handle) {
    std::lock_guard<std::mutex> lock(assetsMutex_);
    auto it = assets_.find(handle.uuid);
    if (it == assets_.end() || !it->second.data.has_value()) {
        return nullptr;
    }
    return &it->second.data;
}

const void* AssetSystem::getRawAsset(AssetHandle handle) const {
    std::lock_guard<std::mutex> lock(assetsMutex_);
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

    std::lock_guard<std::mutex> lock(assetsMutex_);

    for (auto& [uuid, entry] : assets_) {
        // Skip assets that are not loaded
        if (entry.metadata.state != AssetState::Loaded) {
            continue;
        }

        // Skip assets that are currently being loaded asynchronously
        bool isBeingLoaded = false;
        for (const auto& pending : pendingLoads_) {
            if (pending.handle.uuid == uuid) {
                isBeingLoaded = true;
                break;
            }
        }
        if (isBeingLoaded) {
            continue;
        }

        // Get current file modification time
        std::error_code ec;
        auto currentWriteTime = std::filesystem::last_write_time(entry.metadata.sourcePath, ec);

        // Handle errors gracefully
        if (ec) {
            // File no longer exists or permission error - don't reload
            continue;
        }

        // If we have a stored write time, compare it
        if (entry.lastWriteTime.has_value()) {
            if (currentWriteTime > entry.lastWriteTime.value()) {
                // File has been modified, reload it
                // We need to release the lock before calling reloadAsset
                // Store the handle for reloading after the loop
                AssetHandle handle = entry.metadata.handle;

                // Temporarily release lock to avoid deadlock
                assetsMutex_.unlock();
                reloadAsset(handle);
                assetsMutex_.lock();

                // Update the stored write time
                // Note: entry reference may be invalidated, so look it up again
                auto it = assets_.find(uuid);
                if (it != assets_.end()) {
                    it->second.lastWriteTime = currentWriteTime;
                }
            }
        } else {
            // No stored write time, just store the current one
            entry.lastWriteTime = currentWriteTime;
        }
    }
}

void AssetSystem::reloadAsset(AssetHandle handle) {
    unloadAsset(handle);
    loadAsset(handle);
}

//==========================================================================
// Helper functions for JSON access (MSVC C++23 module compatibility)
//==========================================================================
// These functions provide type-safe access to JSON data stored as std::any,
// allowing the nlohmann/json header to be confined to .cpp files only.

void setDataAssetJson(DataAsset& asset, const std::string& jsonText) {
    if (nlohmann::json::accept(jsonText)) {
        asset.jsonData = nlohmann::json::parse(jsonText);
        asset.isJson = true;
    } else {
        asset.isJson = false;
    }
}

bool hasDataAssetJson(const DataAsset& asset) {
    return asset.isJson && asset.jsonData.has_value();
}

const std::any& getDataAssetJsonAny(const DataAsset& asset) {
    return asset.jsonData;
}

void setBehaviorTreeJson(BehaviorTreeData& data, const std::string& jsonText) {
    if (nlohmann::json::accept(jsonText)) {
        data.treeData = nlohmann::json::parse(jsonText);
        data.isJson = true;
    } else {
        data.isJson = false;
    }
}

bool hasBehaviorTreeJson(const BehaviorTreeData& data) {
    return data.isJson && data.treeData.has_value();
}

const std::any& getBehaviorTreeJsonAny(const BehaviorTreeData& data) {
    return data.treeData;
}

}  // namespace jframe

// bestow-assets/src/AssetSystem.cpp
// Asset system implementation

module;

#include <any>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <functional>
#include <memory>
#include <mutex>
#include <queue>
#include <unordered_set>
#include <nlohmann/json.hpp>
#include <optional>
#include <sstream>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>
#include <efsw/efsw.hpp>
#include <spdlog/spdlog.h>

// Lua support for material parsing
#define SOL_ALL_SAFETIES_ON 1
#include <sol/sol.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#include <shaderc/shaderc.hpp>

module bestow.assets.impl;

import bestow.events;  // For Events namespace
import bestow.types;   // For PathResolver

namespace bestow {

//==========================================================================
// FileWatchListener Implementation (efsw callback handler)
//==========================================================================

void AssetSystem::FileWatchListener::handleFileAction(
    efsw::WatchID watchId,
    const std::string& dir,
    const std::string& filename,
    efsw::Action action,
    std::string oldFilename)
{
    FileChangeEvent::Action changeAction;
    switch (action) {
        case efsw::Actions::Add:
            changeAction = FileChangeEvent::Action::Added;
            break;
        case efsw::Actions::Modified:
            changeAction = FileChangeEvent::Action::Modified;
            break;
        case efsw::Actions::Delete:
            changeAction = FileChangeEvent::Action::Deleted;
            break;
        default:
            return;  // Ignore other actions
    }

    std::filesystem::path fullPath = std::filesystem::path(dir) / filename;

    // Queue the file change event (thread-safe)
    std::lock_guard<std::mutex> lock(owner_->fileChangesMutex_);
    owner_->pendingFileChanges_.push(FileChangeEvent{
        .path = std::move(fullPath),
        .action = changeAction
    });

    spdlog::debug("[AssetSystem] File change detected: {} (action: {})",
                  filename, static_cast<int>(action));
}

UUID AssetSystem::generateUUID() {
    return nextUUID_++;
}

void AssetSystem::update() {
    // Process pending async loads
    for (auto it = pendingLoads_.begin(); it != pendingLoads_.end();) {
        // Check if the load is complete (non-blocking)
        if (it->completed->load(std::memory_order_acquire)) {
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

    // Process file change events from efsw (event-driven hot reload)
    if (hotReloadEnabled_) {
        processFileChanges();
    }
}

AssetHandle AssetSystem::registerAsset(AssetType type, const std::filesystem::path& path) {
    AssetHandle handle{generateUUID(), type};

    // Resolve path prefixes (:library:/, :assets:/) to actual filesystem paths
    std::filesystem::path resolvedPath = PathResolver::resolve(path.string());

    AssetEntry entry;
    entry.metadata.handle = handle;
    entry.metadata.sourcePath = resolvedPath;
    entry.metadata.state = AssetState::Unloaded;

    assets_[handle.uuid] = std::move(entry);

    // Track path -> handle mapping for file watcher lookups
    {
        std::error_code ec;
        auto canonicalPath = std::filesystem::canonical(resolvedPath, ec);
        if (!ec) {
            std::lock_guard<std::mutex> lock(pathMapMutex_);
            pathToHandle_[canonicalPath.string()] = handle;
        } else {
            // If canonical fails (file doesn't exist yet), use absolute path
            std::lock_guard<std::mutex> lock(pathMapMutex_);
            pathToHandle_[std::filesystem::absolute(resolvedPath).string()] = handle;
        }
    }

    // Start watching the directory if hot reload is enabled
    // IMPORTANT: Use canonical path so we watch the REAL directory, not symlinks
    if (hotReloadEnabled_) {
        std::error_code ec;
        auto canonicalPath = std::filesystem::canonical(resolvedPath, ec);
        auto parentDir = ec ? resolvedPath.parent_path() : canonicalPath.parent_path();
        std::string dirStr = parentDir.string();

        std::lock_guard<std::mutex> lock(pathMapMutex_);
        if (watchedDirectories_.find(dirStr) == watchedDirectories_.end()) {
            if (!fileWatcher_) {
                fileWatcher_ = std::make_unique<efsw::FileWatcher>();
                fileWatchListener_ = std::make_unique<FileWatchListener>(this);
            }
            fileWatcher_->addWatch(dirStr, fileWatchListener_.get(), false);
            watchedDirectories_.insert(dirStr);
            spdlog::info("[AssetSystem] Now watching directory: {}", dirStr);
        }
    }

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
                shaderData.glslSource = shaderSource;
                shaderData.entryPoint = "main";

                // Infer shader stage from file extension
                std::string ext = sourcePath.extension().string();
                if (ext == ".vert") {
                    shaderData.stage = ShaderData::Stage::Vertex;
                } else if (ext == ".frag") {
                    shaderData.stage = ShaderData::Stage::Fragment;
                } else if (ext == ".geom") {
                    shaderData.stage = ShaderData::Stage::Geometry;
                } else if (ext == ".comp") {
                    shaderData.stage = ShaderData::Stage::Compute;
                } else if (ext == ".tesc") {
                    shaderData.stage = ShaderData::Stage::TessControl;
                } else if (ext == ".tese") {
                    shaderData.stage = ShaderData::Stage::TessEval;
                }

                // SPIR-V compilation happens on demand via loadShaderCompiled()
                shaderData.compiled = false;

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

            case AssetType::Mesh: {
                // Load mesh file as raw binary for Graphics3D to parse
                // (OBJ, glTF parsing requires tinyobjloader/tinygltf in Graphics3D)
                std::ifstream file(sourcePath, std::ios::binary | std::ios::ate);
                if (!file.is_open()) {
                    throw std::runtime_error("Failed to open mesh file: " + sourcePath.string());
                }

                auto fileSize = file.tellg();
                file.seekg(0, std::ios::beg);

                MeshData meshData;
                // Store raw file content for later parsing by Graphics3D
                std::vector<char> fileContent(static_cast<size_t>(fileSize));
                if (!file.read(fileContent.data(), fileSize)) {
                    throw std::runtime_error("Failed to read mesh file: " + sourcePath.string());
                }

                // MeshData will be populated by Graphics3D when uploaded to GPU
                loadedData = std::move(meshData);
                loadedSize = static_cast<size_t>(fileSize);
                break;
            }

            case AssetType::Model: {
                // Load model using assimp (FBX, glTF, OBJ, etc.)
                ModelData modelData = loadModelFromFile(sourcePath);
                loadedSize = modelData.meshes.size() * sizeof(MeshData);  // Approximate
                loadedData = std::move(modelData);
                break;
            }

            case AssetType::Material: {
                // Load material definition (JSON or Lua)
                std::ifstream file(sourcePath);
                if (!file.is_open()) {
                    throw std::runtime_error("Failed to open material file: " + sourcePath.string());
                }

                std::stringstream buffer;
                buffer << file.rdbuf();
                std::string fileContents = buffer.str();

                MaterialData matData;
                // MaterialData will be populated based on file type
                loadedData = std::move(matData);
                loadedSize = fileContents.size();
                break;
            }

            case AssetType::Cubemap: {
                // Load single-file cubemap (e.g., HDR, or folder path)
                // Multi-face cubemaps are loaded via loadCubemap(6 paths) overload
                std::ifstream file(sourcePath, std::ios::binary | std::ios::ate);
                if (!file.is_open()) {
                    throw std::runtime_error("Failed to open cubemap file: " + sourcePath.string());
                }

                auto fileSize = file.tellg();
                file.seekg(0, std::ios::beg);

                // For now, just store empty CubemapData - real loading happens
                // via the 6-face loadCubemap overload or Graphics3D integration
                CubemapData cubemapData;
                loadedData = std::move(cubemapData);
                loadedSize = static_cast<size_t>(fileSize);
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

    // Add pending load entry
    pendingLoads_.emplace_back(handle, std::move(callback));

    // Get shared_ptr to the completed flag for the lambda to update
    auto completedFlag = pendingLoads_.back().completed;

    // Load asynchronously using std::async
    // TODO: Consider integrating with a proper JobSystem for better thread pooling
    std::thread([this, handle, completedFlag]() {
        loadAssetImpl(handle);
        completedFlag->store(true, std::memory_order_release);
    }).detach();
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
    if (enable && !hotReloadEnabled_) {
        // Start file watching
        if (!fileWatcher_) {
            fileWatcher_ = std::make_unique<efsw::FileWatcher>();
            fileWatchListener_ = std::make_unique<FileWatchListener>(this);
        }

        // Add watches for all directories containing registered assets
        // IMPORTANT: Use canonical paths so we watch the REAL directory, not symlinks
        {
            std::lock_guard<std::mutex> assetsLock(assetsMutex_);
            std::lock_guard<std::mutex> pathLock(pathMapMutex_);

            for (const auto& [uuid, entry] : assets_) {
                std::error_code ec;
                auto canonicalPath = std::filesystem::canonical(entry.metadata.sourcePath, ec);
                auto parentDir = ec ? entry.metadata.sourcePath.parent_path()
                                    : canonicalPath.parent_path();
                std::string dirStr = parentDir.string();

                if (watchedDirectories_.find(dirStr) == watchedDirectories_.end()) {
                    fileWatcher_->addWatch(dirStr, fileWatchListener_.get(), false);
                    watchedDirectories_.insert(dirStr);
                    spdlog::info("[AssetSystem] Now watching directory: {}", dirStr);
                }
            }
        }

        // Start the file watcher background thread
        fileWatcher_->watch();
        spdlog::info("[AssetSystem] Hot reload enabled with efsw file watcher");
    } else if (!enable && hotReloadEnabled_) {
        // Stop file watching
        fileWatcher_.reset();
        fileWatchListener_.reset();
        watchedDirectories_.clear();
        spdlog::info("[AssetSystem] Hot reload disabled");
    }

    hotReloadEnabled_ = enable;
}

void AssetSystem::checkForReloads() {
    // NOTE: This method is now DEPRECATED for external callers.
    // Hot reload is now handled automatically via efsw file watcher events.
    // Kept for backwards compatibility - it now just processes queued events.
    if (!hotReloadEnabled_) return;

    processFileChanges();
}

void AssetSystem::processFileChanges() {
    // Dequeue all pending file changes (thread-safe)
    std::queue<FileChangeEvent> toProcess;
    {
        std::lock_guard<std::mutex> lock(fileChangesMutex_);
        std::swap(toProcess, pendingFileChanges_);
    }

    // Process each file change event
    while (!toProcess.empty()) {
        handleFileChange(toProcess.front());
        toProcess.pop();
    }
}

void AssetSystem::handleFileChange(const FileChangeEvent& event) {
    // Only handle modified files (not added/deleted for now)
    if (event.action != FileChangeEvent::Action::Modified) {
        return;
    }

    spdlog::info("[AssetSystem] Processing file change: {}", event.path.string());

    // Try to find the asset handle for this file path
    AssetHandle handle{};
    {
        std::error_code ec;
        auto canonicalPath = std::filesystem::canonical(event.path, ec);
        std::string pathKey = ec ? std::filesystem::absolute(event.path).string()
                                 : canonicalPath.string();

        spdlog::info("[AssetSystem] Looking up path key: {}", pathKey);

        std::lock_guard<std::mutex> lock(pathMapMutex_);
        auto it = pathToHandle_.find(pathKey);
        if (it == pathToHandle_.end()) {
            // File changed but not tracked - log for debugging
            spdlog::warn("[AssetSystem] File not tracked. Known paths ({}):", pathToHandle_.size());
            for (const auto& [path, h] : pathToHandle_) {
                spdlog::warn("  - {}", path);
            }
            return;
        }
        handle = it->second;
    }

    // Check if asset is loaded and not currently being reloaded
    {
        std::lock_guard<std::mutex> lock(assetsMutex_);
        auto it = assets_.find(handle.uuid);
        if (it == assets_.end() || it->second.metadata.state != AssetState::Loaded) {
            return;
        }

        // Check if already pending reload
        for (const auto& pending : pendingLoads_) {
            if (pending.handle.uuid == handle.uuid) {
                return;  // Already being reloaded
            }
        }
    }

    spdlog::info("[AssetSystem] File changed, reloading: {}", event.path.string());

    // For shaders, reload and recompile if it was previously compiled
    if (handle.type == AssetType::Shader) {
        // Check if shader was previously compiled (had SPIR-V)
        bool wasCompiled = false;
        {
            std::lock_guard<std::mutex> lock(assetsMutex_);
            auto it = assets_.find(handle.uuid);
            if (it != assets_.end()) {
                const ShaderData* oldData = std::any_cast<ShaderData>(&it->second.data);
                wasCompiled = oldData && oldData->compiled;
            }
        }

        // Reload the GLSL source
        unloadAsset(handle);
        loadAsset(handle);

        // Recompile if it was previously compiled
        if (wasCompiled) {
            std::lock_guard<std::mutex> lock(assetsMutex_);
            auto it = assets_.find(handle.uuid);
            if (it != assets_.end()) {
                ShaderData* shaderData = std::any_cast<ShaderData>(&it->second.data);
                if (shaderData && !shaderData->glslSource.empty()) {
                    compileShaderToSpirv(*shaderData);
                    if (shaderData->compiled) {
                        spdlog::info("[AssetSystem] Shader recompiled successfully: {}", shaderData->path);
                    } else {
                        spdlog::error("[AssetSystem] Shader recompilation failed: {}", shaderData->compileError);
                    }
                }
            }
        }

        // Notify subscribers that the shader has been updated
        notifySubscribers(handle, handle.type);
    } else {
        // For other assets, just reload
        reloadAsset(handle);
    }
}

void AssetSystem::reloadAsset(AssetHandle handle) {
    unloadAsset(handle);
    loadAsset(handle);

    // Notify subscribers that this asset changed
    notifySubscribers(handle, handle.type);
}

//==========================================================================
// 3D Asset Loading and Access
//==========================================================================
// NOTE: Full 3D asset loading (mesh parsing, model loading, cubemaps) is
// integrated with the Graphics3D system which handles the GPU upload.
// These methods provide CPU-side data access for assets that have been
// loaded through the normal asset pipeline.

const MeshData* AssetSystem::getMeshData(AssetHandle handle) const {
    std::lock_guard<std::mutex> lock(assetsMutex_);
    auto it = assets_.find(handle.uuid);
    if (it == assets_.end() || !it->second.data.has_value()) {
        return nullptr;
    }
    try {
        return &std::any_cast<const MeshData&>(it->second.data);
    } catch (const std::bad_any_cast&) {
        return nullptr;
    }
}

const ModelData* AssetSystem::getModelData(AssetHandle handle) const {
    std::lock_guard<std::mutex> lock(assetsMutex_);
    auto it = assets_.find(handle.uuid);
    if (it == assets_.end() || !it->second.data.has_value()) {
        return nullptr;
    }
    try {
        return &std::any_cast<const ModelData&>(it->second.data);
    } catch (const std::bad_any_cast&) {
        return nullptr;
    }
}

const MaterialData* AssetSystem::getMaterialData(AssetHandle handle) const {
    std::lock_guard<std::mutex> lock(assetsMutex_);
    auto it = assets_.find(handle.uuid);
    if (it == assets_.end() || !it->second.data.has_value()) {
        return nullptr;
    }
    try {
        return &std::any_cast<const MaterialData&>(it->second.data);
    } catch (const std::bad_any_cast&) {
        return nullptr;
    }
}

const CubemapData* AssetSystem::getCubemapData(AssetHandle handle) const {
    std::lock_guard<std::mutex> lock(assetsMutex_);
    auto it = assets_.find(handle.uuid);
    if (it == assets_.end() || !it->second.data.has_value()) {
        return nullptr;
    }
    try {
        return &std::any_cast<const CubemapData&>(it->second.data);
    } catch (const std::bad_any_cast&) {
        return nullptr;
    }
}

const SoundData* AssetSystem::getSoundData(AssetHandle handle) const {
    std::lock_guard<std::mutex> lock(assetsMutex_);
    auto it = assets_.find(handle.uuid);
    if (it == assets_.end() || !it->second.data.has_value()) {
        return nullptr;
    }
    try {
        return &std::any_cast<const SoundData&>(it->second.data);
    } catch (const std::bad_any_cast&) {
        return nullptr;
    }
}

AssetHandle AssetSystem::loadMesh(const std::filesystem::path& path) {
    AssetHandle handle = registerAsset(AssetType::Mesh, path);
    loadAsset(handle);
    return handle;
}

AssetHandle AssetSystem::loadModel(const std::filesystem::path& path) {
    AssetHandle handle = registerAsset(AssetType::Model, path);
    loadAsset(handle);
    return handle;
}

AssetHandle AssetSystem::loadCubemap(const std::filesystem::path& path) {
    AssetHandle handle = registerAsset(AssetType::Cubemap, path);
    loadAsset(handle);
    return handle;
}

AssetHandle AssetSystem::loadCubemap(
    const std::filesystem::path& right,
    const std::filesystem::path& left,
    const std::filesystem::path& top,
    const std::filesystem::path& bottom,
    const std::filesystem::path& front,
    const std::filesystem::path& back)
{
    // For multi-face cubemaps, we store just the first path and load all faces
    // The actual loading happens in loadAssetImpl for AssetType::Cubemap
    AssetHandle handle = registerAsset(AssetType::Cubemap, right);

    // Store additional paths in metadata or load all faces here
    // For now, this registers the cubemap - actual multi-face loading
    // would require extended metadata storage

    // Load all faces using stb_image
    std::lock_guard<std::mutex> lock(assetsMutex_);
    auto it = assets_.find(handle.uuid);
    if (it == assets_.end()) {
        return handle;
    }

    CubemapData cubemap;
    cubemap.facePixels.resize(6);  // 6 faces
    std::filesystem::path facePaths[6] = {right, left, top, bottom, front, back};
    const char* faceNames[6] = {"right", "left", "top", "bottom", "front", "back"};

    bool success = true;
    for (int i = 0; i < 6 && success; ++i) {
        int width, height, channels;
        std::string pathStr = facePaths[i].string();
        unsigned char* pixels = stbi_load(pathStr.c_str(), &width, &height, &channels, 4);

        if (!pixels) {
            success = false;
            it->second.metadata.state = AssetState::Failed;
            std::string errorMsg = "Failed to load cubemap face: ";
            errorMsg += faceNames[i];
            it->second.metadata.errorMessage = errorMsg;
            break;
        }

        if (i == 0) {
            cubemap.faceWidth = width;
            cubemap.faceHeight = height;
            cubemap.channels = 4;  // Force RGBA
        }

        std::size_t faceSize = width * height * 4;
        cubemap.facePixels[i].resize(faceSize);
        std::memcpy(cubemap.facePixels[i].data(), pixels, faceSize);
        stbi_image_free(pixels);
    }

    if (success) {
        it->second.data = std::move(cubemap);
        it->second.metadata.state = AssetState::Loaded;
    }

    return handle;
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

//==========================================================================
// Asset Change Subscriptions
//==========================================================================

SubscriptionId AssetSystem::subscribe(AssetHandle handle, AssetChangeCallback callback) {
    std::lock_guard<std::mutex> lock(subscriptionsMutex_);

    SubscriptionId id = nextSubscriptionId_++;
    subscriptions_.push_back(Subscription{
        .id = id,
        .handle = handle,
        .type = handle.type,
        .callback = std::move(callback),
        .isTypeSubscription = false
    });

    return id;
}

SubscriptionId AssetSystem::subscribeToType(AssetType type, AssetChangeCallback callback) {
    std::lock_guard<std::mutex> lock(subscriptionsMutex_);

    SubscriptionId id = nextSubscriptionId_++;
    subscriptions_.push_back(Subscription{
        .id = id,
        .handle = AssetHandle{},  // Invalid handle for type subscriptions
        .type = type,
        .callback = std::move(callback),
        .isTypeSubscription = true
    });

    return id;
}

void AssetSystem::unsubscribe(SubscriptionId id) {
    std::lock_guard<std::mutex> lock(subscriptionsMutex_);

    auto it = std::remove_if(subscriptions_.begin(), subscriptions_.end(),
        [id](const Subscription& sub) { return sub.id == id; });
    subscriptions_.erase(it, subscriptions_.end());
}

void AssetSystem::notifySubscribers(AssetHandle handle, AssetType type) {
    // Copy callbacks while holding the lock, then invoke outside
    std::vector<std::pair<AssetHandle, AssetChangeCallback>> toNotify;

    {
        std::lock_guard<std::mutex> lock(subscriptionsMutex_);

        for (const auto& sub : subscriptions_) {
            if (sub.isTypeSubscription) {
                // Type subscription: notify if asset type matches
                if (sub.type == type) {
                    toNotify.emplace_back(handle, sub.callback);
                }
            } else {
                // Specific asset subscription: notify if handle matches
                if (sub.handle.uuid == handle.uuid) {
                    toNotify.emplace_back(handle, sub.callback);
                }
            }
        }
    }

    // Invoke callbacks outside the lock to prevent deadlocks
    for (const auto& [h, callback] : toNotify) {
        if (callback) {
            callback(h, type);
        }
    }

    // Publish to EventSystem if available
    if (pIEventSystem_) {
        pIEventSystem_->publish(Events::AssetReloaded, AssetEventData{
            .handle = handle,
            .type = type,
            .state = AssetState::Loaded,
            .error = ""
        });
    }
}

//==========================================================================
// Shader Loading and Compilation (using shaderc library - in-process)
//==========================================================================

namespace {
    // Convert ShaderData::Stage to shaderc shader kind
    shaderc_shader_kind stageToShadercKind(ShaderData::Stage stage) {
        switch (stage) {
            case ShaderData::Stage::Vertex: return shaderc_glsl_vertex_shader;
            case ShaderData::Stage::Fragment: return shaderc_glsl_fragment_shader;
            case ShaderData::Stage::Geometry: return shaderc_glsl_geometry_shader;
            case ShaderData::Stage::Compute: return shaderc_glsl_compute_shader;
            case ShaderData::Stage::TessControl: return shaderc_glsl_tess_control_shader;
            case ShaderData::Stage::TessEval: return shaderc_glsl_tess_evaluation_shader;
            default: return shaderc_glsl_vertex_shader;
        }
    }

    // Compile GLSL to SPIR-V using shaderc library (in-process, no subprocess)
    // Returns empty vector on failure, populates errorOut with error message
    std::vector<std::uint32_t> compileGlslToSpirv(
        const std::string& glslSource,
        const std::string& sourcePath,
        ShaderData::Stage stage,
        std::string& errorOut)
    {
        shaderc::Compiler compiler;
        shaderc::CompileOptions options;

        // Set optimization level for release builds
#ifdef NDEBUG
        options.SetOptimizationLevel(shaderc_optimization_level_performance);
#else
        options.SetOptimizationLevel(shaderc_optimization_level_zero);
        options.SetGenerateDebugInfo();
#endif

        // Target Vulkan 1.0 SPIR-V
        options.SetTargetEnvironment(shaderc_target_env_vulkan, shaderc_env_version_vulkan_1_0);

        // Compile GLSL to SPIR-V
        shaderc_shader_kind kind = stageToShadercKind(stage);
        shaderc::SpvCompilationResult result = compiler.CompileGlslToSpv(
            glslSource,
            kind,
            sourcePath.c_str(),
            options
        );

        if (result.GetCompilationStatus() != shaderc_compilation_status_success) {
            errorOut = result.GetErrorMessage();
            return {};
        }

        // Copy SPIR-V bytecode to vector
        std::vector<std::uint32_t> spirv(result.cbegin(), result.cend());
        errorOut.clear();
        return spirv;
    }
}  // anonymous namespace

//==========================================================================
// Shader Loading - Public API
//==========================================================================

bool AssetSystem::warnIfSpvFile(const std::filesystem::path& path) const {
    if (path.extension() == ".spv") {
        spdlog::warn(
            "[AssetSystem] Direct SPIR-V loading is not supported: '{}'. "
            "Please provide GLSL source (.vert, .frag, .geom, .comp, .tesc, .tese). "
            "The engine compiles to SPIR-V internally when needed for Vulkan backends.",
            path.string()
        );
        return true;
    }
    return false;
}

std::filesystem::path AssetSystem::getShaderCachePath(const std::filesystem::path& glslPath) const {
    // Cache location: .shader_cache/ directory next to the shader
    auto cacheDir = glslPath.parent_path() / ".shader_cache";
    auto cacheName = glslPath.filename().string() + ".spv";
    return cacheDir / cacheName;
}

bool AssetSystem::tryLoadCachedSpirv(ShaderData& shaderData) {
    if (shaderData.path.empty() || shaderData.glslSource.empty()) {
        return false;
    }

    auto cachePath = getShaderCachePath(shaderData.path);
    auto metaPath = std::filesystem::path(cachePath.string() + ".meta");

    // Check if cache files exist
    if (!std::filesystem::exists(cachePath) || !std::filesystem::exists(metaPath)) {
        return false;
    }

    try {
        // Read metadata to check hash
        std::ifstream metaFile(metaPath);
        if (!metaFile.is_open()) {
            return false;
        }

        nlohmann::json meta;
        metaFile >> meta;
        metaFile.close();

        // Compute current source hash
        std::uint64_t currentHash = std::hash<std::string>{}(shaderData.glslSource);

        // Check if hash matches
        if (!meta.contains("sourceHash") || meta["sourceHash"].get<std::uint64_t>() != currentHash) {
            spdlog::debug("[AssetSystem] Shader cache miss (hash mismatch): {}", shaderData.path);
            return false;
        }

        // Load cached SPIR-V
        std::ifstream spvFile(cachePath, std::ios::binary | std::ios::ate);
        if (!spvFile.is_open()) {
            return false;
        }

        auto size = spvFile.tellg();
        spvFile.seekg(0, std::ios::beg);

        // SPIR-V is stored as uint32_t words
        std::size_t wordCount = static_cast<std::size_t>(size) / sizeof(std::uint32_t);
        shaderData.spirvBytecode.resize(wordCount);
        spvFile.read(reinterpret_cast<char*>(shaderData.spirvBytecode.data()), size);
        spvFile.close();

        shaderData.compiled = true;
        shaderData.sourceHash = currentHash;
        shaderData.compileError.clear();

        spdlog::debug("[AssetSystem] Shader cache hit: {}", shaderData.path);
        return true;

    } catch (const std::exception& e) {
        spdlog::warn("[AssetSystem] Failed to read shader cache: {}", e.what());
        return false;
    }
}

void AssetSystem::cacheCompiledSpirv(const ShaderData& shaderData) {
    if (shaderData.path.empty() || shaderData.spirvBytecode.empty()) {
        return;
    }

    auto cachePath = getShaderCachePath(shaderData.path);
    auto metaPath = std::filesystem::path(cachePath.string() + ".meta");

    try {
        // Create cache directory if it doesn't exist
        auto cacheDir = cachePath.parent_path();
        if (!std::filesystem::exists(cacheDir)) {
            std::filesystem::create_directories(cacheDir);
        }

        // Write SPIR-V bytecode
        std::ofstream spvFile(cachePath, std::ios::binary);
        if (!spvFile.is_open()) {
            spdlog::warn("[AssetSystem] Failed to write shader cache: {}", cachePath.string());
            return;
        }
        spvFile.write(
            reinterpret_cast<const char*>(shaderData.spirvBytecode.data()),
            static_cast<std::streamsize>(shaderData.spirvBytecode.size() * sizeof(std::uint32_t))
        );
        spvFile.close();

        // Write metadata
        nlohmann::json meta;
        meta["sourceHash"] = shaderData.sourceHash;
        meta["sourcePath"] = shaderData.path;
        meta["stage"] = static_cast<int>(shaderData.stage);

        std::ofstream metaFile(metaPath);
        if (metaFile.is_open()) {
            metaFile << meta.dump(2);
            metaFile.close();
        }

        spdlog::debug("[AssetSystem] Cached compiled shader: {}", cachePath.string());

    } catch (const std::exception& e) {
        spdlog::warn("[AssetSystem] Failed to cache shader: {}", e.what());
    }
}

void AssetSystem::compileShaderToSpirv(ShaderData& shaderData) {
    if (shaderData.glslSource.empty()) {
        shaderData.compileError = "No GLSL source to compile";
        shaderData.compiled = false;
        return;
    }

    // Compute source hash for caching
    shaderData.sourceHash = std::hash<std::string>{}(shaderData.glslSource);

    // Try to load from cache first
    if (tryLoadCachedSpirv(shaderData)) {
        return;  // Cache hit
    }

    // Compile using shaderc
    std::string errorOut;
    auto spirv = compileGlslToSpirv(
        shaderData.glslSource,
        shaderData.path,
        shaderData.stage,
        errorOut
    );

    if (spirv.empty()) {
        shaderData.compileError = errorOut;
        shaderData.compiled = false;
        spdlog::error("[AssetSystem] Shader compilation failed: {} - {}", shaderData.path, errorOut);
    } else {
        shaderData.spirvBytecode = std::move(spirv);
        shaderData.compileError.clear();
        shaderData.compiled = true;

        // Cache the result
        cacheCompiledSpirv(shaderData);

        spdlog::debug("[AssetSystem] Shader compiled successfully: {}", shaderData.path);
    }
}

AssetHandle AssetSystem::loadShader(const std::filesystem::path& path) {
    // Warn and reject .spv files
    if (warnIfSpvFile(path)) {
        return AssetHandle::invalid();
    }

    AssetHandle handle = registerAsset(AssetType::Shader, path);
    loadAsset(handle);
    return handle;
}

AssetHandle AssetSystem::loadShaderCompiled(const std::filesystem::path& glslPath) {
    // Warn and reject .spv files
    if (warnIfSpvFile(glslPath)) {
        return AssetHandle::invalid();
    }

    // Register and load the shader (source only first)
    AssetHandle handle = registerAsset(AssetType::Shader, glslPath);
    loadAsset(handle);

    // Now compile to SPIR-V
    {
        std::lock_guard<std::mutex> lock(assetsMutex_);
        auto it = assets_.find(handle.uuid);
        if (it != assets_.end()) {
            ShaderData* shaderData = std::any_cast<ShaderData>(&it->second.data);
            if (shaderData && !shaderData->glslSource.empty()) {
                compileShaderToSpirv(*shaderData);
            }
        }
    }

    return handle;
}

const ShaderData* AssetSystem::getShaderData(AssetHandle handle) const {
    std::lock_guard<std::mutex> lock(assetsMutex_);
    auto it = assets_.find(handle.uuid);
    if (it == assets_.end() || !it->second.data.has_value()) {
        return nullptr;
    }
    try {
        return &std::any_cast<const ShaderData&>(it->second.data);
    } catch (const std::bad_any_cast&) {
        return nullptr;
    }
}

//==========================================================================
// Lua Material Loading Implementation
//==========================================================================

// Helper to get value with default from sol::table (avoids sol2 get_or ambiguity)
template<typename T>
static T getWithDefault(const sol::table& t, const char* key, T defaultVal) {
    sol::optional<T> val = t[key];
    return val.value_or(defaultVal);
}

template<typename T>
static T getWithDefault(const sol::table& t, int key, T defaultVal) {
    sol::optional<T> val = t[key];
    return val.value_or(defaultVal);
}

static BlendMode parseBlendMode(const std::string& mode) {
    if (mode == "alphaBlend" || mode == "alpha") return BlendMode::AlphaBlend;
    if (mode == "additive") return BlendMode::Additive;
    if (mode == "multiply") return BlendMode::Multiply;
    if (mode == "alphaTest") return BlendMode::AlphaTest;
    return BlendMode::Opaque;
}

static CullMode parseCullMode(const std::string& mode) {
    if (mode == "none" || mode == "off") return CullMode::None;
    if (mode == "front") return CullMode::Front;
    return CullMode::Back;
}

bool AssetSystem::parseLuaMaterialFile(const std::filesystem::path& luaPath, LuaMaterialData& outData) {
    sol::state lua;
    lua.open_libraries(sol::lib::base, sol::lib::math, sol::lib::table);

    // Sandbox: remove dangerous functions
    lua["os"] = sol::lua_nil;
    lua["io"] = sol::lua_nil;
    lua["loadfile"] = sol::lua_nil;
    lua["dofile"] = sol::lua_nil;
    lua["load"] = sol::lua_nil;

    try {
        sol::protected_function_result result = lua.safe_script_file(luaPath.string());
        if (!result.valid()) {
            sol::error err = result;
            spdlog::error("[AssetSystem] Failed to parse Lua material '{}': {}",
                          luaPath.string(), err.what());
            return false;
        }

        sol::table mat = result;

        // Parse shader paths
        if (mat["shader"].valid()) {
            sol::table shader = mat["shader"];
            outData.vertexShaderPath = getWithDefault<std::string>(shader, "vertex", "shaders/pbr.vert");
            outData.fragmentShaderPath = getWithDefault<std::string>(shader, "fragment", "shaders/pbr.frag");
        }

        // Parse uniforms (stored as std::any)
        if (mat["uniforms"].valid()) {
            sol::table uniforms = mat["uniforms"];
            for (auto& [key, val] : uniforms) {
                std::string name = key.as<std::string>();
                if (val.is<double>()) {
                    outData.uniforms[name] = std::any(static_cast<float>(val.as<double>()));
                } else if (val.is<bool>()) {
                    outData.uniforms[name] = std::any(val.as<bool>());
                } else if (val.is<int>()) {
                    outData.uniforms[name] = std::any(val.as<int>());
                } else if (val.is<sol::table>()) {
                    sol::table t = val.as<sol::table>();
                    size_t size = t.size();
                    if (size == 2) {
                        outData.uniforms[name] = std::any(Vec2{
                            getWithDefault<float>(t, 1, 0.0f),
                            getWithDefault<float>(t, 2, 0.0f)
                        });
                    } else if (size == 3) {
                        outData.uniforms[name] = std::any(Vec3{
                            getWithDefault<float>(t, 1, 0.0f),
                            getWithDefault<float>(t, 2, 0.0f),
                            getWithDefault<float>(t, 3, 0.0f)
                        });
                    } else if (size == 4) {
                        outData.uniforms[name] = std::any(Vec4{
                            getWithDefault<float>(t, 1, 0.0f),
                            getWithDefault<float>(t, 2, 0.0f),
                            getWithDefault<float>(t, 3, 0.0f),
                            getWithDefault<float>(t, 4, 1.0f)
                        });
                    }
                }
            }
        }

        // Parse textures (sampler name -> texture path)
        if (mat["textures"].valid()) {
            sol::table textures = mat["textures"];
            for (auto& [key, val] : textures) {
                outData.texturePaths[key.as<std::string>()] = val.as<std::string>();
            }
        }

        // Parse render state
        outData.blendMode = parseBlendMode(getWithDefault<std::string>(mat, "blendMode", "opaque"));
        outData.cullMode = parseCullMode(getWithDefault<std::string>(mat, "cullMode", "back"));
        outData.depthWrite = getWithDefault<bool>(mat, "depthWrite", true);
        outData.depthTest = getWithDefault<bool>(mat, "depthTest", true);
        outData.hotReload = getWithDefault<bool>(mat, "hotReload", true);

        // Set metadata
        outData.path = luaPath.string();
        outData.name = luaPath.stem().string();

        return true;

    } catch (const std::exception& e) {
        spdlog::error("[AssetSystem] Exception parsing Lua material '{}': {}",
                      luaPath.string(), e.what());
        return false;
    }
}

AssetHandle AssetSystem::loadMaterial(const std::filesystem::path& luaPath) {
    // Register as a material-type asset (using Data asset type internally)
    AssetHandle handle = registerAsset(AssetType::Data, luaPath);

    // Parse the Lua file into LuaMaterialData
    LuaMaterialData matData;
    if (!parseLuaMaterialFile(luaPath, matData)) {
        spdlog::error("[AssetSystem] Failed to load material: {}", luaPath.string());
        return AssetHandle::invalid();
    }

    // Store the parsed material data
    {
        std::lock_guard<std::mutex> lock(assetsMutex_);
        auto it = assets_.find(handle.uuid);
        if (it != assets_.end()) {
            it->second.data = std::move(matData);
            it->second.metadata.state = AssetState::Loaded;
        }
    }

    return handle;
}

const LuaMaterialData* AssetSystem::getLuaMaterialData(AssetHandle handle) const {
    std::lock_guard<std::mutex> lock(assetsMutex_);
    auto it = assets_.find(handle.uuid);
    if (it == assets_.end() || !it->second.data.has_value()) {
        return nullptr;
    }
    try {
        return &std::any_cast<const LuaMaterialData&>(it->second.data);
    } catch (const std::bad_any_cast&) {
        return nullptr;
    }
}

}  // namespace bestow

// bestow-assets/src/AssetSystem.cpp
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

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#include <shaderc/shaderc.hpp>

module bestow.assets.impl;

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

    // Process file change events from efsw (event-driven hot reload)
    if (hotReloadEnabled_) {
        processFileChanges();
    }
}

AssetHandle AssetSystem::registerAsset(AssetType type, const std::filesystem::path& path) {
    AssetHandle handle{generateUUID(), type};

    AssetEntry entry;
    entry.metadata.handle = handle;
    entry.metadata.sourcePath = path;
    entry.metadata.state = AssetState::Unloaded;

    assets_[handle.uuid] = std::move(entry);

    // Track path -> handle mapping for file watcher lookups
    {
        std::error_code ec;
        auto canonicalPath = std::filesystem::canonical(path, ec);
        if (!ec) {
            std::lock_guard<std::mutex> lock(pathMapMutex_);
            pathToHandle_[canonicalPath.string()] = handle;
        } else {
            // If canonical fails (file doesn't exist yet), use absolute path
            std::lock_guard<std::mutex> lock(pathMapMutex_);
            pathToHandle_[std::filesystem::absolute(path).string()] = handle;
        }
    }

    // Start watching the directory if hot reload is enabled
    // IMPORTANT: Use canonical path so we watch the REAL directory, not symlinks
    if (hotReloadEnabled_) {
        std::error_code ec;
        auto canonicalPath = std::filesystem::canonical(path, ec);
        auto parentDir = ec ? path.parent_path() : canonicalPath.parent_path();
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

                // SPIR-V compilation happens on demand via compileShaderAsync
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
                // Load model file as raw binary for Graphics3D to parse
                // (glTF, FBX parsing requires external libraries in Graphics3D)
                std::ifstream file(sourcePath, std::ios::binary | std::ios::ate);
                if (!file.is_open()) {
                    throw std::runtime_error("Failed to open model file: " + sourcePath.string());
                }

                auto fileSize = file.tellg();
                file.seekg(0, std::ios::beg);

                ModelData modelData;
                // ModelData will be populated by Graphics3D when loaded
                loadedData = std::move(modelData);
                loadedSize = static_cast<size_t>(fileSize);
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

    // For shaders, reload and recompile automatically
    if (handle.type == AssetType::Shader) {
        // Reload the GLSL source
        unloadAsset(handle);
        loadAsset(handle);

        // Automatically compile shader to SPIR-V
        compileShaderAsync(handle, [this, handle](AssetHandle h, AssetState state) {
            if (state == AssetState::Loaded) {
                // Get compilation result
                const ShaderData* shaderData = getShaderData(h);
                if (shaderData && shaderData->compiled) {
                    spdlog::info("[AssetSystem] Shader compiled successfully: {}", shaderData->path);
                    // Notify subscribers that the shader has been updated
                    notifySubscribers(handle, handle.type);
                } else if (shaderData && !shaderData->compileError.empty()) {
                    spdlog::error("[AssetSystem] Shader compilation failed: {}", shaderData->compileError);
                }
            }
        });
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

AssetHandle AssetSystem::loadShader(const std::filesystem::path& path) {
    AssetHandle handle = registerAsset(AssetType::Shader, path);
    loadAsset(handle);
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

void AssetSystem::compileShaderAsync(AssetHandle handle, AssetLoadCallback callback) {
    // Verify asset exists and is a shader
    {
        std::lock_guard<std::mutex> lock(assetsMutex_);
        auto it = assets_.find(handle.uuid);
        if (it == assets_.end() || handle.type != AssetType::Shader) {
            if (callback) {
                callback(handle, AssetState::Failed);
            }
            return;
        }
    }

    // Launch async compilation task
    std::future<void> future = std::async(std::launch::async, [this, handle]() {
        // Extract shader data while holding lock
        std::string glslSource;
        std::string sourcePath;
        ShaderData::Stage stage;

        {
            std::lock_guard<std::mutex> lock(assetsMutex_);
            auto it = assets_.find(handle.uuid);
            if (it == assets_.end()) return;

            ShaderData* shaderData = std::any_cast<ShaderData>(&it->second.data);
            if (!shaderData || shaderData->glslSource.empty()) {
                return;
            }

            glslSource = shaderData->glslSource;
            sourcePath = shaderData->path;
            stage = shaderData->stage;
        }

        // Compile outside the lock (this is the slow part)
        std::string errorMessage;
        std::vector<std::uint32_t> spirv = compileGlslToSpirv(glslSource, sourcePath, stage, errorMessage);

        // Update shader data with result
        {
            std::lock_guard<std::mutex> lock(assetsMutex_);
            auto it = assets_.find(handle.uuid);
            if (it == assets_.end()) return;

            ShaderData* shaderData = std::any_cast<ShaderData>(&it->second.data);
            if (shaderData) {
                if (spirv.empty()) {
                    shaderData->compileError = errorMessage;
                    shaderData->compiled = false;
                } else {
                    shaderData->spirvBytecode = std::move(spirv);
                    shaderData->compileError.clear();
                    shaderData->compiled = true;
                }
            }
        }
    });

    // Store pending load for callback processing in update()
    pendingLoads_.push_back(PendingLoad{
        .handle = handle,
        .callback = std::move(callback),
        .future = std::move(future)
    });
}

bool AssetSystem::isShaderCompilationSupported() const {
    // shaderc is linked at compile time, always available
    return true;
}

}  // namespace bestow

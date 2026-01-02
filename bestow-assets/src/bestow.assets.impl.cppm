// bestow-assets/src/bestow.assets.impl.cppm
// Asset system implementation

module;

// Note: nlohmann/json is NOT included here due to MSVC C++23 module compatibility issues.
// JSON data is stored as std::any and accessed via getJson()/setJson() helper functions.
// The actual nlohmann/json include is in AssetSystem.cpp.

#include <cstddef>  // For size_t
#include <kangaru/kangaru.hpp>
#include <bestow/kangaru_macros.hpp>
#include <efsw/efsw.hpp>

export module bestow.assets.impl;

import std;
import bestow.assets;
import bestow.types;
import bestow.services;

export namespace bestow {

// Note: TextureData and DataAsset are now in bestow.assets contract module

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

// Note: ShaderData is now defined in bestow.assets contract module
// It includes GLSL source, compiled SPIR-V bytecode, and compilation status

// Note: NavMeshData is now defined in bestow.assets contract module

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
    explicit AssetSystem(IEventSystem* pIEventSystem = nullptr)
        : pIEventSystem_(pIEventSystem)
        , assetLibrary_(AssetLibrary::create()) {}
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

    // Asset change subscriptions
    SubscriptionId subscribe(AssetHandle handle, AssetChangeCallback callback) override;
    SubscriptionId subscribeToType(AssetType type, AssetChangeCallback callback) override;
    void unsubscribe(SubscriptionId id) override;

    // Shader loading
    AssetHandle loadShader(const std::filesystem::path& glslPath) override;
    AssetHandle loadShaderCompiled(const std::filesystem::path& glslPath) override;
    const ShaderData* getShaderData(AssetHandle handle) const override;

    // 3D Asset loading and access
    const MeshData* getMeshData(AssetHandle handle) const override;
    const ModelData* getModelData(AssetHandle handle) const override;
    const MaterialData* getMaterialData(AssetHandle handle) const override;
    const CubemapData* getCubemapData(AssetHandle handle) const override;
    const SoundData* getSoundData(AssetHandle handle) const override;
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

    // Lua material loading
    AssetHandle loadMaterial(const std::filesystem::path& luaPath) override;
    const LuaMaterialData* getLuaMaterialData(AssetHandle handle) const override;

    // Library discovery
    std::optional<AssetLibrary> getAssetLibrary() const override;
    std::vector<LibraryAssetInfo> listLibraryAssets(std::string_view relativeDir = "") const override;
    std::vector<LibraryAssetInfo> listLibraryAssetsRecursive(std::string_view relativeDir = "") const override;
    std::vector<std::string> listLibraryCategories() const override;

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
        std::shared_ptr<std::atomic<bool>> completed;  // shared_ptr is copyable, atomic<bool> is not

        PendingLoad(AssetHandle h, AssetLoadCallback cb)
            : handle(h), callback(std::move(cb)), completed(std::make_shared<std::atomic<bool>>(false)) {}
    };

    // File change event queued by the file watcher
    struct FileChangeEvent {
        std::filesystem::path path;
        enum class Action { Added, Modified, Deleted } action;
    };

    // File watcher listener - forwards events to AssetSystem
    class FileWatchListener : public efsw::FileWatchListener {
    public:
        explicit FileWatchListener(AssetSystem* owner) : owner_(owner) {}

        void handleFileAction(efsw::WatchID watchId,
                              const std::string& dir,
                              const std::string& filename,
                              efsw::Action action,
                              std::string oldFilename) override;
    private:
        AssetSystem* owner_;
    };

    UUID generateUUID();
    void loadAssetImpl(AssetHandle handle);  // Thread-safe loading implementation
    void processFileChanges();               // Process queued file change events
    void handleFileChange(const FileChangeEvent& event);  // Handle a single file change
    ModelData loadModelFromFile(const std::filesystem::path& path);  // Load model with assimp

    // Shader compilation helpers (internal use only)
    bool warnIfSpvFile(const std::filesystem::path& path) const;  // Returns true if .spv, logs warning
    std::filesystem::path getShaderCachePath(const std::filesystem::path& glslPath) const;
    bool tryLoadCachedSpirv(ShaderData& shaderData);  // Returns true if cache hit
    void cacheCompiledSpirv(const ShaderData& shaderData);
    void compileShaderToSpirv(ShaderData& shaderData);  // Synchronous compilation

    // Lua material parsing helpers (internal use only)
    bool parseLuaMaterialFile(const std::filesystem::path& luaPath, LuaMaterialData& outData);

    std::unordered_map<UUID, AssetEntry> assets_;
    std::vector<PendingLoad> pendingLoads_;
    mutable std::mutex assetsMutex_;  // Protects assets_ during async loads
    bool hotReloadEnabled_ = false;
    UUID nextUUID_ = 1;

    // File watcher infrastructure (event-driven, not polling)
    std::unique_ptr<efsw::FileWatcher> fileWatcher_;
    std::unique_ptr<FileWatchListener> fileWatchListener_;
    std::queue<FileChangeEvent> pendingFileChanges_;
    mutable std::mutex fileChangesMutex_;
    std::unordered_map<std::string, AssetHandle> pathToHandle_;  // Canonical path -> handle
    mutable std::mutex pathMapMutex_;
    std::unordered_set<std::string> watchedDirectories_;

    // Subscription storage
    struct Subscription {
        SubscriptionId id;
        AssetHandle handle;         // Specific asset (invalid for type subscriptions)
        AssetType type;             // Asset type (for type subscriptions)
        AssetChangeCallback callback;
        bool isTypeSubscription = false;
    };
    std::vector<Subscription> subscriptions_;
    mutable std::mutex subscriptionsMutex_;
    SubscriptionId nextSubscriptionId_ = 1;

    // Helper to notify subscribers when an asset changes
    void notifySubscribers(AssetHandle handle, AssetType type);

    // Asset library for path resolution and discovery
    std::optional<AssetLibrary> assetLibrary_;

    // Injected dependencies
    IEventSystem* pIEventSystem_ = nullptr;

public:
    // Forward declaration - defined after class is complete
    struct Service;
};

// Service type for Engine::use<IAssetSystem, AssetSystem>()
struct AssetSystem::Service : kgr::single_service<AssetSystem>, kgr::overrides<IAssetSystemService> {
    static auto construct(kgr::inject_t<IEventSystemService> d1)
        -> kgr::inject_result<IEventSystem*> {
        return kgr::inject(&d1.forward());
    }
};

// Backwards compatibility alias
using AssetSystemService = AssetSystem::Service;

}  // namespace bestow

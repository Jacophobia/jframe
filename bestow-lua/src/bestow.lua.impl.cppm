// bestow-lua/src/bestow.lua.impl.cppm
// Lua Runtime implementation module
//
// This is the unified Lua runtime that replaces both ConfigSystem and BlueprintFactory.
// It provides:
// 1. Sandboxed Lua execution with sol2
// 2. Hot reload of all Lua files via AssetSystem
// 3. Bindings to all engine systems
// 4. Behavior system with state preservation
// 5. Blueprint-based entity creation
// 6. Lua-defined game systems

module;

#include <kangaru/kangaru.hpp>
#include <sol/sol.hpp>
#include <spdlog/spdlog.h>

export module bestow.lua.impl;

import std;
import bestow.services;

export namespace bestow {

//==============================================================================
// Forward Declarations
//==============================================================================

// Behavior instance data
struct BehaviorInstance {
    BehaviorHandle handle;
    Entity entity;
    sol::table state;          // Preserved across hot reload
    sol::table callbacks;      // Current callbacks (update, fixedUpdate, etc.)
    bool enabled = true;
    std::string behaviorName;
};

// Loaded behavior definition
struct LoadedBehavior {
    BehaviorHandle handle;
    std::string name;
    std::string path;
    AssetHandle asset;
    sol::table definition;     // The behavior table from Lua
};

// Loaded Lua system definition
struct LoadedLuaSystem {
    LuaSystemHandle handle;
    std::string name;
    std::string path;
    AssetHandle asset;
    sol::table definition;
    int priority = 0;
    bool enabled = true;
};

// Config value storage
struct ConfigValue {
    std::variant<float, int, bool, std::string> value;
};

//==============================================================================
// LuaRuntime Implementation
//==============================================================================

BESTOW_SYSTEM(LuaRuntime, ILuaRuntime, IAssetSystem, IEventSystem, IEntitySystem) {
public:
    ~LuaRuntime() override;

    //==========================================================================
    // Lifecycle
    //==========================================================================

    bool initialize() override;
    void update(float dt) override;
    void fixedUpdate(float fixedDt) override;
    void shutdown() override;

    //==========================================================================
    // App Entry Point
    //==========================================================================

    LuaResult<void> loadApp() override;
    LuaResult<void> loadApp(std::string_view path) override;
    sol::table getAppConfig() override;

    //==========================================================================
    // Blueprint Management
    //==========================================================================

    LuaResult<void> loadBlueprints(std::string_view path) override;
    LuaResult<void> loadAllBlueprints() override;
    bool hasBlueprint(std::string_view name) const override;
    std::optional<LuaBlueprintDef> getBlueprint(std::string_view name) const override;
    std::vector<std::string> getBlueprintNames() const override;
    void clearBlueprints() override;

    //==========================================================================
    // Entity Creation
    //==========================================================================

    LuaResult<Entity> spawn(std::string_view blueprintName, float x, float y) override;
    LuaResult<Entity> spawn(std::string_view blueprintName, float x, float y,
                            float width, float height) override;
    LuaResult<Entity> spawn(std::string_view blueprintName, float x, float y,
                            const PropertyMap& overrides) override;
    LuaResult<Entity> spawn(std::string_view blueprintName, float x, float y,
                            float width, float height, const PropertyMap& overrides) override;

    //==========================================================================
    // Component Registration
    //==========================================================================

    void registerComponent(std::string_view name, ComponentCreator creator) override;
    bool isComponentRegistered(std::string_view name) const override;
    std::vector<std::string> getRegisteredComponents() const override;

    //==========================================================================
    // Behavior Management
    //==========================================================================

    LuaResult<BehaviorHandle> loadBehavior(std::string_view path) override;
    LuaResult<void> loadAllBehaviors() override;
    LuaResult<void> attachBehavior(Entity entity, BehaviorHandle behavior) override;
    LuaResult<void> attachBehavior(Entity entity, std::string_view behaviorName) override;
    void detachBehavior(Entity entity, BehaviorHandle behavior) override;
    void detachAllBehaviors(Entity entity) override;
    std::vector<BehaviorHandle> getBehaviors(Entity entity) const override;
    void setBehaviorEnabled(Entity entity, BehaviorHandle behavior, bool enabled) override;
    sol::table getBehaviorState(Entity entity, BehaviorHandle behavior) override;

    //==========================================================================
    // Lua System Management
    //==========================================================================

    LuaResult<LuaSystemHandle> loadSystem(std::string_view path) override;
    LuaResult<void> loadAllSystems() override;
    void setSystemEnabled(LuaSystemHandle system, bool enabled) override;
    std::vector<LuaSystemHandle> getLuaSystems() const override;
    std::optional<LuaSystemDef> getSystemInfo(LuaSystemHandle system) const override;

    //==========================================================================
    // Hot Reload
    //==========================================================================

    void enableHotReload(bool enable) override;
    bool isHotReloadEnabled() const override;
    LuaResult<void> reloadFile(std::string_view path) override;
    LuaResult<void> reloadAll() override;

    //==========================================================================
    // Direct Lua Access
    //==========================================================================

    LuaResult<sol::object> execute(std::string_view code, std::string_view description) override;
    LuaResult<void> run(std::string_view code, std::string_view description) override;
    void setGlobal(std::string_view name, sol::object value) override;
    sol::object getGlobal(std::string_view name) override;
    sol::state& getLuaState() override;

    //==========================================================================
    // Configuration Values
    //==========================================================================

    std::optional<float> getFloat(std::string_view key) const override;
    std::optional<int> getInt(std::string_view key) const override;
    std::optional<bool> getBool(std::string_view key) const override;
    std::optional<std::string> getString(std::string_view key) const override;
    float getFloatOr(std::string_view key, float defaultVal) const override;
    int getIntOr(std::string_view key, int defaultVal) const override;
    bool getBoolOr(std::string_view key, bool defaultVal) const override;
    std::string getStringOr(std::string_view key, std::string_view defaultVal) const override;
    std::vector<PropertyValue> getTable(std::string_view key) const override;
    void setFloat(std::string_view key, float value) override;
    void setInt(std::string_view key, int value) override;
    void setBool(std::string_view key, bool value) override;
    void setString(std::string_view key, std::string_view value) override;
    bool hasKey(std::string_view key) const override;
    LuaResult<void> loadConfig(std::string_view path) override;

    //==========================================================================
    // Engine System Bindings
    //==========================================================================

    bool isSystemAvailable(std::string_view systemName) const override;
    std::vector<std::string> getAvailableSystems() const override;

    //==========================================================================
    // Internal Methods (called from other translation units)
    //==========================================================================

    // Called from LuaBindings.cpp
    void setupBindings();

    // Called from LuaBlueprints.cpp
    LuaResult<LuaBlueprintDef> parseBlueprintTable(const sol::table& table,
                                                    std::string_view name);
    void applyComponentsToEntity(Entity entity, const LuaBlueprintDef& blueprint,
                                  const PropertyMap& overrides);

    // Called from LuaBehaviors.cpp
    void callBehaviorCallback(BehaviorInstance& instance, std::string_view callback,
                               float dt = 0.0f);
    void reloadBehavior(LoadedBehavior& behavior);
    sol::table createEntityWrapper(Entity entity);

    // Called from LuaBlueprints.cpp
    PropertyValue parsePropertyValue(const sol::object& obj);
    LuaBlueprintDef resolveInheritance(const LuaBlueprintDef& blueprint);

protected:
    LuaResult<sol::object> callImpl(std::string_view funcName, sol::variadic_args args) override;

private:
    //==========================================================================
    // Internal Helpers
    //==========================================================================

    void setupSandbox();
    void registerBuiltinComponents();
    LuaResult<void> loadLuaFile(std::string_view path, std::string_view description);
    void onAssetReloaded(AssetHandle handle, AssetType type);

    //==========================================================================
    // State
    //==========================================================================

    sol::state lua_;
    bool initialized_ = false;
    bool hotReloadEnabled_ = false;

    // App state
    sol::table appConfig_;
    std::string appPath_;

    // Blueprints
    std::unordered_map<std::string, LuaBlueprintDef> blueprints_;
    std::vector<AssetHandle> blueprintAssets_;

    // Component creators
    std::unordered_map<std::string, ComponentCreator> componentCreators_;

    // Behaviors
    std::uint64_t nextBehaviorId_ = 1;
    std::unordered_map<std::uint64_t, LoadedBehavior> loadedBehaviors_;
    std::unordered_map<std::string, BehaviorHandle> behaviorsByName_;
    std::vector<BehaviorInstance> behaviorInstances_;

    // Lua systems
    std::uint64_t nextSystemId_ = 1;
    std::unordered_map<std::uint64_t, LoadedLuaSystem> loadedSystems_;
    std::vector<LuaSystemHandle> systemUpdateOrder_;  // Sorted by priority

    // Config values
    std::unordered_map<std::string, ConfigValue> configValues_;
    std::vector<AssetHandle> configAssets_;

    // Hot reload subscriptions
    std::vector<SubscriptionId> assetSubscriptions_;

    // Available engine systems (set during initialization based on what's registered)
    std::unordered_set<std::string> availableSystems_;
};

}  // namespace bestow

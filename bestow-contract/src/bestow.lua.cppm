// bestow-contract/src/bestow.lua.cppm
// Unified Lua Runtime interface - THE entry point for all Lua execution
//
// This system replaces both IConfigSystem and IBlueprintFactory with a single
// unified Lua runtime that:
// 1. Provides sandboxed Lua execution with sol2
// 2. Handles hot reload of all Lua files via AssetSystem
// 3. Binds all engine systems to Lua
// 4. Manages behaviors with state preservation across hot reloads
// 5. Provides entity creation from Lua blueprints
// 6. Manages Lua-defined game systems

module;

#include <sol/sol.hpp>

export module bestow.lua;

import std;
import bestow.types;
import bestow.entity;

export namespace bestow {

//==============================================================================
// Path Schemes for Lua files
//==============================================================================

namespace LuaPath {
    // App entry point search paths (in order)
    inline constexpr std::array<std::string_view, 4> AppSearchPaths = {
        "./app.lua",
        "./main.lua",
        "./data/app.lua",
        "./game/app.lua"
    };

    // Content directories (relative to assets path)
    inline constexpr std::string_view Blueprints = "blueprints/";
    inline constexpr std::string_view Behaviors = "behaviors/";
    inline constexpr std::string_view Systems = "systems/";
    inline constexpr std::string_view Levels = "levels/";
    inline constexpr std::string_view Materials = "materials/";
    inline constexpr std::string_view Config = "config/";
    inline constexpr std::string_view UI = "ui/";
}

//==============================================================================
// Lua Error Types
//==============================================================================

enum class LuaErrorCode : std::uint8_t {
    None = 0,
    ParseError,         // Lua syntax error
    RuntimeError,       // Lua runtime error
    FileNotFound,       // Lua file not found
    SandboxViolation,   // Attempt to use blocked function
    InvalidBlueprint,   // Blueprint definition invalid
    InvalidBehavior,    // Behavior definition invalid
    InvalidSystem,      // System definition invalid
    ComponentNotFound,  // Component type not registered
    EntityNotFound,     // Entity doesn't exist
    SystemNotAvailable, // Engine system not registered
};

struct LuaError {
    LuaErrorCode code = LuaErrorCode::None;
    std::string message;
    std::string source;  // File or script name
    int line = 0;        // Line number if available

    explicit operator bool() const { return code != LuaErrorCode::None; }
};

template<typename T>
using LuaResult = std::expected<T, LuaError>;

//==============================================================================
// Blueprint Types (Lua-defined entity templates)
//==============================================================================

/// Property value types supported in Lua blueprints
using PropertyValue = std::variant<
    std::monostate,                     // nil
    bool,
    std::int64_t,
    double,
    std::string,
    std::vector<double>,                // Vec2, Vec3, Vec4, Color
    std::vector<std::int64_t>,          // Int arrays
    std::vector<std::string>            // String arrays
>;

/// Property map for component/entity data
using PropertyMap = std::unordered_map<std::string, PropertyValue>;

/// Component definition in a blueprint
struct LuaComponentDef {
    std::string type;              // Component type name (e.g., "Transform", "Sprite")
    PropertyMap properties;        // Component properties
};

/// Physics body definition in a blueprint
struct LuaPhysicsDef {
    std::string bodyType = "dynamic";  // "static", "dynamic", "kinematic"
    std::optional<std::array<float, 2>> size;  // Override size
    bool sensor = false;
    bool fixedRotation = true;
    float density = 1.0f;
    float friction = 0.3f;
    float restitution = 0.0f;
    float linearDamping = 0.0f;
    float angularDamping = 0.0f;
    std::string collisionLayer;
};

/// Complete blueprint definition
struct LuaBlueprintDef {
    std::string name;                              // Blueprint identifier
    std::string inherits;                          // Parent blueprint (optional)
    std::vector<LuaComponentDef> components;       // Components to add
    std::optional<LuaPhysicsDef> physics;          // Physics configuration
    std::string behavior;                          // Default behavior script (optional)
    PropertyMap metadata;                          // Additional metadata
};

//==============================================================================
// Behavior Types (Lua-defined entity scripts)
//==============================================================================

/// Handle to a loaded behavior definition
struct BehaviorHandle {
    std::uint64_t id = 0;
    bool isValid() const { return id != 0; }
    auto operator<=>(const BehaviorHandle&) const = default;
};

/// State of a behavior instance attached to an entity
struct BehaviorState {
    BehaviorHandle behavior;
    Entity entity;
    sol::table state;          // Lua state table (preserved across hot reload)
    bool enabled = true;
};

//==============================================================================
// System Types (Lua-defined game systems)
//==============================================================================

/// Handle to a loaded Lua system
struct LuaSystemHandle {
    std::uint64_t id = 0;
    bool isValid() const { return id != 0; }
    auto operator<=>(const LuaSystemHandle&) const = default;
};

/// Lua system definition
struct LuaSystemDef {
    std::string name;
    int priority = 0;              // Update order (lower = earlier)
    std::vector<std::string> requires;  // Required engine systems
};

//==============================================================================
// Component Creator Callback
//==============================================================================

/// Callback for creating C++ components from Lua property maps
using ComponentCreator = std::function<void(Entity, IEntitySystem&, const PropertyMap&)>;

//==============================================================================
// Lua Runtime Interface
//==============================================================================

class ILuaRuntime {
public:
    virtual ~ILuaRuntime() = default;

    //==========================================================================
    // Lifecycle
    //==========================================================================

    /// Initialize the Lua runtime
    virtual bool initialize() = 0;

    /// Update all active behaviors and Lua systems
    /// @param dt Delta time in seconds
    virtual void update(float dt) = 0;

    /// Fixed update for physics-related behaviors
    /// @param fixedDt Fixed timestep in seconds
    virtual void fixedUpdate(float fixedDt) = 0;

    /// Shutdown and release all Lua resources
    virtual void shutdown() = 0;

    //==========================================================================
    // App Entry Point
    //==========================================================================

    /// Load and run the app entry point (app.lua or main.lua)
    /// Searches standard locations in order
    /// @return Error if no entry point found or execution failed
    virtual LuaResult<void> loadApp() = 0;

    /// Load and run a specific app entry point
    /// @param path Path to the app Lua file
    /// @return Error if file not found or execution failed
    virtual LuaResult<void> loadApp(std::string_view path) = 0;

    /// Get the loaded app's configuration table
    /// @return The app config table, or nil if no app loaded
    virtual sol::table getAppConfig() = 0;

    //==========================================================================
    // Blueprint Management
    //==========================================================================

    /// Load blueprint definitions from a Lua file
    /// @param path Path to blueprints Lua file
    /// @return Error if parsing failed
    virtual LuaResult<void> loadBlueprints(std::string_view path) = 0;

    /// Load all blueprints from the blueprints directory
    /// @return Error if any file failed to load
    virtual LuaResult<void> loadAllBlueprints() = 0;

    /// Check if a blueprint exists
    virtual bool hasBlueprint(std::string_view name) const = 0;

    /// Get a blueprint definition
    virtual std::optional<LuaBlueprintDef> getBlueprint(std::string_view name) const = 0;

    /// Get all blueprint names
    virtual std::vector<std::string> getBlueprintNames() const = 0;

    /// Clear all loaded blueprints
    virtual void clearBlueprints() = 0;

    //==========================================================================
    // Entity Creation (from Blueprints)
    //==========================================================================

    /// Create an entity from a blueprint
    /// @param blueprintName Name of the blueprint
    /// @param x X position
    /// @param y Y position
    /// @return Created entity or error
    virtual LuaResult<Entity> spawn(std::string_view blueprintName, float x, float y) = 0;

    /// Create an entity from a blueprint with size
    virtual LuaResult<Entity> spawn(std::string_view blueprintName,
                                    float x, float y,
                                    float width, float height) = 0;

    /// Create an entity from a blueprint with property overrides
    virtual LuaResult<Entity> spawn(std::string_view blueprintName,
                                    float x, float y,
                                    const PropertyMap& overrides) = 0;

    /// Create an entity from a blueprint with size and overrides
    virtual LuaResult<Entity> spawn(std::string_view blueprintName,
                                    float x, float y,
                                    float width, float height,
                                    const PropertyMap& overrides) = 0;

    //==========================================================================
    // Component Registration
    //==========================================================================

    /// Register a C++ component type that can be created from Lua blueprints
    /// @param name Component type name (e.g., "Transform", "Sprite")
    /// @param creator Function to create the component from properties
    virtual void registerComponent(std::string_view name, ComponentCreator creator) = 0;

    /// Check if a component type is registered
    virtual bool isComponentRegistered(std::string_view name) const = 0;

    /// Get all registered component names
    virtual std::vector<std::string> getRegisteredComponents() const = 0;

    //==========================================================================
    // Behavior Management
    //==========================================================================

    /// Load a behavior script
    /// @param path Path to behavior Lua file
    /// @return Handle to the loaded behavior or error
    virtual LuaResult<BehaviorHandle> loadBehavior(std::string_view path) = 0;

    /// Load all behaviors from the behaviors directory
    virtual LuaResult<void> loadAllBehaviors() = 0;

    /// Attach a behavior to an entity
    /// @param entity Entity to attach to
    /// @param behavior Behavior handle
    /// @return Error if entity or behavior invalid
    virtual LuaResult<void> attachBehavior(Entity entity, BehaviorHandle behavior) = 0;

    /// Attach a behavior by name
    virtual LuaResult<void> attachBehavior(Entity entity, std::string_view behaviorName) = 0;

    /// Detach a behavior from an entity
    virtual void detachBehavior(Entity entity, BehaviorHandle behavior) = 0;

    /// Detach all behaviors from an entity
    virtual void detachAllBehaviors(Entity entity) = 0;

    /// Get all behaviors attached to an entity
    virtual std::vector<BehaviorHandle> getBehaviors(Entity entity) const = 0;

    /// Enable/disable a behavior on an entity
    virtual void setBehaviorEnabled(Entity entity, BehaviorHandle behavior, bool enabled) = 0;

    /// Get behavior state table (for inspection/debugging)
    virtual sol::table getBehaviorState(Entity entity, BehaviorHandle behavior) = 0;

    //==========================================================================
    // Lua System Management
    //==========================================================================

    /// Load a Lua system
    /// @param path Path to system Lua file
    /// @return Handle to the loaded system or error
    virtual LuaResult<LuaSystemHandle> loadSystem(std::string_view path) = 0;

    /// Load all systems from the systems directory
    virtual LuaResult<void> loadAllSystems() = 0;

    /// Enable/disable a Lua system
    virtual void setSystemEnabled(LuaSystemHandle system, bool enabled) = 0;

    /// Get all loaded Lua systems
    virtual std::vector<LuaSystemHandle> getLuaSystems() const = 0;

    /// Get system info
    virtual std::optional<LuaSystemDef> getSystemInfo(LuaSystemHandle system) const = 0;

    //==========================================================================
    // Hot Reload
    //==========================================================================

    /// Enable/disable hot reload for Lua files
    virtual void enableHotReload(bool enable) = 0;

    /// Check if hot reload is enabled
    virtual bool isHotReloadEnabled() const = 0;

    /// Force reload of a specific file
    /// @param path Path to Lua file to reload
    /// @return Error if reload failed
    virtual LuaResult<void> reloadFile(std::string_view path) = 0;

    /// Reload all loaded Lua files
    virtual LuaResult<void> reloadAll() = 0;

    /// Callback for when a config file is reloaded
    using ConfigReloadCallback = std::function<void(std::string_view path)>;

    /// Subscribe to config reload events
    /// @param callback Called when any config file is reloaded
    /// @return Subscription ID for unsubscribing
    virtual SubscriptionId onConfigReloaded(ConfigReloadCallback callback) = 0;

    /// Subscribe to reload events for a specific config path pattern
    /// @param pathPattern Glob pattern (e.g., "config/*.lua")
    /// @param callback Called when matching file is reloaded
    virtual SubscriptionId onConfigReloaded(std::string_view pathPattern,
                                             ConfigReloadCallback callback) = 0;

    /// Unsubscribe from config reload events
    virtual void unsubscribeConfigReload(SubscriptionId id) = 0;

    /// Watch a config file for hot reload
    /// @param path Path to the config file
    /// @return Asset handle for the watched file
    virtual AssetHandle watchConfig(std::string_view path) = 0;

    //==========================================================================
    // Direct Lua Access
    //==========================================================================

    /// Execute Lua code directly
    /// @param code Lua source code
    /// @param description Description for error messages
    /// @return Result object or error
    virtual LuaResult<sol::object> execute(std::string_view code,
                                            std::string_view description = "script") = 0;

    /// Execute Lua code for side effects only
    /// @param code Lua source code
    /// @param description Description for error messages
    /// @return Error if execution failed
    virtual LuaResult<void> run(std::string_view code,
                                 std::string_view description = "script") = 0;

    /// Call a global Lua function
    /// @param funcName Name of the global function
    /// @param args Arguments to pass
    /// @return Result or error
    template<typename... Args>
    LuaResult<sol::object> call(std::string_view funcName, Args&&... args) {
        return callImpl(funcName, std::forward<Args>(args)...);
    }

    /// Set a global Lua variable
    virtual void setGlobal(std::string_view name, sol::object value) = 0;

    /// Get a global Lua variable
    virtual sol::object getGlobal(std::string_view name) = 0;

    /// Get direct access to the Lua state
    /// WARNING: The state is shared; modifications may affect the entire runtime
    virtual sol::state& getLuaState() = 0;

    //==========================================================================
    // Configuration Values (Legacy IConfigSystem compatibility)
    //==========================================================================

    /// Get a config value (from loaded config files)
    virtual std::optional<float> getFloat(std::string_view key) const = 0;
    virtual std::optional<int> getInt(std::string_view key) const = 0;
    virtual std::optional<bool> getBool(std::string_view key) const = 0;
    virtual std::optional<std::string> getString(std::string_view key) const = 0;

    /// Get with defaults
    virtual float getFloatOr(std::string_view key, float defaultVal) const = 0;
    virtual int getIntOr(std::string_view key, int defaultVal) const = 0;
    virtual bool getBoolOr(std::string_view key, bool defaultVal) const = 0;
    virtual std::string getStringOr(std::string_view key, std::string_view defaultVal) const = 0;

    /// Get a table/array as a vector of variants
    /// @param key Dot-separated path to the table (e.g., "graphics.clearColor")
    /// @return Vector of property values (empty if not found or not a table)
    virtual std::vector<PropertyValue> getTable(std::string_view key) const = 0;

    /// Set config values at runtime
    virtual void setFloat(std::string_view key, float value) = 0;
    virtual void setInt(std::string_view key, int value) = 0;
    virtual void setBool(std::string_view key, bool value) = 0;
    virtual void setString(std::string_view key, std::string_view value) = 0;

    /// Check if a config key exists
    virtual bool hasKey(std::string_view key) const = 0;

    /// Load a config file
    virtual LuaResult<void> loadConfig(std::string_view path) = 0;

    //==========================================================================
    // Engine System Bindings
    //==========================================================================

    /// Check if an engine system is available from Lua
    virtual bool isSystemAvailable(std::string_view systemName) const = 0;

    /// Get list of available engine systems
    virtual std::vector<std::string> getAvailableSystems() const = 0;

protected:
    /// Implementation of variadic call
    virtual LuaResult<sol::object> callImpl(std::string_view funcName,
                                             sol::variadic_args args) = 0;
};

}  // namespace bestow

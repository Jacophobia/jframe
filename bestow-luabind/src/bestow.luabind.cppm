// bestow-luabind/src/bestow.luabind.cppm
// Lua contract bindings - Exposes C++ contracts to Lua via bestow.* namespace
//
// This module provides the LuaContractBinder class that binds all registered
// C++ contract interfaces to Lua, making them accessible via the bestow.* namespace.
//
// Example Lua usage:
//   bestow.input.isActionActive("jump")
//   bestow.audio.playOnChannel(soundHandle, 1)
//   bestow.entity.create()

module;

#include <bestow/sol2_compat.hpp>
#include <spdlog/spdlog.h>

export module bestow.luabind;

import std;
import bestow.services;  // Re-exports all contracts
import bestow.core;      // Engine class

export namespace bestow {

//=============================================================================
// Forward declarations for binding functions
//=============================================================================

// Type bindings (Vec2, Vec3, Color, Transform, etc.)
void bindTypes(sol::state& lua);

// Contract bindings - 2D systems
void bindInputSystem(sol::state& lua, IInputSystem& input);
void bindAudioSystem(sol::state& lua, IAudioSystem& audio);
void bindPhysicsSystem(sol::state& lua, IPhysicsSystem& physics);

// Contract bindings - 3D systems
void bindPhysics3DSystem(sol::state& lua, IPhysics3DSystem& physics3d);
void bindGraphics3DSystem(sol::state& lua, IGraphics3DSystem& graphics3d,
                          IEntitySystem* entities = nullptr, IAnimationSystem* animation = nullptr);
void bindAnimationSystem(sol::state& lua, IAnimationSystem& animation,
                         IAssetSystem* assets = nullptr, IGraphics3DSystem* graphics = nullptr);

// Character system binding (high-level animated character API)
void bindCharacterSystem(sol::state& lua, IAnimationSystem& animation,
                         IAssetSystem* assets, IGraphics3DSystem* graphics);

// Entity system binding (reflection-based component access)
void bindEntitySystem(sol::state& lua, IEntitySystem& entity);

// Config and Assets bindings
void bindConfigSystem(sol::state& lua, IConfigSystem& config);
void bindAssetSystem(sol::state& lua, IAssetSystem& assets);

// Gameplay Ability System binding
void bindGASSystem(sol::state& lua, IGASSystem& gas);

// Scene System binding
void bindSceneSystem(sol::state& lua, ISceneSystem& scene, IAssetSystem* assets = nullptr);

// Blueprint Factory binding
void bindBlueprintFactory(sol::state& lua, IBlueprintFactory& blueprints);

// Game State System binding
void bindGameStateSystem(sol::state& lua, IGameStateSystem& gamestate);

// Save System binding
void bindSaveSystem(sol::state& lua, ISaveSystem& save);

// Event System binding
void bindEventSystem(sol::state& lua, IEventSystem& events);

// UI System binding
void bindUISystem(sol::state& lua, IUISystem& ui);

// Action Builder and event-driven input API binding
void bindActionBuilder(sol::state& lua, IInputSystem& input);

// Metrics/Profiling System binding (always available, Tracy optional)
void bindMetricsSystem(sol::state& lua);

// Timer System binding (hot-reload-safe timers, no contract needed)
void bindTimerSystem(sol::state& lua);

// Update function for timers - call this every frame from game loop
void updateTimers(float dt);

// Cleanup function - MUST be called before lua_close() to release Lua references
// This clears global managers (timers, events) that hold sol::table/sol::object refs
void cleanupLuaBindings();

// Future bindings
// void bindAISystem(sol::state& lua, IAISystem& ai);
// void bindCameraSystem(sol::state& lua, ICameraSystem& camera);

//=============================================================================
// API Documentation Data Model
//=============================================================================

struct ParamDoc {
    std::string name;
    std::string type;
    std::string description;
    bool optional = false;
    std::string defaultVal;
};

struct ReturnDoc {
    std::string type;
    std::string description;
};

struct MethodDoc {
    std::string name;
    std::string qualifiedName;
    std::string description;
    std::vector<ParamDoc> params;
    std::vector<ReturnDoc> returns;
    std::string example;
    bool deprecated = false;
    std::vector<std::string> seeAlso;
};

struct EnumValueDoc {
    std::string name;
    std::string description;
};

struct EnumDoc {
    std::string name;
    std::string qualifiedName;
    std::string description;
    std::vector<EnumValueDoc> values;
};

struct FieldDoc {
    std::string name;
    std::string type;
    std::string description;
    bool readOnly = false;
};

struct TypeDoc {
    std::string name;
    std::string qualifiedName;
    std::string description;
    std::vector<FieldDoc> fields;
    std::vector<MethodDoc> methods;
    std::string example;
};

struct PropertyDoc {
    std::string name;
    std::string type;
    std::string description;
    bool readOnly = false;
};

struct SystemDoc {
    std::string name;
    std::string qualifiedName;
    std::string description;
    std::vector<MethodDoc> methods;
    std::vector<EnumDoc> enums;
    std::vector<TypeDoc> types;
    std::vector<PropertyDoc> properties;
    std::vector<std::string> seeAlso;
};

//=============================================================================
// DocRegistry - Stores and queries API documentation
//=============================================================================

class DocRegistry {
public:
    // Registration
    void addSystem(SystemDoc system);

    // Queries
    const std::vector<SystemDoc>& getAllSystems() const;
    const SystemDoc* getSystem(const std::string& name) const;
    const MethodDoc* getMethod(const std::string& qualifiedName) const;
    const EnumDoc* getEnum(const std::string& qualifiedName) const;
    const TypeDoc* getType(const std::string& qualifiedName) const;

    struct SearchResult {
        std::string qualifiedName;
        std::string kind; // "method", "enum", "type", "system", "property"
        std::string description;
        std::string systemName;
    };
    std::vector<SearchResult> search(const std::string& query) const;

private:
    std::vector<SystemDoc> systems_;
};

//=============================================================================
// DocFormatter - Formats documentation for terminal output
//=============================================================================

class DocFormatter {
public:
    explicit DocFormatter(bool useColor = true);

    std::string formatSystemList(const DocRegistry& registry) const;
    std::string formatSystem(const SystemDoc& system) const;
    std::string formatMethod(const MethodDoc& method) const;
    std::string formatEnum(const EnumDoc& e) const;
    std::string formatType(const TypeDoc& type) const;
    std::string formatSearchResults(const std::vector<DocRegistry::SearchResult>& results,
                                    const std::string& query) const;
    std::string formatNotFound(const std::string& query) const;

private:
    std::string bold(const std::string& text) const;
    std::string dim(const std::string& text) const;
    std::string cyan(const std::string& text) const;
    std::string green(const std::string& text) const;
    std::string yellow(const std::string& text) const;
    std::string magenta(const std::string& text) const;
    std::string underline(const std::string& text) const;
    std::string reset() const;

    bool useColor_;
};

//=============================================================================
// createFullDocRegistry - Populates registry with all API docs
//=============================================================================

DocRegistry createFullDocRegistry();

// Per-system doc registration functions (implemented in docs/*_binding_doc.cpp)
void registerTypesDoc(DocRegistry& registry);
void registerCoreDoc(DocRegistry& registry);
void registerInputDoc(DocRegistry& registry);
void registerActionDoc(DocRegistry& registry);
void registerAudioDoc(DocRegistry& registry);
void registerPhysicsDoc(DocRegistry& registry);
void registerPhysics3DDoc(DocRegistry& registry);
void registerGraphics3DDoc(DocRegistry& registry);
void registerAnimationDoc(DocRegistry& registry);
void registerCharacterDoc(DocRegistry& registry);
void registerEntityDoc(DocRegistry& registry);
void registerConfigDoc(DocRegistry& registry);
void registerAssetsDoc(DocRegistry& registry);
void registerGASDoc(DocRegistry& registry);
void registerSceneDoc(DocRegistry& registry);
void registerBlueprintsDoc(DocRegistry& registry);
void registerGamestateDoc(DocRegistry& registry);
void registerSaveDoc(DocRegistry& registry);
void registerEventsDoc(DocRegistry& registry);
void registerUIDoc(DocRegistry& registry);
void registerMetricsDoc(DocRegistry& registry);
void registerTimerDoc(DocRegistry& registry);

//=============================================================================
// LuaErrorContext - Captures Lua call context for better error messages
//=============================================================================

/// Captures the Lua call stack context for generating helpful error messages.
/// Use this to provide file:line information and stack traces in errors.
struct LuaErrorContext {
    std::string file;                      // Source file
    int line = 0;                          // Line number
    std::string functionName;              // Function name (if available)
    std::vector<std::string> stackTrace;   // Full stack trace

    /// Capture context from current Lua state
    /// @param L Lua state
    /// @param startLevel Stack level to start from (default 2 skips capture call)
    static LuaErrorContext capture(lua_State* L, int startLevel = 2);

    /// Get formatted location string "file:line"
    std::string location() const;

    /// Get formatted stack trace
    std::string formatStackTrace() const;
};

//=============================================================================
// Error Throwing Utilities - Use these instead of raw std::runtime_error
//=============================================================================

/// Throw a Lua error with source location and stack trace
/// @param L Lua state
/// @param message Error message
void luaError(lua_State* L, const std::string& message);

/// Throw a Lua error with API name prefix
/// @param L Lua state
/// @param apiName Name of the API (e.g., "bestow.timer.after")
/// @param message Error message
void luaError(lua_State* L, const std::string& apiName, const std::string& message);

/// Throw a type error for wrong argument type
/// @param L Lua state
/// @param apiName Name of the API
/// @param argNum Argument number (1-indexed)
/// @param expected Expected type name
/// @param actual Actual type name or sol::type
void luaTypeError(lua_State* L, const std::string& apiName, int argNum,
                  const std::string& expected, const std::string& actual);
void luaTypeError(lua_State* L, const std::string& apiName, int argNum,
                  const std::string& expected, sol::type actual);

/// Throw an argument validation error
/// @param L Lua state
/// @param apiName Name of the API
/// @param argNum Argument number (1-indexed)
/// @param requirement Description of the requirement that wasn't met
void luaArgError(lua_State* L, const std::string& apiName, int argNum,
                 const std::string& requirement);

/// Throw a range error for out-of-bounds values
/// @param L Lua state
/// @param apiName Name of the API
/// @param paramName Name of the parameter
/// @param value The invalid value
/// @param minVal Minimum valid value
/// @param maxVal Maximum valid value
void luaRangeError(lua_State* L, const std::string& apiName,
                   const std::string& paramName, double value,
                   double minVal, double maxVal);

/// Throw a not-found error for missing resources
/// @param L Lua state
/// @param apiName Name of the API
/// @param resourceType Type of resource (e.g., "Entity", "Sound", "Blueprint")
/// @param name Name/identifier of the missing resource
void luaNotFoundError(lua_State* L, const std::string& apiName,
                      const std::string& resourceType, const std::string& name);

/// Get simple location string for logging (returns "file:line")
/// @param L Lua state
/// @param level Stack level (default 2)
std::string getLuaLocation(lua_State* L, int level = 2);

//=============================================================================
// LuaContractBinder - Main class for binding contracts to Lua
//=============================================================================

/// LuaContractBinder binds all registered C++ contract interfaces to Lua.
///
/// It creates the `bestow` global table with sub-tables for each system:
/// - bestow.input  -> IInputSystem methods
/// - bestow.audio  -> IAudioSystem methods
/// - bestow.entity -> IEntitySystem methods
/// - etc.
///
/// Usage:
/// @code
/// bestow::core::Engine engine;
/// // ... register systems ...
///
/// sol::state lua;
/// LuaContractBinder binder(engine, lua);
/// binder.bindAll();
/// @endcode
class LuaContractBinder {
public:
    /// Construct a binder with an Engine reference and Lua state.
    /// @param engine The Engine containing registered systems
    /// @param lua The Lua state to bind to
    explicit LuaContractBinder(core::Engine& engine, sol::state& lua);

    ~LuaContractBinder() = default;

    // Non-copyable, non-movable
    LuaContractBinder(const LuaContractBinder&) = delete;
    LuaContractBinder& operator=(const LuaContractBinder&) = delete;
    LuaContractBinder(LuaContractBinder&&) = delete;
    LuaContractBinder& operator=(LuaContractBinder&&) = delete;

    //=========================================================================
    // Binding Methods
    //=========================================================================

    /// Bind all registered systems to Lua.
    /// Creates the bestow.* namespace with sub-tables for each available system.
    void bindAll();

    /// Bind only core types (Vec2, Vec3, Color, etc.) without system methods.
    /// Useful for testing or minimal setups.
    void bindTypesOnly();

    //=========================================================================
    // Stub Generation (for IDE support)
    //=========================================================================

    /// Generate EmmyLua-annotated stub files for IDE autocomplete.
    /// @param outputDir Directory to write stub files to
    void generateStubs(const std::filesystem::path& outputDir);

    //=========================================================================
    // Queries
    //=========================================================================

    /// Check if a system is bound.
    /// @param systemName Name of the system (e.g., "input", "audio")
    bool isSystemBound(const std::string& systemName) const;

    /// Get list of all bound systems.
    std::vector<std::string> getBoundSystems() const;

private:
    void createBestowTable();
    void bindCoreUtilities();

    core::Engine* engine_ = nullptr;
    sol::state* lua_ = nullptr;
    std::vector<std::string> boundSystems_;
    bool initialized_ = false;
};

//=============================================================================
// StubGenerator - Generates EmmyLua stubs for IDE support
//=============================================================================

/// StubGenerator creates EmmyLua-annotated Lua stub files for IDE autocomplete.
///
/// These stubs provide:
/// - Type definitions for Vec2, Vec3, Color, etc.
/// - Function signatures for bestow.* API
/// - Documentation comments for hover info
///
/// Usage:
/// @code
/// StubGenerator gen;
/// gen.generate("sdk/stubs/");
/// @endcode
///
/// Then configure your IDE (e.g., VS Code with Lua Language Server):
/// @code
/// {
///   "Lua.workspace.library": ["path/to/sdk/stubs"]
/// }
/// @endcode
class StubGenerator {
public:
    /// Generate all stub files to the output directory.
    /// @param outputDir Directory to write stub files to
    void generate(const std::filesystem::path& outputDir);

private:
    void generateTypesStub();
    void generateEntityStub();
    void generateInputStub();
    void generateAudioStub();
    void generatePhysicsStub();
    void generatePhysics3DStub();
    void generateGraphics3DStub();
    void generateAnimationStub();
    void generateCoreStub();
    void generateIndexStub();

    std::filesystem::path outputDir_;
};

}  // namespace bestow

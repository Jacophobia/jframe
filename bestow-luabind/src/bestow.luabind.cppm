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
void bindGraphics3DSystem(sol::state& lua, IGraphics3DSystem& graphics3d);
void bindAnimationSystem(sol::state& lua, IAnimationSystem& animation);

// Entity system binding (reflection-based component access)
void bindEntitySystem(sol::state& lua, IEntitySystem& entity);

// Future bindings (Step 5+)
// void bindAssetSystem(sol::state& lua, IAssetSystem& assets);
// void bindEventSystem(sol::state& lua, IEventSystem& events);

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

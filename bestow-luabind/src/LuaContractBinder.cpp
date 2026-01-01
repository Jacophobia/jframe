// bestow-luabind/src/LuaContractBinder.cpp
// Main LuaContractBinder implementation

module;

#include <bestow/sol2_compat.hpp>
#include <spdlog/spdlog.h>

module bestow.luabind;

import std;

namespace bestow {

//=============================================================================
// Constructor
//=============================================================================

LuaContractBinder::LuaContractBinder(core::Engine& engine, sol::state& lua)
    : engine_(&engine), lua_(&lua) {}

//=============================================================================
// Binding Methods
//=============================================================================

void LuaContractBinder::bindAll() {
    if (initialized_) {
        spdlog::warn("[LuaContractBinder] Already initialized, rebinding...");
        boundSystems_.clear();
    }

    spdlog::info("[LuaContractBinder] Binding C++ contracts to Lua...");

    // Create the bestow table
    createBestowTable();

    // Bind core types first (Vec2, Vec3, Color, etc.)
    bindTypes(*lua_);
    spdlog::debug("[LuaContractBinder] Bound core types");

    // Bind core utilities (deltaTime, etc.)
    bindCoreUtilities();

    // Bind each registered system
    if (engine_->has<IInputSystem>()) {
        bindInputSystem(*lua_, engine_->get<IInputSystem>());
        boundSystems_.push_back("input");
        spdlog::debug("[LuaContractBinder] Bound IInputSystem -> bestow.input");
    }

    if (engine_->has<IAudioSystem>()) {
        bindAudioSystem(*lua_, engine_->get<IAudioSystem>());
        boundSystems_.push_back("audio");
        spdlog::debug("[LuaContractBinder] Bound IAudioSystem -> bestow.audio");
    }

    if (engine_->has<IPhysicsSystem>()) {
        bindPhysicsSystem(*lua_, engine_->get<IPhysicsSystem>());
        boundSystems_.push_back("physics");
        spdlog::debug("[LuaContractBinder] Bound IPhysicsSystem -> bestow.physics");
    }

    // 3D Systems
    if (engine_->has<IPhysics3DSystem>()) {
        bindPhysics3DSystem(*lua_, engine_->get<IPhysics3DSystem>());
        boundSystems_.push_back("physics3d");
        spdlog::debug("[LuaContractBinder] Bound IPhysics3DSystem -> bestow.physics3d");
    }

    if (engine_->has<IGraphics3DSystem>()) {
        bindGraphics3DSystem(*lua_, engine_->get<IGraphics3DSystem>());
        boundSystems_.push_back("graphics3d");
        spdlog::debug("[LuaContractBinder] Bound IGraphics3DSystem -> bestow.graphics3d");
    }

    if (engine_->has<IAnimationSystem>()) {
        bindAnimationSystem(*lua_, engine_->get<IAnimationSystem>());
        boundSystems_.push_back("animation");
        spdlog::debug("[LuaContractBinder] Bound IAnimationSystem -> bestow.animation");
    }

    // Entity System (with reflection-based component access)
    if (engine_->has<IEntitySystem>()) {
        bindEntitySystem(*lua_, engine_->get<IEntitySystem>());
        boundSystems_.push_back("entity");
        spdlog::debug("[LuaContractBinder] Bound IEntitySystem -> bestow.entity");
    }

    // Future bindings will be added here as they're implemented:
    // - IAssetSystem -> bestow.assets
    // - IEventSystem -> bestow.events

    initialized_ = true;
    spdlog::info("[LuaContractBinder] Bound {} systems to Lua", boundSystems_.size());
}

void LuaContractBinder::bindTypesOnly() {
    createBestowTable();
    bindTypes(*lua_);
    spdlog::info("[LuaContractBinder] Bound core types only");
}

//=============================================================================
// Private Helpers
//=============================================================================

void LuaContractBinder::createBestowTable() {
    // Create the bestow global table if it doesn't exist
    if (!(*lua_)["bestow"].valid()) {
        (*lua_)["bestow"] = lua_->create_table();
    }
}

void LuaContractBinder::bindCoreUtilities() {
    sol::table bestow = (*lua_)["bestow"];

    // Create bestow.core table for utility functions
    sol::table core = lua_->create_table();

    // Note: deltaTime() will be set per-frame by the game loop
    // For now, we provide a placeholder that returns 0
    core["deltaTime"] = []() -> float {
        // This should be updated by the game loop each frame
        // For now, return a small default value
        return 0.016f;  // ~60fps
    };

    // Version info
    core["version"] = "1.0.0";
    core["engine"] = "Bestow";

    bestow["core"] = core;
}

//=============================================================================
// Stub Generation
//=============================================================================

void LuaContractBinder::generateStubs(const std::filesystem::path& outputDir) {
    StubGenerator generator;
    generator.generate(outputDir);
}

//=============================================================================
// Queries
//=============================================================================

bool LuaContractBinder::isSystemBound(const std::string& systemName) const {
    return std::find(boundSystems_.begin(), boundSystems_.end(), systemName) != boundSystems_.end();
}

std::vector<std::string> LuaContractBinder::getBoundSystems() const {
    return boundSystems_;
}

}  // namespace bestow

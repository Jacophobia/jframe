// jframe-level/src/LevelSystem.cpp

module;

#include <any>
#include <cstdint>
#include <optional>
#include <string>
#include <system_error>
#include <unordered_map>
#include <vector>

#include <sol/sol.hpp>
#include <sol/types.hpp>

module jframe.level.impl;

namespace jframe {

UUID LevelSystem::generateLevelId() {
    return nextLevelId_++;
}

bool LevelSystem::initialize(IAssetSystem* assetSystem) {
    assetSystem_ = assetSystem;

    // Initialize Lua state with sandboxing
    lua_.open_libraries(sol::lib::base, sol::lib::math, sol::lib::table, sol::lib::string);

    // Apply sandboxing - remove dangerous functions
    lua_["os"] = sol::lua_nil;
    lua_["io"] = sol::lua_nil;
    lua_["loadfile"] = sol::lua_nil;
    lua_["dofile"] = sol::lua_nil;
    lua_["load"] = sol::lua_nil;
    lua_["loadstring"] = sol::lua_nil;
    lua_["require"] = sol::lua_nil;
    lua_["package"] = sol::lua_nil;

    return true;
}

void LevelSystem::update(DeltaTime dt) {
    // Handle pending transitions
    if (pendingTransition_) {
        if (pendingTransition_->unloadPrevious && activeLevel_) {
            unloadLevel(*activeLevel_);
        }
        activeLevel_ = pendingTransition_->toLevel;
        pendingTransition_ = std::nullopt;
    }
}

Result<LevelId, std::error_code> LevelSystem::loadLevel(AssetHandle levelAsset) {
    LevelId id = generateLevelId();
    LoadedLevel level;
    level.metadata.id = id;
    level.metadata.assetHandle = levelAsset;
    level.metadata.state = LevelState::Loaded;

    // If we have access to the AssetSystem, load and parse the Lua level file
    if (assetSystem_ != nullptr) {
        // Get the level asset data (assumes it's already loaded)
        const void* rawAsset = assetSystem_->getRawAsset(levelAsset);
        if (rawAsset != nullptr) {
            try {
                // getRawAsset returns a pointer to std::any, so we need to dereference and cast
                const std::any* assetAny = static_cast<const std::any*>(rawAsset);
                const DataAsset& dataAsset = std::any_cast<const DataAsset&>(*assetAny);
                if (!dataAsset.rawText.empty()) {
                    // Parse the Lua level file
                    if (!parseLevelLua(dataAsset.rawText, level)) {
                        // Parsing failed - return error
                        return std::unexpected(std::make_error_code(std::errc::invalid_argument));
                    }
                }
            } catch (const std::bad_any_cast&) {
                // Asset is not a DataAsset, skip parsing
            }
        }
    }

    levels_[id] = std::move(level);
    return id;
}

void LevelSystem::unloadLevel(LevelId levelId) {
    levels_.erase(levelId);
    if (activeLevel_ == levelId) {
        activeLevel_ = std::nullopt;
    }
}

void LevelSystem::setActiveLevel(LevelId levelId) {
    if (levels_.contains(levelId)) {
        activeLevel_ = levelId;
    }
}

void LevelSystem::transition(const LevelTransition& transition) {
    pendingTransition_ = transition;
}

std::optional<LevelId> LevelSystem::getActiveLevel() const {
    return activeLevel_;
}

LevelState LevelSystem::getLevelState(LevelId levelId) const {
    if (auto it = levels_.find(levelId); it != levels_.end()) {
        return it->second.metadata.state;
    }
    return LevelState::Unloaded;
}

LevelMetadata LevelSystem::getLevelMetadata(LevelId levelId) const {
    if (auto it = levels_.find(levelId); it != levels_.end()) {
        return it->second.metadata;
    }
    return {};
}

std::vector<LevelMetadata> LevelSystem::getLoadedLevels() const {
    std::vector<LevelMetadata> result;
    for (const auto& [id, level] : levels_) {
        result.push_back(level.metadata);
    }
    return result;
}

std::optional<Transform2D> LevelSystem::getSpawnPoint(LevelId levelId,
                                                       const std::string& name) const {
    if (auto it = levels_.find(levelId); it != levels_.end()) {
        if (auto sp = it->second.spawnPoints.find(name); sp != it->second.spawnPoints.end()) {
            return sp->second;
        }
    }
    return std::nullopt;
}

std::vector<std::string> LevelSystem::getSpawnPointNames(LevelId levelId) const {
    std::vector<std::string> names;
    if (auto it = levels_.find(levelId); it != levels_.end()) {
        for (const auto& [name, _] : it->second.spawnPoints) {
            names.push_back(name);
        }
    }
    return names;
}

std::vector<Entity> LevelSystem::getLevelEntities(LevelId levelId) const {
    if (auto it = levels_.find(levelId); it != levels_.end()) {
        return it->second.entities;
    }
    return {};
}

std::vector<EntityDef> LevelSystem::getEntityDefs(LevelId levelId) const {
    if (auto it = levels_.find(levelId); it != levels_.end()) {
        return it->second.entityDefs;
    }
    return {};
}

}  // namespace jframe

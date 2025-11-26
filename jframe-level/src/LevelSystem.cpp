// jframe-level/src/LevelSystem.cpp

module;

#include <cstdint>
#include <optional>
#include <string>
#include <system_error>
#include <unordered_map>
#include <vector>

module jframe.level.impl;

namespace jframe {

UUID LevelSystem::generateLevelId() {
    return nextLevelId_++;
}

bool LevelSystem::initialize() {
    // Initialize Lua state with sandboxing
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

}  // namespace jframe

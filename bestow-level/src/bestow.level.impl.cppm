// bestow-level/src/bestow.level.impl.cppm
// Level system implementation using Lua

module;

#include <kangaru/kangaru.hpp>
#include <bestow/sol2_compat.hpp>

export module bestow.level.impl;

import std;
import bestow.level;
import bestow.types;
import bestow.assets;
import bestow.services;

export namespace bestow {

class LevelSystem : public ILevelSystem {
public:
    LevelSystem() = default;
    ~LevelSystem() override = default;

    bool initialize(IAssetSystem* assetSystem = nullptr);

    void update(DeltaTime dt) override;

    // Level management
    Result<LevelId, std::error_code> loadLevel(AssetHandle levelAsset) override;
    void unloadLevel(LevelId levelId) override;
    void setActiveLevel(LevelId levelId) override;

    // Level transitions
    void transition(const LevelTransition& transition) override;

    // State queries
    std::optional<LevelId> getActiveLevel() const override;
    LevelState getLevelState(LevelId levelId) const override;
    LevelMetadata getLevelMetadata(LevelId levelId) const override;
    std::vector<LevelMetadata> getLoadedLevels() const override;

    // Spawn points
    std::optional<Transform2D> getSpawnPoint(LevelId levelId,
                                              const std::string& name) const override;
    std::vector<std::string> getSpawnPointNames(LevelId levelId) const override;

    // Level queries
    std::vector<Entity> getLevelEntities(LevelId levelId) const override;

    // Entity definitions
    std::vector<EntityDef> getEntityDefs(LevelId levelId) const override;

private:
    struct LoadedLevel {
        LevelMetadata metadata;
        std::vector<Entity> entities;
        std::unordered_map<std::string, Transform2D> spawnPoints;
        std::vector<EntityDef> entityDefs;
    };

    UUID generateLevelId();
    bool parseLevelLua(const std::string& luaCode, LoadedLevel& level);

    IAssetSystem* assetSystem_ = nullptr;
    sol::state lua_;
    std::unordered_map<LevelId, LoadedLevel> levels_;
    std::optional<LevelId> activeLevel_;
    std::optional<LevelTransition> pendingTransition_;
    UUID nextLevelId_ = 1;
};

// Kangaru service definitions
// Concrete service that provides LevelSystem as ILevelSystem
struct LevelSystemService : kgr::single_service<LevelSystem>, kgr::overrides<ILevelSystemService> {};

}  // namespace bestow

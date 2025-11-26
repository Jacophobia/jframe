// jframe-level/src/jframe.level.impl.cppm
// Level system implementation using Lua

module;

export module jframe.level.impl;

import std;
import jframe.level;
import jframe.types;

export namespace jframe {

class LevelSystem : public ILevelSystem {
public:
    LevelSystem() = default;
    ~LevelSystem() override = default;

    bool initialize();

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

private:
    struct LoadedLevel {
        LevelMetadata metadata;
        std::vector<Entity> entities;
        std::unordered_map<std::string, Transform2D> spawnPoints;
    };

    UUID generateLevelId();

    std::unordered_map<LevelId, LoadedLevel> levels_;
    std::optional<LevelId> activeLevel_;
    std::optional<LevelTransition> pendingTransition_;
    UUID nextLevelId_ = 1;
};

// Factory function (exported via namespace)
inline std::unique_ptr<ILevelSystem> createLevelSystem() {
    return std::make_unique<LevelSystem>();
}

}  // namespace jframe

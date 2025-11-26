// jframe-contract/src/jframe.level.cppm
// Level system interface

module;

#include <optional>
#include <string>
#include <system_error>
#include <vector>

export module jframe.level;

import jframe.types;

export namespace jframe {

struct LevelMetadata {
    LevelId id;
    AssetHandle assetHandle;
    std::string levelName;
    LevelState state = LevelState::Unloaded;
    float width = 0.0f;
    float height = 0.0f;
};

class ILevelSystem {
public:
    virtual ~ILevelSystem() = default;

    //======================================================================
    // Lifecycle
    //======================================================================

    virtual void update(DeltaTime dt) = 0;

    //======================================================================
    // Level Management
    //======================================================================

    virtual Result<LevelId, std::error_code> loadLevel(AssetHandle levelAsset) = 0;
    virtual void unloadLevel(LevelId levelId) = 0;
    virtual void setActiveLevel(LevelId levelId) = 0;

    //======================================================================
    // Level Transitions
    //======================================================================

    virtual void transition(const LevelTransition& transition) = 0;

    //======================================================================
    // State Queries
    //======================================================================

    virtual std::optional<LevelId> getActiveLevel() const = 0;
    virtual LevelState getLevelState(LevelId levelId) const = 0;
    virtual LevelMetadata getLevelMetadata(LevelId levelId) const = 0;
    virtual std::vector<LevelMetadata> getLoadedLevels() const = 0;

    //======================================================================
    // Spawn Points
    //======================================================================

    virtual std::optional<Transform2D> getSpawnPoint(LevelId levelId,
                                                      const std::string& name) const = 0;
    virtual std::vector<std::string> getSpawnPointNames(LevelId levelId) const = 0;

    //======================================================================
    // Level Queries
    //======================================================================

    virtual std::vector<Entity> getLevelEntities(LevelId levelId) const = 0;
};

}  // namespace jframe

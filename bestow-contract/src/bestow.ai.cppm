// bestow-contract/src/bestow.ai.cppm
// AI system interface

module;

#include <any>
#include <optional>
#include <string>
#include <vector>

export module bestow.ai;

import bestow.types;

export namespace bestow {

struct NavMeshQuery {
    Vec2 start;
    Vec2 end;
    float agentRadius = 0.5f;
};

struct NavigationPath {
    std::vector<Vec2> waypoints;
    float totalLength = 0.0f;
    bool isComplete = false;
};

// Patrol behavior configuration
struct PatrolBehavior {
    float startX = 0.0f;      // Center X position of patrol
    float range = 100.0f;     // Distance to patrol in each direction
    float speed = 50.0f;      // Movement speed
    bool movingRight = true;  // Current direction (for sprite flipping)
};

class IAISystem {
public:
    virtual ~IAISystem() = default;

    //======================================================================
    // Lifecycle
    //======================================================================

    virtual void update(DeltaTime dt) = 0;

    //======================================================================
    // Behavior Trees
    //======================================================================

    virtual void attachBehaviorTree(Entity entity, AssetHandle treeAsset) = 0;
    virtual void detachBehaviorTree(Entity entity) = 0;
    virtual bool hasBehaviorTree(Entity entity) const = 0;

    virtual void setBehaviorTreeBlackboard(Entity entity,
                                            const std::string& key,
                                            const std::any& value) = 0;
    virtual std::any getBehaviorTreeBlackboard(Entity entity,
                                                const std::string& key) const = 0;

    //======================================================================
    // Navigation
    //======================================================================

    virtual void loadNavMesh(AssetHandle navMeshAsset) = 0;
    virtual void unloadNavMesh() = 0;
    virtual bool hasNavMesh() const = 0;

    virtual std::optional<NavigationPath> findPath(const NavMeshQuery& query) const = 0;
    virtual bool isPointOnNavMesh(Vec2 point) const = 0;
    virtual std::optional<Vec2> getClosestPointOnNavMesh(Vec2 point) const = 0;

    //======================================================================
    // Steering Behaviors
    //======================================================================

    virtual void setNavigationTarget(Entity entity, Vec2 target) = 0;
    virtual void clearNavigationTarget(Entity entity) = 0;
    virtual std::optional<Vec2> getNavigationTarget(Entity entity) const = 0;

    virtual void setMaxSpeed(Entity entity, float speed) = 0;
    virtual void setMaxAcceleration(Entity entity, float acceleration) = 0;

    //======================================================================
    // Patrol Behavior
    //======================================================================

    virtual void setPatrolBehavior(Entity entity, const PatrolBehavior& patrol) = 0;
    virtual void clearPatrolBehavior(Entity entity) = 0;
    virtual std::optional<PatrolBehavior> getPatrolBehavior(Entity entity) const = 0;

    //======================================================================
    // Spatial Queries
    //======================================================================

    virtual std::vector<Entity> findEntitiesInRadius(Vec2 center, float radius,
                                                      CollisionMask mask = 0xFFFF) const = 0;
    virtual std::optional<Entity> findClosestEntity(Vec2 position,
                                                     CollisionMask mask = 0xFFFF) const = 0;
    virtual bool hasLineOfSight(Vec2 from, Vec2 to,
                                CollisionMask obstacleMask = 0xFFFF) const = 0;
};

}  // namespace bestow

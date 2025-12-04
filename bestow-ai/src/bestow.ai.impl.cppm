// bestow-ai/src/bestow.ai.impl.cppm
// AI system implementation

module;

#include <DetourNavMesh.h>
#include <DetourNavMeshQuery.h>
#include <DetourStatus.h>

export module bestow.ai.impl;

import std;
import bestow.ai;
import bestow.assets;
import bestow.assets.impl;
import bestow.physics;
import bestow.types;

export namespace bestow {

class AISystem : public IAISystem {
public:
    explicit AISystem(IPhysicsSystem* physicsSystem, IAssetSystem* assetSystem);
    ~AISystem() override;

    bool initialize();

    void update(DeltaTime dt) override;

    // Behavior trees
    void attachBehaviorTree(Entity entity, AssetHandle treeAsset) override;
    void detachBehaviorTree(Entity entity) override;
    bool hasBehaviorTree(Entity entity) const override;
    void setBehaviorTreeBlackboard(Entity entity, const std::string& key,
                                    const std::any& value) override;
    std::any getBehaviorTreeBlackboard(Entity entity,
                                        const std::string& key) const override;

    // Navigation
    void loadNavMesh(AssetHandle navMeshAsset) override;
    void unloadNavMesh() override;
    bool hasNavMesh() const override;
    std::optional<NavigationPath> findPath(const NavMeshQuery& query) const override;
    bool isPointOnNavMesh(Vec2 point) const override;
    std::optional<Vec2> getClosestPointOnNavMesh(Vec2 point) const override;

    // Steering behaviors
    void setNavigationTarget(Entity entity, Vec2 target) override;
    void clearNavigationTarget(Entity entity) override;
    std::optional<Vec2> getNavigationTarget(Entity entity) const override;
    void setMaxSpeed(Entity entity, float speed) override;
    void setMaxAcceleration(Entity entity, float acceleration) override;

    // Patrol behavior
    void setPatrolBehavior(Entity entity, const PatrolBehavior& patrol) override;
    void clearPatrolBehavior(Entity entity) override;
    std::optional<PatrolBehavior> getPatrolBehavior(Entity entity) const override;

    // Spatial queries
    std::vector<Entity> findEntitiesInRadius(Vec2 center, float radius,
                                              CollisionMask mask = 0xFFFF) const override;
    std::optional<Entity> findClosestEntity(Vec2 position,
                                             CollisionMask mask = 0xFFFF) const override;
    bool hasLineOfSight(Vec2 from, Vec2 to,
                        CollisionMask obstacleMask = 0xFFFF) const override;

private:
    struct AIComponent {
        AssetHandle behaviorTree;
        std::unordered_map<std::string, std::any> blackboard;
        std::optional<Vec2> navigationTarget;
        float maxSpeed = 100.0f;
        float maxAcceleration = 500.0f;
        std::optional<PatrolBehavior> patrol;
    };

    IPhysicsSystem* physicsSystem_;
    IAssetSystem* assetSystem_;
    std::unordered_map<std::uint32_t, AIComponent> aiComponents_;
    AssetHandle navMeshAsset_;
    bool hasNavMesh_ = false;

    // Detour navigation
    dtNavMesh* navMesh_ = nullptr;
    dtNavMeshQuery* navQuery_ = nullptr;
};

// Factory function (exported via namespace)
inline std::unique_ptr<IAISystem> createAISystem(IPhysicsSystem* physicsSystem, IAssetSystem* assetSystem) {
    return std::make_unique<AISystem>(physicsSystem, assetSystem);
}

}  // namespace bestow

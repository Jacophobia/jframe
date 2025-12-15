// bestow-ai/src/bestow.ai.impl.cppm
// AI system implementation

module;

#include <kangaru/kangaru.hpp>
#include <DetourNavMesh.h>
#include <DetourNavMeshQuery.h>
#include <DetourStatus.h>

#if defined(BESTOW_HAS_BTCPP)
#include <behaviortree_cpp/bt_factory.h>
#include <behaviortree_cpp/behavior_tree.h>
#endif

export module bestow.ai.impl;

import std;
import bestow.services;  // Re-exports all contracts including bestow.ai, bestow.types, etc.

export namespace bestow {

// Steering behavior types
enum class SteeringBehaviorType {
    None,
    Seek,       // Move toward target at max speed
    Flee,       // Move away from target at max speed
    Arrive,     // Move toward target with deceleration
    Pursue,     // Predict and intercept moving target
    Evade       // Predict and avoid moving target
};

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

    // Extended steering behavior API
    void setSteeringBehavior(Entity entity, SteeringBehaviorType behavior);
    void setArrivalRadius(Entity entity, float radius);

private:
    struct AIComponent {
        AssetHandle behaviorTree;
        std::unordered_map<std::string, std::any> blackboard;
        std::optional<Vec2> navigationTarget;
        float maxSpeed = 100.0f;
        float maxAcceleration = 500.0f;
        std::optional<PatrolBehavior> patrol;
        SteeringBehaviorType steeringBehavior = SteeringBehaviorType::None;
        float arrivalRadius = 50.0f;  // Deceleration radius for Arrive behavior
#if defined(BESTOW_HAS_BTCPP)
        std::unique_ptr<BT::Tree> btTree;
#endif
    };

    IPhysicsSystem* physicsSystem_;
    IAssetSystem* assetSystem_;
    std::unordered_map<std::uint32_t, AIComponent> aiComponents_;
    AssetHandle navMeshAsset_;
    bool hasNavMesh_ = false;

    // Detour navigation
    dtNavMesh* navMesh_ = nullptr;
    dtNavMeshQuery* navQuery_ = nullptr;

#if defined(BESTOW_HAS_BTCPP)
    // BehaviorTree.CPP factory
    BT::BehaviorTreeFactory btFactory_;
    void initializeBehaviorTreeFactory();
    void tickBehaviorTree(Entity entity, AIComponent& ai, DeltaTime dt);
#endif

    // Steering behavior helpers
    Vec2 calculateSeek(Vec2 position, Vec2 target, float maxSpeed) const;
    Vec2 calculateFlee(Vec2 position, Vec2 target, float maxSpeed) const;
    Vec2 calculateArrive(Vec2 position, Vec2 target, float maxSpeed, float arrivalRadius) const;
    void applySteeringBehavior(Entity entity, AIComponent& ai, DeltaTime dt);
};

// Kangaru service definitions
// AISystem has constructor dependencies (IPhysicsSystem*, IAssetSystem*)
// that require interface pointers, so it must be emplaced manually with dependencies:
//   auto& physics = container.service<PhysicsSystemService>();
//   auto& assets = container.service<AssetSystemService>();
//   container.emplace<AISystemService>(&physics, &assets);
struct AISystemService : kgr::single_service<AISystem>, kgr::overrides<IAISystemService> {};

}  // namespace bestow

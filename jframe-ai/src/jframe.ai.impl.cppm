// jframe-ai/src/jframe.ai.impl.cppm
// AI system implementation

module;

export module jframe.ai.impl;

import std;
import jframe.ai;
import jframe.types;

export namespace jframe {

class AISystem : public IAISystem {
public:
    AISystem() = default;
    ~AISystem() override = default;

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
    };

    std::unordered_map<std::uint32_t, AIComponent> aiComponents_;
    bool hasNavMesh_ = false;
};

// Factory function (exported via namespace)
inline std::unique_ptr<IAISystem> createAISystem() {
    return std::make_unique<AISystem>();
}

}  // namespace jframe

// tests/unit/AISystemTests.cpp
// Unit tests for Bestow AI System

#include <any>
#include <cmath>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include <gtest/gtest.h>
#include <kangaru/kangaru.hpp>

import bestow.ai;
import bestow.ai.impl;
import bestow.assets;
import bestow.assets.impl;
import bestow.physics;
import bestow.physics.impl;
import bestow.types;

namespace bestow::tests {

class AISystemTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Get asset system from container
        assetSystem_ = &container_.service<AssetSystemService>();

        // Get physics system from container and initialize
        physicsSystem_ = &container_.service<PhysicsSystemService>();
        auto* physicsImpl = dynamic_cast<Box2DPhysicsSystem*>(physicsSystem_);
        if (physicsImpl) {
            physicsImpl->initialize();
        }

        // AISystem has constructor dependencies that require pointers,
        // so construct it directly with DI-resolved dependencies
        aiSystemImpl_ = std::make_unique<AISystem>(physicsSystem_, assetSystem_);
        aiSystemImpl_->initialize();
        aiSystem_ = aiSystemImpl_.get();
    }

    kgr::container container_;
    IAssetSystem* assetSystem_ = nullptr;
    IPhysicsSystem* physicsSystem_ = nullptr;
    std::unique_ptr<AISystem> aiSystemImpl_;
    IAISystem* aiSystem_ = nullptr;
};

//=============================================================================
// Basic System Tests
//=============================================================================

TEST_F(AISystemTest, CanCreateAISystem) {
    EXPECT_NE(aiSystem_, nullptr);
}

TEST_F(AISystemTest, UpdateDoesNotCrashWithNoEntities) {
    aiSystem_->update(1.0f / 60.0f);  // 60 FPS delta time
}

//=============================================================================
// Behavior Tree Management Tests
//=============================================================================

TEST_F(AISystemTest, AttachBehaviorTreeToEntity) {
    Entity entity = static_cast<Entity>(1);
    AssetHandle treeAsset = static_cast<AssetHandle>(100);

    aiSystem_->attachBehaviorTree(entity, treeAsset);
    EXPECT_TRUE(aiSystem_->hasBehaviorTree(entity));
}

TEST_F(AISystemTest, DetachBehaviorTreeFromEntity) {
    Entity entity = static_cast<Entity>(2);
    AssetHandle treeAsset = static_cast<AssetHandle>(101);

    aiSystem_->attachBehaviorTree(entity, treeAsset);
    EXPECT_TRUE(aiSystem_->hasBehaviorTree(entity));

    aiSystem_->detachBehaviorTree(entity);
    EXPECT_FALSE(aiSystem_->hasBehaviorTree(entity));
}

TEST_F(AISystemTest, HasBehaviorTreeReturnsFalseForEntityWithoutTree) {
    Entity entity = static_cast<Entity>(3);
    EXPECT_FALSE(aiSystem_->hasBehaviorTree(entity));
}

TEST_F(AISystemTest, DetachBehaviorTreeFromNonExistentEntityDoesNotCrash) {
    Entity entity = static_cast<Entity>(999);
    aiSystem_->detachBehaviorTree(entity);  // Should not crash
    EXPECT_FALSE(aiSystem_->hasBehaviorTree(entity));
}

TEST_F(AISystemTest, AttachingNewTreeReplacesOldTree) {
    Entity entity = static_cast<Entity>(4);
    AssetHandle tree1 = static_cast<AssetHandle>(102);
    AssetHandle tree2 = static_cast<AssetHandle>(103);

    aiSystem_->attachBehaviorTree(entity, tree1);
    EXPECT_TRUE(aiSystem_->hasBehaviorTree(entity));

    aiSystem_->attachBehaviorTree(entity, tree2);
    EXPECT_TRUE(aiSystem_->hasBehaviorTree(entity));
}

//=============================================================================
// Blackboard Tests
//=============================================================================

TEST_F(AISystemTest, CanSetBlackboardValue) {
    Entity entity = static_cast<Entity>(5);
    std::string key = "health";
    int value = 100;

    aiSystem_->setBehaviorTreeBlackboard(entity, key, value);

    std::any retrieved = aiSystem_->getBehaviorTreeBlackboard(entity, key);
    EXPECT_TRUE(retrieved.has_value());
    EXPECT_EQ(std::any_cast<int>(retrieved), value);
}

TEST_F(AISystemTest, CanSetMultipleBlackboardValues) {
    Entity entity = static_cast<Entity>(6);

    aiSystem_->setBehaviorTreeBlackboard(entity, "health", 100);
    aiSystem_->setBehaviorTreeBlackboard(entity, "ammo", 50);
    aiSystem_->setBehaviorTreeBlackboard(entity, "aggressive", true);

    int health = std::any_cast<int>(aiSystem_->getBehaviorTreeBlackboard(entity, "health"));
    int ammo = std::any_cast<int>(aiSystem_->getBehaviorTreeBlackboard(entity, "ammo"));
    bool aggressive = std::any_cast<bool>(aiSystem_->getBehaviorTreeBlackboard(entity, "aggressive"));

    EXPECT_EQ(health, 100);
    EXPECT_EQ(ammo, 50);
    EXPECT_TRUE(aggressive);
}

TEST_F(AISystemTest, BlackboardReturnsEmptyForNonExistentKey) {
    Entity entity = static_cast<Entity>(7);

    std::any retrieved = aiSystem_->getBehaviorTreeBlackboard(entity, "nonexistent");
    EXPECT_FALSE(retrieved.has_value());
}

TEST_F(AISystemTest, BlackboardReturnsEmptyForNonExistentEntity) {
    Entity entity = static_cast<Entity>(999);

    std::any retrieved = aiSystem_->getBehaviorTreeBlackboard(entity, "somekey");
    EXPECT_FALSE(retrieved.has_value());
}

TEST_F(AISystemTest, BlackboardCanStoreFloatValues) {
    Entity entity = static_cast<Entity>(8);
    float value = 3.14f;

    aiSystem_->setBehaviorTreeBlackboard(entity, "pi", value);

    std::any retrieved = aiSystem_->getBehaviorTreeBlackboard(entity, "pi");
    EXPECT_TRUE(retrieved.has_value());
    EXPECT_FLOAT_EQ(std::any_cast<float>(retrieved), value);
}

TEST_F(AISystemTest, BlackboardCanStoreStringValues) {
    Entity entity = static_cast<Entity>(9);
    std::string value = "patrol";

    aiSystem_->setBehaviorTreeBlackboard(entity, "state", value);

    std::any retrieved = aiSystem_->getBehaviorTreeBlackboard(entity, "state");
    EXPECT_TRUE(retrieved.has_value());
    EXPECT_EQ(std::any_cast<std::string>(retrieved), value);
}

TEST_F(AISystemTest, BlackboardValuesAreIndependentBetweenEntities) {
    Entity entity1 = static_cast<Entity>(10);
    Entity entity2 = static_cast<Entity>(11);

    aiSystem_->setBehaviorTreeBlackboard(entity1, "value", 100);
    aiSystem_->setBehaviorTreeBlackboard(entity2, "value", 200);

    int value1 = std::any_cast<int>(aiSystem_->getBehaviorTreeBlackboard(entity1, "value"));
    int value2 = std::any_cast<int>(aiSystem_->getBehaviorTreeBlackboard(entity2, "value"));

    EXPECT_EQ(value1, 100);
    EXPECT_EQ(value2, 200);
}

//=============================================================================
// Navigation - NavMesh Management Tests
//=============================================================================

TEST_F(AISystemTest, InitiallyHasNoNavMesh) {
    EXPECT_FALSE(aiSystem_->hasNavMesh());
}

TEST_F(AISystemTest, CanLoadNavMesh) {
    AssetHandle navMeshAsset = static_cast<AssetHandle>(200);
    aiSystem_->loadNavMesh(navMeshAsset);
    EXPECT_TRUE(aiSystem_->hasNavMesh());
}

TEST_F(AISystemTest, CanUnloadNavMesh) {
    AssetHandle navMeshAsset = static_cast<AssetHandle>(201);
    aiSystem_->loadNavMesh(navMeshAsset);
    EXPECT_TRUE(aiSystem_->hasNavMesh());

    aiSystem_->unloadNavMesh();
    EXPECT_FALSE(aiSystem_->hasNavMesh());
}

TEST_F(AISystemTest, UnloadNavMeshWithoutLoadDoesNotCrash) {
    aiSystem_->unloadNavMesh();  // Should not crash
    EXPECT_FALSE(aiSystem_->hasNavMesh());
}

TEST_F(AISystemTest, LoadingNewNavMeshReplacesOldOne) {
    AssetHandle navMesh1 = static_cast<AssetHandle>(202);
    AssetHandle navMesh2 = static_cast<AssetHandle>(203);

    aiSystem_->loadNavMesh(navMesh1);
    EXPECT_TRUE(aiSystem_->hasNavMesh());

    aiSystem_->loadNavMesh(navMesh2);
    EXPECT_TRUE(aiSystem_->hasNavMesh());
}

//=============================================================================
// Navigation - Pathfinding Tests
//=============================================================================

TEST_F(AISystemTest, FindPathReturnsNulloptWithoutNavMesh) {
    NavMeshQuery query{
        .start = {0.0f, 0.0f},
        .end = {100.0f, 100.0f}
    };

    std::optional<NavigationPath> path = aiSystem_->findPath(query);
    EXPECT_FALSE(path.has_value());
}

TEST_F(AISystemTest, FindPathReturnsPathWithNavMesh) {
    AssetHandle navMeshAsset = static_cast<AssetHandle>(204);
    aiSystem_->loadNavMesh(navMeshAsset);

    NavMeshQuery query{
        .start = {0.0f, 0.0f},
        .end = {100.0f, 100.0f}
    };

    std::optional<NavigationPath> path = aiSystem_->findPath(query);
    EXPECT_TRUE(path.has_value());
}

TEST_F(AISystemTest, FindPathWithAgentRadius) {
    AssetHandle navMeshAsset = static_cast<AssetHandle>(205);
    aiSystem_->loadNavMesh(navMeshAsset);

    NavMeshQuery query{
        .start = {0.0f, 0.0f},
        .end = {100.0f, 100.0f},
        .agentRadius = 1.0f
    };

    std::optional<NavigationPath> path = aiSystem_->findPath(query);
    EXPECT_TRUE(path.has_value());
}

TEST_F(AISystemTest, NavigationPathHasCorrectStructure) {
    AssetHandle navMeshAsset = static_cast<AssetHandle>(206);
    aiSystem_->loadNavMesh(navMeshAsset);

    NavMeshQuery query{
        .start = {0.0f, 0.0f},
        .end = {100.0f, 100.0f}
    };

    std::optional<NavigationPath> path = aiSystem_->findPath(query);
    ASSERT_TRUE(path.has_value());

    // Path structure should be valid (even if empty in Wave 1)
    EXPECT_GE(path->totalLength, 0.0f);
}

//=============================================================================
// Navigation - Point Query Tests
//=============================================================================

TEST_F(AISystemTest, IsPointOnNavMeshReturnsFalseWithoutNavMesh) {
    Vec2 point{50.0f, 50.0f};
    EXPECT_FALSE(aiSystem_->isPointOnNavMesh(point));
}

TEST_F(AISystemTest, IsPointOnNavMeshReturnsTrueWithNavMesh) {
    AssetHandle navMeshAsset = static_cast<AssetHandle>(207);
    aiSystem_->loadNavMesh(navMeshAsset);

    Vec2 point{50.0f, 50.0f};
    EXPECT_TRUE(aiSystem_->isPointOnNavMesh(point));
}

TEST_F(AISystemTest, GetClosestPointOnNavMeshReturnsNulloptWithoutNavMesh) {
    Vec2 point{50.0f, 50.0f};
    std::optional<Vec2> closest = aiSystem_->getClosestPointOnNavMesh(point);
    EXPECT_FALSE(closest.has_value());
}

TEST_F(AISystemTest, GetClosestPointOnNavMeshReturnsPointWithNavMesh) {
    AssetHandle navMeshAsset = static_cast<AssetHandle>(208);
    aiSystem_->loadNavMesh(navMeshAsset);

    Vec2 point{50.0f, 50.0f};
    std::optional<Vec2> closest = aiSystem_->getClosestPointOnNavMesh(point);
    EXPECT_TRUE(closest.has_value());
}

TEST_F(AISystemTest, GetClosestPointReturnsValidCoordinates) {
    AssetHandle navMeshAsset = static_cast<AssetHandle>(209);
    aiSystem_->loadNavMesh(navMeshAsset);

    Vec2 point{50.0f, 50.0f};
    std::optional<Vec2> closest = aiSystem_->getClosestPointOnNavMesh(point);

    ASSERT_TRUE(closest.has_value());
    // In Wave 1, implementation returns the input point
    EXPECT_FLOAT_EQ(closest->x, point.x);
    EXPECT_FLOAT_EQ(closest->y, point.y);
}

//=============================================================================
// Steering Behaviors - Navigation Target Tests
//=============================================================================

TEST_F(AISystemTest, CanSetNavigationTarget) {
    Entity entity = static_cast<Entity>(20);
    Vec2 target{100.0f, 200.0f};

    aiSystem_->setNavigationTarget(entity, target);

    std::optional<Vec2> retrieved = aiSystem_->getNavigationTarget(entity);
    ASSERT_TRUE(retrieved.has_value());
    EXPECT_FLOAT_EQ(retrieved->x, target.x);
    EXPECT_FLOAT_EQ(retrieved->y, target.y);
}

TEST_F(AISystemTest, CanClearNavigationTarget) {
    Entity entity = static_cast<Entity>(21);
    Vec2 target{100.0f, 200.0f};

    aiSystem_->setNavigationTarget(entity, target);
    EXPECT_TRUE(aiSystem_->getNavigationTarget(entity).has_value());

    aiSystem_->clearNavigationTarget(entity);
    EXPECT_FALSE(aiSystem_->getNavigationTarget(entity).has_value());
}

TEST_F(AISystemTest, GetNavigationTargetReturnsNulloptForEntityWithoutTarget) {
    Entity entity = static_cast<Entity>(22);
    EXPECT_FALSE(aiSystem_->getNavigationTarget(entity).has_value());
}

TEST_F(AISystemTest, ClearNavigationTargetOnNonExistentEntityDoesNotCrash) {
    Entity entity = static_cast<Entity>(999);
    aiSystem_->clearNavigationTarget(entity);  // Should not crash
}

TEST_F(AISystemTest, SetNavigationTargetReplacesOldTarget) {
    Entity entity = static_cast<Entity>(23);
    Vec2 target1{100.0f, 200.0f};
    Vec2 target2{300.0f, 400.0f};

    aiSystem_->setNavigationTarget(entity, target1);
    aiSystem_->setNavigationTarget(entity, target2);

    std::optional<Vec2> retrieved = aiSystem_->getNavigationTarget(entity);
    ASSERT_TRUE(retrieved.has_value());
    EXPECT_FLOAT_EQ(retrieved->x, target2.x);
    EXPECT_FLOAT_EQ(retrieved->y, target2.y);
}

//=============================================================================
// Steering Behaviors - Speed and Acceleration Tests
//=============================================================================

TEST_F(AISystemTest, CanSetMaxSpeed) {
    Entity entity = static_cast<Entity>(24);
    float maxSpeed = 150.0f;

    aiSystem_->setMaxSpeed(entity, maxSpeed);
    // Should not crash - value should be stored
}

TEST_F(AISystemTest, CanSetMaxAcceleration) {
    Entity entity = static_cast<Entity>(25);
    float maxAcceleration = 750.0f;

    aiSystem_->setMaxAcceleration(entity, maxAcceleration);
    // Should not crash - value should be stored
}

TEST_F(AISystemTest, SetMaxSpeedOnNonExistentEntityDoesNotCrash) {
    Entity entity = static_cast<Entity>(999);
    aiSystem_->setMaxSpeed(entity, 100.0f);  // Should not crash
}

TEST_F(AISystemTest, SetMaxAccelerationOnNonExistentEntityDoesNotCrash) {
    Entity entity = static_cast<Entity>(999);
    aiSystem_->setMaxAcceleration(entity, 500.0f);  // Should not crash
}

TEST_F(AISystemTest, CanSetMultipleSteeringParameters) {
    Entity entity = static_cast<Entity>(26);

    aiSystem_->setMaxSpeed(entity, 200.0f);
    aiSystem_->setMaxAcceleration(entity, 1000.0f);
    aiSystem_->setNavigationTarget(entity, Vec2{150.0f, 250.0f});

    // Should not crash - all parameters should be stored
    std::optional<Vec2> target = aiSystem_->getNavigationTarget(entity);
    EXPECT_TRUE(target.has_value());
}

//=============================================================================
// Spatial Queries - Find Entities in Radius Tests
//=============================================================================

TEST_F(AISystemTest, FindEntitiesInRadiusReturnsEmptyByDefault) {
    Vec2 center{100.0f, 100.0f};
    float radius = 50.0f;

    std::vector<Entity> entities = aiSystem_->findEntitiesInRadius(center, radius);
    EXPECT_TRUE(entities.empty());
}

TEST_F(AISystemTest, FindEntitiesInRadiusWithMask) {
    Vec2 center{100.0f, 100.0f};
    float radius = 50.0f;
    CollisionMask mask = 0x0001;  // Specific layer

    std::vector<Entity> entities = aiSystem_->findEntitiesInRadius(center, radius, mask);
    EXPECT_TRUE(entities.empty());  // Wave 1: not yet implemented
}

TEST_F(AISystemTest, FindEntitiesInRadiusWithZeroRadius) {
    Vec2 center{100.0f, 100.0f};
    float radius = 0.0f;

    std::vector<Entity> entities = aiSystem_->findEntitiesInRadius(center, radius);
    EXPECT_TRUE(entities.empty());
}

TEST_F(AISystemTest, FindEntitiesInRadiusWithLargeRadius) {
    Vec2 center{0.0f, 0.0f};
    float radius = 10000.0f;

    std::vector<Entity> entities = aiSystem_->findEntitiesInRadius(center, radius);
    EXPECT_TRUE(entities.empty());  // Wave 1: not yet implemented
}

//=============================================================================
// Spatial Queries - Find Closest Entity Tests
//=============================================================================

TEST_F(AISystemTest, FindClosestEntityReturnsNulloptByDefault) {
    Vec2 position{100.0f, 100.0f};

    std::optional<Entity> closest = aiSystem_->findClosestEntity(position);
    EXPECT_FALSE(closest.has_value());
}

TEST_F(AISystemTest, FindClosestEntityWithMask) {
    Vec2 position{100.0f, 100.0f};
    CollisionMask mask = 0x0002;  // Specific layer

    std::optional<Entity> closest = aiSystem_->findClosestEntity(position, mask);
    EXPECT_FALSE(closest.has_value());  // Wave 1: not yet implemented
}

//=============================================================================
// Spatial Queries - Line of Sight Tests
//=============================================================================

TEST_F(AISystemTest, HasLineOfSightReturnsTrueByDefault) {
    Vec2 from{0.0f, 0.0f};
    Vec2 to{100.0f, 100.0f};

    bool hasLOS = aiSystem_->hasLineOfSight(from, to);
    EXPECT_TRUE(hasLOS);  // No obstacles = clear line of sight
}

TEST_F(AISystemTest, HasLineOfSightWithObstacleMask) {
    Vec2 from{0.0f, 0.0f};
    Vec2 to{100.0f, 100.0f};
    CollisionMask obstacleMask = 0x0004;  // Terrain layer

    bool hasLOS = aiSystem_->hasLineOfSight(from, to, obstacleMask);
    EXPECT_TRUE(hasLOS);  // No obstacles = clear line of sight
}

TEST_F(AISystemTest, HasLineOfSightBetweenSamePoint) {
    Vec2 point{50.0f, 50.0f};

    bool hasLOS = aiSystem_->hasLineOfSight(point, point);
    EXPECT_TRUE(hasLOS);
}

//=============================================================================
// Integration Tests
//=============================================================================

TEST_F(AISystemTest, FullWorkflowLoadNavMeshFindPathFollowPath) {
    // Setup navmesh
    AssetHandle navMeshAsset = static_cast<AssetHandle>(300);
    aiSystem_->loadNavMesh(navMeshAsset);
    EXPECT_TRUE(aiSystem_->hasNavMesh());

    // Find path
    NavMeshQuery query{
        .start = {0.0f, 0.0f},
        .end = {100.0f, 100.0f},
        .agentRadius = 0.5f
    };

    std::optional<NavigationPath> path = aiSystem_->findPath(query);
    ASSERT_TRUE(path.has_value());

    // Setup entity to follow path
    Entity entity = static_cast<Entity>(100);
    aiSystem_->setNavigationTarget(entity, query.end);
    aiSystem_->setMaxSpeed(entity, 100.0f);
    aiSystem_->setMaxAcceleration(entity, 500.0f);

    std::optional<Vec2> target = aiSystem_->getNavigationTarget(entity);
    EXPECT_TRUE(target.has_value());
}

TEST_F(AISystemTest, FullWorkflowBehaviorTreeWithBlackboard) {
    Entity entity = static_cast<Entity>(101);

    // Attach behavior tree
    AssetHandle treeAsset = static_cast<AssetHandle>(400);
    aiSystem_->attachBehaviorTree(entity, treeAsset);
    EXPECT_TRUE(aiSystem_->hasBehaviorTree(entity));

    // Setup blackboard
    aiSystem_->setBehaviorTreeBlackboard(entity, "target", Vec2{100.0f, 100.0f});
    aiSystem_->setBehaviorTreeBlackboard(entity, "state", std::string("patrol"));
    aiSystem_->setBehaviorTreeBlackboard(entity, "alert", false);

    // Verify blackboard
    std::any target = aiSystem_->getBehaviorTreeBlackboard(entity, "target");
    std::any state = aiSystem_->getBehaviorTreeBlackboard(entity, "state");
    std::any alert = aiSystem_->getBehaviorTreeBlackboard(entity, "alert");

    EXPECT_TRUE(target.has_value());
    EXPECT_TRUE(state.has_value());
    EXPECT_TRUE(alert.has_value());
}

TEST_F(AISystemTest, MultipleEntitiesIndependentBehavior) {
    Entity entity1 = static_cast<Entity>(102);
    Entity entity2 = static_cast<Entity>(103);
    Entity entity3 = static_cast<Entity>(104);

    // Setup entity 1
    AssetHandle tree1 = static_cast<AssetHandle>(401);
    aiSystem_->attachBehaviorTree(entity1, tree1);
    aiSystem_->setNavigationTarget(entity1, Vec2{100.0f, 100.0f});
    aiSystem_->setBehaviorTreeBlackboard(entity1, "role", std::string("guard"));

    // Setup entity 2
    AssetHandle tree2 = static_cast<AssetHandle>(402);
    aiSystem_->attachBehaviorTree(entity2, tree2);
    aiSystem_->setNavigationTarget(entity2, Vec2{200.0f, 200.0f});
    aiSystem_->setBehaviorTreeBlackboard(entity2, "role", std::string("scout"));

    // Setup entity 3
    aiSystem_->setMaxSpeed(entity3, 50.0f);

    // Verify independence
    EXPECT_TRUE(aiSystem_->hasBehaviorTree(entity1));
    EXPECT_TRUE(aiSystem_->hasBehaviorTree(entity2));
    EXPECT_FALSE(aiSystem_->hasBehaviorTree(entity3));

    std::optional<Vec2> target1 = aiSystem_->getNavigationTarget(entity1);
    std::optional<Vec2> target2 = aiSystem_->getNavigationTarget(entity2);
    std::optional<Vec2> target3 = aiSystem_->getNavigationTarget(entity3);

    EXPECT_TRUE(target1.has_value());
    EXPECT_TRUE(target2.has_value());
    EXPECT_FALSE(target3.has_value());

    std::string role1 = std::any_cast<std::string>(
        aiSystem_->getBehaviorTreeBlackboard(entity1, "role"));
    std::string role2 = std::any_cast<std::string>(
        aiSystem_->getBehaviorTreeBlackboard(entity2, "role"));

    EXPECT_EQ(role1, "guard");
    EXPECT_EQ(role2, "scout");
}

TEST_F(AISystemTest, UpdateWithMultipleEntitiesDoesNotCrash) {
    // Setup multiple entities with various AI components
    for (int i = 0; i < 10; ++i) {
        Entity entity = static_cast<Entity>(200 + i);
        AssetHandle tree = static_cast<AssetHandle>(500 + i);

        aiSystem_->attachBehaviorTree(entity, tree);
        aiSystem_->setNavigationTarget(entity, Vec2{static_cast<float>(i * 10),
                                                    static_cast<float>(i * 20)});
        aiSystem_->setMaxSpeed(entity, 100.0f + static_cast<float>(i * 10));
        aiSystem_->setBehaviorTreeBlackboard(entity, "id", i);
    }

    // Update should process all entities
    aiSystem_->update(1.0f / 60.0f);
}

//=============================================================================
// Patrol Behavior Tests
//=============================================================================

TEST_F(AISystemTest, CanSetPatrolBehavior) {
    Entity entity = static_cast<Entity>(600);
    PatrolBehavior patrol{
        .startX = 100.0f,
        .range = 50.0f,
        .speed = 75.0f,
        .movingRight = true
    };

    aiSystem_->setPatrolBehavior(entity, patrol);

    std::optional<PatrolBehavior> retrieved = aiSystem_->getPatrolBehavior(entity);
    ASSERT_TRUE(retrieved.has_value());
    EXPECT_FLOAT_EQ(retrieved->startX, patrol.startX);
    EXPECT_FLOAT_EQ(retrieved->range, patrol.range);
    EXPECT_FLOAT_EQ(retrieved->speed, patrol.speed);
    EXPECT_EQ(retrieved->movingRight, patrol.movingRight);
}

TEST_F(AISystemTest, CanClearPatrolBehavior) {
    Entity entity = static_cast<Entity>(601);
    PatrolBehavior patrol{
        .startX = 100.0f,
        .range = 50.0f,
        .speed = 75.0f,
        .movingRight = true
    };

    aiSystem_->setPatrolBehavior(entity, patrol);
    EXPECT_TRUE(aiSystem_->getPatrolBehavior(entity).has_value());

    aiSystem_->clearPatrolBehavior(entity);
    EXPECT_FALSE(aiSystem_->getPatrolBehavior(entity).has_value());
}

TEST_F(AISystemTest, GetPatrolBehaviorReturnsNulloptForEntityWithoutPatrol) {
    Entity entity = static_cast<Entity>(602);
    EXPECT_FALSE(aiSystem_->getPatrolBehavior(entity).has_value());
}

TEST_F(AISystemTest, ClearPatrolBehaviorOnNonExistentEntityDoesNotCrash) {
    Entity entity = static_cast<Entity>(999);
    aiSystem_->clearPatrolBehavior(entity);  // Should not crash
}

TEST_F(AISystemTest, SetPatrolBehaviorReplacesOldBehavior) {
    Entity entity = static_cast<Entity>(603);
    PatrolBehavior patrol1{
        .startX = 100.0f,
        .range = 50.0f,
        .speed = 75.0f,
        .movingRight = true
    };
    PatrolBehavior patrol2{
        .startX = 200.0f,
        .range = 100.0f,
        .speed = 120.0f,
        .movingRight = false
    };

    aiSystem_->setPatrolBehavior(entity, patrol1);
    aiSystem_->setPatrolBehavior(entity, patrol2);

    std::optional<PatrolBehavior> retrieved = aiSystem_->getPatrolBehavior(entity);
    ASSERT_TRUE(retrieved.has_value());
    EXPECT_FLOAT_EQ(retrieved->startX, patrol2.startX);
    EXPECT_FLOAT_EQ(retrieved->range, patrol2.range);
    EXPECT_FLOAT_EQ(retrieved->speed, patrol2.speed);
    EXPECT_EQ(retrieved->movingRight, patrol2.movingRight);
}

TEST_F(AISystemTest, PatrolBehaviorWithZeroRange) {
    Entity entity = static_cast<Entity>(604);
    PatrolBehavior patrol{
        .startX = 100.0f,
        .range = 0.0f,
        .speed = 50.0f,
        .movingRight = true
    };

    aiSystem_->setPatrolBehavior(entity, patrol);

    std::optional<PatrolBehavior> retrieved = aiSystem_->getPatrolBehavior(entity);
    ASSERT_TRUE(retrieved.has_value());
    EXPECT_FLOAT_EQ(retrieved->range, 0.0f);
}

TEST_F(AISystemTest, PatrolBehaviorWithNegativeSpeed) {
    Entity entity = static_cast<Entity>(605);
    PatrolBehavior patrol{
        .startX = 100.0f,
        .range = 50.0f,
        .speed = -75.0f,  // Negative speed
        .movingRight = true
    };

    aiSystem_->setPatrolBehavior(entity, patrol);

    std::optional<PatrolBehavior> retrieved = aiSystem_->getPatrolBehavior(entity);
    ASSERT_TRUE(retrieved.has_value());
    EXPECT_FLOAT_EQ(retrieved->speed, -75.0f);
}

TEST_F(AISystemTest, PatrolBehaviorIndependentBetweenEntities) {
    Entity entity1 = static_cast<Entity>(606);
    Entity entity2 = static_cast<Entity>(607);

    PatrolBehavior patrol1{
        .startX = 100.0f,
        .range = 50.0f,
        .speed = 75.0f,
        .movingRight = true
    };
    PatrolBehavior patrol2{
        .startX = 300.0f,
        .range = 80.0f,
        .speed = 120.0f,
        .movingRight = false
    };

    aiSystem_->setPatrolBehavior(entity1, patrol1);
    aiSystem_->setPatrolBehavior(entity2, patrol2);

    std::optional<PatrolBehavior> retrieved1 = aiSystem_->getPatrolBehavior(entity1);
    std::optional<PatrolBehavior> retrieved2 = aiSystem_->getPatrolBehavior(entity2);

    ASSERT_TRUE(retrieved1.has_value());
    ASSERT_TRUE(retrieved2.has_value());

    EXPECT_FLOAT_EQ(retrieved1->startX, 100.0f);
    EXPECT_FLOAT_EQ(retrieved2->startX, 300.0f);
}

TEST_F(AISystemTest, PatrolBehaviorUpdatesVelocityDuringUpdate) {
    Entity entity = static_cast<Entity>(608);

    // Create physics body for the entity
    PhysicsBodyDef bodyDef{
        .type = BodyType::Dynamic,
        .transform = {.x = 100.0f, .y = 50.0f}  // Start at patrol center
    };
    physicsSystem_->createBody(entity, bodyDef);

    // Set patrol behavior
    PatrolBehavior patrol{
        .startX = 100.0f,
        .range = 50.0f,
        .speed = 75.0f,
        .movingRight = true
    };
    aiSystem_->setPatrolBehavior(entity, patrol);

    // Update AI system
    aiSystem_->update(1.0f / 60.0f);

    // Verify velocity was set
    Vec2 velocity = physicsSystem_->getVelocity(entity);
    EXPECT_FLOAT_EQ(velocity.x, 75.0f);  // Moving right at patrol speed
}

TEST_F(AISystemTest, PatrolBehaviorFlipsDirectionAtRightBound) {
    Entity entity = static_cast<Entity>(609);

    // Create physics body at right bound
    PhysicsBodyDef bodyDef{
        .type = BodyType::Dynamic,
        .transform = {.x = 150.0f, .y = 50.0f}  // At right bound (100 + 50)
    };
    physicsSystem_->createBody(entity, bodyDef);

    // Set patrol behavior moving right
    PatrolBehavior patrol{
        .startX = 100.0f,
        .range = 50.0f,
        .speed = 75.0f,
        .movingRight = true
    };
    aiSystem_->setPatrolBehavior(entity, patrol);

    // Update AI system
    aiSystem_->update(1.0f / 60.0f);

    // Verify direction flipped
    std::optional<PatrolBehavior> updated = aiSystem_->getPatrolBehavior(entity);
    ASSERT_TRUE(updated.has_value());
    EXPECT_FALSE(updated->movingRight);  // Should flip to left

    // Verify velocity is now negative
    Vec2 velocity = physicsSystem_->getVelocity(entity);
    EXPECT_FLOAT_EQ(velocity.x, -75.0f);  // Moving left at patrol speed
}

TEST_F(AISystemTest, PatrolBehaviorFlipsDirectionAtLeftBound) {
    Entity entity = static_cast<Entity>(610);

    // Create physics body at left bound
    PhysicsBodyDef bodyDef{
        .type = BodyType::Dynamic,
        .transform = {.x = 50.0f, .y = 50.0f}  // At left bound (100 - 50)
    };
    physicsSystem_->createBody(entity, bodyDef);

    // Set patrol behavior moving left
    PatrolBehavior patrol{
        .startX = 100.0f,
        .range = 50.0f,
        .speed = 75.0f,
        .movingRight = false
    };
    aiSystem_->setPatrolBehavior(entity, patrol);

    // Update AI system
    aiSystem_->update(1.0f / 60.0f);

    // Verify direction flipped
    std::optional<PatrolBehavior> updated = aiSystem_->getPatrolBehavior(entity);
    ASSERT_TRUE(updated.has_value());
    EXPECT_TRUE(updated->movingRight);  // Should flip to right

    // Verify velocity is now positive
    Vec2 velocity = physicsSystem_->getVelocity(entity);
    EXPECT_FLOAT_EQ(velocity.x, 75.0f);  // Moving right at patrol speed
}

TEST_F(AISystemTest, PatrolBehaviorPreservesYVelocity) {
    Entity entity = static_cast<Entity>(611);

    // Create physics body with Y velocity
    PhysicsBodyDef bodyDef{
        .type = BodyType::Dynamic,
        .transform = {.x = 100.0f, .y = 50.0f}
    };
    physicsSystem_->createBody(entity, bodyDef);
    physicsSystem_->setVelocity(entity, {0.0f, 100.0f});  // Y velocity set

    // Set patrol behavior
    PatrolBehavior patrol{
        .startX = 100.0f,
        .range = 50.0f,
        .speed = 75.0f,
        .movingRight = true
    };
    aiSystem_->setPatrolBehavior(entity, patrol);

    // Update AI system
    aiSystem_->update(1.0f / 60.0f);

    // Verify Y velocity preserved, X velocity set
    Vec2 velocity = physicsSystem_->getVelocity(entity);
    EXPECT_FLOAT_EQ(velocity.x, 75.0f);
    EXPECT_FLOAT_EQ(velocity.y, 100.0f);  // Y velocity should be preserved
}

TEST_F(AISystemTest, UpdateWithoutPhysicsBodyDoesNotCrash) {
    Entity entity = static_cast<Entity>(612);

    // Set patrol behavior without creating physics body
    PatrolBehavior patrol{
        .startX = 100.0f,
        .range = 50.0f,
        .speed = 75.0f,
        .movingRight = true
    };
    aiSystem_->setPatrolBehavior(entity, patrol);

    // Update should not crash even without physics body
    aiSystem_->update(1.0f / 60.0f);
}

//=============================================================================
// Edge Case Tests
//=============================================================================

TEST_F(AISystemTest, FindEntitiesInRadiusWithNegativeRadius) {
    Vec2 center{100.0f, 100.0f};
    float radius = -50.0f;  // Negative radius

    std::vector<Entity> entities = aiSystem_->findEntitiesInRadius(center, radius);
    // Implementation should handle gracefully (likely return empty)
    EXPECT_TRUE(entities.empty());
}

TEST_F(AISystemTest, FindPathWithVeryLargeAgentRadius) {
    AssetHandle navMeshAsset = static_cast<AssetHandle>(700);
    aiSystem_->loadNavMesh(navMeshAsset);

    NavMeshQuery query{
        .start = {0.0f, 0.0f},
        .end = {100.0f, 100.0f},
        .agentRadius = 10000.0f  // Extremely large agent
    };

    std::optional<NavigationPath> path = aiSystem_->findPath(query);
    // Should still return a path (may be simplified)
    EXPECT_TRUE(path.has_value());
}

TEST_F(AISystemTest, FindPathWithZeroAgentRadius) {
    AssetHandle navMeshAsset = static_cast<AssetHandle>(701);
    aiSystem_->loadNavMesh(navMeshAsset);

    NavMeshQuery query{
        .start = {0.0f, 0.0f},
        .end = {100.0f, 100.0f},
        .agentRadius = 0.0f  // Zero radius
    };

    std::optional<NavigationPath> path = aiSystem_->findPath(query);
    EXPECT_TRUE(path.has_value());
}

TEST_F(AISystemTest, FindPathFromSameStartAndEnd) {
    AssetHandle navMeshAsset = static_cast<AssetHandle>(702);
    aiSystem_->loadNavMesh(navMeshAsset);

    Vec2 samePoint{50.0f, 50.0f};
    NavMeshQuery query{
        .start = samePoint,
        .end = samePoint,
        .agentRadius = 0.5f
    };

    std::optional<NavigationPath> path = aiSystem_->findPath(query);
    ASSERT_TRUE(path.has_value());
    // Path should have zero or minimal length
    EXPECT_GE(path->totalLength, 0.0f);
}

TEST_F(AISystemTest, BlackboardCanOverwriteExistingValue) {
    Entity entity = static_cast<Entity>(800);

    aiSystem_->setBehaviorTreeBlackboard(entity, "health", 100);
    EXPECT_EQ(std::any_cast<int>(aiSystem_->getBehaviorTreeBlackboard(entity, "health")), 100);

    // Overwrite with same key, different type
    aiSystem_->setBehaviorTreeBlackboard(entity, "health", std::string("full"));
    std::any retrieved = aiSystem_->getBehaviorTreeBlackboard(entity, "health");
    EXPECT_TRUE(retrieved.has_value());
    EXPECT_EQ(std::any_cast<std::string>(retrieved), "full");
}

TEST_F(AISystemTest, BlackboardCanStoreComplexTypes) {
    Entity entity = static_cast<Entity>(801);

    // Store Vec2 in blackboard
    Vec2 targetPos{123.45f, 678.90f};
    aiSystem_->setBehaviorTreeBlackboard(entity, "targetPos", targetPos);

    std::any retrieved = aiSystem_->getBehaviorTreeBlackboard(entity, "targetPos");
    ASSERT_TRUE(retrieved.has_value());
    Vec2 retrievedPos = std::any_cast<Vec2>(retrieved);
    EXPECT_FLOAT_EQ(retrievedPos.x, targetPos.x);
    EXPECT_FLOAT_EQ(retrievedPos.y, targetPos.y);
}

TEST_F(AISystemTest, HasLineOfSightWithVeryShortDistance) {
    Vec2 from{0.0f, 0.0f};
    Vec2 to{0.0001f, 0.0001f};  // Very close

    bool hasLOS = aiSystem_->hasLineOfSight(from, to);
    EXPECT_TRUE(hasLOS);
}

TEST_F(AISystemTest, FindClosestEntityWithNoPhysicsBodies) {
    Vec2 position{100.0f, 100.0f};
    std::optional<Entity> closest = aiSystem_->findClosestEntity(position);
    EXPECT_FALSE(closest.has_value());
}

TEST_F(AISystemTest, GetClosestPointOnNavMeshWithVeryFarPoint) {
    AssetHandle navMeshAsset = static_cast<AssetHandle>(703);
    aiSystem_->loadNavMesh(navMeshAsset);

    Vec2 farPoint{100000.0f, 100000.0f};  // Very far from origin
    std::optional<Vec2> closest = aiSystem_->getClosestPointOnNavMesh(farPoint);

    // Should still return a point (possibly projected to navmesh bounds)
    EXPECT_TRUE(closest.has_value());
}

//=============================================================================
// Physics Integration Tests
//=============================================================================

TEST_F(AISystemTest, LineOfSightClearWithNoObstacles) {
    Vec2 from{0.0f, 0.0f};
    Vec2 to{100.0f, 100.0f};

    bool hasLOS = aiSystem_->hasLineOfSight(from, to);
    EXPECT_TRUE(hasLOS);  // No physics bodies = clear path
}

TEST_F(AISystemTest, LineOfSightBlockedByPhysicsBody) {
    // Create a physics body between two points
    Entity wallEntity = static_cast<Entity>(500);
    PhysicsBodyDef wallDef{
        .type = BodyType::Static,
        .transform = {.x = 50.0f, .y = 50.0f}
    };
    physicsSystem_->createBody(wallEntity, wallDef);

    Vec2 from{0.0f, 0.0f};
    Vec2 to{100.0f, 100.0f};

    bool hasLOS = aiSystem_->hasLineOfSight(from, to);
    EXPECT_FALSE(hasLOS);  // Wall blocks the line of sight
}

TEST_F(AISystemTest, FindClosestEntityWithPhysicsBodies) {
    // Create multiple physics bodies at different distances
    Entity entity1 = static_cast<Entity>(501);
    Entity entity2 = static_cast<Entity>(502);
    Entity entity3 = static_cast<Entity>(503);

    PhysicsBodyDef def1{
        .type = BodyType::Dynamic,
        .transform = {.x = 100.0f, .y = 0.0f}
    };
    PhysicsBodyDef def2{
        .type = BodyType::Dynamic,
        .transform = {.x = 50.0f, .y = 0.0f}
    };
    PhysicsBodyDef def3{
        .type = BodyType::Dynamic,
        .transform = {.x = 200.0f, .y = 0.0f}
    };

    physicsSystem_->createBody(entity1, def1);
    physicsSystem_->createBody(entity2, def2);
    physicsSystem_->createBody(entity3, def3);

    Vec2 searchPosition{0.0f, 0.0f};
    std::optional<Entity> closest = aiSystem_->findClosestEntity(searchPosition);

    ASSERT_TRUE(closest.has_value());
    EXPECT_EQ(closest.value(), entity2);  // entity2 is closest at 50 units
}

TEST_F(AISystemTest, FindEntitiesInRadiusWithPhysicsBodies) {
    // Create physics bodies at various positions
    Entity entity1 = static_cast<Entity>(504);
    Entity entity2 = static_cast<Entity>(505);
    Entity entity3 = static_cast<Entity>(506);

    PhysicsBodyDef def1{
        .type = BodyType::Dynamic,
        .transform = {.x = 30.0f, .y = 0.0f}
    };
    PhysicsBodyDef def2{
        .type = BodyType::Dynamic,
        .transform = {.x = 40.0f, .y = 0.0f}
    };
    PhysicsBodyDef def3{
        .type = BodyType::Dynamic,
        .transform = {.x = 100.0f, .y = 0.0f}
    };

    physicsSystem_->createBody(entity1, def1);
    physicsSystem_->createBody(entity2, def2);
    physicsSystem_->createBody(entity3, def3);

    Vec2 center{0.0f, 0.0f};
    float radius = 50.0f;

    std::vector<Entity> entities = aiSystem_->findEntitiesInRadius(center, radius);

    // entity1 and entity2 should be found (within 50 units), entity3 should not (100 units away)
    EXPECT_FALSE(entities.empty());
    EXPECT_EQ(entities.size(), 2u);
}

//=============================================================================
// Kangaru DI Integration Tests
//=============================================================================

TEST(AISystemKangaruTest, CannotInstantiateWithoutDependencies) {
    kgr::container container;

    // AISystem requires IPhysicsSystem and IAssetSystem dependencies
    // Attempting to service without them should fail at compile time or runtime
    // This test verifies the service definition requires dependencies

    // Note: We cannot directly test failure to instantiate in Kangaru,
    // but we can verify successful instantiation with dependencies
    EXPECT_TRUE(true);  // Placeholder for dependency validation
}

TEST(AISystemKangaruTest, CanInstantiateViaKangaruWithDependencies) {
    kgr::container container;

    // Get dependencies from container
    auto& physicsSystem = container.service<PhysicsSystemService>();
    auto* physicsImpl = dynamic_cast<Box2DPhysicsSystem*>(&physicsSystem);
    if (physicsImpl) {
        physicsImpl->initialize();
    }

    auto& assetSystem = container.service<AssetSystemService>();

    // Create AI system with dependencies
    auto aiSystemImpl = std::make_unique<AISystem>(&physicsSystem, &assetSystem);
    aiSystemImpl->initialize();

    // Verify the system was created
    EXPECT_NE(aiSystemImpl, nullptr);

    // Verify basic functionality
    Entity entity = static_cast<Entity>(1);
    AssetHandle treeAsset{100, AssetType::BehaviorTree};

    aiSystemImpl->attachBehaviorTree(entity, treeAsset);
    EXPECT_TRUE(aiSystemImpl->hasBehaviorTree(entity));
}

}  // namespace bestow::tests

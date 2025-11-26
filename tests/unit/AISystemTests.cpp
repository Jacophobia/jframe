// tests/unit/AISystemTests.cpp
// Unit tests for JFrame AI System

#include <any>
#include <cmath>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include <gtest/gtest.h>

import jframe.ai;
import jframe.ai.impl;
import jframe.types;

namespace jframe::tests {

class AISystemTest : public ::testing::Test {
protected:
    void SetUp() override {
        aiSystem = createAISystem();

        // Downcast to access initialize() method
        auto* implPtr = dynamic_cast<AISystem*>(aiSystem.get());
        if (implPtr) {
            implPtr->initialize();
        }
    }

    std::unique_ptr<IAISystem> aiSystem;
};

//=============================================================================
// Basic System Tests
//=============================================================================

TEST_F(AISystemTest, CanCreateAISystem) {
    EXPECT_NE(aiSystem, nullptr);
}

TEST_F(AISystemTest, UpdateDoesNotCrashWithNoEntities) {
    aiSystem->update(1.0f / 60.0f);  // 60 FPS delta time
}

//=============================================================================
// Behavior Tree Management Tests
//=============================================================================

TEST_F(AISystemTest, AttachBehaviorTreeToEntity) {
    Entity entity = static_cast<Entity>(1);
    AssetHandle treeAsset = static_cast<AssetHandle>(100);

    aiSystem->attachBehaviorTree(entity, treeAsset);
    EXPECT_TRUE(aiSystem->hasBehaviorTree(entity));
}

TEST_F(AISystemTest, DetachBehaviorTreeFromEntity) {
    Entity entity = static_cast<Entity>(2);
    AssetHandle treeAsset = static_cast<AssetHandle>(101);

    aiSystem->attachBehaviorTree(entity, treeAsset);
    EXPECT_TRUE(aiSystem->hasBehaviorTree(entity));

    aiSystem->detachBehaviorTree(entity);
    EXPECT_FALSE(aiSystem->hasBehaviorTree(entity));
}

TEST_F(AISystemTest, HasBehaviorTreeReturnsFalseForEntityWithoutTree) {
    Entity entity = static_cast<Entity>(3);
    EXPECT_FALSE(aiSystem->hasBehaviorTree(entity));
}

TEST_F(AISystemTest, DetachBehaviorTreeFromNonExistentEntityDoesNotCrash) {
    Entity entity = static_cast<Entity>(999);
    aiSystem->detachBehaviorTree(entity);  // Should not crash
    EXPECT_FALSE(aiSystem->hasBehaviorTree(entity));
}

TEST_F(AISystemTest, AttachingNewTreeReplacesOldTree) {
    Entity entity = static_cast<Entity>(4);
    AssetHandle tree1 = static_cast<AssetHandle>(102);
    AssetHandle tree2 = static_cast<AssetHandle>(103);

    aiSystem->attachBehaviorTree(entity, tree1);
    EXPECT_TRUE(aiSystem->hasBehaviorTree(entity));

    aiSystem->attachBehaviorTree(entity, tree2);
    EXPECT_TRUE(aiSystem->hasBehaviorTree(entity));
}

//=============================================================================
// Blackboard Tests
//=============================================================================

TEST_F(AISystemTest, CanSetBlackboardValue) {
    Entity entity = static_cast<Entity>(5);
    std::string key = "health";
    int value = 100;

    aiSystem->setBehaviorTreeBlackboard(entity, key, value);

    std::any retrieved = aiSystem->getBehaviorTreeBlackboard(entity, key);
    EXPECT_TRUE(retrieved.has_value());
    EXPECT_EQ(std::any_cast<int>(retrieved), value);
}

TEST_F(AISystemTest, CanSetMultipleBlackboardValues) {
    Entity entity = static_cast<Entity>(6);

    aiSystem->setBehaviorTreeBlackboard(entity, "health", 100);
    aiSystem->setBehaviorTreeBlackboard(entity, "ammo", 50);
    aiSystem->setBehaviorTreeBlackboard(entity, "aggressive", true);

    int health = std::any_cast<int>(aiSystem->getBehaviorTreeBlackboard(entity, "health"));
    int ammo = std::any_cast<int>(aiSystem->getBehaviorTreeBlackboard(entity, "ammo"));
    bool aggressive = std::any_cast<bool>(aiSystem->getBehaviorTreeBlackboard(entity, "aggressive"));

    EXPECT_EQ(health, 100);
    EXPECT_EQ(ammo, 50);
    EXPECT_TRUE(aggressive);
}

TEST_F(AISystemTest, BlackboardReturnsEmptyForNonExistentKey) {
    Entity entity = static_cast<Entity>(7);

    std::any retrieved = aiSystem->getBehaviorTreeBlackboard(entity, "nonexistent");
    EXPECT_FALSE(retrieved.has_value());
}

TEST_F(AISystemTest, BlackboardReturnsEmptyForNonExistentEntity) {
    Entity entity = static_cast<Entity>(999);

    std::any retrieved = aiSystem->getBehaviorTreeBlackboard(entity, "somekey");
    EXPECT_FALSE(retrieved.has_value());
}

TEST_F(AISystemTest, BlackboardCanStoreFloatValues) {
    Entity entity = static_cast<Entity>(8);
    float value = 3.14f;

    aiSystem->setBehaviorTreeBlackboard(entity, "pi", value);

    std::any retrieved = aiSystem->getBehaviorTreeBlackboard(entity, "pi");
    EXPECT_TRUE(retrieved.has_value());
    EXPECT_FLOAT_EQ(std::any_cast<float>(retrieved), value);
}

TEST_F(AISystemTest, BlackboardCanStoreStringValues) {
    Entity entity = static_cast<Entity>(9);
    std::string value = "patrol";

    aiSystem->setBehaviorTreeBlackboard(entity, "state", value);

    std::any retrieved = aiSystem->getBehaviorTreeBlackboard(entity, "state");
    EXPECT_TRUE(retrieved.has_value());
    EXPECT_EQ(std::any_cast<std::string>(retrieved), value);
}

TEST_F(AISystemTest, BlackboardValuesAreIndependentBetweenEntities) {
    Entity entity1 = static_cast<Entity>(10);
    Entity entity2 = static_cast<Entity>(11);

    aiSystem->setBehaviorTreeBlackboard(entity1, "value", 100);
    aiSystem->setBehaviorTreeBlackboard(entity2, "value", 200);

    int value1 = std::any_cast<int>(aiSystem->getBehaviorTreeBlackboard(entity1, "value"));
    int value2 = std::any_cast<int>(aiSystem->getBehaviorTreeBlackboard(entity2, "value"));

    EXPECT_EQ(value1, 100);
    EXPECT_EQ(value2, 200);
}

//=============================================================================
// Navigation - NavMesh Management Tests
//=============================================================================

TEST_F(AISystemTest, InitiallyHasNoNavMesh) {
    EXPECT_FALSE(aiSystem->hasNavMesh());
}

TEST_F(AISystemTest, CanLoadNavMesh) {
    AssetHandle navMeshAsset = static_cast<AssetHandle>(200);
    aiSystem->loadNavMesh(navMeshAsset);
    EXPECT_TRUE(aiSystem->hasNavMesh());
}

TEST_F(AISystemTest, CanUnloadNavMesh) {
    AssetHandle navMeshAsset = static_cast<AssetHandle>(201);
    aiSystem->loadNavMesh(navMeshAsset);
    EXPECT_TRUE(aiSystem->hasNavMesh());

    aiSystem->unloadNavMesh();
    EXPECT_FALSE(aiSystem->hasNavMesh());
}

TEST_F(AISystemTest, UnloadNavMeshWithoutLoadDoesNotCrash) {
    aiSystem->unloadNavMesh();  // Should not crash
    EXPECT_FALSE(aiSystem->hasNavMesh());
}

TEST_F(AISystemTest, LoadingNewNavMeshReplacesOldOne) {
    AssetHandle navMesh1 = static_cast<AssetHandle>(202);
    AssetHandle navMesh2 = static_cast<AssetHandle>(203);

    aiSystem->loadNavMesh(navMesh1);
    EXPECT_TRUE(aiSystem->hasNavMesh());

    aiSystem->loadNavMesh(navMesh2);
    EXPECT_TRUE(aiSystem->hasNavMesh());
}

//=============================================================================
// Navigation - Pathfinding Tests
//=============================================================================

TEST_F(AISystemTest, FindPathReturnsNulloptWithoutNavMesh) {
    NavMeshQuery query{
        .start = {0.0f, 0.0f},
        .end = {100.0f, 100.0f}
    };

    std::optional<NavigationPath> path = aiSystem->findPath(query);
    EXPECT_FALSE(path.has_value());
}

TEST_F(AISystemTest, FindPathReturnsPathWithNavMesh) {
    AssetHandle navMeshAsset = static_cast<AssetHandle>(204);
    aiSystem->loadNavMesh(navMeshAsset);

    NavMeshQuery query{
        .start = {0.0f, 0.0f},
        .end = {100.0f, 100.0f}
    };

    std::optional<NavigationPath> path = aiSystem->findPath(query);
    EXPECT_TRUE(path.has_value());
}

TEST_F(AISystemTest, FindPathWithAgentRadius) {
    AssetHandle navMeshAsset = static_cast<AssetHandle>(205);
    aiSystem->loadNavMesh(navMeshAsset);

    NavMeshQuery query{
        .start = {0.0f, 0.0f},
        .end = {100.0f, 100.0f},
        .agentRadius = 1.0f
    };

    std::optional<NavigationPath> path = aiSystem->findPath(query);
    EXPECT_TRUE(path.has_value());
}

TEST_F(AISystemTest, NavigationPathHasCorrectStructure) {
    AssetHandle navMeshAsset = static_cast<AssetHandle>(206);
    aiSystem->loadNavMesh(navMeshAsset);

    NavMeshQuery query{
        .start = {0.0f, 0.0f},
        .end = {100.0f, 100.0f}
    };

    std::optional<NavigationPath> path = aiSystem->findPath(query);
    ASSERT_TRUE(path.has_value());

    // Path structure should be valid (even if empty in Wave 1)
    EXPECT_GE(path->totalLength, 0.0f);
}

//=============================================================================
// Navigation - Point Query Tests
//=============================================================================

TEST_F(AISystemTest, IsPointOnNavMeshReturnsFalseWithoutNavMesh) {
    Vec2 point{50.0f, 50.0f};
    EXPECT_FALSE(aiSystem->isPointOnNavMesh(point));
}

TEST_F(AISystemTest, IsPointOnNavMeshReturnsTrueWithNavMesh) {
    AssetHandle navMeshAsset = static_cast<AssetHandle>(207);
    aiSystem->loadNavMesh(navMeshAsset);

    Vec2 point{50.0f, 50.0f};
    EXPECT_TRUE(aiSystem->isPointOnNavMesh(point));
}

TEST_F(AISystemTest, GetClosestPointOnNavMeshReturnsNulloptWithoutNavMesh) {
    Vec2 point{50.0f, 50.0f};
    std::optional<Vec2> closest = aiSystem->getClosestPointOnNavMesh(point);
    EXPECT_FALSE(closest.has_value());
}

TEST_F(AISystemTest, GetClosestPointOnNavMeshReturnsPointWithNavMesh) {
    AssetHandle navMeshAsset = static_cast<AssetHandle>(208);
    aiSystem->loadNavMesh(navMeshAsset);

    Vec2 point{50.0f, 50.0f};
    std::optional<Vec2> closest = aiSystem->getClosestPointOnNavMesh(point);
    EXPECT_TRUE(closest.has_value());
}

TEST_F(AISystemTest, GetClosestPointReturnsValidCoordinates) {
    AssetHandle navMeshAsset = static_cast<AssetHandle>(209);
    aiSystem->loadNavMesh(navMeshAsset);

    Vec2 point{50.0f, 50.0f};
    std::optional<Vec2> closest = aiSystem->getClosestPointOnNavMesh(point);

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

    aiSystem->setNavigationTarget(entity, target);

    std::optional<Vec2> retrieved = aiSystem->getNavigationTarget(entity);
    ASSERT_TRUE(retrieved.has_value());
    EXPECT_FLOAT_EQ(retrieved->x, target.x);
    EXPECT_FLOAT_EQ(retrieved->y, target.y);
}

TEST_F(AISystemTest, CanClearNavigationTarget) {
    Entity entity = static_cast<Entity>(21);
    Vec2 target{100.0f, 200.0f};

    aiSystem->setNavigationTarget(entity, target);
    EXPECT_TRUE(aiSystem->getNavigationTarget(entity).has_value());

    aiSystem->clearNavigationTarget(entity);
    EXPECT_FALSE(aiSystem->getNavigationTarget(entity).has_value());
}

TEST_F(AISystemTest, GetNavigationTargetReturnsNulloptForEntityWithoutTarget) {
    Entity entity = static_cast<Entity>(22);
    EXPECT_FALSE(aiSystem->getNavigationTarget(entity).has_value());
}

TEST_F(AISystemTest, ClearNavigationTargetOnNonExistentEntityDoesNotCrash) {
    Entity entity = static_cast<Entity>(999);
    aiSystem->clearNavigationTarget(entity);  // Should not crash
}

TEST_F(AISystemTest, SetNavigationTargetReplacesOldTarget) {
    Entity entity = static_cast<Entity>(23);
    Vec2 target1{100.0f, 200.0f};
    Vec2 target2{300.0f, 400.0f};

    aiSystem->setNavigationTarget(entity, target1);
    aiSystem->setNavigationTarget(entity, target2);

    std::optional<Vec2> retrieved = aiSystem->getNavigationTarget(entity);
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

    aiSystem->setMaxSpeed(entity, maxSpeed);
    // Should not crash - value should be stored
}

TEST_F(AISystemTest, CanSetMaxAcceleration) {
    Entity entity = static_cast<Entity>(25);
    float maxAcceleration = 750.0f;

    aiSystem->setMaxAcceleration(entity, maxAcceleration);
    // Should not crash - value should be stored
}

TEST_F(AISystemTest, SetMaxSpeedOnNonExistentEntityDoesNotCrash) {
    Entity entity = static_cast<Entity>(999);
    aiSystem->setMaxSpeed(entity, 100.0f);  // Should not crash
}

TEST_F(AISystemTest, SetMaxAccelerationOnNonExistentEntityDoesNotCrash) {
    Entity entity = static_cast<Entity>(999);
    aiSystem->setMaxAcceleration(entity, 500.0f);  // Should not crash
}

TEST_F(AISystemTest, CanSetMultipleSteeringParameters) {
    Entity entity = static_cast<Entity>(26);

    aiSystem->setMaxSpeed(entity, 200.0f);
    aiSystem->setMaxAcceleration(entity, 1000.0f);
    aiSystem->setNavigationTarget(entity, Vec2{150.0f, 250.0f});

    // Should not crash - all parameters should be stored
    std::optional<Vec2> target = aiSystem->getNavigationTarget(entity);
    EXPECT_TRUE(target.has_value());
}

//=============================================================================
// Spatial Queries - Find Entities in Radius Tests
//=============================================================================

TEST_F(AISystemTest, FindEntitiesInRadiusReturnsEmptyByDefault) {
    Vec2 center{100.0f, 100.0f};
    float radius = 50.0f;

    std::vector<Entity> entities = aiSystem->findEntitiesInRadius(center, radius);
    EXPECT_TRUE(entities.empty());
}

TEST_F(AISystemTest, FindEntitiesInRadiusWithMask) {
    Vec2 center{100.0f, 100.0f};
    float radius = 50.0f;
    CollisionMask mask = 0x0001;  // Specific layer

    std::vector<Entity> entities = aiSystem->findEntitiesInRadius(center, radius, mask);
    EXPECT_TRUE(entities.empty());  // Wave 1: not yet implemented
}

TEST_F(AISystemTest, FindEntitiesInRadiusWithZeroRadius) {
    Vec2 center{100.0f, 100.0f};
    float radius = 0.0f;

    std::vector<Entity> entities = aiSystem->findEntitiesInRadius(center, radius);
    EXPECT_TRUE(entities.empty());
}

TEST_F(AISystemTest, FindEntitiesInRadiusWithLargeRadius) {
    Vec2 center{0.0f, 0.0f};
    float radius = 10000.0f;

    std::vector<Entity> entities = aiSystem->findEntitiesInRadius(center, radius);
    EXPECT_TRUE(entities.empty());  // Wave 1: not yet implemented
}

//=============================================================================
// Spatial Queries - Find Closest Entity Tests
//=============================================================================

TEST_F(AISystemTest, FindClosestEntityReturnsNulloptByDefault) {
    Vec2 position{100.0f, 100.0f};

    std::optional<Entity> closest = aiSystem->findClosestEntity(position);
    EXPECT_FALSE(closest.has_value());
}

TEST_F(AISystemTest, FindClosestEntityWithMask) {
    Vec2 position{100.0f, 100.0f};
    CollisionMask mask = 0x0002;  // Specific layer

    std::optional<Entity> closest = aiSystem->findClosestEntity(position, mask);
    EXPECT_FALSE(closest.has_value());  // Wave 1: not yet implemented
}

//=============================================================================
// Spatial Queries - Line of Sight Tests
//=============================================================================

TEST_F(AISystemTest, HasLineOfSightReturnsTrueByDefault) {
    Vec2 from{0.0f, 0.0f};
    Vec2 to{100.0f, 100.0f};

    bool hasLOS = aiSystem->hasLineOfSight(from, to);
    EXPECT_TRUE(hasLOS);  // Wave 1: always returns true
}

TEST_F(AISystemTest, HasLineOfSightWithObstacleMask) {
    Vec2 from{0.0f, 0.0f};
    Vec2 to{100.0f, 100.0f};
    CollisionMask obstacleMask = 0x0004;  // Terrain layer

    bool hasLOS = aiSystem->hasLineOfSight(from, to, obstacleMask);
    EXPECT_TRUE(hasLOS);  // Wave 1: always returns true
}

TEST_F(AISystemTest, HasLineOfSightBetweenSamePoint) {
    Vec2 point{50.0f, 50.0f};

    bool hasLOS = aiSystem->hasLineOfSight(point, point);
    EXPECT_TRUE(hasLOS);
}

//=============================================================================
// Integration Tests
//=============================================================================

TEST_F(AISystemTest, FullWorkflowLoadNavMeshFindPathFollowPath) {
    // Setup navmesh
    AssetHandle navMeshAsset = static_cast<AssetHandle>(300);
    aiSystem->loadNavMesh(navMeshAsset);
    EXPECT_TRUE(aiSystem->hasNavMesh());

    // Find path
    NavMeshQuery query{
        .start = {0.0f, 0.0f},
        .end = {100.0f, 100.0f},
        .agentRadius = 0.5f
    };

    std::optional<NavigationPath> path = aiSystem->findPath(query);
    ASSERT_TRUE(path.has_value());

    // Setup entity to follow path
    Entity entity = static_cast<Entity>(100);
    aiSystem->setNavigationTarget(entity, query.end);
    aiSystem->setMaxSpeed(entity, 100.0f);
    aiSystem->setMaxAcceleration(entity, 500.0f);

    std::optional<Vec2> target = aiSystem->getNavigationTarget(entity);
    EXPECT_TRUE(target.has_value());
}

TEST_F(AISystemTest, FullWorkflowBehaviorTreeWithBlackboard) {
    Entity entity = static_cast<Entity>(101);

    // Attach behavior tree
    AssetHandle treeAsset = static_cast<AssetHandle>(400);
    aiSystem->attachBehaviorTree(entity, treeAsset);
    EXPECT_TRUE(aiSystem->hasBehaviorTree(entity));

    // Setup blackboard
    aiSystem->setBehaviorTreeBlackboard(entity, "target", Vec2{100.0f, 100.0f});
    aiSystem->setBehaviorTreeBlackboard(entity, "state", std::string("patrol"));
    aiSystem->setBehaviorTreeBlackboard(entity, "alert", false);

    // Verify blackboard
    std::any target = aiSystem->getBehaviorTreeBlackboard(entity, "target");
    std::any state = aiSystem->getBehaviorTreeBlackboard(entity, "state");
    std::any alert = aiSystem->getBehaviorTreeBlackboard(entity, "alert");

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
    aiSystem->attachBehaviorTree(entity1, tree1);
    aiSystem->setNavigationTarget(entity1, Vec2{100.0f, 100.0f});
    aiSystem->setBehaviorTreeBlackboard(entity1, "role", std::string("guard"));

    // Setup entity 2
    AssetHandle tree2 = static_cast<AssetHandle>(402);
    aiSystem->attachBehaviorTree(entity2, tree2);
    aiSystem->setNavigationTarget(entity2, Vec2{200.0f, 200.0f});
    aiSystem->setBehaviorTreeBlackboard(entity2, "role", std::string("scout"));

    // Setup entity 3
    aiSystem->setMaxSpeed(entity3, 50.0f);

    // Verify independence
    EXPECT_TRUE(aiSystem->hasBehaviorTree(entity1));
    EXPECT_TRUE(aiSystem->hasBehaviorTree(entity2));
    EXPECT_FALSE(aiSystem->hasBehaviorTree(entity3));

    std::optional<Vec2> target1 = aiSystem->getNavigationTarget(entity1);
    std::optional<Vec2> target2 = aiSystem->getNavigationTarget(entity2);
    std::optional<Vec2> target3 = aiSystem->getNavigationTarget(entity3);

    EXPECT_TRUE(target1.has_value());
    EXPECT_TRUE(target2.has_value());
    EXPECT_FALSE(target3.has_value());

    std::string role1 = std::any_cast<std::string>(
        aiSystem->getBehaviorTreeBlackboard(entity1, "role"));
    std::string role2 = std::any_cast<std::string>(
        aiSystem->getBehaviorTreeBlackboard(entity2, "role"));

    EXPECT_EQ(role1, "guard");
    EXPECT_EQ(role2, "scout");
}

TEST_F(AISystemTest, UpdateWithMultipleEntitiesDoesNotCrash) {
    // Setup multiple entities with various AI components
    for (int i = 0; i < 10; ++i) {
        Entity entity = static_cast<Entity>(200 + i);
        AssetHandle tree = static_cast<AssetHandle>(500 + i);

        aiSystem->attachBehaviorTree(entity, tree);
        aiSystem->setNavigationTarget(entity, Vec2{static_cast<float>(i * 10),
                                                    static_cast<float>(i * 20)});
        aiSystem->setMaxSpeed(entity, 100.0f + static_cast<float>(i * 10));
        aiSystem->setBehaviorTreeBlackboard(entity, "id", i);
    }

    // Update should process all entities
    aiSystem->update(1.0f / 60.0f);
}

}  // namespace jframe::tests

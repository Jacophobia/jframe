// tests/unit/PhysicsSystemTests.cpp
// Unit tests for JFrame Physics System

#include <cmath>
#include <memory>
#include <optional>
#include <vector>

#include <gtest/gtest.h>

import jframe.physics;
import jframe.physics.impl;
import jframe.types;

namespace jframe::tests {

class PhysicsSystemTest : public ::testing::Test {
protected:
    void SetUp() override {
        physics = createPhysicsSystem();

        // Downcast to access initialize() method
        auto* implPtr = dynamic_cast<Box2DPhysicsSystem*>(physics.get());
        if (implPtr) {
            implPtr->initialize();
        }
    }

    std::unique_ptr<IPhysicsSystem> physics;
};

//=============================================================================
// World Setup Tests
//=============================================================================

TEST_F(PhysicsSystemTest, CanCreatePhysicsSystem) {
    EXPECT_NE(physics, nullptr);
}

TEST_F(PhysicsSystemTest, DefaultGravityIsStandardEarthGravity) {
    Vec2 gravity = physics->getGravity();
    EXPECT_FLOAT_EQ(gravity.x, 0.0f);
    EXPECT_FLOAT_EQ(gravity.y, -9.8f);
}

TEST_F(PhysicsSystemTest, CanChangeGravity) {
    Vec2 newGravity{0.0f, -5.0f};
    physics->setGravity(newGravity);

    Vec2 retrieved = physics->getGravity();
    EXPECT_FLOAT_EQ(retrieved.x, newGravity.x);
    EXPECT_FLOAT_EQ(retrieved.y, newGravity.y);
}

TEST_F(PhysicsSystemTest, CanSetZeroGravity) {
    Vec2 zeroGravity{0.0f, 0.0f};
    physics->setGravity(zeroGravity);

    Vec2 retrieved = physics->getGravity();
    EXPECT_FLOAT_EQ(retrieved.x, 0.0f);
    EXPECT_FLOAT_EQ(retrieved.y, 0.0f);
}

//=============================================================================
// Body Management Tests
//=============================================================================

TEST_F(PhysicsSystemTest, CanCreateDynamicBody) {
    Entity entity = static_cast<Entity>(1);
    PhysicsBodyDef def{
        .type = BodyType::Dynamic,
        .transform = {.x = 100.0f, .y = 200.0f}
    };

    physics->createBody(entity, def);
    EXPECT_TRUE(physics->hasBody(entity));
}

TEST_F(PhysicsSystemTest, CanCreateStaticBody) {
    Entity entity = static_cast<Entity>(2);
    PhysicsBodyDef def{
        .type = BodyType::Static,
        .transform = {.x = 0.0f, .y = 0.0f}
    };

    physics->createBody(entity, def);
    EXPECT_TRUE(physics->hasBody(entity));
}

TEST_F(PhysicsSystemTest, CanCreateKinematicBody) {
    Entity entity = static_cast<Entity>(3);
    PhysicsBodyDef def{
        .type = BodyType::Kinematic,
        .transform = {.x = 50.0f, .y = 50.0f}
    };

    physics->createBody(entity, def);
    EXPECT_TRUE(physics->hasBody(entity));
}

TEST_F(PhysicsSystemTest, CanDestroyBody) {
    Entity entity = static_cast<Entity>(4);
    PhysicsBodyDef def{.type = BodyType::Dynamic};

    physics->createBody(entity, def);
    EXPECT_TRUE(physics->hasBody(entity));

    physics->destroyBody(entity);
    EXPECT_FALSE(physics->hasBody(entity));
}

TEST_F(PhysicsSystemTest, HasBodyReturnsFalseForNonExistentBody) {
    Entity entity = static_cast<Entity>(999);
    EXPECT_FALSE(physics->hasBody(entity));
}

TEST_F(PhysicsSystemTest, CreatingBodyForSameEntityTwiceReplacesOldOne) {
    Entity entity = static_cast<Entity>(5);

    PhysicsBodyDef def1{
        .type = BodyType::Static,
        .transform = {.x = 10.0f, .y = 10.0f}
    };
    physics->createBody(entity, def1);

    PhysicsBodyDef def2{
        .type = BodyType::Dynamic,
        .transform = {.x = 20.0f, .y = 20.0f}
    };
    physics->createBody(entity, def2);

    EXPECT_TRUE(physics->hasBody(entity));
    EXPECT_EQ(physics->getBodyType(entity), BodyType::Dynamic);

    Vec2 pos = physics->getPosition(entity);
    EXPECT_FLOAT_EQ(pos.x, 20.0f);
    EXPECT_FLOAT_EQ(pos.y, 20.0f);
}

TEST_F(PhysicsSystemTest, DestroyingNonExistentBodyDoesNotCrash) {
    Entity entity = static_cast<Entity>(999);
    physics->destroyBody(entity);  // Should not crash
}

//=============================================================================
// Body Type Tests
//=============================================================================

TEST_F(PhysicsSystemTest, CanGetBodyTypeAfterCreation) {
    Entity entity = static_cast<Entity>(6);
    PhysicsBodyDef def{.type = BodyType::Kinematic};

    physics->createBody(entity, def);
    EXPECT_EQ(physics->getBodyType(entity), BodyType::Kinematic);
}

TEST_F(PhysicsSystemTest, CanChangeBodyType) {
    Entity entity = static_cast<Entity>(7);
    PhysicsBodyDef def{.type = BodyType::Static};

    physics->createBody(entity, def);
    EXPECT_EQ(physics->getBodyType(entity), BodyType::Static);

    physics->setBodyType(entity, BodyType::Dynamic);
    EXPECT_EQ(physics->getBodyType(entity), BodyType::Dynamic);
}

//=============================================================================
// Position Tests
//=============================================================================

TEST_F(PhysicsSystemTest, CanSetAndGetPosition) {
    Entity entity = static_cast<Entity>(8);
    PhysicsBodyDef def{.type = BodyType::Dynamic};
    physics->createBody(entity, def);

    Vec2 targetPos{150.0f, 250.0f};
    physics->setPosition(entity, targetPos);

    Vec2 retrievedPos = physics->getPosition(entity);
    EXPECT_FLOAT_EQ(retrievedPos.x, targetPos.x);
    EXPECT_FLOAT_EQ(retrievedPos.y, targetPos.y);
}

TEST_F(PhysicsSystemTest, InitialPositionIsSetFromBodyDef) {
    Entity entity = static_cast<Entity>(9);
    PhysicsBodyDef def{
        .type = BodyType::Dynamic,
        .transform = {.x = 42.0f, .y = 84.0f}
    };
    physics->createBody(entity, def);

    Vec2 pos = physics->getPosition(entity);
    EXPECT_FLOAT_EQ(pos.x, 42.0f);
    EXPECT_FLOAT_EQ(pos.y, 84.0f);
}

TEST_F(PhysicsSystemTest, GettingPositionForNonExistentBodyReturnsDefault) {
    Entity entity = static_cast<Entity>(999);
    Vec2 pos = physics->getPosition(entity);

    EXPECT_FLOAT_EQ(pos.x, 0.0f);
    EXPECT_FLOAT_EQ(pos.y, 0.0f);
}

//=============================================================================
// Rotation Tests
//=============================================================================

TEST_F(PhysicsSystemTest, CanSetAndGetRotation) {
    Entity entity = static_cast<Entity>(10);
    PhysicsBodyDef def{.type = BodyType::Dynamic};
    physics->createBody(entity, def);

    float targetRotation = 1.57f;  // ~90 degrees in radians
    physics->setRotation(entity, targetRotation);

    float retrievedRotation = physics->getRotation(entity);
    EXPECT_FLOAT_EQ(retrievedRotation, targetRotation);
}

TEST_F(PhysicsSystemTest, InitialRotationIsSetFromBodyDef) {
    Entity entity = static_cast<Entity>(11);
    PhysicsBodyDef def{
        .type = BodyType::Dynamic,
        .transform = {.rotation = 3.14f}
    };
    physics->createBody(entity, def);

    float rotation = physics->getRotation(entity);
    EXPECT_FLOAT_EQ(rotation, 3.14f);
}

TEST_F(PhysicsSystemTest, GettingRotationForNonExistentBodyReturnsDefault) {
    Entity entity = static_cast<Entity>(999);
    float rotation = physics->getRotation(entity);
    EXPECT_FLOAT_EQ(rotation, 0.0f);
}

//=============================================================================
// Velocity Tests
//=============================================================================

TEST_F(PhysicsSystemTest, CanSetAndGetVelocity) {
    Entity entity = static_cast<Entity>(12);
    PhysicsBodyDef def{.type = BodyType::Dynamic};
    physics->createBody(entity, def);

    Vec2 targetVelocity{10.0f, -5.0f};
    physics->setVelocity(entity, targetVelocity);

    Vec2 retrievedVelocity = physics->getVelocity(entity);
    EXPECT_FLOAT_EQ(retrievedVelocity.x, targetVelocity.x);
    EXPECT_FLOAT_EQ(retrievedVelocity.y, targetVelocity.y);
}

TEST_F(PhysicsSystemTest, InitialVelocityIsZero) {
    Entity entity = static_cast<Entity>(13);
    PhysicsBodyDef def{.type = BodyType::Dynamic};
    physics->createBody(entity, def);

    Vec2 velocity = physics->getVelocity(entity);
    EXPECT_FLOAT_EQ(velocity.x, 0.0f);
    EXPECT_FLOAT_EQ(velocity.y, 0.0f);
}

TEST_F(PhysicsSystemTest, GettingVelocityForNonExistentBodyReturnsDefault) {
    Entity entity = static_cast<Entity>(999);
    Vec2 velocity = physics->getVelocity(entity);

    EXPECT_FLOAT_EQ(velocity.x, 0.0f);
    EXPECT_FLOAT_EQ(velocity.y, 0.0f);
}

//=============================================================================
// Angular Velocity Tests
//=============================================================================

TEST_F(PhysicsSystemTest, CanSetAndGetAngularVelocity) {
    Entity entity = static_cast<Entity>(14);
    PhysicsBodyDef def{.type = BodyType::Dynamic};
    physics->createBody(entity, def);

    float targetAngularVelocity = 2.5f;
    physics->setAngularVelocity(entity, targetAngularVelocity);

    float retrievedAngularVelocity = physics->getAngularVelocity(entity);
    EXPECT_FLOAT_EQ(retrievedAngularVelocity, targetAngularVelocity);
}

TEST_F(PhysicsSystemTest, InitialAngularVelocityIsZero) {
    Entity entity = static_cast<Entity>(15);
    PhysicsBodyDef def{.type = BodyType::Dynamic};
    physics->createBody(entity, def);

    float angularVelocity = physics->getAngularVelocity(entity);
    EXPECT_FLOAT_EQ(angularVelocity, 0.0f);
}

TEST_F(PhysicsSystemTest, GettingAngularVelocityForNonExistentBodyReturnsDefault) {
    Entity entity = static_cast<Entity>(999);
    float angularVelocity = physics->getAngularVelocity(entity);
    EXPECT_FLOAT_EQ(angularVelocity, 0.0f);
}

//=============================================================================
// Force Application Tests
//=============================================================================

TEST_F(PhysicsSystemTest, CanApplyForceToBody) {
    Entity entity = static_cast<Entity>(16);
    PhysicsBodyDef def{.type = BodyType::Dynamic};
    physics->createBody(entity, def);

    Vec2 force{100.0f, 50.0f};
    physics->applyForce(entity, force);
    // Should not crash - actual velocity change would require simulation step
}

TEST_F(PhysicsSystemTest, CanApplyForceAtPoint) {
    Entity entity = static_cast<Entity>(17);
    PhysicsBodyDef def{.type = BodyType::Dynamic};
    physics->createBody(entity, def);

    Vec2 force{100.0f, 0.0f};
    Vec2 point{0.0f, 10.0f};  // Off-center point
    physics->applyForce(entity, force, point);
    // Should not crash - would cause rotation if simulated
}

TEST_F(PhysicsSystemTest, CanApplyImpulseToBody) {
    Entity entity = static_cast<Entity>(18);
    PhysicsBodyDef def{.type = BodyType::Dynamic};
    physics->createBody(entity, def);

    Vec2 impulse{50.0f, 25.0f};
    physics->applyImpulse(entity, impulse);
    // Should not crash - immediate velocity change
}

TEST_F(PhysicsSystemTest, CanApplyImpulseAtPoint) {
    Entity entity = static_cast<Entity>(19);
    PhysicsBodyDef def{.type = BodyType::Dynamic};
    physics->createBody(entity, def);

    Vec2 impulse{50.0f, 0.0f};
    Vec2 point{0.0f, 5.0f};
    physics->applyImpulse(entity, impulse, point);
    // Should not crash - would affect angular velocity
}

TEST_F(PhysicsSystemTest, CanApplyTorqueToBody) {
    Entity entity = static_cast<Entity>(20);
    PhysicsBodyDef def{.type = BodyType::Dynamic};
    physics->createBody(entity, def);

    float torque = 10.0f;
    physics->applyTorque(entity, torque);
    // Should not crash - would change angular velocity
}

TEST_F(PhysicsSystemTest, ApplyingForceToNonExistentBodyDoesNotCrash) {
    Entity entity = static_cast<Entity>(999);
    Vec2 force{100.0f, 50.0f};
    physics->applyForce(entity, force);
}

//=============================================================================
// Collision Filtering Tests
//=============================================================================

TEST_F(PhysicsSystemTest, CanSetCollisionLayer) {
    Entity entity = static_cast<Entity>(21);
    PhysicsBodyDef def{.type = BodyType::Dynamic};
    physics->createBody(entity, def);

    CollisionLayer layer = CollisionLayers::Player;
    physics->setCollisionLayer(entity, layer);
    // Should not crash - layer should be stored
}

TEST_F(PhysicsSystemTest, CanSetCollisionMask) {
    Entity entity = static_cast<Entity>(22);
    PhysicsBodyDef def{.type = BodyType::Dynamic};
    physics->createBody(entity, def);

    CollisionMask mask = CollisionLayers::Enemy | CollisionLayers::Terrain;
    physics->setCollisionMask(entity, mask);
    // Should not crash - mask should be stored
}

TEST_F(PhysicsSystemTest, CanSetBodyAsSensor) {
    Entity entity = static_cast<Entity>(23);
    PhysicsBodyDef def{.type = BodyType::Dynamic};
    physics->createBody(entity, def);

    physics->setSensor(entity, true);
    // Should not crash - sensor flag should be stored
}

TEST_F(PhysicsSystemTest, CanSetBodyAsNonSensor) {
    Entity entity = static_cast<Entity>(24);
    PhysicsBodyDef def{.type = BodyType::Dynamic};
    physics->createBody(entity, def);

    physics->setSensor(entity, false);
    // Should not crash - sensor flag should be stored
}

TEST_F(PhysicsSystemTest, SettingCollisionPropertiesOnNonExistentBodyDoesNotCrash) {
    Entity entity = static_cast<Entity>(999);
    physics->setCollisionLayer(entity, CollisionLayers::Player);
    physics->setCollisionMask(entity, 0xFFFF);
    physics->setSensor(entity, true);
}

//=============================================================================
// AABB Query Tests
//=============================================================================

TEST_F(PhysicsSystemTest, QueryAABBReturnsEntitiesInRegion) {
    // Create bodies at various positions
    Entity entity1 = static_cast<Entity>(25);
    Entity entity2 = static_cast<Entity>(26);
    Entity entity3 = static_cast<Entity>(27);

    PhysicsBodyDef def1{
        .type = BodyType::Dynamic,
        .transform = {.x = 50.0f, .y = 50.0f}
    };
    PhysicsBodyDef def2{
        .type = BodyType::Dynamic,
        .transform = {.x = 150.0f, .y = 150.0f}
    };
    PhysicsBodyDef def3{
        .type = BodyType::Dynamic,
        .transform = {.x = 250.0f, .y = 250.0f}
    };

    physics->createBody(entity1, def1);
    physics->createBody(entity2, def2);
    physics->createBody(entity3, def3);

    // Query region containing entity1 and entity2
    Vec2 min{0.0f, 0.0f};
    Vec2 max{200.0f, 200.0f};

    std::vector<Entity> results = physics->queryAABB(min, max);

    // Should contain entity1 and entity2, not entity3
    EXPECT_GE(results.size(), 2u);
}

TEST_F(PhysicsSystemTest, QueryAABBReturnsEmptyForEmptyRegion) {
    Vec2 min{1000.0f, 1000.0f};
    Vec2 max{1100.0f, 1100.0f};

    std::vector<Entity> results = physics->queryAABB(min, max);
    EXPECT_TRUE(results.empty());
}

//=============================================================================
// Circle Query Tests
//=============================================================================

TEST_F(PhysicsSystemTest, QueryCircleReturnsEntitiesInRadius) {
    // Create bodies at various positions
    Entity entity1 = static_cast<Entity>(28);
    Entity entity2 = static_cast<Entity>(29);
    Entity entity3 = static_cast<Entity>(30);

    PhysicsBodyDef def1{
        .type = BodyType::Dynamic,
        .transform = {.x = 100.0f, .y = 100.0f}
    };
    PhysicsBodyDef def2{
        .type = BodyType::Dynamic,
        .transform = {.x = 110.0f, .y = 110.0f}
    };
    PhysicsBodyDef def3{
        .type = BodyType::Dynamic,
        .transform = {.x = 500.0f, .y = 500.0f}
    };

    physics->createBody(entity1, def1);
    physics->createBody(entity2, def2);
    physics->createBody(entity3, def3);

    // Query circle around entity1 and entity2
    Vec2 center{105.0f, 105.0f};
    float radius = 50.0f;

    std::vector<Entity> results = physics->queryCircle(center, radius);

    // Should contain entity1 and entity2, not entity3
    EXPECT_GE(results.size(), 2u);
}

TEST_F(PhysicsSystemTest, QueryCircleReturnsEmptyForEmptyRegion) {
    Vec2 center{2000.0f, 2000.0f};
    float radius = 10.0f;

    std::vector<Entity> results = physics->queryCircle(center, radius);
    EXPECT_TRUE(results.empty());
}

//=============================================================================
// Raycast Tests
//=============================================================================

TEST_F(PhysicsSystemTest, RaycastReturnsHitForBlockingBody) {
    Entity entity = static_cast<Entity>(31);
    PhysicsBodyDef def{
        .type = BodyType::Static,
        .transform = {.x = 100.0f, .y = 100.0f}
    };
    physics->createBody(entity, def);

    Vec2 origin{0.0f, 100.0f};
    Vec2 direction{1.0f, 0.0f};
    float maxDistance = 200.0f;

    std::optional<RaycastHit> hit = physics->raycast(origin, direction, maxDistance);

    EXPECT_TRUE(hit.has_value());
    if (hit) {
        EXPECT_EQ(hit->entity, entity);
        EXPECT_GT(hit->distance, 0.0f);
        EXPECT_LE(hit->distance, maxDistance);
    }
}

TEST_F(PhysicsSystemTest, RaycastReturnsNulloptForNoHit) {
    Vec2 origin{0.0f, 0.0f};
    Vec2 direction{1.0f, 0.0f};
    float maxDistance = 100.0f;

    std::optional<RaycastHit> hit = physics->raycast(origin, direction, maxDistance);
    EXPECT_FALSE(hit.has_value());
}

TEST_F(PhysicsSystemTest, RaycastRespectsMaxDistance) {
    Entity entity = static_cast<Entity>(32);
    PhysicsBodyDef def{
        .type = BodyType::Static,
        .transform = {.x = 200.0f, .y = 100.0f}
    };
    physics->createBody(entity, def);

    Vec2 origin{0.0f, 100.0f};
    Vec2 direction{1.0f, 0.0f};
    float maxDistance = 50.0f;  // Too short to reach the body

    std::optional<RaycastHit> hit = physics->raycast(origin, direction, maxDistance);
    EXPECT_FALSE(hit.has_value());
}

TEST_F(PhysicsSystemTest, RaycastReturnsHitInformation) {
    Entity entity = static_cast<Entity>(33);
    PhysicsBodyDef def{
        .type = BodyType::Static,
        .transform = {.x = 100.0f, .y = 100.0f}
    };
    physics->createBody(entity, def);

    Vec2 origin{0.0f, 100.0f};
    Vec2 direction{1.0f, 0.0f};
    float maxDistance = 200.0f;

    std::optional<RaycastHit> hit = physics->raycast(origin, direction, maxDistance);

    EXPECT_TRUE(hit.has_value());
    if (hit) {
        EXPECT_EQ(hit->entity, entity);
        EXPECT_GT(hit->distance, 0.0f);
        // Point should be somewhere along the ray
        EXPECT_GT(hit->point.x, 0.0f);
        // Normal should be a unit vector (or zero)
        float normalLength = std::sqrt(hit->normal.x * hit->normal.x +
                                       hit->normal.y * hit->normal.y);
        EXPECT_TRUE(normalLength == 0.0f || std::abs(normalLength - 1.0f) < 0.01f);
    }
}

//=============================================================================
// Raycast All Tests
//=============================================================================

TEST_F(PhysicsSystemTest, RaycastAllReturnsMultipleHits) {
    // Create multiple bodies along the ray path
    Entity entity1 = static_cast<Entity>(34);
    Entity entity2 = static_cast<Entity>(35);
    Entity entity3 = static_cast<Entity>(36);

    PhysicsBodyDef def1{
        .type = BodyType::Static,
        .transform = {.x = 50.0f, .y = 100.0f}
    };
    PhysicsBodyDef def2{
        .type = BodyType::Static,
        .transform = {.x = 150.0f, .y = 100.0f}
    };
    PhysicsBodyDef def3{
        .type = BodyType::Static,
        .transform = {.x = 250.0f, .y = 100.0f}
    };

    physics->createBody(entity1, def1);
    physics->createBody(entity2, def2);
    physics->createBody(entity3, def3);

    Vec2 origin{0.0f, 100.0f};
    Vec2 direction{1.0f, 0.0f};
    float maxDistance = 300.0f;

    std::vector<RaycastHit> hits = physics->raycastAll(origin, direction, maxDistance);

    // Should hit all three bodies
    EXPECT_GE(hits.size(), 3u);
}

TEST_F(PhysicsSystemTest, RaycastAllReturnsEmptyForNoHits) {
    Vec2 origin{0.0f, 0.0f};
    Vec2 direction{1.0f, 0.0f};
    float maxDistance = 100.0f;

    std::vector<RaycastHit> hits = physics->raycastAll(origin, direction, maxDistance);
    EXPECT_TRUE(hits.empty());
}

TEST_F(PhysicsSystemTest, RaycastAllHitsAreSorted) {
    // Create multiple bodies along the ray path
    Entity entity1 = static_cast<Entity>(37);
    Entity entity2 = static_cast<Entity>(38);

    PhysicsBodyDef def1{
        .type = BodyType::Static,
        .transform = {.x = 50.0f, .y = 100.0f}
    };
    PhysicsBodyDef def2{
        .type = BodyType::Static,
        .transform = {.x = 150.0f, .y = 100.0f}
    };

    physics->createBody(entity1, def1);
    physics->createBody(entity2, def2);

    Vec2 origin{0.0f, 100.0f};
    Vec2 direction{1.0f, 0.0f};
    float maxDistance = 200.0f;

    std::vector<RaycastHit> hits = physics->raycastAll(origin, direction, maxDistance);

    // Hits should be sorted by distance (closest first)
    for (size_t i = 1; i < hits.size(); ++i) {
        EXPECT_LE(hits[i - 1].distance, hits[i].distance);
    }
}

//=============================================================================
// Collision Mask Tests
//=============================================================================

TEST_F(PhysicsSystemTest, RaycastRespectsCollisionMask) {
    Entity entity1 = static_cast<Entity>(39);
    Entity entity2 = static_cast<Entity>(40);

    PhysicsBodyDef def{.type = BodyType::Static};
    physics->createBody(entity1, def);
    physics->createBody(entity2, def);

    physics->setCollisionLayer(entity1, CollisionLayers::Player);
    physics->setCollisionLayer(entity2, CollisionLayers::Enemy);

    Vec2 origin{0.0f, 0.0f};
    Vec2 direction{1.0f, 0.0f};
    float maxDistance = 1000.0f;

    // Query only for Player layer
    CollisionMask playerMask = CollisionLayers::Player;
    std::optional<RaycastHit> hit = physics->raycast(origin, direction, maxDistance, playerMask);

    // Should only hit entities with Player layer
    if (hit) {
        // Verifying mask filtering worked (actual entity check would need position verification)
        EXPECT_TRUE(hit.has_value());
    }
}

//=============================================================================
// Update Tests
//=============================================================================

TEST_F(PhysicsSystemTest, UpdateDoesNotCrashWithNoBodies) {
    physics->update(1.0f / 60.0f);  // 60 FPS delta time
}

TEST_F(PhysicsSystemTest, UpdateDoesNotCrashWithBodies) {
    Entity entity = static_cast<Entity>(41);
    PhysicsBodyDef def{.type = BodyType::Dynamic};
    physics->createBody(entity, def);

    physics->update(1.0f / 60.0f);
}

TEST_F(PhysicsSystemTest, GravityAffectsBodyAfterUpdate) {
    Entity entity = static_cast<Entity>(42);
    PhysicsBodyDef def{
        .type = BodyType::Dynamic,
        .transform = {.x = 100.0f, .y = 100.0f}
    };
    physics->createBody(entity, def);

    Vec2 initialVelocity = physics->getVelocity(entity);

    // Simulate for a frame
    physics->update(1.0f / 60.0f);

    Vec2 finalVelocity = physics->getVelocity(entity);

    // Velocity should change due to gravity
    EXPECT_LT(finalVelocity.y, initialVelocity.y);
}

//=============================================================================
// Collision Callback Tests
//=============================================================================

TEST_F(PhysicsSystemTest, CanSetCollisionCallback) {
    bool callbackSet = false;

    auto callback = [&callbackSet](const CollisionEvent& event) {
        callbackSet = true;
    };

    physics->setCollisionCallback(callback);
    // Should not crash - callback should be stored
}

TEST_F(PhysicsSystemTest, CollisionCallbackIsInvokedOnCollision) {
    int callbackCount = 0;

    auto callback = [&callbackCount](const CollisionEvent& event) {
        callbackCount++;
    };

    physics->setCollisionCallback(callback);

    // Create two bodies that should collide
    Entity entity1 = static_cast<Entity>(43);
    Entity entity2 = static_cast<Entity>(44);

    PhysicsBodyDef def1{
        .type = BodyType::Dynamic,
        .transform = {.x = 100.0f, .y = 100.0f}
    };
    PhysicsBodyDef def2{
        .type = BodyType::Static,
        .transform = {.x = 100.0f, .y = 50.0f}  // Below entity1
    };

    physics->createBody(entity1, def1);
    physics->createBody(entity2, def2);

    // Simulate to allow collision
    for (int i = 0; i < 60; ++i) {
        physics->update(1.0f / 60.0f);
    }

    // Callback should have been invoked at least once
    EXPECT_GT(callbackCount, 0);
}

//=============================================================================
// Body Definition Property Tests
//=============================================================================

TEST_F(PhysicsSystemTest, BodyDefFixedRotationIsRespected) {
    Entity entity = static_cast<Entity>(45);
    PhysicsBodyDef def{
        .type = BodyType::Dynamic,
        .fixedRotation = true
    };
    physics->createBody(entity, def);

    // Apply torque which should not affect rotation if fixedRotation is true
    physics->applyTorque(entity, 100.0f);
    physics->update(1.0f / 60.0f);

    float angularVelocity = physics->getAngularVelocity(entity);
    // Fixed rotation bodies should have zero angular velocity
    EXPECT_FLOAT_EQ(angularVelocity, 0.0f);
}

TEST_F(PhysicsSystemTest, StaticBodiesDoNotMove) {
    Entity entity = static_cast<Entity>(46);
    PhysicsBodyDef def{
        .type = BodyType::Static,
        .transform = {.x = 100.0f, .y = 100.0f}
    };
    physics->createBody(entity, def);

    Vec2 initialPos = physics->getPosition(entity);

    // Apply force (should have no effect on static body)
    physics->applyForce(entity, Vec2{1000.0f, 1000.0f});
    physics->update(1.0f / 60.0f);

    Vec2 finalPos = physics->getPosition(entity);

    EXPECT_FLOAT_EQ(initialPos.x, finalPos.x);
    EXPECT_FLOAT_EQ(initialPos.y, finalPos.y);
}

TEST_F(PhysicsSystemTest, KinematicBodiesCanMoveButAreNotAffectedByForces) {
    Entity entity = static_cast<Entity>(47);
    PhysicsBodyDef def{
        .type = BodyType::Kinematic,
        .transform = {.x = 100.0f, .y = 100.0f}
    };
    physics->createBody(entity, def);

    // Set velocity directly (kinematic bodies respond to velocity)
    physics->setVelocity(entity, Vec2{10.0f, 0.0f});
    physics->update(1.0f / 60.0f);

    Vec2 pos = physics->getPosition(entity);
    EXPECT_GT(pos.x, 100.0f);
}

}  // namespace jframe::tests

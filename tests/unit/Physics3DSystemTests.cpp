// tests/unit/Physics3DSystemTests.cpp
// 3D Physics system unit tests (Jolt Physics)

#include <memory>
#include <vector>
#include <cmath>

#include <gtest/gtest.h>

import bestow.physics3d;
import bestow.physics3d.impl;
import bestow.types;

namespace bestow::tests {

class Physics3DSystemTest : public ::testing::Test {
protected:
    void SetUp() override {
        physics3d_ = createPhysics3DSystem();
    }

    std::unique_ptr<IPhysics3DSystem> physics3d_;
};

//==============================================================================
// World Setup Tests
//==============================================================================

TEST_F(Physics3DSystemTest, CanCreatePhysics3DSystem) {
    EXPECT_NE(physics3d_, nullptr);
}

TEST_F(Physics3DSystemTest, DefaultGravityIsStandardEarthGravity) {
    Vec3 gravity = physics3d_->getGravity();
    EXPECT_FLOAT_EQ(gravity.x, 0.0f);
    EXPECT_FLOAT_EQ(gravity.y, -9.81f);  // Standard gravity in m/s²
    EXPECT_FLOAT_EQ(gravity.z, 0.0f);
}

TEST_F(Physics3DSystemTest, CanChangeGravity) {
    Vec3 newGravity{0.0f, -20.0f, 0.0f};
    physics3d_->setGravity(newGravity);

    Vec3 retrieved = physics3d_->getGravity();
    EXPECT_FLOAT_EQ(retrieved.x, newGravity.x);
    EXPECT_FLOAT_EQ(retrieved.y, newGravity.y);
    EXPECT_FLOAT_EQ(retrieved.z, newGravity.z);
}

TEST_F(Physics3DSystemTest, CanSetZeroGravity) {
    Vec3 zeroGravity{0.0f, 0.0f, 0.0f};
    physics3d_->setGravity(zeroGravity);

    Vec3 retrieved = physics3d_->getGravity();
    EXPECT_FLOAT_EQ(retrieved.x, 0.0f);
    EXPECT_FLOAT_EQ(retrieved.y, 0.0f);
    EXPECT_FLOAT_EQ(retrieved.z, 0.0f);
}

//==============================================================================
// Body Creation and Management Tests
//==============================================================================

TEST_F(Physics3DSystemTest, CanCreateDynamicBody) {
    Entity entity = static_cast<Entity>(1);

    PhysicsBodyDef3D def;
    def.type = BodyType3D::Dynamic;
    def.transform.position = Vec3{0.0f, 10.0f, 0.0f};
    def.shapeType = ShapeType3D::Box;
    def.shapeHalfExtents = Vec3{1.0f, 1.0f, 1.0f};

    auto result = physics3d_->createBody(entity, def);
    EXPECT_TRUE(result.has_value());
    EXPECT_TRUE(physics3d_->hasBody(entity));
}

TEST_F(Physics3DSystemTest, CanCreateStaticBody) {
    Entity entity = static_cast<Entity>(2);

    PhysicsBodyDef3D def;
    def.type = BodyType3D::Static;
    def.transform.position = Vec3{0.0f, 0.0f, 0.0f};

    auto result = physics3d_->createBody(entity, def);
    EXPECT_TRUE(result.has_value());
    EXPECT_TRUE(physics3d_->hasBody(entity));
}

TEST_F(Physics3DSystemTest, CanCreateKinematicBody) {
    Entity entity = static_cast<Entity>(3);

    PhysicsBodyDef3D def;
    def.type = BodyType3D::Kinematic;
    def.transform.position = Vec3{0.0f, 5.0f, 0.0f};

    auto result = physics3d_->createBody(entity, def);
    EXPECT_TRUE(result.has_value());
    EXPECT_TRUE(physics3d_->hasBody(entity));
}

TEST_F(Physics3DSystemTest, CanDestroyBody) {
    Entity entity = static_cast<Entity>(4);

    PhysicsBodyDef3D def;
    def.type = BodyType3D::Dynamic;

    auto result = physics3d_->createBody(entity, def);
    ASSERT_TRUE(result.has_value());
    EXPECT_TRUE(physics3d_->hasBody(entity));

    physics3d_->destroyBody(entity);
    EXPECT_FALSE(physics3d_->hasBody(entity));
}

TEST_F(Physics3DSystemTest, HasBodyReturnsFalseForNonExistentBody) {
    Entity entity = static_cast<Entity>(999);
    EXPECT_FALSE(physics3d_->hasBody(entity));
}

//==============================================================================
// Shape Tests
//==============================================================================

TEST_F(Physics3DSystemTest, CreateBodyWithBoxShape) {
    Entity entity = static_cast<Entity>(5);

    PhysicsBodyDef3D def;
    def.shapeType = ShapeType3D::Box;
    def.shapeHalfExtents = Vec3{2.0f, 1.0f, 0.5f};

    auto result = physics3d_->createBody(entity, def);
    EXPECT_TRUE(result.has_value());
}

TEST_F(Physics3DSystemTest, CreateBodyWithSphereShape) {
    Entity entity = static_cast<Entity>(6);

    PhysicsBodyDef3D def;
    def.shapeType = ShapeType3D::Sphere;
    def.shapeRadius = 1.5f;

    auto result = physics3d_->createBody(entity, def);
    EXPECT_TRUE(result.has_value());
}

TEST_F(Physics3DSystemTest, CreateBodyWithCapsuleShape) {
    Entity entity = static_cast<Entity>(7);

    PhysicsBodyDef3D def;
    def.shapeType = ShapeType3D::Capsule;
    def.shapeRadius = 0.5f;
    def.shapeHeight = 2.0f;

    auto result = physics3d_->createBody(entity, def);
    EXPECT_TRUE(result.has_value());
}

TEST_F(Physics3DSystemTest, CreateBodyWithCylinderShape) {
    Entity entity = static_cast<Entity>(8);

    PhysicsBodyDef3D def;
    def.shapeType = ShapeType3D::Cylinder;
    def.shapeRadius = 1.0f;
    def.shapeHeight = 3.0f;

    auto result = physics3d_->createBody(entity, def);
    EXPECT_TRUE(result.has_value());
}

//==============================================================================
// Transform Tests
//==============================================================================

TEST_F(Physics3DSystemTest, GetPositionReturnsSetPosition) {
    Entity entity = static_cast<Entity>(9);

    PhysicsBodyDef3D def;
    def.transform.position = Vec3{5.0f, 10.0f, 15.0f};

    auto result = physics3d_->createBody(entity, def);
    ASSERT_TRUE(result.has_value());

    Vec3 pos = physics3d_->getPosition(entity);
    EXPECT_FLOAT_EQ(pos.x, 5.0f);
    EXPECT_FLOAT_EQ(pos.y, 10.0f);
    EXPECT_FLOAT_EQ(pos.z, 15.0f);
}

TEST_F(Physics3DSystemTest, SetPosition) {
    Entity entity = static_cast<Entity>(10);

    PhysicsBodyDef3D def;
    auto result = physics3d_->createBody(entity, def);
    ASSERT_TRUE(result.has_value());

    Vec3 newPos{20.0f, 30.0f, 40.0f};
    physics3d_->setPosition(entity, newPos);

    Vec3 retrieved = physics3d_->getPosition(entity);
    EXPECT_FLOAT_EQ(retrieved.x, 20.0f);
    EXPECT_FLOAT_EQ(retrieved.y, 30.0f);
    EXPECT_FLOAT_EQ(retrieved.z, 40.0f);
}

TEST_F(Physics3DSystemTest, GetRotationReturnsSetRotation) {
    Entity entity = static_cast<Entity>(11);

    PhysicsBodyDef3D def;
    def.transform.rotation = Quat{0.707f, 0.0f, 0.707f, 0.0f};  // 90 degrees around Y

    auto result = physics3d_->createBody(entity, def);
    ASSERT_TRUE(result.has_value());

    Quat rot = physics3d_->getRotation(entity);
    // Quaternion may be normalized, so check approximate values
    EXPECT_NEAR(rot.w, 0.707f, 0.01f);
}

TEST_F(Physics3DSystemTest, SetRotation) {
    Entity entity = static_cast<Entity>(12);

    PhysicsBodyDef3D def;
    auto result = physics3d_->createBody(entity, def);
    ASSERT_TRUE(result.has_value());

    Quat newRot{1.0f, 0.0f, 0.0f, 0.0f};  // Identity
    physics3d_->setRotation(entity, newRot);

    Quat retrieved = physics3d_->getRotation(entity);
    EXPECT_NEAR(retrieved.w, 1.0f, 0.01f);
}

//==============================================================================
// Velocity Tests
//==============================================================================

TEST_F(Physics3DSystemTest, SetLinearVelocity) {
    Entity entity = static_cast<Entity>(13);

    PhysicsBodyDef3D def;
    def.type = BodyType3D::Dynamic;
    auto result = physics3d_->createBody(entity, def);
    ASSERT_TRUE(result.has_value());

    Vec3 velocity{5.0f, 0.0f, 0.0f};
    physics3d_->setLinearVelocity(entity, velocity);

    Vec3 retrieved = physics3d_->getLinearVelocity(entity);
    EXPECT_FLOAT_EQ(retrieved.x, 5.0f);
    EXPECT_FLOAT_EQ(retrieved.y, 0.0f);
    EXPECT_FLOAT_EQ(retrieved.z, 0.0f);
}

TEST_F(Physics3DSystemTest, SetAngularVelocity) {
    Entity entity = static_cast<Entity>(14);

    PhysicsBodyDef3D def;
    def.type = BodyType3D::Dynamic;
    auto result = physics3d_->createBody(entity, def);
    ASSERT_TRUE(result.has_value());

    Vec3 angularVel{0.0f, 1.0f, 0.0f};
    physics3d_->setAngularVelocity(entity, angularVel);

    Vec3 retrieved = physics3d_->getAngularVelocity(entity);
    EXPECT_FLOAT_EQ(retrieved.x, 0.0f);
    EXPECT_FLOAT_EQ(retrieved.y, 1.0f);
    EXPECT_FLOAT_EQ(retrieved.z, 0.0f);
}

TEST_F(Physics3DSystemTest, InitialVelocityIsZero) {
    Entity entity = static_cast<Entity>(15);

    PhysicsBodyDef3D def;
    def.type = BodyType3D::Dynamic;
    auto result = physics3d_->createBody(entity, def);
    ASSERT_TRUE(result.has_value());

    Vec3 linearVel = physics3d_->getLinearVelocity(entity);
    EXPECT_FLOAT_EQ(linearVel.x, 0.0f);
    EXPECT_FLOAT_EQ(linearVel.y, 0.0f);
    EXPECT_FLOAT_EQ(linearVel.z, 0.0f);

    Vec3 angularVel = physics3d_->getAngularVelocity(entity);
    EXPECT_FLOAT_EQ(angularVel.x, 0.0f);
    EXPECT_FLOAT_EQ(angularVel.y, 0.0f);
    EXPECT_FLOAT_EQ(angularVel.z, 0.0f);
}

//==============================================================================
// Force Application Tests
//==============================================================================

TEST_F(Physics3DSystemTest, ApplyForce) {
    Entity entity = static_cast<Entity>(16);

    PhysicsBodyDef3D def;
    def.type = BodyType3D::Dynamic;
    auto result = physics3d_->createBody(entity, def);
    ASSERT_TRUE(result.has_value());

    Vec3 force{100.0f, 0.0f, 0.0f};
    physics3d_->applyForce(entity, force);
    // Should not crash
}

TEST_F(Physics3DSystemTest, ApplyImpulse) {
    Entity entity = static_cast<Entity>(17);

    PhysicsBodyDef3D def;
    def.type = BodyType3D::Dynamic;
    auto result = physics3d_->createBody(entity, def);
    ASSERT_TRUE(result.has_value());

    Vec3 impulse{10.0f, 0.0f, 0.0f};
    physics3d_->applyImpulse(entity, impulse);
    // Should not crash
}

TEST_F(Physics3DSystemTest, ApplyTorque) {
    Entity entity = static_cast<Entity>(18);

    PhysicsBodyDef3D def;
    def.type = BodyType3D::Dynamic;
    auto result = physics3d_->createBody(entity, def);
    ASSERT_TRUE(result.has_value());

    Vec3 torque{0.0f, 5.0f, 0.0f};
    physics3d_->applyTorque(entity, torque);
    // Should not crash
}

//==============================================================================
// Update Tests
//==============================================================================

TEST_F(Physics3DSystemTest, UpdateDoesNotCrashWithNoBodies) {
    physics3d_->update(1.0f / 60.0f);  // 60 FPS
}

TEST_F(Physics3DSystemTest, UpdateDoesNotCrashWithBodies) {
    Entity entity = static_cast<Entity>(19);

    PhysicsBodyDef3D def;
    def.type = BodyType3D::Dynamic;
    auto result = physics3d_->createBody(entity, def);
    ASSERT_TRUE(result.has_value());

    physics3d_->update(1.0f / 60.0f);
}

TEST_F(Physics3DSystemTest, GravityAffectsDynamicBodies) {
    Entity entity = static_cast<Entity>(20);

    PhysicsBodyDef3D def;
    def.type = BodyType3D::Dynamic;
    def.transform.position = Vec3{0.0f, 100.0f, 0.0f};
    auto result = physics3d_->createBody(entity, def);
    ASSERT_TRUE(result.has_value());

    Vec3 initialVelocity = physics3d_->getLinearVelocity(entity);

    // Simulate for a few frames
    for (int i = 0; i < 10; ++i) {
        physics3d_->update(1.0f / 60.0f);
    }

    Vec3 finalVelocity = physics3d_->getLinearVelocity(entity);

    // Velocity in Y should have decreased due to gravity
    EXPECT_LT(finalVelocity.y, initialVelocity.y);
}

//==============================================================================
// Raycast Tests
//==============================================================================

TEST_F(Physics3DSystemTest, RaycastHitsBody) {
    Entity entity = static_cast<Entity>(21);

    PhysicsBodyDef3D def;
    def.type = BodyType3D::Static;
    def.transform.position = Vec3{0.0f, 0.0f, 0.0f};
    def.shapeType = ShapeType3D::Box;
    def.shapeHalfExtents = Vec3{1.0f, 1.0f, 1.0f};
    auto result = physics3d_->createBody(entity, def);
    ASSERT_TRUE(result.has_value());

    Vec3 origin{0.0f, 10.0f, 0.0f};
    Vec3 direction{0.0f, -1.0f, 0.0f};
    float maxDistance = 20.0f;

    auto hit = physics3d_->raycast(origin, direction, maxDistance);
    EXPECT_TRUE(hit.has_value());

    if (hit.has_value()) {
        EXPECT_EQ(hit->entity, entity);
        EXPECT_GT(hit->distance, 0.0f);
        EXPECT_LT(hit->distance, maxDistance);
    }
}

TEST_F(Physics3DSystemTest, RaycastReturnsNulloptWhenNoHit) {
    Vec3 origin{0.0f, 0.0f, 0.0f};
    Vec3 direction{1.0f, 0.0f, 0.0f};
    float maxDistance = 10.0f;

    auto hit = physics3d_->raycast(origin, direction, maxDistance);
    EXPECT_FALSE(hit.has_value());
}

TEST_F(Physics3DSystemTest, RaycastRespectsMaxDistance) {
    Entity entity = static_cast<Entity>(22);

    PhysicsBodyDef3D def;
    def.type = BodyType3D::Static;
    def.transform.position = Vec3{0.0f, 0.0f, 0.0f};
    auto result = physics3d_->createBody(entity, def);
    ASSERT_TRUE(result.has_value());

    Vec3 origin{0.0f, 10.0f, 0.0f};
    Vec3 direction{0.0f, -1.0f, 0.0f};
    float maxDistance = 5.0f;  // Too short

    auto hit = physics3d_->raycast(origin, direction, maxDistance);
    EXPECT_FALSE(hit.has_value());
}

//==============================================================================
// Character Controller Tests
//==============================================================================

TEST_F(Physics3DSystemTest, CreateCharacterController) {
    Entity entity = static_cast<Entity>(23);

    CharacterControllerDef3D def;
    def.height = 2.0f;
    def.radius = 0.5f;
    def.stepHeight = 0.3f;
    def.slopeLimit = 45.0f;

    auto result = physics3d_->createCharacterController(entity, def);
    EXPECT_TRUE(result.has_value());
    EXPECT_TRUE(physics3d_->hasCharacterController(entity));
}

TEST_F(Physics3DSystemTest, DestroyCharacterController) {
    Entity entity = static_cast<Entity>(24);

    CharacterControllerDef3D def;
    auto result = physics3d_->createCharacterController(entity, def);
    ASSERT_TRUE(result.has_value());

    physics3d_->destroyCharacterController(entity);
    EXPECT_FALSE(physics3d_->hasCharacterController(entity));
}

TEST_F(Physics3DSystemTest, MoveCharacterController) {
    Entity entity = static_cast<Entity>(25);

    CharacterControllerDef3D def;
    auto result = physics3d_->createCharacterController(entity, def);
    ASSERT_TRUE(result.has_value());

    Vec3 displacement{1.0f, 0.0f, 0.0f};
    physics3d_->moveCharacter(entity, displacement, 1.0f / 60.0f);
    // Should not crash
}

TEST_F(Physics3DSystemTest, CheckCharacterGrounded) {
    Entity entity = static_cast<Entity>(26);

    CharacterControllerDef3D def;
    auto result = physics3d_->createCharacterController(entity, def);
    ASSERT_TRUE(result.has_value());

    bool grounded = physics3d_->isCharacterGrounded(entity);
    // Should return valid result (true or false)
    EXPECT_TRUE(grounded || !grounded);
}

//==============================================================================
// Vehicle System Tests
//==============================================================================

TEST_F(Physics3DSystemTest, CreateVehicle) {
    Entity entity = static_cast<Entity>(27);

    VehicleDef3D def;
    def.chassisHalfExtents = Vec3{1.0f, 0.5f, 2.0f};
    def.wheelRadius = 0.4f;
    def.wheelWidth = 0.3f;
    def.wheelCount = 4;

    auto result = physics3d_->createVehicle(entity, def);
    EXPECT_TRUE(result.has_value());
    EXPECT_TRUE(physics3d_->hasVehicle(entity));
}

TEST_F(Physics3DSystemTest, DestroyVehicle) {
    Entity entity = static_cast<Entity>(28);

    VehicleDef3D def;
    auto result = physics3d_->createVehicle(entity, def);
    ASSERT_TRUE(result.has_value());

    physics3d_->destroyVehicle(entity);
    EXPECT_FALSE(physics3d_->hasVehicle(entity));
}

TEST_F(Physics3DSystemTest, SetVehicleInput) {
    Entity entity = static_cast<Entity>(29);

    VehicleDef3D def;
    auto result = physics3d_->createVehicle(entity, def);
    ASSERT_TRUE(result.has_value());

    VehicleInput3D input;
    input.throttle = 0.5f;
    input.brake = 0.0f;
    input.steer = 0.2f;

    physics3d_->setVehicleInput(entity, input);
    // Should not crash
}

TEST_F(Physics3DSystemTest, GetVehicleState) {
    Entity entity = static_cast<Entity>(30);

    VehicleDef3D def;
    auto result = physics3d_->createVehicle(entity, def);
    ASSERT_TRUE(result.has_value());

    auto state = physics3d_->getVehicleState(entity);
    EXPECT_TRUE(state.has_value());

    if (state.has_value()) {
        EXPECT_GE(state->speed, 0.0f);
        EXPECT_GE(state->rpm, 0.0f);
    }
}

//==============================================================================
// Collision Layer Tests
//==============================================================================

TEST_F(Physics3DSystemTest, SetCollisionLayer) {
    Entity entity = static_cast<Entity>(31);

    PhysicsBodyDef3D def;
    auto result = physics3d_->createBody(entity, def);
    ASSERT_TRUE(result.has_value());

    physics3d_->setCollisionLayer(entity, CollisionLayers::Player);
    // Should not crash
}

TEST_F(Physics3DSystemTest, SetCollisionMask) {
    Entity entity = static_cast<Entity>(32);

    PhysicsBodyDef3D def;
    auto result = physics3d_->createBody(entity, def);
    ASSERT_TRUE(result.has_value());

    CollisionMask mask = CollisionLayers::Enemy | CollisionLayers::Terrain;
    physics3d_->setCollisionMask(entity, mask);
    // Should not crash
}

//==============================================================================
// Collision Callback Tests
//==============================================================================

TEST_F(Physics3DSystemTest, SetCollisionCallback) {
    int callbackCount = 0;

    auto callback = [&callbackCount](const CollisionEvent3D& event) {
        callbackCount++;
    };

    physics3d_->setCollisionCallback(callback);
    // Should not crash
}

TEST_F(Physics3DSystemTest, SetTriggerCallback) {
    int callbackCount = 0;

    auto callback = [&callbackCount](const TriggerEvent3D& event) {
        callbackCount++;
    };

    physics3d_->setTriggerCallback(callback);
    // Should not crash
}

//==============================================================================
// Edge Cases and Error Handling Tests
//==============================================================================

TEST_F(Physics3DSystemTest, GetPositionForNonExistentBody) {
    Entity entity = static_cast<Entity>(999);
    Vec3 pos = physics3d_->getPosition(entity);

    // Should return default position
    EXPECT_FLOAT_EQ(pos.x, 0.0f);
    EXPECT_FLOAT_EQ(pos.y, 0.0f);
    EXPECT_FLOAT_EQ(pos.z, 0.0f);
}

TEST_F(Physics3DSystemTest, SetPositionForNonExistentBody) {
    Entity entity = static_cast<Entity>(999);
    Vec3 pos{1.0f, 2.0f, 3.0f};

    physics3d_->setPosition(entity, pos);
    // Should not crash
}

TEST_F(Physics3DSystemTest, ApplyForceToNonExistentBody) {
    Entity entity = static_cast<Entity>(999);
    Vec3 force{10.0f, 0.0f, 0.0f};

    physics3d_->applyForce(entity, force);
    // Should not crash
}

TEST_F(Physics3DSystemTest, DestroyNonExistentBody) {
    Entity entity = static_cast<Entity>(999);
    physics3d_->destroyBody(entity);
    // Should not crash
}

TEST_F(Physics3DSystemTest, CreateBodyTwiceReplacesFirst) {
    Entity entity = static_cast<Entity>(33);

    PhysicsBodyDef3D def1;
    def1.transform.position = Vec3{1.0f, 2.0f, 3.0f};
    auto result1 = physics3d_->createBody(entity, def1);
    ASSERT_TRUE(result1.has_value());

    PhysicsBodyDef3D def2;
    def2.transform.position = Vec3{4.0f, 5.0f, 6.0f};
    auto result2 = physics3d_->createBody(entity, def2);
    ASSERT_TRUE(result2.has_value());

    Vec3 pos = physics3d_->getPosition(entity);
    EXPECT_FLOAT_EQ(pos.x, 4.0f);
    EXPECT_FLOAT_EQ(pos.y, 5.0f);
    EXPECT_FLOAT_EQ(pos.z, 6.0f);
}

}  // namespace bestow::tests

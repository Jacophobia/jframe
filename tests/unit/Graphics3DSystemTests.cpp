// tests/unit/Graphics3DSystemTests.cpp
// 3D Graphics system unit tests including Lock-On targeting system

#include <memory>

#include <gtest/gtest.h>
#include <kangaru/kangaru.hpp>
#include <entt/entity/entity.hpp>

import std;
import bestow;
import bestow.types;
import bestow.graphics3d;
import bestow.entity;
import bestow.entity.impl;

#include "../mocks/MockGraphics3DSystem.hpp"

namespace bestow::tests {

//==============================================================================
// Lock-On System Tests
//==============================================================================

class LockOnSystemTest : public ::testing::Test {
protected:
    void SetUp() override {
        entitySystem_ = std::make_unique<EntitySystem>();
        graphics3D_ = std::make_unique<MockGraphics3DSystem>();
    }

    void TearDown() override {
        graphics3D_->resetLockOnTestState();
    }

    // Helper to create a mock target
    LockOnResult createMockTarget(Entity entity, Vec3 worldPos, Vec2 screenPos, float distance, float score) {
        LockOnResult result;
        result.entity = entity;
        result.lockPointIndex = 0;
        result.worldPosition = worldPos;
        result.screenPosition = screenPos;
        result.distance = distance;
        result.score = score;
        return result;
    }

    std::unique_ptr<IEntitySystem> entitySystem_;
    std::unique_ptr<MockGraphics3DSystem> graphics3D_;
};

//==============================================================================
// LockOnConfig Tests
//==============================================================================

TEST_F(LockOnSystemTest, DefaultConfigValues) {
    LockOnConfig config = graphics3D_->getLockOnConfig();
    // Check default values
    EXPECT_FLOAT_EQ(config.maxRange, 50.0f);
    EXPECT_FLOAT_EQ(config.fovMargin, 0.9f);
    EXPECT_FLOAT_EQ(config.centerBias, 0.7f);
    EXPECT_FLOAT_EQ(config.priorityWeight, 0.3f);
    EXPECT_TRUE(config.preferCurrentTarget);
    EXPECT_FLOAT_EQ(config.hysteresis, 1.2f);
}

TEST_F(LockOnSystemTest, SetLockOnConfigMaxRange) {
    LockOnConfig config;
    config.maxRange = 100.0f;
    graphics3D_->setLockOnConfig(config);

    LockOnConfig retrieved = graphics3D_->getLockOnConfig();
    EXPECT_FLOAT_EQ(retrieved.maxRange, 100.0f);
}

TEST_F(LockOnSystemTest, SetLockOnConfigFovMargin) {
    LockOnConfig config;
    config.fovMargin = 0.8f;
    graphics3D_->setLockOnConfig(config);

    LockOnConfig retrieved = graphics3D_->getLockOnConfig();
    EXPECT_FLOAT_EQ(retrieved.fovMargin, 0.8f);
}

TEST_F(LockOnSystemTest, SetLockOnConfigCenterBias) {
    LockOnConfig config;
    config.centerBias = 0.5f;
    graphics3D_->setLockOnConfig(config);

    LockOnConfig retrieved = graphics3D_->getLockOnConfig();
    EXPECT_FLOAT_EQ(retrieved.centerBias, 0.5f);
}

TEST_F(LockOnSystemTest, SetLockOnConfigPriorityWeight) {
    LockOnConfig config;
    config.priorityWeight = 0.5f;
    graphics3D_->setLockOnConfig(config);

    LockOnConfig retrieved = graphics3D_->getLockOnConfig();
    EXPECT_FLOAT_EQ(retrieved.priorityWeight, 0.5f);
}

TEST_F(LockOnSystemTest, SetLockOnConfigPreferCurrentTarget) {
    LockOnConfig config;
    config.preferCurrentTarget = false;
    graphics3D_->setLockOnConfig(config);

    LockOnConfig retrieved = graphics3D_->getLockOnConfig();
    EXPECT_FALSE(retrieved.preferCurrentTarget);
}

TEST_F(LockOnSystemTest, SetLockOnConfigHysteresis) {
    LockOnConfig config;
    config.hysteresis = 1.5f;
    graphics3D_->setLockOnConfig(config);

    LockOnConfig retrieved = graphics3D_->getLockOnConfig();
    EXPECT_FLOAT_EQ(retrieved.hysteresis, 1.5f);
}

TEST_F(LockOnSystemTest, SetLockOnConfigMultipleValues) {
    LockOnConfig config;
    config.maxRange = 75.0f;
    config.fovMargin = 0.85f;
    config.centerBias = 0.6f;
    config.priorityWeight = 0.4f;
    config.preferCurrentTarget = false;
    config.hysteresis = 1.3f;
    graphics3D_->setLockOnConfig(config);

    LockOnConfig retrieved = graphics3D_->getLockOnConfig();
    EXPECT_FLOAT_EQ(retrieved.maxRange, 75.0f);
    EXPECT_FLOAT_EQ(retrieved.fovMargin, 0.85f);
    EXPECT_FLOAT_EQ(retrieved.centerBias, 0.6f);
    EXPECT_FLOAT_EQ(retrieved.priorityWeight, 0.4f);
    EXPECT_FALSE(retrieved.preferCurrentTarget);
    EXPECT_FLOAT_EQ(retrieved.hysteresis, 1.3f);
}

//==============================================================================
// Lock-On State Tests
//==============================================================================

TEST_F(LockOnSystemTest, NotLockedByDefault) {
    EXPECT_FALSE(graphics3D_->isLocked());
}

TEST_F(LockOnSystemTest, GetLockTargetReturnsNulloptWhenNotLocked) {
    auto target = graphics3D_->getLockTarget();
    EXPECT_FALSE(target.has_value());
}

TEST_F(LockOnSystemTest, LockOnWithNoTargetsReturnsInvalidResult) {
    // No mock targets set
    LockOnResult result = graphics3D_->lockOn(*entitySystem_, nullptr);
    EXPECT_FALSE(result.isValid());
    EXPECT_FALSE(graphics3D_->isLocked());
}

TEST_F(LockOnSystemTest, LockOnWithSingleTargetSucceeds) {
    Entity enemy = entitySystem_->createEntity();
    LockOnResult mockTarget = createMockTarget(
        enemy, Vec3{10.0f, 0.0f, -20.0f}, Vec2{400.0f, 300.0f}, 22.36f, 0.9f);
    graphics3D_->setMockPotentialTargets({mockTarget});

    LockOnResult result = graphics3D_->lockOn(*entitySystem_, nullptr);

    EXPECT_TRUE(result.isValid());
    EXPECT_EQ(result.entity, enemy);
    EXPECT_TRUE(graphics3D_->isLocked());
    EXPECT_TRUE(graphics3D_->wasLockOnCalled());
}

TEST_F(LockOnSystemTest, GetLockTargetReturnsTargetWhenLocked) {
    Entity enemy = entitySystem_->createEntity();
    LockOnResult mockTarget = createMockTarget(
        enemy, Vec3{5.0f, 2.0f, -15.0f}, Vec2{420.0f, 280.0f}, 15.81f, 0.85f);
    graphics3D_->setMockPotentialTargets({mockTarget});

    graphics3D_->lockOn(*entitySystem_, nullptr);

    auto target = graphics3D_->getLockTarget();
    EXPECT_TRUE(target.has_value());
    EXPECT_EQ(target->entity, enemy);
    EXPECT_FLOAT_EQ(target->worldPosition.x, 5.0f);
    EXPECT_FLOAT_EQ(target->worldPosition.y, 2.0f);
    EXPECT_FLOAT_EQ(target->worldPosition.z, -15.0f);
}

TEST_F(LockOnSystemTest, LockOnSelectsBestTarget) {
    Entity enemy1 = entitySystem_->createEntity();
    Entity enemy2 = entitySystem_->createEntity();

    // enemy2 has higher score
    LockOnResult target1 = createMockTarget(
        enemy1, Vec3{10.0f, 0.0f, -20.0f}, Vec2{350.0f, 300.0f}, 22.36f, 0.7f);
    LockOnResult target2 = createMockTarget(
        enemy2, Vec3{5.0f, 0.0f, -10.0f}, Vec2{400.0f, 300.0f}, 11.18f, 0.95f);

    graphics3D_->setMockPotentialTargets({target2, target1});  // target2 first (highest score)

    LockOnResult result = graphics3D_->lockOn(*entitySystem_, nullptr);

    EXPECT_TRUE(result.isValid());
    EXPECT_EQ(result.entity, enemy2);  // Should select enemy2 with higher score
}

//==============================================================================
// Unlock Tests
//==============================================================================

TEST_F(LockOnSystemTest, UnlockClearsLockState) {
    Entity enemy = entitySystem_->createEntity();
    LockOnResult mockTarget = createMockTarget(
        enemy, Vec3{10.0f, 0.0f, -20.0f}, Vec2{400.0f, 300.0f}, 22.36f, 0.9f);
    graphics3D_->setMockPotentialTargets({mockTarget});

    graphics3D_->lockOn(*entitySystem_, nullptr);
    EXPECT_TRUE(graphics3D_->isLocked());

    graphics3D_->unlock();

    EXPECT_FALSE(graphics3D_->isLocked());
    EXPECT_TRUE(graphics3D_->wasUnlockCalled());
    EXPECT_FALSE(graphics3D_->getLockTarget().has_value());
}

TEST_F(LockOnSystemTest, UnlockWhenNotLockedDoesNotCrash) {
    EXPECT_FALSE(graphics3D_->isLocked());
    EXPECT_NO_THROW(graphics3D_->unlock());
    EXPECT_FALSE(graphics3D_->isLocked());
}

TEST_F(LockOnSystemTest, CanRelockAfterUnlock) {
    Entity enemy1 = entitySystem_->createEntity();
    Entity enemy2 = entitySystem_->createEntity();

    LockOnResult target1 = createMockTarget(
        enemy1, Vec3{10.0f, 0.0f, -20.0f}, Vec2{400.0f, 300.0f}, 22.36f, 0.9f);
    graphics3D_->setMockPotentialTargets({target1});

    graphics3D_->lockOn(*entitySystem_, nullptr);
    graphics3D_->unlock();

    LockOnResult target2 = createMockTarget(
        enemy2, Vec3{5.0f, 0.0f, -10.0f}, Vec2{420.0f, 320.0f}, 11.18f, 0.85f);
    graphics3D_->setMockPotentialTargets({target2});

    LockOnResult result = graphics3D_->lockOn(*entitySystem_, nullptr);

    EXPECT_TRUE(graphics3D_->isLocked());
    EXPECT_EQ(result.entity, enemy2);
}

//==============================================================================
// PollLockPosition Tests
//==============================================================================

TEST_F(LockOnSystemTest, PollLockPositionReturnsNulloptWhenNotLocked) {
    auto pos = graphics3D_->pollLockPosition(*entitySystem_, nullptr);
    EXPECT_FALSE(pos.has_value());
}

TEST_F(LockOnSystemTest, PollLockPositionReturnsPositionWhenLocked) {
    Entity enemy = entitySystem_->createEntity();
    LockOnResult mockTarget = createMockTarget(
        enemy, Vec3{15.0f, 5.0f, -25.0f}, Vec2{400.0f, 300.0f}, 30.0f, 0.9f);
    graphics3D_->setMockPotentialTargets({mockTarget});

    graphics3D_->lockOn(*entitySystem_, nullptr);

    auto pos = graphics3D_->pollLockPosition(*entitySystem_, nullptr);

    EXPECT_TRUE(pos.has_value());
    EXPECT_FLOAT_EQ(pos->x, 15.0f);
    EXPECT_FLOAT_EQ(pos->y, 5.0f);
    EXPECT_FLOAT_EQ(pos->z, -25.0f);
    EXPECT_TRUE(graphics3D_->wasPollLockPositionCalled());
}

TEST_F(LockOnSystemTest, PollLockPositionReturnsNulloptAfterUnlock) {
    Entity enemy = entitySystem_->createEntity();
    LockOnResult mockTarget = createMockTarget(
        enemy, Vec3{10.0f, 0.0f, -20.0f}, Vec2{400.0f, 300.0f}, 22.36f, 0.9f);
    graphics3D_->setMockPotentialTargets({mockTarget});

    graphics3D_->lockOn(*entitySystem_, nullptr);
    graphics3D_->unlock();

    auto pos = graphics3D_->pollLockPosition(*entitySystem_, nullptr);
    EXPECT_FALSE(pos.has_value());
}

//==============================================================================
// ShiftLockTarget Tests
//==============================================================================

TEST_F(LockOnSystemTest, ShiftLockTargetRecordsDirection) {
    Entity enemy = entitySystem_->createEntity();
    LockOnResult mockTarget = createMockTarget(
        enemy, Vec3{10.0f, 0.0f, -20.0f}, Vec2{400.0f, 300.0f}, 22.36f, 0.9f);
    graphics3D_->setMockPotentialTargets({mockTarget});
    graphics3D_->lockOn(*entitySystem_, nullptr);

    Vec2 direction{1.0f, 0.0f};  // Shift right
    graphics3D_->shiftLockTarget(direction, *entitySystem_, nullptr);

    EXPECT_TRUE(graphics3D_->wasShiftLockTargetCalled());
    Vec2 recorded = graphics3D_->getLastShiftDirection();
    EXPECT_FLOAT_EQ(recorded.x, 1.0f);
    EXPECT_FLOAT_EQ(recorded.y, 0.0f);
}

TEST_F(LockOnSystemTest, ShiftLockTargetCyclesToNextTarget) {
    Entity enemy1 = entitySystem_->createEntity();
    Entity enemy2 = entitySystem_->createEntity();

    LockOnResult target1 = createMockTarget(
        enemy1, Vec3{10.0f, 0.0f, -20.0f}, Vec2{350.0f, 300.0f}, 22.36f, 0.9f);
    LockOnResult target2 = createMockTarget(
        enemy2, Vec3{-10.0f, 0.0f, -20.0f}, Vec2{450.0f, 300.0f}, 22.36f, 0.8f);

    graphics3D_->setMockPotentialTargets({target1, target2});
    graphics3D_->lockOn(*entitySystem_, nullptr);

    // Initially locked on target1
    auto initial = graphics3D_->getLockTarget();
    EXPECT_EQ(initial->entity, enemy1);

    // Shift to next target
    LockOnResult shifted = graphics3D_->shiftLockTarget(Vec2{1.0f, 0.0f}, *entitySystem_, nullptr);

    EXPECT_EQ(shifted.entity, enemy2);
}

TEST_F(LockOnSystemTest, ShiftLockTargetWrapsAround) {
    Entity enemy1 = entitySystem_->createEntity();
    Entity enemy2 = entitySystem_->createEntity();

    LockOnResult target1 = createMockTarget(
        enemy1, Vec3{10.0f, 0.0f, -20.0f}, Vec2{350.0f, 300.0f}, 22.36f, 0.9f);
    LockOnResult target2 = createMockTarget(
        enemy2, Vec3{-10.0f, 0.0f, -20.0f}, Vec2{450.0f, 300.0f}, 22.36f, 0.8f);

    graphics3D_->setMockPotentialTargets({target1, target2});
    graphics3D_->lockOn(*entitySystem_, nullptr);

    // Shift twice to wrap around
    graphics3D_->shiftLockTarget(Vec2{1.0f, 0.0f}, *entitySystem_, nullptr);
    LockOnResult shifted = graphics3D_->shiftLockTarget(Vec2{1.0f, 0.0f}, *entitySystem_, nullptr);

    EXPECT_EQ(shifted.entity, enemy1);  // Back to first target
}

TEST_F(LockOnSystemTest, ShiftLockTargetWithSingleTargetStaysOnSame) {
    Entity enemy = entitySystem_->createEntity();
    LockOnResult mockTarget = createMockTarget(
        enemy, Vec3{10.0f, 0.0f, -20.0f}, Vec2{400.0f, 300.0f}, 22.36f, 0.9f);
    graphics3D_->setMockPotentialTargets({mockTarget});
    graphics3D_->lockOn(*entitySystem_, nullptr);

    LockOnResult shifted = graphics3D_->shiftLockTarget(Vec2{1.0f, 0.0f}, *entitySystem_, nullptr);

    EXPECT_EQ(shifted.entity, enemy);  // Still same target
}

//==============================================================================
// GetPotentialTargets Tests
//==============================================================================

TEST_F(LockOnSystemTest, GetPotentialTargetsReturnsEmptyWhenNoTargets) {
    auto targets = graphics3D_->getPotentialTargets(*entitySystem_, nullptr);
    EXPECT_TRUE(targets.empty());
    EXPECT_TRUE(graphics3D_->wasGetPotentialTargetsCalled());
}

TEST_F(LockOnSystemTest, GetPotentialTargetsReturnsAllTargets) {
    Entity enemy1 = entitySystem_->createEntity();
    Entity enemy2 = entitySystem_->createEntity();
    Entity enemy3 = entitySystem_->createEntity();

    LockOnResult target1 = createMockTarget(
        enemy1, Vec3{10.0f, 0.0f, -20.0f}, Vec2{350.0f, 300.0f}, 22.36f, 0.9f);
    LockOnResult target2 = createMockTarget(
        enemy2, Vec3{-10.0f, 0.0f, -20.0f}, Vec2{450.0f, 300.0f}, 22.36f, 0.8f);
    LockOnResult target3 = createMockTarget(
        enemy3, Vec3{0.0f, 5.0f, -30.0f}, Vec2{400.0f, 250.0f}, 30.41f, 0.7f);

    graphics3D_->setMockPotentialTargets({target1, target2, target3});

    auto targets = graphics3D_->getPotentialTargets(*entitySystem_, nullptr);

    EXPECT_EQ(targets.size(), 3);
}

TEST_F(LockOnSystemTest, GetPotentialTargetsContainsCorrectData) {
    Entity enemy = entitySystem_->createEntity();
    LockOnResult mockTarget = createMockTarget(
        enemy, Vec3{12.0f, 3.0f, -18.0f}, Vec2{420.0f, 280.0f}, 21.84f, 0.88f);
    graphics3D_->setMockPotentialTargets({mockTarget});

    auto targets = graphics3D_->getPotentialTargets(*entitySystem_, nullptr);

    ASSERT_EQ(targets.size(), 1);
    EXPECT_EQ(targets[0].entity, enemy);
    EXPECT_FLOAT_EQ(targets[0].worldPosition.x, 12.0f);
    EXPECT_FLOAT_EQ(targets[0].worldPosition.y, 3.0f);
    EXPECT_FLOAT_EQ(targets[0].worldPosition.z, -18.0f);
    EXPECT_FLOAT_EQ(targets[0].screenPosition.x, 420.0f);
    EXPECT_FLOAT_EQ(targets[0].screenPosition.y, 280.0f);
    EXPECT_FLOAT_EQ(targets[0].distance, 21.84f);
    EXPECT_FLOAT_EQ(targets[0].score, 0.88f);
}

//==============================================================================
// LockOnResult Tests
//==============================================================================

TEST(LockOnResultTest, DefaultResultIsInvalid) {
    LockOnResult result;
    EXPECT_FALSE(result.isValid());
}

TEST(LockOnResultTest, ResultWithValidEntityIsValid) {
    LockOnResult result;
    result.entity = static_cast<Entity>(1);
    result.lockPointIndex = 0;  // isValid() requires both entity and lockPointIndex
    EXPECT_TRUE(result.isValid());
}

TEST(LockOnResultTest, ResultWithNullEntityIsInvalid) {
    LockOnResult result;
    result.entity = entt::null;
    EXPECT_FALSE(result.isValid());
}

//==============================================================================
// LockableTarget Component Tests
//==============================================================================

TEST_F(LockOnSystemTest, LockableTargetDefaultValues) {
    LockableTarget target;
    EXPECT_TRUE(target.enabled);
    EXPECT_TRUE(target.lockPoints.empty());
}

TEST_F(LockOnSystemTest, LockableTargetWithOffsetLockPoint) {
    LockableTarget target;
    target.enabled = true;

    LockPointDef lockPoint;
    lockPoint.name = "center";
    lockPoint.source = LockPointSource::Offset;
    lockPoint.localOffset = Vec3{0.0f, 1.0f, 0.0f};
    lockPoint.priority = 1.0f;

    target.lockPoints.push_back(lockPoint);

    EXPECT_EQ(target.lockPoints.size(), 1);
    EXPECT_EQ(target.lockPoints[0].name, "center");
    EXPECT_EQ(target.lockPoints[0].source, LockPointSource::Offset);
    EXPECT_FLOAT_EQ(target.lockPoints[0].localOffset.y, 1.0f);
}

TEST_F(LockOnSystemTest, LockableTargetWithSocketLockPoint) {
    LockableTarget target;
    target.enabled = true;

    LockPointDef lockPoint;
    lockPoint.name = "head";
    lockPoint.source = LockPointSource::Socket;
    lockPoint.socketName = "head_socket";
    lockPoint.priority = 1.5f;

    target.lockPoints.push_back(lockPoint);

    EXPECT_EQ(target.lockPoints.size(), 1);
    EXPECT_EQ(target.lockPoints[0].name, "head");
    EXPECT_EQ(target.lockPoints[0].source, LockPointSource::Socket);
    EXPECT_EQ(target.lockPoints[0].socketName, "head_socket");
    EXPECT_FLOAT_EQ(target.lockPoints[0].priority, 1.5f);
}

TEST_F(LockOnSystemTest, LockableTargetWithMultipleLockPoints) {
    LockableTarget target;
    target.enabled = true;

    LockPointDef headPoint;
    headPoint.name = "head";
    headPoint.source = LockPointSource::Socket;
    headPoint.socketName = "head_socket";
    headPoint.priority = 1.5f;

    LockPointDef chestPoint;
    chestPoint.name = "chest";
    chestPoint.source = LockPointSource::Socket;
    chestPoint.socketName = "spine_02";
    chestPoint.priority = 1.0f;

    LockPointDef weakPoint;
    weakPoint.name = "weakPoint";
    weakPoint.source = LockPointSource::Offset;
    weakPoint.localOffset = Vec3{0.0f, 0.5f, 0.0f};
    weakPoint.priority = 2.0f;

    target.lockPoints.push_back(headPoint);
    target.lockPoints.push_back(chestPoint);
    target.lockPoints.push_back(weakPoint);

    EXPECT_EQ(target.lockPoints.size(), 3);
    EXPECT_EQ(target.lockPoints[2].name, "weakPoint");
    EXPECT_FLOAT_EQ(target.lockPoints[2].priority, 2.0f);
}

TEST_F(LockOnSystemTest, LockableTargetCanBeDisabled) {
    LockableTarget target;
    target.enabled = false;

    LockPointDef lockPoint;
    lockPoint.name = "center";
    target.lockPoints.push_back(lockPoint);

    EXPECT_FALSE(target.enabled);
    EXPECT_EQ(target.lockPoints.size(), 1);
}

//==============================================================================
// AnimatorRef Component Tests
//==============================================================================

TEST(AnimatorRefTest, DefaultAnimatorRefIsInvalid) {
    AnimatorRef ref;
    EXPECT_FALSE(ref.isValid());
    EXPECT_EQ(ref.animator, InvalidAnimator);
}

TEST(AnimatorRefTest, AnimatorRefWithValidHandleIsValid) {
    AnimatorRef ref;
    ref.animator = 12345;
    EXPECT_TRUE(ref.isValid());
}

TEST(AnimatorRefTest, AnimatorRefWithInvalidAnimatorIsInvalid) {
    AnimatorRef ref;
    ref.animator = InvalidAnimator;
    EXPECT_FALSE(ref.isValid());
}

//==============================================================================
// LockPointDef Tests
//==============================================================================

TEST(LockPointDefTest, DefaultValues) {
    LockPointDef def;
    EXPECT_TRUE(def.name.empty());
    EXPECT_EQ(def.source, LockPointSource::Offset);
    EXPECT_TRUE(def.socketName.empty());
    EXPECT_FLOAT_EQ(def.localOffset.x, 0.0f);
    EXPECT_FLOAT_EQ(def.localOffset.y, 0.0f);
    EXPECT_FLOAT_EQ(def.localOffset.z, 0.0f);
    EXPECT_FLOAT_EQ(def.priority, 1.0f);
}

TEST(LockPointDefTest, OffsetSourceConfiguration) {
    LockPointDef def;
    def.name = "body_center";
    def.source = LockPointSource::Offset;
    def.localOffset = Vec3{0.0f, 1.5f, 0.0f};
    def.priority = 0.8f;

    EXPECT_EQ(def.source, LockPointSource::Offset);
    EXPECT_FLOAT_EQ(def.localOffset.y, 1.5f);
}

TEST(LockPointDefTest, SocketSourceConfiguration) {
    LockPointDef def;
    def.name = "head";
    def.source = LockPointSource::Socket;
    def.socketName = "head_socket";
    def.priority = 1.5f;

    EXPECT_EQ(def.source, LockPointSource::Socket);
    EXPECT_EQ(def.socketName, "head_socket");
}

//==============================================================================
// Edge Case Tests
//==============================================================================

TEST_F(LockOnSystemTest, MultipleLocksOnSameTarget) {
    Entity enemy = entitySystem_->createEntity();
    LockOnResult mockTarget = createMockTarget(
        enemy, Vec3{10.0f, 0.0f, -20.0f}, Vec2{400.0f, 300.0f}, 22.36f, 0.9f);
    graphics3D_->setMockPotentialTargets({mockTarget});

    LockOnResult result1 = graphics3D_->lockOn(*entitySystem_, nullptr);
    LockOnResult result2 = graphics3D_->lockOn(*entitySystem_, nullptr);

    EXPECT_EQ(result1.entity, result2.entity);
    EXPECT_TRUE(graphics3D_->isLocked());
}

TEST_F(LockOnSystemTest, LockOnAfterTargetRemoved) {
    Entity enemy = entitySystem_->createEntity();
    LockOnResult mockTarget = createMockTarget(
        enemy, Vec3{10.0f, 0.0f, -20.0f}, Vec2{400.0f, 300.0f}, 22.36f, 0.9f);
    graphics3D_->setMockPotentialTargets({mockTarget});
    graphics3D_->lockOn(*entitySystem_, nullptr);

    // Remove targets
    graphics3D_->setMockPotentialTargets({});

    // Poll should still return the cached position
    auto pos = graphics3D_->pollLockPosition(*entitySystem_, nullptr);
    EXPECT_TRUE(pos.has_value());
}

TEST_F(LockOnSystemTest, RapidLockUnlockCycles) {
    Entity enemy = entitySystem_->createEntity();
    LockOnResult mockTarget = createMockTarget(
        enemy, Vec3{10.0f, 0.0f, -20.0f}, Vec2{400.0f, 300.0f}, 22.36f, 0.9f);
    graphics3D_->setMockPotentialTargets({mockTarget});

    for (int i = 0; i < 100; ++i) {
        graphics3D_->lockOn(*entitySystem_, nullptr);
        EXPECT_TRUE(graphics3D_->isLocked());
        graphics3D_->unlock();
        EXPECT_FALSE(graphics3D_->isLocked());
    }
}

TEST_F(LockOnSystemTest, ShiftWithVariousDirections) {
    Entity enemy1 = entitySystem_->createEntity();
    Entity enemy2 = entitySystem_->createEntity();

    LockOnResult target1 = createMockTarget(
        enemy1, Vec3{10.0f, 0.0f, -20.0f}, Vec2{350.0f, 300.0f}, 22.36f, 0.9f);
    LockOnResult target2 = createMockTarget(
        enemy2, Vec3{-10.0f, 0.0f, -20.0f}, Vec2{450.0f, 300.0f}, 22.36f, 0.8f);
    graphics3D_->setMockPotentialTargets({target1, target2});
    graphics3D_->lockOn(*entitySystem_, nullptr);

    // Test various directions
    std::vector<Vec2> directions = {
        {1.0f, 0.0f},   // Right
        {-1.0f, 0.0f},  // Left
        {0.0f, 1.0f},   // Down
        {0.0f, -1.0f},  // Up
        {0.707f, 0.707f},  // Diagonal
    };

    for (const auto& dir : directions) {
        graphics3D_->shiftLockTarget(dir, *entitySystem_, nullptr);
        EXPECT_TRUE(graphics3D_->isLocked());
    }
}

}  // namespace bestow::tests

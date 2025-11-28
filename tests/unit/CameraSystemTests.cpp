// tests/unit/CameraSystemTests.cpp
// Camera system unit tests

#include <memory>
#include <cmath>

#include <entt/entity/entity.hpp>
#include <gtest/gtest.h>

import jframe.camera;
import jframe.camera.impl;
import jframe.types;

namespace jframe::tests {

class CameraSystemTest : public ::testing::Test {
protected:
    void SetUp() override {
        Size viewportSize{800, 600};
        cameraSystem_ = std::make_unique<CameraSystem>(viewportSize);
    }

    std::unique_ptr<CameraSystem> cameraSystem_;
};

// Basic state tests
TEST_F(CameraSystemTest, InitialState) {
    EXPECT_FLOAT_EQ(cameraSystem_->getZoom(), 1.0f);
    Vec2 pos = cameraSystem_->getPosition();
    EXPECT_FLOAT_EQ(pos.x, 0.0f);
    EXPECT_FLOAT_EQ(pos.y, 0.0f);
}

TEST_F(CameraSystemTest, SetZoom) {
    cameraSystem_->setZoom(2.0f);
    EXPECT_FLOAT_EQ(cameraSystem_->getZoom(), 2.0f);
}

TEST_F(CameraSystemTest, ZoomClamping) {
    cameraSystem_->setZoom(100.0f);
    EXPECT_LE(cameraSystem_->getZoom(), 10.0f);

    cameraSystem_->setZoom(-5.0f);
    EXPECT_GE(cameraSystem_->getZoom(), 0.1f);
}

// Target following tests
TEST_F(CameraSystemTest, SetTarget) {
    Entity entity = entt::entity{42};
    cameraSystem_->setTarget(entity);
    EXPECT_EQ(cameraSystem_->getTarget(), entity);
}

TEST_F(CameraSystemTest, ClearTarget) {
    Entity entity = entt::entity{42};
    cameraSystem_->setTarget(entity);
    cameraSystem_->clearTarget();
    EXPECT_TRUE(cameraSystem_->getTarget() == entt::null);
}

// Camera movement tests
TEST_F(CameraSystemTest, UpdateWithoutSmoothing) {
    cameraSystem_->setFollowSmoothing(0.0f);
    Vec2 targetPos{100.0f, 200.0f};

    cameraSystem_->update(0.016f, targetPos);

    Vec2 pos = cameraSystem_->getPosition();
    EXPECT_FLOAT_EQ(pos.x, targetPos.x);
    EXPECT_FLOAT_EQ(pos.y, targetPos.y);
}

TEST_F(CameraSystemTest, UpdateWithSmoothing) {
    cameraSystem_->setFollowSmoothing(0.9f);
    Vec2 targetPos{100.0f, 100.0f};

    // Update multiple times
    for (int i = 0; i < 10; ++i) {
        cameraSystem_->update(0.016f, targetPos);
    }

    Vec2 pos = cameraSystem_->getPosition();
    // With smoothing, camera should have moved but not reached target yet
    EXPECT_GT(pos.x, 0.0f);
    EXPECT_LT(pos.x, targetPos.x);
}

// Offset tests
TEST_F(CameraSystemTest, CameraOffset) {
    cameraSystem_->setFollowSmoothing(0.0f);
    Vec2 offset{50.0f, -50.0f};
    cameraSystem_->setOffset(offset);

    Vec2 targetPos{100.0f, 100.0f};
    cameraSystem_->update(0.016f, targetPos);

    Vec2 pos = cameraSystem_->getPosition();
    EXPECT_FLOAT_EQ(pos.x, targetPos.x + offset.x);
    EXPECT_FLOAT_EQ(pos.y, targetPos.y + offset.y);
}

// Deadzone tests
TEST_F(CameraSystemTest, DeadzoneNoMovementInsideBounds) {
    cameraSystem_->setFollowSmoothing(0.0f);
    cameraSystem_->setDeadzone(Vec2{100.0f, 100.0f});

    // Initial position
    Vec2 initialTarget{0.0f, 0.0f};
    cameraSystem_->update(0.016f, initialTarget);
    Vec2 initialPos = cameraSystem_->getPosition();

    // Move target slightly within deadzone
    Vec2 newTarget{20.0f, 20.0f};
    cameraSystem_->update(0.016f, newTarget);
    Vec2 newPos = cameraSystem_->getPosition();

    // Camera shouldn't move if target is within deadzone
    EXPECT_FLOAT_EQ(initialPos.x, newPos.x);
    EXPECT_FLOAT_EQ(initialPos.y, newPos.y);
}

TEST_F(CameraSystemTest, DeadzoneMovementOutsideBounds) {
    cameraSystem_->setFollowSmoothing(0.0f);
    cameraSystem_->setDeadzone(Vec2{100.0f, 100.0f});

    // Move target outside deadzone
    Vec2 farTarget{200.0f, 200.0f};
    cameraSystem_->update(0.016f, farTarget);

    Vec2 pos = cameraSystem_->getPosition();
    // Camera should have moved
    EXPECT_GT(pos.x, 0.0f);
    EXPECT_GT(pos.y, 0.0f);
}

// Bounds tests
TEST_F(CameraSystemTest, BoundsConstrainPosition) {
    cameraSystem_->setFollowSmoothing(0.0f);
    cameraSystem_->setBounds(0.0f, 1000.0f, 0.0f, 1000.0f);

    // Try to move camera beyond bounds
    Vec2 targetPos{2000.0f, 2000.0f};
    cameraSystem_->update(0.016f, targetPos);

    Vec2 pos = cameraSystem_->getPosition();
    EXPECT_LE(pos.x, 1000.0f);
    EXPECT_LE(pos.y, 1000.0f);
}

TEST_F(CameraSystemTest, ClearBounds) {
    cameraSystem_->setBounds(0.0f, 100.0f, 0.0f, 100.0f);
    cameraSystem_->clearBounds();

    // Camera should now be able to move freely
    cameraSystem_->setFollowSmoothing(0.0f);
    Vec2 targetPos{500.0f, 500.0f};
    cameraSystem_->update(0.016f, targetPos);

    Vec2 pos = cameraSystem_->getPosition();
    EXPECT_FLOAT_EQ(pos.x, targetPos.x);
    EXPECT_FLOAT_EQ(pos.y, targetPos.y);
}

// Shake tests
TEST_F(CameraSystemTest, CameraShake) {
    Vec2 initialPos = cameraSystem_->getPosition();

    cameraSystem_->shake(10.0f, 1.0f);
    cameraSystem_->update(0.016f, Vec2{0.0f, 0.0f});

    Camera camera = cameraSystem_->getCamera();
    // Camera position should be offset during shake
    // (shake affects the rendered camera, not the base position)
    EXPECT_TRUE(
        camera.transform.x != initialPos.x ||
        camera.transform.y != initialPos.y
    );
}

TEST_F(CameraSystemTest, ShakeDecaysOverTime) {
    cameraSystem_->shake(10.0f, 0.5f);

    // Get initial shake offset
    cameraSystem_->update(0.016f, Vec2{0.0f, 0.0f});
    Camera camera1 = cameraSystem_->getCamera();
    float initialOffset = std::abs(camera1.transform.x) + std::abs(camera1.transform.y);

    // Update until near end of shake
    for (int i = 0; i < 25; ++i) {
        cameraSystem_->update(0.016f, Vec2{0.0f, 0.0f});
    }

    Camera camera2 = cameraSystem_->getCamera();
    float laterOffset = std::abs(camera2.transform.x) + std::abs(camera2.transform.y);

    // Shake intensity should decrease over time
    EXPECT_LT(laterOffset, initialOffset);
}

TEST_F(CameraSystemTest, StopShake) {
    cameraSystem_->shake(10.0f, 1.0f);
    cameraSystem_->update(0.016f, Vec2{0.0f, 0.0f});

    cameraSystem_->stopShake();
    cameraSystem_->update(0.016f, Vec2{0.0f, 0.0f});

    Camera camera = cameraSystem_->getCamera();
    Vec2 pos = cameraSystem_->getPosition();
    EXPECT_FLOAT_EQ(camera.transform.x, pos.x);
    EXPECT_FLOAT_EQ(camera.transform.y, pos.y);
}

// Coordinate conversion tests
TEST_F(CameraSystemTest, ScreenToWorldConversion) {
    cameraSystem_->setFollowSmoothing(0.0f);
    cameraSystem_->update(0.016f, Vec2{100.0f, 100.0f});

    // Center of screen should map to camera position
    Vec2 screenCenter{400.0f, 300.0f};  // 800x600 viewport
    Vec2 worldPos = cameraSystem_->screenToWorld(screenCenter);

    Vec2 camPos = cameraSystem_->getPosition();
    EXPECT_NEAR(worldPos.x, camPos.x, 0.01f);
    EXPECT_NEAR(worldPos.y, camPos.y, 0.01f);
}

TEST_F(CameraSystemTest, WorldToScreenConversion) {
    cameraSystem_->setFollowSmoothing(0.0f);
    Vec2 cameraPos{100.0f, 100.0f};
    cameraSystem_->update(0.016f, cameraPos);

    // Camera position should map to screen center
    Vec2 screenPos = cameraSystem_->worldToScreen(cameraPos);

    EXPECT_NEAR(screenPos.x, 400.0f, 0.01f);  // Half of 800
    EXPECT_NEAR(screenPos.y, 300.0f, 0.01f);  // Half of 600
}

TEST_F(CameraSystemTest, CoordinateConversionRoundTrip) {
    Vec2 originalWorld{123.0f, 456.0f};

    Vec2 screen = cameraSystem_->worldToScreen(originalWorld);
    Vec2 backToWorld = cameraSystem_->screenToWorld(screen);

    EXPECT_NEAR(backToWorld.x, originalWorld.x, 0.01f);
    EXPECT_NEAR(backToWorld.y, originalWorld.y, 0.01f);
}

// Zoom with coordinate conversion
TEST_F(CameraSystemTest, ZoomAffectsConversion) {
    cameraSystem_->setZoom(2.0f);

    Vec2 worldPos{100.0f, 100.0f};
    Vec2 screenPos = cameraSystem_->worldToScreen(worldPos);
    Vec2 backToWorld = cameraSystem_->screenToWorld(screenPos);

    EXPECT_NEAR(backToWorld.x, worldPos.x, 0.01f);
    EXPECT_NEAR(backToWorld.y, worldPos.y, 0.01f);
}

// Camera object tests
TEST_F(CameraSystemTest, GetCameraReturnsCorrectZoom) {
    cameraSystem_->setZoom(3.0f);
    Camera camera = cameraSystem_->getCamera();
    EXPECT_FLOAT_EQ(camera.zoom, 3.0f);
}

TEST_F(CameraSystemTest, GetCameraReturnsCorrectViewportSize) {
    Camera camera = cameraSystem_->getCamera();
    EXPECT_EQ(camera.viewportSize.width, 800);
    EXPECT_EQ(camera.viewportSize.height, 600);
}

}  // namespace jframe::tests

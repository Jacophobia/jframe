// tests/unit/CameraSystemTests.cpp
// Camera system unit tests

#include <cmath>
#include <compare>
#include <iterator>
#include <memory>

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

// Additional tests for missing coverage

// setFollowSmoothing tests
TEST_F(CameraSystemTest, SetFollowSmoothingClamps) {
    // Test values outside valid range [0, 1]
    cameraSystem_->setFollowSmoothing(1.5f);
    // Can't directly query smoothing, but test its effect
    Vec2 targetPos{100.0f, 100.0f};
    cameraSystem_->update(0.016f, targetPos);
    Vec2 pos = cameraSystem_->getPosition();
    // With clamped max smoothing (1.0), camera should move very slowly
    EXPECT_LT(pos.x, targetPos.x);
    EXPECT_LT(pos.y, targetPos.y);
}

TEST_F(CameraSystemTest, SetFollowSmoothingNegativeClamps) {
    cameraSystem_->setFollowSmoothing(-0.5f);
    Vec2 targetPos{100.0f, 100.0f};
    cameraSystem_->update(0.016f, targetPos);
    Vec2 pos = cameraSystem_->getPosition();
    // Negative should clamp to 0.0 (instant movement)
    EXPECT_FLOAT_EQ(pos.x, targetPos.x);
    EXPECT_FLOAT_EQ(pos.y, targetPos.y);
}

// setOffset tests
TEST_F(CameraSystemTest, SetOffsetPersists) {
    cameraSystem_->setFollowSmoothing(0.0f);
    Vec2 offset1{50.0f, 50.0f};
    cameraSystem_->setOffset(offset1);

    Vec2 targetPos{100.0f, 100.0f};
    cameraSystem_->update(0.016f, targetPos);

    Vec2 pos1 = cameraSystem_->getPosition();

    // Change offset
    Vec2 offset2{-25.0f, -25.0f};
    cameraSystem_->setOffset(offset2);
    cameraSystem_->update(0.016f, targetPos);

    Vec2 pos2 = cameraSystem_->getPosition();

    // Position should change with offset
    EXPECT_NE(pos1.x, pos2.x);
    EXPECT_NE(pos1.y, pos2.y);
}

TEST_F(CameraSystemTest, SetOffsetNegativeValues) {
    cameraSystem_->setFollowSmoothing(0.0f);
    Vec2 offset{-100.0f, -200.0f};
    cameraSystem_->setOffset(offset);

    Vec2 targetPos{100.0f, 100.0f};
    cameraSystem_->update(0.016f, targetPos);

    Vec2 pos = cameraSystem_->getPosition();
    EXPECT_FLOAT_EQ(pos.x, targetPos.x + offset.x);
    EXPECT_FLOAT_EQ(pos.y, targetPos.y + offset.y);
}

// setDeadzone tests
TEST_F(CameraSystemTest, SetDeadzoneNegativeValues) {
    cameraSystem_->setFollowSmoothing(0.0f);
    // Negative values should be converted to absolute
    cameraSystem_->setDeadzone(Vec2{-100.0f, -100.0f});

    Vec2 initialTarget{0.0f, 0.0f};
    cameraSystem_->update(0.016f, initialTarget);

    // Move within (now positive) deadzone
    Vec2 newTarget{20.0f, 20.0f};
    cameraSystem_->update(0.016f, newTarget);

    Vec2 pos = cameraSystem_->getPosition();
    EXPECT_FLOAT_EQ(pos.x, 0.0f);
    EXPECT_FLOAT_EQ(pos.y, 0.0f);
}

TEST_F(CameraSystemTest, SetDeadzoneZeroSize) {
    cameraSystem_->setFollowSmoothing(0.0f);
    cameraSystem_->setDeadzone(Vec2{0.0f, 0.0f});

    Vec2 targetPos{50.0f, 50.0f};
    cameraSystem_->update(0.016f, targetPos);

    // With zero deadzone, camera should follow immediately
    Vec2 pos = cameraSystem_->getPosition();
    EXPECT_FLOAT_EQ(pos.x, targetPos.x);
    EXPECT_FLOAT_EQ(pos.y, targetPos.y);
}

TEST_F(CameraSystemTest, SetDeadzoneAsymmetric) {
    cameraSystem_->setFollowSmoothing(0.0f);
    // Different deadzone for X and Y
    cameraSystem_->setDeadzone(Vec2{200.0f, 50.0f});

    Vec2 initialTarget{0.0f, 0.0f};
    cameraSystem_->update(0.016f, initialTarget);

    // Move within Y deadzone but outside X deadzone
    Vec2 newTarget{150.0f, 20.0f};
    cameraSystem_->update(0.016f, newTarget);

    Vec2 pos = cameraSystem_->getPosition();
    // X should move, Y should not
    EXPECT_GT(pos.x, 0.0f);
    EXPECT_FLOAT_EQ(pos.y, 0.0f);
}

// setBounds tests
TEST_F(CameraSystemTest, SetBoundsInvertedCoordinates) {
    cameraSystem_->setFollowSmoothing(0.0f);
    // Test with inverted min/max (edge case)
    cameraSystem_->setBounds(1000.0f, 0.0f, 1000.0f, 0.0f);

    Vec2 targetPos{500.0f, 500.0f};
    cameraSystem_->update(0.016f, targetPos);

    // System should still apply the bounds (even if inverted)
    Vec2 pos = cameraSystem_->getPosition();
    // The exact behavior with inverted bounds may vary, but it shouldn't crash
    EXPECT_TRUE(std::isfinite(pos.x));
    EXPECT_TRUE(std::isfinite(pos.y));
}

TEST_F(CameraSystemTest, SetBoundsWithZoom) {
    cameraSystem_->setFollowSmoothing(0.0f);
    cameraSystem_->setZoom(2.0f);
    cameraSystem_->setBounds(0.0f, 500.0f, 0.0f, 500.0f);

    Vec2 targetPos{1000.0f, 1000.0f};
    cameraSystem_->update(0.016f, targetPos);

    Vec2 pos = cameraSystem_->getPosition();
    // With higher zoom, the effective bounds are tighter
    EXPECT_LE(pos.x, 500.0f);
    EXPECT_LE(pos.y, 500.0f);
}

TEST_F(CameraSystemTest, SetBoundsNearZero) {
    cameraSystem_->setFollowSmoothing(0.0f);
    // Very tight bounds
    cameraSystem_->setBounds(-10.0f, 10.0f, -10.0f, 10.0f);

    Vec2 targetPos{100.0f, 100.0f};
    cameraSystem_->update(0.016f, targetPos);

    Vec2 pos = cameraSystem_->getPosition();
    EXPECT_GE(pos.x, -10.0f);
    EXPECT_LE(pos.x, 10.0f);
    EXPECT_GE(pos.y, -10.0f);
    EXPECT_LE(pos.y, 10.0f);
}

// shake tests
TEST_F(CameraSystemTest, ShakeWithZeroDuration) {
    cameraSystem_->shake(10.0f, 0.0f);
    cameraSystem_->update(0.001f, Vec2{0.0f, 0.0f});

    Camera camera = cameraSystem_->getCamera();
    Vec2 pos = cameraSystem_->getPosition();
    // Should stop immediately with zero duration
    EXPECT_FLOAT_EQ(camera.transform.x, pos.x);
    EXPECT_FLOAT_EQ(camera.transform.y, pos.y);
}

TEST_F(CameraSystemTest, ShakeWithNegativeIntensity) {
    // Edge case: negative intensity
    cameraSystem_->shake(-10.0f, 1.0f);
    cameraSystem_->update(0.016f, Vec2{0.0f, 0.0f});

    Camera camera = cameraSystem_->getCamera();
    // Should still produce shake (implementation uses intensity as-is)
    // The test just verifies it doesn't crash
    EXPECT_TRUE(std::isfinite(camera.transform.x));
    EXPECT_TRUE(std::isfinite(camera.transform.y));
}

TEST_F(CameraSystemTest, ShakeMultipleCalls) {
    // Start a shake
    cameraSystem_->shake(5.0f, 1.0f);
    cameraSystem_->update(0.016f, Vec2{0.0f, 0.0f});

    // Immediately start another shake with different intensity
    cameraSystem_->shake(20.0f, 0.5f);
    cameraSystem_->update(0.016f, Vec2{0.0f, 0.0f});

    Camera camera = cameraSystem_->getCamera();
    // Second shake should override the first
    EXPECT_TRUE(std::isfinite(camera.transform.x));
    EXPECT_TRUE(std::isfinite(camera.transform.y));
}

// Zoom edge cases
TEST_F(CameraSystemTest, ZoomExactBoundaryValues) {
    cameraSystem_->setZoom(0.1f);
    EXPECT_FLOAT_EQ(cameraSystem_->getZoom(), 0.1f);

    cameraSystem_->setZoom(10.0f);
    EXPECT_FLOAT_EQ(cameraSystem_->getZoom(), 10.0f);
}

TEST_F(CameraSystemTest, ZoomVeryLargeValue) {
    cameraSystem_->setZoom(1000.0f);
    EXPECT_LE(cameraSystem_->getZoom(), 10.0f);
}

TEST_F(CameraSystemTest, ZoomNearZero) {
    cameraSystem_->setZoom(0.01f);
    EXPECT_GE(cameraSystem_->getZoom(), 0.1f);
}

TEST_F(CameraSystemTest, ZoomAffectsBounds) {
    cameraSystem_->setFollowSmoothing(0.0f);
    cameraSystem_->setBounds(0.0f, 1000.0f, 0.0f, 1000.0f);

    // Low zoom - wider view
    cameraSystem_->setZoom(0.5f);
    Vec2 targetPos{2000.0f, 2000.0f};
    cameraSystem_->update(0.016f, targetPos);
    Vec2 pos1 = cameraSystem_->getPosition();

    // High zoom - narrower view
    cameraSystem_->setZoom(5.0f);
    cameraSystem_->update(0.016f, targetPos);
    Vec2 pos2 = cameraSystem_->getPosition();

    // Different zoom should affect how bounds constrain the camera
    EXPECT_TRUE(pos1.x != pos2.x || pos1.y != pos2.y);
}

// Coordinate conversion edge cases
TEST_F(CameraSystemTest, ScreenToWorldNegativeCoordinates) {
    Vec2 screenPos{-100.0f, -100.0f};
    Vec2 worldPos = cameraSystem_->screenToWorld(screenPos);

    // Should handle negative screen coordinates
    EXPECT_TRUE(std::isfinite(worldPos.x));
    EXPECT_TRUE(std::isfinite(worldPos.y));
}

TEST_F(CameraSystemTest, WorldToScreenNegativeCoordinates) {
    Vec2 worldPos{-500.0f, -500.0f};
    Vec2 screenPos = cameraSystem_->worldToScreen(worldPos);

    // Should handle negative world coordinates
    EXPECT_TRUE(std::isfinite(screenPos.x));
    EXPECT_TRUE(std::isfinite(screenPos.y));
}

TEST_F(CameraSystemTest, CoordinateConversionWithOffset) {
    cameraSystem_->setFollowSmoothing(0.0f);
    cameraSystem_->setOffset(Vec2{100.0f, 100.0f});

    Vec2 targetPos{200.0f, 200.0f};
    cameraSystem_->update(0.016f, targetPos);

    // Test conversion still works with offset applied
    Vec2 originalWorld{150.0f, 150.0f};
    Vec2 screen = cameraSystem_->worldToScreen(originalWorld);
    Vec2 backToWorld = cameraSystem_->screenToWorld(screen);

    EXPECT_NEAR(backToWorld.x, originalWorld.x, 0.01f);
    EXPECT_NEAR(backToWorld.y, originalWorld.y, 0.01f);
}

TEST_F(CameraSystemTest, CoordinateConversionWithExtremeZoom) {
    cameraSystem_->setZoom(10.0f);

    Vec2 worldPos{1.0f, 1.0f};
    Vec2 screenPos = cameraSystem_->worldToScreen(worldPos);
    Vec2 backToWorld = cameraSystem_->screenToWorld(screenPos);

    EXPECT_NEAR(backToWorld.x, worldPos.x, 0.01f);
    EXPECT_NEAR(backToWorld.y, worldPos.y, 0.01f);
}

TEST_F(CameraSystemTest, CoordinateConversionAtViewportEdges) {
    // Top-left corner
    Vec2 topLeft{0.0f, 0.0f};
    Vec2 worldTopLeft = cameraSystem_->screenToWorld(topLeft);
    Vec2 screenTopLeft = cameraSystem_->worldToScreen(worldTopLeft);
    EXPECT_NEAR(screenTopLeft.x, topLeft.x, 0.01f);
    EXPECT_NEAR(screenTopLeft.y, topLeft.y, 0.01f);

    // Bottom-right corner
    Vec2 bottomRight{800.0f, 600.0f};
    Vec2 worldBottomRight = cameraSystem_->screenToWorld(bottomRight);
    Vec2 screenBottomRight = cameraSystem_->worldToScreen(worldBottomRight);
    EXPECT_NEAR(screenBottomRight.x, bottomRight.x, 0.01f);
    EXPECT_NEAR(screenBottomRight.y, bottomRight.y, 0.01f);
}

// Combined feature tests
TEST_F(CameraSystemTest, ShakeWithBounds) {
    cameraSystem_->setFollowSmoothing(0.0f);
    cameraSystem_->setBounds(0.0f, 100.0f, 0.0f, 100.0f);

    Vec2 targetPos{50.0f, 50.0f};
    cameraSystem_->update(0.016f, targetPos);

    // Shake should not push camera outside bounds
    cameraSystem_->shake(1000.0f, 1.0f);
    cameraSystem_->update(0.016f, targetPos);

    Camera camera = cameraSystem_->getCamera();
    // The shake affects the rendered camera, but base position should respect bounds
    Vec2 basePos = cameraSystem_->getPosition();
    EXPECT_GE(basePos.x, 0.0f);
    EXPECT_LE(basePos.x, 100.0f);
}

TEST_F(CameraSystemTest, DeadzoneWithOffset) {
    cameraSystem_->setFollowSmoothing(0.0f);
    cameraSystem_->setDeadzone(Vec2{50.0f, 50.0f});
    cameraSystem_->setOffset(Vec2{100.0f, 100.0f});

    Vec2 initialTarget{0.0f, 0.0f};
    cameraSystem_->update(0.016f, initialTarget);
    Vec2 initialPos = cameraSystem_->getPosition();

    // Small movement (within deadzone after offset applied)
    Vec2 newTarget{10.0f, 10.0f};
    cameraSystem_->update(0.016f, newTarget);
    Vec2 newPos = cameraSystem_->getPosition();

    // Deadzone should work with offset
    EXPECT_FLOAT_EQ(initialPos.x, newPos.x);
    EXPECT_FLOAT_EQ(initialPos.y, newPos.y);
}

TEST_F(CameraSystemTest, UpdateWithZeroDeltaTime) {
    cameraSystem_->setFollowSmoothing(0.5f);
    Vec2 targetPos{100.0f, 100.0f};

    Vec2 posBefore = cameraSystem_->getPosition();
    cameraSystem_->update(0.0f, targetPos);
    Vec2 posAfter = cameraSystem_->getPosition();

    // With zero dt, position might change but should not crash
    EXPECT_TRUE(std::isfinite(posAfter.x));
    EXPECT_TRUE(std::isfinite(posAfter.y));
}

TEST_F(CameraSystemTest, UpdateWithVeryLargeDeltaTime) {
    cameraSystem_->setFollowSmoothing(0.5f);
    Vec2 targetPos{100.0f, 100.0f};

    // Simulate a huge frame time spike
    cameraSystem_->update(10.0f, targetPos);

    Vec2 pos = cameraSystem_->getPosition();
    // Should converge to target with large dt
    EXPECT_NEAR(pos.x, targetPos.x, 1.0f);
    EXPECT_NEAR(pos.y, targetPos.y, 1.0f);
}

TEST_F(CameraSystemTest, GetCameraWithActiveShake) {
    cameraSystem_->shake(5.0f, 1.0f);
    cameraSystem_->update(0.016f, Vec2{0.0f, 0.0f});

    Camera camera = cameraSystem_->getCamera();

    // Verify camera struct has reasonable values during shake
    EXPECT_GT(camera.zoom, 0.0f);
    EXPECT_EQ(camera.viewportSize.width, 800);
    EXPECT_EQ(camera.viewportSize.height, 600);
    EXPECT_FLOAT_EQ(camera.transform.rotation, 0.0f);
    EXPECT_FLOAT_EQ(camera.transform.scaleX, 1.0f);
    EXPECT_FLOAT_EQ(camera.transform.scaleY, 1.0f);
}

TEST_F(CameraSystemTest, PositionNotAffectedByShake) {
    cameraSystem_->setFollowSmoothing(0.0f);
    Vec2 targetPos{100.0f, 100.0f};
    cameraSystem_->update(0.016f, targetPos);

    Vec2 posBeforeShake = cameraSystem_->getPosition();

    cameraSystem_->shake(10.0f, 1.0f);
    cameraSystem_->update(0.016f, targetPos);

    Vec2 posAfterShake = cameraSystem_->getPosition();

    // Base position should not be affected by shake
    EXPECT_FLOAT_EQ(posBeforeShake.x, posAfterShake.x);
    EXPECT_FLOAT_EQ(posBeforeShake.y, posAfterShake.y);
}

TEST_F(CameraSystemTest, TargetEntityDoesNotAffectUpdate) {
    // setTarget only stores the entity, doesn't affect update behavior
    Entity entity = entt::entity{42};
    cameraSystem_->setTarget(entity);

    cameraSystem_->setFollowSmoothing(0.0f);
    Vec2 targetPos{100.0f, 100.0f};
    cameraSystem_->update(0.016f, targetPos);

    Vec2 pos = cameraSystem_->getPosition();
    // Update uses the Vec2 parameter, not the entity
    EXPECT_FLOAT_EQ(pos.x, targetPos.x);
    EXPECT_FLOAT_EQ(pos.y, targetPos.y);
}

}  // namespace jframe::tests

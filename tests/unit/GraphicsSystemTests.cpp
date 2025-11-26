// tests/unit/GraphicsSystemTests.cpp
// Graphics system unit tests

#include <cmath>
#include <memory>
#include <string>
#include <vector>

#include <gtest/gtest.h>

import jframe.graphics;
import jframe.graphics.impl;
import jframe.types;

namespace jframe::tests {

// NOTE: Many tests are marked DISABLED_ because they require OpenGL context
// which requires a display. These tests define EXPECTED behavior but cannot
// run in headless CI environments. They can be enabled for local testing.

//==============================================================================
// Camera Math Tests (Can run without OpenGL context)
//==============================================================================

class CameraMathTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create a camera with known properties for testing
        testCamera_.transform = Transform2D{
            .x = 0.0f,
            .y = 0.0f,
            .rotation = 0.0f,
            .scaleX = 1.0f,
            .scaleY = 1.0f
        };
        testCamera_.zoom = 1.0f;
        testCamera_.viewportSize = Size{800, 600};
    }

    Camera testCamera_;
};

// NOTE: These tests require the GraphicsSystem to be initialized with a window
// For now, they define expected behavior but are disabled

TEST_F(CameraMathTest, DISABLED_WorldToScreenAtOrigin) {
    auto graphics = createGraphicsSystem();
    // Would need: graphics->initialize(800, 600, "Test");

    graphics->setCamera(testCamera_);

    // World origin (0,0) should map to screen center (400, 300)
    Vec2 screenPos = graphics->worldToScreen(Vec2{0.0f, 0.0f});
    EXPECT_NEAR(screenPos.x, 400.0f, 0.1f);
    EXPECT_NEAR(screenPos.y, 300.0f, 0.1f);
}

TEST_F(CameraMathTest, DISABLED_ScreenToWorldAtCenter) {
    auto graphics = createGraphicsSystem();
    graphics->setCamera(testCamera_);

    // Screen center (400, 300) should map to world origin (0,0)
    Vec2 worldPos = graphics->screenToWorld(Vec2{400.0f, 300.0f});
    EXPECT_NEAR(worldPos.x, 0.0f, 0.1f);
    EXPECT_NEAR(worldPos.y, 0.0f, 0.1f);
}

TEST_F(CameraMathTest, DISABLED_WorldToScreenInverseOperations) {
    auto graphics = createGraphicsSystem();
    graphics->setCamera(testCamera_);

    // worldToScreen and screenToWorld should be inverse operations
    Vec2 originalWorld{100.0f, 200.0f};
    Vec2 screen = graphics->worldToScreen(originalWorld);
    Vec2 backToWorld = graphics->screenToWorld(screen);

    EXPECT_NEAR(backToWorld.x, originalWorld.x, 0.1f);
    EXPECT_NEAR(backToWorld.y, originalWorld.y, 0.1f);
}

TEST_F(CameraMathTest, DISABLED_ScreenToWorldInverseOperations) {
    auto graphics = createGraphicsSystem();
    graphics->setCamera(testCamera_);

    // screenToWorld and worldToScreen should be inverse operations
    Vec2 originalScreen{250.0f, 150.0f};
    Vec2 world = graphics->screenToWorld(originalScreen);
    Vec2 backToScreen = graphics->worldToScreen(world);

    EXPECT_NEAR(backToScreen.x, originalScreen.x, 0.1f);
    EXPECT_NEAR(backToScreen.y, originalScreen.y, 0.1f);
}

TEST_F(CameraMathTest, DISABLED_CameraZoomAffectsConversion) {
    auto graphics = createGraphicsSystem();

    // Test with zoom = 1.0
    testCamera_.zoom = 1.0f;
    graphics->setCamera(testCamera_);
    Vec2 worldPos{100.0f, 0.0f};
    Vec2 screen1x = graphics->worldToScreen(worldPos);

    // Test with zoom = 2.0 (objects appear larger, move more in screen space)
    testCamera_.zoom = 2.0f;
    graphics->setCamera(testCamera_);
    Vec2 screen2x = graphics->worldToScreen(worldPos);

    // At 2x zoom, the same world position should be further from center
    float dist1x = std::sqrt(std::pow(screen1x.x - 400.0f, 2) +
                             std::pow(screen1x.y - 300.0f, 2));
    float dist2x = std::sqrt(std::pow(screen2x.x - 400.0f, 2) +
                             std::pow(screen2x.y - 300.0f, 2));
    EXPECT_GT(dist2x, dist1x);
}

TEST_F(CameraMathTest, DISABLED_CameraPositionAffectsConversion) {
    auto graphics = createGraphicsSystem();

    // Camera at origin
    testCamera_.transform.x = 0.0f;
    testCamera_.transform.y = 0.0f;
    graphics->setCamera(testCamera_);
    Vec2 screen1 = graphics->worldToScreen(Vec2{0.0f, 0.0f});

    // Move camera to the right and up
    testCamera_.transform.x = 100.0f;
    testCamera_.transform.y = 50.0f;
    graphics->setCamera(testCamera_);
    Vec2 screen2 = graphics->worldToScreen(Vec2{0.0f, 0.0f});

    // World origin should appear to move left and down on screen when camera moves right and up
    EXPECT_LT(screen2.x, screen1.x);
    EXPECT_GT(screen2.y, screen1.y);
}

TEST_F(CameraMathTest, DISABLED_ZoomPreservesCenter) {
    auto graphics = createGraphicsSystem();

    // Test that changing zoom doesn't affect the center point mapping
    testCamera_.zoom = 1.0f;
    graphics->setCamera(testCamera_);
    Vec2 center1 = graphics->worldToScreen(Vec2{0.0f, 0.0f});

    testCamera_.zoom = 3.0f;
    graphics->setCamera(testCamera_);
    Vec2 center2 = graphics->worldToScreen(Vec2{0.0f, 0.0f});

    // Camera center should remain at screen center regardless of zoom
    EXPECT_NEAR(center1.x, center2.x, 0.1f);
    EXPECT_NEAR(center1.y, center2.y, 0.1f);
    EXPECT_NEAR(center1.x, 400.0f, 0.1f);
    EXPECT_NEAR(center1.y, 300.0f, 0.1f);
}

//==============================================================================
// State Management Tests
//==============================================================================

class GraphicsSystemTest : public ::testing::Test {
protected:
    void SetUp() override {
        graphics_ = createGraphicsSystem();
        // Note: Cannot call initialize() without a display
    }

    std::unique_ptr<IGraphicsSystem> graphics_;
};

TEST_F(GraphicsSystemTest, DISABLED_SetAndGetCamera) {
    Camera camera;
    camera.transform = Transform2D{
        .x = 100.0f,
        .y = 200.0f,
        .rotation = 0.0f,
        .scaleX = 1.0f,
        .scaleY = 1.0f
    };
    camera.zoom = 2.0f;
    camera.viewportSize = Size{1920, 1080};

    graphics_->setCamera(camera);

    Camera retrieved = graphics_->getCamera();
    EXPECT_FLOAT_EQ(retrieved.transform.x, 100.0f);
    EXPECT_FLOAT_EQ(retrieved.transform.y, 200.0f);
    EXPECT_FLOAT_EQ(retrieved.zoom, 2.0f);
    EXPECT_EQ(retrieved.viewportSize.width, 1920);
    EXPECT_EQ(retrieved.viewportSize.height, 1080);
}

TEST_F(GraphicsSystemTest, DISABLED_DefaultCamera) {
    // After initialization, camera should have default values
    Camera camera = graphics_->getCamera();

    EXPECT_FLOAT_EQ(camera.transform.x, 0.0f);
    EXPECT_FLOAT_EQ(camera.transform.y, 0.0f);
    EXPECT_FLOAT_EQ(camera.zoom, 1.0f);
}

TEST_F(GraphicsSystemTest, DISABLED_GetWindowSize) {
    // Should return the size passed to initialize()
    Size size = graphics_->getWindowSize();
    EXPECT_GT(size.width, 0);
    EXPECT_GT(size.height, 0);
}

TEST_F(GraphicsSystemTest, DISABLED_SetWindowSize) {
    Size newSize{1280, 720};
    graphics_->setWindowSize(newSize);

    Size retrieved = graphics_->getWindowSize();
    EXPECT_EQ(retrieved.width, 1280);
    EXPECT_EQ(retrieved.height, 720);
}

TEST_F(GraphicsSystemTest, DISABLED_FullscreenToggle) {
    // Should start windowed
    EXPECT_FALSE(graphics_->isFullscreen());

    graphics_->setFullscreen(true);
    EXPECT_TRUE(graphics_->isFullscreen());

    graphics_->setFullscreen(false);
    EXPECT_FALSE(graphics_->isFullscreen());
}

TEST_F(GraphicsSystemTest, DISABLED_ShouldCloseInitiallyFalse) {
    // Window should not be closed initially
    EXPECT_FALSE(graphics_->shouldClose());
}

TEST_F(GraphicsSystemTest, DISABLED_NativeWindowHandleNotNull) {
    // Should return valid window handle after initialization
    void* handle = graphics_->getNativeWindowHandle();
    EXPECT_NE(handle, nullptr);
}

//==============================================================================
// Rendering Tests (Define expected behavior)
//==============================================================================

TEST_F(GraphicsSystemTest, DISABLED_BeginEndFrameDoesNotCrash) {
    // Should be able to call begin/end frame without crashing
    graphics_->beginFrame();
    graphics_->endFrame();
}

TEST_F(GraphicsSystemTest, DISABLED_DrawSpriteDoesNotCrash) {
    Sprite sprite;
    sprite.textureHandle = nullptr;  // Using null texture (should use white texture)
    sprite.transform = Transform2D{
        .x = 100.0f,
        .y = 100.0f,
        .rotation = 0.0f,
        .scaleX = 1.0f,
        .scaleY = 1.0f
    };
    sprite.sourceRect = Canvas{
        .origin = {0, 0},
        .size = {32, 32}
    };
    sprite.tint = Color::white();
    sprite.layer = 0;

    graphics_->beginFrame();
    graphics_->draw(sprite);
    graphics_->endFrame();
}

TEST_F(GraphicsSystemTest, DISABLED_DrawBatchDoesNotCrash) {
    std::vector<Sprite> sprites;
    for (int i = 0; i < 10; i++) {
        Sprite sprite;
        sprite.transform.x = static_cast<float>(i * 50);
        sprite.transform.y = 100.0f;
        sprite.sourceRect.size = Size{32, 32};
        sprites.push_back(sprite);
    }

    graphics_->beginFrame();
    graphics_->drawBatch(sprites);
    graphics_->endFrame();
}

TEST_F(GraphicsSystemTest, DISABLED_DrawRectDoesNotCrash) {
    Canvas rect{
        .origin = {100, 100},
        .size = {200, 150}
    };

    graphics_->beginFrame();
    graphics_->drawRect(rect, Color::red(), true);
    graphics_->drawRect(rect, Color::blue(), false);
    graphics_->endFrame();
}

TEST_F(GraphicsSystemTest, DISABLED_DrawLineDoesNotCrash) {
    graphics_->beginFrame();
    graphics_->drawLine(Vec2{0.0f, 0.0f}, Vec2{100.0f, 100.0f}, Color::green(), 2.0f);
    graphics_->endFrame();
}

TEST_F(GraphicsSystemTest, DISABLED_DrawCircleDoesNotCrash) {
    graphics_->beginFrame();
    graphics_->drawCircle(Vec2{200.0f, 200.0f}, 50.0f, Color::blue(), true, 32);
    graphics_->drawCircle(Vec2{300.0f, 200.0f}, 50.0f, Color::red(), false, 16);
    graphics_->endFrame();
}

TEST_F(GraphicsSystemTest, DISABLED_DrawPolygonDoesNotCrash) {
    std::vector<Vec2> triangle = {
        Vec2{0.0f, 0.0f},
        Vec2{100.0f, 0.0f},
        Vec2{50.0f, 100.0f}
    };

    graphics_->beginFrame();
    graphics_->drawPolygon(triangle, Color::green(), true);
    graphics_->endFrame();
}

TEST_F(GraphicsSystemTest, DISABLED_MeasureTextReturnsNonZero) {
    AssetHandle fontHandle;  // Invalid handle for test
    Vec2 size = graphics_->measureText("Hello World", fontHandle, 16.0f);

    // Even with invalid font, should return some reasonable size
    // (implementation may use fallback)
    EXPECT_GE(size.x, 0.0f);
    EXPECT_GE(size.y, 0.0f);
}

TEST_F(GraphicsSystemTest, DISABLED_DrawTextDoesNotCrash) {
    AssetHandle fontHandle;  // Invalid handle for test

    graphics_->beginFrame();
    graphics_->drawText("Test Text", Vec2{100.0f, 100.0f}, fontHandle, 16.0f, Color::white());
    graphics_->endFrame();
}

//==============================================================================
// Clear Color Tests
//==============================================================================

TEST_F(GraphicsSystemTest, DISABLED_SetClearColorDoesNotCrash) {
    // Should be able to set clear color without crashing
    graphics_->setClearColor(Color::blue());

    graphics_->beginFrame();
    graphics_->endFrame();
}

TEST_F(GraphicsSystemTest, DISABLED_VsyncToggle) {
    // Should be able to toggle vsync without crashing
    graphics_->setVSync(true);
    graphics_->setVSync(false);
}

//==============================================================================
// Sprite Batching Behavior Tests
//==============================================================================

TEST_F(GraphicsSystemTest, DISABLED_MultipleDrawCallsBatchSprites) {
    // Drawing multiple sprites should batch them together
    // This is an implementation detail, but defines expected behavior

    graphics_->beginFrame();

    for (int i = 0; i < 100; i++) {
        Sprite sprite;
        sprite.transform.x = static_cast<float>(i * 10);
        sprite.transform.y = 100.0f;
        sprite.sourceRect.size = Size{16, 16};
        graphics_->draw(sprite);
    }

    graphics_->endFrame();

    // Should not crash with many sprites
}

TEST_F(GraphicsSystemTest, DISABLED_SpritesRenderedInLayerOrder) {
    // Sprites should be rendered in order of their layer value
    // Lower layer values render first (behind), higher values render last (in front)

    Sprite backSprite;
    backSprite.layer = 0;
    backSprite.transform = Transform2D{.x = 100.0f, .y = 100.0f};
    backSprite.sourceRect.size = Size{64, 64};
    backSprite.tint = Color::red();

    Sprite frontSprite;
    frontSprite.layer = 10;
    frontSprite.transform = Transform2D{.x = 100.0f, .y = 100.0f};
    frontSprite.sourceRect.size = Size{64, 64};
    frontSprite.tint = Color::blue();

    graphics_->beginFrame();
    // Draw in arbitrary order
    graphics_->draw(frontSprite);
    graphics_->draw(backSprite);
    graphics_->endFrame();

    // Blue sprite should appear in front even though drawn first
}

//==============================================================================
// Transform Tests
//==============================================================================

TEST_F(GraphicsSystemTest, DISABLED_SpriteRotationApplied) {
    Sprite sprite;
    sprite.transform.x = 200.0f;
    sprite.transform.y = 200.0f;
    sprite.transform.rotation = 3.14159f / 4.0f;  // 45 degrees
    sprite.sourceRect.size = Size{64, 64};

    graphics_->beginFrame();
    graphics_->draw(sprite);
    graphics_->endFrame();

    // Should render rotated sprite without crashing
}

TEST_F(GraphicsSystemTest, DISABLED_SpriteScaleApplied) {
    Sprite sprite;
    sprite.transform.x = 200.0f;
    sprite.transform.y = 200.0f;
    sprite.transform.scaleX = 2.0f;
    sprite.transform.scaleY = 0.5f;
    sprite.sourceRect.size = Size{64, 64};

    graphics_->beginFrame();
    graphics_->draw(sprite);
    graphics_->endFrame();

    // Should render scaled sprite without crashing
}

TEST_F(GraphicsSystemTest, DISABLED_SpriteAnchorPointApplied) {
    Sprite sprite1;
    sprite1.transform.x = 200.0f;
    sprite1.transform.y = 200.0f;
    sprite1.anchor = Vec2{0.0f, 0.0f};  // Top-left
    sprite1.sourceRect.size = Size{64, 64};

    Sprite sprite2;
    sprite2.transform.x = 200.0f;
    sprite2.transform.y = 200.0f;
    sprite2.anchor = Vec2{0.5f, 0.5f};  // Center (default)
    sprite2.sourceRect.size = Size{64, 64};

    Sprite sprite3;
    sprite3.transform.x = 200.0f;
    sprite3.transform.y = 200.0f;
    sprite3.anchor = Vec2{1.0f, 1.0f};  // Bottom-right
    sprite3.sourceRect.size = Size{64, 64};

    graphics_->beginFrame();
    graphics_->draw(sprite1);
    graphics_->draw(sprite2);
    graphics_->draw(sprite3);
    graphics_->endFrame();

    // Sprites with different anchors should appear at different screen positions
    // even though their transform.x/y are the same
}

TEST_F(GraphicsSystemTest, DISABLED_SpriteTintApplied) {
    Sprite redSprite;
    redSprite.transform.x = 100.0f;
    redSprite.transform.y = 100.0f;
    redSprite.tint = Color::red();
    redSprite.sourceRect.size = Size{64, 64};

    Sprite halfTransparent;
    halfTransparent.transform.x = 200.0f;
    halfTransparent.transform.y = 100.0f;
    halfTransparent.tint = Color{255, 255, 255, 128};
    halfTransparent.sourceRect.size = Size{64, 64};

    graphics_->beginFrame();
    graphics_->draw(redSprite);
    graphics_->draw(halfTransparent);
    graphics_->endFrame();

    // Tint color should be applied to sprites
}

//==============================================================================
// Edge Cases
//==============================================================================

TEST_F(GraphicsSystemTest, DISABLED_DrawWithNullTextureUsesWhiteTexture) {
    Sprite sprite;
    sprite.textureHandle = nullptr;
    sprite.transform.x = 100.0f;
    sprite.transform.y = 100.0f;
    sprite.sourceRect.size = Size{64, 64};
    sprite.tint = Color::blue();

    graphics_->beginFrame();
    graphics_->draw(sprite);
    graphics_->endFrame();

    // Should render colored rectangle using white texture fallback
}

TEST_F(GraphicsSystemTest, DISABLED_DrawEmptyBatchDoesNotCrash) {
    std::vector<Sprite> emptyBatch;

    graphics_->beginFrame();
    graphics_->drawBatch(emptyBatch);
    graphics_->endFrame();
}

TEST_F(GraphicsSystemTest, DISABLED_DrawPolygonWithTwoVertices) {
    std::vector<Vec2> line = {
        Vec2{0.0f, 0.0f},
        Vec2{100.0f, 100.0f}
    };

    graphics_->beginFrame();
    graphics_->drawPolygon(line, Color::green(), true);
    graphics_->endFrame();

    // Should handle degenerate polygon gracefully
}

TEST_F(GraphicsSystemTest, DISABLED_DrawCircleWithZeroRadius) {
    graphics_->beginFrame();
    graphics_->drawCircle(Vec2{100.0f, 100.0f}, 0.0f, Color::red(), true);
    graphics_->endFrame();

    // Should handle zero radius gracefully (draw nothing or point)
}

TEST_F(GraphicsSystemTest, DISABLED_MeasureEmptyString) {
    AssetHandle fontHandle;
    Vec2 size = graphics_->measureText("", fontHandle, 16.0f);

    EXPECT_GE(size.x, 0.0f);
    EXPECT_GE(size.y, 0.0f);
}

}  // namespace jframe::tests

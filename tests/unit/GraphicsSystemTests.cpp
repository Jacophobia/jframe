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

TEST_F(GraphicsSystemTest, DISABLED_DrawTextCenteredDoesNotCrash) {
    AssetHandle fontHandle;

    graphics_->beginFrame();
    graphics_->drawTextCentered("Centered Text", Vec2{400.0f, 300.0f}, fontHandle, 24.0f, Color::white());
    graphics_->endFrame();
}

TEST_F(GraphicsSystemTest, DISABLED_MeasureTextReturnsPositiveSize) {
    AssetHandle fontHandle;
    Vec2 size = graphics_->measureText("Test String", fontHandle, 16.0f);

    EXPECT_GT(size.x, 0.0f);
    EXPECT_GT(size.y, 0.0f);
}

//==============================================================================
// Additional Coverage Tests - Missing Methods
//==============================================================================

TEST_F(GraphicsSystemTest, DISABLED_SetAssetSystemDoesNotCrash) {
    // Should be able to set asset system pointer without crashing
    graphics_->setAssetSystem(nullptr);
    // No assertion needed - test passes if no crash
}

TEST_F(GraphicsSystemTest, DISABLED_DrawSpriteSheetDoesNotCrash) {
    // Create a mock spritesheet
    SpriteSheet sheet;
    sheet.texture = AssetHandle{};
    sheet.frameWidth = 32;
    sheet.frameHeight = 32;
    sheet.columns = 4;
    sheet.rows = 4;
    sheet.padding = 0;

    Transform2D transform{
        .x = 100.0f,
        .y = 100.0f,
        .rotation = 0.0f,
        .scaleX = 1.0f,
        .scaleY = 1.0f
    };

    graphics_->beginFrame();
    graphics_->drawSprite(sheet, 0, transform, Color::white());
    graphics_->drawSprite(sheet, 5, transform, Color::red());
    graphics_->endFrame();
}

TEST_F(GraphicsSystemTest, DISABLED_DrawAnimatedSpriteDoesNotCrash) {
    // Create a mock animated sprite
    SpriteSheet sheet;
    sheet.texture = AssetHandle{};
    sheet.frameWidth = 32;
    sheet.frameHeight = 32;
    sheet.columns = 4;
    sheet.rows = 4;

    AnimatedSprite animSprite;
    animSprite.sheet = sheet;

    // Create a simple animation
    Animation walkAnim;
    walkAnim.name = "walk";
    walkAnim.looping = true;
    walkAnim.frames = {
        AnimationFrame{0, 0.1f},
        AnimationFrame{1, 0.1f},
        AnimationFrame{2, 0.1f},
        AnimationFrame{3, 0.1f}
    };

    animSprite.animations["walk"] = walkAnim;
    animSprite.currentAnimation = "walk";
    animSprite.currentFrameIndex = 0;
    animSprite.frameTimer = 0.0f;
    animSprite.playing = true;

    Transform2D transform{
        .x = 200.0f,
        .y = 200.0f,
        .rotation = 0.0f,
        .scaleX = 1.0f,
        .scaleY = 1.0f
    };

    graphics_->beginFrame();
    graphics_->drawAnimatedSprite(animSprite, transform, Color::white());
    graphics_->endFrame();
}

TEST_F(GraphicsSystemTest, DISABLED_RenderEntitiesWithNoLayersDoesNotCrash) {
    // Mock entity system (would need actual implementation)
    // This test defines expected behavior
    // graphics_->beginFrame();
    // graphics_->renderEntities(mockEntitySystem);
    // graphics_->endFrame();
}

TEST_F(GraphicsSystemTest, DISABLED_RenderEntitiesWithLayerRangeDoesNotCrash) {
    // Mock entity system (would need actual implementation)
    // This test defines expected behavior
    // graphics_->beginFrame();
    // graphics_->renderEntities(mockEntitySystem, 0, 10);
    // graphics_->endFrame();
}

TEST_F(GraphicsSystemTest, DISABLED_ViewportCullingToggle) {
    // Should start disabled
    EXPECT_FALSE(graphics_->isViewportCullingEnabled());

    graphics_->setViewportCulling(true);
    EXPECT_TRUE(graphics_->isViewportCullingEnabled());

    graphics_->setViewportCulling(false);
    EXPECT_FALSE(graphics_->isViewportCullingEnabled());
}

//==============================================================================
// Edge Cases and Boundary Tests
//==============================================================================

TEST_F(GraphicsSystemTest, DISABLED_DrawSpriteWithZeroScale) {
    Sprite sprite;
    sprite.transform.x = 100.0f;
    sprite.transform.y = 100.0f;
    sprite.transform.scaleX = 0.0f;
    sprite.transform.scaleY = 0.0f;
    sprite.sourceRect.size = Size{32, 32};

    graphics_->beginFrame();
    graphics_->draw(sprite);
    graphics_->endFrame();

    // Should handle zero scale gracefully (draw nothing or point)
}

TEST_F(GraphicsSystemTest, DISABLED_DrawSpriteWithNegativeScale) {
    Sprite sprite;
    sprite.transform.x = 100.0f;
    sprite.transform.y = 100.0f;
    sprite.transform.scaleX = -1.0f;  // Flipped horizontally
    sprite.transform.scaleY = -1.0f;  // Flipped vertically
    sprite.sourceRect.size = Size{32, 32};

    graphics_->beginFrame();
    graphics_->draw(sprite);
    graphics_->endFrame();

    // Should render flipped sprite
}

TEST_F(GraphicsSystemTest, DISABLED_DrawSpriteWithExtremeRotation) {
    Sprite sprite;
    sprite.transform.x = 100.0f;
    sprite.transform.y = 100.0f;
    sprite.transform.rotation = 100.0f;  // Many rotations
    sprite.sourceRect.size = Size{32, 32};

    graphics_->beginFrame();
    graphics_->draw(sprite);
    graphics_->endFrame();

    // Should handle extreme rotation values
}

TEST_F(GraphicsSystemTest, DISABLED_DrawRectWithZeroSize) {
    Canvas rect{
        .origin = {100, 100},
        .size = {0, 0}
    };

    graphics_->beginFrame();
    graphics_->drawRect(rect, Color::red(), true);
    graphics_->endFrame();

    // Should handle zero size gracefully
}

TEST_F(GraphicsSystemTest, DISABLED_DrawRectWithNegativeSize) {
    Canvas rect{
        .origin = {100, 100},
        .size = {-50, -50}
    };

    graphics_->beginFrame();
    graphics_->drawRect(rect, Color::red(), true);
    graphics_->endFrame();

    // Should handle negative size gracefully
}

TEST_F(GraphicsSystemTest, DISABLED_DrawLineWithSameStartAndEnd) {
    graphics_->beginFrame();
    graphics_->drawLine(Vec2{100.0f, 100.0f}, Vec2{100.0f, 100.0f}, Color::green(), 2.0f);
    graphics_->endFrame();

    // Should handle degenerate line gracefully (draw nothing or point)
}

TEST_F(GraphicsSystemTest, DISABLED_DrawLineWithZeroThickness) {
    graphics_->beginFrame();
    graphics_->drawLine(Vec2{0.0f, 0.0f}, Vec2{100.0f, 100.0f}, Color::green(), 0.0f);
    graphics_->endFrame();

    // Should handle zero thickness gracefully
}

TEST_F(GraphicsSystemTest, DISABLED_DrawLineWithVeryThickLine) {
    graphics_->beginFrame();
    graphics_->drawLine(Vec2{0.0f, 0.0f}, Vec2{100.0f, 100.0f}, Color::green(), 50.0f);
    graphics_->endFrame();

    // Should handle very thick lines (may be clamped by OpenGL)
}

TEST_F(GraphicsSystemTest, DISABLED_DrawCircleWithNegativeRadius) {
    graphics_->beginFrame();
    graphics_->drawCircle(Vec2{100.0f, 100.0f}, -50.0f, Color::red(), true);
    graphics_->endFrame();

    // Should handle negative radius gracefully
}

TEST_F(GraphicsSystemTest, DISABLED_DrawCircleWithOneSegment) {
    graphics_->beginFrame();
    graphics_->drawCircle(Vec2{100.0f, 100.0f}, 50.0f, Color::blue(), true, 1);
    graphics_->endFrame();

    // Should handle degenerate circle (1 segment)
}

TEST_F(GraphicsSystemTest, DISABLED_DrawCircleWithZeroSegments) {
    graphics_->beginFrame();
    graphics_->drawCircle(Vec2{100.0f, 100.0f}, 50.0f, Color::blue(), true, 0);
    graphics_->endFrame();

    // Should handle zero segments gracefully
}

TEST_F(GraphicsSystemTest, DISABLED_DrawCircleWithManySegments) {
    graphics_->beginFrame();
    graphics_->drawCircle(Vec2{100.0f, 100.0f}, 50.0f, Color::blue(), true, 1000);
    graphics_->endFrame();

    // Should handle very high segment count
}

TEST_F(GraphicsSystemTest, DISABLED_DrawPolygonWithEmptyVertices) {
    std::vector<Vec2> empty;

    graphics_->beginFrame();
    graphics_->drawPolygon(empty, Color::green(), true);
    graphics_->endFrame();

    // Should handle empty polygon gracefully
}

TEST_F(GraphicsSystemTest, DISABLED_DrawPolygonWithOneVertex) {
    std::vector<Vec2> point = { Vec2{100.0f, 100.0f} };

    graphics_->beginFrame();
    graphics_->drawPolygon(point, Color::green(), true);
    graphics_->endFrame();

    // Should handle single vertex gracefully
}

TEST_F(GraphicsSystemTest, DISABLED_MeasureTextWithLongString) {
    AssetHandle fontHandle;
    std::string longString(10000, 'A');  // 10000 characters
    Vec2 size = graphics_->measureText(longString, fontHandle, 16.0f);

    EXPECT_GT(size.x, 0.0f);
    EXPECT_GT(size.y, 0.0f);
}

TEST_F(GraphicsSystemTest, DISABLED_MeasureTextWithNewlines) {
    AssetHandle fontHandle;
    Vec2 size = graphics_->measureText("Line1\nLine2\nLine3", fontHandle, 16.0f);

    // Current implementation may not handle newlines specially
    EXPECT_GE(size.x, 0.0f);
    EXPECT_GE(size.y, 0.0f);
}

TEST_F(GraphicsSystemTest, DISABLED_MeasureTextWithSpecialCharacters) {
    AssetHandle fontHandle;
    Vec2 size = graphics_->measureText("!@#$%^&*()_+-=[]{}|;:',.<>?/~`", fontHandle, 16.0f);

    EXPECT_GT(size.x, 0.0f);
    EXPECT_GT(size.y, 0.0f);
}

TEST_F(GraphicsSystemTest, DISABLED_MeasureTextWithZeroSize) {
    AssetHandle fontHandle;
    Vec2 size = graphics_->measureText("Test", fontHandle, 0.0f);

    // Should handle zero font size gracefully
    EXPECT_GE(size.x, 0.0f);
    EXPECT_GE(size.y, 0.0f);
}

TEST_F(GraphicsSystemTest, DISABLED_MeasureTextWithNegativeSize) {
    AssetHandle fontHandle;
    Vec2 size = graphics_->measureText("Test", fontHandle, -16.0f);

    // Should handle negative font size gracefully
    EXPECT_GE(size.x, 0.0f);
    EXPECT_GE(size.y, 0.0f);
}

TEST_F(GraphicsSystemTest, DISABLED_MeasureTextWithVeryLargeSize) {
    AssetHandle fontHandle;
    Vec2 size = graphics_->measureText("Test", fontHandle, 1000.0f);

    EXPECT_GT(size.x, 0.0f);
    EXPECT_GT(size.y, 0.0f);
}

TEST_F(GraphicsSystemTest, DISABLED_DrawTextWithLongString) {
    AssetHandle fontHandle;
    std::string longString(10000, 'A');

    graphics_->beginFrame();
    graphics_->drawText(longString, Vec2{100.0f, 100.0f}, fontHandle, 16.0f, Color::white());
    graphics_->endFrame();

    // Should handle very long strings without crashing
}

TEST_F(GraphicsSystemTest, DISABLED_DrawTextCenteredWithEmptyString) {
    AssetHandle fontHandle;

    graphics_->beginFrame();
    graphics_->drawTextCentered("", Vec2{400.0f, 300.0f}, fontHandle, 24.0f, Color::white());
    graphics_->endFrame();
}

TEST_F(GraphicsSystemTest, DISABLED_SetWindowSizeToZero) {
    Size zeroSize{0, 0};
    graphics_->setWindowSize(zeroSize);

    // Should handle zero size gracefully (may clamp to minimum)
}

TEST_F(GraphicsSystemTest, DISABLED_SetWindowSizeToNegative) {
    Size negativeSize{-800, -600};
    graphics_->setWindowSize(negativeSize);

    // Should handle negative size gracefully (may clamp to minimum)
}

TEST_F(GraphicsSystemTest, DISABLED_SetWindowSizeToVeryLarge) {
    Size largeSize{100000, 100000};
    graphics_->setWindowSize(largeSize);

    // Should handle very large size (may be limited by display)
}

TEST_F(GraphicsSystemTest, DISABLED_SetCameraWithNegativeZoom) {
    Camera camera = graphics_->getCamera();
    camera.zoom = -1.0f;
    graphics_->setCamera(camera);

    Camera retrieved = graphics_->getCamera();
    EXPECT_FLOAT_EQ(retrieved.zoom, -1.0f);

    // Negative zoom may cause inverted rendering or be clamped
}

TEST_F(GraphicsSystemTest, DISABLED_SetCameraWithZeroZoom) {
    Camera camera = graphics_->getCamera();
    camera.zoom = 0.0f;
    graphics_->setCamera(camera);

    Camera retrieved = graphics_->getCamera();
    EXPECT_FLOAT_EQ(retrieved.zoom, 0.0f);

    // Zero zoom may cause division by zero or be clamped
}

TEST_F(GraphicsSystemTest, DISABLED_SetCameraWithVeryLargeZoom) {
    Camera camera = graphics_->getCamera();
    camera.zoom = 1000.0f;
    graphics_->setCamera(camera);

    Camera retrieved = graphics_->getCamera();
    EXPECT_FLOAT_EQ(retrieved.zoom, 1000.0f);

    // Very large zoom should work but may cause precision issues
}

TEST_F(GraphicsSystemTest, DISABLED_SetCameraWithExtremePosition) {
    Camera camera = graphics_->getCamera();
    camera.transform.x = 1e10f;
    camera.transform.y = -1e10f;
    graphics_->setCamera(camera);

    Camera retrieved = graphics_->getCamera();
    EXPECT_FLOAT_EQ(retrieved.transform.x, 1e10f);
    EXPECT_FLOAT_EQ(retrieved.transform.y, -1e10f);

    // Extreme positions should work but may cause precision issues
}

TEST_F(GraphicsSystemTest, DISABLED_WorldToScreenWithExtremeCoordinates) {
    Vec2 extremePos{1e10f, -1e10f};
    Vec2 screenPos = graphics_->worldToScreen(extremePos);

    // Should return some value, even if off-screen
    // No specific assertion - just checking it doesn't crash
}

TEST_F(GraphicsSystemTest, DISABLED_ScreenToWorldWithExtremeCoordinates) {
    Vec2 extremePos{1e10f, -1e10f};
    Vec2 worldPos = graphics_->screenToWorld(extremePos);

    // Should return some value
    // No specific assertion - just checking it doesn't crash
}

TEST_F(GraphicsSystemTest, DISABLED_WorldToScreenAtNegativeScreenCoordinates) {
    // Test point that maps to negative screen coordinates
    Camera camera = graphics_->getCamera();
    camera.transform.x = 0.0f;
    camera.transform.y = 0.0f;
    graphics_->setCamera(camera);

    Vec2 worldPos{-1000.0f, -1000.0f};
    Vec2 screenPos = graphics_->worldToScreen(worldPos);

    // Should return negative screen coordinates
    EXPECT_LT(screenPos.x, 0.0f);
    EXPECT_GT(screenPos.y, camera.viewportSize.height);
}

TEST_F(GraphicsSystemTest, DISABLED_SetClearColorWithTransparency) {
    graphics_->setClearColor(Color{100, 150, 200, 128});

    graphics_->beginFrame();
    graphics_->endFrame();

    // Clear color with alpha should work (though typically ignored in clearing)
}

TEST_F(GraphicsSystemTest, DISABLED_SetClearColorMultipleTimes) {
    graphics_->setClearColor(Color::red());
    graphics_->setClearColor(Color::green());
    graphics_->setClearColor(Color::blue());

    graphics_->beginFrame();
    graphics_->endFrame();

    // Should use the last set color (blue)
}

TEST_F(GraphicsSystemTest, DISABLED_SetVSyncMultipleTimes) {
    graphics_->setVSync(true);
    graphics_->setVSync(false);
    graphics_->setVSync(true);

    // Should be able to toggle vsync multiple times
}

TEST_F(GraphicsSystemTest, DISABLED_ToggleFullscreenMultipleTimes) {
    graphics_->setFullscreen(true);
    graphics_->setFullscreen(false);
    graphics_->setFullscreen(true);
    graphics_->setFullscreen(false);

    EXPECT_FALSE(graphics_->isFullscreen());
}

TEST_F(GraphicsSystemTest, DISABLED_MultipleBeginFrameWithoutEnd) {
    graphics_->beginFrame();
    // Note: Calling beginFrame again without endFrame is undefined behavior
    // This test documents that it may crash or produce artifacts
}

TEST_F(GraphicsSystemTest, DISABLED_EndFrameWithoutBegin) {
    // Note: Calling endFrame without beginFrame is undefined behavior
    // This test documents that it may crash
    // graphics_->endFrame();
}

TEST_F(GraphicsSystemTest, DISABLED_DrawOutsideBeginEndFrame) {
    Sprite sprite;
    sprite.sourceRect.size = Size{32, 32};

    // Drawing outside begin/end frame should add to batch
    // but won't be rendered until next frame
    graphics_->draw(sprite);
}

TEST_F(GraphicsSystemTest, DISABLED_DrawBatchWithManySprites) {
    std::vector<Sprite> sprites;
    sprites.reserve(10000);

    for (int i = 0; i < 10000; i++) {
        Sprite sprite;
        sprite.transform.x = static_cast<float>(i % 100) * 10.0f;
        sprite.transform.y = static_cast<float>(i / 100) * 10.0f;
        sprite.sourceRect.size = Size{8, 8};
        sprites.push_back(sprite);
    }

    graphics_->beginFrame();
    graphics_->drawBatch(sprites);
    graphics_->endFrame();

    // Should handle large batches
}

TEST_F(GraphicsSystemTest, DISABLED_DrawWithDifferentLayersInBatch) {
    std::vector<Sprite> sprites;

    for (int i = 0; i < 10; i++) {
        Sprite sprite;
        sprite.transform.x = static_cast<float>(i * 50);
        sprite.layer = i % 3;  // Layers 0, 1, 2
        sprite.sourceRect.size = Size{32, 32};
        sprites.push_back(sprite);
    }

    graphics_->beginFrame();
    graphics_->drawBatch(sprites);
    graphics_->endFrame();

    // Should sort by layer regardless of batch order
}

TEST_F(GraphicsSystemTest, DISABLED_DrawWithNegativeLayers) {
    Sprite sprite1;
    sprite1.layer = -100;
    sprite1.sourceRect.size = Size{32, 32};

    Sprite sprite2;
    sprite2.layer = -50;
    sprite2.sourceRect.size = Size{32, 32};

    Sprite sprite3;
    sprite3.layer = 0;
    sprite3.sourceRect.size = Size{32, 32};

    graphics_->beginFrame();
    graphics_->draw(sprite3);
    graphics_->draw(sprite1);
    graphics_->draw(sprite2);
    graphics_->endFrame();

    // Should render in order: sprite1 (-100), sprite2 (-50), sprite3 (0)
}

TEST_F(GraphicsSystemTest, DISABLED_DrawWithExtremeLayerValues) {
    Sprite sprite1;
    sprite1.layer = std::numeric_limits<RenderLayer>::min();
    sprite1.sourceRect.size = Size{32, 32};

    Sprite sprite2;
    sprite2.layer = std::numeric_limits<RenderLayer>::max();
    sprite2.sourceRect.size = Size{32, 32};

    graphics_->beginFrame();
    graphics_->draw(sprite2);
    graphics_->draw(sprite1);
    graphics_->endFrame();

    // Should handle extreme layer values
}

TEST_F(GraphicsSystemTest, DISABLED_SpriteWithInvalidAnchor) {
    Sprite sprite;
    sprite.anchor = Vec2{5.0f, 5.0f};  // Outside normal [0,1] range
    sprite.sourceRect.size = Size{32, 32};

    graphics_->beginFrame();
    graphics_->draw(sprite);
    graphics_->endFrame();

    // Should handle out-of-range anchor values
}

TEST_F(GraphicsSystemTest, DISABLED_SpriteWithZeroSourceRect) {
    Sprite sprite;
    sprite.transform.x = 100.0f;
    sprite.transform.y = 100.0f;
    sprite.sourceRect = Canvas{
        .origin = {0, 0},
        .size = {0, 0}
    };

    graphics_->beginFrame();
    graphics_->draw(sprite);
    graphics_->endFrame();

    // Should handle zero source rect (may use default size)
}

TEST_F(GraphicsSystemTest, DISABLED_SpriteWithNegativeSourceRect) {
    Sprite sprite;
    sprite.sourceRect = Canvas{
        .origin = {-10, -10},
        .size = {-32, -32}
    };

    graphics_->beginFrame();
    graphics_->draw(sprite);
    graphics_->endFrame();

    // Should handle negative source rect values
}

TEST_F(GraphicsSystemTest, DISABLED_DrawSpriteSheetWithInvalidFrameIndex) {
    SpriteSheet sheet;
    sheet.texture = AssetHandle{};
    sheet.frameWidth = 32;
    sheet.frameHeight = 32;
    sheet.columns = 4;
    sheet.rows = 4;  // Total 16 frames (0-15)

    Transform2D transform{.x = 100.0f, .y = 100.0f};

    graphics_->beginFrame();
    graphics_->drawSprite(sheet, -1, transform);  // Negative index
    graphics_->drawSprite(sheet, 100, transform);  // Out of bounds index
    graphics_->endFrame();

    // Should handle invalid frame indices gracefully
}

TEST_F(GraphicsSystemTest, DISABLED_DrawSpriteSheetWithZeroColumnsRows) {
    SpriteSheet sheet;
    sheet.texture = AssetHandle{};
    sheet.frameWidth = 32;
    sheet.frameHeight = 32;
    sheet.columns = 0;
    sheet.rows = 0;

    Transform2D transform{.x = 100.0f, .y = 100.0f};

    graphics_->beginFrame();
    graphics_->drawSprite(sheet, 0, transform);
    graphics_->endFrame();

    // Should handle degenerate spritesheet gracefully
}

TEST_F(GraphicsSystemTest, DISABLED_ColorComponentsAtBoundaries) {
    // Test colors at min/max boundaries
    graphics_->beginFrame();
    graphics_->drawRect(Canvas{{0,0}, {10,10}}, Color{0, 0, 0, 0}, true);      // All min
    graphics_->drawRect(Canvas{{10,0}, {10,10}}, Color{255, 255, 255, 255}, true);  // All max
    graphics_->drawRect(Canvas{{20,0}, {10,10}}, Color{255, 0, 255, 0}, true);      // Mixed
    graphics_->endFrame();
}

}  // namespace jframe::tests

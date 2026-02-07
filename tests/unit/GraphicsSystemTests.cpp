// tests/unit/GraphicsSystemTests.cpp
// Graphics system unit tests (2D)

#include <memory>

#include <gtest/gtest.h>
#include <kangaru/kangaru.hpp>

import std;
import bestow;
import bestow.types;

// Note: The shared MockGraphicsSystem in tests/mocks/ is designed for use by
// files that already import the specific graphics module. This file uses a local
// mock to avoid C++20 module visibility issues with IGraphicsContext.

namespace bestow::tests {

//==============================================================================
// Local MockGraphicsSystem for 2D graphics interface testing
//==============================================================================

class MockGraphicsSystem : public IGraphicsSystem {
public:
    void beginFrame() override { frameCount_++; beginFrameCalled_ = true; }
    void endFrame() override { endFrameCalled_ = true; }

    void setCamera(const Camera& camera) override { camera_ = camera; }
    Camera getCamera() const override { return camera_; }

    void draw(const Sprite& sprite) override {}
    void drawBatch(std::span<const Sprite> sprites) override {}
    void drawSprite(const SpriteSheet& sheet, int frameIndex, const Transform2D& transform, Color tint) override {}
    void drawAnimatedSprite(AnimatedSprite& sprite, const Transform2D& transform, Color tint) override {}

    void drawRect(const Canvas& rect, const Color& color, bool filled) override {}
    void drawLine(Vec2 from, Vec2 to, const Color& color, float thickness) override {}
    void drawCircle(Vec2 center, float radius, const Color& color, bool filled, int segments) override {}
    void drawPolygon(std::span<const Vec2> vertices, const Color& color, bool filled) override {}

    void drawText(const std::string& text, Vec2 position, AssetHandle fontHandle,
                  float size, const Color& color) override {}
    void drawTextCentered(const std::string& text, Vec2 position, AssetHandle fontHandle,
                         float size, const Color& color) override {}
    Vec2 measureText(const std::string& text, AssetHandle fontHandle, float size) const override {
        return Vec2{static_cast<float>(text.length()) * size * 0.5f, size};
    }

    Vec2 worldToScreen(Vec2 worldPos) const override {
        return Vec2{
            (worldPos.x - camera_.transform.x) * camera_.zoom + windowSize_.width / 2.0f,
            (worldPos.y - camera_.transform.y) * camera_.zoom + windowSize_.height / 2.0f
        };
    }
    Vec2 screenToWorld(Vec2 screenPos) const override {
        return Vec2{
            (screenPos.x - windowSize_.width / 2.0f) / camera_.zoom + camera_.transform.x,
            (screenPos.y - windowSize_.height / 2.0f) / camera_.zoom + camera_.transform.y
        };
    }

    void setWindowSize(Size size) override { windowSize_ = size; }
    Size getWindowSize() const override { return windowSize_; }
    void setFullscreen(bool fullscreen) override { fullscreen_ = fullscreen; }
    bool isFullscreen() const override { return fullscreen_; }
    void setClearColor(const Color& color) override { clearColor_ = color; }
    void setVSync(bool enabled) override { vsync_ = enabled; }
    void setViewportCulling(bool enabled) override { viewportCulling_ = enabled; }
    bool isViewportCullingEnabled() const override { return viewportCulling_; }

    bool shouldClose() const override { return false; }
    void* getNativeWindowHandle() const override { return nullptr; }

    void renderEntities(IEntitySystem& entities) override {}
    void renderEntities(IEntitySystem& entities, RenderLayer minLayer, RenderLayer maxLayer) override {}

    // IGraphicsContext methods
    IUIRenderBackend* getUIRenderBackend() override { return nullptr; }
    bool isInFrame() const override { return beginFrameCalled_ && !endFrameCalled_; }
    void* getRenderContext() const override { return nullptr; }
    void* getCurrentCommandBuffer() const override { return nullptr; }

    // Test helpers
    bool wasBeginFrameCalled() const { return beginFrameCalled_; }
    bool wasEndFrameCalled() const { return endFrameCalled_; }
    int getFrameCount() const { return frameCount_; }

private:
    Camera camera_;
    Size windowSize_{800, 600};
    Color clearColor_{0, 0, 0, 255};
    bool fullscreen_ = false;
    bool vsync_ = true;
    bool viewportCulling_ = false;
    bool beginFrameCalled_ = false;
    bool endFrameCalled_ = false;
    int frameCount_ = 0;
};

class GraphicsSystemTest : public ::testing::Test {
protected:
    void SetUp() override {
        graphics_ = std::make_unique<MockGraphicsSystem>();
    }

    std::unique_ptr<IGraphicsSystem> graphics_;
};

TEST_F(GraphicsSystemTest, CanInstantiateSystem) {
    // System should instantiate without requiring OpenGL context
    EXPECT_NE(graphics_, nullptr);
}

TEST_F(GraphicsSystemTest, GetCameraReturnsDefaultCamera) {
    // Should be able to get camera even without initialization
    Camera camera = graphics_->getCamera();

    // Default camera transform should be at origin
    EXPECT_FLOAT_EQ(camera.transform.x, 0.0f);
    EXPECT_FLOAT_EQ(camera.transform.y, 0.0f);
}

TEST_F(GraphicsSystemTest, SetCameraDoesNotCrash) {
    Camera camera;
    camera.transform.x = 10.0f;
    camera.transform.y = 5.0f;
    camera.zoom = 2.0f;

    EXPECT_NO_THROW(graphics_->setCamera(camera));

    Camera retrieved = graphics_->getCamera();
    EXPECT_FLOAT_EQ(retrieved.transform.x, 10.0f);
    EXPECT_FLOAT_EQ(retrieved.transform.y, 5.0f);
    EXPECT_FLOAT_EQ(retrieved.zoom, 2.0f);
}

//==============================================================================
// Interface-based Testing (without concrete implementations)
//==============================================================================

// Note: Kangaru DI tests removed to avoid dependency on concrete implementation classes
// Interface-based testing ensures tests remain valid regardless of implementation changes

//==============================================================================
// MockGraphicsSystem Tests (Headless - Full API Testing)
//==============================================================================

class MockGraphicsSystemTest : public ::testing::Test {
protected:
    void SetUp() override {
        mockGraphics_ = std::make_unique<MockGraphicsSystem>();
    }

    std::unique_ptr<MockGraphicsSystem> mockGraphics_;
};

TEST_F(MockGraphicsSystemTest, CanInstantiate) {
    EXPECT_NE(mockGraphics_, nullptr);
}

TEST_F(MockGraphicsSystemTest, BeginEndFrameTracksState) {
    EXPECT_FALSE(mockGraphics_->wasBeginFrameCalled());
    EXPECT_FALSE(mockGraphics_->wasEndFrameCalled());
    EXPECT_EQ(mockGraphics_->getFrameCount(), 0);

    mockGraphics_->beginFrame();

    EXPECT_TRUE(mockGraphics_->wasBeginFrameCalled());
    EXPECT_EQ(mockGraphics_->getFrameCount(), 1);

    mockGraphics_->endFrame();

    EXPECT_TRUE(mockGraphics_->wasEndFrameCalled());
}

TEST_F(MockGraphicsSystemTest, DrawRectDoesNotCrash) {
    Canvas rect{{100, 100}, {200, 150}};
    EXPECT_NO_THROW(mockGraphics_->drawRect(rect, Color::red(), true));
}

TEST_F(MockGraphicsSystemTest, DrawLineDoesNotCrash) {
    Vec2 from{0.0f, 0.0f};
    Vec2 to{100.0f, 100.0f};
    EXPECT_NO_THROW(mockGraphics_->drawLine(from, to, Color::green(), 2.0f));
}

TEST_F(MockGraphicsSystemTest, DrawCircleDoesNotCrash) {
    Vec2 center{100.0f, 100.0f};
    EXPECT_NO_THROW(mockGraphics_->drawCircle(center, 50.0f, Color::blue(), true, 32));
}

TEST_F(MockGraphicsSystemTest, DrawPolygonDoesNotCrash) {
    std::vector<Vec2> vertices = {{0, 0}, {100, 0}, {100, 100}, {0, 100}};
    EXPECT_NO_THROW(mockGraphics_->drawPolygon(vertices, Color{255, 255, 0, 255}, true));
}

TEST_F(MockGraphicsSystemTest, DrawTextDoesNotCrash) {
    EXPECT_NO_THROW(mockGraphics_->drawText("Hello World", Vec2{50.0f, 50.0f}, AssetHandle{}, 24.0f, Color::white()));
}

TEST_F(MockGraphicsSystemTest, DrawTextCenteredDoesNotCrash) {
    EXPECT_NO_THROW(mockGraphics_->drawTextCentered("Centered", Vec2{200.0f, 100.0f}, AssetHandle{}, 18.0f, Color::white()));
}

TEST_F(MockGraphicsSystemTest, MeasureTextReturnsApproximation) {
    Vec2 size = mockGraphics_->measureText("Hello", AssetHandle{}, 20.0f);

    // Mock returns length * size * 0.5
    EXPECT_GT(size.x, 0.0f);
    EXPECT_FLOAT_EQ(size.y, 20.0f);
}

TEST_F(MockGraphicsSystemTest, CameraSetAndGet) {
    Camera camera;
    camera.transform.x = 100.0f;
    camera.transform.y = 200.0f;
    camera.zoom = 1.5f;

    mockGraphics_->setCamera(camera);
    Camera retrieved = mockGraphics_->getCamera();

    EXPECT_FLOAT_EQ(retrieved.transform.x, 100.0f);
    EXPECT_FLOAT_EQ(retrieved.transform.y, 200.0f);
    EXPECT_FLOAT_EQ(retrieved.zoom, 1.5f);
}

TEST_F(MockGraphicsSystemTest, WorldToScreenConvertsCoordinates) {
    Camera camera;
    camera.transform.x = 0.0f;
    camera.transform.y = 0.0f;
    camera.zoom = 1.0f;
    mockGraphics_->setCamera(camera);
    mockGraphics_->setWindowSize(Size{800, 600});

    Vec2 worldPos{0.0f, 0.0f};
    Vec2 screenPos = mockGraphics_->worldToScreen(worldPos);

    // Origin should map to center of screen
    EXPECT_FLOAT_EQ(screenPos.x, 400.0f);
    EXPECT_FLOAT_EQ(screenPos.y, 300.0f);
}

TEST_F(MockGraphicsSystemTest, ScreenToWorldConvertsCoordinates) {
    Camera camera;
    camera.transform.x = 0.0f;
    camera.transform.y = 0.0f;
    camera.zoom = 1.0f;
    mockGraphics_->setCamera(camera);
    mockGraphics_->setWindowSize(Size{800, 600});

    Vec2 screenPos{400.0f, 300.0f};
    Vec2 worldPos = mockGraphics_->screenToWorld(screenPos);

    // Center of screen should map to origin
    EXPECT_FLOAT_EQ(worldPos.x, 0.0f);
    EXPECT_FLOAT_EQ(worldPos.y, 0.0f);
}

TEST_F(MockGraphicsSystemTest, WindowSizeSetAndGet) {
    mockGraphics_->setWindowSize(Size{1920, 1080});
    Size size = mockGraphics_->getWindowSize();

    EXPECT_EQ(size.width, 1920);
    EXPECT_EQ(size.height, 1080);
}

TEST_F(MockGraphicsSystemTest, FullscreenSetAndGet) {
    EXPECT_FALSE(mockGraphics_->isFullscreen());

    mockGraphics_->setFullscreen(true);
    EXPECT_TRUE(mockGraphics_->isFullscreen());

    mockGraphics_->setFullscreen(false);
    EXPECT_FALSE(mockGraphics_->isFullscreen());
}

TEST_F(MockGraphicsSystemTest, ClearColorDoesNotCrash) {
    // setClearColor exists in interface, but getClearColor does not
    EXPECT_NO_THROW(mockGraphics_->setClearColor(Color::red()));
}

TEST_F(MockGraphicsSystemTest, VSyncDoesNotCrash) {
    // setVSync exists in interface, but isVSyncEnabled does not
    EXPECT_NO_THROW(mockGraphics_->setVSync(false));
    EXPECT_NO_THROW(mockGraphics_->setVSync(true));
}

TEST_F(MockGraphicsSystemTest, ViewportCullingSetAndGet) {
    EXPECT_FALSE(mockGraphics_->isViewportCullingEnabled());

    mockGraphics_->setViewportCulling(true);
    EXPECT_TRUE(mockGraphics_->isViewportCullingEnabled());

    mockGraphics_->setViewportCulling(false);
    EXPECT_FALSE(mockGraphics_->isViewportCullingEnabled());
}

TEST_F(MockGraphicsSystemTest, MultipleDrawCallsDoNotCrash) {
    EXPECT_NO_THROW(mockGraphics_->drawRect(Canvas{{0, 0}, {100, 100}}, Color::red(), true));
    EXPECT_NO_THROW(mockGraphics_->drawLine(Vec2{0, 0}, Vec2{100, 100}, Color::green(), 1.0f));
    EXPECT_NO_THROW(mockGraphics_->drawCircle(Vec2{50, 50}, 25, Color::blue(), true, 32));
}

TEST_F(MockGraphicsSystemTest, ShouldCloseDefaultsFalse) {
    EXPECT_FALSE(mockGraphics_->shouldClose());
}

TEST_F(MockGraphicsSystemTest, NativeWindowHandleReturnsNull) {
    EXPECT_EQ(mockGraphics_->getNativeWindowHandle(), nullptr);
}

}  // namespace bestow::tests

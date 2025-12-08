// tests/unit/GraphicsSystemTests.cpp
// Graphics system unit tests (2D)

#include <memory>

#include <gtest/gtest.h>
#include <kangaru/kangaru.hpp>

import std;
import bestow.graphics;
import bestow.opengl.impl;
import bestow.types;

// Include MockGraphicsSystem for headless testing
#include "../mocks/MockGraphicsSystem.hpp"

namespace bestow::tests {

//==============================================================================
// Real GraphicsSystem Tests (Basic Instantiation Only)
//==============================================================================

class GraphicsSystemTest : public ::testing::Test {
protected:
    void SetUp() override {
        graphics_ = std::make_unique<OpenGLGraphicsSystem>();
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
// Kangaru DI Integration Tests
//==============================================================================

TEST(GraphicsSystemKangaruTest, CanInstantiateViaService) {
    kgr::container container;

    // Register the service
    container.emplace<GraphicsSystemService>();

    // Get the service instance
    auto& graphics = container.service<GraphicsSystemService>();

    EXPECT_NE(&graphics, nullptr);
}

TEST(GraphicsSystemKangaruTest, ServiceIsSingleton) {
    kgr::container container;
    container.emplace<GraphicsSystemService>();

    auto& graphics1 = container.service<GraphicsSystemService>();
    auto& graphics2 = container.service<GraphicsSystemService>();

    // Should be the same instance (singleton)
    EXPECT_EQ(&graphics1, &graphics2);
}

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

TEST_F(MockGraphicsSystemTest, DrawRectRecordsCall) {
    Canvas rect{{100, 100}, {200, 150}};
    mockGraphics_->drawRect(rect, Color::red(), true);

    EXPECT_EQ(mockGraphics_->getDrawCallCount(), 1);

    const auto& calls = mockGraphics_->getDrawCalls();
    ASSERT_EQ(calls.size(), 1);
    EXPECT_EQ(calls[0].type, DrawCall::Type::Rect);

    auto& data = std::any_cast<const RectDrawData&>(calls[0].data);
    EXPECT_EQ(data.rect.origin.x, 100);
    EXPECT_EQ(data.rect.origin.y, 100);
    EXPECT_EQ(data.rect.size.width, 200);
    EXPECT_EQ(data.rect.size.height, 150);
    EXPECT_EQ(data.color.r, Color::red().r);
    EXPECT_TRUE(data.filled);
}

TEST_F(MockGraphicsSystemTest, DrawLineRecordsCall) {
    Vec2 from{0.0f, 0.0f};
    Vec2 to{100.0f, 100.0f};
    mockGraphics_->drawLine(from, to, Color::green(), 2.0f);

    EXPECT_EQ(mockGraphics_->getDrawCallCount(), 1);

    const auto& calls = mockGraphics_->getDrawCalls();
    ASSERT_EQ(calls.size(), 1);
    EXPECT_EQ(calls[0].type, DrawCall::Type::Line);

    auto& data = std::any_cast<const LineDrawData&>(calls[0].data);
    EXPECT_FLOAT_EQ(data.from.x, 0.0f);
    EXPECT_FLOAT_EQ(data.from.y, 0.0f);
    EXPECT_FLOAT_EQ(data.to.x, 100.0f);
    EXPECT_FLOAT_EQ(data.to.y, 100.0f);
    EXPECT_FLOAT_EQ(data.thickness, 2.0f);
}

TEST_F(MockGraphicsSystemTest, DrawCircleRecordsCall) {
    Vec2 center{100.0f, 100.0f};
    mockGraphics_->drawCircle(center, 50.0f, Color::blue(), true, 32);

    EXPECT_EQ(mockGraphics_->getDrawCallCount(), 1);

    const auto& calls = mockGraphics_->getDrawCalls();
    ASSERT_EQ(calls.size(), 1);
    EXPECT_EQ(calls[0].type, DrawCall::Type::Circle);

    auto& data = std::any_cast<const CircleDrawData&>(calls[0].data);
    EXPECT_FLOAT_EQ(data.center.x, 100.0f);
    EXPECT_FLOAT_EQ(data.center.y, 100.0f);
    EXPECT_FLOAT_EQ(data.radius, 50.0f);
    EXPECT_TRUE(data.filled);
}

TEST_F(MockGraphicsSystemTest, DrawTextRecordsCall) {
    mockGraphics_->drawText("Hello World", Vec2{50.0f, 50.0f}, AssetHandle{}, 24.0f, Color::white());

    EXPECT_EQ(mockGraphics_->getDrawCallCount(), 1);

    const auto& calls = mockGraphics_->getDrawCalls();
    ASSERT_EQ(calls.size(), 1);
    EXPECT_EQ(calls[0].type, DrawCall::Type::Text);

    auto& data = std::any_cast<const TextDrawData&>(calls[0].data);
    EXPECT_EQ(data.text, "Hello World");
    EXPECT_FLOAT_EQ(data.position.x, 50.0f);
    EXPECT_FLOAT_EQ(data.position.y, 50.0f);
    EXPECT_FLOAT_EQ(data.size, 24.0f);
    EXPECT_FALSE(data.centered);
}

TEST_F(MockGraphicsSystemTest, DrawTextCenteredRecordsCall) {
    mockGraphics_->drawTextCentered("Centered", Vec2{200.0f, 100.0f}, AssetHandle{}, 18.0f, Color::white());

    EXPECT_EQ(mockGraphics_->getDrawCallCount(), 1);

    const auto& calls = mockGraphics_->getDrawCalls();
    ASSERT_EQ(calls.size(), 1);
    EXPECT_EQ(calls[0].type, DrawCall::Type::TextCentered);

    auto& data = std::any_cast<const TextDrawData&>(calls[0].data);
    EXPECT_EQ(data.text, "Centered");
    EXPECT_TRUE(data.centered);
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

TEST_F(MockGraphicsSystemTest, ClearColorSetAndGet) {
    mockGraphics_->setClearColor(Color::red());
    Color color = mockGraphics_->getClearColor();

    EXPECT_FLOAT_EQ(color.r, Color::red().r);
    EXPECT_FLOAT_EQ(color.g, Color::red().g);
    EXPECT_FLOAT_EQ(color.b, Color::red().b);
}

TEST_F(MockGraphicsSystemTest, VSyncSetAndGet) {
    mockGraphics_->setVSync(false);
    EXPECT_FALSE(mockGraphics_->isVSyncEnabled());

    mockGraphics_->setVSync(true);
    EXPECT_TRUE(mockGraphics_->isVSyncEnabled());
}

TEST_F(MockGraphicsSystemTest, ViewportCullingSetAndGet) {
    EXPECT_FALSE(mockGraphics_->isViewportCullingEnabled());

    mockGraphics_->setViewportCulling(true);
    EXPECT_TRUE(mockGraphics_->isViewportCullingEnabled());

    mockGraphics_->setViewportCulling(false);
    EXPECT_FALSE(mockGraphics_->isViewportCullingEnabled());
}

TEST_F(MockGraphicsSystemTest, MultipleDrawCallsAreRecorded) {
    mockGraphics_->drawRect(Canvas{{0, 0}, {100, 100}}, Color::red(), true);
    mockGraphics_->drawLine(Vec2{0, 0}, Vec2{100, 100}, Color::green(), 1.0f);
    mockGraphics_->drawCircle(Vec2{50, 50}, 25, Color::blue(), true, 32);

    EXPECT_EQ(mockGraphics_->getDrawCallCount(), 3);
}

TEST_F(MockGraphicsSystemTest, BeginFrameClearsDrawCalls) {
    mockGraphics_->drawRect(Canvas{{0, 0}, {100, 100}}, Color::red(), true);
    EXPECT_EQ(mockGraphics_->getDrawCallCount(), 1);

    mockGraphics_->beginFrame();
    EXPECT_EQ(mockGraphics_->getDrawCallCount(), 0);
}

TEST_F(MockGraphicsSystemTest, ShouldCloseDefaultsFalse) {
    EXPECT_FALSE(mockGraphics_->shouldClose());
}

TEST_F(MockGraphicsSystemTest, NativeWindowHandleReturnsNull) {
    EXPECT_EQ(mockGraphics_->getNativeWindowHandle(), nullptr);
}

//==============================================================================
// MockGraphicsSystem Kangaru DI Tests
//==============================================================================

TEST(MockGraphicsSystemKangaruTest, CanInstantiateViaService) {
    kgr::container container;
    container.emplace<MockGraphicsSystemService>();

    auto& mockGraphics = container.service<MockGraphicsSystemService>();
    EXPECT_NE(&mockGraphics, nullptr);
}

TEST(MockGraphicsSystemKangaruTest, ServiceIsSingleton) {
    kgr::container container;
    container.emplace<MockGraphicsSystemService>();

    auto& mock1 = container.service<MockGraphicsSystemService>();
    auto& mock2 = container.service<MockGraphicsSystemService>();

    EXPECT_EQ(&mock1, &mock2);
}

TEST(MockGraphicsSystemKangaruTest, CanUseAsIGraphicsSystem) {
    kgr::container container;
    container.emplace<MockGraphicsSystemService>();

    auto& mockGraphics = container.service<MockGraphicsSystemService>();

    // Use through interface
    IGraphicsSystem* iface = &mockGraphics;

    Camera camera;
    camera.transform.x = 50.0f;
    iface->setCamera(camera);

    Camera retrieved = iface->getCamera();
    EXPECT_FLOAT_EQ(retrieved.transform.x, 50.0f);
}

}  // namespace bestow::tests

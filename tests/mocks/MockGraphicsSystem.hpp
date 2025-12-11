// tests/mocks/MockGraphicsSystem.hpp
// Mock graphics system for headless testing

#pragma once

#include <kangaru/kangaru.hpp>

import std;
import bestow.graphics;
import bestow.types;
import bestow.assets;
import bestow.entity;

namespace bestow::tests {

// Track what operations were performed for test verification
struct DrawCall {
    enum class Type {
        Sprite,
        SpriteBatch,
        SpriteSheet,
        AnimatedSprite,
        Rect,
        Line,
        Circle,
        Polygon,
        Text,
        TextCentered
    };
    Type type;
    std::any data;
};

struct RectDrawData {
    Canvas rect;
    Color color;
    bool filled;
};

struct LineDrawData {
    Vec2 from;
    Vec2 to;
    Color color;
    float thickness;
};

struct CircleDrawData {
    Vec2 center;
    float radius;
    Color color;
    bool filled;
    int segments;
};

struct TextDrawData {
    std::string text;
    Vec2 position;
    AssetHandle fontHandle;
    float size;
    Color color;
    bool centered;
};

class MockGraphicsSystem : public IGraphicsSystem {
public:
    MockGraphicsSystem() = default;
    ~MockGraphicsSystem() override = default;

    //==========================================================================
    // Test Utilities
    //==========================================================================

    void clearRecordedCalls() { drawCalls_.clear(); }
    const std::vector<DrawCall>& getDrawCalls() const { return drawCalls_; }
    std::size_t getDrawCallCount() const { return drawCalls_.size(); }

    int getFrameCount() const { return frameCount_; }
    bool wasBeginFrameCalled() const { return beginFrameCalled_; }
    bool wasEndFrameCalled() const { return endFrameCalled_; }

    //==========================================================================
    // Frame Lifecycle
    //==========================================================================

    void beginFrame() override {
        beginFrameCalled_ = true;
        frameCount_++;
        clearRecordedCalls();
    }

    void endFrame() override {
        endFrameCalled_ = true;
    }

    //==========================================================================
    // Sprite Rendering
    //==========================================================================

    void draw(const Sprite& sprite) override {
        drawCalls_.push_back({DrawCall::Type::Sprite, sprite});
    }

    void drawBatch(std::span<const Sprite> sprites) override {
        std::vector<Sprite> spriteVec(sprites.begin(), sprites.end());
        drawCalls_.push_back({DrawCall::Type::SpriteBatch, std::move(spriteVec)});
    }

    void drawSprite(const SpriteSheet& sheet, int frameIndex,
                   const Transform2D& transform, Color tint) override {
        // Store the relevant data
        drawCalls_.push_back({DrawCall::Type::SpriteSheet, frameIndex});
    }

    void drawAnimatedSprite(AnimatedSprite& sprite,
                           const Transform2D& transform, Color tint) override {
        drawCalls_.push_back({DrawCall::Type::AnimatedSprite, 0});
    }

    //==========================================================================
    // Primitive Rendering
    //==========================================================================

    void drawRect(const Canvas& rect, const Color& color, bool filled) override {
        RectDrawData data{rect, color, filled};
        drawCalls_.push_back({DrawCall::Type::Rect, data});
    }

    void drawLine(Vec2 from, Vec2 to, const Color& color, float thickness) override {
        LineDrawData data{from, to, color, thickness};
        drawCalls_.push_back({DrawCall::Type::Line, data});
    }

    void drawCircle(Vec2 center, float radius, const Color& color,
                    bool filled, int segments) override {
        CircleDrawData data{center, radius, color, filled, segments};
        drawCalls_.push_back({DrawCall::Type::Circle, data});
    }

    void drawPolygon(std::span<const Vec2> vertices, const Color& color,
                     bool filled) override {
        std::vector<Vec2> vertVec(vertices.begin(), vertices.end());
        drawCalls_.push_back({DrawCall::Type::Polygon, std::move(vertVec)});
    }

    //==========================================================================
    // Text Rendering
    //==========================================================================

    void drawText(const std::string& text, Vec2 position,
                  AssetHandle fontHandle, float size,
                  const Color& color) override {
        TextDrawData data{text, position, fontHandle, size, color, false};
        drawCalls_.push_back({DrawCall::Type::Text, data});
    }

    void drawTextCentered(const std::string& text, Vec2 position,
                          AssetHandle fontHandle, float size,
                          const Color& color) override {
        TextDrawData data{text, position, fontHandle, size, color, true};
        drawCalls_.push_back({DrawCall::Type::TextCentered, data});
    }

    Vec2 measureText(const std::string& text, AssetHandle fontHandle,
                     float size) const override {
        // Return a simple approximation based on character count
        return Vec2{static_cast<float>(text.length()) * size * 0.5f, size};
    }

    //==========================================================================
    // Camera
    //==========================================================================

    void setCamera(const Camera& camera) override {
        camera_ = camera;
    }

    Camera getCamera() const override {
        return camera_;
    }

    Vec2 worldToScreen(Vec2 worldPos) const override {
        // Simple transform: apply camera offset and zoom
        return Vec2{
            (worldPos.x - camera_.transform.x) * camera_.zoom + windowSize_.width / 2.0f,
            (worldPos.y - camera_.transform.y) * camera_.zoom + windowSize_.height / 2.0f
        };
    }

    Vec2 screenToWorld(Vec2 screenPos) const override {
        // Inverse of worldToScreen
        return Vec2{
            (screenPos.x - windowSize_.width / 2.0f) / camera_.zoom + camera_.transform.x,
            (screenPos.y - windowSize_.height / 2.0f) / camera_.zoom + camera_.transform.y
        };
    }

    //==========================================================================
    // Window Management
    //==========================================================================

    Size getWindowSize() const override {
        return windowSize_;
    }

    void setWindowSize(Size size) override {
        windowSize_ = size;
    }

    bool isFullscreen() const override {
        return isFullscreen_;
    }

    void setFullscreen(bool fullscreen) override {
        isFullscreen_ = fullscreen;
    }

    bool shouldClose() const override {
        return shouldClose_;
    }

    void setShouldClose(bool close) {
        shouldClose_ = close;
    }

    void* getNativeWindowHandle() const override {
        return nullptr;  // No window handle for mock
    }

    //==========================================================================
    // Render State
    //==========================================================================

    void setClearColor(const Color& color) override {
        clearColor_ = color;
    }

    Color getClearColor() const {
        return clearColor_;
    }

    void setVSync(bool enabled) override {
        vsyncEnabled_ = enabled;
    }

    bool isVSyncEnabled() const {
        return vsyncEnabled_;
    }

    //==========================================================================
    //==========================================================================
    // Automatic Entity Rendering
    //==========================================================================

    void renderEntities(IEntitySystem& entities) override {
        entitiesRenderedCount_++;
    }

    void renderEntities(IEntitySystem& entities,
                        RenderLayer minLayer, RenderLayer maxLayer) override {
        entitiesRenderedCount_++;
        lastMinLayer_ = minLayer;
        lastMaxLayer_ = maxLayer;
    }

    void setViewportCulling(bool enabled) override {
        viewportCullingEnabled_ = enabled;
    }

    bool isViewportCullingEnabled() const override {
        return viewportCullingEnabled_;
    }

    //==========================================================================
    // Additional Test Accessors
    //==========================================================================

    int getEntitiesRenderedCount() const { return entitiesRenderedCount_; }
    RenderLayer getLastMinLayer() const { return lastMinLayer_; }
    RenderLayer getLastMaxLayer() const { return lastMaxLayer_; }

private:
    // State tracking
    Camera camera_;
    Size windowSize_{800, 600};
    bool isFullscreen_ = false;
    bool shouldClose_ = false;
    Color clearColor_ = Color::black();
    bool vsyncEnabled_ = true;
    IAssetSystem* assetSystem_ = nullptr;
    bool viewportCullingEnabled_ = false;

    // Frame tracking
    int frameCount_ = 0;
    bool beginFrameCalled_ = false;
    bool endFrameCalled_ = false;

    // Entity rendering tracking
    int entitiesRenderedCount_ = 0;
    RenderLayer lastMinLayer_ = 0;
    RenderLayer lastMaxLayer_ = 0;

    // Draw call recording
    std::vector<DrawCall> drawCalls_;
};

// Kangaru service definition for DI integration
struct MockGraphicsSystemService : kgr::single_service<MockGraphicsSystem> {};

}  // namespace bestow::tests

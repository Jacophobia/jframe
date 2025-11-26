// jframe-contract/src/jframe.graphics.cppm
// Graphics system interface

module;

#include <span>
#include <string>

export module jframe.graphics;

import jframe.types;

export namespace jframe {

class IGraphicsSystem {
public:
    virtual ~IGraphicsSystem() = default;

    //======================================================================
    // Frame Lifecycle
    //======================================================================

    virtual void beginFrame() = 0;
    virtual void endFrame() = 0;

    //======================================================================
    // Sprite Rendering
    //======================================================================

    virtual void draw(const Sprite& sprite) = 0;
    virtual void drawBatch(std::span<const Sprite> sprites) = 0;

    //======================================================================
    // Primitive Rendering (Debug)
    //======================================================================

    virtual void drawRect(const Canvas& rect, const Color& color,
                          bool filled = true) = 0;
    virtual void drawLine(Vec2 from, Vec2 to, const Color& color,
                          float thickness = 1.0f) = 0;
    virtual void drawCircle(Vec2 center, float radius, const Color& color,
                            bool filled = true, int segments = 32) = 0;
    virtual void drawPolygon(std::span<const Vec2> vertices,
                             const Color& color, bool filled = true) = 0;

    //======================================================================
    // Text Rendering
    //======================================================================

    virtual void drawText(const std::string& text, Vec2 position,
                          AssetHandle fontHandle, float size,
                          const Color& color = Color::white()) = 0;
    virtual Vec2 measureText(const std::string& text, AssetHandle fontHandle,
                             float size) const = 0;

    //======================================================================
    // Camera
    //======================================================================

    virtual void setCamera(const Camera& camera) = 0;
    virtual Camera getCamera() const = 0;

    virtual Vec2 worldToScreen(Vec2 worldPos) const = 0;
    virtual Vec2 screenToWorld(Vec2 screenPos) const = 0;

    //======================================================================
    // Window Management
    //======================================================================

    virtual Size getWindowSize() const = 0;
    virtual void setWindowSize(Size size) = 0;
    virtual bool isFullscreen() const = 0;
    virtual void setFullscreen(bool fullscreen) = 0;
    virtual bool shouldClose() const = 0;
    virtual void* getNativeWindowHandle() const = 0;

    //======================================================================
    // Render State
    //======================================================================

    virtual void setClearColor(const Color& color) = 0;
    virtual void setVSync(bool enabled) = 0;
};

}  // namespace jframe

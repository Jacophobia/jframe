// bestow-contract/src/bestow.graphics.cppm
// Graphics system interface

module;

#include <span>
#include <string>

export module bestow.graphics;

import bestow.types;
import bestow.assets;  // For IAssetSystem forward reference
import bestow.entity;  // For IEntitySystem in renderEntities

export namespace bestow {

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

    virtual void drawSprite(const SpriteSheet& sheet, int frameIndex,
                           const Transform2D& transform, Color tint = Color::white()) = 0;
    virtual void drawAnimatedSprite(AnimatedSprite& sprite,
                                   const Transform2D& transform, Color tint = Color::white()) = 0;

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
    virtual void drawTextCentered(const std::string& text, Vec2 position,
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

    //======================================================================
    // Automatic Entity Rendering
    //======================================================================

    /// Render all entities with visual components (Sprite, DebugRect, etc.)
    /// Entities must have Transform2D plus a visual component
    /// Respects RenderLayer for draw order
    virtual void renderEntities(IEntitySystem& entities) = 0;

    /// Render entities within a specific layer range
    virtual void renderEntities(IEntitySystem& entities,
                                RenderLayer minLayer, RenderLayer maxLayer) = 0;

    /// Enable/disable viewport culling for renderEntities
    virtual void setViewportCulling(bool enabled) = 0;
    virtual bool isViewportCullingEnabled() const = 0;
};

}  // namespace bestow

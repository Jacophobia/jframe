# Contract Specifications

This document contains the full contract specifications for the UI overhaul.

## New Contract: `bestow.graphics.context`

**File:** `bestow-contract/src/bestow.graphics.context.cppm`

```cpp
// bestow-contract/src/bestow.graphics.context.cppm
// Base interface for all graphics systems (2D and 3D)
// Provides common capabilities needed by cross-cutting systems like UI

module;

#include <cstdint>

export module bestow.graphics.context;

import bestow.types;

export namespace bestow {

// Forward declaration - defined in bestow.uirender
class IUIRenderBackend;

/// Base interface for all graphics systems.
/// Both IGraphicsSystem (2D) and IGraphics3DSystem (3D) extend this.
/// Provides access to shared functionality needed by UI and other overlay systems.
class IGraphicsContext {
public:
    virtual ~IGraphicsContext() = default;

    //======================================================================
    // UI Render Backend
    //======================================================================

    /// Get the UI render backend for this graphics system.
    /// The graphics system owns the backend - caller should not delete.
    /// Returns nullptr if UI rendering is not supported.
    /// The backend is created lazily on first call.
    virtual IUIRenderBackend* getUIRenderBackend() = 0;

    //======================================================================
    // Window/Viewport Information
    //======================================================================

    /// Get current window size in pixels.
    /// Note: Both IGraphicsSystem and IGraphics3DSystem already have this method,
    /// so implementations can simply delegate to their existing getWindowSize().
    virtual Size getWindowSize() const = 0;

    /// Get native window handle (e.g., GLFWwindow*).
    /// Used for input integration.
    virtual void* getNativeWindowHandle() const = 0;

    //======================================================================
    // Frame State
    //======================================================================

    /// Check if currently inside a frame (between beginFrame/endFrame).
    /// UI rendering should only occur within a frame.
    virtual bool isInFrame() const = 0;

    //======================================================================
    // Render Context Access (for backend implementation)
    //======================================================================

    /// Get renderer-specific context for internal use.
    /// Returns:
    /// - Vulkan: pointer to internal VulkanContext struct
    /// - OpenGL: nullptr (context is thread-bound)
    /// - Metal: pointer to MTLDevice (future)
    /// This is used by IUIRenderBackend implementations, not by game code.
    virtual void* getRenderContext() const = 0;

    /// Get current command buffer/state for synchronized rendering.
    /// Returns:
    /// - Vulkan: VkCommandBuffer
    /// - OpenGL: nullptr
    /// Used by UI backend to record into the current frame's commands.
    virtual void* getCurrentCommandBuffer() const = 0;
};

}  // namespace bestow
```

## New Contract: `bestow.uirender`

**File:** `bestow-contract/src/bestow.uirender.cppm`

```cpp
// bestow-contract/src/bestow.uirender.cppm
// UI Render Backend interface
// Abstracts low-level 2D rendering primitives needed by UI frameworks like RmlUi

module;

#include <cstdint>
#include <span>
#include <filesystem>

export module bestow.uirender;

import bestow.types;

export namespace bestow {

//==========================================================================
// Handle Types
//==========================================================================

/// Handle to compiled UI geometry (vertices + indices on GPU)
using UIGeometryHandle = std::uint64_t;

/// Handle to a UI texture
using UITextureHandle = std::uint64_t;

/// Invalid handle constants
constexpr UIGeometryHandle InvalidUIGeometry = 0;
constexpr UITextureHandle InvalidUITexture = 0;

//==========================================================================
// Vertex Format
//==========================================================================

/// UI vertex format - matches RmlUi's vertex layout for easy conversion.
/// Position is in screen coordinates (pixels).
/// Color components are 0-255, will be converted to premultiplied alpha by backend.
struct UIVertex {
    Vec2 position;     // Screen-space position in pixels
    Color color;       // RGBA color (0-255 per component)
    Vec2 texCoord;     // Texture UV coordinates (0.0 - 1.0)
};

//==========================================================================
// Scissor Region
//==========================================================================

/// Scissor rectangle for clipping UI elements.
/// Coordinates are in screen pixels, origin at top-left.
struct UIScissorRect {
    int x = 0;
    int y = 0;
    int width = 0;
    int height = 0;
};

//==========================================================================
// Error Types
//==========================================================================

enum class UIRenderError {
    Success,
    NotInitialized,
    OutOfMemory,
    InvalidGeometry,
    InvalidTexture,
    TextureLoadFailed,
    ShaderCompilationFailed,
    PipelineCreationFailed,
    InternalError
};

//==========================================================================
// IUIRenderBackend Interface
//==========================================================================

/// Low-level 2D rendering interface for UI systems.
/// Implementations exist for each graphics backend (OpenGL, Vulkan).
/// The graphics system owns and creates this backend.
class IUIRenderBackend {
public:
    virtual ~IUIRenderBackend() = default;

    //======================================================================
    // Lifecycle
    //======================================================================

    /// Initialize the UI render backend.
    /// Called by the graphics system after its own initialization.
    virtual bool initialize() = 0;

    /// Shutdown and release all resources.
    virtual void shutdown() = 0;

    /// Check if the backend is initialized and ready.
    virtual bool isInitialized() const = 0;

    //======================================================================
    // Geometry Management
    //======================================================================

    /// Compile vertex and index data into GPU-optimized geometry.
    /// Returns a handle for use with renderGeometry().
    /// The geometry is immutable once compiled.
    virtual UIGeometryHandle compileGeometry(
        std::span<const UIVertex> vertices,
        std::span<const std::uint32_t> indices) = 0;

    /// Release compiled geometry and free GPU resources.
    virtual void releaseGeometry(UIGeometryHandle geometry) = 0;

    //======================================================================
    // Rendering
    //======================================================================

    /// Begin a UI rendering pass.
    /// Must be called before any renderGeometry() calls.
    /// Sets up state for 2D overlay rendering:
    /// - Disables depth testing
    /// - Enables blending (premultiplied alpha)
    /// - Sets orthographic projection
    virtual void beginUIPass() = 0;

    /// Render compiled geometry.
    /// @param geometry Handle from compileGeometry()
    /// @param translation Offset in screen pixels
    /// @param texture Texture handle, or InvalidUITexture for solid color
    virtual void renderGeometry(
        UIGeometryHandle geometry,
        Vec2 translation,
        UITextureHandle texture = InvalidUITexture) = 0;

    /// End the UI rendering pass.
    /// Restores previous render state.
    virtual void endUIPass() = 0;

    //======================================================================
    // Texture Management
    //======================================================================

    /// Load a texture from file path.
    /// @param path Path to texture file (PNG, JPG, etc.)
    /// @param outWidth Receives the texture width
    /// @param outHeight Receives the texture height
    /// @return Texture handle or InvalidUITexture on failure
    virtual UITextureHandle loadTexture(
        const std::filesystem::path& path,
        int& outWidth,
        int& outHeight) = 0;

    /// Create a texture from raw RGBA8 pixel data.
    /// Used for runtime-generated textures (font atlases, etc.)
    /// @param data RGBA8 pixel data (4 bytes per pixel)
    /// @param width Texture width in pixels
    /// @param height Texture height in pixels
    /// @return Texture handle or InvalidUITexture on failure
    virtual UITextureHandle createTexture(
        std::span<const std::uint8_t> data,
        int width,
        int height) = 0;

    /// Release a texture and free GPU memory.
    virtual void releaseTexture(UITextureHandle texture) = 0;

    //======================================================================
    // Scissor (Clipping)
    //======================================================================

    /// Enable or disable scissor testing.
    /// When enabled, only pixels within the scissor region are drawn.
    virtual void enableScissor(bool enable) = 0;

    /// Set the scissor region.
    /// Coordinates are in screen pixels, origin at top-left.
    /// Only effective when scissor is enabled.
    virtual void setScissorRegion(const UIScissorRect& region) = 0;

    //======================================================================
    // Viewport
    //======================================================================

    /// Update the viewport size.
    /// Should be called when the window resizes.
    /// Updates the orthographic projection matrix.
    virtual void setViewportSize(int width, int height) = 0;

    /// Get current viewport size.
    virtual Size getViewportSize() const = 0;

    //======================================================================
    // Statistics (Debug)
    //======================================================================

    /// Get number of draw calls in the last frame.
    virtual std::uint32_t getDrawCallCount() const = 0;

    /// Get number of triangles rendered in the last frame.
    virtual std::uint32_t getTriangleCount() const = 0;
};

}  // namespace bestow
```

## Modified Contract: `bestow.graphics` (2D)

**File:** `bestow-contract/src/bestow.graphics.cppm`

Changes:
- Add `import bestow.graphics.context;`
- Make `IGraphicsSystem` extend `IGraphicsContext`
- Add new methods from `IGraphicsContext`

```cpp
// bestow-contract/src/bestow.graphics.cppm
// 2D Graphics system interface

module;

#include <span>
#include <string>

export module bestow.graphics;

import bestow.types;
import bestow.assets;
import bestow.entity;
import bestow.graphics.context;  // NEW: Import base interface

export namespace bestow {

class IGraphicsSystem : public IGraphicsContext {  // CHANGED: Extend IGraphicsContext
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

    virtual void drawRect(const Canvas& rect, const Color& color, bool filled = true) = 0;
    virtual void drawLine(Vec2 from, Vec2 to, const Color& color, float thickness = 1.0f) = 0;
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

    // Inherited from IGraphicsContext:
    // - void* getNativeWindowHandle() const
    // - Size getViewportSize() const  (can delegate to getWindowSize)
    // - IUIRenderBackend* getUIRenderBackend()
    // - bool isInFrame() const
    // - void* getRenderContext() const
    // - void* getCurrentCommandBuffer() const

    //======================================================================
    // Render State
    //======================================================================

    virtual void setClearColor(const Color& color) = 0;
    virtual void setVSync(bool enabled) = 0;

    //======================================================================
    // Automatic Entity Rendering
    //======================================================================

    virtual void renderEntities(IEntitySystem& entities) = 0;
    virtual void renderEntities(IEntitySystem& entities,
                                RenderLayer minLayer, RenderLayer maxLayer) = 0;
    virtual void setViewportCulling(bool enabled) = 0;
    virtual bool isViewportCullingEnabled() const = 0;
};

}  // namespace bestow
```

## Modified Contract: `bestow.graphics3d` (3D)

**File:** `bestow-contract/src/bestow.graphics3d.cppm`

Changes:
- Add `import bestow.graphics.context;`
- Make `IGraphics3DSystem` extend `IGraphicsContext`
- Add new methods from `IGraphicsContext`

```cpp
// At the top of the file, add:
import bestow.graphics.context;

// Change class declaration:
class IGraphics3DSystem : public IGraphicsContext {
public:
    // ... existing interface ...

    // Inherited from IGraphicsContext:
    // - void* getNativeWindowHandle() const  (already exists, line 612)
    // - Size getViewportSize() const  (can delegate to getWindowSize, line 607)
    // - IUIRenderBackend* getUIRenderBackend()  (NEW)
    // - bool isInFrame() const  (NEW)
    // - void* getRenderContext() const  (NEW)
    // - void* getCurrentCommandBuffer() const  (NEW)
};
```

## Kangaru Service Definitions

Add to `bestow-contract/src/bestow.services.cppm`:

```cpp
// Add imports
import bestow.graphics.context;
import bestow.uirender;

// Add abstract services
struct IGraphicsContextService : kgr::abstract_service<IGraphicsContext> {};
struct IUIRenderBackendService : kgr::abstract_service<IUIRenderBackend> {};
```

## Contract Import Dependencies

```
bestow.types
    ↑
bestow.graphics.context  (imports bestow.types)
    ↑
bestow.uirender  (imports bestow.types, forward-declares IUIRenderBackend)
    ↑
bestow.graphics  (imports bestow.graphics.context)
bestow.graphics3d  (imports bestow.graphics.context)
    ↑
bestow.ui  (imports bestow.types, depends on IGraphicsContext at runtime)
```

## Type Compatibility

### UIVertex and RmlUi Vertex

```cpp
// RmlUi vertex (for reference)
namespace Rml {
    struct Vertex {
        Vector2f position;
        ColourPremultiplied colour;  // RGBA, premultiplied alpha
        Vector2f tex_coord;
    };
}

// Bestow UIVertex
struct UIVertex {
    Vec2 position;
    Color color;      // RGBA, NOT premultiplied (backend converts)
    Vec2 texCoord;
};

// Conversion in BestowRmlRenderInterface:
UIVertex convertVertex(const Rml::Vertex& v) {
    return UIVertex{
        .position = {v.position.x, v.position.y},
        .color = Color(
            v.colour.red,
            v.colour.green,
            v.colour.blue,
            v.colour.alpha
        ),
        .texCoord = {v.tex_coord.x, v.tex_coord.y}
    };
}
```

Note: RmlUi uses premultiplied alpha. The backend handles this in the shader or blend mode.

### Handle Compatibility

Both `UIGeometryHandle` and `UITextureHandle` are `uint64_t`, matching RmlUi's handle types:

```cpp
// RmlUi (for reference)
using CompiledGeometryHandle = uintptr_t;
using TextureHandle = uintptr_t;

// Bestow
using UIGeometryHandle = std::uint64_t;
using UITextureHandle = std::uint64_t;

// Direct cast is safe:
Rml::CompiledGeometryHandle rmlHandle = static_cast<Rml::CompiledGeometryHandle>(uiHandle);
```

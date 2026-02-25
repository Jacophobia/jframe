# Graphics 2D System

> **Visibility:** Public (Lua + C++ API)
> **Tier:** 5
> **Dependencies:** Types, Entity, Assets, Shader, GraphicsContext
> **Lua Paths:** `bestow.graphics` (high-level), `bestow.graphics.core` (low-level)

## Purpose

The Graphics 2D System handles all 2D rendering: sprites, geometric primitives, text, and camera control. The high-level API provides path-based sprite drawing, simple shape drawing, text rendering, camera positioning, and window queries. The low-level API exposes the full frame lifecycle, batch sprite rendering, sprite sheet and animated sprite support, complete primitive rendering (rect, line, circle, polygon, arc), font-handle-based text with measurement, camera struct control with coordinate transforms, window management, render state configuration, automatic entity rendering with layer filtering and viewport culling, render targets, and screenshots. The system inherits from IGraphicsContextCore, providing the base window/viewport interface used by UI and DevTools.

## High-Level API: `IGraphics2DSystem`

The simplified API for common game development tasks. Path-based, sensible defaults, no lifecycle management.

### Drawing

| Method | Returns | Description |
|--------|---------|-------------|
| `drawSprite(std::string_view texturePath, Vec2 position, Vec2 size = {}, Color tint = Color::white())` | `void` | Draw a sprite from a texture path at a position; size {0,0} uses the texture's native size |
| `drawRect(Vec2 position, Vec2 size, Color color, bool filled = true)` | `void` | Draw a rectangle at the given position with a specified size and color |
| `drawCircle(Vec2 center, float radius, Color color, bool filled = true)` | `void` | Draw a circle at the given center with a specified radius and color |
| `drawLine(Vec2 from, Vec2 to, Color color, float thickness = 1.0f)` | `void` | Draw a line between two points with a specified color and thickness |
| `drawText(std::string_view text, Vec2 position, float size = 16.0f, Color color = Color::white())` | `void` | Draw text at a position using the default font with a specified size and color |

### Camera

| Method | Returns | Description |
|--------|---------|-------------|
| `setCameraPosition(Vec2 position)` | `void` | Set the 2D camera center position in world coordinates |
| `setCameraZoom(float zoom)` | `void` | Set the camera zoom level (1.0 = normal, 2.0 = 2x magnification) |
| `screenToWorld(Vec2 screenPos)` | `Vec2` | Convert a screen-space coordinate to world-space using the current camera |
| `worldToScreen(Vec2 worldPos)` | `Vec2` | Convert a world-space coordinate to screen-space using the current camera |

### Window

| Method | Returns | Description |
|--------|---------|-------------|
| `getWindowSize()` | `Size` | Return the current window dimensions in pixels |
| `shouldClose()` | `bool` | Check whether the window close was requested (e.g., user clicked X) |
| `setClearColor(Color color)` | `void` | Set the background color used to clear the screen each frame |

## Low-Level API: `IGraphics2DCore`

Full control API. Inherits from IGraphicsContextCore. Exposes frame lifecycle, batch rendering, sprite sheets, full primitives, font-based text, camera struct, window management, render state, auto entity rendering, render targets, and screenshots.

### Frame Lifecycle

| Method | Returns | Description |
|--------|---------|-------------|
| `beginFrame()` | `void` | Begin a new rendering frame; clears the screen and prepares render state |
| `endFrame()` | `void` | End the current frame; flushes all draw calls and presents to the screen |

### Sprite Rendering

| Method | Returns | Description |
|--------|---------|-------------|
| `draw(const Sprite& sprite)` | `void` | Draw a single sprite with full control over transform, tint, layer, and anchor |
| `drawBatch(std::span<const Sprite> sprites)` | `void` | Draw multiple sprites in a single batched draw call for performance |
| `drawSpriteSheet(AssetHandle sheet, int frameIndex, const Transform2D& transform, Color tint = Color::white())` | `void` | Draw a specific frame from a sprite sheet atlas at the given transform |

### Primitive Rendering

| Method | Returns | Description |
|--------|---------|-------------|
| `drawRect(Vec2 position, Vec2 size, const Color& color, bool filled = true)` | `void` | Draw a rectangle at the given position with specified size and color |
| `drawLine(Vec2 from, Vec2 to, const Color& color, float thickness = 1.0f)` | `void` | Draw a line between two points with specified color and thickness |
| `drawCircle(Vec2 center, float radius, const Color& color, bool filled = true, int segments = 32)` | `void` | Draw a circle with configurable segment count for smoothness |
| `drawPolygon(std::span<const Vec2> vertices, const Color& color, bool filled = true)` | `void` | Draw a polygon defined by a list of vertex positions |
| `drawArc(Vec2 center, float radius, float startAngle, float endAngle, const Color& color, int segments = 32)` | `void` | Draw an arc (partial circle) between two angles in radians |

### Text

| Method | Returns | Description |
|--------|---------|-------------|
| `drawText(std::string_view text, Vec2 position, FontHandle font, float size, const Color& color = Color::white())` | `void` | Draw text at a position using a specific loaded font |
| `drawTextCentered(std::string_view text, Vec2 position, FontHandle font, float size, const Color& color = Color::white())` | `void` | Draw text centered horizontally and vertically at the given position |
| `measureText(std::string_view text, FontHandle font, float size)` | `Vec2` | Measure the bounding box size of rendered text without drawing it |

### Camera

| Method | Returns | Description |
|--------|---------|-------------|
| `setCamera(const Camera2D& camera)` | `void` | Set the full camera state using a Camera2D struct |
| `getCamera()` | `Camera2D` | Return the current camera state as a Camera2D struct |
| `worldToScreen(Vec2 worldPos)` | `Vec2` | Convert a world-space coordinate to screen-space |
| `screenToWorld(Vec2 screenPos)` | `Vec2` | Convert a screen-space coordinate to world-space |

### Window

| Method | Returns | Description |
|--------|---------|-------------|
| `setWindowSize(Size size)` | `void` | Resize the window to the specified dimensions in pixels |
| `getWindowMode()` | `WindowMode` | Return the current window mode (Windowed, Fullscreen, or BorderlessFullscreen) |
| `setWindowMode(WindowMode mode)` | `void` | Change the window mode |
| `shouldClose()` | `bool` | Check whether the window close was requested |
| `setWindowTitle(std::string_view title)` | `void` | Set the window title bar text |

### Render State

| Method | Returns | Description |
|--------|---------|-------------|
| `setClearColor(const Color& color)` | `void` | Set the background color used to clear the screen each frame |
| `setVSync(bool enabled)` | `void` | Enable or disable vertical synchronization |
| `setBlendMode(BlendMode mode)` | `void` | Set the active blend mode for subsequent draw calls |

### Auto Entity Rendering

| Method | Returns | Description |
|--------|---------|-------------|
| `renderEntities(IEntityCore& entities)` | `void` | Automatically render all entities that have Sprite or DebugRect/DebugCircle components |
| `renderEntities(IEntityCore& entities, int minLayer, int maxLayer)` | `void` | Render only entities whose render layer falls within the specified range |
| `setViewportCulling(bool enabled)` | `void` | Enable or disable automatic viewport culling for off-screen entities |

### Render Targets

| Method | Returns | Description |
|--------|---------|-------------|
| `createRenderTarget(int width, int height)` | `Result<TextureHandle>` | Create an off-screen render target texture of the specified dimensions |
| `setRenderTarget(TextureHandle target)` | `Result<void>` | Redirect all subsequent draw calls to the specified render target |
| `resetRenderTarget()` | `void` | Reset rendering back to the default screen framebuffer |

### Screenshots

| Method | Returns | Description |
|--------|---------|-------------|
| `captureScreenshot(std::string_view path)` | `Result<void>` | Save the current frame to an image file at the given path |

## Types

### Sprite

A renderable 2D sprite with transform, tinting, layering, and anchor point.

| Field | Type | Default | Description |
|-------|------|---------|-------------|
| `textureHandle` | `AssetHandle*` | `nullptr` | Pointer to the texture asset handle to render |
| `sourceRect` | `Canvas` | `{}` | The source rectangle within the texture (for atlasing); empty = full texture |
| `transform` | `Transform2D` | `{}` | World-space position, rotation, and scale of the sprite |
| `tint` | `Color` | `Color::white()` | Color tint applied multiplicatively to the sprite |
| `layer` | `RenderLayer` | `0` | Render order layer; lower values draw first (behind higher values) |
| `anchor` | `Vec2` | `{0.5, 0.5}` | Pivot point for rotation and positioning (0,0 = top-left, 1,1 = bottom-right) |

### SpriteSheet

A texture atlas divided into a grid of uniformly-sized frames.

| Field | Type | Default | Description |
|-------|------|---------|-------------|
| `texture` | `AssetHandle` | `invalid` | The atlas texture asset handle |
| `frameWidth` | `int` | `32` | Width of each frame in pixels |
| `frameHeight` | `int` | `32` | Height of each frame in pixels |
| `columns` | `int` | `1` | Number of columns in the atlas grid |
| `rows` | `int` | `1` | Number of rows in the atlas grid |
| `padding` | `int` | `0` | Pixels of padding between frames |

### AnimatedSprite

A sprite sheet with named animations and automatic frame advancement.

| Field | Type | Default | Description |
|-------|------|---------|-------------|
| `sheet` | `SpriteSheet` | `{}` | The underlying sprite sheet atlas |
| `animations` | `std::unordered_map<std::string, Animation>` | `{}` | Named animation definitions mapping names to frame sequences |
| `currentAnimation` | `std::string` | `""` | Name of the currently playing animation |
| `currentFrameIndex` | `int` | `0` | Index into the current animation's frame list |
| `frameTimer` | `float` | `0.0f` | Accumulated time since the last frame change |
| `playing` | `bool` | `true` | Whether the animation is currently advancing |

### Camera2D

2D camera state for viewport control.

| Field | Type | Default | Description |
|-------|------|---------|-------------|
| `transform` | `Transform2D` | `{}` | Camera position, rotation, and scale in world coordinates |
| `zoom` | `float` | `1.0f` | Zoom level (1.0 = normal, 2.0 = 2x magnification) |
| `viewportSize` | `Size` | `{}` | The viewport dimensions in pixels |

### BlendMode

Controls how new pixels are combined with existing framebuffer pixels.

```cpp
enum class BlendMode : std::uint8_t {
    Opaque,        // No blending; fully replaces destination pixels
    AlphaTest,     // Discard pixels below alpha threshold; no blending
    AlphaBlend,    // Standard alpha blending (src * alpha + dst * (1 - alpha))
    Additive,      // Add source color to destination (for glows, particles)
    Multiply       // Multiply source and destination colors (for shadows, darkening)
};
```

| Value | Description |
|-------|-------------|
| `Opaque` | No blending; source pixel fully replaces the destination |
| `AlphaTest` | Pixels with alpha below a threshold are discarded; others are fully opaque |
| `AlphaBlend` | Standard transparency blending using the source alpha channel |
| `Additive` | Source color is added to the destination; useful for glows and fire effects |
| `Multiply` | Source and destination colors are multiplied; useful for shadows and tinting |

### WindowMode

```cpp
enum class WindowMode : std::uint8_t {
    Windowed,               // Standard resizable window
    Fullscreen,             // Exclusive fullscreen mode
    BorderlessFullscreen    // Borderless window covering the full screen
};
```

| Value | Description |
|-------|-------------|
| `Windowed` | Standard windowed mode with title bar and borders |
| `Fullscreen` | Exclusive fullscreen mode with potential resolution change |
| `BorderlessFullscreen` | Borderless window stretched to cover the entire screen |

## Lua Examples

```lua
-- High-level: Draw sprites
bestow.graphics.drawSprite("textures/player.png", { x = 100, y = 200 })
bestow.graphics.drawSprite("textures/coin.png", { x = 300, y = 150 }, { x = 32, y = 32 }, { r = 1, g = 1, b = 0, a = 1 })

-- High-level: Draw primitives
bestow.graphics.drawRect({ x = 50, y = 50 }, { x = 100, y = 60 }, { r = 1, g = 0, b = 0, a = 1 })
bestow.graphics.drawCircle({ x = 400, y = 300 }, 25, { r = 0, g = 1, b = 0, a = 1 }, true)
bestow.graphics.drawLine({ x = 0, y = 0 }, { x = 800, y = 600 }, { r = 1, g = 1, b = 1, a = 0.5 }, 2)
bestow.graphics.drawText("Score: 1000", { x = 10, y = 10 }, 24, { r = 1, g = 1, b = 1, a = 1 })

-- High-level: Camera
bestow.graphics.setCameraPosition({ x = playerX, y = playerY })
bestow.graphics.setCameraZoom(2.0)
local worldPos = bestow.graphics.screenToWorld({ x = mouseX, y = mouseY })
local screenPos = bestow.graphics.worldToScreen({ x = 100, y = 200 })

-- High-level: Window
local size = bestow.graphics.getWindowSize()
if bestow.graphics.shouldClose() then return end
bestow.graphics.setClearColor({ r = 0.1, g = 0.1, b = 0.2, a = 1 })

-- Low-level: Frame lifecycle
bestow.graphics.core.beginFrame()
-- ... all draw calls here ...
bestow.graphics.core.endFrame()

-- Low-level: Sprite batch rendering
bestow.graphics.core.drawBatch(spriteArray)
bestow.graphics.core.drawSpriteSheet(sheetHandle, 5, transform)

-- Low-level: Full primitive API
bestow.graphics.core.drawPolygon(vertices, { r = 0, g = 0, b = 1, a = 1 }, true)
bestow.graphics.core.drawArc(
    { x = 400, y = 300 }, 50,
    0, 3.14,
    { r = 1, g = 0.5, b = 0, a = 1 },
    64
)

-- Low-level: Font-based text
bestow.graphics.core.drawText("Hello", { x = 100, y = 50 }, fontHandle, 32)
bestow.graphics.core.drawTextCentered("Centered", { x = 400, y = 300 }, fontHandle, 48)
local textSize = bestow.graphics.core.measureText("Measure me", fontHandle, 24)

-- Low-level: Camera struct
bestow.graphics.core.setCamera({
    transform = { position = { x = 100, y = 200 }, rotation = 0, scale = { x = 1, y = 1 } },
    zoom = 1.5,
    viewportSize = { width = 800, height = 600 }
})
local cam = bestow.graphics.core.getCamera()

-- Low-level: Window management
bestow.graphics.core.setWindowSize({ width = 1920, height = 1080 })
bestow.graphics.core.setWindowMode("BorderlessFullscreen")
bestow.graphics.core.setWindowTitle("My Game")

-- Low-level: Render state
bestow.graphics.core.setVSync(true)
bestow.graphics.core.setBlendMode("AlphaBlend")

-- Low-level: Auto entity rendering
bestow.graphics.core.renderEntities(entityCore)
bestow.graphics.core.renderEntities(entityCore, -100, 50)  -- layer range
bestow.graphics.core.setViewportCulling(true)

-- Low-level: Render targets
local rt, err = bestow.graphics.core.createRenderTarget(512, 512)
if not err then
    bestow.graphics.core.setRenderTarget(rt)
    -- draw to texture ...
    bestow.graphics.core.resetRenderTarget()
end

-- Low-level: Screenshots
bestow.graphics.core.captureScreenshot("screenshot.png")
```

## C++ Examples

```cpp
// High-level: Simple drawing
graphics2D->drawSprite("textures/player.png", {100, 200});
graphics2D->drawRect({50, 50}, {100, 60}, Color::red());
graphics2D->drawCircle({400, 300}, 25, Color::green());
graphics2D->drawLine({0, 0}, {800, 600}, Color::white(), 2.0f);
graphics2D->drawText("Score: 1000", {10, 10}, 24.0f, Color::white());

graphics2D->setCameraPosition(playerPosition);
graphics2D->setCameraZoom(2.0f);
Vec2 worldPos = graphics2D->screenToWorld(mouseScreenPos);

Size winSize = graphics2D->getWindowSize();
if (graphics2D->shouldClose()) return;
graphics2D->setClearColor({0.1f, 0.1f, 0.2f, 1.0f});

// Low-level: Frame lifecycle
graphics2DCore->beginFrame();

// Low-level: Sprite rendering
graphics2DCore->draw(Sprite{
    .textureHandle = &playerTexture,
    .transform = {{100, 200}, 0.0f, {1, 1}},
    .tint = Color::white(),
    .layer = 30
});
graphics2DCore->drawBatch(spriteArray);
graphics2DCore->drawSpriteSheet(sheetHandle, frameIndex, transform);

// Low-level: Primitives
graphics2DCore->drawRect({50, 50}, {100, 60}, Color::red(), true);
graphics2DCore->drawLine({0, 0}, {800, 600}, Color::white(), 2.0f);
graphics2DCore->drawCircle({400, 300}, 25, Color::green(), true, 64);
graphics2DCore->drawPolygon(triangleVertices, Color::blue(), true);
graphics2DCore->drawArc({400, 300}, 50, 0.0f, 3.14f, Color::orange(), 48);

// Low-level: Text with font handles
graphics2DCore->drawText("Hello", {100, 50}, fontHandle, 32.0f, Color::white());
graphics2DCore->drawTextCentered("Centered", {400, 300}, fontHandle, 48.0f);
Vec2 textSize = graphics2DCore->measureText("Measure me", fontHandle, 24.0f);

// Low-level: Camera
graphics2DCore->setCamera(Camera2D{
    .transform = {{100, 200}, 0.0f, {1, 1}},
    .zoom = 1.5f,
    .viewportSize = {800, 600}
});
Camera2D cam = graphics2DCore->getCamera();
Vec2 world = graphics2DCore->screenToWorld(mousePos);

// Low-level: Window
graphics2DCore->setWindowSize({1920, 1080});
graphics2DCore->setWindowMode(WindowMode::BorderlessFullscreen);
graphics2DCore->setWindowTitle("My Game");

// Low-level: Render state
graphics2DCore->setClearColor({0.1f, 0.1f, 0.2f, 1.0f});
graphics2DCore->setVSync(true);
graphics2DCore->setBlendMode(BlendMode::AlphaBlend);

// Low-level: Auto entity rendering
graphics2DCore->renderEntities(*entityCore);
graphics2DCore->renderEntities(*entityCore, -100, 50);
graphics2DCore->setViewportCulling(true);

// Low-level: Render targets
auto rtResult = graphics2DCore->createRenderTarget(512, 512);
if (rtResult) {
    graphics2DCore->setRenderTarget(rtResult.value());
    // draw to off-screen target ...
    graphics2DCore->resetRenderTarget();
}

// Low-level: Screenshots
graphics2DCore->captureScreenshot("screenshot.png");

graphics2DCore->endFrame();
```

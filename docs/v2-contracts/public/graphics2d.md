# 2D Graphics System

> **Visibility:** Public (Lua + C++ API)
> **Tier:** 2
> **Dependencies:** Types, Entity, Assets, Graphics Context
> **Lua Paths:** `bestow.graphics2d` (high-level), `bestow.graphics2d.core` (low-level)

## Purpose

The 2D Graphics System provides sprite rendering, primitive drawing, text display, camera management, and window control for 2D games. It is the primary rendering interface for Bestow's 2D pipeline, supporting textured sprites with batching, sprite sheets and frame-based animation, geometric primitives (rectangles, circles, lines, polygons), bitmap and MSDF text rendering, a 2D camera with zoom and world/screen coordinate conversion, and automatic entity rendering sorted by render layer with viewport culling. The high-level API offers path-based drawing with sensible defaults for rapid prototyping: draw a sprite by texture path, draw text by font path, draw shapes by position and color, and position the camera with a single call. The low-level API exposes the full rendering pipeline: frame lifecycle control, batch sprite submission, sprite sheet and animated sprite rendering, handle-based font management with text measurement, full camera struct access, window mode and VSync control, render layer filtering, viewport culling toggle, and access to the underlying graphics context for UI backend integration.

## High-Level API: `IGraphics2DSystem`

The simplified API for common 2D rendering tasks. Accepts file paths instead of handles, creates and caches internal resources automatically, and exposes only the most common operations. No lifecycle methods -- the engine manages `beginFrame()`, `endFrame()`, and entity rendering internally.

### Sprite Rendering

| Method | Returns | Description |
|--------|---------|-------------|
| `drawSprite(std::string_view texturePath, Transform2D transform, Color tint = Color::white())` | `void` | Draw a textured sprite at the given transform; the texture is loaded and cached automatically from the file path |

### Text Rendering

| Method | Returns | Description |
|--------|---------|-------------|
| `drawText(std::string_view text, Vec2 position, std::string_view fontPath, float size, Color color = Color::white())` | `void` | Draw text at a screen position using a font loaded from the given path; the font is loaded and cached automatically |

### Primitive Drawing

| Method | Returns | Description |
|--------|---------|-------------|
| `drawRect(Vec2 position, Vec2 size, Color color, bool filled = true)` | `void` | Draw a rectangle at the given position with the given size; filled or outline only |
| `drawCircle(Vec2 center, float radius, Color color, bool filled = true)` | `void` | Draw a circle at the given center with the given radius; filled or outline only |
| `drawLine(Vec2 from, Vec2 to, Color color, float thickness = 1.0f)` | `void` | Draw a line segment between two points with the given color and thickness |

### Camera

| Method | Returns | Description |
|--------|---------|-------------|
| `setCamera(Vec2 position, float zoom = 1.0f)` | `void` | Position the 2D camera at a world-space location with an optional zoom level |
| `worldToScreen(Vec2 worldPos)` | `Vec2` | Convert a world-space position to screen-space coordinates |
| `screenToWorld(Vec2 screenPos)` | `Vec2` | Convert a screen-space position to world-space coordinates |

### Window

| Method | Returns | Description |
|--------|---------|-------------|
| `getWindowSize()` | `Size` | Return the current window dimensions in pixels |
| `setClearColor(Color color)` | `void` | Set the color used to clear the framebuffer each frame |

## Low-Level API: `IGraphics2DCore`

Full control API inheriting from `IGraphicsContext`. Exposes frame lifecycle management, batch sprite submission, sprite sheet and animated sprite rendering, handle-based font management with text measurement, full camera struct access, window mode and VSync control, automatic entity rendering with layer filtering and viewport culling, and graphics context access for UI backend integration.

### Frame Lifecycle

| Method | Returns | Description |
|--------|---------|-------------|
| `beginFrame()` | `void` | Begin a new rendering frame; clears the screen and prepares render state; must be called before any draw calls |
| `endFrame()` | `void` | End the current frame; flushes all draw calls and presents the result to the screen |

### Sprite Rendering

| Method | Returns | Description |
|--------|---------|-------------|
| `draw(const Sprite& sprite)` | `void` | Submit a single sprite for rendering using a fully configured Sprite struct with texture handle, source rect, transform, tint, layer, and anchor |
| `drawBatch(std::span<const Sprite> sprites)` | `void` | Submit multiple sprites in a single batched draw call for optimal performance; all sprites share the same draw state |
| `drawSprite(const SpriteSheet& sheet, int frameIndex, const Transform2D& transform, Color tint = Color::white())` | `void` | Draw a specific frame from a sprite sheet at the given transform with an optional color tint |
| `drawAnimatedSprite(AnimatedSprite& sprite, const Transform2D& transform, Color tint = Color::white())` | `void` | Draw the current frame of an animated sprite at the given transform; the AnimatedSprite's internal timer is not advanced by this call (use `AnimatedSprite::update(dt)` separately) |

### Primitive Drawing

| Method | Returns | Description |
|--------|---------|-------------|
| `drawRect(const Canvas& rect, const Color& color, bool filled = true)` | `void` | Draw a rectangle defined by a Canvas (origin + size) with the given color; filled or outline only |
| `drawLine(Vec2 from, Vec2 to, const Color& color, float thickness = 1.0f)` | `void` | Draw a line segment between two points with the given color and thickness |
| `drawCircle(Vec2 center, float radius, const Color& color, bool filled = true, int segments = 32)` | `void` | Draw a circle at the given center with configurable segment count for smoothness |
| `drawPolygon(std::span<const Vec2> vertices, const Color& color, bool filled = true)` | `void` | Draw a convex polygon from a list of vertex positions; vertices must be in winding order |

### Text Rendering

| Method | Returns | Description |
|--------|---------|-------------|
| `drawText(std::string_view text, Vec2 position, FontHandle font, float size, Color color = Color::white())` | `void` | Draw text at a position using a preloaded font handle with the given size and color |
| `drawTextCentered(std::string_view text, Vec2 position, FontHandle font, float size, Color color = Color::white())` | `void` | Draw text centered horizontally and vertically on the given position |
| `measureText(std::string_view text, FontHandle font, float size)` | `Vec2` | Measure the width and height that the given text would occupy without drawing it; useful for layout calculations |

### Camera

| Method | Returns | Description |
|--------|---------|-------------|
| `setCamera(const Camera& camera)` | `void` | Set the active 2D camera using a full Camera struct with transform, zoom, and viewport size |
| `getCamera()` | `Camera` | Return the current active camera state as a Camera struct |
| `worldToScreen(Vec2 worldPos)` | `Vec2` | Convert a world-space position to screen-space coordinates using the current camera transform and zoom |
| `screenToWorld(Vec2 screenPos)` | `Vec2` | Convert a screen-space position to world-space coordinates using the current camera transform and zoom |

### Window

| Method | Returns | Description |
|--------|---------|-------------|
| `getWindowSize()` | `Size` | Return the current window dimensions in pixels |
| `setWindowSize(Size size)` | `void` | Resize the window to the given dimensions in pixels |
| `getWindowMode()` | `WindowMode` | Return the current window mode (Windowed, Fullscreen, or BorderlessFullscreen) |
| `setWindowMode(WindowMode mode)` | `void` | Change the window mode |
| `shouldClose()` | `bool` | Return true if the window close has been requested by the user or operating system |

### Render State

| Method | Returns | Description |
|--------|---------|-------------|
| `setClearColor(const Color& color)` | `void` | Set the color used to clear the framebuffer each frame |
| `setVSync(bool enabled)` | `void` | Enable or disable vertical synchronization |

### Entity Rendering

| Method | Returns | Description |
|--------|---------|-------------|
| `renderEntities(IEntitySystem& entities)` | `void` | Automatically render all entities that have a Transform2D plus a visual component (Sprite, DebugRect, DebugCircle, DebugLine); entities are sorted by RenderLayer before drawing |
| `renderEntities(IEntitySystem& entities, RenderLayer minLayer, RenderLayer maxLayer)` | `void` | Render only entities whose render layer falls within the specified inclusive range; useful for separating background, gameplay, and UI passes |
| `setViewportCulling(bool enabled)` | `void` | Enable or disable viewport culling for automatic entity rendering; when enabled, entities whose world-space bounds fall outside the camera viewport are skipped |
| `isViewportCullingEnabled()` | `bool` | Return whether viewport culling is currently enabled |

### Graphics Context (Inherited from IGraphicsContext)

| Method | Returns | Description |
|--------|---------|-------------|
| `getUIRenderBackend()` | `IUIRenderBackend*` | Return the UI render backend for overlay rendering; returns nullptr if UI rendering is not supported |
| `isInFrame()` | `bool` | Check whether the system is currently between `beginFrame()` and `endFrame()` |
| `getNativeWindowHandle()` | `void*` | Return the native window handle (e.g., GLFWwindow*) for input system integration |

## Types

### Sprite

A fully configured sprite ready for rendering.

| Field | Type | Default | Description |
|-------|------|---------|-------------|
| `textureHandle` | `AssetHandle*` | `nullptr` | Pointer to the texture asset handle to render |
| `sourceRect` | `Canvas` | `{}` | Source rectangle within the texture for sprite sheet or atlas regions; empty means the full texture |
| `transform` | `Transform2D` | `{}` | World-space position, rotation, and scale of the sprite |
| `tint` | `Color` | `Color::white()` | Color tint multiplied with the texture color |
| `layer` | `RenderLayer` | `0` | Render order; lower layers are drawn first (behind higher layers) |
| `anchor` | `Vec2` | `{0.5, 0.5}` | Pivot point for rotation and positioning as a fraction of sprite size (0,0 = top-left, 0.5,0.5 = center, 1,1 = bottom-right) |

### SpriteSheet

A texture divided into a uniform grid of frames for animation or tilesets.

| Field | Type | Default | Description |
|-------|------|---------|-------------|
| `texture` | `AssetHandle` | -- | The texture asset containing all frames |
| `frameWidth` | `int` | `32` | Width of each frame in pixels |
| `frameHeight` | `int` | `32` | Height of each frame in pixels |
| `columns` | `int` | `1` | Number of columns in the sprite sheet grid |
| `rows` | `int` | `1` | Number of rows in the sprite sheet grid |
| `padding` | `int` | `0` | Pixel padding between frames in the grid |

### AnimatedSprite

A sprite sheet with named animations and playback state. Call `play(name)` to switch animations and `update(dt)` each frame to advance the timer.

| Field | Type | Default | Description |
|-------|------|---------|-------------|
| `sheet` | `SpriteSheet` | -- | The sprite sheet containing all animation frames |
| `animations` | `std::unordered_map<std::string, Animation>` | `{}` | Map of animation name to animation definition |
| `currentAnimation` | `std::string` | `""` | Name of the currently playing animation |
| `currentFrameIndex` | `int` | `0` | Index into the current animation's frame list |
| `frameTimer` | `float` | `0.0f` | Time accumulator for frame advancement |
| `playing` | `bool` | `true` | Whether the animation is currently advancing |

**Methods:**

| Method | Returns | Description |
|--------|---------|-------------|
| `play(const std::string& animName)` | `void` | Switch to a named animation; resets frame index and timer if the animation is different from the current one |
| `update(float dt)` | `void` | Advance the animation timer by delta time; automatically advances to the next frame when the current frame's duration expires |
| `getCurrentFrame()` | `int` | Return the sprite sheet frame index for the current animation frame |

### Animation

A named sequence of frames with per-frame timing.

| Field | Type | Default | Description |
|-------|------|---------|-------------|
| `name` | `std::string` | `""` | Animation name (e.g., "idle", "run", "attack") |
| `frames` | `std::vector<AnimationFrame>` | `{}` | Ordered list of frames with individual durations |
| `looping` | `bool` | `true` | Whether the animation loops back to the first frame after the last |

### AnimationFrame

A single frame within an animation sequence.

| Field | Type | Default | Description |
|-------|------|---------|-------------|
| `frameIndex` | `int` | -- | Index into the sprite sheet's frame grid |
| `duration` | `float` | -- | Duration in seconds this frame is displayed before advancing |

### Camera

2D camera state controlling the visible area of the world.

| Field | Type | Default | Description |
|-------|------|---------|-------------|
| `transform` | `Transform2D` | `{}` | Camera position and rotation in world space |
| `zoom` | `float` | `1.0f` | Zoom level; values greater than 1.0 zoom in (showing less world area), values less than 1.0 zoom out (showing more) |
| `viewportSize` | `Size` | `{}` | Size of the camera viewport in pixels; typically matches the window size |

### Canvas

A rectangle defined by an origin point and size, used for source rectangles and primitive drawing.

| Field | Type | Default | Description |
|-------|------|---------|-------------|
| `origin` | `Coordinate` | `{0, 0}` | Top-left corner position in pixels |
| `size` | `Size` | `{0, 0}` | Width and height in pixels |

### WindowMode

Window display mode options.

```cpp
enum class WindowMode : std::uint8_t {
    Windowed,              // Standard resizable window with title bar
    Fullscreen,            // Exclusive fullscreen with display mode change
    BorderlessFullscreen   // Borderless window covering the entire screen
};
```

| Value | Description |
|-------|-------------|
| `Windowed` | Standard windowed mode with title bar and borders |
| `Fullscreen` | Exclusive fullscreen mode; may change display resolution |
| `BorderlessFullscreen` | Borderless window stretched to cover the entire screen at desktop resolution |

### RenderLayer

```cpp
using RenderLayer = std::int32_t;
```

An integer controlling draw order. Lower values are drawn first (behind), higher values are drawn last (in front). Bestow provides predefined layer constants in the `RenderLayers` namespace:

| Constant | Value | Description |
|----------|-------|-------------|
| `RenderLayers::Background` | `-100` | Background imagery and parallax layers |
| `RenderLayers::BackgroundDecor` | `-50` | Background decorations behind gameplay |
| `RenderLayers::Platforms` | `0` | Level geometry and platforms |
| `RenderLayers::Items` | `10` | Collectibles and interactive objects |
| `RenderLayers::Enemies` | `20` | Enemy entities |
| `RenderLayers::Player` | `30` | Player character |
| `RenderLayers::Effects` | `40` | Particle effects and visual feedback |
| `RenderLayers::Foreground` | `50` | Foreground decorations in front of gameplay |
| `RenderLayers::UI` | `100` | UI elements rendered on top of the game world |
| `RenderLayers::Debug` | `1000` | Debug overlays and visualization |

### FontHandle

```cpp
using FontHandle = Handle<struct FontTag>;
```

A strong typed handle identifying a loaded font resource. Obtained through the asset system and used with the low-level text rendering methods.

## Lua Mapping

High-level functions are accessed via `bestow.graphics2d.<method>`. Low-level functions are accessed via `bestow.graphics2d.core.<method>`. Colors map to Lua tables with `{r, g, b, a}` fields using 0-255 integer values. Vec2 maps to `{x, y}`. Transform2D maps to `{x, y, rotation, scaleX, scaleY}`. Canvas maps to `{origin = {x, y}, size = {width, height}}`. Size maps to `{width, height}`. Enums map to strings (e.g., `"Windowed"`, `"Fullscreen"`, `"BorderlessFullscreen"`). AnimatedSprite objects in Lua have `:play(name)` and `:update(dt)` methods available directly.

## Examples

### Lua

```lua
-- High-level: Draw sprites
bestow.graphics2d.setClearColor({ r = 30, g = 30, b = 50, a = 255 })
bestow.graphics2d.drawSprite("textures/player.png", { x = 100, y = 200 })
bestow.graphics2d.drawSprite("textures/enemy.png",
    { x = 300, y = 200, rotation = 0.5, scaleX = 2, scaleY = 2 },
    { r = 255, g = 100, b = 100, a = 255 })

-- High-level: Draw text
bestow.graphics2d.drawText("Score: 1000", { x = 10, y = 10 },
    "fonts/pixel.ttf", 24, { r = 255, g = 255, b = 0, a = 255 })

-- High-level: Draw primitives
bestow.graphics2d.drawRect({ x = 50, y = 50 }, { x = 100, y = 60 },
    { r = 255, g = 0, b = 0, a = 128 }, true)
bestow.graphics2d.drawCircle({ x = 400, y = 300 }, 25,
    { r = 0, g = 255, b = 0, a = 255 }, false)
bestow.graphics2d.drawLine({ x = 0, y = 0 }, { x = 800, y = 600 },
    { r = 255, g = 255, b = 255, a = 255 }, 2)

-- High-level: Camera
bestow.graphics2d.setCamera({ x = playerX, y = playerY }, 1.5)
local screenPos = bestow.graphics2d.worldToScreen({ x = 100, y = 200 })
local worldPos = bestow.graphics2d.screenToWorld({ x = mouseX, y = mouseY })

-- High-level: Window
local size = bestow.graphics2d.getWindowSize()
print("Window: " .. size.width .. "x" .. size.height)

-- Low-level: Frame lifecycle (typically managed by engine)
bestow.graphics2d.core.beginFrame()

-- Low-level: Single sprite with full control
bestow.graphics2d.core.draw({
    transform = { x = 100, y = 200, rotation = 0, scaleX = 1, scaleY = 1 },
    tint = { r = 255, g = 255, b = 255, a = 255 },
    layer = 30,
    anchor = { x = 0.5, y = 0.5 }
})

-- Low-level: Batch sprite rendering
bestow.graphics2d.core.drawBatch(spriteArray)

-- Low-level: Sprite sheet animation
bestow.graphics2d.core.drawSprite(playerSheet, 3,
    { x = 100, y = 200 })

-- Low-level: Animated sprite
playerAnim:play("run")
playerAnim:update(dt)
bestow.graphics2d.core.drawAnimatedSprite(playerAnim,
    { x = 100, y = 200 })

-- Low-level: Primitives with full control
bestow.graphics2d.core.drawRect(
    { origin = { x = 50, y = 50 }, size = { width = 100, height = 60 } },
    { r = 255, g = 0, b = 0, a = 128 }, true)
bestow.graphics2d.core.drawCircle({ x = 400, y = 300 }, 25,
    { r = 0, g = 255, b = 0, a = 255 }, true, 64)
bestow.graphics2d.core.drawLine({ x = 0, y = 0 }, { x = 800, y = 600 },
    { r = 255, g = 255, b = 255, a = 255 }, 2)
bestow.graphics2d.core.drawPolygon(
    { { x = 100, y = 100 }, { x = 150, y = 50 }, { x = 200, y = 100 } },
    { r = 0, g = 0, b = 255, a = 255 }, true)

-- Low-level: Text with font handles
bestow.graphics2d.core.drawText("Hello World", { x = 100, y = 50 },
    myFontHandle, 32, { r = 255, g = 255, b = 255, a = 255 })
bestow.graphics2d.core.drawTextCentered("GAME OVER",
    { x = 400, y = 300 }, titleFontHandle, 64,
    { r = 255, g = 0, b = 0, a = 255 })
local textSize = bestow.graphics2d.core.measureText(
    "Score: 9999", myFontHandle, 24)
print("Text width: " .. textSize.x .. " height: " .. textSize.y)

-- Low-level: Full camera control
bestow.graphics2d.core.setCamera({
    transform = { x = playerX, y = playerY, rotation = 0, scaleX = 1, scaleY = 1 },
    zoom = 2.0,
    viewportSize = { width = 1280, height = 720 }
})
local cam = bestow.graphics2d.core.getCamera()
print("Camera zoom: " .. cam.zoom)

-- Low-level: Window management
bestow.graphics2d.core.setWindowSize({ width = 1920, height = 1080 })
bestow.graphics2d.core.setWindowMode("BorderlessFullscreen")
bestow.graphics2d.core.setVSync(true)
if bestow.graphics2d.core.shouldClose() then
    -- handle quit
end

-- Low-level: Entity rendering with layer filtering
bestow.graphics2d.core.renderEntities(entities)
bestow.graphics2d.core.renderEntities(entities, -100, 0)   -- backgrounds only
bestow.graphics2d.core.renderEntities(entities, 1, 50)     -- gameplay objects
bestow.graphics2d.core.renderEntities(entities, 100, 1000)  -- UI and debug

-- Low-level: Viewport culling
bestow.graphics2d.core.setViewportCulling(true)
local culling = bestow.graphics2d.core.isViewportCullingEnabled()

bestow.graphics2d.core.endFrame()
```

### C++

```cpp
// High-level: Quick 2D rendering
graphics2D->setClearColor(Color{30, 30, 50, 255});
graphics2D->drawSprite("textures/player.png", Transform2D{100, 200});
graphics2D->drawSprite("textures/enemy.png",
    Transform2D{300, 200, 0.5f, 2.0f, 2.0f}, Color::red());

graphics2D->drawText("Score: 1000", {10, 10}, "fonts/pixel.ttf", 24.0f,
    Color{255, 255, 0, 255});

graphics2D->drawRect({50, 50}, {100, 60}, Color::red(), true);
graphics2D->drawCircle({400, 300}, 25.0f, Color::green(), false);
graphics2D->drawLine({0, 0}, {800, 600}, Color::white(), 2.0f);

graphics2D->setCamera({playerX, playerY}, 1.5f);
Vec2 screenPos = graphics2D->worldToScreen({100, 200});
Vec2 worldPos = graphics2D->screenToWorld({mouseX, mouseY});

Size winSize = graphics2D->getWindowSize();

// Low-level: Frame lifecycle
graphics2DCore->beginFrame();

// Low-level: Single sprite rendering
graphics2DCore->draw(Sprite{
    .textureHandle = &playerTexture,
    .sourceRect = {{0, 0}, {64, 64}},
    .transform = {100, 200, 0, 1, 1},
    .tint = Color::white(),
    .layer = RenderLayers::Player,
    .anchor = {0.5f, 0.5f}
});

// Low-level: Batch sprite rendering for bullets, particles, etc.
std::vector<Sprite> bulletSprites = buildBulletSprites();
graphics2DCore->drawBatch(bulletSprites);

// Low-level: Sprite sheet frame drawing
graphics2DCore->drawSprite(playerSheet, currentFrame,
    Transform2D{100, 200});

// Low-level: Animated sprite with playback control
playerAnim.play("run");
playerAnim.update(dt);
graphics2DCore->drawAnimatedSprite(playerAnim,
    Transform2D{100, 200});

// Low-level: Primitive drawing
graphics2DCore->drawRect(Canvas{{50, 50}, {100, 60}},
    Color::red(), true);
graphics2DCore->drawLine({0, 0}, {800, 600},
    Color::white(), 2.0f);
graphics2DCore->drawCircle({400, 300}, 25.0f,
    Color::green(), true, 64);
std::vector<Vec2> triangle = {{100, 100}, {150, 50}, {200, 100}};
graphics2DCore->drawPolygon(triangle, Color::blue(), true);

// Low-level: Text rendering with font handles
graphics2DCore->drawText("Hello World", {100, 50},
    pixelFont, 32.0f, Color::white());
graphics2DCore->drawTextCentered("GAME OVER", {400, 300},
    titleFont, 64.0f, Color::red());
Vec2 textSize = graphics2DCore->measureText("Score: 9999",
    pixelFont, 24.0f);

// Low-level: Full camera control
graphics2DCore->setCamera(Camera{
    .transform = {playerX, playerY, 0, 1, 1},
    .zoom = 2.0f,
    .viewportSize = {1280, 720}
});
Camera cam = graphics2DCore->getCamera();
Vec2 world = graphics2DCore->screenToWorld({mouseX, mouseY});
Vec2 screen = graphics2DCore->worldToScreen({100, 200});

// Low-level: Window management
graphics2DCore->setWindowSize({1920, 1080});
graphics2DCore->setWindowMode(WindowMode::BorderlessFullscreen);
graphics2DCore->setVSync(true);
if (graphics2DCore->shouldClose()) {
    engine.quit();
}

// Low-level: Render state
graphics2DCore->setClearColor(Color{30, 30, 50, 255});

// Low-level: Automatic entity rendering with layer filtering
graphics2DCore->renderEntities(*entities);
graphics2DCore->renderEntities(*entities,
    RenderLayers::Background, RenderLayers::Platforms);
graphics2DCore->renderEntities(*entities,
    RenderLayers::Items, RenderLayers::Foreground);
graphics2DCore->renderEntities(*entities,
    RenderLayers::UI, RenderLayers::Debug);

// Low-level: Viewport culling
graphics2DCore->setViewportCulling(true);
bool culling = graphics2DCore->isViewportCullingEnabled();

// Low-level: Graphics context access for UI integration
auto* uiBackend = graphics2DCore->getUIRenderBackend();
bool inFrame = graphics2DCore->isInFrame();
void* nativeWindow = graphics2DCore->getNativeWindowHandle();

graphics2DCore->endFrame();
```

# GraphicsSystem API

The `GraphicsSystem` provides 2D rendering with OpenGL, including sprites, primitives, text, and automatic entity rendering.

## Overview

```cpp
auto& graphics = sys.graphics;

// Frame lifecycle
graphics->beginFrame();

// Render entities automatically
graphics->renderEntities(*sys.entities);

// Manual rendering
graphics->drawRect({{100, 100}, {50, 50}}, Color::red(), true);
graphics->drawText("Score: 100", Vec2{10, 10}, fontHandle, 24.0f);

graphics->endFrame();
```

## Frame Lifecycle

### beginFrame()

```cpp
void beginFrame();
```

Begins a new frame. Clears the screen and prepares for rendering.

**Call once per frame** before any draw calls.

---

### endFrame()

```cpp
void endFrame();
```

Ends the frame and swaps buffers. Presents the rendered image to the screen.

**Call once per frame** after all draw calls.

---

## Automatic Entity Rendering

### renderEntities(IEntitySystem& entities)

```cpp
void renderEntities(IEntitySystem& entities);
```

Automatically renders all entities with visual components:
- `Sprite` + `Transform2D`
- `AnimatedSprite` + `Transform2D`
- `DebugRect` + `Transform2D`
- `DebugCircle` + `Transform2D`
- `DebugLine` + `Transform2D`

**Sorted by `RenderLayer`** (lower values render first).

**Example:**

```cpp
void render(float alpha) override {
    graphics->beginFrame();
    graphics->renderEntities(*sys.entities);
    graphics->endFrame();
}
```

---

### renderEntities(IEntitySystem& entities, RenderLayer minLayer, RenderLayer maxLayer)

```cpp
void renderEntities(IEntitySystem& entities,
                   RenderLayer minLayer,
                   RenderLayer maxLayer);
```

Renders only entities within a specific layer range.

**Example:**

```cpp
// Render background layers
graphics->renderEntities(*sys.entities,
    RenderLayers::Background,
    RenderLayers::BackgroundDecor
);

// Render gameplay layers
graphics->renderEntities(*sys.entities,
    RenderLayers::Platforms,
    RenderLayers::Effects
);

// Render UI on top
graphics->renderEntities(*sys.entities,
    RenderLayers::UI,
    RenderLayers::UI
);
```

---

### Viewport Culling

```cpp
void setViewportCulling(bool enabled);
bool isViewportCullingEnabled() const;
```

Enables/disables automatic culling of entities outside the camera view.

**Default:** Enabled

---

## Sprite Rendering

### draw(const Sprite& sprite)

```cpp
void draw(const Sprite& sprite);
```

Draws a single sprite.

**Example:**

```cpp
Sprite sprite{
    .textureHandle = &playerTexture,
    .sourceRect = {{0, 0}, {32, 32}},
    .transform = {.x = 100.0f, .y = 200.0f},
    .tint = Color::white(),
    .layer = 0,
    .anchor = {0.5f, 0.5f}  // Center pivot
};
graphics->draw(sprite);
```

---

### drawBatch(std::span<const Sprite> sprites)

```cpp
void drawBatch(std::span<const Sprite> sprites);
```

Draws multiple sprites efficiently in a single batch.

**Example:**

```cpp
std::vector<Sprite> particles;
// Fill particles...
graphics->drawBatch(particles);
```

---

### drawSprite(const SpriteSheet& sheet, int frameIndex, const Transform2D& transform, Color tint)

```cpp
void drawSprite(const SpriteSheet& sheet, int frameIndex,
               const Transform2D& transform, Color tint = Color::white());
```

Draws a single frame from a sprite sheet.

**Example:**

```cpp
SpriteSheet playerSheet{
    .texture = playerTexture,
    .frameWidth = 32,
    .frameHeight = 32,
    .columns = 8,
    .rows = 4
};

Transform2D transform{.x = 100.0f, .y = 200.0f};
graphics->drawSprite(playerSheet, 5, transform);  // Frame 5
```

---

### drawAnimatedSprite(AnimatedSprite& sprite, const Transform2D& transform, Color tint)

```cpp
void drawAnimatedSprite(AnimatedSprite& sprite,
                       const Transform2D& transform,
                       Color tint = Color::white());
```

Draws an animated sprite at the current frame.

**Note:** Call `sprite.update(dt)` in your update loop to advance the animation.

---

## Primitive Rendering (Debug)

### drawRect(const Canvas& rect, const Color& color, bool filled)

```cpp
void drawRect(const Canvas& rect, const Color& color, bool filled = true);
```

Draws a rectangle.

**Example:**

```cpp
// Filled rectangle
graphics->drawRect({{100, 100}, {50, 30}}, Color::red(), true);

// Outline only
graphics->drawRect({{200, 100}, {50, 30}}, Color::green(), false);
```

---

### drawLine(Vec2 from, Vec2 to, const Color& color, float thickness)

```cpp
void drawLine(Vec2 from, Vec2 to, const Color& color, float thickness = 1.0f);
```

Draws a line between two points.

**Example:**

```cpp
graphics->drawLine(Vec2{0, 0}, Vec2{100, 100}, Color::white(), 2.0f);
```

---

### drawCircle(Vec2 center, float radius, const Color& color, bool filled, int segments)

```cpp
void drawCircle(Vec2 center, float radius, const Color& color,
               bool filled = true, int segments = 32);
```

Draws a circle.

**Example:**

```cpp
// Filled circle
graphics->drawCircle(Vec2{100, 100}, 50.0f, Color::blue(), true);

// Low-poly outline (hexagon)
graphics->drawCircle(Vec2{200, 100}, 50.0f, Color::red(), false, 6);
```

---

### drawPolygon(std::span<const Vec2> vertices, const Color& color, bool filled)

```cpp
void drawPolygon(std::span<const Vec2> vertices, const Color& color,
                bool filled = true);
```

Draws a polygon from vertices.

**Example:**

```cpp
std::vector<Vec2> triangle = {
    {100.0f, 50.0f},
    {150.0f, 150.0f},
    {50.0f, 150.0f}
};
graphics->drawPolygon(triangle, Color::green(), true);
```

---

## Text Rendering

### drawText(const std::string& text, Vec2 position, AssetHandle fontHandle, float size, const Color& color)

```cpp
void drawText(const std::string& text, Vec2 position,
             AssetHandle fontHandle, float size,
             const Color& color = Color::white());
```

Draws text at the specified position (top-left corner).

**Example:**

```cpp
// Load font first
AssetHandle font = assets->registerAsset(AssetType::Font, "fonts/arial.ttf");
assets->loadAsset(font);

// Draw text
graphics->drawText("Score: 100", Vec2{10, 10}, font, 24.0f, Color::white());
```

---

### drawTextCentered(const std::string& text, Vec2 position, AssetHandle fontHandle, float size, const Color& color)

```cpp
void drawTextCentered(const std::string& text, Vec2 position,
                     AssetHandle fontHandle, float size,
                     const Color& color = Color::white());
```

Draws text centered at the specified position.

**Example:**

```cpp
// Center title on screen
Size window = graphics->getWindowSize();
Vec2 center{window.width / 2.0f, 100.0f};
graphics->drawTextCentered("GAME OVER", center, font, 48.0f, Color::red());
```

---

### measureText(const std::string& text, AssetHandle fontHandle, float size)

```cpp
Vec2 measureText(const std::string& text, AssetHandle fontHandle, float size) const;
```

Measures the bounding box of text without rendering it.

**Returns:** `Vec2{width, height}` in pixels

**Example:**

```cpp
Vec2 size = graphics->measureText("Hello", font, 24.0f);
// Draw background box
graphics->drawRect({{10, 10}, {(int)size.x, (int)size.y}}, Color::black());
// Draw text on top
graphics->drawText("Hello", Vec2{10, 10}, font, 24.0f);
```

---

## Camera

### setCamera(const Camera& camera)

```cpp
void setCamera(const Camera& camera);
Camera getCamera() const;
```

Sets the active camera for rendering.

**Camera Structure:**

```cpp
struct Camera {
    Transform2D transform;
    float zoom = 1.0f;
    Size viewportSize;
};
```

**Example:**

```cpp
Camera cam{
    .transform = {.x = 400.0f, .y = 300.0f},
    .zoom = 1.5f,
    .viewportSize = {1280, 720}
};
graphics->setCamera(cam);
```

**Tip:** Use `CameraSystem` for automatic camera following and effects.

---

### worldToScreen(Vec2 worldPos)

```cpp
Vec2 worldToScreen(Vec2 worldPos) const;
```

Converts world coordinates to screen coordinates.

**Example:**

```cpp
Vec2 worldPos = physics->getPosition(player);
Vec2 screenPos = graphics->worldToScreen(worldPos);
// Draw UI element at player's screen position
```

---

### screenToWorld(Vec2 screenPos)

```cpp
Vec2 screenToWorld(Vec2 screenPos) const;
```

Converts screen coordinates to world coordinates.

**Example:**

```cpp
Vec2 mouseScreen = input->getMousePosition();
Vec2 mouseWorld = graphics->screenToWorld(mouseScreen);
// Spawn entity at mouse click
```

---

## Window Management

### getWindowSize()

```cpp
Size getWindowSize() const;
```

Returns the current window size in pixels.

---

### setWindowSize(Size size)

```cpp
void setWindowSize(Size size);
```

Resizes the window.

---

### isFullscreen()

```cpp
bool isFullscreen() const;
```

Returns `true` if fullscreen mode is active.

---

### setFullscreen(bool fullscreen)

```cpp
void setFullscreen(bool fullscreen);
```

Toggles fullscreen mode.

---

### shouldClose()

```cpp
bool shouldClose() const;
```

Returns `true` if the user requested to close the window.

---

### getNativeWindowHandle()

```cpp
void* getNativeWindowHandle() const;
```

Returns the native window handle (`GLFWwindow*`).

**Use for:** Integrating third-party libraries that need the window handle.

---

## Render State

### setClearColor(const Color& color)

```cpp
void setClearColor(const Color& color);
```

Sets the background color used by `beginFrame()`.

**Example:**

```cpp
graphics->setClearColor(Color{0, 128, 255, 255});  // Sky blue
```

---

### setVSync(bool enabled)

```cpp
void setVSync(bool enabled);
```

Enables or disables vertical sync.

**Default:** Enabled

---

## Asset System Integration

### setAssetSystem(IAssetSystem* assets)

```cpp
void setAssetSystem(IAssetSystem* assets);
```

Wires the graphics system to the asset system for texture loading.

**Note:** Automatically called by `EngineBuilder`.

---

## Render Layers

Predefined render layer constants:

```cpp
namespace RenderLayers {
    inline constexpr RenderLayer Background = -100;
    inline constexpr RenderLayer BackgroundDecor = -50;
    inline constexpr RenderLayer Platforms = 0;
    inline constexpr RenderLayer Items = 10;
    inline constexpr RenderLayer Enemies = 20;
    inline constexpr RenderLayer Player = 30;
    inline constexpr RenderLayer Effects = 40;
    inline constexpr RenderLayer Foreground = 50;
    inline constexpr RenderLayer UI = 100;
    inline constexpr RenderLayer Debug = 1000;
}
```

**Lower values render first** (further back).

---

## Common Patterns

### Parallax Background Layers

```cpp
// Background scrolls slower than camera
void renderParallax() {
    Camera cam = graphics->getCamera();

    // Far background (0.3x speed)
    Camera bgCam = cam;
    bgCam.transform.x *= 0.3f;
    graphics->setCamera(bgCam);
    graphics->renderEntities(*sys.entities,
        RenderLayers::Background,
        RenderLayers::Background
    );

    // Close background (0.7x speed)
    bgCam.transform.x = cam.transform.x * 0.7f;
    graphics->setCamera(bgCam);
    graphics->renderEntities(*sys.entities,
        RenderLayers::BackgroundDecor,
        RenderLayers::BackgroundDecor
    );

    // Foreground (normal speed)
    graphics->setCamera(cam);
    graphics->renderEntities(*sys.entities,
        RenderLayers::Platforms,
        RenderLayers::UI
    );
}
```

---

### Debug Visualization

```cpp
void renderDebug() {
    if (!showDebug) return;

    // Physics body outlines
    for (auto [entity, transform] : entities->view<Transform2D, PhysicsBody>().each()) {
        Vec2 size = physics->getBodySize(entity);
        graphics->drawRect(
            {{(int)transform.x - size.x/2, (int)transform.y - size.y/2},
             {(int)size.x, (int)size.y}},
            Color::green(),
            false  // outline only
        );
    }

    // Velocity vectors
    for (auto [entity, transform] : entities->view<Transform2D, Velocity>().each()) {
        Vec2 vel = physics->getVelocity(entity);
        graphics->drawLine(
            {transform.x, transform.y},
            {transform.x + vel.x * 0.1f, transform.y + vel.y * 0.1f},
            Color::red(),
            2.0f
        );
    }
}
```

---

### Screen-Space UI

```cpp
void renderUI() {
    // Save camera
    Camera worldCam = graphics->getCamera();

    // Set identity camera for screen-space rendering
    Camera uiCam{
        .transform = {.x = 0.0f, .y = 0.0f},
        .zoom = 1.0f,
        .viewportSize = graphics->getWindowSize()
    };
    graphics->setCamera(uiCam);

    // Draw UI elements in screen space
    graphics->drawText("Score: 100", Vec2{10, 10}, font, 24.0f);
    graphics->drawRect({{10, 50}, {100, 20}}, Color::red());  // Health bar

    // Restore world camera
    graphics->setCamera(worldCam);
}
```

---

### Render to Texture (Advanced)

```cpp
// Not directly supported - use OpenGL FBO manually
void* window = graphics->getNativeWindowHandle();
// Use OpenGL API directly for advanced rendering
```

---

## Performance Tips

1. **Batch sprites** - Use `drawBatch()` for particles and similar sprites
2. **Layer rendering** - Render by layer to minimize state changes
3. **Cull offscreen** - Keep viewport culling enabled
4. **Minimize draw calls** - Combine primitives when possible
5. **Text caching** - Don't measure text every frame

## See Also

- [EntitySystem](EntitySystem.md) - Components for visual entities
- [AssetSystem](AssetSystem.md) - Loading textures and fonts
- [CameraSystem](CameraSystem.md) - Camera control and effects
- [InputSystem](InputSystem.md) - Mouse position for UI

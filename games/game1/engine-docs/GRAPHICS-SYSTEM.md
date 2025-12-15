# Graphics System Guide

> **Comprehensive guide for 2D rendering in Bestow Engine**

## Table of Contents

1. [Overview](#overview)
2. [Frame Lifecycle](#frame-lifecycle)
3. [Sprite Rendering](#sprite-rendering)
4. [Primitive Rendering](#primitive-rendering)
5. [Text Rendering](#text-rendering)
6. [Camera System](#camera-system)
7. [Entity Rendering](#entity-rendering)
8. [Window Management](#window-management)
9. [Render State](#render-state)
10. [Best Practices](#best-practices)
11. [Complete Examples](#complete-examples)

---

## Overview

The **IGraphicsSystem** provides complete 2D rendering capabilities for Bestow games. It handles sprites, primitives, text, and automatic entity rendering with layer-based sorting.

### Key Features

- **Sprite rendering** - Individual sprites and batch rendering for performance
- **Sprite sheets & animation** - Frame-based animation system
- **Primitive shapes** - Rectangles, circles, lines, and polygons for debug visualization
- **Text rendering** - MSDF (Multi-channel Signed Distance Field) for crisp text at any size
- **Camera system** - Viewport control with zoom and coordinate conversion
- **Entity rendering** - Automatic rendering from ECS components with layer sorting
- **Viewport culling** - Only render entities visible in the camera view

### Interface Location

```cpp
// bestow-contract/src/bestow.graphics.cppm
import bestow.graphics;

// Access through engine
auto& sys = engine.systems();
sys.graphics->beginFrame();
```

---

## Frame Lifecycle

Every frame must begin and end properly. All rendering calls must occur between `beginFrame()` and `endFrame()`.

```cpp
void MyGame::render(float alpha) {
    auto& sys = engine_->systems();

    // Begin the frame (clears screen, resets state)
    sys.graphics->beginFrame();

    // All rendering calls go here
    sys.graphics->renderEntities(*sys.entities);
    renderUI();

    // End the frame (presents to screen)
    sys.graphics->endFrame();
}
```

**Visual:**
```
┌─────────────────┐
│  beginFrame()   │ ← Clear screen, prepare for drawing
├─────────────────┤
│  draw(...)      │
│  drawBatch(...) │ ← All rendering calls
│  drawText(...)  │
│  renderEntities │
├─────────────────┤
│  endFrame()     │ ← Present to screen, swap buffers
└─────────────────┘
```

---

## Sprite Rendering

### Simple Sprite Drawing

```cpp
void draw(const Sprite& sprite);
```

**Example:**

```cpp
// Load texture through AssetSystem
AssetHandle playerTexture = sys.assets->registerAsset(
    AssetType::Texture, "textures/player.png"
);
sys.assets->loadAsset(playerTexture);

// Create sprite
Sprite sprite{
    .textureHandle = &playerTexture,
    .sourceRect = Canvas{{0, 0}, {32, 32}},  // Source region in texture
    .transform = Transform2D{
        .x = 100,
        .y = 200,
        .rotation = 0.0f,
        .scaleX = 1.0f,
        .scaleY = 1.0f
    },
    .tint = Color::white(),
    .layer = RenderLayers::Player,
    .anchor = {0.5f, 0.5f}  // Pivot point: (0,0) = top-left, (0.5,0.5) = center
};

// Draw the sprite
sys.graphics->draw(sprite);
```

**Visual:**
```
Anchor points:
  (0,0)─────────(0.5,0)─────────(1,0)
    │             │              │
    │             │              │
(0,0.5)────────(0.5,0.5)────────(1,0.5)  ← Center anchor
    │             │              │
    │             │              │
  (0,1)────────(0.5,1)──────────(1,1)
```

### Batch Rendering

For better performance when rendering many sprites:

```cpp
void drawBatch(std::span<const Sprite> sprites);
```

**Example:**

```cpp
// Collect sprites into a batch
std::vector<Sprite> enemies;
for (const auto& enemy : enemyList) {
    enemies.push_back(Sprite{
        .textureHandle = &enemyTexture,
        .sourceRect = Canvas{{0, 0}, {32, 32}},
        .transform = enemy.transform,
        .tint = enemy.color,
        .layer = RenderLayers::Enemies
    });
}

// Single draw call for all enemies
sys.graphics->drawBatch(enemies);
```

**Performance:**
```
Without batching:  100 sprites = 100 draw calls
With batching:     100 sprites = 1 draw call (100x faster!)
```

### Sprite Sheet Rendering

```cpp
void drawSprite(const SpriteSheet& sheet, int frameIndex,
               const Transform2D& transform, Color tint = Color::white());
```

**Example:**

```cpp
// Define sprite sheet layout
SpriteSheet coinSheet{
    .texture = coinTexture,
    .frameWidth = 16,
    .frameHeight = 16,
    .columns = 8,   // 8 frames per row
    .rows = 1,      // 1 row
    .padding = 0    // No padding between frames
};

// Draw specific frame
int frameIndex = 3;  // Fourth frame (0-indexed)
Transform2D transform{.x = 100, .y = 200};
sys.graphics->drawSprite(coinSheet, frameIndex, transform, Color::white());
```

**Visual:**
```
Sprite sheet layout (8 columns × 1 row):
┌────┬────┬────┬────┬────┬────┬────┬────┐
│ 0  │ 1  │ 2  │ 3  │ 4  │ 5  │ 6  │ 7  │
└────┴────┴────┴────┴────┴────┴────┴────┘
  ^frameIndex 3 draws the 4th frame
```

### Animated Sprite Rendering

```cpp
void drawAnimatedSprite(AnimatedSprite& sprite,
                       const Transform2D& transform,
                       Color tint = Color::white());
```

**Example:**

```cpp
// Define animation
AnimatedSprite player;
player.sheet = playerSheet;
player.animations["idle"] = Animation{
    .name = "idle",
    .frames = {
        {0, 0.1f},  // Frame 0 for 0.1 seconds
        {1, 0.1f},  // Frame 1 for 0.1 seconds
        {2, 0.1f},  // Frame 2 for 0.1 seconds
        {3, 0.1f}   // Frame 3 for 0.1 seconds
    },
    .looping = true
};

player.animations["run"] = Animation{
    .name = "run",
    .frames = {
        {4, 0.08f}, {5, 0.08f}, {6, 0.08f}, {7, 0.08f}
    },
    .looping = true
};

// Start animation
player.play("idle");

// Update in game loop (fixed timestep)
void updateFixed(float dt) {
    player.update(dt);  // Advances frame timer
}

// Render in render loop
void render(float alpha) {
    sys.graphics->beginFrame();
    sys.graphics->drawAnimatedSprite(player, playerTransform);
    sys.graphics->endFrame();
}
```

---

## Primitive Rendering

Perfect for prototyping, debug visualization, and physics debugging.

### Rectangle

```cpp
void drawRect(const Canvas& rect, const Color& color, bool filled = true);
```

**Example:**

```cpp
// Filled rectangle
Canvas rect{{100, 100}, {50, 50}};  // Position (100,100), size 50×50
sys.graphics->drawRect(rect, Color::red(), true);

// Outline only
sys.graphics->drawRect(rect, Color::green(), false);
```

**Visual:**
```
Filled:              Outline:
┏━━━━━━━━━┓         ┌─────────┐
┃█████████┃         │         │
┃█████████┃         │         │
┃█████████┃         │         │
┗━━━━━━━━━┛         └─────────┘
```

### Circle

```cpp
void drawCircle(Vec2 center, float radius, const Color& color,
               bool filled = true, int segments = 32);
```

**Example:**

```cpp
// Filled circle
sys.graphics->drawCircle(
    Vec2{200, 200},  // Center
    25.0f,           // Radius
    Color::green(),
    true,            // Filled
    32               // Segments (higher = smoother)
);

// Outline circle
sys.graphics->drawCircle(
    Vec2{200, 200}, 25.0f, Color::blue(), false, 64
);
```

**Visual:**
```
segments=8 (low):    segments=32 (smooth):
    ╱──╲                 ╭───╮
   ╱    ╲               ╱     ╲
  │      │             │       │
   ╲    ╱               ╲     ╱
    ╲──╱                 ╰───╯
```

### Line

```cpp
void drawLine(Vec2 from, Vec2 to, const Color& color, float thickness = 1.0f);
```

**Example:**

```cpp
// Thin line
sys.graphics->drawLine(
    Vec2{0, 0},      // Start
    Vec2{100, 100},  // End
    Color::blue(),
    1.0f             // Thickness in pixels
);

// Thick line
sys.graphics->drawLine(
    Vec2{0, 100}, Vec2{100, 0}, Color::red(), 5.0f
);
```

### Polygon

```cpp
void drawPolygon(std::span<const Vec2> vertices,
                const Color& color,
                bool filled = true);
```

**Example:**

```cpp
// Triangle
std::vector<Vec2> triangle = {
    {100, 100}, {150, 50}, {200, 100}
};
sys.graphics->drawPolygon(triangle, Color::yellow(), true);

// Pentagon
std::vector<Vec2> pentagon;
for (int i = 0; i < 5; ++i) {
    float angle = (i * 2.0f * 3.14159f) / 5.0f;
    pentagon.push_back({
        200 + std::cos(angle) * 50,
        200 + std::sin(angle) * 50
    });
}
sys.graphics->drawPolygon(pentagon, Color::cyan(), false);
```

---

## Text Rendering

Bestow uses **MSDF (Multi-channel Signed Distance Field)** fonts for crisp text at any scale.

### Draw Text

```cpp
void drawText(const std::string& text, Vec2 position,
             AssetHandle fontHandle, float size,
             const Color& color = Color::white());
```

**Example:**

```cpp
// Load font through AssetSystem
AssetHandle font = sys.assets->registerAsset(
    AssetType::Font, "fonts/PressStart2P.ttf"
);
sys.assets->loadAsset(font);

// Draw text at position (top-left)
sys.graphics->drawText(
    "Score: 100",
    Vec2{10, 10},    // Top-left corner
    font,
    24.0f,           // Size in pixels
    Color::white()
);
```

### Draw Centered Text

```cpp
void drawTextCentered(const std::string& text, Vec2 position,
                     AssetHandle fontHandle, float size,
                     const Color& color = Color::white());
```

**Example:**

```cpp
// Center text at a point
Size windowSize = sys.graphics->getWindowSize();
sys.graphics->drawTextCentered(
    "GAME OVER",
    Vec2{windowSize.width / 2.0f, windowSize.height / 2.0f},  // Screen center
    font,
    48.0f,
    Color::red()
);
```

**Visual:**
```
drawText (top-left):      drawTextCentered (center):
┌──────────────            ──────────────
│HELLO                          HELLO
│                                  ↑
↑ position                    position
```

### Measure Text

```cpp
Vec2 measureText(const std::string& text, AssetHandle fontHandle,
                float size) const;
```

**Example:**

```cpp
// Measure text bounds for UI layout
Vec2 textSize = sys.graphics->measureText("Hello World", font, 24.0f);
// textSize.x = width in pixels
// textSize.y = height in pixels

// Center text manually
float x = (windowSize.width - textSize.x) / 2.0f;
float y = (windowSize.height - textSize.y) / 2.0f;
sys.graphics->drawText("Hello World", Vec2{x, y}, font, 24.0f);
```

---

## Camera System

The camera controls what portion of the game world is visible on screen.

### Set Camera

```cpp
void setCamera(const Camera& camera);
Camera getCamera() const;
```

**Example:**

```cpp
// Create camera
Camera camera{
    .transform = Transform2D{.x = 0, .y = 0},  // Camera center position
    .zoom = 1.0f,                               // 1.0 = normal, 2.0 = 2x zoom in
    .viewportSize = sys.graphics->getWindowSize()
};

sys.graphics->setCamera(camera);

// Update camera to follow player
void updateCamera(float dt) {
    auto playerPos = sys.entities->get<Transform2D>(player_);

    Camera cam = sys.graphics->getCamera();
    cam.transform.x = playerPos.x;
    cam.transform.y = playerPos.y;
    sys.graphics->setCamera(cam);
}
```

**Visual:**
```
Zoom levels:
zoom=0.5 (zoomed out):   zoom=1.0 (normal):    zoom=2.0 (zoomed in):
┌──────────────────┐     ┌──────────┐          ┌─────┐
│  ╭────────────╮  │     │ ╭──────╮ │          │ ╭─╮ │
│  │            │  │     │ │      │ │          │ │█│ │
│  │     ██     │  │     │ │  ██  │ │          │ ╰─╯ │
│  │            │  │     │ │      │ │          │     │
│  ╰────────────╯  │     │ ╰──────╯ │          └─────┘
└──────────────────┘     └──────────┘
```

### Coordinate Conversion

```cpp
Vec2 worldToScreen(Vec2 worldPos) const;
Vec2 screenToWorld(Vec2 screenPos) const;
```

**Example:**

```cpp
// Convert mouse position to world coordinates
Vec2 mouseScreenPos = sys.input->getMousePosition();
Vec2 worldPos = sys.graphics->screenToWorld(mouseScreenPos);

// Check if mouse clicked on an entity
if (sys.input->isMouseButtonJustPressed(MouseButton::Left)) {
    // worldPos now contains the click position in game world coordinates
    checkEntityAtPosition(worldPos);
}

// Convert world position to screen coordinates (for UI above entities)
Vec2 entityWorldPos = sys.entities->get<Transform2D>(enemy).position();
Vec2 screenPos = sys.graphics->worldToScreen(entityWorldPos);
drawHealthBarAt(screenPos);  // Draw UI at screen position
```

**Visual:**
```
Screen space:         World space:
┌──────────────┐
│ 0,0          │      Camera (400,300)
│    cursor    │         ↓
│    @800,450  │      ┌──────────────┐
│              │      │  world pos   │
└──────────────┘      │  @1200,750   │
                      │              │
                      └──────────────┘
```

---

## Entity Rendering

Automatically render all entities with visual components.

### Render All Entities

```cpp
void renderEntities(IEntitySystem& entities);
```

**Example:**

```cpp
void render(float alpha) {
    sys.graphics->beginFrame();

    // Automatically draws all entities with visual components
    sys.graphics->renderEntities(*sys.entities);

    sys.graphics->endFrame();
}
```

**How it works:**

The system automatically finds and renders entities with:
- `Sprite` + `Transform2D` → Textured sprites
- `AnimatedSprite` + `Transform2D` → Animated sprites
- `DebugRect` + `Transform2D` → Debug rectangles
- `DebugCircle` + `Transform2D` → Debug circles
- `DebugLine` + `Transform2D` → Debug lines

Entities are sorted by `RenderLayer` (lower values drawn first).

### Render Layer Range

```cpp
void renderEntities(IEntitySystem& entities,
                   RenderLayer minLayer, RenderLayer maxLayer);
```

**Example:**

```cpp
// Render background layers
sys.graphics->renderEntities(
    *sys.entities,
    RenderLayers::Background,   // Min layer (-100)
    RenderLayers::Platforms     // Max layer (0)
);

// Render gameplay layers
sys.graphics->renderEntities(
    *sys.entities,
    RenderLayers::Items,        // Min layer (10)
    RenderLayers::Effects       // Max layer (40)
);

// Render UI on top
sys.graphics->renderEntities(
    *sys.entities,
    RenderLayers::UI,           // Min layer (100)
    RenderLayers::Debug         // Max layer (1000)
);
```

**Visual:**
```
Layer order (bottom to top):
┌──────────────────────┐
│ Debug (1000)         │ ← Drawn last (on top)
│ UI (100)             │
│ Foreground (50)      │
│ Effects (40)         │
│ Player (30)          │
│ Enemies (20)         │
│ Items (10)           │
│ Platforms (0)        │
│ BackgroundDecor (-50)│
│ Background (-100)    │ ← Drawn first (on bottom)
└──────────────────────┘
```

### Viewport Culling

```cpp
void setViewportCulling(bool enabled);
bool isViewportCullingEnabled() const;
```

**Example:**

```cpp
// Enable culling to only render visible entities
sys.graphics->setViewportCulling(true);

// Entities outside the camera viewport are skipped
sys.graphics->renderEntities(*sys.entities);
```

**Visual:**
```
Without culling:         With culling:
(all entities drawn)     (only visible drawn)

   ┌──────────┐            ┌──────────┐
   │ Camera   │            │ Camera   │
 ██│  ██  ██  │██        ██│  ██  ██  │██
   │  ██  ██  │            │  ██  ██  │
 ██│  ██  ██  │██        ██│  ██  ██  │██
   └──────────┘            └──────────┘
   ↑ Wasted draws          ↑ Efficient!
```

---

## Window Management

### Window Size

```cpp
Size getWindowSize() const;
void setWindowSize(Size size);
```

**Example:**

```cpp
Size windowSize = sys.graphics->getWindowSize();
std::cout << "Window: " << windowSize.width << "x" << windowSize.height << std::endl;

// Resize window
sys.graphics->setWindowSize(Size{1920, 1080});
```

### Fullscreen

```cpp
bool isFullscreen() const;
void setFullscreen(bool fullscreen);
```

**Example:**

```cpp
// Toggle fullscreen
if (sys.input->isKeyJustPressed(Key::F11)) {
    bool currentFullscreen = sys.graphics->isFullscreen();
    sys.graphics->setFullscreen(!currentFullscreen);
}
```

### Should Close

```cpp
bool shouldClose() const;
```

**Example:**

```cpp
// Custom game loop
while (!sys.graphics->shouldClose()) {
    updateFixed(dt);
    render(alpha);
}
```

### Native Window Handle

```cpp
void* getNativeWindowHandle() const;
```

**Example:**

```cpp
// For integrating with native APIs or third-party libraries
void* nativeHandle = sys.graphics->getNativeWindowHandle();
// Cast to platform-specific type (GLFWwindow*, HWND, etc.)
```

---

## Render State

### Clear Color

```cpp
void setClearColor(const Color& color);
```

**Example:**

```cpp
// Set sky blue background
sys.graphics->setClearColor(Color::fromFloat(0.53f, 0.81f, 0.92f));

// Black background
sys.graphics->setClearColor(Color::black());
```

### VSync

```cpp
void setVSync(bool enabled);
```

**Example:**

```cpp
// Enable VSync (cap framerate to monitor refresh rate)
sys.graphics->setVSync(true);

// Disable VSync (uncapped framerate)
sys.graphics->setVSync(false);
```

**Visual:**
```
VSync ON:                VSync OFF:
60 FPS (smooth)          300 FPS (screen tearing)
┌─┬─┬─┬─┬─┬─┐           ┌┬┬┬┬┬┬┬┬┬┬┬┬┐
└─┴─┴─┴─┴─┴─┘           └┴┴┴┴┴┴┴┴┴┴┴┴┘
```

---

## Best Practices

### 1. Layer Organization

Use `RenderLayer` constants for predictable draw order:

```cpp
// Define in your game
namespace MyLayers {
    inline constexpr RenderLayer Sky = RenderLayers::Background;
    inline constexpr RenderLayer Ground = RenderLayers::Platforms;
    inline constexpr RenderLayer Collectibles = RenderLayers::Items;
    inline constexpr RenderLayer Characters = RenderLayers::Player;
    inline constexpr RenderLayer Particles = RenderLayers::Effects;
    inline constexpr RenderLayer HUD = RenderLayers::UI;
}

// Use in components
entity.emplace<Sprite>(Sprite{
    .layer = MyLayers::Characters
});
```

### 2. Batch Rendering

Collect sprites and draw in batches:

```cpp
// BAD - Many draw calls
for (const auto& coin : coins) {
    sys.graphics->draw(coin.sprite);  // 100 draw calls
}

// GOOD - Single draw call
std::vector<Sprite> coinSprites;
for (const auto& coin : coins) {
    coinSprites.push_back(coin.sprite);
}
sys.graphics->drawBatch(coinSprites);  // 1 draw call
```

### 3. Asset Loading

Always use AssetSystem:

```cpp
// CORRECT - Through AssetSystem
AssetHandle texture = sys.assets->registerAsset(
    AssetType::Texture, "textures/player.png"
);
sys.assets->loadAsset(texture);

// WRONG - Direct file I/O (DON'T DO THIS)
// std::ifstream file("textures/player.png");
```

### 4. Enable Viewport Culling

For large worlds with many entities:

```cpp
void initialize() {
    // Enable culling to skip off-screen entities
    sys.graphics->setViewportCulling(true);
}

void render(float alpha) {
    // Only visible entities are drawn
    sys.graphics->renderEntities(*sys.entities);
}
```

### 5. Camera Smoothing

Smooth camera movement for better feel:

```cpp
void updateCamera(float dt) {
    auto& playerPos = sys.entities->get<Transform2D>(player_);

    Camera cam = sys.graphics->getCamera();

    // Lerp camera toward player
    float lerpFactor = 5.0f * dt;
    cam.transform.x += (playerPos.x - cam.transform.x) * lerpFactor;
    cam.transform.y += (playerPos.y - cam.transform.y) * lerpFactor;

    sys.graphics->setCamera(cam);
}
```

### 6. Debug Primitives in Debug Builds

```cpp
#ifdef BESTOW_DEBUG
    // Draw physics debug shapes
    for (const auto& body : physicsBodies) {
        sys.graphics->drawRect(body.bounds, Color::green(), false);
    }

    // Draw AI paths
    for (size_t i = 0; i < path.size() - 1; ++i) {
        sys.graphics->drawLine(path[i], path[i+1], Color::yellow(), 2.0f);
    }
#endif
```

---

## Complete Examples

### Example 1: Platformer Renderer

```cpp
class PlatformerRenderer {
public:
    PlatformerRenderer(bestow::core::Engine& engine)
        : engine_(&engine)
    {
        auto& sys = engine.systems();

        // Load assets
        playerTexture_ = sys.assets->registerAsset(
            AssetType::Texture, "textures/player.png"
        );
        sys.assets->loadAsset(playerTexture_);

        font_ = sys.assets->registerAsset(
            AssetType::Font, "fonts/PressStart2P.ttf"
        );
        sys.assets->loadAsset(font_);

        // Setup camera
        Camera camera{
            .transform = {.x = 0, .y = 0},
            .zoom = 1.0f,
            .viewportSize = sys.graphics->getWindowSize()
        };
        sys.graphics->setCamera(camera);

        // Enable culling for performance
        sys.graphics->setViewportCulling(true);

        // Set clear color
        sys.graphics->setClearColor(
            Color::fromFloat(0.53f, 0.81f, 0.92f)  // Sky blue
        );
    }

    void render(float alpha) {
        auto& sys = engine_->systems();

        sys.graphics->beginFrame();

        // Update camera to follow player
        updateCamera();

        // Render world layers
        renderBackground();
        sys.graphics->renderEntities(*sys.entities);

        // Render UI on top
        renderUI();

        sys.graphics->endFrame();
    }

private:
    void updateCamera() {
        auto& sys = engine_->systems();

        // Find player
        auto view = sys.entities->view<Transform2D, PlayerTag>();
        for (auto entity : view) {
            const auto& transform = view.get<Transform2D>(entity);

            Camera cam = sys.graphics->getCamera();
            cam.transform.x = transform.x;
            cam.transform.y = transform.y;
            sys.graphics->setCamera(cam);
            break;
        }
    }

    void renderBackground() {
        auto& sys = engine_->systems();

        // Simple gradient sky
        Size windowSize = sys.graphics->getWindowSize();
        Canvas skyRect{{0, 0}, windowSize};
        sys.graphics->drawRect(skyRect,
            Color::fromFloat(0.3f, 0.5f, 0.7f), true);
    }

    void renderUI() {
        auto& sys = engine_->systems();

        // Draw score
        sys.graphics->drawText(
            std::format("Score: {}", score_),
            Vec2{10, 10},
            font_,
            16.0f,
            Color::white()
        );

        // Draw health hearts
        for (int i = 0; i < health_; ++i) {
            Canvas heart{{10 + i * 20, 40}, {16, 16}};
            sys.graphics->drawRect(heart, Color::red(), true);
        }
    }

    bestow::core::Engine* engine_;
    AssetHandle playerTexture_;
    AssetHandle font_;
    int score_ = 0;
    int health_ = 3;
};
```

### Example 2: Sprite Batching System

```cpp
class ParticleSystem {
public:
    void update(float dt) {
        // Update all particles
        for (auto& particle : particles_) {
            particle.x += particle.vx * dt;
            particle.y += particle.vy * dt;
            particle.lifetime -= dt;
        }

        // Remove dead particles
        std::erase_if(particles_, [](const Particle& p) {
            return p.lifetime <= 0.0f;
        });
    }

    void render(IGraphicsSystem* graphics) {
        // Batch all particle sprites
        std::vector<Sprite> sprites;
        sprites.reserve(particles_.size());

        for (const auto& particle : particles_) {
            sprites.push_back(Sprite{
                .textureHandle = &particleTexture_,
                .sourceRect = Canvas{{0, 0}, {8, 8}},
                .transform = Transform2D{
                    .x = particle.x,
                    .y = particle.y,
                    .scaleX = particle.scale,
                    .scaleY = particle.scale
                },
                .tint = particle.color,
                .layer = RenderLayers::Effects
            });
        }

        // Single draw call for all particles
        graphics->drawBatch(sprites);
    }

    void spawn(Vec2 position, Vec2 velocity, Color color) {
        particles_.push_back(Particle{
            .x = position.x,
            .y = position.y,
            .vx = velocity.x,
            .vy = velocity.y,
            .color = color,
            .scale = 1.0f,
            .lifetime = 2.0f
        });
    }

private:
    struct Particle {
        float x, y;
        float vx, vy;
        Color color;
        float scale;
        float lifetime;
    };

    std::vector<Particle> particles_;
    AssetHandle particleTexture_;
};
```

### Example 3: Debug Overlay

```cpp
class DebugOverlay {
public:
    void render(IGraphicsSystem* graphics, IEntitySystem* entities) {
        #ifdef BESTOW_DEBUG
        if (!enabled_) return;

        // Draw entity bounds
        auto view = entities->view<Transform2D, Sprite>();
        for (auto entity : view) {
            const auto& transform = view.get<Transform2D>(entity);
            const auto& sprite = view.get<Sprite>(entity);

            Canvas bounds{
                {static_cast<int>(transform.x), static_cast<int>(transform.y)},
                sprite.sourceRect.size
            };
            graphics->drawRect(bounds, Color::green(), false);
        }

        // Draw camera bounds
        Camera cam = graphics->getCamera();
        Size viewport = cam.viewportSize;
        Canvas cameraBounds{
            {static_cast<int>(cam.transform.x - viewport.width / 2),
             static_cast<int>(cam.transform.y - viewport.height / 2)},
            viewport
        };
        graphics->drawRect(cameraBounds, Color::cyan(), false);

        // Draw FPS
        graphics->drawText(
            std::format("FPS: {:.1f}", fps_),
            Vec2{10, graphics->getWindowSize().height - 30},
            debugFont_,
            12.0f,
            Color::yellow()
        );
        #endif
    }

    void toggle() { enabled_ = !enabled_; }
    void setFPS(float fps) { fps_ = fps; }

private:
    bool enabled_ = true;
    float fps_ = 60.0f;
    AssetHandle debugFont_;
};
```

---

## Summary

The **IGraphicsSystem** provides everything you need for 2D rendering:

| Feature | Methods | Use Case |
|---------|---------|----------|
| **Frame lifecycle** | `beginFrame()`, `endFrame()` | Required for every frame |
| **Sprite rendering** | `draw()`, `drawBatch()`, `drawSprite()`, `drawAnimatedSprite()` | Textured graphics |
| **Primitives** | `drawRect()`, `drawCircle()`, `drawLine()`, `drawPolygon()` | Debug visualization |
| **Text** | `drawText()`, `drawTextCentered()`, `measureText()` | UI and HUD |
| **Camera** | `setCamera()`, `worldToScreen()`, `screenToWorld()` | Viewport control |
| **Entity rendering** | `renderEntities()`, `setViewportCulling()` | Automatic ECS rendering |
| **Window** | `getWindowSize()`, `setFullscreen()`, `shouldClose()` | Window management |
| **Render state** | `setClearColor()`, `setVSync()` | Graphics configuration |

**Key Principles:**
- Always wrap rendering in `beginFrame()` / `endFrame()`
- Use batch rendering for multiple sprites
- Enable viewport culling for large worlds
- Use RenderLayers for proper draw order (lower = back, higher = front)
- Load all assets through AssetSystem
- Use primitives for debug visualization only

For more information, see:
- `/bestow-contract/src/bestow.graphics.cppm` - Interface definition
- `/bestow-contract/src/bestow.types.cppm` - Type definitions (Sprite, Camera, Color, etc.)
- [Asset System Guide](ASSET-SYSTEM.md) - Asset loading and hot reload
- [Entity System Guide](ENTITY-SYSTEM.md) - ECS architecture

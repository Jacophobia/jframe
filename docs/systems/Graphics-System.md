# JFrame Graphics System

## Overview

The Graphics System is JFrame's core rendering subsystem, providing window management, sprite rendering, text display, debug drawing, and camera controls. Built on OpenGL 4.1 Core with GLFW for windowing, it offers a modern 2D rendering pipeline optimized for game development.

### Key Features

- **Window Management** - Create and configure windowed or fullscreen game windows
- **Sprite Rendering** - Efficient batched sprite rendering with transforms and tinting
- **Sprite Sheets & Animation** - Built-in support for sprite sheets and frame-based animations
- **Text Rendering** - TrueType font rendering via stb_truetype with automatic atlas generation
- **Debug Drawing** - Immediate-mode primitives (lines, rectangles, circles, polygons)
- **Camera System** - 2D camera with zoom, position, and coordinate conversion
- **Render Layers** - Automatic depth sorting for sprite layering
- **Asset Integration** - Seamless texture and font loading via the Asset System

### Architecture

The Graphics System follows the JFrame interface pattern:

```
IGraphicsSystem (interface) <- GraphicsSystem (implementation)
```

The implementation uses:
- **OpenGL 4.1 Core Profile** for rendering
- **GLFW 3.3+** for window management and input
- **GLM** for matrix math
- **stb_truetype** for font rasterization
- **Sprite batching** for efficient draw call reduction

---

## Window Management

### Creating a Window

The Graphics System is typically initialized through the EngineBuilder:

```cpp
import jframe;
import jframe.core;

auto engineResult = jframe::core::EngineBuilder()
    .withGraphics(jframe::core::GraphicsConfig{
        .width = 1280,
        .height = 720,
        .title = "My Game",
        .vsync = true,
        .clearColor = {0, 0, 0, 255}  // Black background
    })
    .build();
```

### Direct Initialization (Advanced)

For standalone use without the engine:

```cpp
import jframe.graphics;
import jframe.graphics.impl;

auto graphics = jframe::createGraphicsSystem();
if (!graphics->initialize(1280, 720, "My Game")) {
    // Handle initialization failure
}

// Game loop
while (!graphics->shouldClose()) {
    graphics->beginFrame();

    // Draw your game here

    graphics->endFrame();
}
```

### Window Properties

#### Get/Set Window Size

```cpp
// Get current window size
Size size = graphics->getWindowSize();
std::println("Window: {}x{}", size.width, size.height);

// Change window size
graphics->setWindowSize(Size{1920, 1080});
```

#### Fullscreen Toggle

```cpp
// Check fullscreen state
bool isFullscreen = graphics->isFullscreen();

// Toggle fullscreen
graphics->setFullscreen(true);   // Enter fullscreen
graphics->setFullscreen(false);  // Exit fullscreen
```

#### VSync Control

```cpp
// Enable VSync (limit to monitor refresh rate)
graphics->setVSync(true);

// Disable VSync (uncapped framerate)
graphics->setVSync(false);
```

#### Clear Color

```cpp
// Set background color (RGB + Alpha, 0-255)
graphics->setClearColor(Color{135, 206, 235, 255});  // Sky blue

// Use predefined colors
graphics->setClearColor(Color::black());
graphics->setClearColor(Color::white());
graphics->setClearColor(Color{50, 50, 50, 255});  // Dark gray
```

#### Window Close Detection

```cpp
while (!graphics->shouldClose()) {
    // Game loop - runs until user closes window (ESC, X button, etc.)
}
```

#### Native Window Handle

```cpp
// Get GLFW window pointer for advanced use cases
void* windowHandle = graphics->getNativeWindowHandle();
GLFWwindow* glfwWindow = static_cast<GLFWwindow*>(windowHandle);
```

---

## Frame Lifecycle

Every frame must be wrapped in `beginFrame()` and `endFrame()` calls:

```cpp
while (!graphics->shouldClose()) {
    graphics->beginFrame();  // Clears screen, resets batch

    // All rendering calls go here
    graphics->draw(sprite);
    graphics->drawRect(rect, Color::red());
    graphics->drawText("Score: 100", Vec2{10, 10}, fontHandle, 24.0f);

    graphics->endFrame();    // Sorts sprites, renders batch, swaps buffers
}
```

### What Happens During beginFrame()

1. Clears the screen with the configured clear color
2. Resets the internal sprite batch
3. Prepares OpenGL state for rendering

### What Happens During endFrame()

1. Sorts all batched sprites by render layer (low to high)
2. Calculates camera view-projection matrix
3. Renders all sprites in a single optimized batch
4. Flushes debug primitives and text
5. Swaps front/back buffers (presents to screen)
6. Polls window events (input, resize, close)

---

## Sprite Rendering

### Basic Sprite

The `Sprite` struct is the fundamental rendering primitive:

```cpp
import jframe.types;

Sprite sprite;
sprite.textureHandle = &myTextureAsset;  // AssetHandle from Asset System
sprite.transform = Transform2D{
    .x = 400.0f,
    .y = 300.0f,
    .rotation = 0.0f,      // Radians
    .scaleX = 1.0f,
    .scaleY = 1.0f
};
sprite.sourceRect = Canvas{
    .origin = {0, 0},
    .size = {64, 64}
};
sprite.tint = Color::white();  // No color modification
sprite.layer = 0;               // Render layer (lower = behind)
sprite.anchor = {0.5f, 0.5f};   // Center anchor point

graphics->draw(sprite);
```

### Sprite Transform Properties

#### Position

```cpp
sprite.transform.x = 100.0f;  // World X coordinate
sprite.transform.y = 200.0f;  // World Y coordinate
```

#### Rotation

```cpp
#include <numbers>

// Rotate 45 degrees
sprite.transform.rotation = std::numbers::pi_v<float> / 4.0f;

// Rotate 90 degrees
sprite.transform.rotation = std::numbers::pi_v<float> / 2.0f;

// Full rotation (360 degrees)
sprite.transform.rotation = 2.0f * std::numbers::pi_v<float>;
```

#### Scale

```cpp
// Uniform scale (2x larger)
sprite.transform.scaleX = 2.0f;
sprite.transform.scaleY = 2.0f;

// Non-uniform scale (wide and flat)
sprite.transform.scaleX = 3.0f;
sprite.transform.scaleY = 0.5f;

// Flip horizontally
sprite.transform.scaleX = -1.0f;

// Flip vertically
sprite.transform.scaleY = -1.0f;
```

### Source Rectangle

The `sourceRect` defines which part of the texture to render:

```cpp
// Render entire 64x64 texture
sprite.sourceRect = Canvas{
    .origin = {0, 0},
    .size = {64, 64}
};

// Render a 32x32 region starting at (16, 16)
sprite.sourceRect = Canvas{
    .origin = {16, 16},
    .size = {32, 32}
};
```

### Anchor Point

The anchor determines the sprite's origin for position and rotation:

```cpp
// Center (default) - position is center of sprite
sprite.anchor = {0.5f, 0.5f};

// Top-left corner
sprite.anchor = {0.0f, 0.0f};

// Bottom-center (useful for characters)
sprite.anchor = {0.5f, 1.0f};

// Top-right corner
sprite.anchor = {1.0f, 0.0f};
```

### Color Tinting

Apply color multiplication to sprites:

```cpp
// No tint (original colors)
sprite.tint = Color::white();

// Red tint
sprite.tint = Color::red();

// Custom color
sprite.tint = Color{255, 128, 0, 255};  // Orange

// Semi-transparent (50% alpha)
sprite.tint = Color{255, 255, 255, 128};

// Invisible
sprite.tint = Color::transparent();
```

### Drawing Without Texture (Colored Rectangles)

If no texture is provided, sprites render as colored rectangles:

```cpp
Sprite coloredRect;
coloredRect.textureHandle = nullptr;  // No texture
coloredRect.transform = Transform2D{.x = 100.0f, .y = 100.0f};
coloredRect.sourceRect.size = {64, 64};
coloredRect.tint = Color::blue();

graphics->draw(coloredRect);  // Draws a blue 64x64 square
```

### Batch Drawing

For better performance when drawing many sprites with the same properties:

```cpp
std::vector<Sprite> enemies;
for (int i = 0; i < 100; i++) {
    Sprite enemy;
    enemy.textureHandle = &enemyTexture;
    enemy.transform.x = static_cast<float>(i * 50);
    enemy.transform.y = 200.0f;
    enemy.sourceRect.size = {32, 32};
    enemies.push_back(enemy);
}

graphics->drawBatch(enemies);  // More efficient than 100 individual draw() calls
```

---

## Sprite Sheets & Animation

### Sprite Sheet Setup

A `SpriteSheet` divides a texture into a grid of frames:

```cpp
import jframe.types;

SpriteSheet sheet;
sheet.texture = characterTexture;  // AssetHandle
sheet.frameWidth = 32;             // Width of each frame
sheet.frameHeight = 32;            // Height of each frame
sheet.columns = 8;                 // 8 frames per row
sheet.rows = 4;                    // 4 rows
sheet.padding = 0;                 // No padding between frames
```

### Drawing Sprite Sheet Frames

```cpp
// Draw frame 5 from the sheet
graphics->drawSprite(sheet, 5, transform, Color::white());

// Draw frame 12 at a specific position
Transform2D position{.x = 200.0f, .y = 150.0f};
graphics->drawSprite(sheet, 12, position);
```

### Animated Sprites

Create frame-based animations:

```cpp
AnimatedSprite character;
character.sheet = characterSheet;  // SpriteSheet configured above

// Define "run" animation
Animation runAnim;
runAnim.name = "run";
runAnim.looping = true;
runAnim.frames = {
    AnimationFrame{.frameIndex = 0, .duration = 0.1f},
    AnimationFrame{.frameIndex = 1, .duration = 0.1f},
    AnimationFrame{.frameIndex = 2, .duration = 0.1f},
    AnimationFrame{.frameIndex = 3, .duration = 0.1f}
};
character.animations["run"] = runAnim;

// Define "jump" animation
Animation jumpAnim;
jumpAnim.name = "jump";
jumpAnim.looping = false;  // Play once
jumpAnim.frames = {
    AnimationFrame{.frameIndex = 8, .duration = 0.2f},
    AnimationFrame{.frameIndex = 9, .duration = 0.3f}
};
character.animations["jump"] = jumpAnim;
```

### Playing Animations

```cpp
// In game initialization
character.play("run");

// In game update loop
void update(DeltaTime dt) {
    character.update(dt);  // Advances animation frame
}

// In render loop
void render() {
    Transform2D transform{.x = playerX, .y = playerY};
    graphics->drawAnimatedSprite(character, transform, Color::white());
}

// Switch animations
if (isJumping) {
    character.play("jump");
}
```

### Animation Control

```cpp
// Pause animation
character.playing = false;

// Resume animation
character.playing = true;

// Check current animation
if (character.currentAnimation == "run") {
    // Character is running
}

// Get current frame index
int currentFrame = character.getCurrentFrame();
```

---

## Text Rendering

### Font Loading

Fonts are loaded through the Asset System:

```cpp
import jframe.assets;

// Register and load a TrueType font
AssetHandle fontHandle = assets->registerAsset(AssetType::Font, "fonts/Roboto-Regular.ttf");
assets->loadAssetSync(fontHandle);

// Set the font in Graphics System
graphics->setAssetSystem(assets);
```

### Drawing Text

```cpp
// Basic text rendering
graphics->drawText(
    "Hello, World!",           // Text to render
    Vec2{50.0f, 50.0f},        // Position (top-left of text)
    fontHandle,                // Font asset handle
    24.0f,                     // Font size in pixels
    Color::white()             // Text color
);

// Multi-line text (manual)
float y = 100.0f;
graphics->drawText("Line 1", Vec2{50.0f, y}, fontHandle, 16.0f);
y += 20.0f;
graphics->drawText("Line 2", Vec2{50.0f, y}, fontHandle, 16.0f);
y += 20.0f;
graphics->drawText("Line 3", Vec2{50.0f, y}, fontHandle, 16.0f);
```

### Centered Text

```cpp
// Draw text centered at a position
Vec2 screenCenter{400.0f, 300.0f};
graphics->drawTextCentered(
    "Game Over",
    screenCenter,
    fontHandle,
    48.0f,
    Color::red()
);

// Centered menu options
float y = 200.0f;
graphics->drawTextCentered("New Game", Vec2{400.0f, y}, fontHandle, 24.0f);
y += 40.0f;
graphics->drawTextCentered("Load Game", Vec2{400.0f, y}, fontHandle, 24.0f);
y += 40.0f;
graphics->drawTextCentered("Quit", Vec2{400.0f, y}, fontHandle, 24.0f);
```

### Measuring Text

Get text dimensions for layout calculations:

```cpp
Vec2 textSize = graphics->measureText("Player Name", fontHandle, 20.0f);
std::println("Text width: {}, height: {}", textSize.x, textSize.y);

// Right-align text
float rightEdge = 780.0f;
Vec2 size = graphics->measureText("Score: 1000", fontHandle, 16.0f);
Vec2 position{rightEdge - size.x, 20.0f};
graphics->drawText("Score: 1000", position, fontHandle, 16.0f);
```

### Text Examples

#### HUD Display

```cpp
void renderHUD() {
    AssetHandle hudFont = /* ... */;

    // Top-left: Health
    graphics->drawText(
        std::format("Health: {}/100", playerHealth),
        Vec2{10.0f, 10.0f},
        hudFont,
        18.0f,
        Color::white()
    );

    // Top-right: Score
    Vec2 scoreSize = graphics->measureText(
        std::format("Score: {}", score),
        hudFont,
        18.0f
    );
    graphics->drawText(
        std::format("Score: {}", score),
        Vec2{780.0f - scoreSize.x, 10.0f},
        hudFont,
        18.0f,
        Color{255, 215, 0, 255}  // Gold
    );

    // Bottom-center: Instructions
    graphics->drawTextCentered(
        "Press SPACE to jump",
        Vec2{400.0f, 550.0f},
        hudFont,
        14.0f,
        Color{200, 200, 200, 255}  // Light gray
    );
}
```

#### Dynamic Text Coloring

```cpp
// Warning text that blinks red
float time = /* game time */;
bool blink = (static_cast<int>(time * 4.0f) % 2) == 0;
Color warnColor = blink ? Color::red() : Color::white();
graphics->drawText("WARNING!", Vec2{300.0f, 200.0f}, fontHandle, 32.0f, warnColor);

// Color-coded status
Color healthColor = playerHealth > 50 ? Color::green() : Color::red();
graphics->drawText(
    std::format("HP: {}", playerHealth),
    Vec2{10.0f, 10.0f},
    fontHandle,
    20.0f,
    healthColor
);
```

---

## Debug Drawing

Debug primitives are rendered immediately (not batched) and are perfect for visualization during development.

### Drawing Rectangles

```cpp
// Filled rectangle
Canvas rect{
    .origin = {100, 100},
    .size = {200, 150}
};
graphics->drawRect(rect, Color::red(), true);

// Rectangle outline
graphics->drawRect(rect, Color::blue(), false);

// Debug physics bounds
Canvas hitbox{
    .origin = {static_cast<int>(entity.x - 16), static_cast<int>(entity.y - 16)},
    .size = {32, 32}
};
graphics->drawRect(hitbox, Color::green(), false);
```

### Drawing Lines

```cpp
// Simple line
graphics->drawLine(
    Vec2{0.0f, 0.0f},      // Start point
    Vec2{100.0f, 100.0f},  // End point
    Color::green(),        // Color
    2.0f                   // Thickness (pixels)
);

// Velocity vector visualization
Vec2 entityPos{200.0f, 200.0f};
Vec2 velocity{50.0f, -30.0f};
graphics->drawLine(entityPos, entityPos + velocity, Color::yellow(), 3.0f);

// Path visualization
for (size_t i = 0; i < path.size() - 1; i++) {
    graphics->drawLine(path[i], path[i + 1], Color::cyan(), 2.0f);
}
```

### Drawing Circles

```cpp
// Filled circle
graphics->drawCircle(
    Vec2{200.0f, 200.0f},  // Center
    50.0f,                 // Radius
    Color::blue(),         // Color
    true,                  // Filled
    32                     // Segments (smoothness)
);

// Circle outline
graphics->drawCircle(Vec2{300.0f, 200.0f}, 50.0f, Color::red(), false, 32);

// Low-segment circle (hexagon)
graphics->drawCircle(Vec2{400.0f, 200.0f}, 50.0f, Color::green(), false, 6);

// Debug attack range
graphics->drawCircle(playerPos, attackRange, Color{255, 0, 0, 64}, true, 64);
```

### Drawing Polygons

```cpp
// Triangle
std::vector<Vec2> triangle = {
    Vec2{100.0f, 100.0f},
    Vec2{200.0f, 100.0f},
    Vec2{150.0f, 200.0f}
};
graphics->drawPolygon(triangle, Color::green(), true);

// Pentagon outline
std::vector<Vec2> pentagon;
for (int i = 0; i < 5; i++) {
    float angle = (2.0f * std::numbers::pi_v<float> * i) / 5.0f;
    pentagon.push_back(Vec2{
        300.0f + 50.0f * std::cos(angle),
        300.0f + 50.0f * std::sin(angle)
    });
}
graphics->drawPolygon(pentagon, Color::purple(), false);

// Convex hull visualization
graphics->drawPolygon(convexHull, Color{0, 255, 0, 128}, true);
```

### Debug Visualization Examples

#### Physics Collision Boxes

```cpp
void debugRenderPhysics() {
    // Draw all physics bodies
    for (auto [entity, body] : view<PhysicsBody>()) {
        Canvas bounds = physics->getEntityBounds(entity);
        graphics->drawRect(bounds, Color::green(), false);
    }

    // Highlight colliding entities
    for (auto [e1, e2] : collisionPairs) {
        Canvas b1 = physics->getEntityBounds(e1);
        Canvas b2 = physics->getEntityBounds(e2);
        graphics->drawRect(b1, Color::red(), false);
        graphics->drawRect(b2, Color::red(), false);
    }
}
```

#### AI Debug Visualization

```cpp
void debugRenderAI() {
    for (auto [entity, ai] : view<AIComponent>()) {
        Vec2 pos = getPosition(entity);

        // Vision cone
        std::vector<Vec2> visionCone = calculateVisionCone(pos, ai.facing, ai.visionRange);
        graphics->drawPolygon(visionCone, Color{255, 255, 0, 64}, true);

        // Path to target
        if (ai.hasTarget) {
            graphics->drawLine(pos, ai.targetPos, Color::cyan(), 2.0f);
            graphics->drawCircle(ai.targetPos, 5.0f, Color::red(), true);
        }

        // State indicator
        graphics->drawCircle(pos, 10.0f, getStateColor(ai.state), true);
    }
}
```

#### Grid Overlay

```cpp
void debugRenderGrid() {
    const float GRID_SIZE = 50.0f;
    Color gridColor{100, 100, 100, 255};

    Size windowSize = graphics->getWindowSize();
    Camera cam = graphics->getCamera();
    Vec2 camPos = cam.transform.position();

    // Calculate visible grid range
    float left = camPos.x - windowSize.width / 2;
    float right = camPos.x + windowSize.width / 2;
    float top = camPos.y - windowSize.height / 2;
    float bottom = camPos.y + windowSize.height / 2;

    // Vertical lines
    for (float x = std::floor(left / GRID_SIZE) * GRID_SIZE; x < right; x += GRID_SIZE) {
        graphics->drawLine(Vec2{x, top}, Vec2{x, bottom}, gridColor, 1.0f);
    }

    // Horizontal lines
    for (float y = std::floor(top / GRID_SIZE) * GRID_SIZE; y < bottom; y += GRID_SIZE) {
        graphics->drawLine(Vec2{left, y}, Vec2{right, y}, gridColor, 1.0f);
    }
}
```

---

## Camera System

The camera controls what portion of the world is visible and provides coordinate conversion utilities.

### Camera Setup

```cpp
Camera camera;
camera.transform = Transform2D{
    .x = 0.0f,      // Camera center X in world space
    .y = 0.0f,      // Camera center Y in world space
    .rotation = 0.0f
};
camera.zoom = 1.0f;  // 1.0 = normal, 2.0 = zoomed in 2x, 0.5 = zoomed out 2x
camera.viewportSize = Size{800, 600};  // Usually matches window size

graphics->setCamera(camera);
```

### Camera Movement

```cpp
// Follow player
void updateCamera(DeltaTime dt) {
    Vec2 playerPos = getPlayerPosition();
    Camera cam = graphics->getCamera();

    // Smooth camera follow
    float smoothing = 5.0f * dt;
    cam.transform.x += (playerPos.x - cam.transform.x) * smoothing;
    cam.transform.y += (playerPos.y - cam.transform.y) * smoothing;

    graphics->setCamera(cam);
}

// Keyboard camera control
void handleCameraInput(IInputSystem* input, DeltaTime dt) {
    Camera cam = graphics->getCamera();
    float speed = 200.0f * dt;

    if (input->isKeyPressed(GLFW_KEY_LEFT))  cam.transform.x -= speed;
    if (input->isKeyPressed(GLFW_KEY_RIGHT)) cam.transform.x += speed;
    if (input->isKeyPressed(GLFW_KEY_UP))    cam.transform.y -= speed;
    if (input->isKeyPressed(GLFW_KEY_DOWN))  cam.transform.y += speed;

    graphics->setCamera(cam);
}
```

### Camera Zoom

```cpp
// Zoom in (2x magnification)
Camera cam = graphics->getCamera();
cam.zoom = 2.0f;
graphics->setCamera(cam);

// Zoom out (show more area)
cam.zoom = 0.5f;
graphics->setCamera(cam);

// Smooth zoom with mouse wheel
void handleZoom(float wheelDelta, DeltaTime dt) {
    Camera cam = graphics->getCamera();
    cam.zoom *= (1.0f + wheelDelta * 0.1f);
    cam.zoom = std::clamp(cam.zoom, 0.25f, 4.0f);  // Limit zoom range
    graphics->setCamera(cam);
}
```

### Coordinate Conversion

#### World to Screen

Convert world coordinates (game objects) to screen coordinates (UI, mouse):

```cpp
// Where does this world position appear on screen?
Vec2 worldPos{100.0f, 200.0f};
Vec2 screenPos = graphics->worldToScreen(worldPos);
std::println("World {} appears at screen {}", worldPos, screenPos);

// Check if object is visible
Vec2 objectWorldPos = getObjectPosition();
Vec2 objectScreenPos = graphics->worldToScreen(objectWorldPos);
Size windowSize = graphics->getWindowSize();

if (objectScreenPos.x >= 0 && objectScreenPos.x <= windowSize.width &&
    objectScreenPos.y >= 0 && objectScreenPos.y <= windowSize.height) {
    // Object is visible on screen
}
```

#### Screen to World

Convert screen coordinates (mouse cursor) to world coordinates (game logic):

```cpp
// What world position is the mouse pointing at?
Vec2 mouseScreenPos = input->getMousePosition();
Vec2 mouseWorldPos = graphics->screenToWorld(mouseScreenPos);
std::println("Mouse at screen {} = world {}", mouseScreenPos, mouseWorldPos);

// Click to move
if (input->isMouseButtonPressed(0)) {
    Vec2 clickWorld = graphics->screenToWorld(input->getMousePosition());
    movePlayerTo(clickWorld);
}

// Hover detection
Vec2 cursorWorld = graphics->screenToWorld(input->getMousePosition());
for (auto [entity, pos, bounds] : view<Position, Bounds>()) {
    if (isPointInBounds(cursorWorld, pos, bounds)) {
        // Mouse is hovering over this entity
        showTooltip(entity);
    }
}
```

### Camera Bounds

Restrict camera movement to level boundaries:

```cpp
void constrainCamera(const Canvas& levelBounds) {
    Camera cam = graphics->getCamera();

    float halfWidth = cam.viewportSize.width / (2.0f * cam.zoom);
    float halfHeight = cam.viewportSize.height / (2.0f * cam.zoom);

    // Clamp camera position to keep view inside level
    cam.transform.x = std::clamp(
        cam.transform.x,
        levelBounds.origin.x + halfWidth,
        levelBounds.origin.x + levelBounds.size.width - halfWidth
    );

    cam.transform.y = std::clamp(
        cam.transform.y,
        levelBounds.origin.y + halfHeight,
        levelBounds.origin.y + levelBounds.size.height - halfHeight
    );

    graphics->setCamera(cam);
}
```

### Camera Shake

Add screen shake for impact effects:

```cpp
class CameraShake {
    float trauma_ = 0.0f;  // 0.0 to 1.0

public:
    void addTrauma(float amount) {
        trauma_ = std::min(trauma_ + amount, 1.0f);
    }

    void update(DeltaTime dt, IGraphicsSystem* graphics) {
        if (trauma_ <= 0.0f) return;

        trauma_ -= dt * 2.0f;  // Decay over time
        trauma_ = std::max(trauma_, 0.0f);

        // Shake magnitude based on trauma squared
        float shake = trauma_ * trauma_;

        Camera cam = graphics->getCamera();
        cam.transform.x += (rand() / float(RAND_MAX) - 0.5f) * shake * 20.0f;
        cam.transform.y += (rand() / float(RAND_MAX) - 0.5f) * shake * 20.0f;
        graphics->setCamera(cam);
    }
};

// Usage
CameraShake shake;
if (explosionOccurred) {
    shake.addTrauma(0.8f);  // Strong shake
}
```

---

## Render Layers

Sprites are automatically sorted and rendered by their `layer` value. Lower layers render first (behind), higher layers render last (in front).

### Layer Assignment

```cpp
// Background layer
Sprite background;
background.layer = -10;
background.textureHandle = &bgTexture;
background.transform = Transform2D{.x = 400.0f, .y = 300.0f};
graphics->draw(background);

// Ground layer
Sprite ground;
ground.layer = 0;
ground.textureHandle = &groundTexture;
graphics->draw(ground);

// Player layer
Sprite player;
player.layer = 5;
player.textureHandle = &playerTexture;
graphics->draw(player);

// Foreground layer
Sprite tree;
tree.layer = 10;
tree.textureHandle = &treeTexture;
graphics->draw(tree);

// UI overlay (always on top)
Sprite healthBar;
healthBar.layer = 100;
graphics->draw(healthBar);
```

### Layer Constants

Define layer constants for clarity:

```cpp
namespace RenderLayers {
    constexpr RenderLayer Background = -20;
    constexpr RenderLayer Ground = -10;
    constexpr RenderLayer GroundDecals = -5;
    constexpr RenderLayer Items = 0;
    constexpr RenderLayer Characters = 5;
    constexpr RenderLayer Effects = 10;
    constexpr RenderLayer Foreground = 15;
    constexpr RenderLayer UI = 100;
}

sprite.layer = RenderLayers::Characters;
```

### Dynamic Depth Sorting (Y-Sorting)

For top-down games, sort entities by Y position:

```cpp
void renderEntities() {
    std::vector<Entity> entities = getAllVisibleEntities();

    // Sort by Y position (entities further up appear behind)
    std::sort(entities.begin(), entities.end(), [](Entity a, Entity b) {
        return getPosition(a).y < getPosition(b).y;
    });

    // Draw with incrementing layers
    for (size_t i = 0; i < entities.size(); i++) {
        Sprite sprite = createSpriteForEntity(entities[i]);
        sprite.layer = static_cast<RenderLayer>(i);
        graphics->draw(sprite);
    }
}
```

---

## Color and Blending

### Color Definition

```cpp
// RGB + Alpha (0-255)
Color white = Color::white();     // {255, 255, 255, 255}
Color black = Color::black();     // {0, 0, 0, 255}
Color red = Color::red();         // {255, 0, 0, 255}
Color green = Color::green();     // {0, 255, 0, 255}
Color blue = Color::blue();       // {0, 0, 255, 255}
Color transparent = Color::transparent();  // {0, 0, 0, 0}

// Custom colors
Color orange{255, 128, 0, 255};
Color semiTransparent{255, 255, 255, 128};  // 50% opacity
Color invisible{0, 0, 0, 0};
```

### Alpha Blending

The Graphics System uses standard alpha blending:
- `GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA`
- Transparent objects blend with background
- Alpha values: 0 = fully transparent, 255 = fully opaque

### Fade Effects

```cpp
// Fade in over time
float fadeTime = 2.0f;  // seconds
float alpha = std::min(elapsedTime / fadeTime, 1.0f);
sprite.tint = Color{255, 255, 255, static_cast<uint8_t>(alpha * 255)};

// Fade out
float alpha = std::max(1.0f - (elapsedTime / fadeTime), 0.0f);
sprite.tint.a = static_cast<uint8_t>(alpha * 255);

// Blinking effect
bool visible = (static_cast<int>(time * 4.0f) % 2) == 0;
sprite.tint.a = visible ? 255 : 0;
```

---

## Complete Examples

### Minimal Rendering Loop

```cpp
import jframe.graphics;
import jframe.graphics.impl;
import jframe.assets;
import jframe.assets.impl;

int main() {
    // Initialize systems
    auto graphics = jframe::createGraphicsSystem();
    auto assets = jframe::createAssetSystem();

    graphics->initialize(800, 600, "My Game");
    graphics->setAssetSystem(assets);

    // Load texture
    AssetHandle texture = assets->registerAsset(AssetType::Texture, "player.png");
    assets->loadAssetSync(texture);

    // Create sprite
    Sprite player;
    player.textureHandle = &texture;
    player.transform = Transform2D{.x = 400.0f, .y = 300.0f};
    player.sourceRect.size = {32, 32};

    // Game loop
    while (!graphics->shouldClose()) {
        graphics->beginFrame();

        // Update position
        player.transform.x += 0.5f;

        // Render
        graphics->draw(player);

        graphics->endFrame();
    }

    return 0;
}
```

### Platformer Rendering

```cpp
void renderGame(IGraphicsSystem* graphics) {
    // Background (parallax layer 1)
    Sprite skyBg;
    skyBg.textureHandle = &skyTexture;
    skyBg.layer = -20;
    skyBg.transform = getCameraPosition() * 0.2f;  // Slow parallax
    graphics->draw(skyBg);

    // Background (parallax layer 2)
    Sprite mountainBg;
    mountainBg.textureHandle = &mountainTexture;
    mountainBg.layer = -10;
    mountainBg.transform = getCameraPosition() * 0.5f;  // Medium parallax
    graphics->draw(mountainBg);

    // Render level tiles
    for (auto& tile : visibleTiles) {
        Sprite tileSprite;
        tileSprite.textureHandle = &tilesetTexture;
        tileSprite.sourceRect = getTileSourceRect(tile.id);
        tileSprite.transform = Transform2D{
            .x = static_cast<float>(tile.x * 32),
            .y = static_cast<float>(tile.y * 32)
        };
        tileSprite.layer = 0;
        graphics->draw(tileSprite);
    }

    // Render collectibles
    for (auto& coin : coins) {
        coin.animation.update(deltaTime);
        graphics->drawAnimatedSprite(coin.animation, coin.transform, Color::white());
    }

    // Render enemies
    for (auto& enemy : enemies) {
        enemy.animation.update(deltaTime);
        graphics->drawAnimatedSprite(enemy.animation, enemy.transform, Color::white());
    }

    // Render player
    playerAnimation.update(deltaTime);
    graphics->drawAnimatedSprite(playerAnimation, playerTransform, Color::white());

    // Render UI
    AssetHandle hudFont;
    graphics->drawText(
        std::format("Score: {}", score),
        Vec2{10.0f, 10.0f},
        hudFont,
        20.0f,
        Color::white()
    );

    // Debug rendering (only in dev builds)
#if defined(JFRAME_DEV_TOOLS)
    for (auto& enemy : enemies) {
        Canvas hitbox = enemy.getHitbox();
        graphics->drawRect(hitbox, Color::red(), false);
    }
#endif
}
```

---

## Best Practices

### Performance

1. **Batch Sprites** - Use `drawBatch()` for multiple sprites with similar properties
2. **Minimize Texture Switches** - Group sprites by texture when possible
3. **Use Render Layers** - Let the system sort sprites instead of manual ordering
4. **Limit Debug Drawing** - Disable debug visualization in release builds
5. **Reuse Sprites** - Don't create new Sprite objects every frame

### Organization

1. **Define Layer Constants** - Use named constants for render layers
2. **Centralize Color Palette** - Define game colors as constants
3. **Cache AssetHandles** - Don't look up assets every frame
4. **Separate Render Logic** - Keep rendering code separate from game logic

### Coordinate Systems

1. **World Space** - Game logic operates in world coordinates
2. **Screen Space** - UI and mouse input use screen coordinates
3. **Camera Space** - Sprites render relative to camera position
4. **Use Conversion Functions** - `worldToScreen()` and `screenToWorld()` for mixing spaces

---

## Troubleshooting

### Sprites Not Appearing

- Check that `textureHandle` is valid and asset is loaded
- Verify `sourceRect` size is non-zero
- Ensure sprite is within camera view
- Check that `tint.a` is not 0 (transparent)
- Verify `layer` is reasonable (not extremely negative/positive)

### Text Not Rendering

- Ensure font asset is loaded: `assets->isLoaded(fontHandle)`
- Check that `graphics->setAssetSystem(assets)` was called
- Verify font file is valid TrueType (.ttf)
- Check text color alpha is not 0

### Performance Issues

- Reduce number of sprites per frame
- Use `drawBatch()` instead of individual `draw()` calls
- Disable debug drawing in release builds
- Reduce circle segment count for debug circles
- Optimize texture sizes (power-of-two dimensions)

### Window Issues

- Verify OpenGL 4.1 is supported (check GPU drivers)
- Check GLFW initialization succeeded
- Ensure window size is reasonable (not 0x0)
- Verify monitor supports requested fullscreen resolution

---

## See Also

- [Asset System Documentation](Asset-System.md) - Loading textures and fonts
- [Input System Documentation](Input-System.md) - Handling mouse and keyboard
- [Entity System Documentation](Entity-System.md) - Managing game objects
- [JFrame Technical Design](../jframe-technical-design.md) - Architecture overview

# Tutorial 02: Rendering Sprites

In this tutorial, you'll learn how to load textures, create sprites, and render them using Bestow's graphics system. You'll understand layers, draw order, camera setup, world coordinates, and animated sprites.

## What You'll Build

By the end of this tutorial, you'll have:
- A textured sprite rendered on screen
- Multiple sprites with proper layering
- A camera that follows the player
- Animated sprite sheets for character movement

## Prerequisites

Before starting, complete Tutorial 01 and ensure you have:
- A working Bestow project
- Basic understanding of the game loop
- Familiarity with the Entity system

## Understanding the Graphics Pipeline

Bestow's rendering system follows this flow:

1. **Load textures** via AssetSystem (file I/O)
2. **Create sprite data** (transform, texture reference, layer)
3. **Render each frame** with `IGraphicsSystem`
4. **Handle layers** for draw order
5. **Position with camera** for world-to-screen transformation

### Key Concepts

- **Texture**: Image file loaded into GPU memory
- **Sprite**: Visual representation with texture, position, size, tint
- **Layer**: Draw order (higher layers draw on top)
- **Camera**: Defines view region of the game world
- **World Coordinates**: Absolute positions in game space
- **Screen Coordinates**: Pixel positions on display

## Step 1: Loading Textures

All file loading must go through the AssetSystem. This provides caching, hot reload support, and proper lifecycle management.

### Register and Load a Texture

```cpp
// In Game.h
private:
    bestow::AssetHandle playerTexture_;
    bestow::Entity player_;
```

```cpp
// In Game.cpp - initialize()
auto& sys = engine_->systems();

// Register the texture asset (doesn't load yet)
playerTexture_ = sys.assets->registerAsset(
    bestow::AssetType::Texture,
    ":assets:/textures/player.png"
);

// Load the asset synchronously
sys.assets->loadAsset(playerTexture_);

// Or load asynchronously with callback
sys.assets->loadAssetAsync(playerTexture_, [this](bestow::AssetHandle h, bestow::AssetState state) {
    if (state == bestow::AssetState::Loaded) {
        bestow::core::logInfo("Player texture loaded!");
    } else if (state == bestow::AssetState::Failed) {
        bestow::core::logError("Failed to load player texture");
    }
});
```

### Asset Path Prefixes

Bestow supports special path prefixes:

- `:assets:/` - Your game's data directory (configured in EngineBuilder)
- `:library:/` - Bestow's shared asset library

```cpp
// Load from your game's data folder
auto playerTex = sys.assets->registerAsset(
    bestow::AssetType::Texture,
    ":assets:/textures/player.png"
);

// Load from Bestow's shared library
auto defaultTex = sys.assets->registerAsset(
    bestow::AssetType::Texture,
    ":library:/textures/missing.png"
);
```

## Step 2: Creating and Drawing Sprites

With textures loaded, create sprite structures and render them.

### Simple Sprite

```cpp
// In Game.cpp - render()
auto& sys = engine_->systems();

// Get player position from physics or transform
bestow::Vec2 pos = sys.physics->getPosition(player_);

// Create sprite structure
bestow::Sprite sprite{
    .textureHandle = &playerTexture_,
    .sourceRect = {0, 0, 32, 32},  // Source rect in texture (x, y, w, h)
    .transform = bestow::Transform2D{
        .x = pos.x,
        .y = pos.y,
        .rotation = 0.0f,
        .scaleX = 1.0f,
        .scaleY = 1.0f
    },
    .tint = bestow::Color::white(),  // Multiply color (white = no tint)
    .layer = 10  // Draw order (higher = on top)
};

// Draw the sprite
sys.graphics->drawSprite(sprite);
```

### Sprite Structure Explained

```cpp
struct Sprite {
    AssetHandle* textureHandle;  // Pointer to loaded texture
    Rect sourceRect;             // Which part of texture to use
    Transform2D transform;       // Position, rotation, scale in world space
    Color tint;                  // Color multiplier (RGBA 0-255)
    int layer;                   // Draw order
};
```

- **textureHandle**: Reference to loaded texture asset
- **sourceRect**: Region of texture to draw (useful for sprite sheets)
- **transform**: World position and orientation
- **tint**: Color modulation (use `Color::white()` for no tint)
- **layer**: Drawing order (background = low, UI = high)

## Step 3: Understanding Layers and Draw Order

Layers control which sprites appear on top of others. Bestow renders from lowest to highest layer.

### Layer Organization

```cpp
// Common layer values
constexpr int LAYER_BACKGROUND = 0;
constexpr int LAYER_ENVIRONMENT = 10;
constexpr int LAYER_GROUND = 20;
constexpr int LAYER_ENTITIES = 30;
constexpr int LAYER_PLAYER = 40;
constexpr int LAYER_EFFECTS = 50;
constexpr int LAYER_UI = 100;

// Background (furthest)
bestow::Sprite background{
    .textureHandle = &bgTexture_,
    .sourceRect = {0, 0, 1920, 1080},
    .transform = {0, 0},
    .tint = bestow::Color::white(),
    .layer = LAYER_BACKGROUND
};

// Player (middle)
bestow::Sprite player{
    .textureHandle = &playerTexture_,
    .sourceRect = {0, 0, 32, 32},
    .transform = {100, 200},
    .tint = bestow::Color::white(),
    .layer = LAYER_PLAYER
};

// UI (closest)
bestow::Sprite healthBar{
    .textureHandle = &uiTexture_,
    .sourceRect = {0, 0, 100, 10},
    .transform = {10, 10},
    .tint = bestow::Color::red(),
    .layer = LAYER_UI
};

// Draw in any order - layers determine final order
sys.graphics->drawSprite(player);
sys.graphics->drawSprite(background);
sys.graphics->drawSprite(healthBar);

// Renders as: background -> player -> healthBar
```

### Sorting Within Layers

Sprites within the same layer are drawn in submission order. For Y-sorting (top-down games):

```cpp
// Sort entities by Y position before rendering
std::vector<bestow::Entity> entities = getAllEntities();
std::sort(entities.begin(), entities.end(), [&sys](auto a, auto b) {
    float ay = sys.physics->getPosition(a).y;
    float by = sys.physics->getPosition(b).y;
    return ay < by;  // Lower Y draws first (appears behind)
});

for (auto entity : entities) {
    drawEntity(entity);
}
```

## Step 4: Camera Setup and World Coordinates

Cameras define the visible region of your game world and transform world coordinates to screen coordinates.

### Creating a Camera

```cpp
// In Game.h
private:
    bestow::Camera2D camera_;
```

```cpp
// In Game.cpp - initialize()
camera_ = bestow::Camera2D{
    .position = {0.0f, 0.0f},     // World position of camera center
    .zoom = 1.0f,                  // Scale (1.0 = normal, 2.0 = 2x zoom)
    .rotation = 0.0f,              // Camera rotation in degrees
    .viewportWidth = 1280,         // Window width
    .viewportHeight = 720          // Window height
};
```

### Applying the Camera

```cpp
// In Game.cpp - render()
auto& sys = engine_->systems();

// Set camera before drawing world objects
sys.graphics->setCamera(camera_);

// All subsequent draws use camera transformation
sys.graphics->drawSprite(backgroundSprite);
sys.graphics->drawSprite(playerSprite);
sys.graphics->drawSprite(enemySprite);

// Reset camera for UI (screen-space rendering)
sys.graphics->resetCamera();

// UI draws at exact screen positions
sys.graphics->drawSprite(healthBarSprite);
```

### Camera Follow

Implement smooth camera following:

```cpp
// In Game.cpp - updateFixed()
auto& sys = engine_->systems();

// Get player position
bestow::Vec2 playerPos = sys.physics->getPosition(player_);

// Smooth lerp (adjust 0.1f for speed)
float lerpSpeed = 0.1f;
camera_.position.x += (playerPos.x - camera_.position.x) * lerpSpeed;
camera_.position.y += (playerPos.y - camera_.position.y) * lerpSpeed;

// Clamp camera to level bounds
camera_.position.x = std::clamp(camera_.position.x, 0.0f, levelWidth_);
camera_.position.y = std::clamp(camera_.position.y, 0.0f, levelHeight_);
```

### Camera Deadzone

Prevent constant movement with a deadzone:

```cpp
void Game::updateCamera() {
    auto& sys = engine_->systems();
    bestow::Vec2 playerPos = sys.physics->getPosition(player_);

    // Camera doesn't move if player is in deadzone
    constexpr float DEADZONE_WIDTH = 200.0f;
    constexpr float DEADZONE_HEIGHT = 150.0f;

    float dx = playerPos.x - camera_.position.x;
    float dy = playerPos.y - camera_.position.y;

    // Only move camera if player exits deadzone
    if (std::abs(dx) > DEADZONE_WIDTH / 2.0f) {
        float sign = dx > 0 ? 1.0f : -1.0f;
        camera_.position.x += (std::abs(dx) - DEADZONE_WIDTH / 2.0f) * sign;
    }

    if (std::abs(dy) > DEADZONE_HEIGHT / 2.0f) {
        float sign = dy > 0 ? 1.0f : -1.0f;
        camera_.position.y += (std::abs(dy) - DEADZONE_HEIGHT / 2.0f) * sign;
    }
}
```

## Step 5: Sprite Sheets and Animation

Sprite sheets pack multiple frames into a single texture. Use them for animated characters, tiles, and UI elements.

### Sprite Sheet Structure

```cpp
struct SpriteSheet {
    AssetHandle texture;      // The sprite sheet texture
    int frameWidth;           // Width of one frame
    int frameHeight;          // Height of one frame
    int columns;              // Frames per row
    int rows;                 // Number of rows
};
```

### Creating a Sprite Sheet

```cpp
// In Game.h
private:
    bestow::AssetHandle playerSheet_;
    bestow::SpriteSheet playerSpriteSheet_;
```

```cpp
// In Game.cpp - initialize()
auto& sys = engine_->systems();

// Load sprite sheet texture (8 frames, 4 columns x 2 rows)
playerSheet_ = sys.assets->registerAsset(
    bestow::AssetType::Texture,
    ":assets:/textures/player_sheet.png"
);
sys.assets->loadAsset(playerSheet_);

// Define sprite sheet layout
playerSpriteSheet_ = bestow::SpriteSheet{
    .texture = playerSheet_,
    .frameWidth = 32,
    .frameHeight = 32,
    .columns = 4,
    .rows = 2
};
```

### Drawing a Frame from Sprite Sheet

```cpp
// In Game.cpp - render()
auto& sys = engine_->systems();

int frameIndex = 3;  // Which frame to draw (0-based)

bestow::Vec2 pos = sys.physics->getPosition(player_);

// Draw specific frame from sprite sheet
sys.graphics->drawSprite(playerSpriteSheet_, frameIndex, bestow::Transform2D{
    .x = pos.x,
    .y = pos.y,
    .rotation = 0.0f,
    .scaleX = 1.0f,
    .scaleY = 1.0f
});
```

### Animated Sprite Component

Create a component to handle animation:

```cpp
// In Game.h or Components.h
struct AnimatedSprite {
    bestow::SpriteSheet sheet;
    int currentFrame = 0;
    int startFrame = 0;
    int endFrame = 0;
    float frameTime = 0.1f;      // Seconds per frame
    float elapsed = 0.0f;
    bool loop = true;
    bool playing = true;
};
```

Add to entity:

```cpp
// In Game.cpp - initialize()
auto& sys = engine_->systems();

player_ = sys.entities->createEntity();

// Add animated sprite component
sys.entities->emplace<AnimatedSprite>(player_, AnimatedSprite{
    .sheet = playerSpriteSheet_,
    .currentFrame = 0,
    .startFrame = 0,
    .endFrame = 3,        // Idle animation uses frames 0-3
    .frameTime = 0.15f,   // 150ms per frame
    .loop = true,
    .playing = true
});
```

### Update Animation

```cpp
// In Game.cpp - updateFixed()
auto& sys = engine_->systems();

// Update all animated sprites
auto view = sys.entities->view<AnimatedSprite>();
for (auto entity : view) {
    auto& anim = view.get<AnimatedSprite>(entity);

    if (!anim.playing) continue;

    anim.elapsed += dt;

    if (anim.elapsed >= anim.frameTime) {
        anim.elapsed -= anim.frameTime;
        anim.currentFrame++;

        if (anim.currentFrame > anim.endFrame) {
            if (anim.loop) {
                anim.currentFrame = anim.startFrame;
            } else {
                anim.currentFrame = anim.endFrame;
                anim.playing = false;
            }
        }
    }
}
```

### Render Animated Sprites

```cpp
// In Game.cpp - render()
auto& sys = engine_->systems();

auto view = sys.entities->view<AnimatedSprite, bestow::Transform2D>();
for (auto entity : view) {
    auto& anim = view.get<AnimatedSprite>(entity);
    auto& transform = view.get<bestow::Transform2D>(entity);

    sys.graphics->drawSprite(anim.sheet, anim.currentFrame, transform);
}
```

### Animation State Machine

For character animations (idle, run, jump):

```cpp
enum class AnimState {
    Idle,
    Run,
    Jump,
    Fall
};

struct CharacterAnimator {
    AnimState state = AnimState::Idle;
    std::unordered_map<AnimState, AnimatedSprite> animations;
};

// In initialize()
CharacterAnimator animator;
animator.animations[AnimState::Idle] = AnimatedSprite{
    .sheet = playerSpriteSheet_,
    .startFrame = 0,
    .endFrame = 3,
    .frameTime = 0.15f,
    .loop = true
};
animator.animations[AnimState::Run] = AnimatedSprite{
    .sheet = playerSpriteSheet_,
    .startFrame = 4,
    .endFrame = 7,
    .frameTime = 0.1f,
    .loop = true
};

sys.entities->emplace<CharacterAnimator>(player_, animator);

// In updateFixed()
auto& animator = sys.entities->get<CharacterAnimator>(player_);
bestow::Vec2 velocity = sys.physics->getVelocity(player_);

// Determine state
if (std::abs(velocity.x) > 10.0f) {
    animator.state = AnimState::Run;
} else {
    animator.state = AnimState::Idle;
}

// Update current animation
auto& anim = animator.animations[animator.state];
anim.elapsed += dt;
if (anim.elapsed >= anim.frameTime) {
    anim.elapsed -= anim.frameTime;
    anim.currentFrame++;
    if (anim.currentFrame > anim.endFrame) {
        anim.currentFrame = anim.startFrame;
    }
}

// In render()
auto& animator = sys.entities->get<CharacterAnimator>(player_);
auto& anim = animator.animations[animator.state];
auto& transform = sys.entities->get<bestow::Transform2D>(player_);

sys.graphics->drawSprite(anim.sheet, anim.currentFrame, transform);
```

## Step 6: Advanced Rendering Techniques

### Sprite Flipping

Flip sprites horizontally or vertically by using negative scale:

```cpp
// Face left
transform.scaleX = -1.0f;

// Face right
transform.scaleX = 1.0f;

// Flip vertically
transform.scaleY = -1.0f;
```

### Color Tinting

Apply color effects with tint:

```cpp
// Red tint for damage flash
sprite.tint = bestow::Color{255, 100, 100, 255};

// Fade out (alpha)
sprite.tint = bestow::Color{255, 255, 255, 128};  // 50% transparent

// Grayscale effect (approximate)
sprite.tint = bestow::Color{128, 128, 128, 255};
```

### Parallax Scrolling

Create depth by moving background layers at different speeds:

```cpp
// In Game.cpp - render()
auto& sys = engine_->systems();

// Far background (moves slowly)
bestow::Sprite farBg{
    .textureHandle = &farBgTexture_,
    .sourceRect = {0, 0, 1920, 1080},
    .transform = {
        .x = camera_.position.x * 0.2f,  // 20% of camera movement
        .y = camera_.position.y * 0.2f
    },
    .tint = bestow::Color::white(),
    .layer = 0
};

// Mid background (moves at medium speed)
bestow::Sprite midBg{
    .textureHandle = &midBgTexture_,
    .sourceRect = {0, 0, 1920, 1080},
    .transform = {
        .x = camera_.position.x * 0.5f,  // 50% of camera movement
        .y = camera_.position.y * 0.5f
    },
    .tint = bestow::Color::white(),
    .layer = 5
};

sys.graphics->drawSprite(farBg);
sys.graphics->drawSprite(midBg);
// Draw foreground at layer 10+
```

## Complete Example: Animated Character

Here's a complete example putting it all together:

```cpp
// Game.h
#pragma once

import bestow;
import bestow.core;

class Game {
public:
    bool initialize(bestow::core::Engine& engine);
    void updateFixed(bestow::DeltaTime dt);
    void render(float alpha);
    void shutdown();

private:
    void updateAnimation(bestow::DeltaTime dt);
    void updateCamera();

    bestow::core::Engine* engine_ = nullptr;
    bestow::Entity player_;

    bestow::AssetHandle playerSheet_;
    bestow::SpriteSheet spriteSheet_;

    bestow::Camera2D camera_;

    int currentFrame_ = 0;
    float animTime_ = 0.0f;
    bool facingRight_ = true;
};

// Game.cpp
import std;
import bestow;
import bestow.core;

#include "Game.h"

bool Game::initialize(bestow::core::Engine& engine) {
    engine_ = &engine;
    auto& sys = engine.systems();

    // Load sprite sheet
    playerSheet_ = sys.assets->registerAsset(
        bestow::AssetType::Texture,
        ":assets:/textures/player.png"
    );
    sys.assets->loadAsset(playerSheet_);

    spriteSheet_ = bestow::SpriteSheet{
        .texture = playerSheet_,
        .frameWidth = 32,
        .frameHeight = 32,
        .columns = 8,
        .rows = 1
    };

    // Create player
    player_ = sys.entities->createEntity();
    sys.entities->emplace<bestow::Transform2D>(player_, bestow::Transform2D{
        .x = 400.0f,
        .y = 300.0f
    });

    // Create physics body
    sys.physics->createBody(player_, bestow::PhysicsBodyDef{
        .type = bestow::BodyType::Dynamic,
        .transform = {400.0f, 300.0f},
        .size = {32.0f, 32.0f},
        .fixedRotation = true
    });

    // Setup camera
    camera_ = bestow::Camera2D{
        .position = {400.0f, 300.0f},
        .zoom = 1.0f,
        .viewportWidth = 1280,
        .viewportHeight = 720
    };

    return true;
}

void Game::updateFixed(bestow::DeltaTime dt) {
    auto& sys = engine_->systems();

    // Get input (assume input mappings are set up)
    float horizontal = 0.0f;
    if (sys.input->isActionActive("move_right")) horizontal += 1.0f;
    if (sys.input->isActionActive("move_left")) horizontal -= 1.0f;

    // Update facing direction
    if (horizontal != 0.0f) {
        facingRight_ = horizontal > 0.0f;
    }

    // Apply velocity
    bestow::Vec2 velocity = sys.physics->getVelocity(player_);
    velocity.x = horizontal * 200.0f;
    sys.physics->setVelocity(player_, velocity);

    // Update animation
    updateAnimation(dt);

    // Update camera
    updateCamera();
}

void Game::updateAnimation(bestow::DeltaTime dt) {
    auto& sys = engine_->systems();
    bestow::Vec2 velocity = sys.physics->getVelocity(player_);

    // If moving, animate
    if (std::abs(velocity.x) > 10.0f) {
        animTime_ += dt;
        if (animTime_ >= 0.1f) {  // 100ms per frame
            animTime_ = 0.0f;
            currentFrame_ = (currentFrame_ + 1) % 8;  // 8 frames
        }
    } else {
        currentFrame_ = 0;  // Idle frame
        animTime_ = 0.0f;
    }
}

void Game::updateCamera() {
    auto& sys = engine_->systems();
    bestow::Vec2 playerPos = sys.physics->getPosition(player_);

    // Smooth follow
    camera_.position.x += (playerPos.x - camera_.position.x) * 0.1f;
    camera_.position.y += (playerPos.y - camera_.position.y) * 0.1f;
}

void Game::render(float alpha) {
    auto& sys = engine_->systems();

    // Set camera
    sys.graphics->setCamera(camera_);

    // Get player position
    bestow::Vec2 pos = sys.physics->getPosition(player_);

    // Draw player sprite
    sys.graphics->drawSprite(spriteSheet_, currentFrame_, bestow::Transform2D{
        .x = pos.x,
        .y = pos.y,
        .rotation = 0.0f,
        .scaleX = facingRight_ ? 1.0f : -1.0f,  // Flip based on direction
        .scaleY = 1.0f
    });
}

void Game::shutdown() {
    bestow::core::logInfo("Shutting down");
}
```

## Best Practices

### 1. Always Load Assets Through AssetSystem

**DON'T:**
```cpp
// Direct file I/O - FORBIDDEN
std::ifstream file("texture.png");
```

**DO:**
```cpp
// Use AssetSystem - correct
auto handle = sys.assets->registerAsset(AssetType::Texture, "texture.png");
sys.assets->loadAsset(handle);
```

### 2. Use Layers Consistently

Define layer constants at the top of your file:

```cpp
namespace Layers {
    constexpr int Background = 0;
    constexpr int Environment = 10;
    constexpr int Entities = 30;
    constexpr int Player = 40;
    constexpr int UI = 100;
}
```

### 3. Separate World and UI Rendering

```cpp
void Game::render(float alpha) {
    auto& sys = engine_->systems();

    // World-space rendering
    sys.graphics->setCamera(camera_);
    drawWorld();

    // Screen-space UI
    sys.graphics->resetCamera();
    drawUI();
}
```

### 4. Cache Sprite Sheets

Don't create sprite sheet structures every frame:

```cpp
// BAD - creates every frame
void render() {
    bestow::SpriteSheet sheet{...};  // Wasteful!
    sys.graphics->drawSprite(sheet, frame, transform);
}

// GOOD - create once, reuse
// In Game.h:
bestow::SpriteSheet playerSheet_;

// In initialize():
playerSheet_ = bestow::SpriteSheet{...};

// In render():
sys.graphics->drawSprite(playerSheet_, frame, transform);
```

## Next Steps

Now you can render sprites with confidence! Continue learning:

- **Tutorial 03: Handling Input** - Map keyboard, mouse, and gamepad inputs to actions
- **Tutorial 04: Physics Basics** - Create physics bodies and handle collisions
- **Tutorial 05: Audio** - Add sound effects and music

## Troubleshooting

**Texture not displaying**
- Verify the texture was loaded: check console for "Loaded asset" messages
- Ensure sourceRect matches texture dimensions
- Check layer order - is something drawing on top?
- Verify camera is set correctly

**Animation is choppy**
- Check frameTime is appropriate (0.1f = 10 FPS animation)
- Ensure updateFixed() is being called
- Verify you're not creating new sprite sheets every frame

**Camera not following player**
- Make sure updateCamera() is called in updateFixed()
- Check camera lerp speed (higher = faster, 1.0f = instant)
- Verify player position is being updated

**Sprite is upside down**
- OpenGL and Vulkan have different Y-axis conventions
- Use negative scaleY to flip: `transform.scaleY = -1.0f`

**Memory leak warnings**
- Ensure you unload assets in shutdown()
- Check you're not loading the same asset multiple times
- Use AssetSystem's caching - it loads assets once

# Camera System Developer Guide

## Overview

The Camera System (`ICameraSystem`) provides 2D camera management for your Bestow game. It handles camera positioning, smooth following, screen shake effects, zoom control, and coordinate transformations between screen space and world space.

**Key Features:**
- Smooth camera following with configurable lerp
- Target tracking with offset and deadzone support
- Bounded camera movement (level edges)
- Screen shake effects with decay
- Zoom control (0.1x to 10x)
- Screen-to-world and world-to-screen coordinate conversion

## Core Concepts

### Camera Transform

The camera uses a 2D transform to determine what portion of the world is visible:

```cpp
struct Camera {
    Transform2D transform;  // Position, rotation, scale
    float zoom;            // Zoom level (1.0 = normal)
    Size viewportSize;     // Screen dimensions (width, height)
};
```

### Active Camera Position

The camera has two position concepts:

1. **Base Position** (`getPosition()`): The logical camera position after smoothing and bounds
2. **Rendered Position** (`getCamera().transform`): Base position + shake offset

This separation allows screen shake to affect rendering without changing the logical camera state.

### Viewport and World Space

- **Screen Space**: Pixel coordinates, origin at top-left (0,0 to width,height)
- **World Space**: Game coordinates, origin at camera center
- Zoom affects the visible area: higher zoom = narrower view

## Setup and Initialization

### Creating the Camera System

The Camera System requires a viewport size:

```cpp
import bestow.camera;
import bestow.camera.impl;

// Create camera with 1920x1080 viewport
Size viewportSize{1920, 1080};
auto camera = std::make_unique<CameraSystem>(viewportSize);
```

### Dependency Injection (Kangaru)

When using Kangaru DI in the engine:

```cpp
// In your engine setup
container.emplace<CameraSystemService>(Size{1920, 1080});

// In your game class constructor
MyGame::MyGame(ICameraSystem& camera)
    : camera_(&camera) {
    // Camera is injected
}
```

## Basic Camera Control

### Position and Zoom

```cpp
// Direct position control (instant, no smoothing)
camera->update(dt, Vec2{100.0f, 200.0f});

// Set zoom level (0.1x to 10x, clamped automatically)
camera->setZoom(2.0f);  // 2x zoom (closer)
camera->setZoom(0.5f);  // 0.5x zoom (farther)

// Get current state
Vec2 pos = camera->getPosition();
float zoom = camera->getZoom();
```

### Target Following

Track an entity's position:

```cpp
// Set the entity to follow
Entity player = entities->createEntity();
camera->setTarget(player);

// Update camera to follow the target position
Vec2 playerPos = entities->get<Transform>(player).position;
camera->update(dt, playerPos);

// Stop following
camera->clearTarget();

// Check current target
Entity currentTarget = camera->getTarget();
if (currentTarget != entt::null) {
    // Camera is following an entity
}
```

**Note:** `setTarget()` only stores the entity reference. You still need to pass the position to `update()`. This gives you flexibility in how you compute the target position.

## Smooth Camera Following

### Follow Smoothing

Control how quickly the camera catches up to its target:

```cpp
// Smoothing range: 0.0 (instant) to 1.0 (very slow)
camera->setFollowSmoothing(0.0f);  // Instant snap
camera->setFollowSmoothing(0.1f);  // Fast follow
camera->setFollowSmoothing(0.5f);  // Medium follow
camera->setFollowSmoothing(0.9f);  // Slow, cinematic follow

// Example: Smooth platformer camera
camera->setFollowSmoothing(0.15f);  // Responsive but not jarring
```

The smoothing uses a frame-rate independent lerp: `lerp(current, target, 1.0 - pow(smoothing, dt * 60))`

### Camera Offset

Position the camera relative to the target:

```cpp
// Offset the camera 50 pixels right, 100 pixels up
camera->setOffset(Vec2{50.0f, -100.0f});

// Common use case: Look-ahead in platformers
Vec2 lookAhead{0.0f, 0.0f};
if (playerVelocity.x > 0) {
    lookAhead.x = 100.0f;  // Player moving right
} else if (playerVelocity.x < 0) {
    lookAhead.x = -100.0f; // Player moving left
}
camera->setOffset(lookAhead);
```

### Deadzone

Create a region where the camera doesn't move (reduces camera shake during small movements):

```cpp
// Set deadzone size (width, height)
camera->setDeadzone(Vec2{100.0f, 100.0f});

// Camera only moves when target leaves the deadzone
// Common in action games and platformers
```

**How it works:**
- If the target is within deadzone bounds relative to the camera, the camera stays still
- Once the target exits the deadzone, the camera follows normally
- Useful for preventing jitter from small player movements

## Camera Bounds

### Setting Level Boundaries

Prevent the camera from showing areas outside your level:

```cpp
// Set bounds: minX, maxX, minY, maxY (world coordinates)
camera->setBounds(0.0f, 2000.0f, 0.0f, 1500.0f);

// Camera position will be clamped to keep viewport within bounds
// Accounts for viewport size and zoom level
```

**How bounds work:**
- Bounds are specified in world coordinates
- The system calculates the valid camera center position to keep the viewport inside bounds
- If the viewport is larger than the bounds, the camera centers within the available space

### Clearing Bounds

```cpp
// Remove all boundary restrictions
camera->clearBounds();
```

### Bounds with Zoom

The system automatically adjusts bounds based on zoom:

```cpp
camera->setBounds(0.0f, 1000.0f, 0.0f, 1000.0f);
camera->setZoom(2.0f);  // Higher zoom = tighter effective bounds
camera->setZoom(0.5f);  // Lower zoom = looser effective bounds
```

## Camera Effects

### Screen Shake

Add impact feedback with camera shake:

```cpp
// shake(intensity, duration)
camera->shake(10.0f, 0.5f);  // Medium shake for 0.5 seconds
camera->shake(5.0f, 0.3f);   // Light shake for 0.3 seconds
camera->shake(20.0f, 1.0f);  // Heavy shake for 1 second

// Stop shake immediately
camera->stopShake();
```

**Shake behavior:**
- Intensity controls the pixel offset magnitude
- Duration is in seconds
- Shake automatically decays over time (reduces to zero at end of duration)
- Calling `shake()` again overrides the current shake
- Shake only affects rendered position, not the logical camera position

### Shake Patterns

```cpp
// Impact shake (short, intense)
void onPlayerHit() {
    camera->shake(15.0f, 0.2f);
}

// Explosion shake (medium, fading)
void onExplosion() {
    camera->shake(25.0f, 0.8f);
}

// Continuous rumble (call repeatedly)
void onBossStomping(DeltaTime dt) {
    if (bossIsStomping && shakeTimer <= 0.0f) {
        camera->shake(8.0f, 0.3f);
        shakeTimer = 0.3f;
    }
    shakeTimer -= dt;
}
```

## Coordinate Conversion

### Screen to World

Convert mouse/touch input to world coordinates:

```cpp
// Get mouse position from input system
Vec2 mouseScreen = input->getMousePosition();

// Convert to world coordinates
Vec2 mouseWorld = camera->screenToWorld(mouseScreen);

// Example: Click to place object
void onMouseClick(Vec2 screenPos) {
    Vec2 worldPos = camera->screenToWorld(screenPos);
    spawnObjectAt(worldPos);
}
```

### World to Screen

Convert game object positions to screen coordinates for UI:

```cpp
// Get player position in world
Vec2 playerWorld = entities->get<Transform>(player).position;

// Convert to screen coordinates
Vec2 playerScreen = camera->worldToScreen(playerWorld);

// Example: Draw name tag above player
void drawNameTag(Entity entity) {
    Vec2 worldPos = entities->get<Transform>(entity).position;
    Vec2 screenPos = camera->worldToScreen(worldPos);
    drawTextAt(screenPos, getName(entity));
}
```

### Round-Trip Conversion

Conversions are bidirectional and account for zoom:

```cpp
Vec2 original{123.0f, 456.0f};
Vec2 screen = camera->worldToScreen(original);
Vec2 backToWorld = camera->screenToWorld(screen);

// backToWorld == original (within floating-point precision)
```

## API Reference

### Target Following

```cpp
// Set entity to track (stores reference only)
void setTarget(Entity target);

// Stop tracking entity
void clearTarget();

// Get current tracked entity
Entity getTarget() const;
```

### Follow Behavior

```cpp
// Set smoothing factor (0.0 = instant, 1.0 = very slow)
// Values are clamped to [0.0, 1.0]
void setFollowSmoothing(float smoothing);

// Set offset from target position
void setOffset(Vec2 offset);

// Set deadzone size (camera doesn't move if target within deadzone)
// Negative values are converted to absolute
void setDeadzone(Vec2 size);
```

### Bounds

```cpp
// Set camera bounds (world coordinates)
void setBounds(float minX, float maxX, float minY, float maxY);

// Remove boundary restrictions
void clearBounds();
```

### Effects

```cpp
// Start screen shake (intensity in pixels, duration in seconds)
void shake(float intensity, float duration);

// Stop current shake immediately
void stopShake();
```

### Zoom

```cpp
// Set zoom level (clamped to [0.1, 10.0])
void setZoom(float zoom);

// Get current zoom level
float getZoom() const;
```

### Update and State

```cpp
// Update camera position (call every frame)
// targetPosition: world position to follow
void update(DeltaTime dt, Vec2 targetPosition);

// Get camera state (includes shake offset)
Camera getCamera() const;

// Get logical camera position (no shake offset)
Vec2 getPosition() const;
```

### Coordinate Conversion

```cpp
// Convert screen coordinates to world coordinates
Vec2 screenToWorld(Vec2 screenPos) const;

// Convert world coordinates to screen coordinates
Vec2 worldToScreen(Vec2 worldPos) const;
```

## Best Practices

### 1. Camera Smoothing for Different Game Types

```cpp
// Fast-paced action (low smoothing)
camera->setFollowSmoothing(0.1f);

// Platformer (medium smoothing)
camera->setFollowSmoothing(0.15f);

// Cinematic/exploration (high smoothing)
camera->setFollowSmoothing(0.3f);

// Twin-stick shooter (instant)
camera->setFollowSmoothing(0.0f);
```

### 2. Use Deadzones for Stability

```cpp
// Prevent camera jitter from idle animations
camera->setDeadzone(Vec2{50.0f, 30.0f});

// Larger deadzone for top-down games
camera->setDeadzone(Vec2{150.0f, 150.0f});

// No deadzone for precise following
camera->setDeadzone(Vec2{0.0f, 0.0f});
```

### 3. Set Bounds from Level Data

```cpp
void loadLevel(const LevelData& level) {
    // Prevent showing empty space outside level
    camera->setBounds(
        0.0f,
        level.width,
        0.0f,
        level.height
    );
}
```

### 4. Context-Sensitive Shake

```cpp
// Vary shake based on impact
void applyDamage(float damage) {
    float intensity = std::min(damage * 2.0f, 30.0f);
    float duration = 0.2f + (damage * 0.01f);
    camera->shake(intensity, duration);
}
```

### 5. Update Order

```cpp
void update(DeltaTime dt) {
    // 1. Update game logic
    physics->update(dt);

    // 2. Get target position AFTER physics
    Vec2 playerPos = entities->get<Transform>(player).position;

    // 3. Update camera last
    camera->update(dt, playerPos);
}
```

## Complete Examples

### Example 1: Platformer Camera with Look-Ahead

```cpp
class PlatformerCamera {
    ICameraSystem* camera_;
    Entity player_;

public:
    void init(ICameraSystem& camera, Entity player) {
        camera_ = &camera;
        player_ = player;

        // Medium smoothing for responsiveness
        camera_->setFollowSmoothing(0.15f);

        // Small deadzone to reduce jitter
        camera_->setDeadzone(Vec2{40.0f, 20.0f});

        // Set level bounds
        camera_->setBounds(0.0f, 5000.0f, 0.0f, 1500.0f);

        // Track player
        camera_->setTarget(player_);
    }

    void update(DeltaTime dt, IEntitySystem& entities) {
        auto& transform = entities.get<Transform>(player_);
        auto& velocity = entities.get<Velocity>(player_);

        // Look-ahead based on horizontal velocity
        Vec2 lookAhead{0.0f, 0.0f};
        if (std::abs(velocity.x) > 50.0f) {
            lookAhead.x = velocity.x > 0 ? 120.0f : -120.0f;
        }

        // Slight upward offset to show more of the level ahead
        lookAhead.y = -80.0f;

        camera_->setOffset(lookAhead);
        camera_->update(dt, transform.position);
    }

    void onPlayerHit() {
        // Light shake on damage
        camera_->shake(8.0f, 0.3f);
    }

    void onPlayerDeath() {
        // Heavy shake on death
        camera_->shake(20.0f, 0.8f);
    }
};
```

### Example 2: Top-Down Camera with Mouse Look

```cpp
class TopDownCamera {
    ICameraSystem* camera_;
    Entity player_;
    IInputSystem* input_;

public:
    void init(ICameraSystem& camera, IInputSystem& input, Entity player) {
        camera_ = &camera;
        input_ = &input;
        player_ = player;

        // Instant following for responsive controls
        camera_->setFollowSmoothing(0.0f);

        // Large deadzone for mouse-look comfort
        camera_->setDeadzone(Vec2{200.0f, 200.0f});

        camera_->setTarget(player_);
    }

    void update(DeltaTime dt, IEntitySystem& entities) {
        auto& playerTransform = entities.get<Transform>(player_);
        Vec2 playerPos = playerTransform.position;

        // Get mouse world position
        Vec2 mouseScreen = input_->getMousePosition();
        Vec2 mouseWorld = camera_->screenToWorld(mouseScreen);

        // Camera target is between player and mouse
        Vec2 delta = mouseWorld - playerPos;
        Vec2 midpoint = playerPos + delta * 0.3f; // 30% toward mouse

        camera_->update(dt, midpoint);
    }
};
```

### Example 3: Cinematic Camera Transitions

```cpp
class CinematicCamera {
    ICameraSystem* camera_;
    Vec2 startPos_;
    Vec2 endPos_;
    float transitionTime_;
    float currentTime_;
    bool isTransitioning_;

public:
    void init(ICameraSystem& camera) {
        camera_ = &camera;
        isTransitioning_ = false;

        // Very smooth for cinematic feel
        camera_->setFollowSmoothing(0.4f);

        // No bounds during cinematics
        camera_->clearBounds();
    }

    void transitionTo(Vec2 targetPos, float duration) {
        startPos_ = camera_->getPosition();
        endPos_ = targetPos;
        transitionTime_ = duration;
        currentTime_ = 0.0f;
        isTransitioning_ = true;
    }

    void update(DeltaTime dt) {
        if (!isTransitioning_) return;

        currentTime_ += dt;
        float t = std::min(currentTime_ / transitionTime_, 1.0f);

        // Smooth ease-in-out
        float smoothT = t * t * (3.0f - 2.0f * t);
        Vec2 currentTarget = startPos_ + (endPos_ - startPos_) * smoothT;

        camera_->update(dt, currentTarget);

        if (t >= 1.0f) {
            isTransitioning_ = false;
        }
    }
};
```

### Example 4: Split-Screen Multiplayer

```cpp
class SplitScreenGame {
    std::unique_ptr<ICameraSystem> camera1_;
    std::unique_ptr<ICameraSystem> camera2_;
    Entity player1_;
    Entity player2_;

public:
    void init() {
        // Create two separate cameras with different viewports
        camera1_ = std::make_unique<CameraSystem>(Size{960, 1080}); // Left half
        camera2_ = std::make_unique<CameraSystem>(Size{960, 1080}); // Right half

        // Both cameras use same settings
        camera1_->setFollowSmoothing(0.1f);
        camera2_->setFollowSmoothing(0.1f);

        camera1_->setTarget(player1_);
        camera2_->setTarget(player2_);
    }

    void update(DeltaTime dt, IEntitySystem& entities) {
        Vec2 p1Pos = entities.get<Transform>(player1_).position;
        Vec2 p2Pos = entities.get<Transform>(player2_).position;

        camera1_->update(dt, p1Pos);
        camera2_->update(dt, p2Pos);
    }

    void render(IGraphicsSystem& graphics) {
        // Render left viewport
        graphics.setViewport(0, 0, 960, 1080);
        graphics.setCamera(camera1_->getCamera());
        renderWorld(graphics);

        // Render right viewport
        graphics.setViewport(960, 0, 960, 1080);
        graphics.setCamera(camera2_->getCamera());
        renderWorld(graphics);
    }
};
```

### Example 5: Camera Shake System with Priorities

```cpp
class ShakeManager {
    ICameraSystem* camera_;
    float currentIntensity_;
    float currentDuration_;

public:
    void init(ICameraSystem& camera) {
        camera_ = &camera;
        currentIntensity_ = 0.0f;
        currentDuration_ = 0.0f;
    }

    // Only apply shake if it's more intense than current shake
    void applyShake(float intensity, float duration) {
        if (intensity > currentIntensity_) {
            camera_->shake(intensity, duration);
            currentIntensity_ = intensity;
            currentDuration_ = duration;
        }
    }

    void update(DeltaTime dt) {
        currentDuration_ -= dt;
        if (currentDuration_ <= 0.0f) {
            currentIntensity_ = 0.0f;
        }
    }

    // Preset shake types
    void lightShake() { applyShake(5.0f, 0.2f); }
    void mediumShake() { applyShake(10.0f, 0.4f); }
    void heavyShake() { applyShake(20.0f, 0.8f); }
    void explosionShake() { applyShake(30.0f, 1.0f); }
};
```

## Performance Considerations

### Update Frequency

```cpp
// Camera updates are lightweight - safe to call every frame
void update(DeltaTime dt) {
    camera->update(dt, targetPosition);
}
```

### Coordinate Conversions

```cpp
// Conversions are cheap (simple math), but cache if used repeatedly
Vec2 mouseWorld = camera->screenToWorld(mouseScreen);
// Don't call screenToWorld multiple times for same input
```

### Multiple Cameras

```cpp
// Each camera is independent - safe to have multiple instances
// Useful for:
// - Split-screen multiplayer
// - Minimap rendering
// - Picture-in-picture effects
```

### Zoom and Bounds

```cpp
// Bounds calculations account for zoom
// No need to manually adjust bounds when zoom changes
camera->setZoom(2.0f);
// Bounds automatically tighten to keep viewport inside
```

## Common Gotchas

### 1. Shake Affects Rendering, Not Logic

```cpp
// Camera position for gameplay logic (no shake)
Vec2 logicalPos = camera->getPosition();

// Camera for rendering (includes shake)
Camera renderCam = camera->getCamera();
Vec2 renderPos{renderCam.transform.x, renderCam.transform.y};

// These may differ during shake
```

### 2. Update After Physics

```cpp
// WRONG: Camera updates before physics
camera->update(dt, playerPos);
physics->update(dt);

// CORRECT: Camera updates after physics
physics->update(dt);
Vec2 newPlayerPos = entities->get<Transform>(player).position;
camera->update(dt, newPlayerPos);
```

### 3. Deadzone is Relative to Camera

```cpp
// Deadzone is centered on the camera position
// Not centered on the target position
camera->setDeadzone(Vec2{100.0f, 100.0f});

// If target is 40 pixels away from camera, camera doesn't move
// If target is 60 pixels away, camera follows
```

### 4. Bounds Account for Viewport

```cpp
// If your level is 1000x1000 and viewport is 800x600:
camera->setBounds(0.0f, 1000.0f, 0.0f, 1000.0f);

// Camera position will be clamped to [400, 600] x [300, 700]
// This keeps the edges of the viewport aligned with level bounds
```

## Integration with Other Systems

### With Entity System

```cpp
// Camera typically follows an entity with a Transform component
auto& transform = entities->get<Transform>(player);
camera->update(dt, transform.position);
```

### With Input System

```cpp
// Convert mouse input to world coordinates
Vec2 mouseScreen = input->getMousePosition();
Vec2 mouseWorld = camera->screenToWorld(mouseScreen);

if (input->isMouseButtonPressed(MouseButton::Left)) {
    interactWithWorld(mouseWorld);
}
```

### With Graphics System

```cpp
// Graphics system uses camera for rendering
Camera cam = camera->getCamera();
graphics->setCamera(cam);
graphics->render();
```

### With Physics System

```cpp
// Get camera bounds for culling
Camera cam = camera->getCamera();
float halfWidth = (cam.viewportSize.width * 0.5f) / cam.zoom;
float halfHeight = (cam.viewportSize.height * 0.5f) / cam.zoom;

Rect visibleArea{
    cam.transform.x - halfWidth,
    cam.transform.y - halfHeight,
    halfWidth * 2,
    halfHeight * 2
};

// Only simulate physics for visible objects
physics->setCullingArea(visibleArea);
```

## Troubleshooting

### Camera Feels Sluggish

```cpp
// Reduce smoothing value
camera->setFollowSmoothing(0.05f); // Lower = faster
```

### Camera Too Jittery

```cpp
// Increase smoothing or add deadzone
camera->setFollowSmoothing(0.2f);
camera->setDeadzone(Vec2{60.0f, 60.0f});
```

### Shake Too Subtle

```cpp
// Increase intensity
camera->shake(25.0f, 0.5f); // Higher intensity
```

### Shake Too Violent

```cpp
// Reduce intensity or duration
camera->shake(5.0f, 0.2f); // Lower intensity, shorter duration
```

### Camera Shows Empty Space

```cpp
// Set bounds to match your level size
camera->setBounds(0.0f, levelWidth, 0.0f, levelHeight);
```

### Mouse Position Incorrect

```cpp
// Make sure you're converting screen to world
Vec2 mouseWorld = camera->screenToWorld(mouseScreen);
// NOT using mouseScreen directly
```

## Summary

The Camera System provides:

- **Smooth following** with configurable lerp and deadzones
- **Bounded movement** to keep camera inside level
- **Screen shake** for impact feedback
- **Zoom control** from 0.1x to 10x
- **Coordinate conversion** between screen and world space

**Typical Usage Pattern:**

```cpp
// Init
camera->setFollowSmoothing(0.15f);
camera->setDeadzone(Vec2{50.0f, 30.0f});
camera->setBounds(0.0f, levelWidth, 0.0f, levelHeight);
camera->setTarget(player);

// Update (every frame)
Vec2 playerPos = entities->get<Transform>(player).position;
camera->update(dt, playerPos);

// Render
graphics->setCamera(camera->getCamera());

// Effects
camera->shake(10.0f, 0.3f); // On impact

// Input handling
Vec2 mouseWorld = camera->screenToWorld(input->getMousePosition());
```

For questions or issues, refer to:
- `/bestow-contract/src/bestow.camera.cppm` - Interface definition
- `/bestow-camera/src/CameraSystem.cpp` - Implementation
- `/tests/unit/CameraSystemTests.cpp` - Usage examples and tests

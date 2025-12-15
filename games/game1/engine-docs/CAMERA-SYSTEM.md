# Camera System Developer Guide

## Overview

The Camera System (`ICameraSystem`) provides 2D camera management for your Bestow game. It handles smooth camera following, target tracking, screen shake effects, zoom control, bounded movement, and coordinate transformations between screen space and world space.

**Key Features:**
- Entity target tracking with smooth following
- Configurable smoothing (0.0 = instant snap, 1.0 = very slow)
- Offset and deadzone support for advanced camera behavior
- Bounded camera movement to prevent showing off-screen areas
- Screen shake effects with automatic intensity decay
- Zoom control (0.1x to 10x, automatically clamped)
- Bidirectional coordinate conversion (screen-to-world and world-to-screen)

## Core Concepts

### Camera Data Structure

The camera's state is represented by the `Camera` struct:

```cpp
struct Camera {
    Transform2D transform;  // Position (x, y), rotation, scale
    float zoom;            // Zoom level (1.0 = normal, 2.0 = 2x closer)
    Size viewportSize;     // Viewport dimensions in pixels
};
```

### Camera Position vs Rendered Position

The camera maintains two distinct position concepts:

1. **Logical Position** (`getPosition()`): The base camera position after smoothing and bounds. This is the "true" camera position used for gameplay logic.

2. **Rendered Position** (`getCamera().transform`): Logical position + shake offset. This is what's actually used for rendering.

This separation allows screen shake to affect the visual output without changing the camera's logical state, ensuring gameplay logic remains stable during shake effects.

### Coordinate Spaces

- **Screen Space**: Pixel coordinates with origin at top-left (0, 0 to width, height)
- **World Space**: Game world coordinates, origin at camera center
- **Zoom Effect**: Higher zoom values = smaller visible area (zoomed in)

The coordinate conversion functions account for both viewport size and zoom level.

## Setup

### Basic Initialization

The Camera System is typically created through the EngineBuilder:

```cpp
import bestow.core;

auto engineResult = bestow::core::EngineBuilder()
    .withCamera({.width = 1280, .height = 720})
    .build();
```

### Manual Construction (Advanced)

If you need to create a camera system directly:

```cpp
import bestow.camera;
import bestow.camera.impl;

Size viewportSize{1920, 1080};
auto camera = std::make_unique<CameraSystem>(viewportSize);
```

## Basic Camera Control

### Direct Position Control

Update the camera to follow a specific world position:

```cpp
// Camera immediately moves toward this position (respecting smoothing and bounds)
Vec2 targetPos{100.0f, 200.0f};
camera->update(dt, targetPos);

// Get current logical position
Vec2 currentPos = camera->getPosition();
```

### Zoom Control

```cpp
// Set zoom level (automatically clamped to 0.1x - 10.0x)
camera->setZoom(2.0f);  // 2x zoom (closer view, smaller visible area)
camera->setZoom(0.5f);  // 0.5x zoom (farther view, larger visible area)

// Get current zoom
float currentZoom = camera->getZoom();
```

## Target Following

### Setting a Target Entity

The camera can track a specific entity. Note that `setTarget()` only stores the entity reference - you still need to pass the entity's position to `update()`.

```cpp
// Tell the camera which entity to conceptually follow
Entity player = entities->createEntity();
camera->setTarget(player);

// In your update loop, pass the entity's actual position
Vec2 playerPos = entities->get<Transform2D>(player).position;
camera->update(dt, playerPos);

// Stop following
camera->clearTarget();

// Check what's being followed
Entity currentTarget = camera->getTarget();
if (currentTarget != entt::null) {
    // Camera has an active target
}
```

**Why separate setTarget() and update()?** This design gives you flexibility in how you compute the target position - you can add prediction, interpolation, or custom logic before passing it to the camera.

## Smooth Following

### Follow Smoothing

Control how quickly the camera catches up to the target position:

```cpp
// Smoothing range: 0.0 (instant) to 1.0 (very slow)
camera->setFollowSmoothing(0.0f);   // Instant snap, no smoothing
camera->setFollowSmoothing(0.1f);   // Very responsive, slight smoothing
camera->setFollowSmoothing(0.15f);  // Balanced (good for platformers)
camera->setFollowSmoothing(0.5f);   // Moderate lag
camera->setFollowSmoothing(0.9f);   // Very slow, cinematic feel
```

**How it works:** The smoothing value controls a framerate-independent lerp. The system uses exponential smoothing that feels consistent across different frame rates.

**Typical values:**
- **Fast action games**: 0.05 - 0.1
- **Platformers**: 0.1 - 0.2
- **Exploration games**: 0.2 - 0.4
- **Cinematic sequences**: 0.5 - 0.9
- **Twin-stick shooters**: 0.0 (instant)

### Camera Offset

Position the camera relative to the target. Useful for look-ahead in platformers:

```cpp
// Offset 50 pixels right, 100 pixels up from target
camera->setOffset(Vec2{50.0f, -100.0f});

// Dynamic look-ahead based on player movement
Vec2 lookAhead{0.0f, 0.0f};
if (playerVelocity.x > 50.0f) {
    lookAhead.x = 120.0f;  // Moving right - look ahead
} else if (playerVelocity.x < -50.0f) {
    lookAhead.x = -120.0f; // Moving left - look ahead
}
camera->setOffset(lookAhead);
```

### Deadzone

Create a region where the camera doesn't move. The target can move freely within this zone without the camera reacting:

```cpp
// Set deadzone size (width, height in world units)
camera->setDeadzone(Vec2{100.0f, 80.0f});

// Camera only moves when target exits this rectangle
```

**How it works:**
- The deadzone is an invisible rectangle centered on the current camera position
- If the target is within this rectangle, `targetPosition_` is set to the current position (no movement)
- Once the target crosses the deadzone boundary, the camera begins following normally
- Negative values are automatically converted to absolute values

**Use cases:**
- Reduce camera jitter from idle animations or small movements
- Create a "comfort zone" for the player in action games
- Implement classic top-down camera behavior

## Camera Bounds

### Setting Level Boundaries

Prevent the camera from showing areas outside your level:

```cpp
// Set bounds: minX, maxX, minY, maxY (world coordinates)
camera->setBounds(0.0f, 2000.0f, 0.0f, 1500.0f);

// Camera position is automatically clamped to keep viewport within bounds
```

**How bounds work:**
- Bounds are specified in world coordinates
- The system calculates the valid camera center position to ensure the viewport edges align with the bounds
- Accounts for viewport size and current zoom level
- If viewport is larger than bounds, camera centers within available space

### Clearing Bounds

```cpp
// Remove all boundary restrictions (camera can move anywhere)
camera->clearBounds();
```

### Bounds with Zoom

Bounds automatically adapt to zoom changes:

```cpp
camera->setBounds(0.0f, 1000.0f, 0.0f, 1000.0f);
camera->setZoom(2.0f);  // Higher zoom = viewport shows smaller area = tighter effective bounds
camera->setZoom(0.5f);  // Lower zoom = viewport shows larger area = looser effective bounds
```

No manual adjustment needed - the system handles this automatically.

## Screen Shake

### Basic Shake

Add impact feedback with camera shake:

```cpp
// shake(intensity, duration)
camera->shake(10.0f, 0.5f);  // Medium shake for 0.5 seconds
camera->shake(5.0f, 0.3f);   // Light shake for 0.3 seconds
camera->shake(20.0f, 1.0f);  // Heavy shake for 1 second

// Immediately stop any active shake
camera->stopShake();
```

**Shake parameters:**
- **Intensity**: Pixel offset magnitude (higher = more violent shake)
- **Duration**: Time in seconds

**Shake behavior:**
- Shake intensity automatically decays from full strength to zero over the duration
- Random offset is generated each frame using `Math::randomFloat()`
- Calling `shake()` again overrides any current shake
- Shake affects only the rendered camera position, not the logical position
- Gameplay logic (collision detection, AI, etc.) uses the unshaken position

### Shake Use Cases

```cpp
// Light impact (player hit)
void onPlayerDamaged() {
    camera->shake(8.0f, 0.2f);
}

// Medium impact (enemy killed)
void onEnemyKilled() {
    camera->shake(12.0f, 0.4f);
}

// Heavy impact (explosion)
void onExplosion(Vec2 position, float radius) {
    float distance = glm::length(playerPos - position);
    if (distance < radius) {
        float intensity = 25.0f * (1.0f - distance / radius);
        camera->shake(intensity, 0.6f);
    }
}

// Continuous rumble (boss stomping)
void updateBossStomp(DeltaTime dt) {
    if (bossIsStomping && stompShakeTimer <= 0.0f) {
        camera->shake(8.0f, 0.3f);
        stompShakeTimer = 0.3f;
    }
    stompShakeTimer -= dt;
}
```

## Coordinate Conversion

### Screen to World

Convert mouse/touch input to world coordinates:

```cpp
// Get mouse position from input system
Vec2 mouseScreen = input->getMousePosition();

// Convert to world space
Vec2 mouseWorld = camera->screenToWorld(mouseScreen);

// Use for gameplay interaction
if (input->isMouseButtonJustPressed(MouseButton::Left)) {
    spawnObjectAt(mouseWorld);
}
```

### World to Screen

Convert game object positions to screen coordinates (useful for UI positioning):

```cpp
// Get entity position in world space
Vec2 entityWorldPos = entities->get<Transform2D>(entity).position;

// Convert to screen space
Vec2 entityScreenPos = camera->worldToScreen(entityWorldPos);

// Draw UI element above entity
drawNameTag(entityScreenPos, entity);
```

### Round-Trip Conversion

Conversions are mathematically reversible (within floating-point precision):

```cpp
Vec2 original{123.0f, 456.0f};
Vec2 screen = camera->worldToScreen(original);
Vec2 backToWorld = camera->screenToWorld(screen);

// backToWorld ≈ original (within floating-point tolerance)
```

## Update Cycle

The camera should be updated every frame after physics and game logic:

```cpp
void MyGame::updateFixed(DeltaTime dt) {
    auto& sys = engine_->systems();

    // 1. Update physics and game logic first
    sys.physics->update(dt);
    updatePlayerLogic(dt);

    // 2. Get target position AFTER physics update
    Vec2 playerPos = sys.entities->get<Transform2D>(player_).position;

    // 3. Update camera last
    sys.camera->update(dt, playerPos);
}
```

**Why update camera last?**
- Ensures the camera sees the most up-to-date entity positions
- Prevents one-frame lag between entity movement and camera following

## API Reference

### Target Following

```cpp
// Set the entity to track (stores reference only, doesn't affect position yet)
void setTarget(Entity target);

// Clear the tracked entity
void clearTarget();

// Get the current tracked entity (returns entt::null if none)
Entity getTarget() const;
```

### Follow Behavior

```cpp
// Set smoothing factor: 0.0 = instant, 1.0 = very slow
// Values automatically clamped to [0.0, 1.0]
void setFollowSmoothing(float smoothing);

// Set offset from target position (applied before deadzone/smoothing)
void setOffset(Vec2 offset);

// Set deadzone size (camera doesn't move if target within deadzone)
// Negative values converted to absolute
void setDeadzone(Vec2 size);
```

### Bounds

```cpp
// Set camera bounds in world coordinates
void setBounds(float minX, float maxX, float minY, float maxY);

// Remove all boundary restrictions
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
// Set zoom level (automatically clamped to [0.1, 10.0])
void setZoom(float zoom);

// Get current zoom level
float getZoom() const;
```

### Update and State

```cpp
// Update camera position (call every frame)
// targetPosition: world position to move toward
void update(DeltaTime dt, Vec2 targetPosition);

// Get camera state for rendering (includes shake offset)
Camera getCamera() const;

// Get logical camera position (no shake offset)
Vec2 getPosition() const;
```

### Coordinate Conversion

```cpp
// Convert screen coordinates (pixels) to world coordinates (game units)
Vec2 screenToWorld(Vec2 screenPos) const;

// Convert world coordinates (game units) to screen coordinates (pixels)
Vec2 worldToScreen(Vec2 worldPos) const;
```

## Complete Examples

### Example 1: Platformer Camera with Look-Ahead

```cpp
class PlatformerCamera {
    ICameraSystem* camera_;
    Entity player_;

public:
    void init(ICameraSystem& camera, Entity player, float levelWidth, float levelHeight) {
        camera_ = &camera;
        player_ = player;

        // Responsive smoothing for platformers
        camera_->setFollowSmoothing(0.15f);

        // Small deadzone to reduce jitter
        camera_->setDeadzone(Vec2{40.0f, 20.0f});

        // Set level bounds
        camera_->setBounds(0.0f, levelWidth, 0.0f, levelHeight);

        // Track player
        camera_->setTarget(player_);
    }

    void update(DeltaTime dt, IEntitySystem& entities) {
        auto& transform = entities.get<Transform2D>(player_);
        auto& velocity = entities.get<Velocity>(player_);

        // Dynamic look-ahead based on horizontal velocity
        Vec2 lookAhead{0.0f, 0.0f};
        if (std::abs(velocity.x) > 50.0f) {
            // Look ahead in movement direction
            lookAhead.x = velocity.x > 0 ? 120.0f : -120.0f;
        }

        // Slight upward offset to show more of the level ahead
        lookAhead.y = -80.0f;

        camera_->setOffset(lookAhead);
        camera_->update(dt, transform.position);
    }

    void onPlayerHit() {
        camera_->shake(8.0f, 0.3f);
    }

    void onPlayerDeath() {
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
        auto& playerTransform = entities.get<Transform2D>(player_);
        Vec2 playerPos = playerTransform.position;

        // Get mouse world position
        Vec2 mouseScreen = input_->getMousePosition();
        Vec2 mouseWorld = camera_->screenToWorld(mouseScreen);

        // Camera target is 30% toward mouse from player
        Vec2 delta = mouseWorld - playerPos;
        Vec2 targetPos = playerPos + delta * 0.3f;

        camera_->update(dt, targetPos);
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

        // Smooth ease-in-out (smoothstep)
        float smoothT = t * t * (3.0f - 2.0f * t);
        Vec2 currentTarget = startPos_ + (endPos_ - startPos_) * smoothT;

        camera_->update(dt, currentTarget);

        if (t >= 1.0f) {
            isTransitioning_ = false;
        }
    }
};
```

### Example 4: Distance-Based Screen Shake

```cpp
class ProximityShake {
    ICameraSystem* camera_;
    Vec2 playerPos_;

public:
    void init(ICameraSystem& camera) {
        camera_ = &camera;
    }

    void updatePlayerPosition(Vec2 pos) {
        playerPos_ = pos;
    }

    // Shake intensity based on distance from event
    void shakeAtPosition(Vec2 eventPos, float maxRadius, float maxIntensity, float duration) {
        float distance = glm::length(playerPos_ - eventPos);

        if (distance < maxRadius) {
            // Linear falloff based on distance
            float falloff = 1.0f - (distance / maxRadius);
            float intensity = maxIntensity * falloff;
            camera_->shake(intensity, duration);
        }
    }

    // Example: Explosion at world position
    void onExplosion(Vec2 pos) {
        shakeAtPosition(pos, 500.0f, 30.0f, 0.8f);
    }

    // Example: Enemy landing
    void onEnemyLand(Vec2 pos) {
        shakeAtPosition(pos, 200.0f, 15.0f, 0.3f);
    }
};
```

## Best Practices

### 1. Choose Appropriate Smoothing

```cpp
// Fast-paced action (minimal lag)
camera->setFollowSmoothing(0.05f - 0.1f);

// Platformers (balanced feel)
camera->setFollowSmoothing(0.1f - 0.2f);

// Exploration/puzzle (cinematic)
camera->setFollowSmoothing(0.2f - 0.4f);

// Twin-stick shooter (instant)
camera->setFollowSmoothing(0.0f);
```

### 2. Use Deadzones Wisely

```cpp
// Small deadzone for platformers (reduce idle jitter)
camera->setDeadzone(Vec2{40.0f, 30.0f});

// Large deadzone for top-down games (player freedom)
camera->setDeadzone(Vec2{150.0f, 150.0f});

// No deadzone for precise following
camera->setDeadzone(Vec2{0.0f, 0.0f});
```

### 3. Set Bounds from Level Data

```cpp
void loadLevel(const LevelData& level) {
    // Prevent showing empty space outside level
    camera->setBounds(0.0f, level.width, 0.0f, level.height);
}
```

### 4. Update Camera After Physics

```cpp
void updateFixed(DeltaTime dt) {
    // 1. Physics first
    physics->update(dt);

    // 2. Get updated entity positions
    Vec2 playerPos = entities->get<Transform2D>(player).position;

    // 3. Camera last
    camera->update(dt, playerPos);
}
```

### 5. Use Logical Position for Gameplay

```cpp
// For raycasting, collision checks, AI
Vec2 logicalCamPos = camera->getPosition();

// For rendering
Camera renderCam = camera->getCamera();
graphics->setCamera(renderCam);
```

### 6. Scale Shake with Impact

```cpp
void applyDamage(float damage) {
    float intensity = std::clamp(damage * 2.0f, 5.0f, 30.0f);
    float duration = 0.2f + (damage * 0.01f);
    camera->shake(intensity, duration);
}
```

## Common Gotchas

### 1. Shake Only Affects Rendering

```cpp
// Logical position (for gameplay logic)
Vec2 logicalPos = camera->getPosition();

// Rendered position (for drawing)
Camera cam = camera->getCamera();
Vec2 renderPos{cam.transform.x, cam.transform.y};

// These differ during shake!
```

### 2. Update Camera After Physics

```cpp
// WRONG - Camera sees old positions
camera->update(dt, playerPos);
physics->update(dt);

// CORRECT - Camera sees new positions
physics->update(dt);
Vec2 newPlayerPos = entities->get<Transform2D>(player).position;
camera->update(dt, newPlayerPos);
```

### 3. Deadzone is Centered on Camera

```cpp
// Deadzone is relative to current camera position, not target
camera->setDeadzone(Vec2{100.0f, 100.0f});

// If target is 40 pixels from camera center -> camera doesn't move
// If target is 60 pixels from camera center -> camera follows
```

### 4. Bounds Account for Viewport Size

```cpp
// Level is 1000x1000, viewport is 800x600
camera->setBounds(0.0f, 1000.0f, 0.0f, 1000.0f);

// Camera center will be clamped to [400, 600] x [300, 700]
// This keeps the viewport edges aligned with level bounds
```

### 5. setTarget() Doesn't Move Camera

```cpp
camera->setTarget(player);  // Just stores entity reference

// Camera doesn't move until you call update() with position:
Vec2 playerPos = entities->get<Transform2D>(player).position;
camera->update(dt, playerPos);
```

## Integration with Other Systems

### With Entity System

```cpp
// Camera typically follows an entity with Transform2D
auto& transform = entities->get<Transform2D>(player);
camera->update(dt, transform.position);
```

### With Input System

```cpp
// Convert mouse to world coordinates for interaction
Vec2 mouseScreen = input->getMousePosition();
Vec2 mouseWorld = camera->screenToWorld(mouseScreen);

if (input->isMouseButtonPressed(MouseButton::Left)) {
    handleWorldClick(mouseWorld);
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
// Calculate visible area for culling
Camera cam = camera->getCamera();
float halfWidth = (cam.viewportSize.width * 0.5f) / cam.zoom;
float halfHeight = (cam.viewportSize.height * 0.5f) / cam.zoom;

Rect visibleArea{
    cam.transform.x - halfWidth,
    cam.transform.y - halfHeight,
    halfWidth * 2.0f,
    halfHeight * 2.0f
};

physics->setCullingArea(visibleArea);
```

## Troubleshooting

### Camera Feels Sluggish

```cpp
// Reduce smoothing value (lower = faster response)
camera->setFollowSmoothing(0.05f);
```

### Camera Too Jittery

```cpp
// Increase smoothing or add deadzone
camera->setFollowSmoothing(0.2f);
camera->setDeadzone(Vec2{60.0f, 60.0f});
```

### Shake Too Subtle

```cpp
// Increase intensity or duration
camera->shake(25.0f, 0.6f);
```

### Shake Too Violent

```cpp
// Reduce intensity or duration
camera->shake(5.0f, 0.2f);
```

### Camera Shows Empty Space

```cpp
// Set bounds to match level size
camera->setBounds(0.0f, levelWidth, 0.0f, levelHeight);
```

### Mouse Position Incorrect

```cpp
// Make sure you're converting screen to world
Vec2 mouseWorld = camera->screenToWorld(mouseScreen);
// NOT using mouseScreen directly for world interaction
```

### Camera Not Following Target

```cpp
// Remember: setTarget() only stores the reference
camera->setTarget(player);

// You must still call update() with position:
Vec2 playerPos = entities->get<Transform2D>(player).position;
camera->update(dt, playerPos);
```

## Summary

The Camera System provides:

- **Entity target tracking** with `setTarget()` / `clearTarget()` / `getTarget()`
- **Smooth following** with configurable lerp via `setFollowSmoothing()`
- **Camera offset** for look-ahead with `setOffset()`
- **Deadzones** to reduce jitter with `setDeadzone()`
- **Bounded movement** with `setBounds()` / `clearBounds()`
- **Screen shake** effects with `shake()` / `stopShake()`
- **Zoom control** from 0.1x to 10.0x with `setZoom()` / `getZoom()`
- **Position access** via `getCamera()` and `getPosition()`
- **Coordinate conversion** with `screenToWorld()` and `worldToScreen()`

**Typical Usage Pattern:**

```cpp
// Init
camera->setTarget(player);
camera->setFollowSmoothing(0.15f);
camera->setDeadzone(Vec2{50.0f, 30.0f});
camera->setBounds(0.0f, levelWidth, 0.0f, levelHeight);

// Update (every frame, after physics)
Vec2 playerPos = entities->get<Transform2D>(player).position;
camera->update(dt, playerPos);

// Render
Camera cam = camera->getCamera();
graphics->setCamera(cam);

// Effects
camera->shake(10.0f, 0.3f);  // On impact

// Input handling
Vec2 mouseWorld = camera->screenToWorld(input->getMousePosition());
```

For more information, see:
- `/bestow-contract/src/bestow.camera.cppm` - Interface definition
- `/bestow-camera/src/CameraSystem.cpp` - Implementation
- `/tests/unit/CameraSystemTests.cpp` - Usage examples and test cases

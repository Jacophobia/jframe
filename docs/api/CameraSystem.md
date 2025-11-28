# CameraSystem API

The `CameraSystem` provides camera control with target following, smoothing, bounds, and effects.

## Overview

```cpp
auto& camera = sys.camera;

// Follow player
camera->setTarget(player);
camera->setFollowSmoothing(0.1f);  // Smooth following
camera->setOffset(Vec2{0.0f, -50.0f});  // Look ahead

// Set bounds
camera->setBounds(0.0f, 3200.0f, 0.0f, 720.0f);

// Camera shake on impact
camera->shake(10.0f, 0.5f);

// Update camera (in update loop)
Vec2 targetPos = physics->getPosition(player);
camera->update(dt, targetPos);

// Apply camera to graphics
graphics->setCamera(camera->getCamera());
```

## Target Following

### setTarget(Entity target)

```cpp
void setTarget(Entity target);
```

Sets the entity for the camera to follow.

**Example:**

```cpp
camera->setTarget(player);
```

---

### clearTarget()

```cpp
void clearTarget();
```

Stops following any target.

---

### getTarget()

```cpp
Entity getTarget() const;
```

Returns the current target entity.

---

## Follow Behavior

### setFollowSmoothing(float smoothing)

```cpp
void setFollowSmoothing(float smoothing);
```

Sets how smoothly the camera follows the target.

**Range:** 0.0 (instant) to 1.0 (very slow)

**Example:**

```cpp
camera->setFollowSmoothing(0.0f);   // Instant (no smoothing)
camera->setFollowSmoothing(0.1f);   // Fast and responsive
camera->setFollowSmoothing(0.3f);   // Smooth
camera->setFollowSmoothing(0.7f);   // Very smooth
```

---

### setOffset(Vec2 offset)

```cpp
void setOffset(Vec2 offset);
```

Sets an offset from the target position.

**Use for:** Looking ahead in the direction of movement

**Example:**

```cpp
// Look ahead vertically (player at bottom third)
camera->setOffset(Vec2{0.0f, -100.0f});

// Look ahead horizontally based on facing direction
Vec2 facingDir = player->facingRight ? Vec2{50.0f, 0.0f} : Vec2{-50.0f, 0.0f};
camera->setOffset(facingDir);
```

---

### setDeadzone(Vec2 size)

```cpp
void setDeadzone(Vec2 size);
```

Sets a deadzone where the camera doesn't move.

**Use for:** Reducing camera jitter during small movements

**Example:**

```cpp
// Small deadzone (16 pixels)
camera->setDeadzone(Vec2{16.0f, 16.0f});

// Larger deadzone (platform feel)
camera->setDeadzone(Vec2{64.0f, 32.0f});

// No deadzone
camera->setDeadzone(Vec2{0.0f, 0.0f});
```

---

## Camera Bounds

### setBounds(float minX, float maxX, float minY, float maxY)

```cpp
void setBounds(float minX, float maxX, float minY, float maxY);
```

Constrains the camera to a rectangular area.

**Use for:** Preventing camera from showing outside the level

**Example:**

```cpp
// Level bounds
auto metadata = levels->getLevelMetadata(currentLevel);
camera->setBounds(
    0.0f, metadata.width,
    0.0f, metadata.height
);
```

---

### clearBounds()

```cpp
void clearBounds();
```

Removes camera bounds.

---

## Camera Effects

### shake(float intensity, float duration)

```cpp
void shake(float intensity, float duration);
```

Shakes the camera with the specified intensity and duration.

**Parameters:**
- `intensity`: Maximum displacement in pixels
- `duration`: Shake duration in seconds

**Example:**

```cpp
// Small shake on landing
camera->shake(5.0f, 0.2f);

// Big shake on explosion
camera->shake(20.0f, 0.5f);

// Earthquake
camera->shake(30.0f, 2.0f);
```

---

### stopShake()

```cpp
void stopShake();
```

Immediately stops camera shake.

---

## Zoom

### setZoom(float zoom)

```cpp
void setZoom(float zoom);
float getZoom() const;
```

Sets the camera zoom level.

**Range:**
- `1.0` = Normal (1:1 pixel mapping)
- `< 1.0` = Zoomed out (see more)
- `> 1.0` = Zoomed in (see less)

**Example:**

```cpp
camera->setZoom(1.0f);   // Normal
camera->setZoom(0.5f);   // Zoomed out (2x view distance)
camera->setZoom(2.0f);   // Zoomed in (2x magnification)
```

---

## Update and State

### update(DeltaTime dt, Vec2 targetPosition)

```cpp
void update(DeltaTime dt, Vec2 targetPosition);
```

Updates the camera position and effects.

**Call once per frame** in your update loop.

**Example:**

```cpp
void updateFixed(DeltaTime dt) override {
    // Update camera to follow player
    Vec2 playerPos = physics->getPosition(player);
    camera->update(dt, playerPos);

    // Apply camera to graphics
    graphics->setCamera(camera->getCamera());
}
```

---

### getCamera()

```cpp
Camera getCamera() const;
```

Returns the current camera state for rendering.

**Camera Structure:**

```cpp
struct Camera {
    Transform2D transform;
    float zoom = 1.0f;
    Size viewportSize;
};
```

---

### getPosition()

```cpp
Vec2 getPosition() const;
```

Returns the current camera position.

---

## Coordinate Conversion

### screenToWorld(Vec2 screenPos)

```cpp
Vec2 screenToWorld(Vec2 screenPos) const;
```

Converts screen coordinates to world coordinates.

**Example:**

```cpp
Vec2 mouseScreen = input->getMousePosition();
Vec2 mouseWorld = camera->screenToWorld(mouseScreen);
```

---

### worldToScreen(Vec2 worldPos)

```cpp
Vec2 worldToScreen(Vec2 worldPos) const;
```

Converts world coordinates to screen coordinates.

**Example:**

```cpp
Vec2 enemyPos = physics->getPosition(enemy);
Vec2 enemyScreen = camera->worldToScreen(enemyPos);

// Draw UI indicator at enemy's screen position
```

---

## Common Patterns

### Basic Player Following

```cpp
void initializeCamera() {
    auto windowSize = graphics->getWindowSize();
    camera = std::make_unique<CameraSystem>(windowSize);

    camera->setTarget(player);
    camera->setFollowSmoothing(0.1f);
}

void update(DeltaTime dt) {
    Vec2 playerPos = physics->getPosition(player);
    camera->update(dt, playerPos);
    graphics->setCamera(camera->getCamera());
}
```

---

### Look-Ahead Camera

```cpp
void updateLookAheadCamera(DeltaTime dt) {
    Vec2 playerPos = physics->getPosition(player);
    Vec2 playerVel = physics->getVelocity(player);

    // Look ahead in movement direction
    Vec2 lookAhead = glm::normalize(playerVel) * 100.0f;
    camera->setOffset(lookAhead);

    camera->update(dt, playerPos);
    graphics->setCamera(camera->getCamera());
}
```

---

### Platform Camera (Mario-style)

```cpp
void setupPlatformCamera() {
    camera->setFollowSmoothing(0.2f);

    // Looser horizontal following, tighter vertical
    camera->setDeadzone(Vec2{32.0f, 8.0f});

    // Keep player at bottom third of screen
    auto windowSize = graphics->getWindowSize();
    camera->setOffset(Vec2{0.0f, -windowSize.height / 3.0f});
}
```

---

### Room-Based Camera (Zelda-style)

```cpp
struct Room {
    float x, y;
    float width, height;
};

void snapToRoom(const Room& room) {
    // Disable smoothing for instant snap
    camera->setFollowSmoothing(0.0f);

    // Set bounds to room
    camera->setBounds(
        room.x,
        room.x + room.width,
        room.y,
        room.y + room.height
    );

    // Center camera on room
    auto windowSize = graphics->getWindowSize();
    Vec2 roomCenter{
        room.x + room.width / 2.0f,
        room.y + room.height / 2.0f
    };

    camera->update(0.0f, roomCenter);
}
```

---

### Cutscene Camera

```cpp
class CutsceneCamera {
    std::vector<Vec2> waypoints_;
    float duration_;
    float elapsed_ = 0.0f;

public:
    void play(const std::vector<Vec2>& waypoints, float duration) {
        waypoints_ = waypoints;
        duration_ = duration;
        elapsed_ = 0.0f;

        // Disable target following
        camera->clearTarget();
        camera->setFollowSmoothing(0.0f);
    }

    void update(DeltaTime dt) {
        elapsed_ += dt;
        float t = std::clamp(elapsed_ / duration_, 0.0f, 1.0f);

        // Interpolate between waypoints
        Vec2 pos = interpolateWaypoints(waypoints_, t);
        camera->update(dt, pos);

        if (elapsed_ >= duration_) {
            // Re-enable player following
            camera->setTarget(player);
        }
    }
};
```

---

### Dynamic Zoom

```cpp
void updateDynamicZoom(DeltaTime dt) {
    Vec2 vel = physics->getVelocity(player);
    float speed = glm::length(vel);

    // Zoom out when moving fast
    float targetZoom = 1.0f;
    if (speed > 300.0f) {
        targetZoom = 0.8f;  // Zoom out
    }

    // Smooth zoom transition
    float currentZoom = camera->getZoom();
    float newZoom = std::lerp(currentZoom, targetZoom, 0.05f);
    camera->setZoom(newZoom);
}
```

---

### Split Screen (Manual)

```cpp
void renderSplitScreen() {
    auto windowSize = graphics->getWindowSize();

    // Player 1 camera (left half)
    Camera cam1 = camera1->getCamera();
    cam1.viewportSize = {windowSize.width / 2, windowSize.height};

    graphics->setCamera(cam1);
    graphics->renderEntities(*entities,
        RenderLayers::Background,
        RenderLayers::UI
    );

    // Player 2 camera (right half)
    Camera cam2 = camera2->getCamera();
    cam2.viewportSize = {windowSize.width / 2, windowSize.height};
    cam2.transform.x += windowSize.width / 2;  // Offset right

    graphics->setCamera(cam2);
    graphics->renderEntities(*entities,
        RenderLayers::Background,
        RenderLayers::UI
    );
}
```

---

### Camera Shake on Events

```cpp
void setupCameraShake() {
    // Shake on explosion
    events->subscribe("Explosion", [](const EventData& data) {
        camera->shake(20.0f, 0.5f);
    });

    // Shake on damage
    events->subscribe(Events::EntityDamaged, [](const EventData& data) {
        auto& damage = std::get<DamageEventData>(data);

        if (entities->allOf<Player>(damage.target)) {
            // Shake based on damage amount
            float intensity = std::min(damage.amount / 10.0f, 15.0f);
            camera->shake(intensity, 0.3f);
        }
    });

    // Shake on landing
    events->subscribe("PlayerLanded", [](const EventData& data) {
        camera->shake(5.0f, 0.15f);
    });
}
```

---

### Smooth Camera Transitions

```cpp
void transitionToPosition(Vec2 target, float duration, std::function<void()> onComplete) {
    camera->clearTarget();

    Vec2 start = camera->getPosition();
    float elapsed = 0.0f;

    auto transition = [=]() mutable {
        elapsed += dt;
        float t = std::clamp(elapsed / duration, 0.0f, 1.0f);

        // Ease out cubic
        float eased = 1.0f - std::pow(1.0f - t, 3.0f);

        Vec2 pos = glm::mix(start, target, eased);
        camera->update(dt, pos);

        if (t >= 1.0f) {
            onComplete();
            return true;  // Done
        }
        return false;  // Continue
    };

    // Add to update loop
}
```

---

## Performance Tips

1. **Call `update()` once per frame** - Don't update multiple times
2. **Use bounds** - Prevent camera from rendering off-level
3. **Moderate smoothing** - Too much smoothing feels sluggish
4. **Small deadzones** - Large deadzones feel unresponsive
5. **Cache camera queries** - Don't call `screenToWorld()` repeatedly for same position

## See Also

- [GraphicsSystem](GraphicsSystem.md) - Applying camera to rendering
- [InputSystem](InputSystem.md) - Mouse input for camera control
- [PhysicsSystem](PhysicsSystem.md) - Getting entity positions

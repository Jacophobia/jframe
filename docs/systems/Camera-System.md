# JFrame Camera System

## Overview

The Camera System provides 2D camera functionality for JFrame games, including position tracking, zoom control, entity following with smooth interpolation, camera bounds, screen shake effects, and coordinate conversion between world and screen space.

**Module:** `jframe.camera` (interface), `jframe.camera.impl` (implementation)
**Dependencies:** `jframe.types`
**Implementation:** `jframe-camera/`

## Key Features

- Smooth camera following with configurable interpolation
- Target entity tracking
- Camera offset and deadzone support
- Viewport bounds constraints
- Zoom control with automatic clamping
- Screen shake effects with decay
- World-to-screen and screen-to-world coordinate conversion
- Independent camera state management

## Core Concepts

### Camera Coordinates

JFrame uses a 2D coordinate system where:
- The camera position represents the center of the viewport
- World coordinates are independent of screen size
- Screen coordinates are pixel-based (0,0 is top-left)
- Zoom factor scales the world-to-screen conversion

### Camera Following

The camera can follow a target entity with several behaviors:
- **Instant following**: Camera snaps directly to target (smoothing = 0.0)
- **Smooth following**: Camera interpolates toward target (smoothing > 0.0)
- **Offset**: Camera maintains a constant offset from target
- **Deadzone**: Camera only moves when target exits a defined region

## Interface

### Basic Operations

```cpp
// Create camera system with viewport size
auto windowSize = sys.graphics->getWindowSize();
auto camera = std::make_unique<jframe::CameraSystem>(windowSize);

// Set target entity to follow
camera->setTarget(playerEntity);

// Clear target (free camera)
camera->clearTarget();

// Get current target
Entity target = camera->getTarget();

// Update camera each frame
camera->update(deltaTime, targetWorldPosition);

// Get camera for rendering
Camera cam = camera->getCamera();
sys.graphics->setCamera(cam);

// Get camera position
Vec2 pos = camera->getPosition();
```

### Zoom Control

```cpp
// Set zoom level (clamped to 0.1 - 10.0)
camera->setZoom(2.0f);  // 2x zoom in
camera->setZoom(0.5f);  // 2x zoom out

// Get current zoom
float zoom = camera->getZoom();
```

### Follow Behavior

```cpp
// Set smoothing (0.0 = instant, 1.0 = very slow)
camera->setFollowSmoothing(0.1f);  // Quick follow
camera->setFollowSmoothing(0.9f);  // Slow, smooth follow

// Set offset from target
camera->setOffset({0.0f, -50.0f});  // Camera above target

// Set deadzone size (camera doesn't move within this area)
camera->setDeadzone({100.0f, 50.0f});  // 100px wide, 50px tall
```

### Camera Bounds

```cpp
// Set world boundaries (camera won't show outside these)
camera->setBounds(0.0f, 1000.0f, 0.0f, 800.0f);
// minX, maxX, minY, maxY

// Remove bounds (camera can move freely)
camera->clearBounds();
```

### Screen Shake

```cpp
// Trigger shake effect
camera->shake(10.0f, 0.5f);  // intensity, duration in seconds

// Stop shake immediately
camera->stopShake();
```

### Coordinate Conversion

```cpp
// Convert screen pixel coordinates to world coordinates
Vec2 worldPos = camera->screenToWorld({400, 300});

// Convert world coordinates to screen pixel coordinates
Vec2 screenPos = camera->worldToScreen({100.0f, 200.0f});

// Common use: mouse click to world position
Vec2 mouseScreen = sys.input->getMousePosition();
Vec2 mouseWorld = camera->screenToWorld(mouseScreen);
```

## Camera Type

The `Camera` type returned by `getCamera()` contains:

```cpp
struct Camera {
    Transform2D transform;  // Position with shake applied
    float zoom;            // Current zoom level
    Size viewportSize;     // Screen dimensions
};
```

## Practical Examples

### Example 1: Basic Following Camera

```cpp
// In game initialization
auto windowSize = sys.graphics->getWindowSize();
cameraSystem_ = std::make_unique<jframe::CameraSystem>(windowSize);
cameraSystem_->setTarget(player_);

// In update loop
void Game::updateCamera(DeltaTime dt) {
    // Get player position
    auto playerTransform = sys.entities->get<Transform2D>(player_);

    // Update camera to follow player
    cameraSystem_->update(dt, playerTransform.position());

    // Apply camera to graphics system
    sys.graphics->setCamera(cameraSystem_->getCamera());
}
```

### Example 2: Smooth Following with Offset

```cpp
// Initialize camera with smooth follow
cameraSystem_->setZoom(1.5f);                      // Zoom in a bit
cameraSystem_->setFollowSmoothing(0.15f);          // Smooth following
cameraSystem_->setOffset({0.0f, -100.0f});         // Camera above player

// Camera will smoothly follow player, staying 100 units above
cameraSystem_->update(dt, playerPosition);
```

### Example 3: Platformer Camera with Deadzone

```cpp
// Setup platformer-style camera
cameraSystem_->setFollowSmoothing(0.0f);           // Instant response
cameraSystem_->setDeadzone({200.0f, 100.0f});      // Wide horizontal, tight vertical

// Camera only moves when player exits deadzone box
cameraSystem_->update(dt, playerPosition);
```

### Example 4: Bounded Camera for Level

```cpp
// Constrain camera to level boundaries
float levelWidth = 2400.0f;
float levelHeight = 1600.0f;
cameraSystem_->setBounds(0.0f, levelWidth, 0.0f, levelHeight);

// Camera will never show areas outside level bounds
cameraSystem_->update(dt, playerPosition);
```

### Example 5: Screen Shake on Hit

```cpp
// In collision handler
void Game::onPlayerHit() {
    // Trigger camera shake
    cameraSystem_->shake(15.0f, 0.3f);  // Strong shake for 0.3 seconds

    // Play hit sound
    sys.audio->playPositional(hitSoundHandle_);
}
```

### Example 6: Mouse Click in World Space

```cpp
// Convert mouse click to world position
void Game::handleMouseClick(Vec2 mouseScreenPos) {
    Vec2 mouseWorldPos = cameraSystem_->screenToWorld(mouseScreenPos);

    // Spawn entity at clicked position
    createEntityAt(mouseWorldPos);
}
```

### Example 7: Zoom with Mouse Wheel

```cpp
// In input handler
void Game::handleMouseWheel(float delta) {
    float currentZoom = cameraSystem_->getZoom();
    float newZoom = currentZoom + (delta * 0.1f);
    cameraSystem_->setZoom(newZoom);  // Automatically clamped to valid range
}
```

### Example 8: Cinematic Camera Movement

```cpp
// Free camera (no target following)
cameraSystem_->clearTarget();

// Manually control camera position
Vec2 cinematicPosition = interpolateCameraPath(time);
cameraSystem_->update(dt, cinematicPosition);

// Resume following player
cameraSystem_->setTarget(player_);
```

### Example 9: Split-Screen Setup

```cpp
// Player 1 camera (left half of screen)
auto camera1 = std::make_unique<CameraSystem>(Size{400, 600});
camera1->setTarget(player1_);

// Player 2 camera (right half of screen)
auto camera2 = std::make_unique<CameraSystem>(Size{400, 600});
camera2->setTarget(player2_);

// In render
camera1->update(dt, player1Pos);
camera2->update(dt, player2Pos);

// Render each view to different viewport regions
```

### Example 10: Top-Down Camera with Smooth Zoom

```cpp
// Setup top-down game camera
cameraSystem_->setFollowSmoothing(0.2f);

// Smooth zoom transition
float targetZoom = isZoomedOut ? 0.5f : 1.5f;
float currentZoom = cameraSystem_->getZoom();
float newZoom = lerp(currentZoom, targetZoom, dt * 2.0f);
cameraSystem_->setZoom(newZoom);
```

## Complete Integration Example

Here's a complete example from the platformer demo:

```cpp
class Game {
public:
    bool initialize(jframe::core::Engine& engine) {
        auto& sys = engine.systems();

        // Initialize camera with window size
        auto windowSize = sys.graphics->getWindowSize();
        cameraSystem_ = std::make_unique<jframe::CameraSystem>(windowSize);

        // Load camera config from Lua
        cameraSystem_->setZoom(config_->getFloatOr("camera.zoom", 1.0f));
        cameraSystem_->setFollowSmoothing(
            config_->getFloatOr("camera.followSmoothing", 0.0f)
        );
        cameraSystem_->setDeadzone({
            config_->getFloatOr("camera.deadzone.x", 0.0f),
            config_->getFloatOr("camera.deadzone.y", 0.0f)
        });

        // Setup player and set as camera target
        setupPlayer();
        cameraSystem_->setTarget(player_);

        // Load shake config
        cameraShakeMagnitude_ = config_->getFloatOr("camera.shake.magnitude", 10.0f);
        cameraShakeDuration_ = config_->getFloatOr("camera.shake.duration", 0.3f);

        return true;
    }

    void updateFixed(DeltaTime dt) {
        updateCamera(dt);
    }

    void updateCamera(DeltaTime dt) {
        auto& sys = engine_->systems();

        // Get player position
        auto playerTransform = sys.entities->get<Transform2D>(player_);

        // Update camera to follow player
        cameraSystem_->update(dt, playerTransform.position());

        // Apply camera to graphics system for rendering
        sys.graphics->setCamera(cameraSystem_->getCamera());
    }

    void onPlayerHit() {
        // Trigger camera shake when player is hit
        cameraSystem_->shake(cameraShakeMagnitude_, cameraShakeDuration_);
    }

    void renderHUD() {
        auto& sys = engine_->systems();

        // HUD must be drawn in world coordinates relative to camera
        auto camera = cameraSystem_->getCamera();
        float camX = camera.transform.x;
        float camY = camera.transform.y;
        float halfWidth = camera.viewportSize.width / 2.0f;
        float halfHeight = camera.viewportSize.height / 2.0f;

        // Calculate top-left corner of screen in world coordinates
        float screenLeft = camX - halfWidth;
        float screenTop = camY - halfHeight;

        // Draw HUD elements at fixed screen positions
        Vec2 scorePos{screenLeft + 620.0f, screenTop + 10.0f};
        sys.graphics->drawText(
            "SCORE: " + std::to_string(score_),
            scorePos,
            fontHandle_,
            16.0f,
            scoreColor_
        );
    }

private:
    std::unique_ptr<jframe::CameraSystem> cameraSystem_;
    Entity player_;
    float cameraShakeMagnitude_;
    float cameraShakeDuration_;
};
```

## Configuration Example

Camera settings in `data/config/game.lua`:

```lua
return {
    camera = {
        zoom = 1.0,
        followSmoothing = 0.1,  -- 0.0 = instant, 1.0 = very slow

        deadzone = {
            x = 100.0,  -- Horizontal deadzone
            y = 50.0    -- Vertical deadzone
        },

        offset = {
            x = 0.0,
            y = -50.0   -- Camera slightly above player
        },

        bounds = {
            enabled = true,
            minX = 0.0,
            maxX = 2400.0,
            minY = 0.0,
            maxY = 1600.0
        },

        shake = {
            magnitude = 10.0,  -- Shake intensity
            duration = 0.3     -- Duration in seconds
        }
    }
}
```

## Technical Details

### Zoom Constraints

The camera system automatically clamps zoom values:
- Minimum zoom: `0.1` (10x zoom out)
- Maximum zoom: `10.0` (10x zoom in)
- Default zoom: `1.0` (no zoom)

### Smoothing Algorithm

The smoothing uses exponential interpolation:

```cpp
float lerpFactor = 1.0f - std::pow(smoothing, dt * 60.0f);
position = lerp(position, target, lerpFactor);
```

Higher smoothing values create slower, more damped movement.

### Deadzone Behavior

The deadzone creates a rectangular area centered on the camera:
- When the target is inside the deadzone, the camera doesn't move
- When the target exits the deadzone, the camera moves to keep the target at the deadzone edge
- Useful for reducing camera jitter and giving players freedom of movement

### Bounds Constraints

When bounds are set:
- The camera position is clamped to ensure the viewport stays within bounds
- The clamping accounts for zoom level and viewport size
- If the viewport is larger than the bounds, the camera centers on the bounded area

### Screen Shake Implementation

The shake effect:
- Applies a random offset to the camera position each frame
- Decays linearly over the shake duration
- Does not affect the base camera position (useful for HUD positioning)
- Can be stopped early with `stopShake()`

### Coordinate Conversion Math

World to screen:
```cpp
screenX = (worldX - cameraX) * zoom + viewportWidth / 2
screenY = (worldY - cameraY) * zoom + viewportHeight / 2
```

Screen to world:
```cpp
worldX = (screenX - viewportWidth / 2) / zoom + cameraX
worldY = (screenY - viewportHeight / 2) / zoom + cameraY
```

## Common Patterns

### Follow Player with Look-Ahead

```cpp
// Add offset in movement direction
Vec2 velocity = sys.physics->getVelocity(player_);
Vec2 lookAhead = {velocity.x * 0.3f, 0.0f};
cameraSystem_->setOffset(lookAhead);
```

### Constrain Camera to Room

```cpp
// When player enters new room
void enterRoom(const Room& room) {
    cameraSystem_->setBounds(
        room.minX, room.maxX,
        room.minY, room.maxY
    );
}
```

### Cutscene Mode

```cpp
// Start cutscene
void startCutscene() {
    cameraSystem_->clearTarget();
    savedPlayerTarget_ = player_;
}

// Animate camera through points
void updateCutscene(DeltaTime dt) {
    Vec2 cutscenePos = getCutscenePosition(cutsceneTime_);
    cameraSystem_->update(dt, cutscenePos);
}

// End cutscene
void endCutscene() {
    cameraSystem_->setTarget(savedPlayerTarget_);
}
```

### Shake on Impact

```cpp
// Vary shake intensity by impact force
void onImpact(float impactForce) {
    float intensity = std::min(impactForce * 0.1f, 20.0f);
    float duration = 0.2f + (intensity * 0.02f);
    cameraSystem_->shake(intensity, duration);
}
```

### Mini-Map Camera

```cpp
// Create separate camera for mini-map
auto miniMapCamera = std::make_unique<CameraSystem>(Size{200, 200});
miniMapCamera->setZoom(0.2f);  // Zoomed out view
miniMapCamera->setTarget(player_);

// Render mini-map to texture or viewport region
```

## Performance Considerations

### Update Frequency

The camera should be updated once per frame in the fixed update loop:

```cpp
void updateFixed(DeltaTime dt) {
    // ... other updates ...
    updateCamera(dt);
}
```

### HUD Rendering

When rendering HUD elements, calculate screen-space positions relative to the camera:

```cpp
auto camera = cameraSystem_->getCamera();
Vec2 topLeft = {
    camera.transform.x - camera.viewportSize.width / 2.0f,
    camera.transform.y - camera.viewportSize.height / 2.0f
};

// HUD element at fixed screen position (10, 10)
Vec2 hudPos = topLeft + Vec2{10.0f, 10.0f};
```

### Coordinate Conversion

Screen-to-world conversion is useful for:
- Mouse input handling
- Click-to-move mechanics
- Placing objects in world space
- Debug visualization

World-to-screen conversion is useful for:
- Drawing HUD markers above entities
- Viewport culling checks
- UI element positioning
- Debug overlays

## Debugging

### Visualize Camera Bounds

```cpp
void debugDrawCamera() {
    auto camera = cameraSystem_->getCamera();
    Vec2 pos = cameraSystem_->getPosition();

    // Draw camera center
    sys.graphics->drawCircle(pos, 5.0f, {255, 0, 0, 255});

    // Draw viewport bounds
    float halfW = camera.viewportSize.width / (2.0f * camera.zoom);
    float halfH = camera.viewportSize.height / (2.0f * camera.zoom);

    sys.graphics->drawRect(
        Canvas{
            .origin = {int(pos.x - halfW), int(pos.y - halfH)},
            .size = {int(halfW * 2), int(halfH * 2)}
        },
        {0, 255, 0, 128},
        false  // Outline only
    );
}
```

### Visualize Deadzone

```cpp
void debugDrawDeadzone() {
    auto pos = cameraSystem_->getPosition();
    auto deadzoneSize = getDeadzoneSize();  // You'll need to expose this

    sys.graphics->drawRect(
        Canvas{
            .origin = {
                int(pos.x - deadzoneSize.x / 2),
                int(pos.y - deadzoneSize.y / 2)
            },
            .size = {int(deadzoneSize.x), int(deadzoneSize.y)}
        },
        {255, 255, 0, 128},
        false
    );
}
```

## Best Practices

1. **Update Order**: Update camera after physics simulation but before rendering
2. **Single Camera**: Most games need only one camera system instance
3. **HUD Coordinates**: Always calculate HUD positions relative to camera for zoom/shake effects
4. **Config-Driven**: Store camera parameters in config files for easy tweaking
5. **Smooth Transitions**: Use smoothing for player-controlled cameras, instant for cinematic cuts
6. **Bounds Testing**: Always set bounds for levels to prevent showing empty space
7. **Shake Moderation**: Use short, subtle shakes - long intense shakes cause nausea
8. **Coordinate Systems**: Be consistent - use world coordinates for gameplay, screen for UI
9. **Viewport Sync**: Update camera viewport size when window is resized
10. **Target Validation**: Check if target entity is valid before following

## Common Pitfalls

### Incorrect HUD Positioning

```cpp
// WRONG: HUD drawn at fixed world position (moves with camera)
sys.graphics->drawText(scoreText, {10.0f, 10.0f}, font, 16.0f, color);

// CORRECT: HUD drawn relative to camera
auto camera = cameraSystem_->getCamera();
Vec2 topLeft = {
    camera.transform.x - camera.viewportSize.width / 2.0f,
    camera.transform.y - camera.viewportSize.height / 2.0f
};
Vec2 hudPos = topLeft + Vec2{10.0f, 10.0f};
sys.graphics->drawText(scoreText, hudPos, font, 16.0f, color);
```

### Forgetting to Apply Camera

```cpp
// WRONG: Camera updated but not applied
cameraSystem_->update(dt, playerPos);
// Rendering happens with old camera

// CORRECT: Apply camera before rendering
cameraSystem_->update(dt, playerPos);
sys.graphics->setCamera(cameraSystem_->getCamera());
```

### Bounds Smaller Than Viewport

```cpp
// WRONG: Bounds smaller than viewport causes jittering
cameraSystem_->setBounds(0.0f, 400.0f, 0.0f, 300.0f);
// With 800x600 viewport, camera can't fit within bounds

// CORRECT: Ensure bounds are larger than viewport
cameraSystem_->setBounds(0.0f, 2400.0f, 0.0f, 1800.0f);
```

### Update Without Target Position

```cpp
// WRONG: Updating with stale position
Vec2 oldPlayerPos = {100.0f, 100.0f};
// ... player moves ...
cameraSystem_->update(dt, oldPlayerPos);  // Uses old position

// CORRECT: Always use current position
auto playerTransform = sys.entities->get<Transform2D>(player_);
cameraSystem_->update(dt, playerTransform.position());
```

## Testing

Camera system tests cover:

```cpp
TEST(CameraSystemTest, InitialState)           // Default state
TEST(CameraSystemTest, SetZoom)                // Zoom control
TEST(CameraSystemTest, ZoomClamping)           // Min/max zoom
TEST(CameraSystemTest, SetTarget)              // Target following
TEST(CameraSystemTest, UpdateWithoutSmoothing) // Instant follow
TEST(CameraSystemTest, UpdateWithSmoothing)    // Smooth follow
TEST(CameraSystemTest, CameraOffset)           // Offset behavior
TEST(CameraSystemTest, DeadzoneNoMovement)     // Deadzone inside
TEST(CameraSystemTest, DeadzoneMovement)       // Deadzone outside
TEST(CameraSystemTest, BoundsConstrain)        // Bounds clamping
TEST(CameraSystemTest, CameraShake)            // Shake effect
TEST(CameraSystemTest, ShakeDecay)             // Shake decay
TEST(CameraSystemTest, ScreenToWorld)          // Coordinate conversion
TEST(CameraSystemTest, WorldToScreen)          // Coordinate conversion
TEST(CameraSystemTest, ConversionRoundTrip)    // Conversion accuracy
```

Run tests:
```bash
ctest --preset macos-debug -R "CameraSystemTest"
```

## EngineBuilder Integration (Planned)

Currently, the Camera System is created manually in game code. A planned enhancement will integrate it into the EngineBuilder:

```cpp
// Current approach - manual creation
auto engine = EngineBuilder()
    .withGraphics(config)
    .withEntities()
    .build();

// Manual camera setup
auto windowSize = engine->systems().graphics->getWindowSize();
cameraSystem_ = std::make_unique<jframe::CameraSystem>(windowSize);

// Planned approach - automatic integration
auto engine = EngineBuilder()
    .withGraphics(config)
    .withEntities()
    .withCamera()  // NEW - automatic camera system
    .build();

// Access via engine systems
auto& camera = engine->systems().camera;
camera->setTarget(player_);
```

### Planned Features

| Feature | Description | Status |
|---------|-------------|--------|
| `.withCamera()` | Add camera system to engine | 🔲 Planned |
| Auto viewport sync | Camera resizes with window | 🔲 Planned |
| Multi-camera support | Named cameras for split-screen | 🔲 Planned |
| Camera as component | Attach camera behavior to entities | 🔲 Planned |

### Camera Component (Alternative Pattern)

For simpler games, camera behavior can be attached directly to entities:

```cpp
// Alternative: Camera as component on entity
Entity cameraEntity = entities->createEntity();
entities->emplace<Camera2D>(cameraEntity, Camera2D{
    .target = player_,
    .smoothing = 0.1f,
    .offset = {0.0f, -50.0f}
});

// Engine automatically updates camera from Camera2D component
// No manual CameraSystem needed
```

This approach is documented in the [Sprite Renderer](Sprite-Renderer.md) system.

## See Also

- [Graphics System](Graphics-System.md) - Uses camera for rendering
- [Input System](Input-System.md) - Mouse input needs coordinate conversion
- [Entity System](Entity-System.md) - Entities are camera targets
- [Physics System](Physics-System.md) - Physics positions feed camera
- [Sprite Renderer](Sprite-Renderer.md) - Automatic entity rendering with camera
- Technical Design: `docs/jframe-technical-design.md`

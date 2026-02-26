# Camera System

> **Visibility:** Public (Lua + C++ API)
> **Tier:** 4
> **Dependencies:** Types, Entity
> **Lua Paths:** `bestow.camera` (high-level), `bestow.camera.core` (low-level)

## Purpose

The Camera System manages 2D and 3D camera behavior, providing entity following, smoothing, dead zones, bounds clamping, zoom, shake effects, and multiple camera modes (Free, Follow2D, FPS, ThirdPerson, Orbit, Fixed, Cinematic). The high-level API offers quick entity-following and common camera effects, while the low-level API exposes full 2D and 3D camera control including orbit parameters, FPS mouse look, third-person collision avoidance, FOV flash effects, and direct access to view/projection matrices and frustum data.

## High-Level API: `ICameraSystem`

The simplified API for common camera operations. Provides 2D follow, 3D look-at, shake, zoom, and coordinate conversion. No lifecycle methods -- the engine manages those internally.

### 2D Camera

| Method | Returns | Description |
|--------|---------|-------------|
| `follow(Entity target)` | `void` | Set the camera to follow the given entity in 2D space |
| `setSmoothing(float smoothing)` | `void` | Set the camera follow smoothing factor (0 = instant snap, 1 = very slow) |
| `setOffset(Vec2 offset)` | `void` | Set a fixed offset from the follow target position |
| `setBounds(float minX, float maxX, float minY, float maxY)` | `void` | Clamp camera movement within the given world-space rectangle |

### 3D Camera

| Method | Returns | Description |
|--------|---------|-------------|
| `lookAt(Vec3 from, Vec3 target)` | `void` | Position the camera at `from` and point it toward `target` |
| `setFOV(float degrees)` | `void` | Set the vertical field of view in degrees |

### Effects

| Method | Returns | Description |
|--------|---------|-------------|
| `shake(float intensity, float duration)` | `void` | Apply a camera shake effect with the given intensity and duration in seconds |
| `setZoom(float zoom)` | `void` | Set the 2D camera zoom level (1.0 = default, 2.0 = zoomed in 2x) |

### Queries

| Method | Returns | Description |
|--------|---------|-------------|
| `getPosition2D()` | `Vec2` | Return the current 2D camera position in world space |
| `getPosition3D()` | `Vec3` | Return the current 3D camera position in world space |
| `screenToWorld(Vec2 screenPos)` | `Vec2` | Convert screen-space coordinates to 2D world-space coordinates |

## Low-Level API: `ICameraCore`

Full control API. Exposes complete 2D and 3D camera configuration, multiple camera modes, orbit/FPS/third-person specializations, effects, and matrix access.

### 2D Camera

| Method | Returns | Description |
|--------|---------|-------------|
| `setTarget(Entity target)` | `void` | Set the entity for the 2D camera to follow |
| `clearTarget()` | `void` | Stop following any entity |
| `getTarget()` | `Entity` | Return the currently followed entity, or NullEntity if none |
| `setFollowSmoothing(float smoothing)` | `void` | Set the interpolation factor for following (0 = instant, higher = smoother) |
| `setOffset(Vec2 offset)` | `void` | Set a fixed offset from the follow target |
| `setDeadzone(Vec2 size)` | `void` | Set the dead zone size; the camera does not move when the target is within this rectangle |
| `setBounds(float minX, float maxX, float minY, float maxY)` | `void` | Clamp camera movement within world-space bounds |
| `clearBounds()` | `void` | Remove camera movement bounds |
| `shake(float intensity, float duration)` | `void` | Apply a 2D camera shake effect |
| `stopShake()` | `void` | Immediately stop any active shake effect |
| `setZoom(float zoom)` | `void` | Set the 2D camera zoom level |
| `getZoom()` | `float` | Return the current 2D zoom level |
| `update(DeltaTime dt, Vec2 targetPosition)` | `void` | Advance the 2D camera state; typically called by the engine each frame |
| `getPosition()` | `Vec2` | Return the current 2D camera position in world space |
| `screenToWorld(Vec2 screenPos)` | `Vec2` | Convert screen coordinates to 2D world coordinates |
| `worldToScreen(Vec2 worldPos)` | `Vec2` | Convert 2D world coordinates to screen coordinates |

### 3D Camera

| Method | Returns | Description |
|--------|---------|-------------|
| `setPosition3D(Vec3 position)` | `void` | Set the 3D camera position in world space |
| `getPosition3D()` | `Vec3` | Return the current 3D camera position |
| `setRotation3D(Quat rotation)` | `void` | Set the 3D camera rotation as a quaternion |
| `getRotation3D()` | `Quat` | Return the current 3D camera rotation |
| `lookAt(Vec3 target, Vec3 up = {0,1,0})` | `void` | Orient the camera to look at a world-space target with an up vector |
| `setFOV(float fovDegrees)` | `void` | Set the vertical field of view in degrees |
| `getFOV()` | `float` | Return the current field of view in degrees |
| `setNearFar(float near, float far)` | `void` | Set the near and far clipping plane distances |
| `setProjection(ProjectionType type)` | `void` | Set the projection type (Perspective or Orthographic) |

### Camera Modes

| Method | Returns | Description |
|--------|---------|-------------|
| `setMode(CameraMode mode)` | `void` | Switch to a camera mode (Free, Follow2D, FPS, ThirdPerson, Orbit, Fixed, Cinematic) |
| `getMode()` | `CameraMode` | Return the current camera mode |

### Orbit Camera

| Method | Returns | Description |
|--------|---------|-------------|
| `orbit(float yawDelta, float pitchDelta)` | `void` | Apply yaw and pitch deltas to the orbit camera (in radians) |
| `setOrbitTarget(Vec3 target)` | `void` | Set the point the orbit camera revolves around |
| `setOrbitDistance(float distance)` | `void` | Set the distance from the orbit target |
| `setOrbitLimits(float minPitch, float maxPitch, float minDist, float maxDist)` | `void` | Set pitch angle limits (radians) and distance limits for orbit mode |

### FPS Camera

| Method | Returns | Description |
|--------|---------|-------------|
| `setFPSOffset(Vec3 eyeOffset)` | `void` | Set the eye offset from the entity position in FPS mode |
| `setMouseSensitivity(float sensitivity)` | `void` | Set the mouse look sensitivity for FPS mode |
| `setPitchLimits(float min, float max)` | `void` | Set minimum and maximum pitch angles in radians |

### Third Person Camera

| Method | Returns | Description |
|--------|---------|-------------|
| `setThirdPersonOffset(Vec3 offset)` | `void` | Set the camera offset from the follow target in third-person mode |
| `setThirdPersonDistance(float distance)` | `void` | Set the distance behind the target in third-person mode |
| `enableCollisionAvoidance(bool enabled)` | `void` | Enable or disable automatic camera repositioning to avoid clipping through geometry |

### Effects

| Method | Returns | Description |
|--------|---------|-------------|
| `shake3D(float intensity, float duration)` | `void` | Apply a 3D camera shake effect with the given intensity and duration |
| `flashFOV(float targetFOV, float duration)` | `void` | Briefly change the FOV and smoothly return to the original value (for impact effects) |

### Matrices

| Method | Returns | Description |
|--------|---------|-------------|
| `getViewMatrix()` | `Mat4` | Return the current view matrix |
| `getProjectionMatrix()` | `Mat4` | Return the current projection matrix |
| `getViewProjectionMatrix()` | `Mat4` | Return the combined view-projection matrix |
| `getFrustum()` | `Frustum` | Return the current view frustum (6 planes) for culling |
| `screenToWorldRay(Vec2 screenPos)` | `Ray3D` | Convert a screen-space position to a world-space ray for picking |

## Types

### CameraMode

| Value | Description |
|-------|-------------|
| `Free` | Unconstrained free-flying camera |
| `Follow2D` | 2D entity following with smoothing and dead zones |
| `FPS` | First-person shooter camera attached to an entity |
| `ThirdPerson` | Third-person camera with offset, distance, and collision avoidance |
| `Orbit` | Orbit camera revolving around a target point |
| `Fixed` | Stationary camera at a fixed position and rotation |
| `Cinematic` | Scripted camera for cutscenes and cinematic sequences |

### ProjectionType

| Value | Description |
|-------|-------------|
| `Perspective` | Perspective projection with field of view |
| `Orthographic` | Orthographic projection with parallel rays |

### Camera2D

| Field | Type | Default | Description |
|-------|------|---------|-------------|
| `position` | `Vec2` | `{0,0}` | Camera center position in world space |
| `zoom` | `float` | `1.0f` | Zoom level (1.0 = default) |
| `rotation` | `float` | `0.0f` | Camera rotation in radians |

### Camera3D

| Field | Type | Default | Description |
|-------|------|---------|-------------|
| `transform` | `Transform3D` | `{}` | Camera position, rotation, and scale in world space |
| `projection` | `ProjectionType` | `Perspective` | Projection type |
| `fovY` | `float` | `60.0f` | Vertical field of view in degrees |
| `aspectRatio` | `float` | `16.0f/9.0f` | Width-to-height aspect ratio |
| `orthoWidth` | `float` | `10.0f` | Orthographic view width |
| `orthoHeight` | `float` | `10.0f` | Orthographic view height |
| `nearPlane` | `float` | `0.1f` | Near clipping plane distance |
| `farPlane` | `float` | `1000.0f` | Far clipping plane distance |

## Lua Examples

```lua
-- High-level: 2D platformer camera
bestow.camera.follow(player)
bestow.camera.setSmoothing(0.08)
bestow.camera.setOffset(0, 50)
bestow.camera.setBounds(0, 5000, 0, 2000)
bestow.camera.setZoom(1.5)

-- Shake on damage
bestow.camera.shake(0.5, 0.3)

-- Query position
local pos = bestow.camera.getPosition2D()
local worldPos = bestow.camera.screenToWorld(mouseX, mouseY)

-- High-level: 3D look-at
bestow.camera.lookAt({0, 5, 10}, {0, 0, 0})
bestow.camera.setFOV(70)

-- Low-level: Orbit camera
bestow.camera.core.setMode("Orbit")
bestow.camera.core.setOrbitTarget({0, 1, 0})
bestow.camera.core.setOrbitDistance(8)
bestow.camera.core.setOrbitLimits(-1.2, 1.2, 3, 20)
bestow.camera.core.orbit(dx * 0.005, dy * 0.005)

-- Low-level: FPS camera
bestow.camera.core.setMode("FPS")
bestow.camera.core.setFPSOffset(0, 1.7, 0)
bestow.camera.core.setMouseSensitivity(0.003)
bestow.camera.core.setPitchLimits(-1.4, 1.4)

-- Low-level: Third person with collision
bestow.camera.core.setMode("ThirdPerson")
bestow.camera.core.setThirdPersonOffset(1, 2, 0)
bestow.camera.core.setThirdPersonDistance(5)
bestow.camera.core.enableCollisionAvoidance(true)

-- Impact effect
bestow.camera.core.flashFOV(80, 0.2)

-- Get frustum for manual culling
local frustum = bestow.camera.core.getFrustum()

-- Screen picking
local ray = bestow.camera.core.screenToWorldRay(mouseX, mouseY)
```

## C++ Examples

```cpp
// High-level: Follow a player entity in 2D
camera->follow(playerEntity);
camera->setSmoothing(0.08f);
camera->setOffset({0, 50});
camera->setBounds(0, 5000, 0, 2000);
camera->setZoom(1.5f);

// Shake on hit
camera->shake(0.5f, 0.3f);

// Get world position from screen click
Vec2 worldPos = camera->screenToWorld(mouseScreenPos);

// High-level: 3D
camera->lookAt({0, 5, 10}, {0, 0, 0});
camera->setFOV(70.0f);

// Low-level: Orbit camera mode
cameraCore->setMode(CameraMode::Orbit);
cameraCore->setOrbitTarget({0, 1, 0});
cameraCore->setOrbitDistance(8.0f);
cameraCore->setOrbitLimits(-1.2f, 1.2f, 3.0f, 20.0f);

// Apply mouse drag
cameraCore->orbit(mouseDelta.x * 0.005f, mouseDelta.y * 0.005f);

// FPS camera
cameraCore->setMode(CameraMode::FPS);
cameraCore->setFPSOffset({0, 1.7f, 0});
cameraCore->setMouseSensitivity(0.003f);
cameraCore->setPitchLimits(-1.4f, 1.4f);

// Third person with collision avoidance
cameraCore->setMode(CameraMode::ThirdPerson);
cameraCore->setThirdPersonOffset({1, 2, 0});
cameraCore->setThirdPersonDistance(5.0f);
cameraCore->enableCollisionAvoidance(true);

// FOV punch for impact
cameraCore->flashFOV(80.0f, 0.2f);

// Get view-projection for rendering
Mat4 vp = cameraCore->getViewProjectionMatrix();

// Frustum culling
Frustum frustum = cameraCore->getFrustum();

// Screen-space picking ray
Ray3D ray = cameraCore->screenToWorldRay(mouseScreenPos);
```

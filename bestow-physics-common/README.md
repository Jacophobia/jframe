# bestow-physics-common

Shared physics code between bestow-physics (2D) and bestow-physics3d (3D) systems.

## Purpose

This module eliminates code duplication between the 2D (Box2D) and 3D (Jolt Physics) physics implementations by providing common types, constants, and utilities that are used by both systems.

## Module Contents

### Exported Types

#### Base Enums
- `BodyTypeBase` - Shared body type enum (Static, Kinematic, Dynamic)
- `GroundState` - 2D character ground state (Grounded, Falling, Sliding)
- `CharacterGroundState` - 3D character ground state (OnGround, OnSteepGround, InAir, Sliding)

#### Collision Types
- `CollisionLayer` (uint16_t) - Bitmask category for collision objects
- `CollisionMask` (uint16_t) - Bitmask defining which layers an object collides with
- `CollisionFilterData` - Combined layer, mask, and sensor flag

#### Collision Layer Constants
Standard collision layer definitions in the `CollisionLayers` namespace:
- `All`, `None`, `Default`, `Player`, `Enemy`, `Ground`, `Trigger`, `Projectile`
- `Static`, `Dynamic`, `Character`, `Debris`, `Vehicle`

#### Ground Check Types
- `GroundCheckParams` - Parameters for ground detection raycasts
- `GroundCheckResult2D` - 2D ground check result with state and contact info
- `GroundCheckResult3D` - 3D ground check result with state and contact info

#### Utility Types
- `PhysicsCallback<EventType>` - Generic callback template for physics events

### Physics Constants

The `PhysicsConstants` namespace provides:
- `PixelsPerMeter` (100.0f) - 2D conversion factor
- `MetersPerPixel` (0.01f) - Inverse conversion
- `DefaultSubsteps` (4) - Simulation substep count
- `DefaultGravityY2D` (980.0f) - Default 2D gravity in pixels/s²
- `DefaultGravityY3D` (-9.81f) - Default 3D gravity in m/s²

### Utility Functions

- `pixelsToMeters(float)` - Convert pixels to meters for 2D physics
- `metersToPixels(float)` - Convert meters to pixels for 2D physics
- `shouldCollide(layer, mask, layer, mask)` - Check if two objects should collide
- `isWalkableSlope(angle, maxAngle)` - Check if slope angle is walkable

## Usage

### In Physics Systems

Both `bestow-physics` and `bestow-physics3d` link to this module:

```cmake
target_link_libraries(bestow-physics
    PUBLIC
        bestow-contract
        bestow-physics-common  # Shared physics types
        box2d::box2d
        kangaru
)
```

### In Code

```cpp
import bestow.physics.common;

using namespace bestow;

// Use common collision layers
CollisionFilterData playerFilter{
    .layer = CollisionLayers::Player,
    .mask = CollisionLayers::All & ~CollisionLayers::Trigger,
    .isSensor = false
};

// Use common constants
float pixelDistance = 100.0f;
float meterDistance = pixelsToMeters(pixelDistance);  // 1.0f

// Use ground check utilities
GroundCheckParams params{
    .rayLength = 0.1f,
    .maxSlopeAngle = 45.0f,
    .groundMask = CollisionLayers::Ground
};
```

## Code Deduplication Summary

This module eliminates approximately 400-500 lines of duplicated code between the 2D and 3D physics systems:

### Shared Constants (previously duplicated)
- Collision layer constants
- Physics scale factors
- Default simulation parameters

### Shared Types (previously duplicated)
- `CollisionLayer` and `CollisionMask` type aliases
- `CollisionFilterData` structure
- Ground state enums and check parameters
- Callback type templates

### Shared Utilities (previously duplicated)
- Pixel/meter conversion functions
- Collision filtering logic
- Slope walkability checks

## Dependencies

- **C++23 Standard Library** - via `import std;`
- **GLM** - For `glm::vec2` and `glm::vec3` in ground check results

## Module Declaration

```cpp
export module bestow.physics.common;
```

All types and constants are exported in the `bestow` namespace.

## Build Configuration

Requires:
- CMake 3.28+
- C++23 with module support
- LLVM Clang 20+ on macOS (for `import std;`)

See `/Users/jaaaacob/Documents/GameDev/jframe/bestow-physics-common/CMakeLists.txt` for full build configuration.

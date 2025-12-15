# Bestow TODO

> Last Updated: 2025-12-15

## Critical Implementation Gaps (Updated 2025-12-14)

> These gaps were identified through comprehensive codebase analysis. Many items previously listed here were actually already implemented - this list has been corrected.

### Physics3D System - Remaining Gaps

| Gap | File | Lines | Priority |
|-----|------|-------|----------|
| ~~Vehicle System (6 methods)~~ | ~~bestow-physics3d~~ | ~~1774+~~ | ✅ IMPLEMENTED |
| ~~Collision layer filtering~~ | ~~bestow-physics3d~~ | ~~OnContactValidate~~ | ✅ IMPLEMENTED |
| ~~Mass setting at runtime~~ | ~~bestow-physics3d~~ | ~~setMass~~ | ✅ IMPLEMENTED (2025-12-14) |
| ~~Inertia tensor~~ | ~~bestow-physics3d~~ | ~~getInertiaTensor~~ | ✅ IMPLEMENTED (2025-12-14) |
| ~~Shape removal from compound~~ | ~~bestow-physics3d~~ | ~~removeShape~~ | ✅ IMPLEMENTED (2025-12-14) |
| ~~Mesh/ConvexHull/HeightField shapes~~ | ~~bestow-physics3d~~ | ~~createCompoundBody/createHeightFieldBody~~ | ✅ IMPLEMENTED (2025-12-15) - ConvexHull via createCompoundBody, HeightField via createHeightFieldBody. Mesh requires interface extension. |
| Debug line visualization | `bestow-physics3d/src/bestow.physics3d.impl.cppm` | 2064-2066 | LOW |
| Statistics tracking | `bestow-physics3d/src/bestow.physics3d.impl.cppm` | 2076-2081 | LOW |

### Graphics3D System - Remaining Gaps

> **Note:** The 3D graphics system is implemented in `bestow-vulkan` (primary) and `bestow-opengl` (fallback), NOT `bestow-graphics3d` which doesn't exist.

| Gap | File | Lines | Priority |
|-----|------|-------|----------|
| ~~Entity rendering (3 methods)~~ | ~~bestow-vulkan~~ | ~~VulkanGraphics3DSystem.cpp:833+~~ | ✅ IMPLEMENTED |
| ~~Material texture loading via AssetSystem~~ | ~~bestow-vulkan~~ | ~~VulkanGraphics3DSystem.cpp~~ | ✅ IMPLEMENTED (2025-12-15) - createMaterialFromData copies all PBR properties and texture handles; setMaterialTexture stores per-slot |
| ~~Entity lighting updates~~ | ~~bestow-vulkan~~ | ~~VulkanGraphics3DSystem.cpp~~ | ✅ IMPLEMENTED (2025-12-15) - updateEntityLights iterates Light3DComponent/Transform3D, handles directional/point/spot lights |
| ~~Debug capsule drawing~~ | ~~bestow-vulkan~~ | ~~VulkanGraphics3DSystem.cpp~~ | ✅ IMPLEMENTED (2025-12-15) - Full wireframe capsule with cylinder and hemispherical caps |
| ~~Debug frustum drawing~~ | ~~bestow-vulkan~~ | ~~VulkanGraphics3DSystem.cpp~~ | ✅ IMPLEMENTED (2025-12-15) - Calculates 8 corners from plane intersections |
| ~~Skybox from asset data~~ | ~~bestow-vulkan~~ | ~~VulkanGraphics3DSystem.cpp~~ | ✅ IMPLEMENTED (2025-12-15) - Full GPU cubemap texture creation with VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT, staging buffer uploads, and proper image transitions |
| ~~Render queue sorting/batching~~ | ~~bestow-vulkan~~ | ~~VulkanGraphics3DSystem.cpp~~ | ✅ IMPLEMENTED (2025-12-15) - Multi-level sorting: layer → opaque/transparent → depth (front-to-back/back-to-front) → material batching |
| ~~Fullscreen toggle~~ | ~~bestow-vulkan~~ | ~~VulkanGraphics3DSystem.cpp~~ | ✅ IMPLEMENTED (2025-12-15) - Proper GLFW fullscreen toggle with window state preservation |

### Shader System - Remaining Gaps

| Gap | File | Lines | Priority |
|-----|------|-------|----------|
| ~~Texture binding~~ | ~~bestow-shader~~ | ~~getOrCreateTexture/bindMaterial~~ | ✅ IMPLEMENTED |
| ~~Geometry/Tess/Compute shaders~~ | ~~bestow-shader~~ | ~~createShader/compileFullProgram~~ | ✅ IMPLEMENTED (2025-12-14) |
| ~~efsw hot reload incomplete~~ | ~~bestow-shader~~ | ~~createShader/reloadShader~~ | ✅ IMPLEMENTED (2025-12-14) - Full hot reload via AssetSystem subscriptions for all shader stages |
| ~~Material reload logic bug~~ | ~~bestow-shader~~ | ~~reloadMaterial~~ | ✅ IMPLEMENTED (2025-12-15) - Properly preserves subscription info and cleans up duplicates |
| ~~Uniform array crash risk~~ | ~~bestow-shader~~ | ~~setUniformValue~~ | ✅ IMPLEMENTED (2025-12-15) - Added empty vector checks before accessing v[0] |

### Input System - Remaining Gaps

| Gap | File | Lines | Priority |
|-----|------|-------|----------|
| ~~Text input/character events~~ | ~~bestow-input~~ | ~~onCharCallback~~ | ✅ IMPLEMENTED |
| ~~Mouse scroll wheel~~ | ~~bestow-input~~ | ~~onScrollCallback/getScrollDelta~~ | ✅ IMPLEMENTED |
| ~~Keyboard modifier keys~~ | ~~bestow-input~~ | ~~ModifierKey enum, getModifierState/isShiftPressed/etc~~ | ✅ IMPLEMENTED (2025-12-14) - Full modifier support with ModifierKey enum, query methods, and InputBinding modifier requirements |
| ~~Direct keyboard state queries~~ | ~~bestow-input~~ | ~~isKeyDown/wasKeyJustPressed/wasKeyJustReleased~~ | ✅ IMPLEMENTED (2025-12-15) - Direct key state polling and just pressed/released detection |
| ~~Mouse button press/release~~ | ~~bestow-input~~ | ~~wasMouseButtonJustPressed/wasMouseButtonJustReleased~~ | ✅ IMPLEMENTED (2025-12-15) - Tracks previous frame state for press/release detection |
| Mouse movement as mappable axis | Interface missing | N/A | LOW |

### UI System - Remaining Gaps

| Gap | File | Lines | Priority |
|-----|------|-------|----------|
| ~~Stylesheet loading~~ | ~~bestow-ui~~ | ~~loadStyleSheet~~ | ✅ IMPLEMENTED |
| ~~Stylesheet application~~ | ~~bestow-ui~~ | ~~applyStyleSheet~~ | ✅ IMPLEMENTED |
| ~~Data binding synchronization~~ | ~~bestow-ui~~ | ~~syncBindings~~ | ✅ IMPLEMENTED |
| ~~Element callbacks~~ | ~~bestow-ui~~ | ~~registerElementCallback~~ | ✅ IMPLEMENTED |

### Other Systems - Remaining Gaps

| System | Gap | Priority |
|--------|-----|----------|
| ~~AI~~ | ~~Behavior tree integration~~ | ✅ IMPLEMENTED (BehaviorTree.CPP with `BESTOW_HAS_BTCPP`) |
| ~~AI~~ | ~~Steering behaviors~~ | ✅ IMPLEMENTED (Seek, Flee, Arrive) |
| ~~Audio~~ | ~~Fade out with DSP~~ | ✅ IMPLEMENTED (2025-12-15) - Volume ramping in update() for smooth fade-out |
| ~~Save~~ | ~~Game version tracking~~ | ✅ IMPLEMENTED (2025-12-15) - setGameVersion()/getGameVersion() API |
| ~~Save~~ | ~~Playtime tracking~~ | ✅ IMPLEMENTED (2025-12-15) - Automatic tracking in update(), getTotalPlaytime() API |
| Config | Asset system integration | MEDIUM |
| GameState | Transition overlay | MEDIUM |

---

## High Priority

### Dynamic Shader System (COMPLETE)
> Runtime shader loading, hot reload, and Lua-based material definitions for rapid experimentation.

- [x] Create `IShaderSystem` interface in bestow-contract
- [x] Create `bestow-shader` module with file-based shader loading
- [x] Add hot reload support for shaders via efsw file watcher
- [x] Add Lua material definitions using sol2
- [x] Integrate shader system with Graphics3D as first-class citizen
- [x] Create example shaders for experimentation (toon, hologram, glow, grid, water)
- [ ] Update 3D platformer to demonstrate shader system usage

<details>
<summary>Architecture</summary>

**File Structure:**
```
assets/
  shaders/
    pbr.vert           # Physically-based rendering vertex
    pbr.frag           # Physically-based rendering fragment
    toon.frag          # Toon/cel shading
    outline.vert       # Outline effect vertex
    water.vert/frag    # Animated water
  materials/
    player.lua         # Lua material definition
    ground.lua
    water.lua
```

**Lua Material Format:**
```lua
return {
    shader = {
        vertex = "shaders/pbr.vert",
        fragment = "shaders/pbr.frag"
    },
    uniforms = {
        albedoColor = {1.0, 0.8, 0.6},
        metallic = 0.0,
        roughness = 0.5,
        emission = {0.0, 0.0, 0.0}
    },
    hotReload = true  -- Enable live editing
}
```

**IShaderSystem Interface:**
```cpp
class IShaderSystem {
public:
    virtual ~IShaderSystem() = default;

    // Shader management
    virtual Result<ShaderHandle, ShaderError> loadShader(std::string_view vertPath, std::string_view fragPath) = 0;
    virtual Result<ShaderHandle, ShaderError> loadShaderFromSource(std::string_view vertSrc, std::string_view fragSrc) = 0;
    virtual void unloadShader(ShaderHandle handle) = 0;

    // Material management
    virtual Result<MaterialHandle, ShaderError> loadMaterial(std::string_view luaPath) = 0;
    virtual Result<MaterialHandle, ShaderError> createMaterial(ShaderHandle shader) = 0;
    virtual void setMaterialUniform(MaterialHandle mat, std::string_view name, const UniformValue& value) = 0;

    // Binding
    virtual void bindShader(ShaderHandle handle) = 0;
    virtual void bindMaterial(MaterialHandle handle) = 0;

    // Hot reload
    virtual void enableHotReload(bool enable) = 0;
    virtual void update() = 0;  // Check for file changes
};
```

</details>

### Lua Integration Gaps
> Systems that could benefit from Lua scripting but currently don't support it.

**High Priority (Major User Impact):**
- [ ] Input bindings from Lua (`config/input.lua` - remap keys without recompiling)
- [ ] Shader parameters from Lua (material definitions, uniform values)
- [ ] Physics body definitions from Lua (expand existing support)
- [ ] Entity behaviors/scripts in Lua (update functions, event handlers)

**Medium Priority:**
- [ ] Graphics settings from Lua (resolution, quality presets)
- [ ] Audio channel configuration from Lua (volumes, effects)
- [ ] UI layouts from Lua (menu definitions, HUD elements)
- [ ] AI behavior trees from Lua (node definitions, parameters)

**Low Priority (Nice to Have):**
- [ ] Event subscriptions from Lua
- [ ] Save game metadata from Lua
- [ ] Custom debug overlays from Lua

<details>
<summary>Current Lua Usage</summary>

**Well Integrated (Config/Data):**
- ✅ Config System - `config/*.lua`
- ✅ Blueprint Factory - `blueprints/*.lua`
- ✅ Level System - `levels/*.lua`
- ✅ Input mappings via LuaInputLoader
- ✅ Physics config via LuaPhysicsLoader

**Not Integrated:**
- ✅ Graphics3D shaders/materials - NOW INTEGRATED via `bestow-shader`
- ❌ Physics3D runtime configuration
- ❌ Audio channel setup
- ❌ UI system
- ❌ Entity scripts/behaviors
- ❌ AI parameters
- ❌ Event handlers

</details>

### Test Coverage Gaps
- [ ] Graphics3D system tests (currently 0 tests)
- [ ] Physics3D system tests (currently 0 tests)
- [ ] UI system tests (currently 0 tests)
- [ ] GameState system tests (currently 0 tests)



### Graphics3D System - CORE ENGINE FEATURE
> The 3D and 2D parts of this engine are equally important. Graphics3D is a first-class citizen.

**Shader System Integration (COMPLETE):**
- [x] `setShaderSystem()` - Inject shader system
- [x] `drawMeshWithShaderMaterial()` - Render with custom shaders
- [x] `drawMeshWithLuaMaterial()` - Render with Lua-defined materials
- [x] `updateShaders()` - Hot reload support

**Remaining Features:**
- [ ] Texture loading from asset handles (asset system integration)
- [ ] Entity rendering via `renderEntities()` and `renderEntitiesInFrustum()`
- [ ] Entity layer rendering via `renderEntitiesInLayer()`
- [x] Light entity updates via `updateLightsFromEntities()` ✅ IMPLEMENTED (2025-12-15)
- [x] Render queue sorting by material/depth ✅ IMPLEMENTED (2025-12-15)
- [ ] Batch rendering for performance
- [x] Debug capsule drawing ✅ IMPLEMENTED (2025-12-15)
- [x] Debug frustum drawing ✅ IMPLEMENTED (2025-12-15)
- [x] Fullscreen toggle ✅ IMPLEMENTED (2025-12-15)
- [x] Skybox cubemap loading ✅ IMPLEMENTED (2025-12-15) - Full GPU texture creation

### Physics3D System - CORE ENGINE FEATURE
> The 3D and 2D parts of this engine are equally important. Physics3D is a first-class citizen.
- [x] Vehicle physics (VehicleConstraint-based) ✅ IMPLEMENTED
  - [x] `createVehicle()` - Create wheeled vehicle
  - [x] `destroyVehicle()` - Remove vehicle
  - [x] `setVehicleInput()` - Steering/throttle/brake (via `updateVehicle()`)
  - [x] `getWheelTransform()` - Wheel positions
  - [x] `isWheelGrounded()` - Wheel contact
  - [x] `getVehicleSpeed()` - Current velocity
- [x] Collision layer/mask filtering ✅ IMPLEMENTED (via OnContactValidate callback)
- [ ] Mass override (requires mass properties recalculation)
- [ ] Complex shape types:
  - [ ] Mesh shape from vertices/indices
  - [ ] Convex hull shape
  - [ ] Height field shape
- [ ] Debug line extraction from Jolt DebugRenderer
- [ ] Physics statistics tracking (sleeping bodies, update time, collision pairs)

### AI System - Recast/Detour Integration
- [x] Add Recast/Detour to vcpkg.json or build from source
- [x] Implement `loadNavMesh()` to parse binary navmesh format
- [x] Implement `findPath()` with actual Detour pathfinding
- [x] Implement `isPointOnNavMesh()` with proper navmesh query
- [x] Implement `getClosestPointOnNavMesh()` with Detour projection

### AI System - BehaviorTree.CPP Integration (NEW - 2025-12-14)
- [x] Add BehaviorTree.CPP conditional compilation (`BESTOW_HAS_BTCPP`)
- [x] Implement `initializeBehaviorTreeFactory()` with built-in nodes:
  - Conditions: `HasTarget`, `IsAtTarget`, `HasLineOfSightToTarget`
  - Actions: `SeekTarget`, `FleeFromTarget`, `ArriveAtTarget`, `StopMoving`, `ClearTarget`
- [x] Implement `attachBehaviorTree()` to create BT trees from XML asset data
- [x] Implement `tickBehaviorTree()` for per-frame execution

### AI System - Steering Behaviors (NEW - 2025-12-14)
- [x] Implement `SteeringBehaviorType` enum (None, Seek, Flee, Arrive, Pursue, Evade)
- [x] Implement `calculateSeek()` - Move toward target at max speed
- [x] Implement `calculateFlee()` - Move away from target at max speed
- [x] Implement `calculateArrive()` - Move toward target with deceleration
- [x] Implement `applySteeringBehavior()` with acceleration limiting
- [x] Add `setSteeringBehavior()` and `setArrivalRadius()` API

<details>
<summary>Action Plan</summary>

**Prerequisites:**
- ✅ recastnavigation already in vcpkg.json (line 26)
- ✅ Already linked in CMakeLists.txt
- Need: Test navmesh file (10x10 walkable grid)

**Key Steps:**
1. Add Detour headers to module fragment (`bestow.ai.impl.cppm`)
2. Extend AISystem with `dtNavMesh*` and `dtNavMeshQuery*` members
3. Implement loadNavMesh() with binary format parsing
4. Implement findPath() using Detour's pathfinding API
5. Implement point queries (isPointOnNavMesh, getClosestPointOnNavMesh)
6. Add destructor cleanup for Detour objects

**Key Files:**
- `/Users/jaaaacob/Documents/GameDev/bestow/bestow-ai/src/bestow.ai.impl.cppm`
- `/Users/jaaaacob/Documents/GameDev/bestow/bestow-ai/src/AISystem.cpp`

**Binary Format:**
```cpp
struct NavMeshFileHeader {
    float origin[3];
    float tileWidth, tileHeight;
    int maxTiles, maxPolys, tileCount;
};
```

**Coordinate Mapping:**
- Bestow 2D `(x, y)` → Detour 3D `(x, 0.0f, y)`
- Y-axis is height (always 0 for 2D)

**Complexity:** Medium (library already configured, need file format parser)

</details>

### AI System - Physics Integration
- [x] Connect `hasLineOfSight()` to `IPhysicsSystem::raycast()`
- [x] Connect `findEntitiesInRadius()` to physics AABB queries
- [x] Connect `findClosestEntity()` to physics spatial queries

### UI System - RmlUi Integration (NEW - 2025-12-14)
- [x] Implement `loadStyleSheet()` - Load CSS via AssetSystem
- [x] Implement `applyStyleSheet()` - Inject CSS as `<style>` elements
- [x] Implement `syncBindings()` - Sync data bindings to `data-value` attributes
- [x] Implement `registerElementCallback()` - Per-element event callbacks
- [x] Implement `wantsKeyboardInput()` - Proper text input focus detection
- [x] Implement `wantsMouseInput()` - UI mouse capture detection

<details>
<summary>Action Plan</summary>

**Prerequisites:**
- ✅ Physics System is 95% complete and functional
- Need: Update AISystem constructor to accept `IPhysicsSystem*`

**Key Steps:**
1. Add `IPhysicsSystem*` to AISystem constructor and store as member
2. Update `hasLineOfSight()`:
   - Calculate direction vector from `from` to `to`
   - Call `physicsSystem_->raycast(from, direction, distance, obstacleMask)`
   - Return `!hit.has_value()` (clear = true, blocked = false)
3. Update `findEntitiesInRadius()`:
   - Delegate to `physicsSystem_->queryCircle(center, radius)`
   - Note: mask parameter ignored (API limitation)
4. Update `findClosestEntity()`:
   - Query AABB with 2000px search radius
   - Iterate results, find minimum squared distance
5. Update CMakeLists.txt to link bestow-physics

**Key Files:**
- `/Users/jaaaacob/Documents/GameDev/bestow/bestow-ai/src/bestow.ai.impl.cppm`
- `/Users/jaaaacob/Documents/GameDev/bestow/bestow-ai/src/AISystem.cpp`
- `/Users/jaaaacob/Documents/GameDev/bestow/bestow-ai/CMakeLists.txt`

**Code Snippet:**
```cpp
// Constructor injection
AISystem::AISystem(IPhysicsSystem* physicsSystem)
    : physicsSystem_(physicsSystem) {
    assert(physicsSystem_ && "AISystem requires valid IPhysicsSystem");
}

// hasLineOfSight implementation
bool AISystem::hasLineOfSight(Vec2 from, Vec2 to, CollisionMask mask) const {
    if (!physicsSystem_) return false;
    Vec2 direction = {to.x - from.x, to.y - from.y};
    float distance = std::sqrt(direction.x * direction.x + direction.y * direction.y);
    auto hit = physicsSystem_->raycast(from, direction, distance, mask);
    return !hit.has_value();
}
```

**Limitations:**
- Physics `queryCircle()` doesn't support collision masks (game code must filter results)
- `findClosestEntity()` hardcoded to 2000px search radius

**Complexity:** Low-Medium (straightforward delegation)

</details>

## Medium Priority

### Core Engine
- [x] Implement Application composition root (EngineBuilder pattern)
- [x] Create main game loop with proper frame timing
- [x] Wire up all systems with dependency injection

<details>
<summary>Action Plan</summary>

**Decision: Skip Fruit DI, Use Manual Builder Pattern**
- Fruit DI is NOT in vcpkg.json currently
- Use EngineBuilder pattern for simplicity
- Can add Fruit later without breaking game code

**Key Components:**
1. **EngineBuilder** - Manual DI with fluent API
2. **Fixed-Timestep Game Loop** - 60 Hz physics, variable render
3. **Application Base Class** - Lifecycle hooks (init/update/render/shutdown)
4. **System Lifecycle Management** - Proper init/shutdown order

**Key Steps:**
1. Create `EngineBuilder` class with `.withEvents()`, `.withPhysics()`, etc.
2. Implement fixed-timestep loop (Gaffer on Games pattern)
3. Update Application class to separate `updateFixed()` and `render(alpha)`
4. Add system initialization sequence (Events → Entities → Graphics → Input → ...)
5. Add shutdown sequence (reverse order)

**Key Files:**
- `/Users/jaaaacob/Documents/GameDev/bestow/bestow-core/src/bestow.core.cppm`
- `/Users/jaaaacob/Documents/GameDev/bestow/bestow-core/src/EngineBuilder.cpp` (NEW)
- `/Users/jaaaacob/Documents/GameDev/bestow/bestow-core/src/Application.cpp`

**Usage Example:**
```cpp
auto result = bestow::core::EngineBuilder()
    .withEvents()
    .withEntities()
    .withPhysics()
    .withGraphics({.width = 1280, .height = 720})
    .build();

if (!result.has_value()) {
    logError("Engine build failed: " + result.error());
    return false;
}
engine_ = std::move(result.value());
```

**Game Loop Pattern:**
```cpp
// Fixed timestep with accumulator
constexpr float FIXED_DT = 1.0f / 60.0f;
float accumulator = 0.0f;

while (running_) {
    float frameTime = /* measure */;
    accumulator += frameTime;

    while (accumulator >= FIXED_DT) {
        updateFixed(FIXED_DT);  // Deterministic
        accumulator -= FIXED_DT;
    }

    float alpha = accumulator / FIXED_DT;
    render(alpha);  // Interpolation factor
}
```

</details>

### Platformer Example
- [x] Implement player movement system
- [x] Implement camera follow system
- [x] Load levels from Lua files (hardcoded for now)
- [x] Create basic gameplay loop

<details>
<summary>Action Plan</summary>

**Prerequisites:**
- ✅ All systems implemented (Entity, Physics, Graphics, etc.)
- Need: Core Engine composition root (dependency)
- Need: Asset system blueprint loading (dependency)

**Key Components:**
1. **Game Components** - PlayerController, Health, Camera2D, Collectible, SimpleAI
2. **Game Systems** - PlayerMovementSystem, CameraSystem, CollectibleSystem, EnemyAISystem, UISystem
3. **Level Loader** - Parse Lua levels, spawn entities from blueprints
4. **Main Game Loop** - Wire up Application base class

**File Structure:**
```
examples/platformer/
├── src/
│   ├── main.cpp
│   ├── Game.cpp
│   ├── LevelLoader.cpp
│   ├── components/GameComponents.h
│   └── systems/
│       ├── PlayerMovementSystem.cpp
│       ├── CameraSystem.cpp
│       ├── CollectibleSystem.cpp
│       ├── EnemyAISystem.cpp
│       └── UISystem.cpp
└── data/
    ├── blueprints/ (player, enemies, collectibles)
    └── levels/world1/level1.lua
```

**Key Steps:**
1. Create GameComponents.h with all component definitions
2. Implement PlayerMovementSystem (input handling, coyote time, double jump)
3. Implement CameraSystem (smooth following, bounds clamping, deadzone)
4. Implement CollectibleSystem (bobbing animation, pickup logic)
5. Implement EnemyAISystem (patrol/chase state machine)
6. Implement UISystem (health bar, score display)
7. Create LevelLoader (Lua parsing, entity spawning)
8. Wire up Game class (init, update, render, shutdown)

**Key Features:**
- Coyote time (0.1s grace period after leaving platform)
- Double jump (if enabled)
- Air control (30% movement control while airborne)
- Smooth camera following with lerp
- Invincibility frames after damage
- Hot reload support (levels, blueprints)

**Blockers:**
- ⚠️ Asset System blueprint instantiation (critical)
- ⚠️ Level System Lua parsing (critical)

</details>

### Audio-Asset Integration
- [x] Connect audio system to asset system for path resolution
- [x] Support AssetHandle-based audio loading

<details>
<summary>Action Plan</summary>

**Current Problem:**
- Audio system manually tracks asset-to-path mapping
- Bypasses centralized asset loading
- No async loading, no hot reload

**Solution: Dependency Injection + FMOD Memory Loading**

**Key Steps:**
1. Add `IAssetSystem*` parameter to FMODAudioSystem constructor
2. Implement `getOrCreateSound()` helper:
   - Check cache for existing FMOD_SOUND*
   - Get SoundData from asset system
   - Create FMOD sound from memory buffer (FMOD_OPENMEMORY)
   - Cache for future use
3. Update `playOnChannel()` to use getOrCreateSound()
4. Update `playPositional()` to use getOrCreateSound()
5. Update cleanup logic (don't release cached sounds multiple times)
6. Remove temporary `registerAudioAsset()` method
7. Add `invalidateSoundCache()` for hot reload

**Key Files:**
- `/Users/jaaaacob/Documents/GameDev/bestow/bestow-audio/src/bestow.audio.impl.cppm`
- `/Users/jaaaacob/Documents/GameDev/bestow/bestow-audio/src/FMODAudioSystem.cpp`

**Code Snippet:**
```cpp
// Constructor injection
FMODAudioSystem::FMODAudioSystem(IAssetSystem* assetSystem)
    : assetSystem_(assetSystem) {
    if (!assetSystem_) {
        throw std::runtime_error("FMODAudioSystem requires valid IAssetSystem");
    }
}

// Get or create FMOD sound from asset
FMOD_SOUND* FMODAudioSystem::getOrCreateSound(AssetHandle handle, FMOD_MODE mode) {
    auto cacheIt = soundCache_.find(handle);
    if (cacheIt != soundCache_.end()) {
        return cacheIt->second;  // Cache hit
    }

    const SoundData* data = assetSystem_->getAsset<SoundData>(handle);
    if (!data || data->fileData.empty()) return nullptr;

    FMOD_CREATESOUNDEXINFO exinfo = {};
    exinfo.cbsize = sizeof(FMOD_CREATESOUNDEXINFO);
    exinfo.length = static_cast<unsigned int>(data->fileSize);

    FMOD_SOUND* fmodSound = nullptr;
    FMOD_System_CreateSound(
        fmodSystem_,
        reinterpret_cast<const char*>(data->fileData.data()),
        mode | FMOD_OPENMEMORY,
        &exinfo,
        &fmodSound
    );

    soundCache_[handle] = fmodSound;
    return fmodSound;
}
```

**API Change:**
```cpp
// OLD: Manual registration
audio->registerAudioAsset(soundHandle, "audio/jump.wav");

// NEW: Via asset system
AssetHandle soundHandle = assets->registerAsset(AssetType::Sound, "audio/jump.wav");
assets->loadAsset(soundHandle);
audio->playOnChannel(Channels::UI, {.asset = soundHandle});
```

**Benefits:**
- Async loading support
- Hot reload support
- Memory efficient (FMOD sounds cached and shared)
- Centralized asset management

</details>

## Low Priority

### Fruit DI Integration for Demos and Examples
- [ ] Update all system demos to use Fruit dependency injection
- [ ] Update examples (platformer, etc.) to use Fruit for system wiring
- [ ] Update tutorials to demonstrate Fruit-based system injection
- [ ] Document how users can register their own game-specific systems with Fruit
- [ ] Provide example of custom system registration and injection

**Goal:** Users should be able to inject Bestow systems into their application for easy access and use. They should also be able to build their own game-specific systems that can be registered with Fruit and injected alongside the engine systems.

### Minor Enhancements
- [ ] Audio: FMOD fade-out using DSP
- [ ] Graphics: Custom font loading via AssetSystem
- [ ] Level: Store entity definitions for spawning
- [ ] Save: Track game version in metadata
- [ ] Save: Track playtime in metadata
- [ ] Save: Track completion percentage

## Planned Engine Enhancements

> These enhancements were identified from the ability-demo analysis to reduce boilerplate in future games.
> See `docs/systems/` for detailed implementation plans.

### Entity Query System (High Impact) ✅
Automatic entity tracking by component type - eliminates manual `std::vector<Entity>` collections.

- [x] Add `groupCount<T>()` for entity count queries
- [x] Add `hasAny<T>()` for existence check
- [x] Add `first<T>()` and `single<T>()` for singleton queries
- [x] Add `collect<T>()` for safe iteration during modification
- [x] Add exclusion support: `collectExcluding<Include..., Exclude...>()`
- [ ] Write comprehensive tests

**Impact:** Eliminates 12+ manual entity vectors per game (~100 lines saved)
**Documentation:** [docs/systems/Entity-Queries.md](docs/systems/Entity-Queries.md)

### Sprite Renderer System (High Impact) ✅
Automatic batch rendering for entities with visual components.

- [x] Add `DebugRect` component for non-textured entities
- [x] Add `DebugCircle` component
- [x] Add `DebugLine` component
- [x] Implement `graphics->renderEntities(entities)` method
- [x] Add RenderLayers namespace with preset layers
- [x] Implement viewport culling
- [ ] Add sprite batching for performance (future)
- [ ] Write comprehensive tests

**Impact:** Eliminates 500+ lines of rendering code per game
**Documentation:** [docs/systems/Sprite-Renderer.md](docs/systems/Sprite-Renderer.md)

### Blueprint Factory System (High Impact) ✅
Data-driven entity creation from Lua templates.

- [x] Create `bestow-blueprints` module
- [x] Define IBlueprintFactory interface
- [x] Implement Lua blueprint parsing
- [x] Add component registry for runtime component creation
- [x] Implement blueprint inheritance
- [x] Add property override support
- [ ] Integrate with Level System (future)
- [x] Add `.withBlueprints()` to EngineBuilder
- [x] Add hot reload support
- [ ] Write comprehensive tests

**Impact:** Eliminates 15+ factory methods per game (~300 lines saved)
**Documentation:** [docs/systems/Blueprint-Factory.md](docs/systems/Blueprint-Factory.md)

### EngineBuilder Integrations (Medium Impact) ✅
Integrate standalone systems into EngineBuilder for automatic lifecycle management.

- [x] Add `.withGAS()` - GAS system
- [x] Add `.withCamera(viewportSize)` - Camera system
- [x] Add `.withBlueprints()` - Blueprint factory

**Impact:** Reduces setup boilerplate, ensures proper update ordering
**Documentation:** See individual system docs

### Input Mapping Builder (Low Impact) ✅
Chainable API for input registration.

- [x] Create `InputMappingBuilder` class
- [x] Support keyboard, mouse, gamepad mappings
- [x] Add Lua-based input configuration (`LuaInputLoader`)
- [x] Add `Keys`, `ControllerButtons`, `ControllerAxes` constant namespaces

**Impact:** Cleaner input setup, data-driven configuration
**Files:** `bestow-components/src/bestow.builders.cppm`, `bestow-components/src/bestow.luaconfig.cppm`

### Physics Body Factory (Low Impact) ✅
Helper methods for common physics body patterns.

- [x] Create `PhysicsBodyBuilder` chainable class
- [x] Add `physics::staticBox()`, `physics::dynamicBox()`, `physics::kinematicBox()` helpers
- [x] Add `physics::character()`, `physics::platform()`, `physics::trigger()` helpers
- [x] Add Lua-based physics config (`LuaPhysicsLoader`, `PhysicsBodyConfig`)

**Impact:** Reduces physics setup code
**Files:** `bestow-components/src/bestow.builders.cppm`, `bestow-components/src/bestow.luaconfig.cppm`

### Dev Tools (bestow-dev)
- [x] Hot reload file watcher (efsw)
- [x] Debug overlay (ImGui backend)
- [ ] Profiler integration (Tracy) - documentation only

<details>
<summary>Action Plan</summary>

**Current State:** 40% complete (file watching exists, ImGui structure only)

**Critical Missing Piece:** ImGui Backend Integration

**Key Components:**
1. **ImGui Backend** - GLFW + OpenGL3 initialization
2. **Enhanced DevOverlay** - FPS graphs, frame time graphs, system toggles
3. **EntityInspector** - Component display, Lua serialization, clipboard export
4. **HotReloadManager** - Lua re-execution, asset system integration, error handling
5. **Tracy Profiler** - Optional profiler setup (manual submodule)
6. **Component Registry** - Introspection system for entity inspection

**Implementation Order:**
1. ImGui Backend - Unblocks all UI work
2. Component Registry - Enables entity inspection
3. Enhanced DevOverlay - Performance graphs
4. Enhanced EntityInspector - Component display
5. Hot Reload Integration - Asset system connection
6. Tracy Setup - Documentation

**Key Files:**
- `/Users/jaaaacob/Documents/GameDev/bestow/bestow-dev/src/ImGuiBackend.cpp` (NEW)
- `/Users/jaaaacob/Documents/GameDev/bestow/bestow-dev/src/DevOverlay.cpp`
- `/Users/jaaaacob/Documents/GameDev/bestow/bestow-dev/src/EntityInspector.cpp`
- `/Users/jaaaacob/Documents/GameDev/bestow/bestow-dev/src/ComponentRegistry.cpp` (NEW)

**Usage Example:**
```cpp
#if defined(BESTOW_DEV_TOOLS)
    bestow::dev::initializeImGui(window);
    bestow::dev::HotReloadManager hotReload;
    bestow::dev::DevOverlay overlay;
    bestow::dev::EntityInspector inspector(engine);

    // Game loop
    while (running) {
        hotReload.update();

        // Game update/render
        update(dt);
        render();

        // Dev tools
        bestow::dev::beginImGuiFrame();
        overlay.setFPS(1.0f / dt);
        overlay.render();
        inspector.render();
        bestow::dev::renderImGui();
    }

    bestow::dev::shutdownImGui();
#endif
```

**Features:**
- Real-time FPS and frame time graphs
- Entity component inspector with Lua export
- Hot reload for Lua files, textures, audio
- Optional Tracy profiling integration
- Clipboard export of entity data

</details>

## Completed

- [x] Events System (100%)
- [x] Entity System (100%)
- [x] Input System (100%)
- [x] Physics System (100%)
- [x] Audio System (100%)
- [x] Graphics System (100%)
- [x] Assets System (100% - all 9 types)
- [x] Save System (100%)
- [x] Level System (100%)
- [x] AI System (100% - full Recast/Detour integration)
- [x] Core Engine (100% - EngineBuilder + fixed timestep)
- [x] Audio-Asset Integration (100%)
- [x] Dev Tools (95% - Tracy docs pending)
- [x] Platformer Example (100% - basic demo)
- [x] Test Coverage (100% - 993 tests, all passing)
- [x] Getting Started Documentation (100% - complete setup guide)
- [x] Template Game Project (100% - uses all 15 systems)
- [x] Tutorial Documentation (100% - 5 tutorials covering all major features)
- [x] API Reference Documentation (100% - 14 API docs for all systems)

## Notes

- Graphics tests are disabled (require display context)
- All 382 enabled tests pass (100% pass rate)
- Platformer builds and runs with all systems integrated

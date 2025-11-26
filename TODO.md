# JFrame TODO

> Last Updated: 2025-11-25

## High Priority

### AI System - Recast/Detour Integration
- [ ] Add Recast/Detour to vcpkg.json or build from source
- [ ] Implement `loadNavMesh()` to parse binary navmesh format
- [ ] Implement `findPath()` with actual Detour pathfinding
- [ ] Implement `isPointOnNavMesh()` with proper navmesh query
- [ ] Implement `getClosestPointOnNavMesh()` with Detour projection

<details>
<summary>Action Plan (3-5 days)</summary>

**Prerequisites:**
- ✅ recastnavigation already in vcpkg.json (line 26)
- ✅ Already linked in CMakeLists.txt
- Need: Test navmesh file (10x10 walkable grid)

**Key Steps:**
1. Add Detour headers to module fragment (`jframe.ai.impl.cppm`)
2. Extend AISystem with `dtNavMesh*` and `dtNavMeshQuery*` members
3. Implement loadNavMesh() with binary format parsing
4. Implement findPath() using Detour's pathfinding API
5. Implement point queries (isPointOnNavMesh, getClosestPointOnNavMesh)
6. Add destructor cleanup for Detour objects

**Key Files:**
- `/Users/jaaaacob/Documents/GameDev/jframe/jframe-ai/src/jframe.ai.impl.cppm`
- `/Users/jaaaacob/Documents/GameDev/jframe/jframe-ai/src/AISystem.cpp`

**Binary Format:**
```cpp
struct NavMeshFileHeader {
    float origin[3];
    float tileWidth, tileHeight;
    int maxTiles, maxPolys, tileCount;
};
```

**Coordinate Mapping:**
- JFrame 2D `(x, y)` → Detour 3D `(x, 0.0f, y)`
- Y-axis is height (always 0 for 2D)

**Complexity:** Medium (library already configured, need file format parser)

</details>

### AI System - Physics Integration
- [ ] Connect `hasLineOfSight()` to `IPhysicsSystem::raycast()`
- [ ] Connect `findEntitiesInRadius()` to physics AABB queries
- [ ] Connect `findClosestEntity()` to physics spatial queries

<details>
<summary>Action Plan (2-3 hours)</summary>

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
5. Update CMakeLists.txt to link jframe-physics

**Key Files:**
- `/Users/jaaaacob/Documents/GameDev/jframe/jframe-ai/src/jframe.ai.impl.cppm`
- `/Users/jaaaacob/Documents/GameDev/jframe/jframe-ai/src/AISystem.cpp`
- `/Users/jaaaacob/Documents/GameDev/jframe/jframe-ai/CMakeLists.txt`

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
- [ ] Implement Application composition root with Fruit DI
- [ ] Create main game loop with proper frame timing
- [ ] Wire up all systems with dependency injection

<details>
<summary>Action Plan (3-4 days)</summary>

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
- `/Users/jaaaacob/Documents/GameDev/jframe/jframe-core/src/jframe.core.cppm`
- `/Users/jaaaacob/Documents/GameDev/jframe/jframe-core/src/EngineBuilder.cpp` (NEW)
- `/Users/jaaaacob/Documents/GameDev/jframe/jframe-core/src/Application.cpp`

**Usage Example:**
```cpp
auto result = jframe::core::EngineBuilder()
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

**Estimated Effort:** 28 hours (~3.5 days)

</details>

### Platformer Example
- [ ] Implement player movement system
- [ ] Implement camera follow system
- [ ] Load levels from Lua files
- [ ] Create basic gameplay loop

<details>
<summary>Action Plan (3-4 days)</summary>

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

**Estimated Effort:** 28 hours (~3.5 days)

**Blockers:**
- ⚠️ Asset System blueprint instantiation (critical)
- ⚠️ Level System Lua parsing (critical)

</details>

### Audio-Asset Integration
- [ ] Connect audio system to asset system for path resolution
- [ ] Support AssetHandle-based audio loading

<details>
<summary>Action Plan (~3 hours)</summary>

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
- `/Users/jaaaacob/Documents/GameDev/jframe/jframe-audio/src/jframe.audio.impl.cppm`
- `/Users/jaaaacob/Documents/GameDev/jframe/jframe-audio/src/FMODAudioSystem.cpp`

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

**Estimated Effort:** 3 hours (2h coding, 30m testing, 30m docs)

</details>

## Low Priority

### Enhancements
- [ ] Audio: FMOD fade-out using DSP
- [ ] Graphics: Custom font loading via AssetSystem
- [ ] Level: Store entity definitions for spawning
- [ ] Save: Track game version in metadata
- [ ] Save: Track playtime in metadata
- [ ] Save: Track completion percentage

### Dev Tools (jframe-dev)
- [ ] Hot reload file watcher (efsw)
- [ ] Debug overlay (ImGui backend)
- [ ] Profiler integration (Tracy)

<details>
<summary>Action Plan (~27 hours / 3.5 days)</summary>

**Current State:** 40% complete (file watching exists, ImGui structure only)

**Critical Missing Piece:** ImGui Backend Integration (4 hours)

**Key Components:**
1. **ImGui Backend** - GLFW + OpenGL3 initialization
2. **Enhanced DevOverlay** - FPS graphs, frame time graphs, system toggles
3. **EntityInspector** - Component display, Lua serialization, clipboard export
4. **HotReloadManager** - Lua re-execution, asset system integration, error handling
5. **Tracy Profiler** - Optional profiler setup (manual submodule)
6. **Component Registry** - Introspection system for entity inspection

**Implementation Order:**
1. ImGui Backend (4h) - Unblocks all UI work
2. Component Registry (4h) - Enables entity inspection
3. Enhanced DevOverlay (3h) - Performance graphs
4. Enhanced EntityInspector (4h) - Component display
5. Hot Reload Integration (4h) - Asset system connection
6. Tracy Setup (2h) - Documentation

**Key Files:**
- `/Users/jaaaacob/Documents/GameDev/jframe/jframe-dev/src/ImGuiBackend.cpp` (NEW)
- `/Users/jaaaacob/Documents/GameDev/jframe/jframe-dev/src/DevOverlay.cpp`
- `/Users/jaaaacob/Documents/GameDev/jframe/jframe-dev/src/EntityInspector.cpp`
- `/Users/jaaaacob/Documents/GameDev/jframe/jframe-dev/src/ComponentRegistry.cpp` (NEW)

**Usage Example:**
```cpp
#if defined(JFRAME_DEV_TOOLS)
    jframe::dev::initializeImGui(window);
    jframe::dev::HotReloadManager hotReload;
    jframe::dev::DevOverlay overlay;
    jframe::dev::EntityInspector inspector(engine);

    // Game loop
    while (running) {
        hotReload.update();

        // Game update/render
        update(dt);
        render();

        // Dev tools
        jframe::dev::beginImGuiFrame();
        overlay.setFPS(1.0f / dt);
        overlay.render();
        inspector.render();
        jframe::dev::renderImGui();
    }

    jframe::dev::shutdownImGui();
#endif
```

**Features:**
- Real-time FPS and frame time graphs
- Entity component inspector with Lua export
- Hot reload for Lua files, textures, audio
- Optional Tracy profiling integration
- Clipboard export of entity data

**Estimated Effort:** 27 hours (~3.5 days)

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
- [x] AI System (95% - navmesh stubbed)

## Notes

- Graphics tests are disabled (require display context)
- AI navmesh uses distance-based heuristics as fallback
- All 378 enabled tests pass (100% pass rate)
- Action plans extracted from `/tmp/jframe-plan-*.md` files

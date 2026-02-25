# V2 Critical Review: Where We're Overfitting to V1 Mistakes

> **Date:** 2026-02-25
> **Purpose:** Honest assessment of architectural flaws the v2 proposal would perpetuate
> **Context:** The v2 contract docs (SYSTEM-ARCHITECTURE.md, V2-CONTRACT-ARCHITECTURE.md) were written by analyzing the current system and projecting forward. This document asks: are we polishing a flawed foundation instead of fixing it?

---

## The Core Thesis

**The v2 proposal is an incremental improvement on a fundamentally flawed architecture.** It adds new systems (Particles, Network, Tween), reorganizes existing ones (dual API split), and fixes the documentation pipeline. But it does not address the deep structural problems that will continue to cause pain:

1. A DI container that provides no value over manual wiring
2. God interfaces that mix abstraction levels
3. An ECS abstraction that is half-implemented and leaky
4. Handle types with zero compile-time safety
5. Pervasive `std::any` destroying type safety
6. A game loop with no flexibility
7. Inconsistent error handling, callback, and lifecycle patterns

**The v2 proposal perpetuates all seven of these problems while adding 200+ new methods on top of them.**

---

## Flaw 1: The DI Container Is Solving a Non-Problem

### What's Wrong Now

The `bestow-di` system (`ServiceCollection` -> `ServiceProvider`) uses `std::type_index`, `void*`, factory lambdas, and runtime resolution to wire systems together. In `GameRunner.cpp`, the actual wiring looks like:

```cpp
engine_.use<IAssetSystem, AssetSystem>([](di::ServiceProvider& sp) {
    return new AssetSystem(sp.get<IEventSystem>());
});
```

This is strictly worse than:

```cpp
auto* events = new EventSystem();
auto* assets = new AssetSystem(events);
```

The container provides:
- **Runtime type lookup** where compile-time would work
- **Implicit ordering** that is fragile (registrations must happen in dependency order)
- **Lazy resolution** that hides errors until `engine_.build()`
- **The `HasInitialize` concept** that auto-calls `initialize()` on some systems but not others (SceneSystem needs `initialize(&lua_)` post-build, AnimationSystem needs manual `initialize()`)
- **Destruction order managed by member declaration order** in GameRunner (comments say "DO NOT REORDER")

### What V2 Proposes (Overfitting)

V2 doubles down on this pattern. The architecture doc shows the same DI registration for new systems:

```cpp
engine.use<IAudioCore, FMODAudioSystem>([](di::ServiceProvider& sp) {
    return new FMODAudioSystem(sp.get<IAssetCore>());
});
engine.use<IAudioSystem, FMODAudioSystem>([](di::ServiceProvider& sp) {
    return &static_cast<FMODAudioSystem&>(sp.get<IAudioCore>());
});
```

Now every system registers **twice** (Core + System). Same fragile ordering. Same runtime type erasure. Double the registration boilerplate.

### The Alternative

**Replace the DI container with a typed SystemContext struct:**

```cpp
struct SystemContext {
    // Tier 1
    std::unique_ptr<EventSystem> events;

    // Tier 2
    std::unique_ptr<AssetSystem> assets;
    std::unique_ptr<ConfigSystem> config;
    std::unique_ptr<EntitySystem> entity;

    // Tier 3
    std::unique_ptr<InputSystem> input;
    std::unique_ptr<AudioSystem> audio;
    std::unique_ptr<PhysicsSystem> physics2d;
    std::unique_ptr<PhysicsSystem3D> physics3d;
    std::unique_ptr<AnimationSystem> animation;
    // ...
};

SystemContext buildDefaultSystems() {
    SystemContext ctx;
    ctx.events = std::make_unique<EventSystem>();
    ctx.assets = std::make_unique<AssetSystem>(ctx.events.get());
    ctx.config = std::make_unique<ConfigSystem>(ctx.assets.get(), ctx.events.get());
    ctx.entity = std::make_unique<EntitySystem>();
    ctx.input = std::make_unique<InputSystem>(ctx.events.get(), ctx.assets.get());
    // ... explicit, typed, compile-time checked
    return ctx;
}
```

**Benefits:**
- Compile-time type safety (no `void*`, no `std::type_index`)
- Dependency order is visible and enforced (can't use `ctx.assets` before creating it)
- Errors happen at compile time, not runtime
- No registration order fragility
- Destruction order is deterministic (reverse of unique_ptr declaration)
- No factory lambdas to debug
- A developer can see every dependency at a glance

**For client extensibility** (the reason DI supposedly exists), use a builder:
```cpp
SystemContextBuilder builder;
builder.replaceAudio<MyCustomAudioSystem>();  // Type-checked replacement
auto ctx = builder.build();
```

**This is simpler, safer, and faster.** The v2 proposal should adopt this.

---

## Flaw 2: God Interfaces (Graphics3D Has ~250 Virtual Methods)

### What's Wrong Now

`IGraphics3DSystem` has methods for:
- Mesh management, primitive factories, materials
- Camera, lighting, environment, shadows
- Post-processing, debug rendering, window management
- Instanced rendering, LOD, culling, statistics
- **Skeletal animation rendering** (bone matrices, skinned mesh)
- **Lock-on targeting** (gameplay logic!)
- **3D text rendering**
- **Runtime config management**

`IPhysics3DSystem` has methods for:
- Rigid bodies, shape management, forces
- Raycasting, shape casting, overlap queries
- **Constraints** (hinge, slider, cone, point, distance)
- **Character controllers** (gameplay!)
- **Vehicle physics** (domain-specific gameplay!)
- Contact queries, debug draw, statistics

### What V2 Proposes (Overfitting)

V2 keeps `IGraphics3DCore` at 65+ methods and `IGraphics3DSystem` at 18 methods. This is better (the facade is smaller), but the Core interface is still a god object. Adding compute shaders, more post-processing, and new rendering features will push it past 80+ methods again.

V2 moves lock-on targeting to Camera. Good. But vehicles and character controllers stay in Physics3D. Skeletal rendering stays in Graphics3D.

### The Alternative

**Split along natural seam lines:**

```
IGraphics3DCore (mesh, material, camera, lighting, window)  ~35 methods
IPostProcessing (bloom, SSAO, DOF, tone mapping, AA)        ~12 methods
IDebugRenderer (lines, boxes, spheres, frustums, clear)      ~10 methods
IShadowRenderer (shadow maps, resolution, distance)           ~5 methods
ITextRenderer3D (fonts, draw text, measure)                   ~5 methods
```

```
IPhysics3DCore (bodies, shapes, forces, queries)             ~40 methods
IConstraintSystem (hinge, slider, cone, distance)            ~12 methods
ICharacterController (create, move, grounded, velocity)       ~8 methods
IVehiclePhysics (create, update, wheels, speed)               ~8 methods
```

**Benefits:**
- Each interface has a single responsibility
- Smaller vtables (better cache behavior in hot paths)
- A game without vehicles doesn't pay for `IVehiclePhysics`
- Can mock individual subsystems in tests
- Clear ownership: character controller is gameplay, not physics primitives

**This is the Interface Segregation Principle.** V2 should embrace it instead of perpetuating the monolith.

---

## Flaw 3: The ECS Abstraction Is the Worst of Both Worlds

### What's Wrong Now

`IEntitySystem` wraps EnTT behind an interface. But:

1. **Type-erased methods are stubbed.** `addComponent()`, `getComponent()`, `hasComponent()` return null. Tests assert they return null. Dead code.

2. **Typed methods bypass the interface.** All rendering systems call `entities.view<Transform, Sprite>()` directly through EnTT templates. The contract boundary is violated immediately.

3. **Two separate component registries.** `IEntitySystem` has `registerComponentType()` for Lua. `dev::ComponentRegistry` has a separate registration for the inspector. They don't share data.

4. **`getRegistry()` returns the raw `entt::registry&`.** Any system can access the implementation directly. The interface provides zero encapsulation.

5. **Missing ECS features:** No component change detection, no entity hierarchies, no proper tag support (empty structs with dummy `bool _` member), no thread safety, no serialization.

### What V2 Proposes (Overfitting)

V2 adds `IEntitySystem` (high-level) and `IEntityCore` (low-level) with an object pooling API. But it doesn't fix:
- The stubbed type-erased methods
- The leaky `getRegistry()` access
- The dual registration systems
- The missing change detection
- The dead query API

### The Alternative

**Choose one path and commit:**

**Option A: EnTT-native (recommended for a Lua-first engine)**

Don't abstract the ECS. Use EnTT directly as a first-class dependency. Build the Lua bridge as a reflection layer on top:

```cpp
// No IEntitySystem. Just EnTT.
entt::registry registry;

// Lua bridge does the type-erasure work properly:
class LuaEntityBridge {
    entt::registry& reg_;
    std::unordered_map<std::string, ComponentMetadata> registered_;

public:
    Entity create() { return reg_.create(); }
    void addComponent(Entity e, std::string_view name, sol::table data);
    sol::table getComponent(Entity e, std::string_view name);
    void setField(Entity e, std::string_view comp,
                  std::string_view field, sol::object value);
    // ... properly implemented type-erased access for Lua
};
```

**Benefits:**
- C++ systems get full EnTT performance (views, groups, change detection)
- Lua gets proper type-erased access through a dedicated bridge
- No false abstraction layer
- One registration system (the bridge)
- Change detection works through EnTT's native `on_update<T>()`

**Option B: Full abstraction (if you truly want to swap ECS libraries)**

Implement every method properly. No stubs. No `getRegistry()` escape hatch. Type-erased query API that actually works. This is vastly more work and loses EnTT's best features.

**V2 should choose Option A.** The engine is Lua-first. C++ systems are internal. There is no real need to swap ECS libraries behind an interface.

---

## Flaw 4: Handle Soup (Zero Compile-Time Safety)

### What's Wrong Now

Every graphics handle is `using XxxHandle = std::uint64_t`:

```cpp
using MeshHandle = std::uint64_t;
using MaterialHandle = std::uint64_t;
using SkeletonHandle = std::uint64_t;
using AnimationClipHandle = std::uint64_t;
using Font3DHandle = std::uint64_t;
using InstanceBufferHandle = std::uint64_t;
using SoundHandle = std::uint64_t;
```

This compiles without error:
```cpp
graphics->drawMesh(materialHandle, meshHandle, transform);  // SWAPPED - no error
```

### What V2 Proposes (Overfitting)

V2 adds `EmitterHandle`, `ParticleEffectHandle`, `TweenHandle`, `NetworkId`, `ClientId`, `RPCId` -- all as `uint32_t` or `uint64_t`. More handles, same lack of type safety.

### The Alternative

**Use strong types:**

```cpp
template<typename Tag>
struct Handle {
    std::uint64_t id = 0;
    bool isValid() const { return id != 0; }
    auto operator<=>(const Handle&) const = default;
};

// Each is a distinct type:
using MeshHandle = Handle<struct MeshTag>;
using MaterialHandle = Handle<struct MaterialTag>;
using SoundHandle = Handle<struct SoundTag>;
using EmitterHandle = Handle<struct EmitterTag>;

// This now fails to compile:
graphics->drawMesh(materialHandle, meshHandle, transform);  // ERROR: type mismatch
```

**Cost:** Zero runtime overhead (same `uint64_t` underneath). **Benefit:** Entire class of bugs eliminated at compile time.

**V2 MUST adopt this.** It's the cheapest fix with the highest impact.

---

## Flaw 5: `std::any` Everywhere

### What's Wrong Now

- **Scene parameters:** `std::unordered_map<std::string, std::any>` -- game developers must guess types
- **Material uniforms:** `std::unordered_map<std::string, std::any>` -- type errors at runtime
- **Asset data:** `std::any jsonData` -- opaque blob
- **Component data:** `ComponentFieldValue` is a variant (better, but still 10+ types)

### What V2 Proposes (Overfitting)

V2 doesn't address this. Scene parameters still use `std::any`. The Particle and Network systems avoid it (good), but existing systems keep it.

### The Alternative

**Use typed variants or Lua tables directly:**

For scene parameters, since this is a Lua engine:
```cpp
// Instead of std::any:
virtual void pushScene(const std::string& name, sol::table params = sol::nil) = 0;

// Lua naturally handles heterogeneous data:
bestow.scene.push("level_1", { difficulty = 3, enemies = "hard" })
```

For material uniforms:
```cpp
// Typed uniform variant with explicit types:
using UniformValue = std::variant<float, Vec2, Vec3, Vec4, Mat4, int, Color>;
std::unordered_map<std::string, UniformValue> uniforms;
```

For asset data:
```cpp
// Use the type system, not std::any:
struct DataAsset {
    nlohmann::json data;  // Just use the JSON type directly
};
```

**V2 should eliminate all `std::any` usage.** Every instance is a type safety hole.

---

## Flaw 6: The Game Loop Has No Flexibility

### What's Wrong Now

`GameRunner::runGameLoop()` has a hardcoded update order:

```
1. Input.update()
2. ScriptManager.update()
3. updateTimers(dt)
4. StateSystem.update(dt)
5. AnimationSystem.update(dt)
6. SceneSystem.update(dt)
7. app.main.update(dt)
8. Graphics.beginFrame()
9. SceneSystem.render()
10. UI.update() + UI.render()
11. app.main.render()
12. Graphics.endFrame()
13. sleep(1ms)
```

Problems:
- **No fixed timestep.** Physics runs at variable rate. Non-deterministic simulation.
- **`sleep(1ms)` at the end.** Causes input latency. Wrong for any vsync/adaptive sync setup.
- **Lua update at step 7** but physics presumably runs inside scene update or Lua. Order isn't clear.
- **No separation between fixed-rate (physics) and variable-rate (rendering).**
- **If a Lua error occurs in step 7, what happens?** The entire frame is in an undefined state.
- **Hot reload at step 2** can change game state that steps 3-13 depend on. Race condition.

### What V2 Proposes (Overfitting)

V2 inserts new systems into the existing fixed order (Tween at step 6, Network at step 7, Particles at step 15). Same rigid structure. More dependencies on the order being correct.

### The Alternative

**Use a phase-based update scheduler:**

```cpp
enum class UpdatePhase {
    EarlyUpdate,    // Input polling, network receive, hot reload
    FixedUpdate,    // Physics, simulation (runs at fixed timestep)
    Update,         // Game logic, AI, animation, tweens
    LateUpdate,     // Camera, particles, things that depend on positions
    PreRender,      // Culling, render queue building
    Render,         // Drawing
    PostRender,     // UI, debug overlay, frame present
};

class UpdateScheduler {
public:
    void registerSystem(UpdatePhase phase, int priority,
                        std::function<void(DeltaTime)> update);
    void run(DeltaTime dt);  // Executes all phases in order
};
```

```cpp
// Systems self-register into phases:
scheduler.registerSystem(UpdatePhase::EarlyUpdate, 0, [&](DeltaTime dt) {
    input->update(dt);
});
scheduler.registerSystem(UpdatePhase::FixedUpdate, 0, [&](DeltaTime dt) {
    physics->update(dt);
});
scheduler.registerSystem(UpdatePhase::Update, 10, [&](DeltaTime dt) {
    animation->update(dt);
});
```

And the core loop:
```cpp
void gameLoop() {
    float accumulator = 0.0f;
    constexpr float fixedDt = 1.0f / 60.0f;

    while (!shouldClose()) {
        float frameDt = frameTimer.delta();

        // Variable-rate phases
        scheduler.runPhase(UpdatePhase::EarlyUpdate, frameDt);

        // Fixed timestep for physics
        accumulator += frameDt;
        while (accumulator >= fixedDt) {
            scheduler.runPhase(UpdatePhase::FixedUpdate, fixedDt);
            accumulator -= fixedDt;
        }

        // Variable-rate phases
        scheduler.runPhase(UpdatePhase::Update, frameDt);
        scheduler.runPhase(UpdatePhase::LateUpdate, frameDt);
        scheduler.runPhase(UpdatePhase::PreRender, frameDt);
        scheduler.runPhase(UpdatePhase::Render, frameDt);
        scheduler.runPhase(UpdatePhase::PostRender, frameDt);
    }
}
```

**Benefits:**
- Fixed timestep for physics (deterministic simulation)
- Systems can be reordered without modifying GameRunner
- New systems register themselves; no hardcoded list to maintain
- Lua callbacks hook into specific phases
- Clear phase boundaries for error recovery

---

## Flaw 7: Inconsistent Everything

### Error Handling (3 patterns)

| System | Pattern | Can game dev catch errors? |
|--------|---------|---------------------------|
| Graphics3D | `Result<T, Graphics3DError>` | Yes |
| Assets | `AssetHandle` (no error) | No |
| Physics | `void` (silent fail) | No |
| Scene | `Result<void, std::error_code>` | Yes, different type |
| Entity | `bool` | Sort of |

### Callbacks (3 patterns)

| System | Pattern | Can unsubscribe? |
|--------|---------|-------------------|
| Events | `SubscriptionId` from `subscribe()` | Yes |
| Assets | `SubscriptionId` from `subscribe()` | Yes |
| Physics | Direct `std::function` set | **No** (leak risk) |

### Resource Lifecycle (3 patterns)

| System | Create | Destroy | Extra steps |
|--------|--------|---------|-------------|
| Physics | `createBody()` | `destroyBody()` | None |
| Graphics | `createMesh()` | `destroyMesh()` | None |
| Assets | `registerAsset()` | `unregisterAsset()` | + `loadAsset()` / `unloadAsset()` |
| Scene | `registerScene()` | `unregisterScene()` | + `pushScene()` / `popScene()` |

### What V2 Should Mandate

**One error pattern:**
```cpp
// Every fallible operation returns Result:
Result<MeshHandle, SystemError> createMesh(const MeshDef&);
Result<void, SystemError> createBody(Entity, const PhysicsBodyDef&);
Result<AssetHandle, SystemError> registerAsset(AssetType, const std::string&);
```

**One callback pattern:**
```cpp
// Every subscription returns an ID and supports unsubscribe:
SubscriptionId onCollision(std::function<void(const CollisionEvent&)>);
void unsubscribe(SubscriptionId);
```

**One lifecycle pattern:**
```cpp
// Create returns handle. Destroy takes handle. No extra load/unload steps.
Result<Handle, Error> create(const Def&);
void destroy(Handle);
```

---

## Flaw 8: Leaky Abstractions (Vulkan Types in Contracts)

### What's Wrong Now

`IGraphicsContext` exposes:
```cpp
virtual void* getRenderContext() const = 0;      // "Returns VkContext"
virtual void* getCurrentCommandBuffer() const = 0; // "Returns VkCommandBuffer"
```

These comments literally name Vulkan types. The `void*` return forces consumers to cast to Vulkan-specific types. The entire contract-based architecture is undermined.

### What V2 Should Do

**Remove backend-specific methods from contracts.** If a system needs Vulkan access, it goes through a Vulkan-specific extension interface:

```cpp
// Contract (vendor-agnostic):
class IGraphicsContext {
    virtual Size getWindowSize() const = 0;
    virtual void* getNativeWindowHandle() const = 0;
    virtual bool isInFrame() const = 0;
};

// Extension (Vulkan-specific, NOT in bestow-contract):
class IVulkanContext : public IGraphicsContext {
    virtual VkCommandBuffer getCurrentCommandBuffer() const = 0;
    virtual VkDevice getDevice() const = 0;
};
```

The extension lives in `bestow-vulkan`, not `bestow-contract`. Only systems that need Vulkan import it.

---

## Summary: What the V2 Proposal Gets Wrong

| V2 Proposal | Problem | What To Do Instead |
|-------------|---------|-------------------|
| Same DI container, double registration | Adds complexity, keeps fragility | Typed SystemContext struct |
| `IGraphics3DCore` at 65+ methods | Still a god interface | Split into 5 focused interfaces |
| `IEntitySystem` + `IEntityCore` | Perpetuates broken abstraction | Use EnTT directly + Lua bridge |
| New handles as `uint32_t`/`uint64_t` | Same type safety hole | Strong typed handles |
| `std::any` not addressed | Type safety holes remain | Eliminate with variants or sol::table |
| Systems inserted into fixed game loop | Same rigid ordering | Phase-based update scheduler |
| 3 error patterns carried forward | Inconsistent error handling | Unified `Result<T, SystemError>` |
| Vulkan types still in contracts | Leaky abstractions | Vendor extensions outside contracts |
| Lock-on moves to Camera | Good | Keep this |
| Self-updating docs | Good | Keep this |
| Dual API (System + Core) | Good concept | Keep, but Core should be split interfaces not monolith |

---

## What To Keep From V2

The v2 proposal has genuinely good ideas that should survive this critique:

1. **Dual API (System + Core)** -- The concept of a simple facade over a full API is sound. But the Core should not be another god interface.

2. **Self-updating documentation** -- The SystemDefinition + static_assert + doc-tests pipeline is excellent. Keep it entirely.

3. **New systems (Particles, Network, Tween)** -- These fill real gaps. Design them from scratch with the corrections above.

4. **Lock-on to Camera** -- Correct separation of concerns.

5. **Shader extraction** -- Good infrastructure split.

---

## Flaw 9: Physics Is Not In The Game Loop

### What's Wrong Now

The game loop in `GameRunner::runGameLoop()` calls `Input.update()`, `Animation.update()`, `Scene.update()`, and Lua `app.main.update()`. **It never calls `Physics.update()`.** Neither `IPhysicsSystem` nor `IPhysics3DSystem` are updated.

Physics bodies exist but never step through time. A developer calling `physics.createBody()` followed by `physics.getVelocity()` will always get `(0,0)`.

This is documented in `TODO.md` as a known issue but never implemented.

### What V2 Proposes (Overfitting)

V2 shows physics at steps 10-12 in the game loop. Good on paper. But the v2 game loop is still hardcoded, still uses variable `dt`, and still has no accumulator pattern. Physics gets a slot but not a correct one.

---

## Flaw 10: Variable Timestep Breaks Physics

### What's Wrong Now

Every system receives the same variable `dt`:
```cpp
float dt = std::chrono::duration<float>(now - lastTime).count();
```

Box2D gets this directly:
```cpp
void Box2DPhysicsSystem::update(DeltaTime dt) {
    b2World_Step(worldId_, dt, SUB_STEP_COUNT);
}
```

This causes frame-rate-dependent physics, instability at high framerates, missed collisions, and non-reproducible bugs. The accumulator pattern is documented in TODO.md but **not implemented**.

---

## Flaw 11: Lua Bindings Are an 8,960-Line Maintenance Trap

### What's Wrong Now

21 binding files + 21 doc files = 42 files, ~8,960 lines. 60-70% is boilerplate:
- Manual enum registration (repeated across files)
- Lambda wrappers for every method
- Manual type checking (`input.is<KeyCode>()`, `input.is<MouseButton>()`, ...)
- Silent failures everywhere (wrong type -> `nil`, no error message)

When a contract changes, 4 files need updating (contract, binding, binding doc, stubs). Forgetting any one causes silent drift.

### What V2 Proposes (Partially Good)

V2's SystemDefinition files eliminate the binding/doc duplication. Good. But the binding code itself is still manual. With the dual API (System + Core), every system now needs **two sets of bindings** -- doubling the lambda boilerplate.

### The Alternative

**Code generation from contracts.** A tool that parses `.cppm` interfaces and generates:
- sol2 binding code (lambda wrappers, enum registrations, usertypes)
- EmmyLua stubs
- CLI help text
- Doc registry entries

The generator is the single source of truth. Human-written binding code drops to near zero -- only custom conversion logic (like `parseInputBinding`) remains hand-written. This is ~2-3 days of work that eliminates thousands of lines and makes the dual API free.

---

## Flaw 12: Silent Failures Everywhere

### What's Wrong Now

Across all bindings, errors are swallowed:

| Binding | Failure | Result in Lua |
|---------|---------|---------------|
| `parseInputBinding(wrongType)` | Wrong type passed | Returns `KeyCode::Unknown` silently |
| `luaToFieldValue(unknownType)` | Unknown type | Returns `monostate` silently |
| `config.getFloat("missing")` | Key not found | Returns `nil` -- same as "key exists but is nil" |
| `graphics3d.createMesh(badData)` | Creation fails | Returns `nil` -- error discarded |
| Timer callback on dead table | Hot reload invalidated table | Logs warning, callback never fires again |

An AI agent or game developer has **no way to distinguish** between "this returned nil because the value is nil" and "this returned nil because something failed." This is the most impactful usability problem for AI agents.

### The Alternative

**Two-value returns** (the Lua convention):
```lua
local value, err = bestow.config.getFloat("player.speed")
if err then
    print("Config error: " .. err)
end
```

Or **explicit error functions** in the contract:
```lua
local mesh = bestow.graphics3d.createMesh(data)
if not mesh then
    local err = bestow.graphics3d.getLastError()
    print("Mesh creation failed: " .. err)
end
```

Either pattern is fine. The current "nil means anything" pattern is not.

---

## What The Contract System Gets RIGHT

**The v2 proposal correctly preserves these architectural strengths:**

1. **Immutable contracts** -- Systems depend only on interfaces. An implementation can be rewritten from scratch without touching any peer system. This is the engine's greatest architectural strength.

2. **Peer dependency isolation** -- A system that depends on `IAssetSystem` works with ANY asset implementation. This enables parallel agent development: each agent owns one system and only reads contracts.

3. **Clear ownership boundaries** -- `bestow-physics/` owns physics. `bestow-audio/` owns audio. No cross-contamination. An agent working on audio never needs to read physics code.

4. **Testability through contracts** -- Mock any dependency by implementing its interface. Unit test a system in complete isolation.

### The Missing Piece: Protected vs Public Contracts

The contract system needs a **visibility layer**:

| Visibility | Exposed in Lua? | Who Consumes? | Example |
|------------|-----------------|---------------|---------|
| **Public** | Yes (`bestow.audio.*`) | Game developers | `IAudioSystem`, `IInputSystem` |
| **Protected** | No | Other engine systems only | `IAssetSystem`, `IShaderSystem`, `IUIRenderBackend` |
| **Internal** | No | Same system only | `IVulkanPipeline`, `IFMODChannel` |

**Protected contracts** solve the leaky abstraction problem. `IGraphicsContext` needs `getRenderContext()` returning Vulkan types? Fine -- it's a protected contract. Game developers never see it. Only the UI render backend and dev tools use it. The contract is still well-defined and immutable, but it doesn't pollute the public API.

**Internal contracts** live inside an implementation. `IVulkanPipeline` is defined in `bestow-vulkan/`, not in `bestow-contract/`. Only Vulkan code sees it. It can change freely because only its own system consumes it.

This three-tier visibility means:
- **Public contracts** are maximally stable, clean, and documented
- **Protected contracts** can expose implementation details safely (they're still immutable interfaces, just not public)
- **Internal contracts** are implementation-specific and freely changeable

### How This Changes the Architecture

```
bestow-contract/
  src/
    public/                    ← Exposed to Lua, documented, stable
      bestow.audio.cppm        ← IAudioSystem (high-level)
      bestow.entity.cppm       ← IEntitySystem
      bestow.input.cppm        ← IInputSystem
      ...

    protected/                 ← Used by peer systems, not Lua
      bestow.audio.core.cppm   ← IAudioCore (full FMOD exposure)
      bestow.assets.cppm       ← IAssetSystem (file gateway)
      bestow.shader.cppm       ← IShaderSystem (compilation)
      bestow.graphics.context.cppm  ← IGraphicsContext (with Vulkan types OK here)
      bestow.uirender.cppm     ← IUIRenderBackend
      bestow.metrics.cppm      ← MetricsCollector
      ...

bestow-vulkan/
  src/
    internal/                  ← Only Vulkan implementation sees these
      VulkanPipeline.h
      VulkanSwapchain.h
      ...
```

This maps directly to the dual API from the v2 proposal:
- `IAudioSystem` (public) -> `bestow.audio.*` in Lua
- `IAudioCore` (protected) -> `bestow.audio.core.*` in Lua (optional exposure for power users)
- Vulkan internals (internal) -> never in Lua

The key insight: **Core contracts don't HAVE to be exposed in Lua.** Some should be (audio effects, physics constraints -- power users want these). Others should not (asset caching internals, shader compilation details, UI render backend).

---

## Revised V2 Recommendation Order

1. **Strong typed handles** -- Cheapest fix, highest impact. Do this first.
2. **Eliminate `std::any`** -- Replace with variants or sol::table.
3. **Establish public/protected/internal contract visibility** -- Before splitting interfaces.
4. **Split god interfaces** -- Graphics3D into 5 (public + protected), Physics3D into 4.
5. **Replace DI with typed SystemContext** -- Simplify the composition root.
6. **Fix the game loop** -- Fixed timestep accumulator + phase-based scheduling + actually call physics.
7. **Standardize error/callback/lifecycle patterns** -- One of each.
8. **ECS: choose EnTT-native + Lua bridge** -- Stop pretending to abstract.
9. **Build binding code generator** -- Eliminate 60-70% of binding boilerplate.
10. **Eliminate silent failures** -- Two-value returns or explicit error functions in Lua.
11. **Then add new systems** (Particles, Network, Tween) using all the above patterns.
12. **Then implement self-updating docs** -- On the clean foundation.

Doing these in order means each new system built is built correctly from the start, rather than being built on top of known architectural flaws.

# Bestow System Architecture

> **Date:** 2026-02-25
> **Purpose:** Complete system inventory (current + planned), dependency graph, and classification
> **Companion:** See `BESTOW-COMPETITIVE-ANALYSIS.md` for the gap analysis that motivates new systems

---

## Table of Contents

1. [Architecture Overview](#architecture-overview)
2. [System Classification](#system-classification)
3. [Dependency Tiers](#dependency-tiers)
4. [Infrastructure Systems (Higher-Order)](#infrastructure-systems-higher-order)
5. [User-Facing Systems](#user-facing-systems)
6. [New Systems (Gap Closers)](#new-systems-gap-closers)
7. [Dependency Graph](#dependency-graph)
8. [Game Loop Integration](#game-loop-integration)
9. [Contract Outlines for New Systems](#contract-outlines-for-new-systems)

---

## Architecture Overview

Bestow's architecture is built on two organizing principles:

1. **Contract-Based DI** -- Every system implements an interface (`I*System`) and receives dependencies through constructor injection. Systems never reference implementations.

2. **Two audiences** -- Systems serve either *other systems* (infrastructure) or *game developers* (user-facing). This distinction determines whether a system needs Lua bindings, documentation depth, and error message quality.

### System Count

| Category | Current | New (Planned) | Total |
|----------|---------|---------------|-------|
| Infrastructure (Higher-Order) | 11 | 1 | 12 |
| User-Facing | 15 | 3 | 18 |
| Integration Layer | 4 | 0 | 4 |
| **Total** | **30** | **4** | **34** |

---

## System Classification

### What "Higher-Order" Means

A **higher-order system** is one whose primary consumers are other engine systems, not game code. Game developers may occasionally interact with these through Lua, but the core value is enabling other systems.

**The test:** If you removed all Lua bindings for this system, would game developers notice? If not, it's higher-order.

| System | Remove Lua bindings... | Classification |
|--------|------------------------|----------------|
| Assets | Game devs barely notice (Graphics/Audio load assets internally) | **Higher-Order** |
| Events | Game devs lose cross-system communication | **User-Facing** |
| Config | Game devs lose `bestow.config.get*()` calls | **User-Facing** |
| Entity | Game breaks completely | **User-Facing** |
| Graphics Context | Game devs never call this directly | **Higher-Order** |
| Metrics | Game devs never call this directly | **Higher-Order** |

### Full Classification

```
┌─────────────────────────────────────────────────────────────────┐
│                    INFRASTRUCTURE (Higher-Order)                 │
│                                                                 │
│  These systems exist to serve other systems. Game developers    │
│  rarely interact with them directly.                            │
│                                                                 │
│  ┌──────────┐ ┌──────────┐ ┌──────────┐ ┌──────────┐          │
│  │  Types   │ │    DI    │ │  Core    │ │ Metrics  │          │
│  │ (Tier 0) │ │ (Tier 1) │ │ (Tier 1) │ │ (Tier 1) │          │
│  └──────────┘ └──────────┘ └──────────┘ └──────────┘          │
│  ┌──────────┐ ┌──────────┐ ┌──────────┐ ┌──────────┐          │
│  │  Assets  │ │  Shader  │ │ Graphics │ │ UIRender │          │
│  │ (Tier 2) │ │ (Tier 2) │ │ Context  │ │ Backend  │          │
│  │          │ │   NEW    │ │ (Tier 2) │ │ (Tier 2) │          │
│  └──────────┘ └──────────┘ └──────────┘ └──────────┘          │
│  ┌──────────┐ ┌──────────┐ ┌──────────┐                       │
│  │ Services │ │ LuaBind  │ │Blueprints│                       │
│  │ (Tier 6) │ │ (Tier 6) │ │ (Tier 4) │                       │
│  └──────────┘ └──────────┘ └──────────┘                       │
│                                                                 │
├─────────────────────────────────────────────────────────────────┤
│                      USER-FACING SYSTEMS                        │
│                                                                 │
│  These are what game developers use through Lua. Each needs     │
│  complete Lua bindings, documentation, and clear error messages.│
│                                                                 │
│  ┌──────────┐ ┌──────────┐ ┌──────────┐ ┌──────────┐          │
│  │  Events  │ │  Config  │ │  Entity  │ │  Input   │          │
│  │ (Tier 1) │ │ (Tier 2) │ │ (Tier 2) │ │ (Tier 3) │          │
│  └──────────┘ └──────────┘ └──────────┘ └──────────┘          │
│                                                                 │
│  ┌──────────┐ ┌──────────┐ ┌──────────┐ ┌──────────┐          │
│  │Graphics  │ │Graphics  │ │Physics   │ │Physics   │          │
│  │   2D     │ │   3D     │ │   2D     │ │   3D     │          │
│  │ (Tier 4) │ │ (Tier 4) │ │ (Tier 3) │ │ (Tier 3) │          │
│  └──────────┘ └──────────┘ └──────────┘ └──────────┘          │
│                                                                 │
│  ┌──────────┐ ┌──────────┐ ┌──────────┐ ┌──────────┐          │
│  │  Audio   │ │Animation │ │ Anim     │ │   UI     │          │
│  │ (Tier 3) │ │ (Tier 3) │ │ StateMch │ │ (Tier 4) │          │
│  │          │ │          │ │ (Tier 3) │ │          │          │
│  └──────────┘ └──────────┘ └──────────┘ └──────────┘          │
│                                                                 │
│  ┌──────────┐ ┌──────────┐ ┌──────────┐ ┌──────────┐          │
│  │  Camera  │ │   AI     │ │   GAS    │ │  Scene   │          │
│  │ (Tier 5) │ │ (Tier 5) │ │ (Tier 5) │ │ (Tier 5) │          │
│  └──────────┘ └──────────┘ └──────────┘ └──────────┘          │
│                                                                 │
│  ┌──────────┐ ┌──────────┐ ┌──────────┐ ┌──────────┐          │
│  │  State   │ │GameState │ │Particles │ │ Network  │          │
│  │ (Tier 3) │ │ (Tier 5) │ │  (NEW)   │ │  (NEW)   │          │
│  │          │ │          │ │ (Tier 4) │ │ (Tier 3) │          │
│  └──────────┘ └──────────┘ └──────────┘ └──────────┘          │
│                                                                 │
│  ┌──────────┐                                                   │
│  │  Tween   │                                                   │
│  │  (NEW)   │                                                   │
│  │ (Tier 3) │                                                   │
│  └──────────┘                                                   │
│                                                                 │
├─────────────────────────────────────────────────────────────────┤
│                      INTEGRATION LAYER                          │
│                                                                 │
│  ┌──────────┐ ┌──────────┐ ┌──────────┐ ┌──────────┐          │
│  │ Launcher │ │ LuaBind  │ │Dev Tools │ │  Shader  │          │
│  │ (Tier 6) │ │ (Tier 6) │ │ (Tier 6) │ │ (Tier 2) │          │
│  └──────────┘ └──────────┘ └──────────┘ └──────────┘          │
└─────────────────────────────────────────────────────────────────┘
```

---

## Dependency Tiers

Systems are organized into tiers based on their dependency requirements. A system at Tier N may only depend on systems at Tier 0 through N-1. **No lateral dependencies within the same tier.**

### Tier 0: Foundation

No dependencies on any other bestow module.

| System | Module | Contract | Methods | Visibility |
|--------|--------|----------|---------|------------|
| **Types** | `bestow.types` | (data only) | 75+ types | Infrastructure |

The types module defines every shared data structure: math types (Vec2/3/4, Mat3/4, Quat), entity types, asset handles, input codes, physics definitions, audio structures, and rendering primitives. All other modules import this.

---

### Tier 1: Core Infrastructure

Depends only on Tier 0 (Types).

| System | Module | Contract | Methods | Visibility |
|--------|--------|----------|---------|------------|
| **Events** | `bestow.events` | `IEventSystem` | 13 | User-Facing |
| **Core Utilities** | `bestow.utils` | (free functions) | 30+ | Infrastructure |
| **DI Container** | `bestow.di` | `ServiceCollection`, `ServiceProvider` | 10 | Infrastructure |
| **Metrics** | `bestow.metrics` | `MetricsCollector` | 8 | Infrastructure |

#### Events (`bestow.events`)
Cross-system publish/subscribe. Both infrastructure (systems communicate through events) and user-facing (game code subscribes to collision events, scene changes, etc.).

**Why user-facing:** Game developers use `bestow.events.subscribe("collision", callback)` regularly. Removing Lua bindings would break typical game patterns.

#### Core Utilities (`bestow.utils`)
Timer, FrameTimer, Easing functions, math utilities, logging, UUID generation. Pure utility -- no interface contract, just free functions.

**Why infrastructure:** Other systems use timers and easing internally. Game developers interact with these through higher-level systems (Animation, Tween).

#### DI Container (`bestow.di`)
ServiceCollection (registration phase) -> ServiceProvider (resolution phase). Manages singleton lifecycle with reverse-order destruction.

**Why infrastructure:** Game developers never see this. Only the Engine/Launcher uses it.

#### Metrics (`bestow.metrics`)
Per-system timing, Tracy integration, code path tracking. Optional (controlled by `BESTOW_ENABLE_TRACY`).

**Why infrastructure:** Only consumed by Dev Tools overlay. Game developers use Tracy directly if profiling.

---

### Tier 2: Service Infrastructure

Depends on Tier 0-1.

| System | Module | Contract | Methods | Visibility | Deps |
|--------|--------|----------|---------|------------|------|
| **Assets** | `bestow.assets` | `IAssetSystem` | 31 | Infrastructure | Events |
| **Config** | `bestow.config` | `IConfigSystem` | 29 | User-Facing | Events |
| **Entity** | `bestow.entity` | `IEntitySystem` | 33+ | User-Facing | (none) |
| **Graphics Context** | `bestow.graphics.context` | `IGraphicsContext` | 7 | Infrastructure | UIRender |
| **UI Render Backend** | `bestow.uirender` | `IUIRenderBackend` | 20 | Infrastructure | (none) |
| **Shader** | `bestow.shader` | `IShaderSystem` | -- | Infrastructure | Assets |

#### Assets (`bestow.assets`)
**The sole gateway to the file system.** No other system directly reads files. Provides registration, async loading, hot reload monitoring (efsw), and asset library discovery.

**Why infrastructure:** Graphics, Audio, Animation, Shader, Config, Level -- all load resources through Assets. Game developers occasionally register custom assets but primarily benefit indirectly.

**Key architectural rule:** Every system that needs a file goes through `IAssetSystem`. The only exception is `IStateSystem` (writes user save data, which is not a game asset).

#### Config (`bestow.config`)
Lua-based configuration with hot reload. Loads `.cfg.lua` files, provides type-safe getters with fallback defaults.

**Why user-facing:** Game developers read config values constantly: `bestow.config.getFloat("player.speed", 100.0)`. This is a primary part of data-driven design.

#### Entity (`bestow.entity`)
EnTT-based ECS. Creates/destroys entities, manages components with both templated (C++) and type-erased (Lua) access. Includes component reflection for runtime introspection.

**Why user-facing:** The backbone of every game. All game objects are entities with components.

#### Graphics Context (`bestow.graphics.context`)
Base interface for all renderers. Provides window handle, viewport size, and UI render backend access. Extended by both `IGraphicsSystem` (2D) and `IGraphics3DSystem` (3D).

**Why infrastructure:** Game developers use Graphics2D or Graphics3D, never the context directly.

#### UI Render Backend (`bestow.uirender`)
Adapter between RmlUI and the active renderer (Vulkan or OpenGL). Handles geometry compilation, texture management, and scissor regions.

**Why infrastructure:** The UI system uses this internally. Game developers interact with `IUISystem`, never the render backend.

#### Shader (`bestow.shader`)
Shader compilation pipeline (GLSL -> SPIR-V), shader hot reload, material definition parsing. Currently embedded within Graphics3D but should be extracted as its own higher-order system.

**Why infrastructure:** Graphics3D and the Particle system (new) both consume compiled shaders. Extracting shader compilation allows sharing.

**Status:** Currently not a standalone contract. Shader functionality lives inside `IGraphics3DSystem` and `IAssetSystem`. Recommend extracting to `IShaderSystem`.

---

### Tier 3: Core User Systems

Depends on Tier 0-2. These are the fundamental systems game developers interact with.

| System | Module | Contract | Methods | Visibility | Deps |
|--------|--------|----------|---------|------------|------|
| **Input** | `bestow.input` | `IInputSystem` | 45+ | User-Facing | Events, Assets |
| **Audio** | `bestow.audio` | `IAudioSystem` | 24 | User-Facing | Assets |
| **Physics 2D** | `bestow.physics` | `IPhysicsSystem` | 24 | User-Facing | (types only) |
| **Physics 3D** | `bestow.physics3d` | `IPhysics3DSystem` | 100+ | User-Facing | (types only) |
| **Animation** | `bestow.animation` | `IAnimationSystem` | 40+ | User-Facing | Assets |
| **Anim StateMachine** | `bestow.animation.statemachine` | `IAnimationStateMachine` | 26 | User-Facing | Animation |
| **State** | `bestow.state` | `IStateSystem` | 29 | User-Facing | (types only) |
| **Tween** | `bestow.tween` | `ITweenSystem` | -- | User-Facing | Entity |
| **Network** | `bestow.network` | `INetworkSystem` | -- | User-Facing | Events, Entity |

#### Input (`bestow.input`)
Phase-based action system. Registers actions with bindings (keyboard/mouse/gamepad), manages input phases (menu, gameplay, cutscene), provides both event-driven and polling APIs.

**Why separate from Events:** Input has complex state (phases, hold duration, axis values, deadzones) that goes far beyond simple pub/sub.

#### Audio (`bestow.audio`)
FMOD-backed channel-based and positional audio. Manages sound playback, volume groups, 3D listener positioning.

**Why separate from a broader "media" system:** Audio has its own real-time requirements, device management, and spatial processing that are fundamentally different from other media.

#### Physics 2D (`bestow.physics`) and Physics 3D (`bestow.physics3d`)
**Kept separate because their implementations are completely independent.** Physics 2D uses Box2D (C, 2D-specific). Physics 3D uses Jolt (C++, 3D-specific). They share no code, no data structures (Transform2D vs Transform3D), and no concepts (ground checks are 2D-only; constraints/vehicles are 3D-only).

**Key difference from competitors:** Unity and Godot use a single physics API that wraps both 2D and 3D. Bestow keeps them separate because:
1. A 2D game never pays for 3D physics overhead
2. Box2D and Jolt have fundamentally different APIs and capabilities
3. Platformer-specific features (ground checks) only make sense in 2D

#### Animation (`bestow.animation`)
Skeletal animation: skeleton management, animation clip playback, blending, IK solving, bone sockets. Separate from the Animation State Machine which is a higher-level controller.

**Why not merged with Graphics3D:** Animation is a simulation system (calculate bone transforms) that happens to feed into rendering. An audio system might also consume animation data (lip sync). Keeping it separate preserves the single-responsibility principle.

#### Animation State Machine (`bestow.animation.statemachine`)
State graph with parameters, transitions, and conditions. Manages which animation plays based on game state (running, jumping, attacking). Built on top of `IAnimationSystem`.

**Why separate from Animation:** The state machine is a controller pattern. Animation is the data/simulation layer. Different complexity, different consumers (state machine is purely game logic; animation is rendering infrastructure).

#### State (`bestow.state`)
SQLite-backed persistence. Key-value storage with profiles, slots, auto-commit, versioned migration, and JSON export/import. Replaces the older "save system."

**Why not "higher-order":** Game developers call `bestow.state.setNumber("player.hp", 100)` directly. This is a primary game development API.

**Architectural exception:** State is the only system permitted to perform direct file I/O (SQLite writes). All other file access goes through Assets.

#### Tween (`bestow.tween`) -- NEW
Value interpolation with easing. Tween chaining, sequences, entity property animation, callbacks. Currently the C++ easing utilities in `bestow.utils` are not exposed to Lua -- this system bridges that gap.

**Why a standalone system:** Tweening is pervasive in games (UI animations, camera transitions, gameplay juice). Every competitor provides it. Embedding it in Core would require Core to know about entities, which violates its zero-dependency nature.

#### Network (`bestow.network`) -- NEW
Client-server networking with state replication and RPCs. See [contract outline](#inetworksystem-bestownetwork) below.

**Why Tier 3 (not higher):** Network is a foundational system for multiplayer games. Scene, AI, and other high-level systems need to know about network authority. Placing it at Tier 3 allows it to be consumed by Tier 4-5 systems.

---

### Tier 4: Rendering & Presentation Systems

Depends on Tier 0-3. These systems produce visual/audio output.

| System | Module | Contract | Methods | Visibility | Deps |
|--------|--------|----------|---------|------------|------|
| **Graphics 2D** | `bestow.graphics` | `IGraphicsSystem` | 20+ | User-Facing | Entity, Assets, GfxContext |
| **Graphics 3D** | `bestow.graphics3d` | `IGraphics3DSystem` | 80+ | User-Facing | Entity, Assets, Config, Animation, GfxContext |
| **Particles** | `bestow.particles` | `IParticleSystem` | -- | User-Facing | Entity |
| **UI** | `bestow.ui` | `IUISystem` | 45+ | User-Facing | UIRenderBackend |

#### Graphics 2D (`bestow.graphics`) and Graphics 3D (`bestow.graphics3d`)
**Kept separate because their implementations are completely independent.** 2D uses a sprite batch renderer. 3D uses a full mesh/material/lighting pipeline. They share `IGraphicsContext` as a common base but have zero implementation overlap.

**Graphics3D scope reduction:** The lock-on targeting system (7 methods: `lockOn()`, `shiftLockTarget()`, `getLockTarget()`, `pollLockPosition()`, `unlock()`, `isLocked()`, `getPotentialTargets()`) should be extracted from Graphics3D and moved to the Camera system. Lock-on targeting is a gameplay feature that uses 3D positions, not a rendering feature. It currently exists in Graphics3D because it needs screen-space calculations, but Camera already handles coordinate transforms.

**What stays in Graphics3D:**
- Mesh management (create, destroy, update, primitives)
- Material management (PBR, unlit, Lua materials)
- Camera & coordinate transforms
- Lighting (directional, point, spot, ambient)
- Environment (skybox, cubemap, fog)
- Post-processing (bloom, SSAO, tone mapping, exposure) -- same render pipeline
- Shadows -- same render pipeline
- Debug rendering (lines, boxes, spheres) -- same render pipeline
- Skeletal mesh rendering (drawSkinnedMesh) -- needs bone matrices from Animation
- LOD management -- render-time decision
- Instancing -- render-time optimization
- 3D text rendering
- Render statistics

**Post-processing stays in Graphics3D** because it's implemented as additional render passes in the same Vulkan pipeline. Splitting it would require coordinating framebuffer ownership across system boundaries, adding complexity with no benefit.

#### Particles (`bestow.particles`) -- NEW
CPU and GPU particle simulation with emitter management. See [contract outline](#iparticlesystem-bestowparticles) below.

**Why separate from Graphics3D:** Particle simulation (movement, lifetime, collision) is compute work. Particle rendering is a Graphics concern. The system manages simulation; it provides particle data to Graphics for rendering, similar to how Animation provides bone matrices.

#### UI (`bestow.ui`)
RmlUI-based document system with HTML/CSS-like markup, DOM manipulation, data binding, and event handling.

**Why not merged with Graphics:** UI has its own layout engine, styling system, event model, and font management. The only connection to Graphics is the `IUIRenderBackend` which adapts RmlUI's render calls to the active GPU API.

---

### Tier 5: Gameplay Systems

Depends on Tier 0-4. These are higher-level systems that orchestrate gameplay.

| System | Module | Contract | Methods | Visibility | Deps |
|--------|--------|----------|---------|------------|------|
| **Camera** | `bestow.camera` | `ICameraSystem` | 12+ | User-Facing | Entity, Input |
| **AI** | `bestow.ai` | `IAISystem` | 18 | User-Facing | Entity |
| **GAS** | `bestow.gas` | `IGASSystem` | 42 | User-Facing | Entity |
| **Blueprints** | `bestow.blueprints` | `IBlueprintFactory` | 12 | Infrastructure | Entity, Assets, Config |
| **Scene** | `bestow.scene` | `ISceneSystem` | 11 | User-Facing | Assets, Input, UI, Events |
| **Game State** | `bestow.gamestate` | `IGameStateSystem` | 22 | User-Facing | (types only) |

#### Camera (`bestow.camera`)
Camera management with follow target, smoothing, dead zones, bounds, shake effects, and zoom. **Should absorb lock-on targeting** from Graphics3D since lock-on is a camera/gameplay feature.

**Expanded scope with lock-on:**
- Current: follow, shake, zoom, dead zone, bounds
- Added: `lockOn()`, `shiftLockTarget()`, `getLockTarget()`, `unlock()`, `isLocked()`, `getPotentialTargets()`
- Camera already has `screenToWorld()`/`worldToScreen()` which lock-on needs

#### AI (`bestow.ai`)
Behavior trees (BehaviorTree.CPP), navigation meshes (Recast/Detour), pathfinding, steering behaviors, patrol routes, spatial queries (radius search, line of sight).

**Critical gap:** Zero Lua bindings exist. This system is invisible to game developers writing Lua.

#### GAS (`bestow.gas`) -- Gameplay Ability System
Gameplay tags, attributes (health, mana, speed), effects (buffs, debuffs, damage over time), and abilities (with costs, cooldowns, activation policies). Modeled after Unreal's GAS.

**Why standalone:** GAS is a complete gameplay framework. It manages its own data (tags, attributes, effects, abilities) and has its own update loop. Embedding it in Entity would bloat ECS with game-specific logic.

#### Blueprints (`bestow.blueprints`)
Lua-defined entity templates. Loads blueprint definitions from Lua files, instantiates entities with preconfigured components, supports inheritance and property overrides.

**Why infrastructure:** Game developers call `bestow.blueprints.create("enemy_goblin")` but never manage the factory itself. The system is consumed by Level/Scene loading and hot reload.

#### Scene (`bestow.scene`) vs Game State (`bestow.gamestate`)
These two systems manage different state stacks and serve different purposes:

| Aspect | Scene | Game State |
|--------|-------|------------|
| **What** | Lua scene scripts (levels, menus) | C++ state objects with enter/exit |
| **Stack type** | Scene files with update/render | State classes with lifecycle |
| **Primary consumer** | Lua game code | C++ engine/framework code |
| **Parameters** | `std::any` map | Per-state typed data |
| **Example** | `pushScene("level_1", {difficulty=3})` | `pushState<PauseMenu>()` |

**Why kept separate:** Scene manages Lua-level scene loading and execution. Game State manages C++ state transitions. A game uses Scenes for levels and Game State for engine-level flow (loading screen, pause overlay). Merging them would force all state management through either Lua or C++, reducing flexibility.

---

### Tier 6: Integration Layer

Depends on all tiers. These systems bind everything together.

| System | Module | Role | Visibility |
|--------|--------|------|------------|
| **Services/Engine** | `bestow.services` | Composition root, DI wiring | Infrastructure |
| **Lua Bindings** | `bestow-luabind` | Binds C++ contracts to Lua | Infrastructure |
| **Dev Tools** | `bestow-dev` | ImGui overlay, inspector, hot reload | Infrastructure (Debug only) |
| **Launcher** | `bestow-launcher` | CLI commands, game runner, main loop | Infrastructure |

#### Services (`bestow.services`)
Defines `IEngine`, `IApplication`, and the `Application<Derived, Deps...>` CRTP base. Re-exports all contracts. This is the top-level aggregation that the Launcher uses.

#### Lua Bindings (`bestow-luabind`)
17+ binding files that expose C++ contracts to Lua as `bestow.*` namespace. Also includes:
- `DocRegistry` -- structured API documentation
- `StubGenerator` -- EmmyLua type annotation output for IDE autocomplete

#### Dev Tools (`bestow-dev`)
Debug-only (compiled with `BESTOW_DEV_TOOLS`):
- `DevOverlay` -- ImGui FPS graphs, system timing, entity inspector
- `HotReloadManager` -- efsw file watcher with graceful error recovery
- `ComponentRegistry` -- runtime component introspection for inspector

#### Launcher (`bestow-launcher`)
Entry point and composition root:
- `GameRunner` -- initializes all systems, runs the game loop, manages shutdown
- `CommandLine` -- argument parsing
- CLI commands: `run`, `new`, `init`, `api`, `generate-stubs`, `version`, `help`, `update`

---

## New Systems (Gap Closers)

These three systems close the most critical competitive gaps identified in the analysis.

### 1. Particle System (`bestow.particles`)

**Gap closed:** 0% -> ~80% particle/VFX parity with competitors
**Priority:** Critical -- particles appear in virtually every game

**Scope:**
- CPU particle simulation (emitters, lifetime, velocity, gravity, color/size over life)
- Emitter management (point, sphere, box, cone, mesh surface emitters)
- Particle collision with physics world
- Sub-emitters (spawn particles on death, collision)
- Trails/ribbons
- GPU compute particles (future phase -- requires compute shader infrastructure)

**What it does NOT include:**
- Particle rendering (Graphics3D handles this given particle position/color/size data)
- Visual editor (Dev Tools concern)

**Depends on:** Types, Entity
**Consumed by:** Graphics3D (for rendering), Lua game code (for spawning effects)

### 2. Network System (`bestow.network`)

**Gap closed:** 0% -> ~60% networking parity with competitors
**Priority:** Critical -- multiplayer is expected in modern engines

**Scope (Phase 1 - Foundation):**
- Transport abstraction (ENet for UDP, WebSocket for web)
- Client-server with server authority
- Property replication (entity component sync)
- RPC system (client-to-server, server-to-client, multicast)
- Connection management (connect, disconnect, timeout)
- Network identity (maps entities across client/server)

**Scope (Phase 2 - Advanced):**
- Client-side prediction and server reconciliation
- Lag compensation
- Lobby/matchmaking
- Interest management (only replicate nearby entities)

**What it does NOT include:**
- Voice chat (third-party service concern)
- Account management (server application concern)

**Depends on:** Types, Events, Entity
**Consumed by:** Scene (network-aware scene transitions), AI (authority checks), Lua game code

### 3. Tween System (`bestow.tween`)

**Gap closed:** Missing Lua animation utility -> Full tween support
**Priority:** High -- tweening is fundamental to game "juice"

**Scope:**
- Value interpolation with all easing functions (already in C++ Core)
- Entity property tweening (position, rotation, scale, color, opacity)
- Tween chaining and sequencing
- Parallel tween groups
- Callbacks (onStart, onUpdate, onComplete)
- Tween cancellation and pausing
- Relative and absolute target values

**Depends on:** Types, Entity
**Consumed by:** Lua game code, UI (for UI animations), Camera (for transitions)

### 4. Shader System Extraction (`bestow.shader`)

**Scope:** Extract shader compilation, caching, hot reload, and material definition parsing from Graphics3D into a standalone higher-order system. This allows Particles and future compute systems to share shader infrastructure.

**This is a refactor, not a new feature.** The code already exists inside Graphics3D/Assets -- it needs to be extracted behind `IShaderSystem`.

---

## Dependency Graph

```
                           ┌──────────┐
                           │  Types   │  Tier 0
                           └────┬─────┘
                    ┌───────────┼───────────┬──────────┐
                    ▼           ▼           ▼          ▼
              ┌──────────┐┌──────────┐┌────────┐┌──────────┐
              │  Events  ││   Core   ││   DI   ││ Metrics  │  Tier 1
              └────┬─────┘└──────────┘└────────┘└──────────┘
         ┌─────────┼─────────────┬──────────────┐
         ▼         ▼             ▼              ▼
   ┌──────────┐┌──────────┐┌──────────┐┌──────────┐┌──────────┐
   │  Assets  ││  Config  ││  Entity  ││ GfxCtx   ││ UIRender │  Tier 2
   └────┬─────┘└──────────┘└────┬─────┘└────┬─────┘└────┬─────┘
        │                       │            │           │
   ┌────┼───────┬───────┬───────┼────────┐   │           │
   ▼    ▼       ▼       ▼       ▼        ▼   ▼           ▼
┌─────┐┌─────┐┌─────┐┌─────┐┌─────┐┌──────┐┌─────┐┌─────┐┌──────┐┌──────┐
│Input││Audio││Phys ││Phys ││Anim ││Anim  ││State││Tween││ Net  ││Shader│ T3
│     ││     ││ 2D  ││ 3D  ││     ││ SM   ││     ││ NEW ││ NEW  ││(extr)│
└─────┘└─────┘└─────┘└─────┘└─────┘└──────┘└─────┘└─────┘└──────┘└──────┘
        │             │              │                         │
   ┌────┼─────┬───────┼──────┬───────┤                         │
   ▼    ▼     ▼       ▼      ▼       ▼                         ▼
┌──────┐┌──────┐┌──────────┐┌──────┐┌──────────┐        ┌──────────┐
│Gfx2D││Gfx3D ││Particles ││  UI  ││ (future  │        │ (future  │  Tier 4
│     ││      ││   NEW    ││      ││ compute) │        │ terrain) │
└─────┘└──────┘└──────────┘└──────┘└──────────┘        └──────────┘
                    │
   ┌────────────────┼────────────────────┐
   ▼                ▼                    ▼
┌──────┐┌──────┐┌──────┐┌──────┐┌──────┐┌──────────┐
│Camera││  AI  ││ GAS  ││Scene ││GmStat││Blueprints│  Tier 5
│      ││      ││      ││      ││      ││          │
└──────┘└──────┘└──────┘└──────┘└──────┘└──────────┘
                    │
              ┌─────┼─────┐
              ▼     ▼     ▼
        ┌──────┐┌──────┐┌──────┐
        │Engine││LuaBnd││DevTls│  Tier 6
        └──────┘└──────┘└──────┘
```

### Dependency Rules

1. **No lateral dependencies.** A Tier 3 system cannot depend on another Tier 3 system (except Animation SM -> Animation, which is a deliberate parent-child relationship).

2. **Infrastructure flows down.** Higher-order systems (Assets, Events, Config) are consumed by lower tiers. They never consume user-facing systems.

3. **User-facing systems are leaves.** Physics, Audio, Input do not depend on each other. They communicate through Events when needed.

4. **Only the integration layer sees everything.** Launcher, LuaBind, and DevTools depend on all systems. No other system has this privilege.

---

## Game Loop Integration

The current game loop runs systems in this order. New systems are marked with their insertion points.

```
Per Frame:
─────────────────────────────────────────────────────

Phase 1: Input & Hot Reload
  1. IInputSystem.update()
  2. ScriptManager.update()          [hot reload]
  3. updateTimers(dt)

Phase 2: Simulation
  4. IStateSystem.update(dt)
  5. IAnimationSystem.update(dt)
  6. ITweenSystem.update(dt)          [NEW - after animation, before scene]
  7. INetworkSystem.update(dt)        [NEW - process network messages]
  8. ISceneSystem.update(dt)          [calls Lua scene update]

Phase 3: Game Logic
  9. app.main.update(dt)              [user Lua code]

Phase 4: Physics
  10. IPhysicsSystem.update(dt)       [2D, if active]
  11. IPhysics3DSystem.update(dt)     [3D, if active]
  12. IPhysics3DSystem.syncTransforms()

Phase 5: Post-Simulation
  13. IGASSystem.update(dt)
  14. IAISystem.update(dt)
  15. IParticleSystem.update(dt)      [NEW - simulate particles]
  16. ICameraSystem.update(dt)        [after physics, uses entity positions]

Phase 6: Rendering
  17. IGraphics3DSystem.beginFrame()
  18. ISceneSystem.render()           [Lua scene render]
  19. IParticleSystem.render()        [NEW - submit particle data to renderer]
  20. IUISystem.update(dt)
  21. IUISystem.render()
  22. app.main.render()               [user Lua overlay]
  23. IGraphics3DSystem.endFrame()

Phase 7: Network Sync
  24. INetworkSystem.sendUpdates()    [NEW - replicate state changes]

Phase 8: Frame End
  25. Window events (glfwPollEvents)
  26. Frame sleep
```

### New System Placement Rationale

- **Tween (step 6):** After animation (tweens may override animated values) but before scene update (tweens may trigger state changes).
- **Network receive (step 7):** Process incoming messages before game logic executes, so Lua code operates on fresh network state.
- **Particles (step 15):** After physics (particles may collide with world) and after game logic (which may spawn emitters).
- **Network send (step 24):** After all state changes are finalized, replicate to clients.

---

## Contract Outlines for New Systems

### `IParticleSystem` (`bestow.particles`)

```cpp
export module bestow.particles;
import bestow.types;

export namespace bestow {

using EmitterHandle = uint32_t;
using ParticleEffectHandle = uint32_t;

enum class EmitterShape {
    Point, Sphere, Box, Cone, Ring, MeshSurface
};

enum class SimulationSpace {
    Local,   // Particles move with emitter
    World    // Particles are world-space once emitted
};

struct ParticleEmitterDef {
    EmitterShape shape = EmitterShape::Point;
    SimulationSpace space = SimulationSpace::World;

    // Emission
    float emissionRate = 10.0f;      // particles per second
    int burstCount = 0;              // one-shot burst
    float lifetime = 1.0f;           // particle lifetime in seconds
    float lifetimeVariance = 0.0f;

    // Initial state
    Vec3 initialVelocity = {0, 1, 0};
    Vec3 velocityVariance = {0, 0, 0};
    float initialSize = 1.0f;
    float sizeVariance = 0.0f;
    Color initialColor = {1, 1, 1, 1};

    // Over lifetime
    float sizeOverLifetime = 0.0f;   // multiplier at end of life
    Color colorOverLifetime = {1, 1, 1, 0}; // fade to this
    float gravityScale = 0.0f;
    float drag = 0.0f;

    // Collision
    bool collisionEnabled = false;
    float collisionBounce = 0.5f;
    float collisionLifetimeLoss = 0.0f; // 0-1, fraction lost on bounce

    // Rendering
    MaterialHandle material;
    BlendMode blendMode = BlendMode::Additive;

    int maxParticles = 1000;
};

class IParticleSystem {
public:
    virtual ~IParticleSystem() = default;

    // Lifecycle
    virtual void update(DeltaTime dt) = 0;

    // Emitter management
    virtual EmitterHandle createEmitter(const ParticleEmitterDef& def) = 0;
    virtual void destroyEmitter(EmitterHandle handle) = 0;
    virtual bool hasEmitter(EmitterHandle handle) const = 0;

    // Emitter control
    virtual void setEmitterPosition(EmitterHandle, const Vec3& pos) = 0;
    virtual void setEmitterRotation(EmitterHandle, const Quat& rot) = 0;
    virtual void startEmitting(EmitterHandle) = 0;
    virtual void stopEmitting(EmitterHandle) = 0;
    virtual void burst(EmitterHandle, int count) = 0;
    virtual bool isEmitting(EmitterHandle) const = 0;

    // Runtime modification
    virtual void setEmissionRate(EmitterHandle, float rate) = 0;
    virtual void setParticleLifetime(EmitterHandle, float lifetime) = 0;
    virtual void setGravityScale(EmitterHandle, float scale) = 0;

    // Entity attachment
    virtual void attachToEntity(EmitterHandle, Entity entity) = 0;
    virtual void detachFromEntity(EmitterHandle) = 0;

    // Queries
    virtual int getActiveParticleCount(EmitterHandle) const = 0;
    virtual int getTotalActiveParticles() const = 0;

    // Effect presets (Lua-defined)
    virtual ParticleEffectHandle loadEffect(const std::string& path) = 0;
    virtual EmitterHandle spawnEffect(ParticleEffectHandle, const Vec3& pos) = 0;

    // Render data (consumed by Graphics3D)
    virtual std::span<const Vec3> getParticlePositions(EmitterHandle) const = 0;
    virtual std::span<const Color> getParticleColors(EmitterHandle) const = 0;
    virtual std::span<const float> getParticleSizes(EmitterHandle) const = 0;
};

} // namespace bestow
```

### `INetworkSystem` (`bestow.network`)

```cpp
export module bestow.network;
import bestow.types;

export namespace bestow {

using NetworkId = uint32_t;
using ClientId = uint32_t;
using RPCId = uint32_t;

enum class NetworkRole {
    None, Server, Client
};

enum class ConnectionState {
    Disconnected, Connecting, Connected, Disconnecting
};

enum class ReplicationMode {
    ServerToClients,  // Server owns, clients receive
    OwnerToServer,    // Owning client sends to server
    ServerToOwner     // Server sends only to owning client
};

struct NetworkConfig {
    std::string address = "127.0.0.1";
    uint16_t port = 7777;
    int maxClients = 16;
    float tickRate = 30.0f;        // network updates per second
    float timeoutSeconds = 10.0f;
};

struct ReplicatedProperty {
    std::string componentName;
    std::string fieldName;
    ReplicationMode mode = ReplicationMode::ServerToClients;
    float updateFrequency = 0.0f;  // 0 = every tick
};

class INetworkSystem {
public:
    virtual ~INetworkSystem() = default;

    // Lifecycle
    virtual void update(DeltaTime dt) = 0;
    virtual void sendUpdates() = 0;

    // Host / Join
    virtual Result<void, std::error_code> host(const NetworkConfig& config) = 0;
    virtual Result<void, std::error_code> connect(const NetworkConfig& config) = 0;
    virtual void disconnect() = 0;

    // State
    virtual NetworkRole getRole() const = 0;
    virtual ConnectionState getConnectionState() const = 0;
    virtual bool isServer() const = 0;
    virtual bool isClient() const = 0;
    virtual bool isConnected() const = 0;
    virtual ClientId getLocalClientId() const = 0;

    // Client management (server only)
    virtual std::vector<ClientId> getConnectedClients() const = 0;
    virtual void kickClient(ClientId, const std::string& reason) = 0;

    // Network entities
    virtual void registerNetworkEntity(Entity, NetworkId netId) = 0;
    virtual void unregisterNetworkEntity(Entity) = 0;
    virtual NetworkId getNetworkId(Entity) const = 0;
    virtual std::optional<Entity> getEntityByNetworkId(NetworkId) const = 0;
    virtual void setEntityOwner(Entity, ClientId) = 0;
    virtual ClientId getEntityOwner(Entity) const = 0;
    virtual bool isLocalAuthority(Entity) const = 0;

    // Replication
    virtual void registerReplicatedProperty(const ReplicatedProperty& prop) = 0;
    virtual void markDirty(Entity, const std::string& component) = 0;

    // RPCs
    virtual RPCId registerRPC(const std::string& name,
                              std::function<void(ClientId, const EventData&)> handler) = 0;
    virtual void callServerRPC(RPCId, const EventData& args) = 0;
    virtual void callClientRPC(RPCId, ClientId target, const EventData& args) = 0;
    virtual void callMulticastRPC(RPCId, const EventData& args) = 0;

    // Callbacks
    virtual SubscriptionId onClientConnected(std::function<void(ClientId)>) = 0;
    virtual SubscriptionId onClientDisconnected(std::function<void(ClientId)>) = 0;
    virtual void unsubscribe(SubscriptionId) = 0;

    // Stats
    virtual float getRoundTripTime() const = 0;
    virtual float getPacketLoss() const = 0;
    virtual int getBytesSentPerSecond() const = 0;
    virtual int getBytesReceivedPerSecond() const = 0;
};

} // namespace bestow
```

### `ITweenSystem` (`bestow.tween`)

```cpp
export module bestow.tween;
import bestow.types;

export namespace bestow {

using TweenHandle = uint32_t;

enum class TweenState {
    Running, Paused, Completed, Cancelled
};

struct TweenDef {
    float duration = 1.0f;
    float delay = 0.0f;
    EasingFunction easing = EasingFunction::Linear;
    int loops = 1;            // -1 for infinite
    bool yoyo = false;        // reverse on each loop
};

class ITweenSystem {
public:
    virtual ~ITweenSystem() = default;

    // Lifecycle
    virtual void update(DeltaTime dt) = 0;

    // Value tweens
    virtual TweenHandle tweenFloat(float from, float to,
                                    const TweenDef& def,
                                    std::function<void(float)> onUpdate) = 0;
    virtual TweenHandle tweenVec2(Vec2 from, Vec2 to,
                                   const TweenDef& def,
                                   std::function<void(Vec2)> onUpdate) = 0;
    virtual TweenHandle tweenVec3(Vec3 from, Vec3 to,
                                   const TweenDef& def,
                                   std::function<void(Vec3)> onUpdate) = 0;
    virtual TweenHandle tweenColor(Color from, Color to,
                                    const TweenDef& def,
                                    std::function<void(Color)> onUpdate) = 0;

    // Entity property tweens (convenience)
    virtual TweenHandle tweenPosition(Entity, Vec2 target, const TweenDef& def) = 0;
    virtual TweenHandle tweenPosition3D(Entity, Vec3 target, const TweenDef& def) = 0;
    virtual TweenHandle tweenRotation(Entity, float targetDegrees, const TweenDef& def) = 0;
    virtual TweenHandle tweenScale(Entity, Vec2 target, const TweenDef& def) = 0;

    // Control
    virtual void pause(TweenHandle) = 0;
    virtual void resume(TweenHandle) = 0;
    virtual void cancel(TweenHandle) = 0;
    virtual void complete(TweenHandle) = 0;  // jump to end
    virtual TweenState getState(TweenHandle) const = 0;

    // Callbacks
    virtual void onComplete(TweenHandle, std::function<void()> callback) = 0;

    // Sequences
    virtual TweenHandle sequence(std::vector<TweenHandle> tweens) = 0;
    virtual TweenHandle parallel(std::vector<TweenHandle> tweens) = 0;

    // Bulk operations
    virtual void cancelAll() = 0;
    virtual void cancelAllForEntity(Entity) = 0;
    virtual int getActiveTweenCount() const = 0;
};

} // namespace bestow
```

---

## Summary: Complete System Inventory

### All Systems (Current + Planned)

| # | System | Module | Contract | Methods | Tier | Visibility | Status |
|---|--------|--------|----------|---------|------|------------|--------|
| 1 | Types | `bestow.types` | -- | 75+ types | 0 | Infra | Exists |
| 2 | Events | `bestow.events` | `IEventSystem` | 13 | 1 | User | Exists |
| 3 | Core Utilities | `bestow.utils` | -- | 30+ | 1 | Infra | Exists |
| 4 | DI Container | `bestow.di` | `ServiceCollection` | 10 | 1 | Infra | Exists |
| 5 | Metrics | `bestow.metrics` | `MetricsCollector` | 8 | 1 | Infra | Exists |
| 6 | Assets | `bestow.assets` | `IAssetSystem` | 31 | 2 | Infra | Exists |
| 7 | Config | `bestow.config` | `IConfigSystem` | 29 | 2 | User | Exists |
| 8 | Entity | `bestow.entity` | `IEntitySystem` | 33+ | 2 | User | Exists |
| 9 | Graphics Context | `bestow.graphics.context` | `IGraphicsContext` | 7 | 2 | Infra | Exists |
| 10 | UI Render Backend | `bestow.uirender` | `IUIRenderBackend` | 20 | 2 | Infra | Exists |
| 11 | Shader | `bestow.shader` | `IShaderSystem` | -- | 2 | Infra | **Extract** |
| 12 | Input | `bestow.input` | `IInputSystem` | 45+ | 3 | User | Exists |
| 13 | Audio | `bestow.audio` | `IAudioSystem` | 24 | 3 | User | Exists |
| 14 | Physics 2D | `bestow.physics` | `IPhysicsSystem` | 24 | 3 | User | Exists |
| 15 | Physics 3D | `bestow.physics3d` | `IPhysics3DSystem` | 100+ | 3 | User | Exists |
| 16 | Animation | `bestow.animation` | `IAnimationSystem` | 40+ | 3 | User | Exists |
| 17 | Anim StateMachine | `bestow.animation.statemachine` | `IAnimationStateMachine` | 26 | 3 | User | Exists |
| 18 | State | `bestow.state` | `IStateSystem` | 29 | 3 | User | Exists |
| 19 | Tween | `bestow.tween` | `ITweenSystem` | ~20 | 3 | User | **New** |
| 20 | Network | `bestow.network` | `INetworkSystem` | ~35 | 3 | User | **New** |
| 21 | Graphics 2D | `bestow.graphics` | `IGraphicsSystem` | 20+ | 4 | User | Exists |
| 22 | Graphics 3D | `bestow.graphics3d` | `IGraphics3DSystem` | 80+ | 4 | User | Exists |
| 23 | Particles | `bestow.particles` | `IParticleSystem` | ~25 | 4 | User | **New** |
| 24 | UI | `bestow.ui` | `IUISystem` | 45+ | 4 | User | Exists |
| 25 | Camera | `bestow.camera` | `ICameraSystem` | 12+ | 5 | User | Exists (expand) |
| 26 | AI | `bestow.ai` | `IAISystem` | 18 | 5 | User | Exists |
| 27 | GAS | `bestow.gas` | `IGASSystem` | 42 | 5 | User | Exists |
| 28 | Blueprints | `bestow.blueprints` | `IBlueprintFactory` | 12 | 5 | Infra | Exists |
| 29 | Scene | `bestow.scene` | `ISceneSystem` | 11 | 5 | User | Exists |
| 30 | Game State | `bestow.gamestate` | `IGameStateSystem` | 22 | 5 | User | Exists |
| 31 | Services | `bestow.services` | `IEngine` | 16+ | 6 | Infra | Exists |
| 32 | Lua Bindings | `bestow-luabind` | -- | -- | 6 | Infra | Exists |
| 33 | Dev Tools | `bestow-dev` | -- | -- | 6 | Infra | Exists |
| 34 | Launcher | `bestow-launcher` | -- | -- | 6 | Infra | Exists |

### Changes from Current State

| Change | Description |
|--------|-------------|
| **NEW: Particles** | CPU/GPU particle system with emitter management |
| **NEW: Network** | Client-server multiplayer with replication and RPCs |
| **NEW: Tween** | Value/entity interpolation with easing and sequencing |
| **EXTRACT: Shader** | Pull shader compilation out of Graphics3D/Assets into standalone |
| **EXPAND: Camera** | Absorb lock-on targeting from Graphics3D |
| **REDUCE: Graphics3D** | Remove lock-on targeting (moves to Camera) |

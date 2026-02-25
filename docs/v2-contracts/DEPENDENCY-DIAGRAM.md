# System Dependency Diagram

## Visibility Tiers

Every system in Bestow belongs to exactly one visibility tier:

```
┌──────────────────────────────────────────────────────────────────┐
│                        VISIBILITY TIERS                          │
├──────────────────────────────────────────────────────────────────┤
│                                                                  │
│  ┌────────────────────────────────────────────────────────────┐  │
│  │  PUBLIC — Exposed to Lua + C++                             │  │
│  │                                                            │  │
│  │  Dual API (System + Core):                                 │  │
│  │    Entity, Input, Audio, Physics2D, Physics3D,             │  │
│  │    Graphics2D, Graphics3D, Animation, Camera, Scene,       │  │
│  │    State, Config, UI, AI, GAS,                             │  │
│  │    Tween*, Particles*, Network*                            │  │
│  │                                                            │  │
│  │  Single API:                                               │  │
│  │    Events, GameState, AnimationStateMachine                │  │
│  │                                                            │  │
│  │  * = new in V2                                             │  │
│  └────────────────────────────────────────────────────────────┘  │
│                                                                  │
│  ┌────────────────────────────────────────────────────────────┐  │
│  │  PROTECTED — C++ only, peer systems only                   │  │
│  │                                                            │  │
│  │    Assets, Shader, GraphicsContext,                         │  │
│  │    UIRenderBackend, Blueprints, Metrics                    │  │
│  └────────────────────────────────────────────────────────────┘  │
│                                                                  │
│  ┌────────────────────────────────────────────────────────────┐  │
│  │  INTERNAL — Implementation details, invisible outside      │  │
│  │                                                            │  │
│  │    VulkanPipeline, FMODChannelManager, Box2DWrapper,       │  │
│  │    JoltPhysicsImpl, EnTTRegistryWrapper, etc.              │  │
│  └────────────────────────────────────────────────────────────┘  │
│                                                                  │
└──────────────────────────────────────────────────────────────────┘
```

## Dependency Tiers

Systems are organized into tiers based on what they depend on. A system may only depend on systems in the same tier or lower.

```
Tier 0 ─── bestow.types (Foundation)
  │        No dependencies. Every other module imports this.
  │
Tier 1 ─── Events, Config, Metrics
  │        Depend only on bestow.types.
  │        No peer system dependencies.
  │
Tier 2 ─── Assets, Entity, Input, State, Tween
  │        May depend on Tier 1 systems.
  │        Assets depends on: Events (for change notifications)
  │        Entity depends on: Events (for lifecycle events)
  │        Input depends on: Events (for action dispatch)
  │        State depends on: Events (for save/load events)
  │        Tween depends on: (none, self-contained)
  │
Tier 3 ─── Shader, Blueprints, Audio, Physics2D, Physics3D, GAS, Network
  │        May depend on Tier 1-2 systems.
  │        Shader depends on: Assets
  │        Blueprints depends on: Entity, Assets, Config
  │        Audio depends on: Assets, Entity
  │        Physics2D depends on: Entity, Events
  │        Physics3D depends on: Entity, Events
  │        GAS depends on: Entity, Events
  │        Network depends on: Entity, Events
  │
Tier 4 ─── GraphicsContext, Animation, AI, Camera
  │        May depend on Tier 1-3 systems.
  │        GraphicsContext depends on: (abstract base, minimal deps)
  │        Animation depends on: Entity, Assets
  │        AI depends on: Entity, Physics3D, Assets
  │        Camera depends on: Entity
  │
Tier 5 ─── Graphics2D, Graphics3D, UIRenderBackend, Particles
  │        May depend on Tier 1-4 systems.
  │        Graphics2D depends on: Entity, Assets, Shader, GraphicsContext
  │        Graphics3D depends on: Entity, Assets, Shader, Animation, GraphicsContext
  │        UIRenderBackend depends on: GraphicsContext
  │        Particles depends on: Entity, Graphics3D or Graphics2D
  │
Tier 6 ─── UI, Scene, GameState
  │        May depend on all lower tiers.
  │        UI depends on: UIRenderBackend, GraphicsContext, Input, Assets
  │        Scene depends on: Entity, Assets, Blueprints, Events, Input
  │        GameState depends on: Input, Events
  │
Tier 7 ─── Engine (Composition Root)
           Owns all systems. Drives the game loop.
           Not a contract — it's the concrete orchestrator.
```

## Full Dependency Graph

```
                              bestow.types
                                   │
                 ┌─────────────────┼─────────────────┐
                 │                 │                  │
              Events           Config             Metrics
                 │                 │
       ┌────────┼────────┐        │
       │        │        │        │
    Assets    Entity   Input    State      Tween
       │        │        │
       │   ┌────┼────────┼──────────────┐
       │   │    │        │              │
    Shader │  Audio   Physics2D    Physics3D    GAS    Network
       │   │    │                    │           │
       │   Blueprints               │           │
       │   │                        │           │
       │   │              ┌─────────┤           │
       │   │              │         │           │
       │   │         Animation      AI          │
       │   │              │                     │
       │   │    GraphicsContext                  │
       │   │         │         │                │
       │   │    Graphics2D  Graphics3D   UIRenderBackend
       │   │                    │              │
       │   │              Particles            │
       │   │                                   │
       │   │              ┌────────────────────┘
       │   │              │
       │   │             UI           GameState
       │   │              │
       │   └──────────────┤
       │                  │
       │               Scene
       │
       └──── (all systems read files through Assets)
```

## Dependency Matrix

Rows depend on columns. `D` = direct dependency, `T` = transitive only.

```
                  types evnt conf metr asst enti inpt stat twen shdr blpr audi ph2d ph3d gas  netw anim ai   cam  gctx g2d  g3d  uirb part ui   scen gst
types              -
events             D    -
config             D    .    -
metrics            D    .    .    -
assets             D    D    .    .    -
entity             D    D    .    .    .    -
input              D    D    .    .    .    .    -
state              D    D    .    .    .    .    .    -
tween              D    .    .    .    .    .    .    .    -
shader             D    .    .    .    D    .    .    .    .    -
blueprints         D    .    D    .    D    D    .    .    .    .    -
audio              D    .    .    .    D    D    .    .    .    .    .    -
physics2d          D    D    .    .    .    D    .    .    .    .    .    .    -
physics3d          D    D    .    .    .    D    .    .    .    .    .    .    .    -
gas                D    D    .    .    .    D    .    .    .    .    .    .    .    .    -
network            D    D    .    .    .    D    .    .    .    .    .    .    .    .    .    -
animation          D    .    .    .    D    D    .    .    .    .    .    .    .    .    .    .    -
ai                 D    .    .    .    D    D    .    .    .    .    .    .    .    D    .    .    .    -
camera             D    .    .    .    .    D    .    .    .    .    .    .    .    .    .    .    .    .    -
gfx_context        D    .    .    .    .    .    .    .    .    .    .    .    .    .    .    .    .    .    .    -
graphics2d         D    .    .    .    D    D    .    .    .    D    .    .    .    .    .    .    .    .    .    D    -
graphics3d         D    .    .    .    D    D    .    .    .    D    .    .    .    .    .    .    D    .    .    D    .    -
ui_render_back     D    .    .    .    .    .    .    .    .    .    .    .    .    .    .    .    .    .    .    D    .    .    -
particles          D    .    .    .    .    D    .    .    .    .    .    .    .    .    .    .    .    .    .    .    .    D    .    -
ui                 D    .    .    .    D    .    D    .    .    .    .    .    .    .    .    .    .    .    .    D    .    .    D    .    -
scene              D    D    .    .    D    D    D    .    .    .    D    .    .    .    .    .    .    .    .    .    .    .    .    .    .    -
gamestate          D    D    .    .    .    .    D    .    .    .    .    .    .    .    .    .    .    .    .    .    .    .    .    .    .    .    -
```

## System Summary Table

| System | Visibility | Tier | Direct Dependencies | Lua Path | API Level |
|--------|-----------|------|--------------------|-----------|----|
| **Types** | Foundation | 0 | None | (imported implicitly) | — |
| **Events** | Public | 1 | Types | `bestow.events` | Single |
| **Config** | Public | 1 | Types | `bestow.config` / `.core` | Dual |
| **Metrics** | Protected | 1 | Types | — | Single |
| **Assets** | Protected | 2 | Types, Events | — | Single |
| **Entity** | Public | 2 | Types, Events | `bestow.entity` / `.core` | Dual |
| **Input** | Public | 2 | Types, Events | `bestow.input` / `.core` | Dual |
| **State** | Public | 2 | Types, Events | `bestow.state` / `.core` | Dual |
| **Tween** | Public | 2 | Types | `bestow.tween` / `.core` | Dual |
| **Shader** | Protected | 3 | Types, Assets | — | Single |
| **Blueprints** | Protected | 3 | Types, Entity, Assets, Config | — | Single |
| **Audio** | Public | 3 | Types, Assets, Entity | `bestow.audio` / `.core` | Dual |
| **Physics2D** | Public | 3 | Types, Entity, Events | `bestow.physics` / `.core` | Dual |
| **Physics3D** | Public | 3 | Types, Entity, Events | `bestow.physics3d` / `.core` | Dual |
| **GAS** | Public | 3 | Types, Entity, Events | `bestow.gas` / `.core` | Dual |
| **Network** | Public | 3 | Types, Entity, Events | `bestow.network` / `.core` | Dual |
| **Animation** | Public | 4 | Types, Entity, Assets | `bestow.animation` / `.core` / `.fsm` | Dual + FSM |
| **AI** | Public | 4 | Types, Entity, Physics3D, Assets | `bestow.ai` / `.core` | Dual |
| **Camera** | Public | 4 | Types, Entity | `bestow.camera` / `.core` | Dual |
| **GraphicsContext** | Protected | 4 | Types | — | Single |
| **Graphics2D** | Public | 5 | Types, Entity, Assets, Shader, GfxCtx | `bestow.graphics` / `.core` | Dual |
| **Graphics3D** | Public | 5 | Types, Entity, Assets, Shader, Anim, GfxCtx | `bestow.graphics3d` / `.core` | Dual |
| **UIRenderBackend** | Protected | 5 | Types, GraphicsContext | — | Single |
| **Particles** | Public | 5 | Types, Entity, Graphics3D | `bestow.particles` / `.core` | Dual |
| **UI** | Public | 6 | Types, UIRenderBackend, GfxCtx, Input, Assets | `bestow.ui` / `.core` | Dual |
| **Scene** | Public | 6 | Types, Entity, Assets, Blueprints, Events, Input | `bestow.scene` / `.core` | Dual |
| **GameState** | Public | 6 | Types, Input, Events | `bestow.gamestate` | Single |

## Rules

1. **A system may ONLY depend on systems in the same tier or lower.** No upward dependencies.
2. **Public systems depend on contracts, never implementations.** `IAssetCore*`, not `AssetSystemImpl*`.
3. **Protected systems are injected into public systems via `SystemContext`.** Public systems never construct protected systems.
4. **All file I/O goes through Assets.** No other system touches `std::filesystem` (except State for save files — documented exception).
5. **Inter-system communication goes through Events.** Direct callbacks only for performance-critical same-system paths.
6. **The Engine (Tier 7) is the only place that knows about concrete implementations.** It builds the `SystemContext` and runs the game loop.

## Game Loop Phase Assignments

Each system runs in a specific phase of the game loop:

```
EarlyUpdate     Input, Assets.checkReloads(), Events.processQueue(), Network.receive()
FixedUpdate     Physics2D, Physics3D, Network.tickSimulation()
Update          Lua app.update(), AI, Animation, Tween, GAS, Scene, GameState
LateUpdate      Camera, Particles, Lua app.lateUpdate()
PreRender       Culling, render queue sorting
Render          Graphics.beginFrame(), Scene.render(), Particles.render(),
                UI.render(), DebugRenderer.flush(), Graphics.endFrame()
PostRender      Network.send(), Metrics.markFrame(), State.update()
```

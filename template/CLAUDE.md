# Bestow Game Development Guide

Lua-first game engine. Games are written entirely in Lua and run via the `bestow` CLI.

```bash
bestow run main.lua                # Run game
bestow run main.lua --hot-reload   # Run with live code reloading
bestow run main.lua --debug        # Run with debug overlay
```

## Architecture

| Namespace | Purpose | Hot Reload |
|-----------|---------|------------|
| `bestow.*` | C++ engine API (graphics, physics, audio, input, animation, UI, timers, events) | No |
| `app.*` | Your game scripts (auto-discovered from project folders) | Yes |

Lua is the **director** - it tells the engine what to do via handles and commands. Assets are paths/handles; the engine manages the data. **NEVER reimplement functionality that the engine provides.**

## Project Structure

```
my-game/
├── main.lua              # Entry point (required)
├── entities/             # Entity blueprints -> app.entities.*
│   └── player.lua
├── systems/              # Game systems -> app.systems.*
│   └── movement.lua
├── levels/               # Level definitions -> app.levels.*
│   └── level1.lua
├── data/                 # Static game data -> app.data.*
│   └── items.lua
└── assets/               # Game assets (textures, sounds, models, materials)
```

Files are auto-discovered and namespaced by path: `entities/player.lua` -> `app.entities.player`

## main.lua Template

```lua
return {
    title = "My Game",
    width = 1280,
    height = 720,

    init = function()
        app.main.state = { player = nil, score = 0, running = true }
        -- Set up camera, lighting, load assets, create entities
    end,

    update = function(dt)
        local state = app.main.state
        -- Handle input, update game logic, physics
        return state.running  -- Return false to quit
    end,

    render = function()
        bestow.graphics3d.beginFrame()
        -- Draw meshes, UI, debug info
        bestow.graphics3d.endFrame()
    end,

    run = function()
        app.main.init()
        while app.main.update(bestow.core.deltaTime()) do
            app.main.render()
        end
    end
}
```

## Critical Rules

### 1. Hot Reload Safety
**ALWAYS access `app.*` inside functions, NEVER at file scope:**
```lua
-- WRONG: Stale after reload
local movement = app.systems.movement

-- CORRECT: Fresh reference each call
return {
    update = function(dt)
        local movement = app.systems.movement
        movement.update(dt)
    end
}
```

### 2. State Survives Hot Reload
Store ALL mutable game state in `app.main.state`. File-scope constants (GRAVITY, MAX_SPEED) are fine.

### 3. Assets Are Handles
```lua
local handle = bestow.assets.registerAsset(AssetType.Sound, "sounds/jump.wav")
bestow.assets.loadAsset(handle)
```
The `io.*`, `require()`, `dofile()`, `loadfile()`, `load()`, `os.execute()` functions are all **disabled**.

### 4. Controls (Dvorak + QWERTY)
Use the Action Builder for input. Always bind BOTH Dvorak (,AOE) and QWERTY (WASD):
```lua
local k = KeyCode
bestow.action.builder():duringPhase("gameplay"):whenActive(k.Comma):emitAction("MoveForward"):continuously()
bestow.action.builder():duringPhase("gameplay"):whenActive(k.W):emitAction("MoveForward"):continuously()
```

| Action | Dvorak | QWERTY |
|--------|--------|--------|
| Forward | `,` (Comma) | W |
| Back | O | S |
| Left | A | A |
| Right | E | D |

## Common Gotchas

1. **Must return from update()** - Return `true` to continue, `false` to quit
2. **Caching `app.*` at file scope** - Breaks hot reload
3. **Forgetting `beginFrame()`/`endFrame()`** - Nothing renders
4. **Not checking nil returns** - Many functions return nil on failure
5. **Using WASD only** - Always support Dvorak (,AOE)
6. **Reimplementing engine features** - Use `bestow.timer`, `bestow.ui`, `bestow.events` etc.
7. **Using `os.clock()`** - Use `bestow.core.time()` instead

## Quick API Cheatsheet

```
bestow.core          deltaTime(), time(), frameCount()
bestow.entity        create(), destroy(), addComponent(), getComponent(), setComponent(),
                     getField(), setField(), hasComponent(), each(), count(), isValid()
bestow.action        builder():duringPhase():whenPressed/Active/Released():emitAction():discretely/continuously()
bestow.input         isActionActive(), getActionValue(), getMousePosition(), getMouseDelta(),
                     isKeyDown(), wasKeyJustPressed(), setCursorMode(), showMouseCursor()
bestow.graphics3d    beginFrame(), endFrame(), setCamera(), drawMesh(), createMaterial(),
                     createCubeMesh/SphereMesh/PlaneMesh(), setDirectionalLight(),
                     setAmbientLight(), setFog(), debugDrawLine/Box/Sphere()
bestow.physics3d     createBody(), createCharacter(), moveCharacter(), getCharacterGroundInfo(),
                     raycast(), applyForce/Impulse(), setGravity(), overlapSphere/Box()
bestow.physics       createBody(), setPosition/Velocity(), raycast(), checkGrounded(), (2D Box2D)
bestow.audio         playOnChannel(), playPositional(), setListener(), setMasterVolume()
bestow.animation     createSkeleton(), createAnimator(), play(), stop(), setLayerWeight(),
                     defineSocket(), getSocketTransform(), defineIKChain(), setIKTarget(),
                     loadCharacter(), subscribeToEvents(), subscribeToComplete()
bestow.assets        registerAsset(), loadAsset(), loadAssetAsync(), isLoaded(),
                     loadModel(), getAnimationNames(), enableHotReload(), listLibraryAssets()
bestow.ui            loadDocument(), getElementById(), setElementText(), addClass(),
                     setAttribute(), setStyle(), onElementEvent(), showDocument()
bestow.events        subscribe(), emit(), unsubscribe()
bestow.timer         after(), every(), cancel(), pause(), resume(), update()
bestow.config        loadConfig(), getFloat/Int/Bool/String(), enableHotReload()
bestow.metrics       beginZone(), endZone(), plot(), message()
```

### Key Type Constructors
```
Vec2.new(x, y)              Vec3.new(x, y, z)           Vec4.new(x, y, z, w)
Quat.identity()             Quat.fromAxisAngle(axis, r)  Quat.fromEuler(p, y, r)
Color.new(r, g, b, a)       Transform3D.identity()       Mat4.identity()
AABB3D.new(min, max)         Ray3D.new(origin, dir)
```

### Key Enums
```
AssetType    .Texture .Sound .Music .Font .Mesh .Model .Material .Shader .Cubemap
BodyType3D   .Static .Kinematic .Dynamic
BlendMode    .Opaque .AlphaBlend .Additive
KeyCode      .Space .Escape .Enter .W .A .S .D .Comma .O .E .Up .Down .Left .Right
MouseButton  .Left .Right .Middle
GamepadButton .A .B .X .Y .LeftShoulder .RightShoulder
GamepadAxis  .LeftX .LeftY .RightX .RightY .TriggerLeft .TriggerRight
```

## Skills Reference

Claude has specialized skills for detailed API docs and patterns. These load automatically when relevant:

**START HERE:** architecture — Best practices for structuring a Bestow game. Read this before writing any code.

**Core Systems:** entity-system, input-system, graphics-system, audio-system, physics-system, animation-system, assets-system, camera-system, ui-and-menus, timer-system, events-system, math-types

**Game Patterns:** player-controller, enemies-and-ai, collectibles-and-items, game-state, level-system, save-system

**Development:** hot-reload, project-structure

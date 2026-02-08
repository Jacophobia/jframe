# Bestow Game Development Guide

Lua-first game engine. Games are written entirely in Lua and run via the `bestow` CLI.

```bash
bestow run main.lua                # Run game
bestow run main.lua -d             # Run with debug mode (extra logging)
bestow run main.lua -v             # Run with verbose logging
```

## Architecture

| Namespace | Purpose | Hot Reload |
|-----------|---------|------------|
| `bestow.*` | C++ engine API (graphics, physics, audio, input, animation, UI, timers, events) | No |
| `app.*` | Your game scripts (auto-discovered from project folders) | Yes |

Lua is the **director** — it tells the engine what to do via handles and commands. Assets are paths/handles; the engine manages the data. **NEVER reimplement functionality that the engine provides.**

### Config-First Philosophy

Engine defaults live in **`config/*.cfg.lua`** files. Each file returns a plain Lua table — no API calls, no side effects, pure data. The **engine reads these files directly in C++** during boot to initialise systems. Config files are NOT part of the Lua `app.*` runtime — they cannot reference other Lua files and are not accessible from game scripts. The `bestow.*` API is for **overrides** at runtime; config files handle defaults.

```lua
-- config/graphics.cfg.lua  (returns data, never calls bestow.*)
return {
    window = { title = "My Game", width = 1920, height = 1080, ... },
    rendering = { clearColor = { r = 0.05, g = 0.05, b = 0.08, a = 1.0 }, ... },
}
```

Config files are engine-required. The engine will not run without `config/input.cfg.lua`. Graphics, audio, and UI configs are optional (engine falls back to defaults).

## Project Structure

```
my-game/
├── main.lua              # Entry point (required)
├── config/               # System configuration files (each returns a table)
│   ├── graphics.cfg.lua  #   Window, rendering, camera, lighting, post-processing
│   ├── audio.cfg.lua     #   Volumes, channel groups
│   ├── input.cfg.lua     #   Action definitions as data table
│   ├── ui.cfg.lua        #   UI scale, theme path, font paths
│   └── save.cfg.lua      #   Save system config (stub)
├── scenes/               # Scene definitions -> lifecycle table
│   ├── main_menu.lua     #   Main menu: Play, Controls, Settings, Quit
│   ├── play.lua          #   Gameplay scene
│   ├── pause.lua         #   Pause overlay
│   ├── controls.lua      #   Controls / rebinding screen
│   └── settings.lua      #   Settings: display, audio, graphics
├── systems/              # Reusable game systems -> app.systems.*
│   └── menu_nav.lua      #   Keyboard/gamepad menu focus management
├── entities/             # Entity blueprints -> app.entities.*
├── data/                 # Static game data -> app.data.*
└── assets/               # Game assets
    ├── ui/               #   RML documents + RCSS theme
    ├── fonts/            #   Fonts
    └── sounds/           #   Sound effects and music
```

Files are auto-discovered and namespaced by path: `entities/player.lua` -> `app.entities.player`

## Scene System

Scenes are Lua files that return a lifecycle table. The engine manages a **stack** — the topmost scene is active; others are paused. The scene system auto-manages input phases and UI documents.

### Scene Lifecycle Table

```lua
-- scenes/my_scene.lua
return {
    phase = "gameplay",                       -- Input phase (auto-pushed/popped)
    ui    = { "assets/ui/my_scene.rml" },     -- UI docs (auto-loaded/shown/hidden)

    init   = function(params) end,   -- First load
    enter  = function(params) end,   -- Became active (params from push/replace)
    update = function(dt) return true end,  -- Each frame (return false to pop)
    render = function() end,         -- Each frame, after update
    exit   = function() end,         -- Lost active status (pushed down or popped)
}
```

### Stack Operations

```lua
bestow.scene.push("pause")                        -- Add on top (current scene paused)
bestow.scene.push("gameplay", { level = 1 })       -- With params
bestow.scene.pop()                                  -- Remove top (previous resumes)
bestow.scene.replace("level2", { from = "level1" }) -- Swap top scene
bestow.scene.clear()                                -- Empty entire stack
```

### Scene-Scoped Events

Use `bestow.scene.subscribe()` instead of `bestow.events.subscribe()` inside scenes. Subscriptions are **automatically unsubscribed** when the scene exits — no manual cleanup needed.

```lua
-- scenes/my_scene.lua
return {
    phase = "gameplay",

    enter = function(params)
        -- These are cleaned up automatically when the scene exits
        bestow.scene.subscribe("action:Pause", function()
            bestow.scene.push("pause")
        end)
        bestow.scene.subscribe("menu:confirm", function(data)
            if data.id == "btn-quit" then app.main.state.running = false end
        end)
    end,

    exit = function()
        -- No manual unsubscribe needed!
    end,
}
```

**Important:** `bestow.scene.subscribe` is for scene-local events only. For entities and systems that live across scene transitions, use `bestow.events.subscribe` with the table+method pattern.

### Template Scenes

| Scene | Phase | Purpose |
|-------|-------|---------|
| `main_menu` | `menu` | Play, Controls, Settings, Quit |
| `play` | `gameplay` | Empty gameplay scaffold — add your game here |
| `pause` | `pause` | Dark overlay: Resume, Main Menu, Quit |
| `controls` | `controls` | Tabbed rebinding (Controller / Mouse & Keyboard) |
| `settings` | `settings` | Display, Audio, Graphics toggles and sliders |

## Input System

### Data-Driven Actions (config/input.cfg.lua)

Input bindings are declared as data in `config/input.cfg.lua`. The **engine reads this file during boot** and registers ActionBuilder calls in C++. No Lua loader needed.

```lua
-- config/input.cfg.lua (excerpt)
return {
    actions = {
        { action = "MoveForward", phases = {"gameplay"},
          keys = {"Comma", "W"},  mode = "continuous" },
        { action = "Jump", phases = {"gameplay"},
          keys = {"Space"}, buttons = {"A"}, mode = "discrete" },
    },
}
```

To add new actions, just add entries to the actions table. The engine handles registration.

### Consuming Actions

```lua
-- Event-driven (recommended)
bestow.events.subscribe("action:Jump", function() player.jump() end)

-- Polling (deprecated but available)
if bestow.input.isActionActive("MoveForward") then ... end
```

### Menu Navigation (systems/menu_nav.lua)

The UI system has no built-in keyboard/gamepad navigation. `menu_nav.lua` manages a `.focused` CSS class across a list of elements:

```lua
local nav = app.systems.menu_nav
nav.setItems(doc, {"btn-play", "btn-settings", "btn-quit"})
nav.focus("btn-play")
nav.onBack = function() bestow.scene.pop() end
```

It listens for `MenuUp`, `MenuDown`, `MenuConfirm`, `MenuBack` actions and emits `menu:confirm`, `menu:left`, `menu:right` events.

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

### 3. Config Files Are Pure Data
`.cfg.lua` files return tables. No `bestow.*` calls, no side effects. The engine reads them directly in C++ during boot.

### 4. Assets Are Handles
```lua
local handle = bestow.assets.registerAsset(AssetType.Sound, "sounds/jump.wav")
bestow.assets.loadAsset(handle)
```
The `io.*`, `require()`, `dofile()`, `loadfile()`, `load()`, `os.execute()` functions are all **disabled**.

### 5. Controls (Dvorak + QWERTY)
Always bind BOTH Dvorak (,AOE) and QWERTY (WASD). Add entries to `config/input.cfg.lua`:

| Action | Dvorak | QWERTY |
|--------|--------|--------|
| Forward | `,` (Comma) | W |
| Back | O | S |
| Left | A | A |
| Right | E | D |

### 6. Scene Phase Isolation
Don't call `bestow.phase.push/pop` manually when using scenes — the scene system handles it via the `phase` field.

## Common Gotchas

1. **Must return from update()** — Return `true` to continue, `false` to quit
2. **Caching `app.*` at file scope** — Breaks hot reload
3. **Forgetting `beginFrame()`/`endFrame()`** — Nothing renders
4. **Not checking nil returns** — Many functions return nil on failure
5. **Using WASD only** — Always support Dvorak (,AOE)
6. **Reimplementing engine features** — Use `bestow.timer`, `bestow.ui`, `bestow.events` etc.
7. **Using `os.clock()`** — Use `bestow.util.time()` for timestamps or `bestow.util.clock()` for high-precision timing
8. **Calling bestow.* in config files** — Config files return data tables only

## API Documentation

Full API docs are built into the CLI:

```bash
bestow api                          # List all systems
bestow api <system>                 # Show system details (methods, enums, types, properties)
bestow api <system>.<method>        # Show method details (params, returns, example)
bestow api search <query>           # Search across all APIs
bestow api --no-color               # Disable ANSI colors (for piping)
```

## Quick API Cheatsheet

```
bestow.core          deltaTime(), frameCount()
bestow.util          time(), clock(), date()
bestow.config        loadConfig(), getFloat/Int/Bool/String(), enableHotReload()
bestow.entity        create(), destroy(), addComponent(), getComponent(), setComponent(),
                     getField(), setField(), hasComponent(), each(), count(), isValid()
bestow.action        builder():duringPhase():whenPressed/Active/Released():emitAction():discretely/continuously()
bestow.input         getMousePosition(), getMouseDelta(), isKeyDown(), wasKeyJustPressed(),
                     setCursorMode(), showMouseCursor(), startListeningForInput(), getLastInput()
                     [DEPRECATED: isActionActive(), getActionValue(), wasActionJustPressed()
                      -- prefer event subscriptions via bestow.events.subscribe()]
bestow.phase         push(), pop(), current(), stack(), change(), isActive()
bestow.scene         register(), push(), pop(), replace(), clear(), active(), stack(), state(), registered(),
                     subscribe()
bestow.graphics3d    beginFrame(), endFrame(), setCamera(), drawMesh(), createMaterial(),
                     createCubeMesh/SphereMesh/PlaneMesh(), setDirectionalLight(),
                     setAmbientLight(), setFog(), setFullscreen(), setClearColor(),
                     debugDrawLine/Box/Sphere()
bestow.physics3d     createBody(), createCharacter(), moveCharacter(), getCharacterGroundInfo(),
                     raycast(), applyForce/Impulse(), setGravity(), overlapSphere/Box()
bestow.physics       createBody(), setPosition/Velocity(), raycast(), checkGrounded(), (2D Box2D)
bestow.audio         playOnChannel(), playPositional(), setListener(), setMasterVolume(),
                     setGroupVolume(), pauseAll(), resumeAll()
bestow.animation     createSkeleton(), createAnimator(), play(), stop(), setLayerWeight(),
                     defineSocket(), getSocketTransform(), defineIKChain(), setIKTarget(),
                     loadCharacter(), subscribeToEvents(), subscribeToComplete()
bestow.assets        registerAsset(), loadAsset(), loadAssetAsync(), isLoaded(),
                     loadModel(), getAnimationNames(), enableHotReload(), listLibraryAssets()
bestow.ui            loadDocument(), loadStyleSheet(), getElementById(), getElementsByClass(),
                     setElementText(), addClass(), removeClass(), hasClass(),
                     setAttribute(), setStyle(), onElementEvent(), showDocument(),
                     hideDocument(), initialize(), update(), render(), focus(), blur()
bestow.events        subscribe(), emit(), unsubscribe()
bestow.timer         after(), every(), cancel(), pause(), resume(), update()
bestow.metrics       beginZone(), endZone(), plot(), message()
```

### Key Type Constructors
```
Vec2.new(x, y)              Vec3.new(x, y, z)           Vec4(x, y, z, w)
Quat.identity()             Quat.fromAxisAngle(axis, r)  Quat.fromEuler(pitch, yaw, roll)
Color.new(r, g, b, a)       Transform3D.identity()       Mat4.identity()
AABB3D.new(min, max)         Ray3D.new(origin, dir)
```

### Key Enums
```
AssetType      .Texture .Sound .Music .Font .Scene .Mesh .Model .Material .Shader .Cubemap
BodyType3D     .Static .Kinematic .Dynamic
BlendMode      .Opaque .AlphaBlend .Additive
KeyCode        .Space .Escape .Enter .W .A .S .D .Comma .O .E .Up .Down .Left .Right
MouseButton    .Left .Right .Middle
GamepadButton  .A .B .X .Y .LeftShoulder .RightShoulder .Start .Back .DPadUp .DPadDown .DPadLeft .DPadRight
GamepadAxis    .LeftX .LeftY .RightX .RightY .TriggerLeft .TriggerRight
```

## Skills Reference

Claude has specialized skills for detailed API docs and patterns. These load automatically when relevant:

**START HERE:** architecture — Best practices for structuring a Bestow game. Read this before writing any code.

**Core Systems:** entity-system, input-system, graphics-system, audio-system, physics-system, animation-system, assets-system, camera-system, scene-system, ui-and-menus, timer-system, events-system, math-types

**Game Patterns:** player-controller, enemies-and-ai, collectibles-and-items, game-state

**Development:** hot-reload, project-structure

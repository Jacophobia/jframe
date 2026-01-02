# Bestow Game Development Guide

This is a **Lua-first game template** for the Bestow engine. Games are written entirely in Lua and run via the `bestow` CLI tool.

## Quick Start

```bash
# Run the game
bestow run main.lua

# Run with hot reload (recommended during development)
bestow run main.lua --hot-reload
```

## Core Architecture

**Bestow uses two Lua namespaces:**

| Namespace | Purpose | Example |
|-----------|---------|---------|
| `bestow.*` | C++ engine API (graphics, physics, audio, input) | `bestow.entity.create()` |
| `app.*` | Your game scripts (auto-discovered, hot-reloadable) | `app.entities.player.create()` |

**Key principle:** Lua is the **director**. It tells the engine what to do, but never handles raw data like file contents or pixel buffers. Assets are referenced by path/handle and the engine manages the actual data.

## Project Structure

```
my-game/
├── main.lua                 # Entry point (required)
├── entities/                # Entity blueprints → app.entities.*
│   ├── player.lua
│   └── enemies/
│       └── goblin.lua       → app.entities.enemies.goblin
├── systems/                 # Game systems → app.systems.*
│   ├── movement.lua
│   └── combat.lua
├── levels/                  # Level definitions → app.levels.*
│   └── level1.lua
└── assets/                  # Game assets (textures, sounds, etc.)
    ├── textures/
    ├── sounds/
    └── materials/
```

## main.lua Structure

Every game must have a `main.lua` that returns a table:

```lua
return {
    -- Window configuration
    title = "My Game",
    width = 1280,
    height = 720,

    -- Called once at startup
    init = function()
        app.main.state = {
            player = app.entities.player.create(),
            score = 0,
            running = true
        }
    end,

    -- Called every frame (return false to quit)
    update = function(dt)
        local state = app.main.state
        app.systems.movement.update(dt)
        return state.running
    end,

    -- Called every frame for rendering
    render = function()
        bestow.graphics3d.beginFrame()
        -- Entities with renderers are drawn automatically
        bestow.graphics3d.endFrame()
    end,

    -- Main entry point
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

**Always access `app.*` inside functions, NEVER at file scope:**

```lua
-- WRONG: Cached reference breaks on hot reload
local movement = app.systems.movement
return {
    update = function(dt)
        movement.update(dt)  -- STALE after reload!
    end
}

-- CORRECT: Fresh reference each call
return {
    update = function(dt)
        local movement = app.systems.movement
        movement.update(dt)
    end
}
```

### 2. Assets Are Handles, Not Data

**Never try to read file contents directly. Pass asset paths to engine systems:**

```lua
-- WRONG: Trying to load file data
local fileData = io.open("sounds/jump.wav")  -- io is disabled!

-- CORRECT: Let the engine handle asset loading
bestow.audio.playOnChannel(Channels.UI, {
    path = "sounds/jump.wav",
    volume = 1.0
})

-- CORRECT: For textures/meshes, reference by path
bestow.entity.addComponent(entity, "MeshRenderer", {
    mesh = "meshes/player.obj",
    material = "materials/player"
})
```

### 3. State Lives in app.main.state

**Game state should be stored in `app.main.state` to survive hot reloads:**

```lua
-- In init()
app.main.state = {
    player = nil,
    enemies = {},
    score = 0,
    currentLevel = 1
}

-- Everywhere else
local state = app.main.state
state.score = state.score + 100
```

### 4. Disabled Functions

These Lua functions are disabled for security and hot-reload correctness:

- `require()` → Use `app.*` instead
- `dofile()`, `loadfile()`, `load()` → Dynamic code not allowed
- `io.*` → No file I/O (use bestow.assets)
- `os.execute()` → No shell access

## Controls (Dvorak-Friendly)

The project owner uses Dvorak. Default movement should support both layouts:

| Action | Dvorak | QWERTY | Arrow Keys |
|--------|--------|--------|------------|
| Forward | `,` (comma) | W | Up |
| Back | `O` | S | Down |
| Left | `A` | A | Left |
| Right | `E` | D | Right |

```lua
-- Handle both layouts
local forward = bestow.input.isKeyDown(Keys.Comma) or bestow.input.isKeyDown(Keys.W)
local back = bestow.input.isKeyDown(Keys.O) or bestow.input.isKeyDown(Keys.S)
local left = bestow.input.isKeyDown(Keys.A)  -- Same on both
local right = bestow.input.isKeyDown(Keys.E) or bestow.input.isKeyDown(Keys.D)
```

## Available Skills

Claude has access to skills for all common game development tasks. These are automatically invoked when relevant:

### Core Systems
- **entity-system** - Creating entities, adding/removing components
- **input-system** - Keyboard, mouse, controller input
- **graphics-system** - 3D rendering, cameras, lighting, fog
- **audio-system** - Music, sound effects, positional audio
- **physics-system** - 2D/3D physics, collision, raycasting
- **camera-system** - Camera following, shake effects, zoom
- **assets-system** - Loading and managing game assets
- **animation-system** - Skeletal animation, playback control

### Game Features
- **level-system** - Loading levels, spawn points
- **save-system** - Saving and loading game progress
- **game-state** - Menu states, pause, transitions

### Common Patterns
- **player-controller** - Movement, jumping, abilities
- **enemies-and-ai** - Enemy behavior, pathfinding
- **collectibles-and-items** - Pickups, inventory
- **ui-and-menus** - HUD, menus, text

### Development
- **hot-reload** - Hot reload patterns and best practices
- **project-structure** - Organizing game code

## Type Reference

### Math Types (Global)
```lua
Vec2.new(x, y)
Vec3.new(x, y, z)
Vec4.new(x, y, z, w)
Quat.identity()
Quat.fromAxisAngle(axis, angle)  -- axis: Vec3, angle: radians
Color.new(r, g, b, a)            -- 0.0-1.0
Mat4.identity()
Mat4.lookAt(eye, target, up)
Mat4.perspective(fov, aspect, near, far)
```

### Vector Operations
```lua
local v = Vec3.new(1, 2, 3)
v:length()      -- magnitude
v:normalize()   -- unit vector
v + other       -- addition
v - other       -- subtraction
v * scalar      -- scale
v.x, v.y, v.z   -- component access
```

### Key Constants
```lua
Keys.Space, Keys.Escape, Keys.Enter, Keys.Tab
Keys.W, Keys.A, Keys.S, Keys.D
Keys.Comma, Keys.O, Keys.E  -- Dvorak equivalents
Keys.Up, Keys.Down, Keys.Left, Keys.Right
Keys.LeftShift, Keys.LeftCtrl, Keys.LeftAlt
Keys.Num0 through Keys.Num9
Keys.F1 through Keys.F12
```

### Audio Channels
```lua
Channels.Music    -- Background music (channel 0)
Channels.Ambience -- Environmental sounds (channel 1)
Channels.UI       -- Interface sounds (channel 2)
Channels.Voice    -- Dialogue (channel 3)
```

## Debugging

```bash
# Run with debug overlay
bestow run main.lua --debug

# Force hot reload of all files
# Press F5 during gameplay

# View FPS and stats
# Press F1 during gameplay
```

## Common Gotchas

1. **Forgetting to return from update()** - Must return true to continue, false to quit
2. **Caching app.* references at file scope** - Breaks hot reload
3. **Trying to read files directly** - Use asset paths instead
4. **Not storing state in app.main.state** - State lost on hot reload
5. **Using WASD only** - Remember Dvorak users (,AOE)

## Next Steps

1. Create entity blueprints in `entities/`
2. Create game systems in `systems/`
3. Define levels in `levels/`
4. Add assets to `assets/`
5. Run with `bestow run main.lua --hot-reload`

For detailed API documentation, see the engine's `docs/Data-Driven-Design.md`.

# Getting Started with Bestow

Bestow is a **Lua-first game engine**. Write your game in Lua, run it instantly with `bestow run`.

> **Looking for C++ documentation?** See [Using Bestow as a C++ Library](Using-CPP-Library.md) for integrating Bestow into C++ projects.

## Prerequisites

**For Lua game development**, you only need:
- The `bestow` CLI tool

**Installation:**
```bash
# macOS (Homebrew)
brew install bestow

# Or build from source - see docs/Installation.md
```

---

## Your First Game (5 Minutes)

### 1. Create a Project

```bash
bestow new my-game
cd my-game
```

This creates:
```
my-game/
├── main.lua           # Entry point
├── entities/          # Entity blueprints
├── systems/           # Game systems
├── levels/            # Level definitions
└── assets/            # Textures, sounds, etc.
```

### 2. Run It

```bash
bestow run main.lua
```

You should see a window with a basic scene. Press **Escape** to quit.

### 3. Edit and Reload

Open `main.lua` in your editor. Try changing the title:

```lua
return {
    title = "My Awesome Game",  -- Change this
    ...
}
```

With hot reload enabled, changes appear instantly:
```bash
bestow run main.lua --hot-reload
```

---

## Understanding main.lua

Every Bestow game has a `main.lua` that returns a table with these functions:

```lua
-- main.lua
return {
    -- Window configuration
    title = "My Game",
    width = 1280,
    height = 720,

    -- Called once at startup
    init = function()
        -- Create entities, load assets, set up game state
        local player = bestow.entity.create()
        bestow.entity.addComponent(player, "Transform3D", {
            position = Vec3.new(0, 1, 0)
        })

        -- Store state in app.main.state (survives hot reload)
        app.main.state = {
            player = player,
            score = 0
        }
    end,

    -- Called every frame
    update = function(dt)
        local state = app.main.state

        -- Handle input (,AOE for Dvorak, WASD for QWERTY)
        if bestow.input.isKeyDown(Keys.Comma) or bestow.input.isKeyDown(Keys.W) then
            local pos = bestow.entity.getField(state.player, "Transform3D", "position")
            pos.z = pos.z - 5 * dt
            bestow.entity.setField(state.player, "Transform3D", "position", pos)
        end

        -- Return false to quit
        return not bestow.input.wasKeyJustPressed(Keys.Escape)
    end,

    -- Called every frame for rendering
    render = function()
        bestow.graphics3d.beginFrame()
        -- Entities with MeshRenderer are drawn automatically
        bestow.graphics3d.endFrame()
    end,

    -- Main entry point
    run = function()
        local main = app.main
        main.init()

        while true do
            local dt = bestow.core.deltaTime()
            if not main.update(dt) then break end
            main.render()
        end
    end
}
```

---

## Adding Game Scripts

Create additional Lua files and they're automatically available via `app.*`:

### Entity Blueprints

```lua
-- entities/player.lua
return {
    create = function(position)
        local entity = bestow.entity.create()
        bestow.entity.addComponent(entity, "Transform3D", {
            position = position or Vec3.new(0, 1, 0)
        })
        bestow.entity.addComponent(entity, "MeshRenderer", {
            mesh = "primitives/cube",
            material = "materials/player"
        })
        return entity
    end
}

-- Use in main.lua:
local player = app.entities.player.create(Vec3.new(0, 1, 0))
```

### Game Systems

```lua
-- systems/movement.lua
return {
    speed = 5.0,

    update = function(dt)
        local self = app.systems.movement

        bestow.entity.each(function(entity)
            if bestow.entity.hasComponent(entity, "PlayerController") then
                self.handlePlayerInput(entity, dt)
            end
        end)
    end,

    handlePlayerInput = function(entity, dt)
        local self = app.systems.movement
        local movement = Vec3.new(0, 0, 0)

        if bestow.input.isKeyDown(Keys.Comma) then movement.z = -1 end
        if bestow.input.isKeyDown(Keys.O) then movement.z = 1 end
        if bestow.input.isKeyDown(Keys.A) then movement.x = -1 end
        if bestow.input.isKeyDown(Keys.E) then movement.x = 1 end

        if movement:length() > 0 then
            movement = movement:normalize() * self.speed * dt
            local pos = bestow.entity.getField(entity, "Transform3D", "position")
            bestow.entity.setField(entity, "Transform3D", "position", pos + movement)
        end
    end
}
```

### Levels

```lua
-- levels/dungeon.lua
return {
    name = "Dark Dungeon",

    spawn = Vec3.new(0, 1, 0),

    load = function()
        local self = app.levels.dungeon
        local player = app.entities.player.create(self.spawn)
        app.main.state.player = player

        -- Create environment
        app.entities.torch.create(Vec3.new(5, 2, 0))
        app.entities.torch.create(Vec3.new(-5, 2, 0))
    end
}
```

---

## Hot Reload

Bestow supports hot reload - edit Lua files and see changes without restarting.

```bash
bestow run main.lua --hot-reload
```

### Hot Reload Rules

**Always access `app.*` inside functions:**

```lua
-- WRONG: Cached at load time, breaks hot reload
local movement = app.systems.movement

return {
    update = function(dt)
        movement.update(dt)  -- Stale reference after reload!
    end
}

-- RIGHT: Resolved fresh each call
return {
    update = function(dt)
        local movement = app.systems.movement  -- Fresh reference
        movement.update(dt)
    end
}
```

**Store state in `app.main.state`:**

```lua
-- State persists across hot reloads
app.main.state = {
    player = player,
    score = 0
}
```

---

## Controls

Bestow uses **Dvorak-friendly** default controls:

| Action | Dvorak | QWERTY | Arrow Keys |
|--------|--------|--------|------------|
| Forward | `,` (comma) | W | Up |
| Back | `O` | S | Down |
| Left | `A` | A | Left |
| Right | `E` | D | Right |

---

## Lua API Quick Reference

### Types

```lua
Vec2.new(x, y)
Vec3.new(x, y, z)
Quat.identity()
Quat.fromAxisAngle(axis, angle)
Color.new(r, g, b, a)
Mat4.identity()
```

### Entity System

```lua
bestow.entity.create() -> Entity
bestow.entity.destroy(entity)
bestow.entity.isValid(entity) -> bool
bestow.entity.addComponent(entity, typeName, data)
bestow.entity.removeComponent(entity, typeName)
bestow.entity.hasComponent(entity, typeName) -> bool
bestow.entity.getComponent(entity, typeName) -> table
bestow.entity.setComponent(entity, typeName, data)
bestow.entity.getField(entity, typeName, fieldName) -> value
bestow.entity.setField(entity, typeName, fieldName, value)
bestow.entity.each(callback)
```

### Input

```lua
bestow.input.isKeyDown(key) -> bool
bestow.input.wasKeyJustPressed(key) -> bool
bestow.input.wasKeyJustReleased(key) -> bool
bestow.input.getMousePosition() -> Vec2
bestow.input.getMouseDelta() -> Vec2
bestow.input.isMouseButtonDown(button) -> bool
```

### Graphics 3D

```lua
bestow.graphics3d.beginFrame()
bestow.graphics3d.endFrame()
bestow.graphics3d.setCamera(camera)
bestow.graphics3d.setFog(fog)
bestow.graphics3d.drawMesh(mesh, material, transform)
```

### Audio

```lua
bestow.audio.loadSound(path) -> SoundHandle
bestow.audio.playSound(handle, volume, pitch)
bestow.audio.stopSound(handle)
bestow.audio.setMasterVolume(volume)
```

See [Data-Driven-Design.md](Data-Driven-Design.md) for the complete API reference.

---

## CLI Commands

```bash
bestow new <name>              # Create new project
bestow run <main.lua>          # Run game
bestow run --hot-reload        # Enable hot reload
bestow run --debug             # Enable debug overlay
bestow version                 # Show version info
bestow help                    # Show all commands
```

See [CLI.md](CLI.md) for the complete CLI reference.

---

## Example Games

Explore complete game implementations:

| Game | Description | Location |
|------|-------------|----------|
| Snake | Classic snake game in 3D | `games/snake-lua/` |
| Lua Demo | Basic 3D scene with controls | `games/lua-demo/` |

Run them:
```bash
bestow run games/snake-lua/main.lua
bestow run games/lua-demo/main.lua
```

---

## Next Steps

1. **Learn the full API** - [Data-Driven-Design.md](Data-Driven-Design.md)
2. **Study example games** - `games/` directory
3. **Explore system docs** - `docs/systems/` directory
4. **Join the community** - Report issues on GitHub

---

## Need C++?

For custom engine systems, maximum performance, or C++ integration:

- [Using Bestow as a C++ Library](Using-CPP-Library.md)
- [Architecture Guide](Architecture.md)
- [C++ API Reference](api/)

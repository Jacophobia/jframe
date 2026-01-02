# Lua API Reference

This is the complete API reference for writing games in Lua with Bestow.

> **Quick links:**
> - [Getting Started](Getting-Started.md) - First game tutorial
> - [CLI Reference](CLI.md) - Command-line options
> - [Example Games](../games/) - Complete game examples

Bestow uses a **Lua-first architecture** where games are defined entirely in Lua. The C++ engine provides systems (graphics, physics, audio), and game developers write their logic, entities, and levels in Lua scripts.

## Core Principle: Lua First

**Games are Lua programs that use the Bestow engine.**

```bash
# Run a game
bestow run games/my-game/main.lua
```

The Lua layer defines *everything* about the game:
- Game logic and systems
- Entity definitions and blueprints
- Levels and spawning
- Configuration and tuning

The C++ layer provides the *engine*:
- Rendering (Vulkan/OpenGL)
- Physics (Box2D, Jolt)
- Audio (FMOD)
- Input handling
- Entity/Component system

## Two Lua Namespaces

Bestow exposes two global Lua namespaces:

### `bestow.*` - Engine API (C++ backed)

Stable, C++-implemented contracts exposed to Lua:

```lua
-- Entity operations
local entity = bestow.entity.create()
bestow.entity.addComponent(entity, "Transform3D", { position = Vec3.new(0, 0, 0) })

-- Input queries
if bestow.input.isKeyDown(Keys.Space) then
    player:jump()
end

-- Graphics
local fog = Fog.new()
fog.enabled = true
fog.color = Color.new(0.5, 0.5, 0.5, 1)
fog.density = 0.01
fog.startDistance = 10
fog.endDistance = 100
bestow.graphics3d.setFog(fog)

-- Physics
bestow.physics3d.raycast(origin, direction, 100)
```

### `app.*` - Game Scripts (Your Code)

Your game scripts, auto-loaded and hot-reloadable:

```lua
-- In main.lua
local player = app.entities.player  -- Access other scripts
local camera = app.systems.camera

function main.update(dt)
    local movement = app.systems.movement
    movement.update(dt)  -- Always resolve fresh for hot reload
end
```

## Game Structure

```
my-game/
├── main.lua                    # Entry point (required)
├── app.config.lua              # Optional: folder ignore list
├── entities/
│   ├── player.lua             → app.entities.player
│   └── enemies/
│       ├── goblin.lua         → app.entities.enemies.goblin
│       └── dragon.lua         → app.entities.enemies.dragon
├── systems/
│   ├── camera.lua             → app.systems.camera
│   └── movement.lua           → app.systems.movement
├── levels/
│   └── dungeon.lua            → app.levels.dungeon
└── assets/
    ├── textures/
    ├── sounds/
    └── materials/
```

Files are auto-discovered and mapped to the `app.*` table based on their path.

## main.lua Structure

Every game needs a `main.lua` that returns a table:

```lua
-- main.lua
return {
    title = "My Game",
    width = 1280,
    height = 720,

    -- Called once at startup
    init = function()
        local player = app.entities.player
        app.main.state = {
            playerEntity = player.create(),
            score = 0
        }
    end,

    -- Called every frame
    update = function(dt)
        local state = app.main.state
        local movement = app.systems.movement
        movement.update(dt)
        return state.running  -- Return false to quit
    end,

    -- Called every frame after update
    render = function()
        bestow.graphics3d.beginFrame()
        -- Render game...
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

## Entity Definitions

Define entity blueprints in `entities/`:

```lua
-- entities/player.lua
return {
    -- Default component values
    defaults = {
        Transform3D = {
            position = Vec3.new(0, 1, 0),
            rotation = Quat.identity(),
            scale = Vec3.new(1, 1, 1)
        },
        PlayerController = {
            moveSpeed = 5.0,
            jumpForce = 10.0
        }
    },

    -- Factory function
    create = function(overrides)
        local self = app.entities.player  -- Hot reload safe!
        local entity = bestow.entity.create()

        for compName, defaults in pairs(self.defaults) do
            local data = {}
            for k, v in pairs(defaults) do data[k] = v end
            if overrides and overrides[compName] then
                for k, v in pairs(overrides[compName]) do data[k] = v end
            end
            bestow.entity.addComponent(entity, compName, data)
        end

        return entity
    end
}
```

## System Definitions

Define game systems in `systems/`:

```lua
-- systems/movement.lua
return {
    gravity = Vec3.new(0, -9.81, 0),

    update = function(dt)
        local self = app.systems.movement  -- Hot reload safe!

        bestow.entity.each(function(entity)
            if not bestow.entity.hasComponent(entity, "Velocity") then return end

            local velocity = bestow.entity.getComponent(entity, "Velocity")
            local transform = bestow.entity.getComponent(entity, "Transform3D")

            -- Apply gravity
            velocity.linear = velocity.linear + self.gravity * dt

            -- Update position
            transform.position = transform.position + velocity.linear * dt

            bestow.entity.setComponent(entity, "Transform3D", transform)
            bestow.entity.setComponent(entity, "Velocity", velocity)
        end)
    end
}
```

## Level Definitions

Define levels in `levels/`:

```lua
-- levels/dungeon.lua
return {
    name = "Dark Dungeon",

    spawns = {
        player = Vec3.new(0, 1, 0)
    },

    lighting = {
        ambient = { color = Color.new(0.1, 0.1, 0.15, 1), intensity = 0.3 },
        sun = { direction = Vec3.new(-0.5, -1, -0.5), intensity = 0.5 }
    },

    objects = {
        { type = "torch", position = Vec3.new(5, 2, 0) },
        { type = "chest", position = Vec3.new(-3, 0, 5) },
    },

    load = function()
        local self = app.levels.dungeon
        local player = app.entities.player.spawnAt(self.spawns.player)
        app.main.state.player = player

        for _, obj in ipairs(self.objects) do
            -- Create level objects...
        end
    end
}
```

## Hot Reload Rules

**Critical**: Always access `app.*` inside functions, never at file scope:

```lua
-- WRONG: Cached at load time, breaks hot reload
local physics = app.systems.physics

return {
    update = function(dt)
        physics.step(dt)  -- Uses stale reference after reload!
    end
}

-- RIGHT: Resolved fresh each call
return {
    update = function(dt)
        local physics = app.systems.physics  -- Fresh reference
        physics.step(dt)
    end
}
```

The ScriptManager lints your code and warns about top-level `app.*` captures.

## Disabled Functions

For security and hot-reload correctness, these Lua functions are disabled:

- `require()` - Use `app.*` instead
- `dofile()` - Use `app.*` instead
- `loadfile()` - Use `app.*` instead
- `load()` - Dynamic code loading not allowed
- `io.*` - File I/O not allowed
- `os.execute()` - Shell access not allowed

## bestow.* API Reference

### Core Types

```lua
Vec2.new(x, y)
Vec3.new(x, y, z)
Vec4.new(x, y, z, w)
Quat.new(x, y, z, w)
Quat.identity()
Quat.fromAxisAngle(axis, angle)
Color.new(r, g, b, a)
Mat4.identity()
Mat4.lookAt(eye, target, up)
Mat4.perspective(fov, aspect, near, far)
```

### Entity System

```lua
bestow.entity.create() -> Entity
bestow.entity.destroy(entity)
bestow.entity.isValid(entity) -> bool
bestow.entity.addComponent(entity, typeName, data)
bestow.entity.removeComponent(entity, typeName)
bestow.entity.hasComponent(entity, typeName) -> bool
bestow.entity.getComponent(entity, typeName) -> table or nil
bestow.entity.setComponent(entity, typeName, data)
bestow.entity.getField(entity, typeName, fieldName) -> value
bestow.entity.setField(entity, typeName, fieldName, value)
bestow.entity.each(callback)
```

### Input System

```lua
-- Keyboard
bestow.input.isKeyDown(key) -> bool
bestow.input.wasKeyJustPressed(key) -> bool
bestow.input.wasKeyJustReleased(key) -> bool

-- Action System (configurable mappings)
bestow.input.isActionActive(action) -> bool
bestow.input.wasActionJustPressed(action) -> bool
bestow.input.wasActionJustReleased(action) -> bool
bestow.input.getActionValue(action) -> number  -- For analog inputs

-- Mouse
bestow.input.getMousePosition() -> Vec2
bestow.input.getMouseDelta() -> Vec2
bestow.input.isMouseButtonDown(button) -> bool
bestow.input.wasMouseButtonJustPressed(button) -> bool
bestow.input.getScrollDelta() -> Vec2

-- Modifiers
bestow.input.isShiftPressed() -> bool
bestow.input.isCtrlPressed() -> bool
bestow.input.isAltPressed() -> bool

-- Text Input (for UI text fields)
bestow.input.enableTextInput()
bestow.input.disableTextInput()
bestow.input.getTextInput() -> string

-- Controllers
bestow.input.getConnectedControllerCount() -> integer
bestow.input.isControllerConnected(index) -> bool

-- Key Constants: Use global Keys table
-- Keys.Space, Keys.Escape, Keys.W, Keys.A, Keys.S, Keys.D, etc.
-- Or use bestow.input.Key.SPACE, bestow.input.Key.ESCAPE, etc.
```

### Graphics 3D

```lua
bestow.graphics3d.beginFrame()
bestow.graphics3d.endFrame()
bestow.graphics3d.setCamera(camera)           -- Camera3D struct
bestow.graphics3d.setFog(fog)                 -- Fog struct
bestow.graphics3d.setAmbientLight(color, intensity)
bestow.graphics3d.drawMesh(mesh, material, transform)  -- Transform3D or Mat4
```

### Physics 3D

```lua
bestow.physics3d.createWorld(gravity) -> WorldHandle
bestow.physics3d.stepWorld(world, dt)
bestow.physics3d.createRigidBody(world, def) -> BodyHandle
bestow.physics3d.createCharacter(world, def) -> CharacterHandle
bestow.physics3d.raycast(world, from, to) -> RaycastResult or nil
```

### Audio

```lua
bestow.audio.loadSound(path) -> SoundHandle
bestow.audio.playSound(handle, volume, pitch)
bestow.audio.stopSound(handle)
bestow.audio.setMasterVolume(volume)
```

### Animation

```lua
bestow.animation.createInstance(entity, skeleton)
bestow.animation.setAnimation(entity, name, loop, speed)
bestow.animation.crossfade(entity, name, duration)
bestow.animation.getAnimationTime(entity) -> number
```

## Benefits

1. **Hot Reload**: Edit Lua, see changes instantly (no recompile)
2. **Designer-Friendly**: Non-programmers can create content
3. **Fast Iteration**: Tweak gameplay without waiting for builds
4. **Modding Support**: Players can easily modify games
5. **Version Control**: Lua changes are easy to diff and review
6. **Testing**: Create test scenarios without rebuilding

## Summary

**Your game is a Lua program. Bestow is the engine it runs on.**

- Define your game in `main.lua`
- Put entities in `entities/`
- Put systems in `systems/`
- Put levels in `levels/`
- Use `bestow.*` for engine features
- Use `app.*` for your own scripts
- Always access `app.*` inside functions (hot reload!)

# Getting Started with Bestow

Bestow is a **Lua-first game engine** where games are defined in Lua and powered by a C++23 engine. This guide shows how to create your first game.

## Prerequisites

- **CMake 3.28+** with C++23 module support
- **LLVM Clang 20+** (macOS) or **MSVC 19.38+** (Windows)
- **Vulkan SDK** installed
- **vcpkg** for dependencies

See `docs/Installation.md` for detailed setup.

## Quick Start (Lua)

### 1. Create Your Game Directory

```bash
# Create a new game project
bestow new my-game

# This creates:
# my-game/
# ├── main.lua
# ├── entities/
# ├── systems/
# ├── levels/
# └── assets/
```

### 2. Edit main.lua

```lua
-- my-game/main.lua
return {
    title = "My First Game",
    width = 1280,
    height = 720,

    init = function()
        print("Game starting!")

        -- Create player entity
        local player = bestow.entity.create()
        bestow.entity.addComponent(player, "Transform3D", {
            position = Vec3.new(0, 1, 0)
        })

        -- Store in game state
        app.main.state = {
            player = player,
            running = true
        }
    end,

    update = function(dt)
        local state = app.main.state

        -- Move player with ,AOE (Dvorak) or WASD
        local movement = Vec3.new(0, 0, 0)
        if bestow.input.isKeyDown(Keys.Comma) or bestow.input.isKeyDown(Keys.W) then
            movement.z = -1
        end
        if bestow.input.isKeyDown(Keys.O) or bestow.input.isKeyDown(Keys.S) then
            movement.z = 1
        end
        if bestow.input.isKeyDown(Keys.A) then
            movement.x = -1
        end
        if bestow.input.isKeyDown(Keys.E) or bestow.input.isKeyDown(Keys.D) then
            movement.x = 1
        end

        if movement:length() > 0 then
            movement = movement:normalize() * 5.0 * dt
            local pos = bestow.entity.getField(state.player, "Transform3D", "position")
            bestow.entity.setField(state.player, "Transform3D", "position", pos + movement)
        end

        -- ESC to quit
        if bestow.input.wasKeyJustPressed(Keys.Escape) then
            state.running = false
        end

        return state.running
    end,

    render = function()
        bestow.graphics3d.beginFrame()
        -- Rendering happens automatically for entities with MeshRenderer
        bestow.graphics3d.endFrame()
    end,

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

### 3. Run Your Game

```bash
bestow run my-game/main.lua
```

That's it! No compilation needed.

## Adding More Scripts

Create additional files and they're automatically available via `app.*`:

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

-- Use it in main.lua:
local player = app.entities.player.create(Vec3.new(0, 1, 0))
```

## Hot Reload

Edit any Lua file and save - changes apply immediately without restarting!

Just remember: always access `app.*` inside functions:

```lua
-- WRONG: breaks hot reload
local player = app.entities.player

-- RIGHT: hot reload safe
return {
    update = function(dt)
        local player = app.entities.player  -- Resolved fresh each call
    end
}
```

## Controls

Bestow uses **Dvorak-friendly** default controls:

| Action | Dvorak | QWERTY Equivalent |
|--------|--------|-------------------|
| Up | `,` (comma) | W |
| Down | `O` | S |
| Left | `A` | A |
| Right | `E` | D |

Arrow keys also work.

## Lua API Overview

### Types

```lua
Vec2.new(x, y)
Vec3.new(x, y, z)
Quat.identity()
Color.new(r, g, b, a)
Mat4.identity()
```

### Entity System

```lua
bestow.entity.create()
bestow.entity.destroy(entity)
bestow.entity.addComponent(entity, "ComponentName", {data})
bestow.entity.getComponent(entity, "ComponentName")
bestow.entity.hasComponent(entity, "ComponentName")
bestow.entity.each(function(entity) ... end)
```

### Input

```lua
bestow.input.isKeyDown(key)
bestow.input.wasKeyJustPressed(key)
bestow.input.isActionActive(action)
bestow.input.getMousePosition()
```

### Graphics

```lua
bestow.graphics3d.beginFrame()
bestow.graphics3d.endFrame()
bestow.graphics3d.setCamera(camera)           -- Camera3D struct
bestow.graphics3d.setFog(fog)                 -- Fog struct
bestow.graphics3d.drawMesh(mesh, material, transform)  -- Transform3D or Mat4
```

See `docs/Data-Driven-Design.md` for complete API reference.

---

## Alternative: C++ Approach

For maximum performance or when you need direct C++ control, you can also write games in C++:

### 1. Create Game Class

```cpp
// src/game.cppm
export module my.game;

import bestow.services;
import bestow.types;

export class MyGame : public bestow::Application<MyGame,
    bestow::IGraphics3DSystem,
    bestow::IInputSystem>
{
public:
    MyGame(bestow::IGraphics3DSystem& graphics, bestow::IInputSystem& input)
        : graphics_(&graphics), input_(&input) {}

    void run() override {
        // Initialize, game loop, cleanup
    }

private:
    bestow::IGraphics3DSystem* graphics_;
    bestow::IInputSystem* input_;
};
```

### 2. Create Entry Point

```cpp
// src/main.cpp
import bestow.core;
import bestow.services;
import bestow.vulkan.impl;
import bestow.input.impl;
import my.game;

int main() {
    bestow::core::Engine engine;
    engine.use<bestow::IGraphics3DSystem, bestow::VulkanGraphics3DSystem>();
    engine.use<bestow::IInputSystem, bestow::InputSystem>();
    engine.run<MyGame>();
    return 0;
}
```

### 3. Build and Run

```bash
cmake --preset macos-debug
cmake --build --preset macos-debug
./build/macos-debug/my-game
```

---

## Example Games

- **`games/lua-demo/`** - Simple Lua game demonstrating the Lua-first approach
- **`games/game1/`** - 3D Snake game in C++ (multi-executable approach)
- **`demos/animation-showcase/`** - Animation system demo

## Next Steps

- Read `docs/Data-Driven-Design.md` for the complete Lua API
- Study `games/lua-demo/` for a Lua game example
- Check `docs/api/` for system API reference
- See `CLAUDE.md` for development guidelines

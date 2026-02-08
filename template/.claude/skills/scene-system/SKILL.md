---
name: scene-system
description: Stack-based scene manager for controlling what's on screen (menus, gameplay, cutscenes). Use when implementing scene transitions, pause menus, loading screens, or multi-screen game flow.
---

# Scene System

The scene system is a **stack-based manager** that controls what's on screen at any given time. The topmost scene is active (receives update/render); others are paused and hidden.

Scenes are Lua files that return a table with lifecycle callbacks. The system auto-manages input phases (`bestow.phase`) and UI documents (`bestow.ui`) when scenes are pushed/popped.

## Scene Definition (Lua File)

Scenes are Lua files in your project. Each returns a table:

```lua
-- scenes/main_menu.lua
return {
    phase = "mainmenu",             -- Input phase (auto-pushed/popped)
    ui = { "ui/main_menu.rml" },    -- UI documents (auto-loaded/shown/hidden)

    init = function(params)
        -- Called once when scene is first registered
    end,

    enter = function(params)
        -- Called each time this scene becomes active (top of stack)
        -- params table comes from bestow.scene.push/replace
    end,

    update = function(dt)
        -- Called every frame while active
        return true  -- return false to request pop
    end,

    render = function()
        -- Called every frame while active, after update
    end,

    exit = function()
        -- Called when scene loses active status (pushed down or popped)
    end,

    shutdown = function()
        -- Called when scene is fully unregistered
    end
}
```

## Registration

Before a scene can be pushed, it must be registered with an asset path:

```lua
bestow.scene.register("main_menu", "scenes/main_menu.lua")
bestow.scene.register("gameplay", "scenes/gameplay.lua")
bestow.scene.register("pause", "scenes/pause.lua")
```

Registration loads the Lua file, parses its definition, and calls `init()` once.

## Stack Operations

### Push — Add a Scene on Top

```lua
-- Push with no params
bestow.scene.push("gameplay")

-- Push with params (passed to enter())
bestow.scene.push("gameplay", { level = 1, difficulty = "hard" })
```

**What happens:**
1. Current top scene: `exit()` called, UI hidden, state set to Paused
2. New scene: input phase pushed, UI loaded + shown, `enter(params)` called, state set to Active

### Pop — Remove the Top Scene

```lua
bestow.scene.pop()
```

**What happens:**
1. Top scene: `exit()` called, UI unloaded, input phase popped, state set to Ready
2. Previous scene: UI shown, `enter()` called (no params on resume), state set to Active

### Replace — Swap the Top Scene

```lua
bestow.scene.replace("level2", { from = "level1" })
```

**What happens:**
1. Top scene: `exit()` + `shutdown()` called, UI unloaded, input phase popped
2. New scene: input phase pushed, UI loaded + shown, `enter(params)` called

### Clear — Empty the Entire Stack

```lua
bestow.scene.clear()
```

Pops all scenes from top to bottom, calling `exit()` + `shutdown()` on each.

## Queries

```lua
-- Get the active scene name (or nil)
local name = bestow.scene.active()

-- Get the full stack as a table (bottom to top)
local stack = bestow.scene.stack()
for i, name in ipairs(stack) do
    bestow.debug("Stack[" .. i .. "] = " .. name)
end

-- Get scene state: "unloaded", "loading", "ready", "active", "paused", "unloading"
local state = bestow.scene.state("main_menu")

-- Get all registered scene names
local scenes = bestow.scene.registered()
```

## Auto-Management

### Input Phases

If a scene has a `phase` field, the system automatically pushes/pops the input phase when the scene enters/leaves the stack. This means your action bindings (defined with `bestow.action.builder():duringPhase("gameplay")`) automatically activate and deactivate.

### UI Documents

If a scene has a `ui` field (table of paths), the system automatically:
- **On push:** loads and shows the documents
- **On pause:** hides the documents (but keeps them loaded)
- **On resume:** shows the documents again
- **On pop:** hides and unloads the documents

### Hot Reload

Scene Lua files support hot reload. When a scene file is modified on disk:
1. The system re-parses the Lua file
2. If the scene is active: calls `exit()` on old definition, `enter()` on new definition
3. Publishes a `scene_reloaded` event

## Common Patterns

### Game with Menu and Gameplay

```lua
-- main.lua
return {
    init = function()
        -- Register all scenes
        bestow.scene.register("main_menu", "scenes/main_menu.lua")
        bestow.scene.register("gameplay", "scenes/gameplay.lua")
        bestow.scene.register("pause", "scenes/pause.lua")

        -- Start with main menu
        bestow.scene.push("main_menu")
    end,

    update = function(dt)
        bestow.timer.update(dt)
        return bestow.scene.active() ~= nil
    end,

    render = function()
        -- Scene system handles render callbacks
    end,

    run = function()
        app.main.init()
        while app.main.update(bestow.core.deltaTime()) do
            app.main.render()
        end
    end
}
```

### Pause Menu Overlay

```lua
-- scenes/pause.lua
return {
    phase = "pause",
    ui = { "ui/pause_menu.rml" },

    enter = function()
        -- Game is paused (gameplay scene frozen underneath)
    end,

    update = function(dt)
        -- Check for unpause
        return true
    end,

    exit = function()
        -- Resuming gameplay
    end
}
```

Push the pause menu on top of gameplay:
```lua
-- In gameplay scene or input handler:
bestow.scene.push("pause")
-- Later: bestow.scene.pop() to resume
```

### Level Transitions

```lua
-- scenes/level1.lua
return {
    phase = "gameplay",
    ui = { "ui/gameplay_hud.rml" },

    enter = function(params)
        if params and params.from == "level2" then
            app.systems.spawner.spawnAt(Vec3.new(50, 2, 0))
        else
            app.systems.spawner.spawnAt(Vec3.new(0, 2, 0))
        end
    end,

    update = function(dt)
        app.systems.player.update(dt)
        if app.main.state.levelComplete then
            bestow.scene.replace("level2", { from = "level1" })
        end
        return true
    end,

    exit = function()
        app.systems.spawner.destroyAll()
    end
}
```

### Update Returns False to Pop

```lua
return {
    update = function(dt)
        -- When update returns false, the scene is automatically popped
        if app.main.state.gameOver then
            return false
        end
        return true
    end
}
```

## Event Subscriptions

Scene lifecycle events are published via the event system:

```lua
bestow.events.subscribe("scene_pushed", function(data)
    bestow.debug("Scene pushed:", data.sceneName)
end)

bestow.events.subscribe("scene_popped", function(data)
    bestow.debug("Scene popped:", data.sceneName)
end)

bestow.events.subscribe("scene_replaced", function(data)
    bestow.debug("Scene replaced with:", data.sceneName)
end)

bestow.events.subscribe("scene_reloaded", function(data)
    bestow.debug("Scene reloaded:", data.sceneName)
end)
```

## Best Practices

1. **Register all scenes in `init()`** before pushing any
2. **Use `phase` for input isolation** — each scene gets its own input context
3. **Use `ui` for automatic document management** — no manual load/show/hide needed
4. **Use `replace` for level-to-level transitions** — cleans up the old scene fully
5. **Use `push` for overlays** (pause, inventory, dialog) — the previous scene is preserved
6. **Return `false` from `update()`** to automatically pop the scene
7. **Store game state in `app.main.state`** — scene callbacks can read/write it freely
8. **Don't call `bestow.phase.push/pop` manually** if using scene phases — the system handles it

---
name: hot-reload
description: Understand and use Bestow's hot reload system for rapid iteration. Use when implementing code that survives reloads, understanding the app.* namespace, or debugging reload issues.
---

# Hot Reload System

Bestow's hot reload system lets you modify Lua code and see changes instantly without restarting.

## How Hot Reload Works

```bash
# Run with hot reload enabled
bestow run main.lua --hot-reload
```

When you save a `.lua` file:
1. Engine detects the file change
2. The modified file is re-executed
3. Any `return { ... }` table replaces the old module
4. Game continues with new code

## The Critical Rule

**Access `app.*` inside functions, NEVER at file scope.**

```lua
-- WRONG: Cached reference breaks on hot reload
local movement = app.systems.movement  -- Captured once!

return {
    update = function(dt)
        movement.update(dt)  -- Uses stale reference after reload
    end
}

-- CORRECT: Fresh reference each call
return {
    update = function(dt)
        local movement = app.systems.movement  -- Always current
        movement.update(dt)
    end
}
```

### Why This Matters

When a file is reloaded:
1. A new table is created and stored in `app.systems.movement`
2. But your local variable `movement` still points to the OLD table
3. Your code runs the old, stale version despite the reload

## State Persistence

Use `app.main.state` to persist data across reloads:

```lua
-- systems/game.lua
return {
    init = function()
        local self = app.systems.game
        local state = app.main.state

        -- Initialize only if not already set (survives reload)
        if state.score == nil then
            state.score = 0
        end
        if state.level == nil then
            state.level = 1
        end
        if state.player == nil then
            state.player = app.entities.player.create(Vec3.new(0, 0, 0))
        end
    end,

    addScore = function(points)
        app.main.state.score = app.main.state.score + points
    end
}
```

### What Survives Reload

| Data Location | Survives Reload? | Use For |
|--------------|------------------|---------|
| `app.main.state` | Yes | Game state, score, player entity |
| Local variables | No | Temporary calculations |
| Module-level variables | No | Don't use these |
| Entity components | Yes | Entity data (managed by engine) |

## Safe Patterns

### Pattern 1: Self-Referencing Tables

```lua
-- systems/movement.lua
return {
    speed = 8.0,

    update = function(dt)
        local self = app.systems.movement  -- Get current self
        local state = app.main.state

        -- Use self.speed, not a captured variable
        local velocity = self.getInputVector() * self.speed * dt
        -- ...
    end,

    getInputVector = function()
        local self = app.systems.movement
        -- ...
    end
}
```

### Pattern 2: Lazy Initialization

```lua
-- systems/spawner.lua
return {
    spawnTimer = nil,  -- Will be set on first use

    update = function(dt)
        local self = app.systems.spawner

        -- Initialize on first call
        if self.spawnTimer == nil then
            self.spawnTimer = 0
        end

        self.spawnTimer = self.spawnTimer - dt
        if self.spawnTimer <= 0 then
            self.spawn()
            self.spawnTimer = 2.0
        end
    end,

    spawn = function()
        local self = app.systems.spawner
        -- ...
    end
}
```

### Pattern 3: Event Subscription Cleanup

```lua
-- systems/damage.lua
return {
    subscriptionId = nil,

    init = function()
        local self = app.systems.damage

        -- Clean up old subscription if reloading
        if self.subscriptionId then
            bestow.events.unsubscribe(self.subscriptionId)
        end

        -- Subscribe fresh
        self.subscriptionId = bestow.events.subscribe("collision", function(event)
            app.systems.damage.onCollision(event)  -- Use full path in callback!
        end)
    end,

    onCollision = function(event)
        local self = app.systems.damage
        -- Handle collision
    end
}
```

### Pattern 4: Timer/Callback Closures

```lua
-- WRONG: Closure captures stale reference
return {
    startTimer = function()
        local self = app.systems.game  -- Captured here
        bestow.timer.after(1.0, function()
            self.onTimerComplete()  -- Stale after reload!
        end)
    end
}

-- CORRECT: Resolve reference inside callback
return {
    startTimer = function()
        bestow.timer.after(1.0, function()
            app.systems.game.onTimerComplete()  -- Fresh reference
        end)
    end,

    onTimerComplete = function()
        local self = app.systems.game
        -- ...
    end
}
```

## Debugging Hot Reload Issues

### Symptom: Old behavior persists after save

**Cause:** Captured reference at file scope

```lua
-- Check for this pattern:
local something = app.systems.something  -- BAD: file scope capture

-- Fix: Move inside functions
```

### Symptom: nil reference errors after reload

**Cause:** Entity or resource was destroyed but reference kept

```lua
-- Problem:
local player = app.main.state.player
if not bestow.entity.isValid(player) then
    -- Entity was destroyed, recreate
    app.main.state.player = app.entities.player.create(spawnPos)
end
```

### Symptom: Duplicate subscriptions

**Cause:** Not cleaning up old subscriptions

```lua
-- Always clean up in init:
if self.subscriptionId then
    bestow.events.unsubscribe(self.subscriptionId)
end
self.subscriptionId = bestow.events.subscribe(...)
```

### Symptom: State resets on reload

**Cause:** Using local/module variables instead of `app.main.state`

```lua
-- BAD: Resets on reload
local score = 0

-- GOOD: Persists across reloads
app.main.state.score = app.main.state.score or 0
```

## Hot Reload Events

Listen for reload events to perform cleanup or reinitialization:

```lua
-- systems/game.lua
return {
    init = function()
        local self = app.systems.game

        bestow.events.subscribe("lua_reload", function(event)
            app.systems.game.onReload(event.path)
        end)
    end,

    onReload = function(path)
        local self = app.systems.game
        print("Reloaded: " .. path)

        -- Reinitialize anything that depends on the reloaded file
        if path:match("levels/") then
            app.systems.level.reload()
        end
    end
}
```

## Asset Hot Reload

Assets (textures, sounds, meshes) also hot reload:

```lua
-- No code changes needed - engine handles this automatically
bestow.entity.addComponent(entity, "MeshRenderer", {
    mesh = "meshes/player.obj",     -- Auto-reloads when file changes
    material = "materials/player"   -- Auto-reloads when .lua changes
})
```

### Responding to Asset Reloads

```lua
bestow.events.subscribe("asset_reloaded", function(event)
    if event.type == "Material" then
        print("Material reloaded: " .. event.path)
        -- Possibly refresh cached material references
    end
end)
```

## main.lua and Callbacks

The `main.lua` file has a special structure for hot reload:

```lua
-- main.lua (entry point)
return {
    init = function()
        -- Called once on startup
        -- Safe to initialize state here
        app.main.state.initialized = true
    end,

    update = function(dt)
        -- Called every frame
        -- Always get fresh references
        app.systems.movement.update(dt)
        app.systems.combat.update(dt)
    end,

    render = function()
        -- Called every frame after update
        app.systems.renderer.render()
    end,

    shutdown = function()
        -- Called on exit
        app.systems.save.saveGame()
    end
}
```

## Best Practices Summary

1. **Never cache `app.*` at file scope** - Always access inside functions
2. **Use `app.main.state` for persistent data** - Survives reloads
3. **Clean up subscriptions in `init`** - Prevent duplicates
4. **Use full paths in callbacks** - `app.systems.x.method()` not `self.method()`
5. **Check entity validity** - Entities can be destroyed
6. **Use `local self = app.systems.name`** - Get fresh reference at start of each function

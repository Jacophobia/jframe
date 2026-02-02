---
name: timer-system
description: Schedule delayed and repeating callbacks using bestow.timer. Use when implementing cooldowns, spawn timers, delayed actions, repeating effects, or any time-based game logic.
---

# Timer System

The timer system provides one-shot and repeating callbacks. Use this instead of tracking time manually or using `os.clock()`.

**IMPORTANT:** You MUST call `bestow.timer.update(dt)` once per frame in your update loop.

## One-Shot Timer (after)

Execute a callback after a delay:

```lua
-- Call a method on a table after delay
-- Pattern: bestow.timer.after(delay, ownerTable, methodName, ...args)
local timerId = bestow.timer.after(2.0, app.main, "onTimerFired")

-- With arguments
local timerId = bestow.timer.after(1.5, app.systems.spawner, "spawnEnemy", "goblin", Vec3.new(5, 0, 0))
```

The callback is invoked as `ownerTable:methodName(self, ...args)`, where self is the owner table. This is hot-reload safe because it stores a reference to the table and method name, not a closure.

## Repeating Timer (every)

Execute a callback at regular intervals:

```lua
-- Call every N seconds
local timerId = bestow.timer.every(0.5, app.systems.spawner, "tick")

-- Repeating with arguments
local timerId = bestow.timer.every(3.0, app.systems.waves, "spawnWave", "hard")
```

## Timer Control

```lua
-- Cancel a timer
bestow.timer.cancel(timerId)

-- Pause/resume
bestow.timer.pause(timerId)
bestow.timer.resume(timerId)

-- Check if active
bestow.timer.isActive(timerId) -> bool

-- Cancel all timers
bestow.timer.cancelAll()

-- Cancel all timers owned by a specific table
bestow.timer.cancelByOwner(app.systems.spawner)
```

## Frame Update (Required)

```lua
-- In your main update loop - MUST be called every frame
return {
    update = function(dt)
        bestow.timer.update(dt)
        -- ... rest of game update
    end
}
```

## Common Patterns

### Cooldown System

```lua
-- systems/combat.lua
return {
    canAttack = true,
    attackCooldown = 0.5,

    tryAttack = function()
        local self = app.systems.combat
        if not self.canAttack then return false end

        self.canAttack = false
        bestow.timer.after(self.attackCooldown, app.systems.combat, "resetAttack")

        -- Do attack
        return true
    end,

    resetAttack = function()
        local self = app.systems.combat
        self.canAttack = true
    end
}
```

### Spawn Waves

```lua
return {
    waveTimerId = nil,

    startWaves = function()
        local self = app.systems.spawner
        self.waveTimerId = bestow.timer.every(5.0, app.systems.spawner, "spawnWave")
    end,

    stopWaves = function()
        local self = app.systems.spawner
        if self.waveTimerId then
            bestow.timer.cancel(self.waveTimerId)
            self.waveTimerId = nil
        end
    end,

    spawnWave = function()
        local self = app.systems.spawner
        for i = 1, 3 do
            local x = math.random(-10, 10)
            app.entities.enemy.create({ Transform3D = { position = Vec3.new(x, 0, -20) } })
        end
    end
}
```

### Delayed Sequence

```lua
-- Chain of delayed events
bestow.timer.after(0.0, app.systems.cutscene, "step1")
bestow.timer.after(2.0, app.systems.cutscene, "step2")
bestow.timer.after(4.0, app.systems.cutscene, "step3")
bestow.timer.after(6.0, app.systems.cutscene, "endCutscene")
```

### Temporary Powerup

```lua
return {
    activatePowerup = function(duration)
        local self = app.systems.powerups
        local state = app.main.state
        state.speedMultiplier = 2.0

        -- Expire after duration
        bestow.timer.after(duration, app.systems.powerups, "deactivatePowerup")
    end,

    deactivatePowerup = function()
        local state = app.main.state
        state.speedMultiplier = 1.0
    end
}
```

## Best Practices

1. **Always call `bestow.timer.update(dt)`** in your main update loop
2. **Use table+method pattern** for hot-reload safety (not closures)
3. **Store timer IDs** if you need to cancel them later
4. **Use `cancelByOwner`** when cleaning up a system
5. **Cancel timers** when entities are destroyed to avoid callbacks on dead objects

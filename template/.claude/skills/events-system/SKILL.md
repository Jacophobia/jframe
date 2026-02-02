---
name: events-system
description: Publish and subscribe to game events using bestow.events. Use when implementing inter-system communication, collision callbacks, damage events, game state changes, or any decoupled event-driven logic.
---

# Events System

The events system enables decoupled communication between game systems. Publishers emit events; subscribers react to them without direct dependencies.

## Subscribing to Events

```lua
-- Subscribe with hot-reload-safe pattern: table + method name
-- Pattern: bestow.events.subscribe(eventType, filterPattern, ownerTable, methodName)
local subId = bestow.events.subscribe("collision", {}, app.systems.combat, "onCollision")

-- With filter pattern (only receive events matching these fields)
local subId = bestow.events.subscribe("entity_damaged", { target = playerEntity },
    app.systems.player, "onDamaged")
```

The callback is invoked as `ownerTable:methodName(eventData, scope)`.

## Emitting Events

```lua
-- Emit an event (all subscribers notified immediately)
bestow.events.emit("collision", {
    entityA = player,
    entityB = enemy,
    contactPoint = Vec3.new(5, 0, 3),
    normal = Vec3.new(0, 1, 0),
    impulse = 10.5
})

bestow.events.emit("entity_damaged", {
    target = enemy,
    source = player,
    amount = 25,
    knockback = Vec3.new(1, 0.5, 0)
})

bestow.events.emit("item_collected", {
    collector = player,
    item = coinEntity,
    itemType = "coin",
    value = 10
})
```

## Unsubscribing

```lua
-- Unsubscribe a specific subscription
bestow.events.unsubscribe(subId)
```

## Common Event Types

These are conventional event names used across bestow systems:

### Physics Events
```lua
"collision"        -- { entityA, entityB, contactPoint, normal, impulse }
"trigger_enter"    -- { entityA, entityB, contactPoint }
"trigger_exit"     -- { entityA, entityB }
"collision_3d"     -- { entityA, entityB, contactPoint, contactNormal, impulse, penetrationDepth }
"trigger_enter_3d" -- { entityA, entityB }
"trigger_exit_3d"  -- { entityA, entityB }
```

### Gameplay Events
```lua
"entity_damaged"   -- { target, source, amount, knockback }
"entity_died"      -- { entity, killer }
"item_collected"   -- { collector, item, itemType, value }
"checkpoint"       -- { entity, checkpointId }
"player_death"     -- { entity, cause }
```

### Level Events
```lua
"level_loaded"     -- { levelId }
"level_unloaded"   -- { levelId }
```

### Asset Events
```lua
"asset_loaded"     -- { handle, type, state }
"asset_reloaded"   -- { handle, type, state }
```

### State Events
```lua
"state_changed"    -- { oldStateName, newStateName }
"state_pushed"     -- { stateName }
"state_popped"     -- { stateName }
```

### Input Events
```lua
"action_triggered" -- { action, phase, source, duration, axis }
"phase_changed"    -- { oldPhase, newPhase, phaseStack }
```

### Save Events
```lua
"game_saved"       -- { slot, saveName }
"game_loaded"      -- { slot }
```

## Common Patterns

### Damage System

```lua
-- systems/combat.lua
return {
    init = function()
        local self = app.systems.combat
        self.damageSubId = bestow.events.subscribe("collision_3d", {},
            app.systems.combat, "onCollision")
    end,

    onCollision = function(event)
        local self = app.systems.combat
        -- Check if one entity is a projectile
        if bestow.entity.hasComponent(event.entityA, "Projectile") then
            self.applyDamage(event.entityA, event.entityB, event.impulse)
        elseif bestow.entity.hasComponent(event.entityB, "Projectile") then
            self.applyDamage(event.entityB, event.entityA, event.impulse)
        end
    end,

    applyDamage = function(source, target, amount)
        if not bestow.entity.hasComponent(target, "Health") then return end
        local health = bestow.entity.getComponent(target, "Health")
        health.current = math.max(0, health.current - amount)
        bestow.entity.setComponent(target, "Health", health)

        bestow.events.emit("entity_damaged", {
            target = target, source = source, amount = amount
        })

        if health.current <= 0 then
            bestow.events.emit("entity_died", { entity = target, killer = source })
        end
    end,

    shutdown = function()
        local self = app.systems.combat
        if self.damageSubId then
            bestow.events.unsubscribe(self.damageSubId)
        end
    end
}
```

### Score Tracking

```lua
-- systems/scoring.lua
return {
    init = function()
        bestow.events.subscribe("item_collected", {},
            app.systems.scoring, "onItemCollected")
        bestow.events.subscribe("entity_died", {},
            app.systems.scoring, "onEnemyKilled")
    end,

    onItemCollected = function(event)
        local state = app.main.state
        state.score = (state.score or 0) + (event.value or 1)
    end,

    onEnemyKilled = function(event)
        local state = app.main.state
        state.score = (state.score or 0) + 100
    end
}
```

### Game Over Detection

```lua
-- In init
bestow.events.subscribe("player_death", {}, app.main, "onPlayerDeath")

-- Handler
return {
    onPlayerDeath = function(event)
        local state = app.main.state
        state.lives = state.lives - 1
        if state.lives <= 0 then
            bestow.input.pushPhase("gameover")
            app.systems.ui.showGameOver()
        else
            app.systems.spawner.respawnPlayer()
        end
    end
}
```

## Best Practices

1. **Use events for cross-system communication** - Don't have systems reference each other directly
2. **Use filter patterns** to reduce unnecessary callback invocations
3. **Store subscription IDs** and unsubscribe when cleaning up
4. **Use conventional event names** from the list above for consistency
5. **Keep event data tables small** - Only include what subscribers need
6. **Use table+method pattern** for hot-reload safety (same as timer system)

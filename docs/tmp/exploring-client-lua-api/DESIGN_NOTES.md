# State Preservation Design Notes

## Core Principle: Implicit State

State is **automatically created by the engine** for each scope level. Game code doesn't register schemas - it just uses `self` and `scope`.

## Auto-Discovery vs Manual Loading

**Auto-discovered** (engine manages lifecycle):
- `app.lua` - App entry point
- `levels/*.lua` - Level definitions
- `components/*.lua` - Entity components
- `systems/*.lua` - Cross-entity systems

These files are:
- **Hot reloadable** - Engine watches for changes and reloads
- **NOT in `app.*` table** - Prevents users from calling lifecycle methods directly
- **Lifecycle managed** - Engine calls init/update/destroy

**User scripts** (accessible via `app.*`):
- `inputs.lua` - Via `app.inputs()` (returns a function)
- Entity blueprints - Via `app.entities.player`, etc.
- Resources - Via `app.resources.foo`, etc.

These files are:
- **Hot reloadable** - Via `app.*` system
- **In `app.*` table** - User controls when/how to use them
- **No lifecycle** - User manages execution

## State Hierarchy

```
Global State (scope.global)
└── Profile State (scope.profile)
    └── Level State (scope.level)
        └── Entity State (scope.entity)
            └── Component State (self)
                └── Transient State (self.transient)
```

## The Pattern

Everything (app, levels, components, systems) follows the same pattern:

```lua
return {
    init = function(self, scope)
        -- Saved state (persists to disk)
        if self.health == nil then
            self.health = 100
        end

        -- Transient state (engine pre-creates self.transient = {})
        self.transient.isAttacking = false
        self.transient.cooldownTimer = 0

        -- Event subscriptions with pattern matching
        self.transient.hitSub = bestow.events.subscribe("Hit", {
            target = scope.entity.id  -- Only fire when this matches
        }, function(event)
            self:takeDamage(event.damage, scope)
        end)
    end,

    destroy = function(self, scope)
        bestow.events.unsubscribe(self.transient.hitSub)
    end,

    update = function(self, dt, scope)
        self.transient.cooldownTimer = self.transient.cooldownTimer + dt
    end,
}
```

## Systems

Systems operate on groups of entities with matching component patterns:

```lua
return {
    -- Only runs on entities with Transform AND Movement components
    pattern = { "Transform", "Movement" },

    -- Update order (lower = earlier, default 0)
    order = -10,

    init = function(self, scope)
        self.transient.speed = 5.0
    end,

    -- Receives list of matching entities
    update = function(self, dt, scope, entities)
        for _, entity in ipairs(entities) do
            entity.Transform.position.x = entity.Transform.position.x + dt
        end
    end,
}
```

### Components vs Systems

| Aspect | Components | Systems |
|--------|------------|---------|
| Scope | Single entity | All matching entities |
| Data | Entity-specific state | Shared state |
| Behavior | Entity-specific logic | Cross-entity logic |
| Pattern | Always attached to entity | Declares component requirements |

Use components for entity-specific state and behavior.
Use systems for cross-cutting concerns (movement, physics, AI).

## Event Pattern Matching

Events support pattern matching to eliminate boilerplate:

```lua
-- Pattern matches event fields
bestow.events.subscribe("Hit", {
    target = scope.entity.id,  -- Must match exactly
}, function(event)
    -- Only called when event.target == scope.entity.id
end)

-- Empty pattern {} matches all events of that type
bestow.events.subscribe("CombatHit", {}, function(event)
    -- Called for every CombatHit event
end)
```

This removes common conditionals:

```lua
-- BEFORE: Manual filtering
bestow.events.subscribe("Hit", function(event)
    if event.target == scope.entity.id then
        self:onHit(event)
    end
end)

-- AFTER: Pattern matching
bestow.events.subscribe("Hit", { target = scope.entity.id }, function(event)
    self:onHit(event)
end)
```

## Update Order

Engine calls updates in this order:

```
1. App update
2. Systems update (sorted by system.order, lower first)
3. Level update
4. Components update (per entity)
```

This ensures:
- App can do global housekeeping first
- Systems process entities before components react
- Level coordinates entity interactions
- Components do entity-specific finalization

## State Rules

| Pattern | Saved to Disk | Persists Across Frames | Example |
|---------|---------------|------------------------|---------|
| `self.foo` | Yes | Yes | `self.health`, `self.score` |
| `self.transient.foo` | No | Yes | `self.transient.isAttacking`, `self.transient.timer` |

**Transient table = not saved to disk.** Both persist across frames during gameplay.

When to use `self.transient.foo`:
- Attack/animation state (`transient.isAttacking`, `transient.attackPhase`)
- Cooldown timers (`transient.cooldownRemaining`)
- Temporary tracking (`transient.hitEntitiesThisSwing`)
- Cached references (`transient.hitSub`)

When to use `self.foo`:
- Anything the player would expect to persist after saving/loading
- Health, inventory, stats, progression

## Scope Access Rules

| In | self | scope.entity | scope.level | scope.profile | scope.global |
|----|------|--------------|-------------|---------------|--------------|
| App | app state | - | current level | active profile | global |
| Level | level state | - | - | active profile | global |
| System | system state | - | current level | active profile | global |
| Component | component state | parent entity | current level | active profile | global |

**Key rule**: Access ancestors, not siblings. Sibling communication via events.

## Entity Queries

Levels and systems can query entities:

```lua
local player = scope.findEntityByTag("player")
local enemies = scope.findAllByTag("enemy")
local nearby = scope.findEntitiesInRadius(position, radius)
```

## Save System

Engine serializer:
- Saves `self.foo` fields
- Skips `self.transient` table entirely

When loading a save:
- `self.foo` fields are restored from save
- `self.transient` is recreated empty, then init() populates it (fresh start)

## Hot Reload Behavior

1. Engine re-executes file
2. Functions replaced
3. **All `self` data preserved** (both `foo` and `transient`)
4. `init()` is NOT re-called

Hot reload preserves everything - you can tweak attack logic mid-swing.

## Lifecycle

```
App init
    └── Level load triggered
        └── Level init
            └── Systems init
                └── Entity/Component inits (as spawned)

Each frame:
    App update
        └── Systems update (sorted by order)
            └── Level update
                └── Components update

Level unload:
    Components destroy
        └── Systems destroy (if level-scoped)
            └── Level destroy

App shutdown:
    App destroy
```

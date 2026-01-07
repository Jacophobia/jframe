# Exploring Client-Side Lua API - State Preservation

Sample exploring the Lua API for Bestow, focusing on **state preservation**.

## Core Principles

1. **State is implicit** - Engine creates state for each scope
2. **Just init/update/destroy** - Simple lifecycle for everything
3. **Transient table for runtime state** - `self.transient.foo` not saved to disk
4. **Explicit event cleanup** - Subscribe in init, unsubscribe in destroy
5. **Pattern matching for events** - Events only fire when pattern matches

## Auto-Discovery

The engine auto-discovers and manages lifecycle for:

| Directory | Purpose | In `app.*`? |
|-----------|---------|-------------|
| `app.lua` | App entry point | No |
| `levels/*.lua` | Level definitions | No |
| `components/*.lua` | Entity components | No |
| `systems/*.lua` | Cross-entity systems | No |

These are hot reloadable but NOT exposed in `app.*` (prevents calling lifecycle methods directly).

Everything else (inputs, entities, resources) is accessible via `app.*`:

```lua
app.inputs()              -- Returns function, call to register
app.entities.player       -- Entity blueprint
app.resources.textures    -- User resources
```

## State Hierarchy

```
scope.global       → Global state (settings, achievements)
scope.profile      → Profile state (save slot)
scope.level        → Level state
scope.entity       → Parent entity (from components)
self               → This component/system/level state
self.transient     → Runtime-only state (engine pre-creates)
```

## Update Order

```
1. App update
2. Systems update (in order defined by system.order)
3. Level update
4. Components update
```

## The Pattern

```lua
return {
    init = function(self, scope)
        -- Saved to disk
        if self.health == nil then
            self.health = 100
        end

        -- NOT saved (engine pre-creates self.transient = {})
        self.transient.isAttacking = false
        self.transient.timer = 0

        -- Event subscription with PATTERN MATCHING
        -- Only fires when event.target == this entity's id
        self.transient.hitSub = bestow.events.subscribe("Hit", {
            target = scope.entity.id
        }, function(event)
            self:onHit(event, scope)
        end)
    end,

    destroy = function(self, scope)
        bestow.events.unsubscribe(self.transient.hitSub)
    end,

    update = function(self, dt, scope)
        self.transient.timer = self.transient.timer + dt
    end,
}
```

## Systems Pattern

Systems operate on entities with matching component patterns:

```lua
return {
    -- Only entities with Transform AND Movement
    pattern = { "Transform", "Movement" },

    -- Lower numbers run first
    order = -10,

    update = function(self, dt, scope, entities)
        for _, entity in ipairs(entities) do
            -- Access components directly
            entity.Transform.position.x = entity.Transform.position.x + dt
        end
    end,
}
```

## Event Pattern Matching

```lua
-- OLD: Manual filtering
bestow.events.subscribe("Hit", function(event)
    if event.target == scope.entity.id then  -- boilerplate
        self:onHit(event)
    end
end)

-- NEW: Pattern matching (engine filters)
bestow.events.subscribe("Hit", { target = scope.entity.id }, function(event)
    self:onHit(event)  -- only called when pattern matches
end)
```

## State Rules

| Pattern | Saved | Example |
|---------|-------|---------|
| `self.foo` | Yes | health, score, inventory |
| `self.transient.foo` | No | timers, attack state, subscriptions |

Both persist across frames during gameplay. The transient table is skipped when serializing to disk.

## Files

```
exploring-client-lua-api/
├── README.md
├── DESIGN_NOTES.md
├── app.lua              # App entry (lifecycle managed, not in app.*)
├── inputs.lua           # Input actions (app.inputs() - returns function)
├── components/          # Entity components (lifecycle managed, not in app.*)
│   ├── health.lua
│   ├── combat.lua
│   └── movement.lua
├── systems/             # Cross-entity systems (lifecycle managed, not in app.*)
│   ├── movement.lua
│   └── combat.lua
└── levels/              # Levels (lifecycle managed, not in app.*)
    └── arena.lua
```

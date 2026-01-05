---
paths: "**/*.lua"
---

# Hot Reload Rules for Lua Files

## Critical: Never Cache app.* at File Scope

```lua
-- WRONG: Will break on hot reload
local player = app.entities.player
return {
    create = function()
        return player.create()  -- STALE!
    end
}

-- CORRECT: Fresh reference each call
return {
    create = function()
        local player = app.entities.player
        return player.create()
    end
}
```

## State Must Live in app.main.state

All game state that should survive hot reload MUST be stored in `app.main.state`:

```lua
-- In init()
app.main.state = {
    player = nil,
    enemies = {},
    score = 0
}

-- Everywhere else - access through app.main.state
local state = app.main.state
state.score = state.score + 100
```

## Self-Reference Pattern

When a module needs to reference itself, use the `self` pattern inside functions:

```lua
return {
    speed = 5.0,

    update = function(dt)
        local self = app.systems.movement  -- Fresh reference!
        -- Use self.speed, not a cached value
    end
}
```

## File Scope Variables Are OK For Constants

You CAN define constants at file scope since they don't change:

```lua
local GRAVITY = -9.81
local MAX_SPEED = 10.0

return {
    update = function(dt)
        -- Using GRAVITY here is fine - it's a constant
    end
}
```

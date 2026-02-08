-- Example: Hot-Reload Safe Patterns
-- Shows correct patterns for code that survives hot reload

-- systems/movement.lua
local MOVE_SPEED = 8.0      -- Constants at file scope are OK
local GRAVITY = -30.0
local JUMP_FORCE = 12.0

return {
    -- System-level values (survive hot reload if not re-initialized)
    velocityY = 0,

    init = function()
        local self = app.systems.movement
        local state = app.main.state

        -- Initialize state ONLY if not already set (preserves across hot reload)
        if state.player == nil then
            state.player = app.entities.player.create()
        end
        if state.score == nil then
            state.score = 0
        end

        -- Clean up old event subscriptions before re-subscribing
        if self.collisionSubId then
            bestow.events.unsubscribe(self.collisionSubId)
        end

        -- Subscribe using table+method (NOT closures)
        self.collisionSubId = bestow.events.subscribe("collision_3d", {},
            app.systems.movement, "onCollision")

        -- Timers: use table+method pattern
        bestow.timer.cancelFor(app.systems.movement)  -- Clean up old timers
        bestow.timer.every(1.0, app.systems.movement, "tick")
    end,

    update = function(dt)
        -- ALWAYS get fresh references inside functions
        local self = app.systems.movement
        local state = app.main.state

        if not state.player then return end
        if not bestow.entity.isValid(state.player) then
            state.player = nil
            return
        end

        -- Use self.velocityY, self.MOVE_SPEED etc.
        -- Never use a cached reference to another system
        local camera = app.systems.camera  -- Fresh reference each frame
        camera.update(dt)
    end,

    onCollision = function(event)
        -- Table+method callback - automatically gets fresh reference
        local self = app.systems.movement
        local state = app.main.state
        -- Handle collision...
    end,

    tick = function()
        -- Timer callback - table+method pattern
        local self = app.systems.movement
        local state = app.main.state
        -- Periodic update...
    end,

    shutdown = function()
        local self = app.systems.movement
        if self.collisionSubId then
            bestow.events.unsubscribe(self.collisionSubId)
        end
        bestow.timer.cancelFor(app.systems.movement)
    end
}

-- KEY RULES:
-- 1. NEVER cache app.* at file scope: `local movement = app.systems.movement`  -- BAD!
-- 2. ALWAYS get fresh references inside functions: `local self = app.systems.movement`
-- 3. Store mutable state in app.main.state (survives reload)
-- 4. File-scope CONSTANTS are fine: `local SPEED = 8.0`
-- 5. Use table+method for events/timers (not closures)
-- 6. Clean up subscriptions in init() before re-subscribing

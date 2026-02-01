-- Example: Timer System Usage
-- Shows one-shot timers, repeating timers, cooldowns, and cleanup

-- systems/spawner.lua
return {
    spawnTimerId = nil,
    canAttack = true,
    attackCooldownId = nil,

    init = function()
        local self = app.systems.spawner

        -- Repeating timer: spawn enemies every 5 seconds
        self.spawnTimerId = bestow.timer.every(5.0, app.systems.spawner, "spawnWave")

        -- One-shot timer: delayed start
        bestow.timer.after(2.0, app.systems.spawner, "onGameReady")
    end,

    update = function(dt)
        -- REQUIRED: Must call timer.update every frame
        bestow.timer.update(dt)
    end,

    onGameReady = function()
        print("Game is ready!")
    end,

    spawnWave = function()
        local self = app.systems.spawner
        for i = 1, 3 do
            local x = math.random(-10, 10)
            print("Spawning enemy at x=" .. x)
        end
    end,

    tryAttack = function()
        local self = app.systems.spawner
        if not self.canAttack then return false end

        self.canAttack = false
        -- Cooldown: re-enable attack after 0.5 seconds
        bestow.timer.after(0.5, app.systems.spawner, "resetAttack")

        print("Attack!")
        return true
    end,

    resetAttack = function()
        local self = app.systems.spawner
        self.canAttack = true
    end,

    -- Pause spawning
    pauseSpawning = function()
        local self = app.systems.spawner
        if self.spawnTimerId then
            bestow.timer.pause(self.spawnTimerId)
        end
    end,

    -- Resume spawning
    resumeSpawning = function()
        local self = app.systems.spawner
        if self.spawnTimerId then
            bestow.timer.resume(self.spawnTimerId)
        end
    end,

    -- Clean up
    shutdown = function()
        local self = app.systems.spawner
        -- Cancel all timers owned by this system
        bestow.timer.cancelByOwner(app.systems.spawner)
    end
}

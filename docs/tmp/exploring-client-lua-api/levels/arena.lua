--[[
    Arena Level

    Auto-discovered from levels/. Has init/update/destroy like components.
    Levels manage level-specific state and entity spawning.

    State Pattern:
    - self.foo = saved to profile
    - self.transient.foo = not saved
]]

return {
    -- Metadata
    name = "arena",

    init = function(self, scope)
        -- Saved state
        if self.completed == nil then
            self.completed = false
            self.bestTime = nil
            self.highScore = 0
        end
        self.playerDamageMultiplier = self.playerDamageMultiplier or 1.0

        -- Transient state
        self.transient.currentScore = 0
        self.transient.enemiesKilledThisRun = 0
        self.transient.timeElapsed = 0
        self.transient.currentWave = 0
        self.transient.waveInProgress = false
        self.transient.enemiesRemainingInWave = 0
        self.transient.comboCount = 0
        self.transient.comboTimer = 0
        self.transient.comboMultiplier = 1.0
        self.transient.difficulty = scope.profile.difficulty or 1.0

        -- Spawn points (level-specific, defined here not in config)
        self.transient.spawnPoints = {
            { x = 10, y = 0, z = 10 },
            { x = -10, y = 0, z = 10 },
            { x = 10, y = 0, z = -10 },
            { x = -10, y = 0, z = -10 },
        }

        -- Set up level environment
        bestow.audio.playMusic(":app:/audio/music/battle_theme.ogg")
        bestow.audio.playAmbient(":app:/audio/ambient/arena_crowd.ogg")

        -- Spawn player
        local playerEntity = bestow.entity.spawn("player", {
            Transform = { position = { x = 0, y = 1, z = 0 } },
        })
        self.transient.playerId = playerEntity.id

        -- Create camera (as an entity with camera component)
        local cameraEntity = bestow.entity.spawn("camera", {
            Transform = { position = { x = 0, y = 10, z = -8 } },
            Camera = {
                type = "follow",
                target = playerEntity.id,
                offset = { x = 0, y = 10, z = -8 },
                lookAhead = 2.0,
                smoothing = 5.0,
            },
        })
        self.transient.cameraId = cameraEntity.id
        bestow.camera.setActive(cameraEntity.id)

        -- Spawn static environment
        bestow.entity.spawn("environment/arena_floor", {
            Transform = { position = { x = 0, y = 0, z = 0 } },
        })

        local wallPositions = {
            { x = 20, y = 2, z = 0, ry = 90 },
            { x = -20, y = 2, z = 0, ry = -90 },
            { x = 0, y = 2, z = 20, ry = 0 },
            { x = 0, y = 2, z = -20, ry = 180 },
        }
        for _, pos in ipairs(wallPositions) do
            bestow.entity.spawn("environment/arena_wall", {
                Transform = {
                    position = { x = pos.x, y = pos.y, z = pos.z },
                    rotation = { y = pos.ry },
                },
            })
        end

        -- Event subscriptions with pattern matching
        self.transient.combatHitSub = bestow.events.subscribe("CombatHit", {}, function(event)
            self:onCombatHit(event, scope)
        end)

        self.transient.entityDiedSub = bestow.events.subscribe("EntityDied", {}, function(event)
            self:onEntityDied(event, scope)
        end)

        -- Start first wave after delay
        bestow.timer.after(2.0, function()
            self:startWave(1, scope)
        end)
    end,

    destroy = function(self, scope)
        bestow.events.unsubscribe(self.transient.combatHitSub)
        bestow.events.unsubscribe(self.transient.entityDiedSub)
        bestow.audio.stopMusic()
        bestow.audio.stopAmbient()
    end,

    update = function(self, dt, scope)
        local t = self.transient
        t.timeElapsed = t.timeElapsed + dt

        -- Combo decay
        if t.comboTimer > 0 then
            t.comboTimer = t.comboTimer - dt
            if t.comboTimer <= 0 then
                self:resetCombo(scope)
            end
        end

        -- Wave completion check
        if t.waveInProgress and t.enemiesRemainingInWave <= 0 then
            self:onWaveComplete(scope)
        end
    end,

    -- Entity queries
    getPlayer = function(self, scope)
        return scope.findEntityById(self.transient.playerId)
    end,

    getPlayerPosition = function(self, scope)
        local player = self:getPlayer(scope)
        return player and player.Transform.position or nil
    end,

    -- Combo system
    onCombatHit = function(self, event, scope)
        local t = self.transient
        t.comboCount = t.comboCount + 1
        t.comboTimer = 2.0
        t.comboMultiplier = math.min(4.0, 1.0 + t.comboCount * 0.1)

        bestow.events.emit("ComboChanged", {
            count = t.comboCount,
            multiplier = t.comboMultiplier,
            timer = t.comboTimer,
        })
    end,

    resetCombo = function(self, scope)
        local t = self.transient
        if t.comboCount > 0 then
            bestow.events.emit("ComboEnded", {
                finalCount = t.comboCount,
                finalMultiplier = t.comboMultiplier,
            })
        end
        t.comboCount = 0
        t.comboMultiplier = 1.0
        t.comboTimer = 0
    end,

    -- Score
    addScore = function(self, baseAmount, scope)
        local t = self.transient
        local actual = math.floor(baseAmount * t.comboMultiplier)
        t.currentScore = t.currentScore + actual

        if t.currentScore > self.highScore then
            self.highScore = t.currentScore
        end

        bestow.events.emit("ScoreChanged", {
            oldScore = t.currentScore - actual,
            newScore = t.currentScore,
            added = actual,
            multiplier = t.comboMultiplier,
        })

        return actual
    end,

    -- Wave system
    startWave = function(self, waveNumber, scope)
        local t = self.transient
        t.currentWave = waveNumber
        t.waveInProgress = true

        local enemyCount = 3 + (waveNumber * 2)
        t.enemiesRemainingInWave = enemyCount

        for i = 1, enemyCount do
            local spawnIndex = ((i - 1) % #t.spawnPoints) + 1
            local pos = t.spawnPoints[spawnIndex]

            bestow.entity.spawn("enemies/goblin", {
                Transform = {
                    position = {
                        x = pos.x + (math.random() - 0.5) * 4,
                        y = pos.y,
                        z = pos.z + (math.random() - 0.5) * 4,
                    },
                },
            })
        end

        bestow.events.emit("WaveStarted", {
            wave = waveNumber,
            enemyCount = enemyCount,
        })
    end,

    onWaveComplete = function(self, scope)
        self.transient.waveInProgress = false

        bestow.events.emit("WaveComplete", { wave = self.transient.currentWave })

        bestow.timer.after(3.0, function()
            self:startWave(self.transient.currentWave + 1, scope)
        end)
    end,

    -- Death handling
    onEntityDied = function(self, event, scope)
        local t = self.transient

        if event.tag == "enemy" then
            t.enemiesKilledThisRun = t.enemiesKilledThisRun + 1
            t.enemiesRemainingInWave = t.enemiesRemainingInWave - 1
            self:addScore(100, scope)

        elseif event.tag == "player" then
            bestow.events.emit("GameOver", {
                score = t.currentScore,
                wave = t.currentWave,
                time = t.timeElapsed,
            })
        end
    end,

    -- Level completion
    completeLevel = function(self, scope)
        local t = self.transient
        self.completed = true

        if not self.bestTime or t.timeElapsed < self.bestTime then
            self.bestTime = t.timeElapsed
        end

        bestow.events.emit("LevelComplete", {
            level = "arena",
            score = t.currentScore,
            time = t.timeElapsed,
            waves = t.currentWave,
        })
    end,
}

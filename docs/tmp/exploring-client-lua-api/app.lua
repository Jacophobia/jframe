--[[
    Combat Demo - App Entry Point

    Auto-discovered by engine. Has init/update/destroy lifecycle.

    State:
    - self = app state (created by engine)
    - scope.global = global state
    - scope.profile = active profile (nil if none)
    - scope.level = current level (nil if none)
]]

return {
    -- Metadata (read by engine)
    title = "Combat Demo",
    version = "0.1.0",

    -- Called once when game starts
    init = function(self, scope)
        bestow.info("Combat Demo initializing...")

        -- Register input actions (returns a function, so we call it)
        app.inputs()

        -- Initialize global state defaults
        if scope.global.windowMode == nil then
            scope.global.windowMode = "windowed_fullscreen"
        end
        if scope.global.masterVolume == nil then
            scope.global.masterVolume = 1.0
        end
        if scope.global.totalPlaytime == nil then
            scope.global.totalPlaytime = 0
        end

        -- Apply settings
        bestow.window.setMode(scope.global.windowMode)
        bestow.audio.setMasterVolume(scope.global.masterVolume)

        -- Subscribe to phase changes (app-level, no pattern needed)
        self.transient.phaseEnterSub = bestow.events.subscribe("PhaseEnter", function(event)
            self:onPhaseEnter(event.phase, scope)
        end)

        self.transient.phaseExitSub = bestow.events.subscribe("PhaseExit", function(event)
            self:onPhaseExit(event.phase, scope)
        end)

        -- Game flow events
        self.transient.newGameSub = bestow.events.subscribe("NewGame", function(event)
            local profileId = bestow.profile.create(event.profileName or "New Game")
            bestow.profile.load(profileId)
            bestow.level.load("arena")
            bestow.phase.change("game.combat")
        end)

        self.transient.continueSub = bestow.events.subscribe("ContinueGame", function(event)
            local profileId = bestow.profile.getMostRecent()
            if profileId then
                bestow.profile.load(profileId)
                local lastLevel = scope.profile.currentLevel
                if lastLevel then
                    bestow.level.load(lastLevel)
                    bestow.phase.change("game.combat")
                else
                    bestow.warn("Profile missing currentLevel - save may be corrupted")
                    bestow.events.emit("QuitToMenu", {})
                end
            else
                bestow.warn("No profile to continue")
            end
        end)

        self.transient.quitMenuSub = bestow.events.subscribe("QuitToMenu", function(event)
            if scope.profile then
                bestow.profile.save()
            end
            if scope.level then
                bestow.level.unload()
            end
            bestow.phase.change("menu.start")
        end)

        self.transient.quitGameSub = bestow.events.subscribe("QuitGame", function(event)
            bestow.quit()
        end)

        self.transient.pauseSub = bestow.events.subscribe("TogglePause", function(event)
            local current = bestow.phase.current()
            if current == "game.combat" then
                bestow.phase.push("game.paused")
            elseif current == "game.paused" then
                bestow.phase.pop()
            end
        end)

        self.transient.inventorySub = bestow.events.subscribe("ToggleInventory", function(event)
            local current = bestow.phase.current()
            if current == "game.combat" then
                bestow.phase.push("game.inventory")
            elseif current == "game.inventory" then
                bestow.phase.pop()
            end
        end)

        self.transient.gameOverSub = bestow.events.subscribe("GameOver", function(event)
            scope.profile.deaths = (scope.profile.deaths or 0) + 1
            bestow.profile.save()
            bestow.phase.change("results")
        end)

        self.transient.levelCompleteSub = bestow.events.subscribe("LevelComplete", function(event)
            scope.profile.levelsCompleted = scope.profile.levelsCompleted or {}
            scope.profile.levelsCompleted[event.level] = true

            if not scope.profile.bestTime or event.time < scope.profile.bestTime then
                scope.profile.bestTime = event.time
            end

            bestow.profile.save()
            bestow.phase.change("results")
        end)

        -- Start at menu
        bestow.phase.change("menu.start")

        bestow.info("Initialization complete")
    end,

    update = function(self, dt, scope)
        scope.global.totalPlaytime = scope.global.totalPlaytime + dt
    end,

    destroy = function(self, scope)
        bestow.info("Shutting down...")

        -- Unsubscribe all events
        bestow.events.unsubscribe(self.transient.phaseEnterSub)
        bestow.events.unsubscribe(self.transient.phaseExitSub)
        bestow.events.unsubscribe(self.transient.newGameSub)
        bestow.events.unsubscribe(self.transient.continueSub)
        bestow.events.unsubscribe(self.transient.quitMenuSub)
        bestow.events.unsubscribe(self.transient.quitGameSub)
        bestow.events.unsubscribe(self.transient.pauseSub)
        bestow.events.unsubscribe(self.transient.inventorySub)
        bestow.events.unsubscribe(self.transient.gameOverSub)
        bestow.events.unsubscribe(self.transient.levelCompleteSub)

        if scope.profile then
            bestow.profile.save()
        end
        bestow.state.saveGlobal()

        bestow.info("Shutdown complete")
    end,

    -- Helper methods
    onPhaseEnter = function(self, phase, scope)
        bestow.info("Entering phase: " .. phase)

        if phase == "menu.start" then
            bestow.ui.show("main_menu")
        elseif phase == "game.combat" then
            bestow.ui.hide("main_menu")
            bestow.ui.show("game_hud")
        elseif phase == "game.paused" then
            bestow.ui.show("pause_menu")
            bestow.time.setScale(0)
        elseif phase == "game.inventory" then
            bestow.ui.show("inventory_screen")
            bestow.time.setScale(0)
        elseif phase == "results" then
            bestow.ui.show("results_screen")
        end
    end,

    onPhaseExit = function(self, phase, scope)
        if phase == "game.paused" then
            bestow.ui.hide("pause_menu")
            bestow.time.setScale(1)
        elseif phase == "game.inventory" then
            bestow.ui.hide("inventory_screen")
            bestow.time.setScale(1)
        end
    end,
}

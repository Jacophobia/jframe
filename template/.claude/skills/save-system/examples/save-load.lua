-- Example: Save/Load System
-- Shows registering saveables, saving, loading, and auto-save

-- systems/save_manager.lua
return {
    init = function()
        local self = app.systems.save_manager

        -- Register game state as saveable
        bestow.save.registerSaveable({
            key = "game_state",

            serialize = function(archive)
                local state = app.main.state
                archive:writeInt("score", state.score or 0)
                archive:writeInt("lives", state.lives or 3)
                archive:writeString("currentLevel", state.currentLevel or "level1")
                archive:writeFloat("playTime", state.totalPlayTime or 0)

                -- Save player position
                if state.player and bestow.entity.isValid(state.player) then
                    local pos = bestow.entity.getField(state.player, "Transform3D", "position")
                    archive:writeBool("hasPlayer", true)
                    archive:writeFloat("playerX", pos.x)
                    archive:writeFloat("playerY", pos.y)
                    archive:writeFloat("playerZ", pos.z)

                    local health = bestow.entity.getComponent(state.player, "Health")
                    if health then
                        archive:writeInt("healthCurrent", health.current)
                        archive:writeInt("healthMax", health.max)
                    end
                else
                    archive:writeBool("hasPlayer", false)
                end
            end,

            deserialize = function(archive)
                local state = app.main.state
                state.score = archive:readInt("score")
                state.lives = archive:readInt("lives")
                state.currentLevel = archive:readString("currentLevel")
                state.totalPlayTime = archive:readFloat("playTime")

                if archive:readBool("hasPlayer") then
                    local pos = Vec3.new(
                        archive:readFloat("playerX"),
                        archive:readFloat("playerY"),
                        archive:readFloat("playerZ")
                    )
                    state.player = app.entities.player.spawnAt(pos)

                    local health = {
                        current = archive:readInt("healthCurrent"),
                        max = archive:readInt("healthMax")
                    }
                    bestow.entity.setComponent(state.player, "Health", health)
                end
            end
        })

        -- Enable auto-save every 2 minutes
        bestow.save.enableAutoSave(120.0)
    end,

    -- Manual save to slot
    saveGame = function(slot)
        local self = app.systems.save_manager
        local result = bestow.save.save(slot or 1)
        return result.success
    end,

    -- Load from slot
    loadGame = function(slot)
        local self = app.systems.save_manager
        local result = bestow.save.load(slot or 1)
        if result.success then
            -- Reload the level we were on
            local state = app.main.state
            if state.currentLevel then
                app.levels[state.currentLevel].load()
            end
        end
        return result.success
    end,

    -- Quick save/load
    quickSave = function()
        bestow.save.quickSave()
    end,

    quickLoad = function()
        bestow.save.quickLoad()
    end,

    -- Check if save exists
    hasSave = function(slot)
        return bestow.save.saveExists(slot or 1)
    end,

    shutdown = function()
        bestow.save.unregisterSaveable("game_state")
    end
}

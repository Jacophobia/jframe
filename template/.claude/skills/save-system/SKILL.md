---
name: save-system
description: Save and load game progress in Bestow. Use when implementing save games, checkpoints, auto-save, or persistent game state.
---

# Save System

The save system handles persisting game progress to disk.

## Basic Save/Load

### Quick Save and Load

```lua
-- Quick save (slot UINT32_MAX - 1)
bestow.save.quickSave()

-- Quick load
bestow.save.quickLoad()
```

### Save to Slot

```lua
-- Save to slot 1
bestow.save.save(1)

-- Load from slot 1
local result = bestow.save.load(1)
if result.success then
    print("Loaded successfully")
else
    print("Load failed: " .. result.error)
end
```

### Check Save Existence

```lua
if bestow.save.saveExists(1) then
    -- Save slot 1 has data
end

-- Get all save metadata
local saves = bestow.save.getAllSaveMetadata()
for slot, metadata in pairs(saves) do
    print("Slot " .. slot .. ": " .. metadata.timestamp)
end
```

## Registering Saveables

To save custom game state, register saveable objects:

```lua
-- systems/game_state.lua
return {
    -- Initialize in main.lua init()
    initSaveSystem = function()
        local self = app.systems.game_state

        -- Register this module as saveable
        bestow.save.registerSaveable({
            key = "game_state",

            -- Called when saving
            serialize = function(archive)
                local state = app.main.state

                archive:writeInt("score", state.score)
                archive:writeInt("lives", state.lives)
                archive:writeString("currentLevel", state.currentLevel)
                archive:writeString("checkpoint", state.lastCheckpoint or "")
                archive:writeFloat("playTime", state.totalPlayTime)

                -- Save inventory
                local inventory = state.inventory or {}
                archive:writeInt("inventoryCount", #inventory)
                for i, item in ipairs(inventory) do
                    archive:writeString("item" .. i, item)
                end
            end,

            -- Called when loading
            deserialize = function(archive)
                local state = app.main.state

                state.score = archive:readInt("score")
                state.lives = archive:readInt("lives")
                state.currentLevel = archive:readString("currentLevel")

                local checkpoint = archive:readString("checkpoint")
                state.lastCheckpoint = checkpoint ~= "" and checkpoint or nil

                state.totalPlayTime = archive:readFloat("playTime")

                -- Load inventory
                state.inventory = {}
                local count = archive:readInt("inventoryCount")
                for i = 1, count do
                    table.insert(state.inventory, archive:readString("item" .. i))
                end
            end
        })
    end,

    -- Unregister on shutdown
    shutdownSaveSystem = function()
        bestow.save.unregisterSaveable("game_state")
    end
}
```

## Save Data Types

The archive supports these data types:

```lua
-- Writing
archive:writeInt("key", 42)
archive:writeFloat("key", 3.14)
archive:writeDouble("key", 3.14159265359)
archive:writeString("key", "hello")
archive:writeBool("key", true)
archive:writeBytes("key", binaryData)  -- For raw binary

-- Reading
local int = archive:readInt("key")
local float = archive:readFloat("key")
local double = archive:readDouble("key")
local str = archive:readString("key")
local bool = archive:readBool("key")
local bytes = archive:readBytes("key")
```

## Saving Player State

```lua
-- entities/player.lua save/load extensions
return {
    -- ... existing code ...

    registerSaveable = function()
        bestow.save.registerSaveable({
            key = "player",

            serialize = function(archive)
                local state = app.main.state
                local player = state.player

                if not player or not bestow.entity.isValid(player) then
                    archive:writeBool("exists", false)
                    return
                end

                archive:writeBool("exists", true)

                -- Position
                local pos = bestow.entity.getField(player, "Transform3D", "position")
                archive:writeFloat("posX", pos.x)
                archive:writeFloat("posY", pos.y)
                archive:writeFloat("posZ", pos.z)

                -- Health
                local health = bestow.entity.getComponent(player, "Health")
                archive:writeInt("healthCurrent", health.current)
                archive:writeInt("healthMax", health.max)

                -- Custom player data
                local controller = bestow.entity.getComponent(player, "PlayerController")
                if controller then
                    archive:writeFloat("speed", controller.speed)
                    archive:writeFloat("jumpForce", controller.jumpForce)
                end
            end,

            deserialize = function(archive)
                if not archive:readBool("exists") then
                    return
                end

                local state = app.main.state

                -- Recreate player at saved position
                local pos = Vec3.new(
                    archive:readFloat("posX"),
                    archive:readFloat("posY"),
                    archive:readFloat("posZ")
                )
                state.player = app.entities.player.spawnAt(pos)

                -- Restore health
                local health = {
                    current = archive:readInt("healthCurrent"),
                    max = archive:readInt("healthMax")
                }
                bestow.entity.setComponent(state.player, "Health", health)
            end
        })
    end
}
```

## Auto-Save

```lua
-- Enable auto-save every 60 seconds
bestow.save.enableAutoSave(60.0)

-- Disable auto-save
bestow.save.disableAutoSave()

-- Manual auto-save trigger
bestow.save.autoSave()
```

### Auto-Save on Checkpoints

```lua
-- systems/checkpoints.lua
return {
    activate = function(checkpointId)
        local self = app.systems.checkpoints
        local state = app.main.state

        state.lastCheckpoint = checkpointId

        -- Auto-save on checkpoint
        bestow.save.autoSave()

        app.systems.audio.playSfx("checkpoint")
    end
}
```

## Save Profiles

For multiple save profiles (different players):

```lua
-- Get available profiles
local profiles = bestow.save.getProfiles()
for _, profile in ipairs(profiles) do
    print("Profile: " .. profile)
end

-- Set active profile
bestow.save.setActiveProfile("Player1")

-- Get active profile
local current = bestow.save.getActiveProfile()

-- Now all save/load operations use this profile
bestow.save.save(1)  -- Saves to Player1's slot 1
```

## Save Metadata

```lua
-- Get metadata for a save
local metadata = bestow.save.getSaveMetadata(1)
if metadata then
    print("Saved: " .. metadata.timestamp)
    print("Version: " .. metadata.gameVersion)
    print("Level: " .. metadata.currentLevel)
    print("Play time: " .. metadata.playTime .. " seconds")
    print("Completion: " .. metadata.completionPercentage .. "%")
end

-- Set metadata for next save
bestow.save.setGameVersion("1.0.0")
bestow.save.setCurrentLevel("level3")
bestow.save.setCompletionPercentage(45)
```

## Play Time Tracking

```lua
-- Get session play time
local sessionTime = bestow.save.getSessionPlaytime()

-- Get total play time (across all sessions)
local totalTime = bestow.save.getTotalPlaytime()

-- Reset session time (e.g., on new game)
bestow.save.resetSessionPlaytime()
```

## Delete Saves

```lua
-- Delete specific slot
bestow.save.deleteSave(1)

-- The save is removed from disk
```

## Complete Save System Integration

```lua
-- In main.lua
init = function()
    -- Initialize state
    app.main.state = {
        score = 0,
        lives = 3,
        currentLevel = "level1",
        lastCheckpoint = nil,
        totalPlayTime = 0,
        inventory = {}
    }

    -- Register all saveables
    app.systems.game_state.initSaveSystem()
    app.entities.player.registerSaveable()

    -- Enable auto-save
    bestow.save.enableAutoSave(120.0)  -- Every 2 minutes

    -- Check for continue game
    if bestow.save.saveExists(0) then  -- Auto-save slot
        -- Could prompt user to continue or new game
    end
end

-- Save menu option
saveGame = function(slot)
    bestow.save.setCurrentLevel(app.main.state.currentLevel)
    bestow.save.setCompletionPercentage(calculateCompletion())

    local result = bestow.save.save(slot)
    if result.success then
        app.systems.ui.showMessage("Game Saved")
    else
        app.systems.ui.showMessage("Save Failed: " .. result.error)
    end
end

-- Load menu option
loadGame = function(slot)
    local result = bestow.save.load(slot)
    if result.success then
        -- Reload the level we were on
        app.main.loadLevel(app.main.state.currentLevel)
        app.systems.ui.showMessage("Game Loaded")
    else
        app.systems.ui.showMessage("Load Failed: " .. result.error)
    end
end
```

## Best Practices

1. **Register saveables in init()** - Before any saves happen
2. **Save on checkpoints** - Natural save points
3. **Use auto-save as backup** - Don't rely only on manual saves
4. **Validate on load** - Handle missing or corrupt data
5. **Version your saves** - For compatibility across updates
6. **Track play time** - Players like to see it
7. **Use profiles for multiple players** - Separate save slots per profile
8. **Test save/load thoroughly** - It's easy to break

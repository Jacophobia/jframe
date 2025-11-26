-- data/levels/world1/level1.lua
-- First level of the platformer

local H = require("levels.helpers")

-- Local constants for this level
local GROUND_Y = 100
local LEVEL_WIDTH = 3200
local COIN_Y = 250

return {
    id = "world1/level1",
    displayName = "Green Hills",

    size = {
        width = LEVEL_WIDTH,
        height = 600
    },

    physics = {
        gravity = { x = 0, y = -980 }
    },

    background = {
        asset = "textures/backgrounds/hills.png",
        parallax = { x = 0.3, y = 0.1 }
    },

    music = {
        asset = "audio/music/level1.ogg",
        volume = 0.7,
        fadeIn = 2.0
    },

    -- Entities placed using Lua's power!
    entities = H.concat(
        -- Player spawn point
        {{
            id = "player_spawn",
            blueprint = "player",
            x = 100,
            y = GROUND_Y + 50
        }},

        -- Row of coins using a loop
        H.map(H.range(0, 9), function(i)
            return {
                id = "coin_" .. i,
                blueprint = "collectibles/coin",
                x = 300 + (i * 50),
                y = COIN_Y
            }
        end),

        -- Coins in a sine wave pattern
        H.wave("collectibles/coin", 800, COIN_Y, 15, 40, 30, 0.3, "wave_coin"),

        -- Grid of breakable blocks
        H.grid("interactables/brick", 600, 350, 5, 2, 32, 32, "brick"),

        -- Enemies at specific positions
        H.map({
            { x = 500, y = GROUND_Y + 20 },
            { x = 900, y = GROUND_Y + 20 },
            { x = 1400, y = GROUND_Y + 20 }
        }, function(pos, i)
            return {
                id = "slime_" .. i,
                blueprint = "enemies/slime",
                x = pos.x,
                y = pos.y
            }
        end),

        -- Debug content (only in debug builds)
        H.ifDebug({
            { id = "debug_warp", blueprint = "debug/warp", x = 50, y = 300 }
        })
    ),

    -- Spawn points for level transitions and respawns
    spawnPoints = {
        player_start = { x = 100, y = GROUND_Y + 50 },
        from_level2 = { x = LEVEL_WIDTH - 100, y = GROUND_Y + 50 },
        checkpoint_1 = { x = 1500, y = GROUND_Y + 50 }
    },

    -- Trigger regions
    regions = {
        {
            name = "exit_right",
            type = "transition",
            bounds = { x = LEVEL_WIDTH - 32, y = 0, width = 32, height = 600 },
            targetLevel = "world1/level2",
            targetSpawn = "from_level1"
        },
        {
            name = "death_zone",
            type = "kill",
            bounds = { x = 0, y = -100, width = LEVEL_WIDTH, height = 100 }
        }
    }
}

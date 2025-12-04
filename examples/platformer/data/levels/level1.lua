-- level1.lua
-- Tutorial Level - Showcases all Bestow engine systems

return {
    name = "Tutorial Level",
    width = 1600,
    height = 600,
    background = "sky_blue",

    -- Spawn points for entities (used by Level System)
    spawnPoints = {
        player = {x = 100, y = 400},
        enemy1 = {x = 600, y = 400},
        enemy2 = {x = 1000, y = 400},
        checkpoint = {x = 800, y = 400}
    },

    -- Static platform geometry (loaded by game code via Level System)
    platforms = {
        -- Ground platform (full width)
        {x = 800, y = 25, width = 1600, height = 50, type = "ground"},

        -- Floating platforms (ascending staircase)
        {x = 300, y = 150, width = 200, height = 20, type = "platform"},
        {x = 600, y = 250, width = 200, height = 20, type = "platform"},
        {x = 900, y = 350, width = 200, height = 20, type = "platform"},
        {x = 1200, y = 450, width = 200, height = 20, type = "platform"},

        -- Walls (optional boundaries)
        {x = 10, y = 300, width = 20, height = 600, type = "wall"},
        {x = 1590, y = 300, width = 20, height = 600, type = "wall"}
    },

    -- Collectibles (coins, power-ups)
    collectibles = {
        {x = 300, y = 200, type = "coin", value = 10},
        {x = 600, y = 300, type = "coin", value = 10},
        {x = 900, y = 400, type = "coin", value = 10},
        {x = 1200, y = 500, type = "coin", value = 10},
        {x = 400, y = 180, type = "health", value = 25}
    },

    -- Enemy definitions (for AI System)
    enemies = {
        {
            spawnPoint = "enemy1",
            type = "patroller",
            patrolRange = 150,
            moveSpeed = 50,
            damage = 10
        },
        {
            spawnPoint = "enemy2",
            type = "patroller",
            patrolRange = 100,
            moveSpeed = 75,
            damage = 15
        }
    }
}

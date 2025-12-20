-- World 1: Forest Realm
-- A peaceful forest with simple enemies and obstacles

return {
    name = "Forest Realm",
    theme = "forest",

    -- Level files in this world (relative paths)
    levels = {
        "level01.lua",
        "level02.lua",
        "level03.lua",
        "boss.lua"
    },

    -- Unlock requirements (cumulative food from previous levels in world)
    unlockRequirements = {
        [1] = 0,    -- Level 1 always unlocked
        [2] = 5,    -- Need 5 food to unlock level 2
        [3] = 15,   -- Need 15 food to unlock level 3
        [4] = 30    -- Need 30 food to unlock boss
    },

    -- World map layout (isometric grid positions)
    nodes = {
        { x = 5, z = 12, levelIndex = 1, name = "Forest Gate" },
        { x = 7, z = 9, levelIndex = 2, name = "Deep Woods" },
        { x = 10, z = 7, levelIndex = 3, name = "Ancient Tree" },
        { x = 12, z = 4, levelIndex = 4, name = "Forest King", isBoss = true }
    },

    -- Paths connecting nodes (pairs of node indices, 1-based)
    paths = {
        { 1, 2 },
        { 2, 3 },
        { 3, 4 }
    },

    -- Boss configuration for this world
    boss = {
        name = "Forest King",
        type = "snake_king",
        health = 10,
        attackPatterns = { "spawn_minions" }
    },

    -- Visual theme settings
    groundColor = { 0.15, 0.4, 0.2, 1.0 },
    skyColor = { 30, 120, 50, 255 },
    ambientColor = { 0.3, 0.35, 0.3 }
}

-- World: Forest Realm
-- A peaceful forest with simple enemies and obstacles

return {
    name = "Forest Realm",
    theme = "forest",

    -- Level files in this world (relative paths)
    levels = {
        "level01.lua"
    },

    -- Unlock requirements (cumulative food from previous levels in world)
    unlockRequirements = {
        [1] = 0     -- Level 1 always unlocked
    },

    -- World map layout (isometric grid positions)
    nodes = {
        { x = 5, z = 12, levelIndex = 1, name = "Forest Gate" }
    },

    -- Paths connecting nodes (pairs of node indices, 1-based)
    paths = {},

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

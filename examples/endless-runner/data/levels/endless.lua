-- Endless Runner Level
-- This demonstrates procedural entity generation using a Lua function
-- The LevelLoader now supports entities = function() return {...} end

-- Configuration for procedural generation
local GROUND_Y = 550
local SECTION_WIDTH = 800
local START_X = 400

-- Helper function to generate random entities for starting area
local function generateStartingEntities()
    local entities = {}

    -- Add some starting platforms
    table.insert(entities, {
        type = "platform",
        x = START_X + 150,
        y = GROUND_Y - 120,
        width = 120,
        height = 30
    })

    table.insert(entities, {
        type = "platform",
        x = START_X + 400,
        y = GROUND_Y - 180,
        width = 100,
        height = 30
    })

    -- Starting coins (easy to collect)
    for i = 1, 5 do
        table.insert(entities, {
            type = "coin",
            x = 300 + (i * 80),
            y = GROUND_Y - 60,
            value = 10
        })
    end

    -- Coins on platforms
    table.insert(entities, { type = "coin", x = START_X + 150, y = GROUND_Y - 160, value = 10 })
    table.insert(entities, { type = "coin", x = START_X + 400, y = GROUND_Y - 220, value = 10 })

    return entities
end

return {
    name = "Endless Runner",
    width = 10000,  -- Infinite conceptually
    height = 720,

    spawnPoints = {
        player = { x = 200, y = GROUND_Y - 100 },
    },

    -- Using a generator function instead of static table!
    -- This is called once at level load time
    -- Runtime generation is handled in C++ Game::generateNextSection()
    entities = generateStartingEntities
}

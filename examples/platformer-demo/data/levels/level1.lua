-- Demo Level 1 - Showcasing Bestow engine features
-- Entity definitions can reference blueprints via the 'blueprint' field
-- The 'type' field is used by Game.cpp for entity creation

return {
    name = "Demo Level 1",

    -- Spawn points are keyed by name
    spawnPoints = {
        player = { x = 100, y = 400 },
    },

    -- Entity definitions
    -- Each entity can have:
    --   type: Entity type (required - used by Game.cpp)
    --   blueprint: Reference to blueprint file (for documentation/future use)
    --   x, y: Position
    --   Additional properties are passed to the creation function
    entities = {
        -- Ground platforms (static physics bodies)
        { type = "platform", blueprint = "platform", x = 400, y = 550, width = 800, height = 50 },
        { type = "platform", blueprint = "platform", x = 200, y = 400, width = 150, height = 30 },
        { type = "platform", blueprint = "platform", x = 500, y = 300, width = 150, height = 30 },
        { type = "platform", blueprint = "platform", x = 700, y = 200, width = 150, height = 30 },

        -- Collectibles (sensor bodies - will trigger events)
        { type = "coin", blueprint = "items/coin", x = 200, y = 350, value = 10 },
        { type = "coin", blueprint = "items/coin", x = 250, y = 350, value = 10 },
        { type = "coin", blueprint = "items/coin", x = 500, y = 250, value = 10 },
        { type = "coin", blueprint = "items/coin", x = 550, y = 250, value = 10 },
        { type = "coin", blueprint = "items/coin", x = 700, y = 150, value = 10 },
        { type = "coin", blueprint = "items/coin", x = 750, y = 150, value = 10 },

        -- Enemies (with AI patrol behavior)
        { type = "enemy", blueprint = "enemies/slime", x = 400, y = 500, patrolRange = 100 },
        { type = "enemy", blueprint = "enemies/slime", x = 600, y = 500, patrolRange = 150 },
    }
}

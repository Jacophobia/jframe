-- template/data/levels/main.lua
-- Main Level Definition
-- Uses Lua programming for efficient level design

-- ============================================================================
-- Constants
-- ============================================================================

local GROUND_Y = 550        -- Y position of ground level
local PLATFORM_HEIGHT = 20  -- Standard platform height
local SCREEN_WIDTH = 800
local SCREEN_HEIGHT = 600

-- ============================================================================
-- Helper Functions
-- ============================================================================

-- Create a platform entity definition
local function makePlatform(x, y, width, height)
    return {
        type = "platform",
        x = x,
        y = y or GROUND_Y,
        width = width or 200,
        height = height or PLATFORM_HEIGHT
    }
end

-- ============================================================================
-- Build Level Entities
-- ============================================================================

local entities = {}

-- Helper to add multiple entities at once
local function addEntities(newEntities)
    for _, entity in ipairs(newEntities) do
        table.insert(entities, entity)
    end
end

-- ============================================================================
-- MAIN AREA
-- ============================================================================

-- Ground floor (full width)
table.insert(entities, makePlatform(SCREEN_WIDTH / 2, GROUND_Y, SCREEN_WIDTH, 30))

-- Starting platform (left side)
table.insert(entities, makePlatform(100, 450, 150, PLATFORM_HEIGHT))

-- Middle platforms (staircase pattern)
for i = 1, 5 do
    table.insert(entities, makePlatform(
        200 + i * 80,           -- X position (spacing = 80)
        500 - i * 40,           -- Y position (descending)
        100,                    -- Width
        PLATFORM_HEIGHT         -- Height
    ))
end

-- High platform (requires jump)
table.insert(entities, makePlatform(600, 300, 120, PLATFORM_HEIGHT))

-- Right side landing platform
table.insert(entities, makePlatform(700, 450, 150, PLATFORM_HEIGHT))

-- Example: Create platforms in a pattern using loops
-- Uncomment to add a wavy pattern of platforms
--[[
for i = 1, 10 do
    local x = 100 + i * 70
    local y = 400 + math.sin(i * 0.5) * 80  -- Sine wave pattern
    table.insert(entities, makePlatform(x, y, 60, PLATFORM_HEIGHT))
end
]]--

-- ============================================================================
-- Return Level Definition
-- ============================================================================

return {
    -- Level metadata
    name = "Main Level",
    width = SCREEN_WIDTH,
    height = SCREEN_HEIGHT,

    -- Player spawn points
    spawnPoints = {
        player = {x = 100, y = 400}  -- Start on left platform
    },

    -- All entities in the level
    entities = entities,

    -- Optional level metadata
    metadata = {
        difficulty = "easy",
        description = "A simple platforming level to demonstrate JFrame basics"
    }
}

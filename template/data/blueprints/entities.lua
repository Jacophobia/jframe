-- template/data/blueprints/entities.lua
-- Entity Blueprint Definitions
-- Defines reusable entity templates for creating game objects

-- Color constants for consistent visuals
local Colors = {
    Player = {50, 200, 50, 255},            -- Green
    PlayerOutline = {25, 100, 25, 255},     -- Dark green
    Platform = {100, 100, 100, 255},        -- Gray
    PlatformOutline = {64, 64, 64, 255},    -- Dark gray
}

-- Render layer constants (higher = rendered on top)
local Layers = {
    Background = -100,
    Platforms = 0,
    Items = 20,
    Player = 40,
    Effects = 60,
}

-- Blueprint definitions
-- Each blueprint defines components and physics properties
Blueprints = {
    -- ========================================================================
    -- Player Entity
    -- ========================================================================
    Player = {
        components = {
            -- Custom tag component (identifies player)
            PlayerTag = {},

            -- Visual representation (debug rectangle)
            DebugRect = {
                fillColor = Colors.Player,
                outlineColor = Colors.PlayerOutline,
                outlineWidth = 2,
                layer = Layers.Player,
                filled = true
            }
        },

        -- Physics body configuration
        physics = {
            type = "dynamic",           -- Affected by gravity and forces
            fixedRotation = true,       -- Prevent rotation
            linearDamping = 0.0,        -- No air resistance
            friction = 0.0,             -- No surface friction (prevents wall sticking)
            collisionLayer = "Player"   -- Player collision layer
        }
    },

    -- ========================================================================
    -- Platform Entity
    -- ========================================================================
    Platform = {
        components = {
            -- Custom tag component (identifies platforms)
            PlatformTag = {},

            -- Visual representation
            DebugRect = {
                fillColor = Colors.Platform,
                outlineColor = Colors.PlatformOutline,
                outlineWidth = 1,
                layer = Layers.Platforms,
                filled = true
            }
        },

        -- Physics body configuration
        physics = {
            type = "static",            -- Doesn't move
            collisionLayer = "Ground"   -- Ground collision layer
        }
    }
}

return Blueprints

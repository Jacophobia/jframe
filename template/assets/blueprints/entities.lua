-- Bestow Template - Entity Blueprints
-- Simple 2D platformer entities using debug shapes

-- Color palette
local Colors = {
    -- Player
    Player = {50, 200, 100, 255},
    PlayerOutline = {30, 150, 70, 255},

    -- Platforms
    Platform = {100, 100, 120, 255},
    PlatformOutline = {60, 60, 80, 255},

    -- Obstacles
    Obstacle = {200, 80, 80, 255},
    ObstacleOutline = {150, 50, 50, 255},

    -- Collectables
    Coin = {255, 215, 0, 255},
    CoinOutline = {200, 170, 0, 255},
}

-- Render layer constants
local Layers = {
    Background = -100,
    Platforms = 0,
    Items = 20,
    Player = 40,
}

Blueprints = {
    -- Player entity: create("player", x, y)
    player = {
        components = {
            PlayerTag = {},
            DebugRect = {
                size = {32, 48},
                fillColor = Colors.Player,
                outlineColor = Colors.PlayerOutline,
                outlineWidth = 2,
                layer = Layers.Player
            }
        },
        physics = {
            type = "dynamic",
            size = {32, 48},
            fixedRotation = true,
            density = 1.0,
            friction = 0.0,
            restitution = 0.0,
            collisionLayer = "Player"
        }
    },

    -- Platform entity: create("platform", x, y, width, height)
    platform = {
        components = {
            PlatformTag = {},
            DebugRect = {
                fillColor = Colors.Platform,
                outlineColor = Colors.PlatformOutline,
                outlineWidth = 1,
                layer = Layers.Platforms
            }
        },
        physics = {
            type = "static",
            friction = 0.5,
            collisionLayer = "Ground"
        }
    },

    -- Obstacle entity: create("obstacle", x, y, width, height)
    obstacle = {
        components = {
            ObstacleTag = {},
            DebugRect = {
                fillColor = Colors.Obstacle,
                outlineColor = Colors.ObstacleOutline,
                outlineWidth = 2,
                layer = Layers.Platforms
            }
        },
        physics = {
            type = "static",
            sensor = true,
            collisionLayer = "Hazard"
        }
    },

    -- Coin collectible: create("coin", x, y)
    coin = {
        components = {
            CoinTag = {
                value = 1
            },
            DebugCircle = {
                radius = 12,
                fillColor = Colors.Coin,
                outlineColor = Colors.CoinOutline,
                outlineWidth = 2,
                layer = Layers.Items
            }
        },
        physics = {
            type = "static",
            size = {24, 24},
            sensor = true,
            collisionLayer = "Pickup"
        }
    }
}

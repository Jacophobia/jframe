-- blueprints/entities.lua
-- Entity blueprints for the example game

return {
    -- Base character with common properties
    Character = {
        components = {
            Transform = {
                x = 0, y = 0,
                rotation = 0,
                scaleX = 1, scaleY = 1
            },
            Velocity = {
                x = 0, y = 0
            }
        },
        physics = {
            bodyType = "dynamic",
            fixedRotation = true,
            density = 1.0,
            friction = 0.3
        }
    },

    -- Player inherits from Character
    Player = {
        inherits = "Character",
        behavior = "player_controller",
        components = {
            Transform = {
                scaleX = 1.5,
                scaleY = 1.5
            },
            Tag = { value = "player" }
        },
        metadata = {
            maxHealth = 100,
            speed = 200
        }
    },

    -- Enemy inherits from Character
    Enemy = {
        inherits = "Character",
        behavior = "enemy_ai",
        components = {
            Tag = { value = "enemy" }
        },
        metadata = {
            damage = 10,
            speed = 100
        }
    },

    -- Static platform
    Platform = {
        components = {
            Transform = {
                x = 0, y = 0,
                scaleX = 1, scaleY = 1
            },
            Tag = { value = "platform" }
        },
        physics = {
            bodyType = "static",
            friction = 0.8
        }
    },

    -- Collectible item
    Coin = {
        behavior = "collectible",
        components = {
            Transform = {
                x = 0, y = 0
            },
            Tag = { value = "coin" }
        },
        physics = {
            bodyType = "static",
            sensor = true
        },
        metadata = {
            points = 10
        }
    }
}

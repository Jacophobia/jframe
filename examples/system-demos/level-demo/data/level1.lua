-- Level 1: Tutorial Area
-- This level demonstrates entity definitions, spawn points, and level metadata

return {
    -- Level Metadata
    metadata = {
        name = "Tutorial Village",
        width = 1920.0,
        height = 1080.0,
        author = "Bestow Demo",
        description = "A simple tutorial level for demonstrating the Level System API"
    },

    -- Spawn Points (player can start at any of these)
    spawnPoints = {
        -- Default spawn point (center of level)
        default = {
            x = 960.0,
            y = 540.0,
            rotation = 0.0
        },

        -- Spawn after entering from left
        fromLeft = {
            x = 100.0,
            y = 540.0,
            rotation = 0.0
        },

        -- Spawn after entering from right
        fromRight = {
            x = 1820.0,
            y = 540.0,
            rotation = 180.0
        },

        -- Checkpoint spawn
        checkpoint1 = {
            x = 1200.0,
            y = 300.0,
            rotation = 0.0
        }
    },

    -- Entity Definitions
    entities = {
        -- Ground platforms
        {
            type = "platform",
            transform = {
                x = 960.0,
                y = 900.0,
                rotation = 0.0,
                scaleX = 1.0,
                scaleY = 1.0
            },
            properties = {
                width = 1920.0,
                height = 100.0,
                texture = "ground_grass",
                isStatic = true,
                layer = 0
            }
        },

        -- Left wall platform
        {
            type = "platform",
            transform = {
                x = 300.0,
                y = 700.0,
                rotation = 0.0
            },
            properties = {
                width = 400.0,
                height = 50.0,
                texture = "platform_wood",
                isStatic = true
            }
        },

        -- Middle floating platform
        {
            type = "platform",
            transform = {
                x = 960.0,
                y = 500.0,
                rotation = 0.0
            },
            properties = {
                width = 300.0,
                height = 40.0,
                texture = "platform_stone",
                isStatic = true
            }
        },

        -- Right wall platform
        {
            type = "platform",
            transform = {
                x = 1620.0,
                y = 650.0,
                rotation = 0.0
            },
            properties = {
                width = 400.0,
                height = 50.0,
                texture = "platform_wood",
                isStatic = true
            }
        },

        -- Enemy 1: Patrol enemy on left platform
        {
            type = "enemy",
            transform = {
                x = 300.0,
                y = 650.0,
                rotation = 0.0
            },
            properties = {
                enemyType = "walker",
                health = 100,
                damage = 10,
                patrolDistance = 150.0,
                speed = 50.0,
                color = "red"
            }
        },

        -- Enemy 2: Stationary enemy on middle platform
        {
            type = "enemy",
            transform = {
                x = 960.0,
                y = 450.0,
                rotation = 0.0
            },
            properties = {
                enemyType = "stationary",
                health = 50,
                damage = 20,
                attackRange = 200.0,
                color = "blue"
            }
        },

        -- Collectible 1: Coin on left platform
        {
            type = "collectible",
            transform = {
                x = 250.0,
                y = 650.0,
                rotation = 0.0
            },
            properties = {
                collectibleType = "coin",
                value = 10,
                effect = "score"
            }
        },

        -- Collectible 2: Power-up on middle platform
        {
            type = "collectible",
            transform = {
                x = 1000.0,
                y = 450.0,
                rotation = 0.0
            },
            properties = {
                collectibleType = "powerup",
                value = 1,
                effect = "double_jump",
                duration = 10.0
            }
        },

        -- Collectible 3: Health pack on right platform
        {
            type = "collectible",
            transform = {
                x = 1650.0,
                y = 600.0,
                rotation = 0.0
            },
            properties = {
                collectibleType = "health",
                value = 25,
                effect = "restore_health"
            }
        },

        -- Level exit trigger
        {
            type = "exit",
            transform = {
                x = 1820.0,
                y = 600.0,
                rotation = 0.0
            },
            properties = {
                width = 64.0,
                height = 128.0,
                targetLevel = "level2",
                targetSpawn = "fromLeft",
                triggerZone = true
            }
        },

        -- Decorative elements
        {
            type = "decoration",
            transform = {
                x = 150.0,
                y = 850.0,
                rotation = 0.0
            },
            properties = {
                sprite = "tree_oak",
                layer = -10,
                width = 100.0,
                height = 200.0
            }
        },

        {
            type = "decoration",
            transform = {
                x = 1750.0,
                y = 850.0,
                rotation = 0.0
            },
            properties = {
                sprite = "tree_pine",
                layer = -10,
                width = 120.0,
                height = 220.0
            }
        }
    }
}

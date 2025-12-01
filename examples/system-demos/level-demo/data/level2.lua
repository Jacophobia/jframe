-- Level 2: Dungeon Depths
-- This level demonstrates more complex entity definitions and procedural generation patterns

return {
    -- Level Metadata
    metadata = {
        name = "Dungeon Depths",
        width = 2560.0,
        height = 1440.0,
        author = "JFrame Demo",
        description = "A more challenging dungeon level with multiple enemies and hazards"
    },

    -- Spawn Points
    spawnPoints = {
        -- Default spawn (entrance from level 1)
        default = {
            x = 150.0,
            y = 1200.0,
            rotation = 0.0
        },

        -- From left (level transition)
        fromLeft = {
            x = 150.0,
            y = 1200.0,
            rotation = 0.0
        },

        -- Mid-level checkpoint
        checkpoint1 = {
            x = 1280.0,
            y = 720.0,
            rotation = 0.0
        },

        -- Boss arena spawn
        bossArena = {
            x = 2200.0,
            y = 1200.0,
            rotation = 0.0
        }
    },

    -- Entity Definitions
    entities = {
        -- Main floor
        {
            type = "platform",
            transform = {
                x = 1280.0,
                y = 1350.0,
                rotation = 0.0
            },
            properties = {
                width = 2560.0,
                height = 150.0,
                texture = "dungeon_floor",
                isStatic = true,
                layer = 0
            }
        },

        -- Staircase platforms (left side, ascending)
        {
            type = "platform",
            transform = {
                x = 400.0,
                y = 1150.0,
                rotation = 0.0
            },
            properties = {
                width = 200.0,
                height = 40.0,
                texture = "platform_stone",
                isStatic = true
            }
        },

        {
            type = "platform",
            transform = {
                x = 600.0,
                y = 1000.0,
                rotation = 0.0
            },
            properties = {
                width = 200.0,
                height = 40.0,
                texture = "platform_stone",
                isStatic = true
            }
        },

        {
            type = "platform",
            transform = {
                x = 800.0,
                y = 850.0,
                rotation = 0.0
            },
            properties = {
                width = 200.0,
                height = 40.0,
                texture = "platform_stone",
                isStatic = true
            }
        },

        -- Upper level platform
        {
            type = "platform",
            transform = {
                x = 1280.0,
                y = 700.0,
                rotation = 0.0
            },
            properties = {
                width = 800.0,
                height = 50.0,
                texture = "platform_stone",
                isStatic = true
            }
        },

        -- Boss arena platform (right side)
        {
            type = "platform",
            transform = {
                x = 2200.0,
                y = 1200.0,
                rotation = 0.0
            },
            properties = {
                width = 600.0,
                height = 80.0,
                texture = "platform_boss",
                isStatic = true
            }
        },

        -- Enemies: Ground patrol (3 enemies)
        {
            type = "enemy",
            transform = {
                x = 600.0,
                y = 1300.0,
                rotation = 0.0
            },
            properties = {
                enemyType = "walker",
                health = 150,
                damage = 15,
                patrolDistance = 300.0,
                speed = 60.0,
                color = "purple"
            }
        },

        {
            type = "enemy",
            transform = {
                x = 1200.0,
                y = 1300.0,
                rotation = 0.0
            },
            properties = {
                enemyType = "walker",
                health = 150,
                damage = 15,
                patrolDistance = 400.0,
                speed = 70.0,
                color = "purple"
            }
        },

        {
            type = "enemy",
            transform = {
                x = 1800.0,
                y = 1300.0,
                rotation = 0.0
            },
            properties = {
                enemyType = "walker",
                health = 150,
                damage = 15,
                patrolDistance = 250.0,
                speed = 50.0,
                color = "purple"
            }
        },

        -- Flying enemy on upper platform
        {
            type = "enemy",
            transform = {
                x = 1280.0,
                y = 500.0,
                rotation = 0.0
            },
            properties = {
                enemyType = "flyer",
                health = 100,
                damage = 20,
                patrolDistance = 200.0,
                speed = 80.0,
                color = "green",
                canFly = true
            }
        },

        -- Boss enemy
        {
            type = "enemy",
            transform = {
                x = 2200.0,
                y = 1100.0,
                rotation = 0.0
            },
            properties = {
                enemyType = "boss",
                health = 500,
                damage = 30,
                speed = 40.0,
                color = "dark_red",
                isBoss = true,
                phases = 3,
                attackPatterns = {"charge", "projectile", "area"}
            }
        },

        -- Hazards: Spikes
        {
            type = "hazard",
            transform = {
                x = 900.0,
                y = 1300.0,
                rotation = 0.0
            },
            properties = {
                hazardType = "spikes",
                damage = 25,
                width = 100.0,
                height = 32.0,
                isPermanent = true
            }
        },

        {
            type = "hazard",
            transform = {
                x = 1500.0,
                y = 1300.0,
                rotation = 0.0
            },
            properties = {
                hazardType = "spikes",
                damage = 25,
                width = 150.0,
                height = 32.0,
                isPermanent = true
            }
        },

        -- Collectibles: Coins scattered around
        {
            type = "collectible",
            transform = {
                x = 400.0,
                y = 1100.0,
                rotation = 0.0
            },
            properties = {
                collectibleType = "coin",
                value = 10,
                effect = "score"
            }
        },

        {
            type = "collectible",
            transform = {
                x = 600.0,
                y = 950.0,
                rotation = 0.0
            },
            properties = {
                collectibleType = "coin",
                value = 10,
                effect = "score"
            }
        },

        {
            type = "collectible",
            transform = {
                x = 800.0,
                y = 800.0,
                rotation = 0.0
            },
            properties = {
                collectibleType = "coin",
                value = 10,
                effect = "score"
            }
        },

        -- Power-ups
        {
            type = "collectible",
            transform = {
                x = 1280.0,
                y = 650.0,
                rotation = 0.0
            },
            properties = {
                collectibleType = "powerup",
                value = 1,
                effect = "invincibility",
                duration = 5.0
            }
        },

        -- Health packs
        {
            type = "collectible",
            transform = {
                x = 1000.0,
                y = 1300.0,
                rotation = 0.0
            },
            properties = {
                collectibleType = "health",
                value = 50,
                effect = "restore_health"
            }
        },

        {
            type = "collectible",
            transform = {
                x = 1900.0,
                y = 1150.0,
                rotation = 0.0
            },
            properties = {
                collectibleType = "health",
                value = 75,
                effect = "restore_health"
            }
        },

        -- Checkpoint trigger
        {
            type = "checkpoint",
            transform = {
                x = 1280.0,
                y = 650.0,
                rotation = 0.0
            },
            properties = {
                width = 64.0,
                height = 128.0,
                checkpointName = "checkpoint1",
                triggerZone = true
            }
        },

        -- Level exit (after defeating boss)
        {
            type = "exit",
            transform = {
                x = 2400.0,
                y = 1150.0,
                rotation = 0.0
            },
            properties = {
                width = 64.0,
                height = 128.0,
                targetLevel = "level1",
                targetSpawn = "fromRight",
                triggerZone = true,
                requiresBossDefeat = true
            }
        },

        -- Decorative elements: Torches
        {
            type = "decoration",
            transform = {
                x = 200.0,
                y = 1250.0,
                rotation = 0.0
            },
            properties = {
                sprite = "torch",
                layer = -5,
                width = 32.0,
                height = 64.0,
                animated = true,
                lightRadius = 150.0
            }
        },

        {
            type = "decoration",
            transform = {
                x = 1280.0,
                y = 650.0,
                rotation = 0.0
            },
            properties = {
                sprite = "torch",
                layer = -5,
                width = 32.0,
                height = 64.0,
                animated = true,
                lightRadius = 150.0
            }
        },

        {
            type = "decoration",
            transform = {
                x = 2200.0,
                y = 1150.0,
                rotation = 0.0
            },
            properties = {
                sprite = "torch",
                layer = -5,
                width = 32.0,
                height = 64.0,
                animated = true,
                lightRadius = 150.0
            }
        }
    }
}

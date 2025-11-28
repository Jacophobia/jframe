-- ability-demo/data/blueprints/entities.lua
-- Entity blueprint definitions for the ability demo

-- Color constants
local Colors = {
    -- Platforms
    Platform = {100, 100, 100, 255},       -- Gray
    PlatformOutline = {64, 64, 64, 255},   -- Dark gray

    -- Jump Zones
    JumpZone = {255, 255, 0, 80},          -- Yellow transparent
    JumpZoneOutline = {255, 255, 0, 255},  -- Yellow solid

    -- Collectables
    Collectable = {0, 255, 255, 255},      -- Cyan
    CollectableOutline = {255, 255, 255, 255}, -- White

    -- Doors
    Door = {139, 90, 43, 255},             -- Brown
    DoorOutline = {100, 60, 30, 255},      -- Dark brown

    -- Breakables
    Breakable = {210, 180, 140, 255},      -- Tan
    BreakableOutline = {139, 90, 43, 255}, -- Brown

    -- Switches
    SwitchInactive = {0, 200, 0, 255},     -- Green
    SwitchActive = {200, 0, 0, 255},       -- Red
    SwitchOutline = {50, 50, 50, 255},     -- Dark gray

    -- Moving Platforms
    MovingPlatform = {139, 90, 43, 255},   -- Brown/orange
    MovingPlatformOutline = {100, 60, 30, 255},

    -- Health Pickups
    HealthPickup = {255, 100, 150, 255},   -- Pink
    HealthPickupOutline = {200, 50, 100, 255},

    -- Checkpoints
    CheckpointInactive = {50, 50, 200, 255}, -- Blue
    CheckpointActive = {255, 215, 0, 255},   -- Gold
    CheckpointOutline = {30, 30, 150, 255},

    -- Enemies
    EnemyWalker = {200, 50, 50, 255},      -- Red
    EnemyJumper = {139, 0, 0, 255},        -- Dark red
    EnemyShooter = {200, 0, 200, 255},     -- Magenta
    EnemyFlying = {255, 140, 0, 255},      -- Orange
    EnemyOutline = {100, 25, 25, 255},     -- Dark red

    -- Boss phases
    BossPhase1 = {139, 0, 0, 255},         -- Dark red
    BossPhase2 = {180, 60, 30, 255},       -- Reddish orange
    BossPhase3 = {255, 140, 0, 255},       -- Orange
    BossOutline = {50, 0, 0, 255},

    -- Projectiles
    ProjectileEnemy = {255, 50, 50, 255},  -- Red
    ProjectilePlayer = {50, 50, 255, 255}, -- Blue
    ProjectileOutline = {200, 200, 200, 255},

    -- Player
    PlayerNormal = {50, 200, 50, 255},     -- Green
    PlayerDashing = {0, 255, 255, 255},    -- Cyan
    PlayerRegen = {100, 255, 100, 255},    -- Bright green
    PlayerStunned = {150, 50, 200, 255},   -- Purple
    PlayerOutline = {25, 100, 25, 255},

    -- Trigger zones (debug)
    TriggerZone = {128, 0, 128, 40},       -- Faint purple
    TriggerZoneOutline = {128, 0, 128, 100},
}

-- Render layer constants
local Layers = {
    Background = -100,
    Platforms = 0,
    JumpZones = 5,
    Switches = 10,
    Doors = 15,
    Items = 20,
    Enemies = 30,
    Player = 40,
    Projectiles = 50,
    Effects = 60,
    Debug = 100,
}

Blueprints = {
    --==========================================================================
    -- Platforms
    --==========================================================================

    Platform = {
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
            collisionLayer = "Ground"
        }
    },

    JumpZone = {
        components = {
            JumpZoneTag = {},
            DebugRect = {
                fillColor = Colors.JumpZone,
                outlineColor = Colors.JumpZoneOutline,
                outlineWidth = 2,
                layer = Layers.JumpZones
            }
        },
        physics = {
            type = "static",
            sensor = true,
            collisionLayer = "JumpZone"
        }
    },

    MovingPlatform = {
        inherits = "Platform",
        components = {
            MovingPlatform = {
                speed = 50
            },
            DebugRect = {
                fillColor = Colors.MovingPlatform,
                outlineColor = Colors.MovingPlatformOutline,
                outlineWidth = 2
            }
        },
        physics = {
            type = "kinematic",
            collisionLayer = "Ground"
        }
    },

    --==========================================================================
    -- Collectables & Pickups
    --==========================================================================

    Collectable = {
        components = {
            CollectableTag = {
                abilityToGrant = "unknown"
            },
            DebugRect = {
                size = {32, 32},
                fillColor = Colors.Collectable,
                outlineColor = Colors.CollectableOutline,
                outlineWidth = 2,
                layer = Layers.Items
            }
        },
        physics = {
            type = "static",
            size = {32, 32},
            sensor = true,
            collisionLayer = "Collectable"
        }
    },

    HealthPickup = {
        components = {
            HealthPickup = {
                healAmount = 25
            },
            DebugRect = {
                size = {24, 24},
                fillColor = Colors.HealthPickup,
                outlineColor = Colors.HealthPickupOutline,
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
    },

    Checkpoint = {
        components = {
            Checkpoint = {
                activated = false
            },
            DebugRect = {
                size = {20, 60},
                fillColor = Colors.CheckpointInactive,
                outlineColor = Colors.CheckpointOutline,
                outlineWidth = 2,
                layer = Layers.Items
            }
        },
        physics = {
            type = "static",
            size = {20, 60},
            sensor = true,
            collisionLayer = "Checkpoint"
        }
    },

    --==========================================================================
    -- Level Mechanics
    --==========================================================================

    Switch = {
        components = {
            SwitchTag = {
                targetDoorId = 0,
                activated = false
            },
            DebugRect = {
                size = {40, 10},
                fillColor = Colors.SwitchInactive,
                outlineColor = Colors.SwitchOutline,
                outlineWidth = 1,
                layer = Layers.Switches
            }
        },
        physics = {
            type = "static",
            size = {40, 10},
            sensor = true,
            collisionLayer = "Switch"
        }
    },

    Door = {
        components = {
            DoorTag = {
                doorId = 0,
                isOpen = false
            },
            DebugRect = {
                fillColor = Colors.Door,
                outlineColor = Colors.DoorOutline,
                outlineWidth = 2,
                layer = Layers.Doors
            }
        },
        physics = {
            type = "static",
            collisionLayer = "Ground"
        }
    },

    Breakable = {
        components = {
            BreakableTag = {
                hits = 1
            },
            DebugRect = {
                fillColor = Colors.Breakable,
                outlineColor = Colors.BreakableOutline,
                outlineWidth = 2,
                layer = Layers.Platforms
            }
        },
        physics = {
            type = "static",
            collisionLayer = "Ground"
        }
    },

    TriggerZone = {
        components = {
            TriggerZone = {
                event = "",
                triggered = false
            },
            DebugRect = {
                fillColor = Colors.TriggerZone,
                outlineColor = Colors.TriggerZoneOutline,
                outlineWidth = 1,
                layer = Layers.Debug
            }
        },
        physics = {
            type = "static",
            sensor = true,
            collisionLayer = "Trigger"
        }
    },

    --==========================================================================
    -- Enemies
    --==========================================================================

    Enemy = {
        -- Base enemy blueprint (not used directly)
        components = {
            EnemyTag = {
                type = "basic",
                health = 30,
                damage = 10,
                detectionRange = 200
            },
            DebugRect = {
                size = {32, 32},
                fillColor = Colors.EnemyWalker,
                outlineColor = Colors.EnemyOutline,
                outlineWidth = 1,
                layer = Layers.Enemies
            }
        },
        physics = {
            type = "dynamic",
            size = {32, 32},
            fixedRotation = true,
            collisionLayer = "Enemy"
        }
    },

    WalkerEnemy = {
        inherits = "Enemy",
        components = {
            EnemyTag = {
                type = "walker",
                health = 30,
                damage = 10
            },
            WalkerAI = {
                patrolLeft = 0,
                patrolRight = 200,
                speed = 80,
                movingRight = true
            },
            DebugRect = {
                fillColor = Colors.EnemyWalker
            }
        }
    },

    JumperEnemy = {
        inherits = "Enemy",
        components = {
            EnemyTag = {
                type = "jumper",
                health = 40,
                damage = 15
            },
            JumperAI = {
                jumpForce = 350,
                jumpCooldown = 1.5,
                jumpTimer = 0
            },
            DebugRect = {
                fillColor = Colors.EnemyJumper
            }
        }
    },

    ShooterEnemy = {
        inherits = "Enemy",
        components = {
            EnemyTag = {
                type = "shooter",
                health = 25,
                damage = 5
            },
            ShooterAI = {
                fireRate = 2.0,
                fireTimer = 0,
                projectileSpeed = 300
            },
            DebugRect = {
                fillColor = Colors.EnemyShooter
            }
        }
    },

    FlyingEnemy = {
        inherits = "Enemy",
        components = {
            EnemyTag = {
                type = "flying",
                health = 20,
                damage = 10
            },
            FlyingAI = {
                topY = 0,
                bottomY = 100,
                speed = 60,
                movingUp = true
            },
            DebugRect = {
                fillColor = Colors.EnemyFlying
            }
        },
        physics = {
            type = "kinematic"  -- Flying enemies don't use gravity
        }
    },

    Boss = {
        components = {
            BossTag = {
                maxHealth = 500,
                phase = 1,
                attackCooldown = 2.0
            },
            EnemyTag = {
                type = "boss",
                health = 500,
                damage = 25,
                detectionRange = 400
            },
            DebugRect = {
                size = {64, 64},
                fillColor = Colors.BossPhase1,
                outlineColor = Colors.BossOutline,
                outlineWidth = 3,
                layer = Layers.Enemies
            }
        },
        physics = {
            type = "dynamic",
            size = {64, 64},
            fixedRotation = true,
            collisionLayer = "Enemy"
        }
    },

    --==========================================================================
    -- Projectiles
    --==========================================================================

    EnemyProjectile = {
        components = {
            Projectile = {
                velocityX = 0,
                velocityY = 0,
                damage = 5,
                isEnemyProjectile = true,
                lifetime = 3.0
            },
            DebugCircle = {
                radius = 6,
                fillColor = Colors.ProjectileEnemy,
                outlineColor = Colors.ProjectileOutline,
                outlineWidth = 1,
                layer = Layers.Projectiles
            }
        },
        physics = {
            type = "dynamic",
            size = {12, 12},
            sensor = true,
            collisionLayer = "EnemyProjectile"
        }
    },

    PlayerProjectile = {
        components = {
            Projectile = {
                velocityX = 0,
                velocityY = 0,
                damage = 15,
                isEnemyProjectile = false,
                lifetime = 2.0
            },
            DebugCircle = {
                radius = 8,
                fillColor = Colors.ProjectilePlayer,
                outlineColor = Colors.ProjectileOutline,
                outlineWidth = 1,
                layer = Layers.Projectiles
            }
        },
        physics = {
            type = "dynamic",
            size = {16, 16},
            sensor = true,
            collisionLayer = "PlayerProjectile"
        }
    },

    --==========================================================================
    -- Player
    --==========================================================================

    Player = {
        components = {
            PlayerTag = {},
            PlayerController = {
                moveSpeed = 200,
                jumpForce = 400,
                isGrounded = false
            },
            DebugRect = {
                size = {32, 48},
                fillColor = Colors.PlayerNormal,
                outlineColor = Colors.PlayerOutline,
                outlineWidth = 2,
                layer = Layers.Player
            }
        },
        physics = {
            type = "dynamic",
            size = {32, 48},
            fixedRotation = true,
            collisionLayer = "Player"
        }
    },

    --==========================================================================
    -- Effects (Temporary Entities)
    --==========================================================================

    SwordHitbox = {
        components = {
            SwordHitbox = {
                damage = 10,
                lifetime = 0.2
            },
            DebugRect = {
                size = {40, 30},
                fillColor = {255, 255, 255, 100},
                outlineColor = {255, 255, 255, 200},
                outlineWidth = 1,
                layer = Layers.Effects
            }
        },
        physics = {
            type = "static",
            size = {40, 30},
            sensor = true,
            collisionLayer = "PlayerAttack"
        }
    }
}

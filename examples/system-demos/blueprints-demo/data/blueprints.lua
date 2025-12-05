-- blueprints.lua
-- Example blueprint definitions for Bestow Blueprint System Demo
-- Demonstrates all features: inheritance, physics, components, metadata

-- Define our blueprints in a global table
Blueprints = {

    --==========================================================================
    -- Simple Blueprints (No Inheritance)
    --==========================================================================

    -- A simple static platform
    ["static_platform"] = {
        components = {
            DebugRect = {
                width = 200,
                height = 20,
                fillColor = {100, 100, 100, 255},  -- Gray
                outlineColor = {50, 50, 50, 255},
                outlineWidth = 2,
                filled = true,
                layer = 0
            }
        },
        physics = {
            type = "static",
            fixedRotation = true,
            density = 0.0,
            friction = 0.5,
            restitution = 0.0,
            collisionLayer = "world"
        },
        metadata = {
            category = "environment",
            description = "Static platform for level geometry"
        }
    },

    -- A collectible item (sensor, no physics response)
    ["coin"] = {
        components = {
            DebugCircle = {
                radius = 12,
                fillColor = {255, 215, 0, 255},  -- Gold
                outlineColor = {200, 170, 0, 255},
                outlineWidth = 2,
                filled = true,
                segments = 32,
                layer = 1
            }
        },
        physics = {
            type = "static",
            sensor = true,  -- No collision response, just triggers
            size = {24, 24}
        },
        metadata = {
            category = "collectible",
            value = 10,
            sound = "coin_pickup.wav"
        }
    },

    -- A simple projectile
    ["bullet"] = {
        components = {
            DebugCircle = {
                radius = 4,
                fillColor = {255, 255, 0, 255},  -- Yellow
                filled = true,
                layer = 2
            }
        },
        physics = {
            type = "dynamic",
            fixedRotation = true,
            density = 0.1,
            friction = 0.0,
            restitution = 0.0,
            linearDamping = 0.0,
            sensor = true,
            collisionLayer = "projectile"
        },
        metadata = {
            damage = 25,
            lifetime = 3.0
        }
    },

    --==========================================================================
    -- Complex Blueprint with Multiple Components
    --==========================================================================

    ["player"] = {
        components = {
            -- Visual representation
            DebugRect = {
                width = 32,
                height = 48,
                fillColor = {0, 128, 255, 255},  -- Blue
                outlineColor = {0, 100, 200, 255},
                outlineWidth = 2,
                filled = true,
                layer = 1
            },

            -- Tag for game logic
            PlayerTag = {}
        },
        physics = {
            type = "dynamic",
            fixedRotation = true,
            density = 1.0,
            friction = 0.0,
            restitution = 0.0,
            linearDamping = 0.0,
            collisionLayer = "player"
        },
        metadata = {
            maxHealth = 100,
            moveSpeed = 200,
            jumpForce = 400,
            doubleJump = true
        }
    },

    --==========================================================================
    -- Blueprint Inheritance - Base Enemy
    --==========================================================================

    ["base_enemy"] = {
        components = {
            DebugRect = {
                width = 32,
                height = 32,
                fillColor = {255, 0, 0, 255},  -- Red
                outlineColor = {200, 0, 0, 255},
                outlineWidth = 1,
                filled = true,
                layer = 1
            },
            EnemyTag = {}
        },
        physics = {
            type = "dynamic",
            fixedRotation = true,
            density = 0.8,
            friction = 0.3,
            restitution = 0.0,
            linearDamping = 1.0,
            collisionLayer = "enemy"
        },
        metadata = {
            health = 50,
            damage = 10,
            points = 100,
            ai_type = "patrol"
        }
    },

    --==========================================================================
    -- Inherited Blueprints (Demonstrate Inheritance)
    --==========================================================================

    -- Fast enemy (inherits from base_enemy, overrides some properties)
    ["fast_enemy"] = {
        inherits = "base_enemy",
        components = {
            DebugRect = {
                -- Override only the color, keep other properties from base
                fillColor = {255, 128, 0, 255},  -- Orange
                outlineColor = {200, 100, 0, 255}
            }
        },
        metadata = {
            -- Override base metadata
            health = 30,
            moveSpeed = 150,
            points = 150
        }
    },

    -- Flying enemy (inherits from base_enemy, different shape and physics)
    ["flying_enemy"] = {
        inherits = "base_enemy",
        components = {
            -- Replace DebugRect with DebugCircle
            DebugCircle = {
                radius = 20,
                fillColor = {128, 0, 255, 255},  -- Purple
                outlineColor = {100, 0, 200, 255},
                outlineWidth = 1,
                filled = true,
                segments = 16,
                layer = 1
            }
        },
        physics = {
            type = "kinematic",  -- Override to kinematic (flies through platforms)
            linearDamping = 2.0
        },
        metadata = {
            health = 40,
            moveSpeed = 80,
            points = 200,
            ai_type = "flying_patrol",
            canFly = true
        }
    },

    -- Tank enemy (inherits from base_enemy, larger and slower)
    ["tank_enemy"] = {
        inherits = "base_enemy",
        components = {
            DebugRect = {
                width = 64,
                height = 64,
                fillColor = {100, 50, 50, 255},  -- Dark red
                outlineColor = {80, 40, 40, 255},
                outlineWidth = 3
            }
        },
        physics = {
            density = 2.0,
            linearDamping = 3.0
        },
        metadata = {
            health = 150,
            damage = 25,
            moveSpeed = 30,
            points = 500,
            armor = 10
        }
    },

    --==========================================================================
    -- Complex Physics Configuration
    --==========================================================================

    -- Bouncy ball with high restitution
    ["bouncy_ball"] = {
        components = {
            DebugCircle = {
                radius = 16,
                fillColor = {255, 100, 255, 255},  -- Pink
                filled = true,
                segments = 32,
                layer = 1
            }
        },
        physics = {
            type = "dynamic",
            fixedRotation = false,  -- Allow rotation
            density = 0.5,
            friction = 0.1,
            restitution = 0.95,  -- Very bouncy!
            linearDamping = 0.1,
            collisionLayer = "prop"
        },
        metadata = {
            sound = "bounce.wav"
        }
    },

    -- Crate with specific size
    ["crate"] = {
        components = {
            DebugRect = {
                fillColor = {139, 90, 43, 255},  -- Brown
                outlineColor = {100, 60, 30, 255},
                outlineWidth = 2,
                filled = true,
                layer = 1
            }
        },
        physics = {
            type = "dynamic",
            size = {40, 40},  -- Explicit size
            fixedRotation = false,
            density = 1.5,
            friction = 0.8,
            restitution = 0.1,
            linearDamping = 0.5,
            collisionLayer = "prop"
        },
        metadata = {
            breakable = true,
            health = 30
        }
    },

    --==========================================================================
    -- Minimal Blueprints for Testing Edge Cases
    --==========================================================================

    -- Minimal blueprint (just position, no components)
    ["marker"] = {
        components = {},
        metadata = {
            type = "spawn_point"
        }
    },

    -- Blueprint with only a line (for debugging)
    ["debug_line"] = {
        components = {
            DebugLine = {
                endOffset = {50, 0},
                color = {255, 255, 255, 255},  -- White
                thickness = 2,
                layer = 10
            }
        },
        metadata = {
            debug = true
        }
    }
}

-- Return the blueprints table (alternative to global)
return Blueprints

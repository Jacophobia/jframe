-- player.lua
-- Player-specific configuration demonstrating various data types and structures

-- This file demonstrates:
-- - Character stats and attributes
-- - Movement physics parameters
-- - Ability configurations
-- - Animation data
-- - Equipment and inventory settings

return {
    -- ========================================================================
    -- PLAYER IDENTITY
    -- ========================================================================

    name = "Hero",
    class = "Warrior",
    level = 1,

    -- ========================================================================
    -- STATS
    -- ========================================================================

    stats = {
        -- Base stats
        health = 100,
        maxHealth = 100,
        stamina = 50,
        maxStamina = 50,

        -- Attributes
        strength = 10,
        agility = 8,
        intelligence = 5,
        vitality = 12,

        -- Derived stats (computed from attributes)
        damage = 10 * 1.5,  -- strength * multiplier
        defense = 12 * 0.8, -- vitality * multiplier
        speed = 8 * 10.0,   -- agility * pixels_per_second
    },

    -- ========================================================================
    -- MOVEMENT PHYSICS
    -- ========================================================================

    movement = {
        -- Horizontal movement
        moveSpeed = 200.0,           -- pixels per second
        acceleration = 1200.0,       -- pixels per second squared
        deceleration = 1500.0,       -- friction/stopping force

        -- Jumping
        jumpVelocity = -500.0,       -- negative = upward
        jumpCutMultiplier = 0.5,     -- how much to reduce jump when releasing button
        maxJumps = 2,                -- allow double jump
        coyoteTime = 0.1,            -- seconds of forgiveness after leaving ground
        jumpBufferTime = 0.15,       -- seconds to buffer jump input before landing

        -- Air control
        airControlFactor = 0.7,      -- 70% control while airborne

        -- Wall mechanics
        canWallSlide = true,
        wallSlideSpeed = 50.0,
        wallJumpVelocityX = 400.0,
        wallJumpVelocityY = -450.0,
    },

    -- ========================================================================
    -- COMBAT
    -- ========================================================================

    combat = {
        -- Attack properties
        attackDamage = 15,
        attackRange = 32.0,          -- pixels
        attackRate = 0.5,            -- seconds between attacks
        critChance = 0.1,            -- 10% critical hit chance
        critMultiplier = 2.0,        -- 2x damage on crit

        -- Defensive properties
        armor = 5,
        dodgeChance = 0.05,          -- 5% chance to dodge
        blockReduction = 0.5,        -- 50% damage reduction when blocking

        -- Special abilities
        specialCooldown = 5.0,       -- seconds
        specialManaCost = 20,
        specialDamage = 50,
    },

    -- ========================================================================
    -- ANIMATIONS
    -- ========================================================================

    animations = {
        -- Animation frame counts
        idleFrames = 8,
        runFrames = 10,
        jumpFrames = 6,
        fallFrames = 4,
        attackFrames = 8,
        hurtFrames = 3,
        deathFrames = 12,

        -- Frame rates (FPS)
        idleFPS = 8.0,
        runFPS = 12.0,
        jumpFPS = 10.0,
        fallFPS = 8.0,
        attackFPS = 15.0,
        hurtFPS = 10.0,
        deathFPS = 8.0,
    },

    -- ========================================================================
    -- ABILITIES (Array of ability IDs)
    -- ========================================================================

    -- Player starts with these abilities unlocked
    unlockedAbilities = {1, 2, 5},   -- IDs reference ability system

    -- Ability hotkeys mapped to slot numbers
    abilitySlots = {
        1,  -- Slot 1: Basic Attack (ability ID 1)
        2,  -- Slot 2: Power Strike (ability ID 2)
        5,  -- Slot 3: Dash (ability ID 5)
        0,  -- Slot 4: Empty
    },

    -- ========================================================================
    -- INVENTORY
    -- ========================================================================

    inventory = {
        maxSlots = 20,
        maxWeight = 100.0,
        currentWeight = 0.0,

        -- Starting items (item IDs)
        startingItems = {101, 102, 105},  -- Sword, Shield, Health Potion

        -- Quick slots
        quickSlotCount = 4,
    },

    -- ========================================================================
    -- VISUAL SETTINGS
    -- ========================================================================

    visual = {
        -- Sprite settings
        spriteWidth = 32,
        spriteHeight = 48,
        scale = 2.0,                 -- 2x upscale for pixel art

        -- Colors (RGBA 0-255)
        tintR = 255,
        tintG = 255,
        tintB = 255,
        tintA = 255,

        -- Effects
        showShadow = true,
        shadowOpacity = 0.3,
        showHealthBar = true,
        healthBarOffset = -10.0,     -- pixels above sprite
    },

    -- ========================================================================
    -- PROGRESSION
    -- ========================================================================

    progression = {
        experiencePoints = 0,
        experienceToLevel = 100,
        skillPoints = 0,

        -- Level up bonuses
        healthPerLevel = 10,
        staminaPerLevel = 5,
        statPointsPerLevel = 3,
    },

    -- ========================================================================
    -- ARRAYS OF DIFFERENT TYPES
    -- ========================================================================

    -- String array for equipped items
    equippedSlots = {
        "Iron Sword",
        "Wooden Shield",
        "Leather Armor",
        "",  -- Empty slot
        "Red Cape",
    },

    -- Float array for stat modifiers from equipment
    statModifiers = {
        1.0,   -- Health multiplier
        1.1,   -- Stamina multiplier
        1.2,   -- Damage multiplier
        0.9,   -- Speed multiplier (armor slows you down)
    },

    -- Integer array for unlocked achievements
    achievements = {1, 3, 5, 7, 10, 15},
}

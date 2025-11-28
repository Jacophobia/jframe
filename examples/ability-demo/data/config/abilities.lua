-- abilities.lua
-- Gameplay Ability System demonstration definitions
-- Comprehensive ability setup with platformer mechanics, combat, and combos

-- Tags used throughout the system
Tags = {
    -- State Tags
    "State.InJumpZone",
    "State.InAir",
    "State.TouchingWall",
    "State.Shielded",
    "State.Stunned",
    "State.Attacking",
    "State.Invincible",
    "State.Dashing",

    -- Ability Unlock Tags
    "Ability.HasDoubleJump",
    "Ability.HasWallJump",
    "Ability.HasGroundPound",
    "Ability.HasSword",
    "Ability.HasShield",
    "Ability.HasRanged",

    -- Combo System Tags
    "Combo.Window1",
    "Combo.Window2",
    "Combo.HasCombo1",
    "Combo.HasCombo2",

    -- Jump Exhaustion System
    "JumpExhaustion.Tracking",
}

-- Attributes tracked by entities
Attributes = {
    {
        name = "Health",
        baseValue = 100,
        minValue = 0,
        maxValue = 100,
        clampEnabled = true,
    },
    {
        name = "Stamina",
        baseValue = 100,
        minValue = 0,
        maxValue = 100,
        clampEnabled = true,
    },
    {
        name = "MoveSpeed",
        baseValue = 200,
        minValue = 0,
        maxValue = 1000,
        clampEnabled = true,
    },
    {
        name = "JumpCount",
        baseValue = 0,
        minValue = 0,
        maxValue = 10,
        clampEnabled = true,
    },
    {
        name = "AttackPower",
        baseValue = 10,
        minValue = 0,
        maxValue = 100,
        clampEnabled = true,
    },
    {
        name = "Defense",
        baseValue = 5,
        minValue = 0,
        maxValue = 50,
        clampEnabled = true,
    },
}

-- Effects that modify attributes or grant tags
Effects = {
    -- Movement Effects
    {
        name = "DashSpeedBoost",
        durationType = "duration",
        duration = 0.2,
        modifiers = {
            { attribute = "MoveSpeed", op = "add", value = 100 },
        },
        grantedTags = { "State.Dashing" },
    },

    -- Stun effect that blocks abilities (reduced from 3 rapid jumps)
    {
        name = "Stun",
        durationType = "duration",
        duration = 0.5,
        grantedTags = { "State.Stunned" },
    },

    -- Jump exhaustion effect (applied after too many jumps)
    {
        name = "JumpExhaustion",
        durationType = "duration",
        duration = 1.0,
        grantedTags = { "State.Stunned", "JumpExhaustion.Tracking" },
    },

    -- Defense Effects
    {
        name = "Shield",
        durationType = "duration",
        duration = 2.0,
        modifiers = {
            { attribute = "Defense", op = "add", value = 10 },
        },
        grantedTags = { "State.Shielded" },
    },

    {
        name = "Invincibility",
        durationType = "duration",
        duration = 0.5,
        grantedTags = { "State.Invincible" },
    },

    -- Damage Effects
    {
        name = "GroundPoundDamage",
        durationType = "instant",
        modifiers = {
            { attribute = "Health", op = "add", value = -30 },
        },
    },

    -- Regeneration Effects
    {
        name = "HealthRegen",
        durationType = "duration",
        duration = 5.0,
        period = 1.0,
        modifiers = {
            { attribute = "Health", op = "add", value = 5 },
        },
    },

    {
        name = "StaminaRegen",
        durationType = "duration",
        duration = 1.0,
        period = 0.1,
        modifiers = {
            { attribute = "Stamina", op = "add", value = 2 },
        },
    },

    -- Combo Window Effects (grant tags for limited time)
    {
        name = "ComboWindow1",
        durationType = "duration",
        duration = 0.5,
        grantedTags = { "Combo.Window1", "Combo.HasCombo1" },
    },

    {
        name = "ComboWindow2",
        durationType = "duration",
        duration = 0.5,
        grantedTags = { "Combo.Window2", "Combo.HasCombo2" },
    },

    -- Attack state marker
    {
        name = "AttackState",
        durationType = "duration",
        duration = 0.3,
        grantedTags = { "State.Attacking" },
    },
}

-- Abilities that entities can activate
Abilities = {
    -- ========================================
    -- MOVEMENT ABILITIES
    -- ========================================

    -- Basic Jump (works anywhere when grounded)
    {
        name = "Jump",
        activationPolicy = "whileInputHeld",
        cooldown = 0.0,
        costs = {
            { attribute = "Stamina", cost = 5 },
        },
        -- No required tags - can jump anywhere
        activationBlockedTags = { "State.Stunned", "State.InAir" },
        abilityTags = { "Ability.Movement.Jump" },
        modifiers = {
            { attribute = "JumpCount", op = "add", value = 1 },
        },
    },

    -- Double Jump (air jump)
    {
        name = "DoubleJump",
        activationPolicy = "onInputPressed",
        cooldown = 0.0,
        costs = {
            { attribute = "Stamina", cost = 15 },
        },
        activationRequiredTags = { "Ability.HasDoubleJump", "State.InAir" },
        activationBlockedTags = { "State.Stunned" },
        abilityTags = { "Ability.Movement.DoubleJump" },
    },

    -- Wall Jump
    {
        name = "WallJump",
        activationPolicy = "onInputPressed",
        cooldown = 0.2,
        costs = {
            { attribute = "Stamina", cost = 10 },
        },
        activationRequiredTags = { "Ability.HasWallJump", "State.TouchingWall" },
        activationBlockedTags = { "State.Stunned" },
        abilityTags = { "Ability.Movement.WallJump" },
    },

    -- Dash
    {
        name = "Dash",
        activationPolicy = "onInputPressed",
        cooldown = 1.0,
        costs = {
            { attribute = "Stamina", cost = 15 },
        },
        activationBlockedTags = { "State.Stunned" },
        abilityTags = { "Ability.Movement.Dash" },
        effectsToApplyOnActivate = { "DashSpeedBoost" },
    },

    -- Ground Pound
    {
        name = "GroundPound",
        activationPolicy = "onInputPressed",
        cooldown = 1.5,
        costs = {
            { attribute = "Stamina", cost = 20 },
        },
        activationRequiredTags = { "Ability.HasGroundPound", "State.InAir" },
        activationBlockedTags = { "State.Stunned" },
        abilityTags = { "Ability.Movement.GroundPound" },
    },

    -- ========================================
    -- COMBAT ABILITIES
    -- ========================================

    -- Sword Attack (first hit in combo)
    {
        name = "SwordAttack",
        activationPolicy = "onInputPressed",
        cooldown = 0.4,
        costs = {
            { attribute = "Stamina", cost = 5 },
        },
        activationRequiredTags = { "Ability.HasSword" },
        activationBlockedTags = { "State.Stunned", "State.Attacking" },
        abilityTags = { "Ability.Combat.Sword" },
        effectsToApplyOnActivate = { "AttackState", "ComboWindow1" },
    },

    -- Sword Combo 2 (second hit)
    {
        name = "SwordCombo2",
        activationPolicy = "onInputPressed",
        cooldown = 0.3,
        costs = {
            { attribute = "Stamina", cost = 8 },
        },
        activationRequiredTags = { "Ability.HasSword", "Combo.HasCombo1" },
        activationBlockedTags = { "State.Stunned", "State.Attacking" },
        abilityTags = { "Ability.Combat.SwordCombo2" },
        effectsToApplyOnActivate = { "AttackState", "ComboWindow2" },
        modifiers = {
            { attribute = "AttackPower", op = "multiply", value = 1.5 },
        },
    },

    -- Sword Combo 3 (finisher)
    {
        name = "SwordCombo3",
        activationPolicy = "onInputPressed",
        cooldown = 0.5,
        costs = {
            { attribute = "Stamina", cost = 12 },
        },
        activationRequiredTags = { "Ability.HasSword", "Combo.HasCombo2" },
        activationBlockedTags = { "State.Stunned", "State.Attacking" },
        abilityTags = { "Ability.Combat.SwordCombo3" },
        effectsToApplyOnActivate = { "AttackState" },
        modifiers = {
            { attribute = "AttackPower", op = "multiply", value = 2.0 },
        },
    },

    -- Shield
    {
        name = "Shield",
        activationPolicy = "onInputPressed",
        cooldown = 3.0,
        costs = {
            { attribute = "Stamina", cost = 20 },
        },
        activationRequiredTags = { "Ability.HasShield" },
        activationBlockedTags = { "State.Stunned", "State.Shielded" },
        abilityTags = { "Ability.Defense.Shield" },
        effectsToApplyOnActivate = { "Shield" },
    },

    -- Ranged Attack
    {
        name = "Ranged",
        activationPolicy = "onInputPressed",
        cooldown = 0.8,
        costs = {
            { attribute = "Stamina", cost = 25 },
        },
        activationRequiredTags = { "Ability.HasRanged" },
        activationBlockedTags = { "State.Stunned", "State.Attacking" },
        abilityTags = { "Ability.Combat.Ranged" },
        effectsToApplyOnActivate = { "AttackState" },
    },

    -- ========================================
    -- UTILITY ABILITIES
    -- ========================================

    -- Health Regen Ability (pickup-based)
    {
        name = "HealingPotion",
        activationPolicy = "onInputPressed",
        cooldown = 10.0,
        costs = {
            { attribute = "Stamina", cost = 30 },
        },
        activationBlockedTags = { "State.Stunned" },
        abilityTags = { "Ability.Utility.Heal" },
        effectsToApplyOnActivate = { "HealthRegen" },
    },
}

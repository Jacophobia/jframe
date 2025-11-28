-- Audio configuration for Ability Demo
-- All sound effect definitions and channel assignments

return {
    -- Sound channels (reserved indices for different sound types)
    channels = {
        music = 0,        -- Background music
        sfx = 1,          -- General sound effects (one-shot)
        player = 2,       -- Player sounds (jump, dash, etc.)
        combat = 3,       -- Combat sounds (sword, hit, etc.)
        ui = 4,           -- UI sounds
        ambient = 5,      -- Ambient/environmental sounds
    },

    -- Sound file paths (relative to data/audio/)
    sounds = {
        -- Player actions
        jump = {
            path = "data/audio/jump.wav",
            volume = 0.8,
            pitch = 1.0,
        },
        doubleJump = {
            path = "data/audio/double_jump.wav",
            volume = 0.7,
            pitch = 1.2,
        },
        dash = {
            path = "data/audio/dash.wav",
            volume = 0.9,
            pitch = 1.0,
        },
        wallJump = {
            path = "data/audio/wall_jump.wav",
            volume = 0.8,
            pitch = 1.1,
        },
        groundPound = {
            path = "data/audio/ground_pound.wav",
            volume = 1.0,
            pitch = 0.8,
        },
        land = {
            path = "data/audio/land.wav",
            volume = 0.5,
            pitch = 1.0,
        },

        -- Combat
        swordSwing1 = {
            path = "data/audio/sword_swing1.wav",
            volume = 0.8,
            pitch = 1.0,
        },
        swordSwing2 = {
            path = "data/audio/sword_swing2.wav",
            volume = 0.8,
            pitch = 0.95,
        },
        swordSwing3 = {
            path = "data/audio/sword_swing3.wav",
            volume = 0.9,
            pitch = 1.1,
        },
        enemyHit = {
            path = "data/audio/enemy_hit.wav",
            volume = 0.7,
            pitch = 1.0,
        },
        enemyDeath = {
            path = "data/audio/enemy_death.wav",
            volume = 0.8,
            pitch = 1.0,
        },
        playerHurt = {
            path = "data/audio/player_hurt.wav",
            volume = 0.9,
            pitch = 1.0,
        },
        shieldBlock = {
            path = "data/audio/shield_block.wav",
            volume = 0.8,
            pitch = 1.0,
        },
        projectileFire = {
            path = "data/audio/projectile_fire.wav",
            volume = 0.6,
            pitch = 1.0,
        },

        -- Pickups & Progression
        collectAbility = {
            path = "data/audio/collect_ability.wav",
            volume = 1.0,
            pitch = 1.0,
        },
        collectHealth = {
            path = "data/audio/collect_health.wav",
            volume = 0.7,
            pitch = 1.0,
        },
        checkpoint = {
            path = "data/audio/checkpoint.wav",
            volume = 0.8,
            pitch = 1.0,
        },

        -- Level mechanics
        switchActivate = {
            path = "data/audio/switch_activate.wav",
            volume = 0.7,
            pitch = 1.0,
        },
        doorOpen = {
            path = "data/audio/door_open.wav",
            volume = 0.8,
            pitch = 1.0,
        },
        breakableDestroy = {
            path = "data/audio/breakable_destroy.wav",
            volume = 0.9,
            pitch = 1.0,
        },

        -- Boss
        bossRoar = {
            path = "data/audio/boss_roar.wav",
            volume = 1.0,
            pitch = 0.8,
        },
        bossAttack = {
            path = "data/audio/boss_attack.wav",
            volume = 0.9,
            pitch = 0.9,
        },
        bossHurt = {
            path = "data/audio/boss_hurt.wav",
            volume = 0.8,
            pitch = 0.9,
        },
        bossDefeat = {
            path = "data/audio/boss_defeat.wav",
            volume = 1.0,
            pitch = 1.0,
        },

        -- Music
        musicExploration = {
            path = "data/audio/music_exploration.ogg",
            volume = 0.5,
            pitch = 1.0,
            looping = true,
        },
        musicCombat = {
            path = "data/audio/music_combat.ogg",
            volume = 0.6,
            pitch = 1.0,
            looping = true,
        },
        musicBoss = {
            path = "data/audio/music_boss.ogg",
            volume = 0.7,
            pitch = 1.0,
            looping = true,
        },
    },

    -- Volume groups for settings
    groups = {
        master = 1.0,
        music = 0.7,
        sfx = 1.0,
        ambient = 0.5,
    },
}

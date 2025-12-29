-- config/game.lua
-- Game configuration values
-- These can be hot-reloaded during development

return {
    -- Player settings
    player = {
        speed = 200,
        jumpForce = 400,
        maxHealth = 100,
        invincibilityDuration = 1.0
    },

    -- Enemy settings
    enemy = {
        speed = 100,
        damage = 10,
        detectionRange = 300
    },

    -- Physics settings
    physics = {
        gravity = -980,
        friction = 0.3,
        restitution = 0.0
    },

    -- Audio settings
    audio = {
        masterVolume = 1.0,
        musicVolume = 0.7,
        sfxVolume = 1.0
    },

    -- Debug settings
    debug = {
        showColliders = false,
        showFPS = true,
        godMode = false
    }
}

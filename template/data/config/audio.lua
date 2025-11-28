-- template/data/config/audio.lua
-- Audio Configuration
-- Volume and pitch settings for all sounds

return {
    -- Master volume settings
    master = {
        musicVolume = 0.5,
        sfxVolume = 0.8
    },

    -- Individual sound settings
    sounds = {
        background = {
            volume = 0.5,
            pitch = 1.0,
            fadeIn = 1.0        -- Fade in duration (seconds)
        },

        jump = {
            volume = 0.8,
            pitch = 1.0
        }
    }
}

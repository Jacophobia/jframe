-- games/snake3d/modules/audio.lua
-- Audio system for sound effects and music
--
-- Matches C++ audio from snake.game.cppm lines 715-843

local Log = bestow.include("modules/log")

local Audio = {}

local log = Log.category("Audio")

-- Sound paths (using asset library)
Audio.sounds = {
    eat = ":library:/sounds/25_item.wav",
    death = ":library:/sounds/63_lose1.wav",
    level_complete = ":library:/sounds/24_levelclear.wav",
    menu_select = ":library:/sounds/59_confirm.wav",
    menu_move = ":library:/sounds/05_cursor1.wav",
    enemy_hit = ":library:/sounds/15_hit.wav",
    chain_break = ":library:/sounds/69_explode.wav",
    pause = ":library:/sounds/07_pause1.wav",
    game_over = ":library:/sounds/64_lose2.wav",
}

-- Music paths
Audio.music = {
    game = ":library:/music/game.ogg",
    menu = ":library:/music/menu.ogg",
}

-- Volume settings
Audio.volumes = {
    master = 1.0,
    sfx = 0.8,
    music = 0.6,
}

-- Current state
Audio.currentMusic = nil
Audio.initialized = false

-- Initialize audio system
function Audio.init()
    if Audio.initialized then return true end

    log.info("Initializing audio system...")

    -- Check if audio is available
    if not bestow.audio then
        log.warn("Audio system not available")
        return false
    end

    -- Set initial volumes
    if bestow.audio.setMasterVolume then
        bestow.audio.setMasterVolume(Audio.volumes.master)
    end
    if bestow.audio.setGroupVolume then
        bestow.audio.setGroupVolume("sfx", Audio.volumes.sfx)
        bestow.audio.setGroupVolume("music", Audio.volumes.music)
    end

    Audio.initialized = true
    log.info("Audio system initialized")
    return true
end

-- Play a sound effect
function Audio.playSFX(name, options)
    if not bestow.audio then return end

    local path = Audio.sounds[name]
    if not path then
        log.warn("Unknown sound effect: %s", tostring(name))
        return
    end

    log.trace("Playing SFX: %s", name)

    local opts = options or {}
    bestow.audio.playSound(path, {
        volume = opts.volume or Audio.volumes.sfx,
        pitch = opts.pitch or 1.0,
        looping = opts.looping or false
    })
end

-- Play music track
function Audio.playMusic(name, fadeIn)
    if not bestow.audio then return end

    local path = Audio.music[name]
    if not path then
        log.warn("Unknown music track: %s", tostring(name))
        return
    end

    -- Don't restart if already playing same track
    if Audio.currentMusic == name then
        log.trace("Music '%s' already playing", name)
        return
    end

    log.debug("Playing music: %s", name)
    Audio.currentMusic = name

    bestow.audio.playMusic(path, {
        volume = Audio.volumes.music,
        fadeIn = fadeIn or 0.5
    })
end

-- Stop current music
function Audio.stopMusic(fadeOut)
    if not bestow.audio then return end

    if Audio.currentMusic then
        log.debug("Stopping music: %s with fade %.2f", Audio.currentMusic, fadeOut or 0.5)
        bestow.audio.stopMusic(fadeOut or 0.5)
        Audio.currentMusic = nil
    end
end

-- Set master volume
function Audio.setMasterVolume(volume)
    Audio.volumes.master = volume
    if bestow.audio and bestow.audio.setMasterVolume then
        bestow.audio.setMasterVolume(volume)
    end
    log.debug("Master volume set to %.2f", volume)
end

-- Set SFX volume
function Audio.setSFXVolume(volume)
    Audio.volumes.sfx = volume
    if bestow.audio and bestow.audio.setGroupVolume then
        bestow.audio.setGroupVolume("sfx", volume)
    end
    log.debug("SFX volume set to %.2f", volume)
end

-- Set music volume
function Audio.setMusicVolume(volume)
    Audio.volumes.music = volume
    if bestow.audio and bestow.audio.setGroupVolume then
        bestow.audio.setGroupVolume("music", volume)
    end
    log.debug("Music volume set to %.2f", volume)
end

-- Check if audio is ready
function Audio.isReady()
    return Audio.initialized
end

return Audio

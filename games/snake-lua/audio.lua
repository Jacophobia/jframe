-- audio.lua - Sound and music
-- Matches C++ audio code exactly

local state = require("state")

local audio = {}

-- Load sound configuration from Lua file
function audio.loadSoundConfig()
    local configPath = "data/config/sounds.lua"

    -- Use bestow config system to parse Lua
    local result = bestow.config.parseLuaFile(configPath)
    if not result then
        print("No sounds.lua config found, audio disabled")
        return
    end

    -- Load volume settings
    if result.volumes then
        if result.volumes.master then
            bestow.audio.setMasterVolume(result.volumes.master)
        end
        if result.volumes.sfx then
            bestow.audio.setGroupVolume("SFX", result.volumes.sfx)
        end
        if result.volumes.music then
            bestow.audio.setGroupVolume("Music", result.volumes.music)
        end
    end

    -- Load sound effects
    if result.sfx then
        state.soundEat = audio.tryLoadSound(result.sfx, "eat")
        state.soundDeath = audio.tryLoadSound(result.sfx, "death")
        state.soundLevelComplete = audio.tryLoadSound(result.sfx, "level_complete")
        state.soundMenuSelect = audio.tryLoadSound(result.sfx, "menu_select")
        state.soundMenuMove = audio.tryLoadSound(result.sfx, "menu_move")
        state.soundEnemyHit = audio.tryLoadSound(result.sfx, "enemy_hit")
        state.soundChainBreak = audio.tryLoadSound(result.sfx, "chain_break")
        state.soundPause = audio.tryLoadSound(result.sfx, "pause")
        state.soundGameOver = audio.tryLoadSound(result.sfx, "game_over")
    end

    -- Load music
    if result.music then
        state.musicGame = audio.tryLoadMusic(result.music, "game")
        state.musicMenu = audio.tryLoadMusic(result.music, "menu")
    end

    state.soundsLoaded = true
    print("Sound config loaded successfully")
end

-- Try to load a sound effect
function audio.tryLoadSound(table, key)
    if not table[key] then return nil end

    local path = table[key]
    print("Loading sound: " .. key .. " -> " .. path)

    -- Register and load the sound asset
    local handle = bestow.assets.registerAsset(AssetType.Sound, path)
    if handle and handle:isValid() then
        bestow.assets.loadAsset(handle)
        return handle
    end
    return nil
end

-- Try to load a music track
function audio.tryLoadMusic(table, key)
    if not table[key] then return nil end

    local path = table[key]
    print("Loading music: " .. key .. " -> " .. path)

    -- Register and load the music asset
    local handle = bestow.assets.registerAsset(AssetType.Sound, path)
    if handle and handle:isValid() then
        bestow.assets.loadAsset(handle)
        return handle
    end
    return nil
end

-- Play a sound effect
function audio.playSound(sound, volume)
    volume = volume or 1.0
    if not state.soundsLoaded or not sound or not sound:isValid() then return end

    bestow.audio.playOnChannel(Channels.UI, {
        asset = sound,
        volume = volume,
        pitch = 1.0,
        looping = false
    })
end

-- Play a positional sound
function audio.playSoundPositional(sound, pos, volume)
    volume = volume or 1.0
    if not state.soundsLoaded or not sound or not sound:isValid() then return end

    bestow.audio.playPositional({
        asset = sound,
        position = pos,
        volume = volume,
        minDistance = 5.0,
        maxDistance = 50.0
    })
end

-- Play music track
function audio.playMusicTrack(music, loop, fadeIn)
    loop = (loop == nil) and true or loop
    fadeIn = fadeIn or 1.0
    if not state.soundsLoaded or not music or not music:isValid() then return end

    bestow.audio.playOnChannel(Channels.Music, {
        asset = music,
        volume = 1.0,
        looping = loop,
        fadeInTime = fadeIn
    })
end

-- Stop music
function audio.stopMusic(fadeOut)
    fadeOut = fadeOut or 1.0
    bestow.audio.stopChannel(Channels.Music, fadeOut)
end

-- Convenience functions for specific sounds
function audio.playEat()
    audio.playSound(state.soundEat)
end

function audio.playDeath()
    audio.playSound(state.soundDeath)
end

function audio.playLevelComplete()
    audio.playSound(state.soundLevelComplete)
end

function audio.playMenuSelect()
    audio.playSound(state.soundMenuSelect)
end

function audio.playMenuMove()
    audio.playSound(state.soundMenuMove)
end

function audio.playEnemyHit()
    audio.playSound(state.soundEnemyHit)
end

function audio.playChainBreak()
    audio.playSound(state.soundChainBreak)
end

function audio.playPause()
    audio.playSound(state.soundPause)
end

function audio.playGameOver()
    audio.playSound(state.soundGameOver)
end

function audio.playGameMusic()
    audio.playMusicTrack(state.musicGame)
end

function audio.playMenuMusic()
    audio.playMusicTrack(state.musicMenu)
end

return audio

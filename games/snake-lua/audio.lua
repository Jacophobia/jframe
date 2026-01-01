-- audio.lua - Sound and music
-- Dependencies: app.state (accessed inside functions)

local audio = {}

-- Load sound configuration from Lua file
function audio.loadSoundConfig()
    local state = app.state

    -- Try to find sounds config in app.data namespace
    -- ScriptManager loads data/config/sounds.lua as app.data.config.sounds
    local result = app.data and app.data.config and app.data.config.sounds

    -- Fallback to config system (deprecated path)
    if not result and bestow.config then
        result = bestow.config.parseLuaFile("data/config/sounds.lua")
    end

    if not result then
        bestow.warn("No sounds.lua config found, audio disabled")
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
    bestow.info("Sound config loaded successfully")
end

-- Try to load a sound effect
function audio.tryLoadSound(tbl, key)
    if not tbl[key] then return nil end

    local path = tbl[key]
    bestow.debug("Loading sound:", key, "->", path)

    -- Register and load the sound asset
    local handle = bestow.assets.registerAsset(bestow.assets.Type.Sound, path)
    if handle and handle:isValid() then
        bestow.assets.loadAsset(handle)
        return handle
    end
    return nil
end

-- Try to load a music track
function audio.tryLoadMusic(tbl, key)
    if not tbl[key] then return nil end

    local path = tbl[key]
    bestow.debug("Loading music:", key, "->", path)

    -- Register and load the music asset
    local handle = bestow.assets.registerAsset(bestow.assets.Type.Sound, path)
    if handle and handle:isValid() then
        bestow.assets.loadAsset(handle)
        return handle
    end
    return nil
end

-- Play a sound effect
function audio.playSound(sound, volume)
    local state = app.state
    volume = volume or 1.0
    if not state.soundsLoaded or not sound or not sound:isValid() then return end

    bestow.audio.playOnChannel(bestow.audio.Channel.UI, {
        asset = sound,
        volume = volume,
        pitch = 1.0,
        looping = false
    })
end

-- Play a positional sound
function audio.playSoundPositional(sound, pos, volume)
    local state = app.state
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
    local state = app.state
    loop = (loop == nil) and true or loop
    fadeIn = fadeIn or 1.0
    if not state.soundsLoaded or not music or not music:isValid() then
        bestow.debug("playMusicTrack: early return - soundsLoaded:", state.soundsLoaded,
                     "music valid:", music and music:isValid())
        return
    end

    bestow.info("Playing music track with looping:", loop)
    bestow.audio.playOnChannel(bestow.audio.Channel.Music, {
        asset = music,
        volume = 1.0,
        looping = loop,
        fadeInTime = fadeIn
    })
end

-- Stop music
function audio.stopMusic(fadeOut)
    fadeOut = fadeOut or 1.0
    bestow.audio.stopChannel(bestow.audio.Channel.Music, fadeOut)
end

-- Convenience functions for specific sounds
function audio.playEat()
    local state = app.state
    audio.playSound(state.soundEat)
end

function audio.playDeath()
    local state = app.state
    audio.playSound(state.soundDeath)
end

function audio.playLevelComplete()
    local state = app.state
    audio.playSound(state.soundLevelComplete)
end

function audio.playMenuSelect()
    local state = app.state
    audio.playSound(state.soundMenuSelect)
end

function audio.playMenuMove()
    local state = app.state
    audio.playSound(state.soundMenuMove)
end

function audio.playEnemyHit()
    local state = app.state
    audio.playSound(state.soundEnemyHit)
end

function audio.playChainBreak()
    local state = app.state
    audio.playSound(state.soundChainBreak)
end

function audio.playPause()
    local state = app.state
    audio.playSound(state.soundPause)
end

function audio.playGameOver()
    local state = app.state
    audio.playSound(state.soundGameOver)
end

function audio.playGameMusic()
    local state = app.state
    audio.playMusicTrack(state.musicGame)
end

function audio.playMenuMusic()
    local state = app.state
    audio.playMusicTrack(state.musicMenu)
end

return audio

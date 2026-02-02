---
name: audio-system
description: Play music, sound effects, and positional audio in Bestow. Use when implementing background music, UI sounds, 3D spatial audio, volume control, or audio mixing.
---

# Audio System

The audio system handles all game audio including music, sound effects, and 3D positional audio.

## Complete API Reference

### Channel-Based Audio

Channels are for dedicated audio streams like music, ambience, and UI:

```lua
-- Play sound on a channel
bestow.audio.playOnChannel(channel: Channel, sound: ChannelSound | table)

-- ChannelSound table structure:
{
    asset = AssetHandle,     -- Required: registered asset handle
    volume = 1.0,            -- Optional: 0.0 to 1.0
    pitch = 1.0,             -- Optional: playback speed multiplier
    looping = false,         -- Optional: loop the sound
    fadeInTime = 0.0         -- Optional: fade in duration in seconds
}

-- Stop channel (with optional fade out)
bestow.audio.stopChannel(channel: Channel)
bestow.audio.stopChannel(channel: Channel, fadeOutTime: float)

-- Pause/resume
bestow.audio.pauseChannel(channel: Channel)
bestow.audio.resumeChannel(channel: Channel)

-- Adjust properties
bestow.audio.setChannelVolume(channel: Channel, volume: float)
bestow.audio.setChannelPitch(channel: Channel, pitch: float)

-- Seek to position (in seconds)
bestow.audio.seekChannel(channel: Channel, position: float)

-- Query state
bestow.audio.isChannelPlaying(channel: Channel) -> bool
bestow.audio.getChannelState(channel: Channel) -> ChannelState
-- ChannelState = { isPlaying: bool, isPaused: bool, position: float, length: float, volume: float }

-- Predefined channels
bestow.audio.Channel.Music     -- Background music
bestow.audio.Channel.Ambience  -- Environmental sounds
bestow.audio.Channel.UI        -- Interface sounds
```

### Positional (3D) Audio

For sounds that exist in 3D space:

```lua
-- Play positional sound
bestow.audio.playPositional(sound: PositionalSound | table) -> SoundHandle

-- PositionalSound table structure:
{
    asset = AssetHandle,      -- Required: registered asset handle
    position = Vec3,          -- Required: world position
    volume = 1.0,             -- Optional: base volume
    pitch = 1.0,              -- Optional: playback speed
    minDistance = 1.0,        -- Optional: full volume distance
    maxDistance = 100.0       -- Optional: silence distance
}

-- Stop positional sound
bestow.audio.stopPositional(handle: SoundHandle)

-- Update position (for moving sources)
bestow.audio.updatePositionalPosition(handle: SoundHandle, position: Vec3)

-- Check if still playing
bestow.audio.isPositionalPlaying(handle: SoundHandle) -> bool
```

### 3D Audio Listener

The listener is typically attached to the camera or player:

```lua
-- Set listener position and orientation
bestow.audio.setListener(listener: AudioListener)

-- AudioListener structure:
{
    position = Vec3,       -- World position
    forward = Vec3,        -- Forward direction (normalized)
    up = Vec3,             -- Up direction (normalized)
    velocity = Vec3        -- For doppler effect (optional)
}

-- Get current listener
bestow.audio.getListener() -> AudioListener
```

### Global Controls

```lua
-- Master volume (affects all audio)
bestow.audio.setMasterVolume(volume: float)
bestow.audio.getMasterVolume() -> float

-- Global pause/resume (for pause menu)
bestow.audio.pauseAll()
bestow.audio.resumeAll()

-- Stop everything
bestow.audio.stopAll()
```

### Channel Groups

For controlling categories of audio together:

```lua
-- Set volume for a group (e.g., "sfx", "music", "voice")
bestow.audio.setGroupVolume(group: string, volume: float)

-- Assign a channel to a group
bestow.audio.assignChannelToGroup(channel: Channel, group: string)
```

## Common Patterns

### Background Music

```lua
-- In init() or level start
local function playBackgroundMusic()
    local musicHandle = bestow.assets.registerAsset(
        AssetType.Music,
        "music/level1_theme.ogg"
    )
    bestow.assets.loadAsset(musicHandle)

    bestow.audio.playOnChannel(bestow.audio.Channel.Music, {
        asset = musicHandle,
        volume = 0.7,
        looping = true,
        fadeInTime = 2.0
    })
end

-- To change music
local function crossfadeToMusic(newMusicPath, fadeDuration)
    -- Fade out current
    bestow.audio.stopChannel(bestow.audio.Channel.Music, fadeDuration)

    -- Load and play new
    local handle = bestow.assets.registerAsset(AssetType.Music, newMusicPath)
    bestow.assets.loadAsset(handle)

    bestow.audio.playOnChannel(bestow.audio.Channel.Music, {
        asset = handle,
        volume = 0.7,
        looping = true,
        fadeInTime = fadeDuration
    })
end
```

### UI Sound Effects

```lua
-- Preload UI sounds in init()
local uiSounds = {}

local function loadUISounds()
    local sounds = {
        click = "sounds/ui/click.wav",
        hover = "sounds/ui/hover.wav",
        confirm = "sounds/ui/confirm.wav",
        cancel = "sounds/ui/cancel.wav"
    }

    for name, path in pairs(sounds) do
        uiSounds[name] = bestow.assets.registerAsset(AssetType.Sound, path)
        bestow.assets.loadAsset(uiSounds[name])
    end
end

-- Play UI sound
local function playUISound(name, volume)
    volume = volume or 1.0
    bestow.audio.playOnChannel(bestow.audio.Channel.UI, {
        asset = uiSounds[name],
        volume = volume
    })
end

-- Usage
playUISound("click")
playUISound("hover", 0.5)
```

### 3D Footstep Sounds

```lua
local footstepHandle = nil

local function initFootsteps()
    footstepHandle = bestow.assets.registerAsset(
        AssetType.Sound,
        "sounds/footstep.wav"
    )
    bestow.assets.loadAsset(footstepHandle)
end

local function playFootstep(position)
    bestow.audio.playPositional({
        asset = footstepHandle,
        position = position,
        volume = 0.8,
        minDistance = 1.0,
        maxDistance = 20.0
    })
end

-- Call from movement system
local function onStep()
    local playerPos = bestow.entity.getField(
        app.main.state.player,
        "Transform3D",
        "position"
    )
    playFootstep(playerPos)
end
```

### Audio Listener Following Camera

```lua
-- In camera system update
local function updateAudioListener()
    local cam = bestow.graphics3d.getCamera()
    if not cam then return end

    bestow.audio.setListener({
        position = cam.position,
        forward = cam.rotation:rotateVector(Vec3.forward()),
        up = cam.rotation:rotateVector(Vec3.up()),
        velocity = Vec3.zero()  -- Add actual velocity for doppler
    })
end
```

### Settings Menu Audio Controls

```lua
-- systems/settings.lua
return {
    masterVolume = 1.0,
    musicVolume = 0.7,
    sfxVolume = 1.0,

    applyAudioSettings = function()
        local self = app.systems.settings
        bestow.audio.setMasterVolume(self.masterVolume)
        bestow.audio.setGroupVolume("music", self.musicVolume)
        bestow.audio.setGroupVolume("sfx", self.sfxVolume)
    end,

    setMasterVolume = function(volume)
        local self = app.systems.settings
        self.masterVolume = math.max(0, math.min(1, volume))
        self.applyAudioSettings()
    end,

    setMusicVolume = function(volume)
        local self = app.systems.settings
        self.musicVolume = math.max(0, math.min(1, volume))
        self.applyAudioSettings()
    end,

    setSFXVolume = function(volume)
        local self = app.systems.settings
        self.sfxVolume = math.max(0, math.min(1, volume))
        self.applyAudioSettings()
    end
}
```

### Pause Menu Audio

```lua
local function pauseGame()
    app.main.state.paused = true
    bestow.audio.pauseAll()

    -- Play pause sound (UI channel might be exempt from pauseAll)
    playUISound("pause")
end

local function resumeGame()
    app.main.state.paused = false
    bestow.audio.resumeAll()
end
```

## Best Practices

1. **Preload frequently used sounds** - Register and load in init()
2. **Use channels for dedicated streams** - Music, ambience, UI should have their own channels
3. **Use positional audio for world sounds** - Footsteps, explosions, NPCs
4. **Update the listener every frame** - Attach to camera or player
5. **Provide volume controls** - Master, music, SFX sliders in settings
6. **Fade transitions** - Use fadeInTime/fadeOutTime for smooth transitions
7. **Keep sound effects short** - Long sounds should be music/ambience
8. **Match minDistance to object size** - Larger objects need larger minDistance

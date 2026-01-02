---
name: audio-system
description: Play music, sound effects, and positional 3D audio in Bestow. Use when adding sounds, background music, adjusting volume, or implementing spatial audio.
---

# Audio System

The audio system handles music, sound effects, and 3D positional audio.

**Key Principle:** Audio assets are referenced by path. The engine loads and manages the actual audio data. Never try to read audio files directly.

## Channel-Based Audio

Channels are persistent audio slots for music and ambient sounds. Playing a new sound on a channel replaces the previous sound.

### Predefined Channels

```lua
Channels.Music    -- 0: Background music
Channels.Ambience -- 1: Environmental sounds
Channels.UI       -- 2: Interface sounds
Channels.Voice    -- 3: Dialogue/narration
```

### Playing on Channels

```lua
-- Play background music
bestow.audio.playOnChannel(Channels.Music, {
    path = "sounds/music/battle.ogg",
    volume = 0.8,
    pitch = 1.0,
    looping = true,
    fadeInTime = 2.0      -- Fade in over 2 seconds
})

-- Play ambient sound
bestow.audio.playOnChannel(Channels.Ambience, {
    path = "sounds/ambience/forest.ogg",
    volume = 0.5,
    looping = true
})

-- Play UI sound (no looping)
bestow.audio.playOnChannel(Channels.UI, {
    path = "sounds/ui/click.wav",
    volume = 1.0
})
```

### Channel Control

```lua
-- Stop with fade out
bestow.audio.stopChannel(Channels.Music, 1.5)  -- 1.5 second fade

-- Stop immediately
bestow.audio.stopChannel(Channels.Music, 0)

-- Pause/Resume
bestow.audio.pauseChannel(Channels.Music)
bestow.audio.resumeChannel(Channels.Music)

-- Adjust volume (0.0 to 1.0)
bestow.audio.setChannelVolume(Channels.Music, 0.5)

-- Adjust pitch (1.0 = normal, 2.0 = octave up, 0.5 = octave down)
bestow.audio.setChannelPitch(Channels.Music, 1.1)

-- Seek to position (seconds)
bestow.audio.seekChannel(Channels.Music, 30.0)

-- Check if playing
if bestow.audio.isChannelPlaying(Channels.Music) then
    -- Music is playing
end
```

## One-Shot Sound Effects

For sounds that play once and don't need control:

```lua
-- Play sound effect
bestow.audio.playOnChannel(Channels.UI, {
    path = "sounds/sfx/explosion.wav",
    volume = 1.0,
    pitch = 0.9 + math.random() * 0.2  -- Random pitch variation
})
```

## Positional (3D) Audio

For sounds that exist in 3D space:

```lua
-- Play positional sound
local handle = bestow.audio.playPositional({
    path = "sounds/sfx/gunshot.wav",
    position = Vec3.new(10, 0, 5),
    volume = 1.0,
    pitch = 1.0,
    minDistance = 5.0,    -- Full volume within this radius
    maxDistance = 50.0,   -- Inaudible beyond this
    velocity = Vec3.new(0, 0, 0)  -- For Doppler effect
})

-- Update position for moving sounds
bestow.audio.updatePositionalPosition(handle, newPosition)

-- Stop positional sound
bestow.audio.stopPositional(handle)

-- Check if still playing
if bestow.audio.isPositionalPlaying(handle) then
    -- Sound is still active
end
```

### Audio Listener

The listener is the "ears" - usually the camera or player:

```lua
-- Set listener position (call every frame)
bestow.audio.setListener({
    position = cameraPosition,
    forward = cameraForward,
    up = Vec3.new(0, 1, 0),
    velocity = playerVelocity  -- For Doppler effect
})
```

## Master Volume

```lua
-- Set master volume (affects everything)
bestow.audio.setMasterVolume(0.8)

-- Get current master volume
local vol = bestow.audio.getMasterVolume()

-- Pause/Resume all audio
bestow.audio.pauseAll()
bestow.audio.resumeAll()

-- Stop everything
bestow.audio.stopAll()
```

## Audio Groups

Organize sounds into groups for volume control:

```lua
-- Set volume for a group
bestow.audio.setGroupVolume("music", 0.7)
bestow.audio.setGroupVolume("sfx", 1.0)
bestow.audio.setGroupVolume("voice", 0.9)

-- Assign channel to group
bestow.audio.assignChannelToGroup(Channels.Music, "music")
bestow.audio.assignChannelToGroup(Channels.UI, "sfx")
```

## Audio System Module Pattern

Create an audio system to manage game sounds:

```lua
-- systems/audio.lua
return {
    -- Sound configuration
    sounds = {
        jump = { path = "sounds/sfx/jump.wav", volume = 0.7 },
        land = { path = "sounds/sfx/land.wav", volume = 0.5 },
        hit = { path = "sounds/sfx/hit.wav", volume = 0.8 },
        pickup = { path = "sounds/sfx/pickup.wav", volume = 0.6 },
        death = { path = "sounds/sfx/death.wav", volume = 1.0 }
    },

    music = {
        menu = { path = "sounds/music/menu.ogg", volume = 0.6 },
        gameplay = { path = "sounds/music/gameplay.ogg", volume = 0.5 },
        boss = { path = "sounds/music/boss.ogg", volume = 0.7 }
    },

    -- Play a sound effect
    playSfx = function(name)
        local self = app.systems.audio
        local sound = self.sounds[name]
        if sound then
            bestow.audio.playOnChannel(Channels.UI, {
                path = sound.path,
                volume = sound.volume
            })
        end
    end,

    -- Play with random pitch variation
    playSfxRandomized = function(name)
        local self = app.systems.audio
        local sound = self.sounds[name]
        if sound then
            bestow.audio.playOnChannel(Channels.UI, {
                path = sound.path,
                volume = sound.volume,
                pitch = 0.9 + math.random() * 0.2
            })
        end
    end,

    -- Play music with crossfade
    playMusic = function(name)
        local self = app.systems.audio
        local music = self.music[name]
        if music then
            bestow.audio.stopChannel(Channels.Music, 1.0)  -- Fade out current

            -- Delay new music slightly for crossfade
            bestow.audio.playOnChannel(Channels.Music, {
                path = music.path,
                volume = music.volume,
                looping = true,
                fadeInTime = 1.0
            })
        end
    end,

    -- Stop all music
    stopMusic = function()
        bestow.audio.stopChannel(Channels.Music, 1.5)
    end,

    -- Update listener position
    updateListener = function()
        local state = app.main.state
        if state.player then
            local pos = bestow.entity.getField(state.player, "Transform3D", "position")
            local camera = bestow.camera3d.getCamera()

            bestow.audio.setListener({
                position = camera.position,
                forward = camera.forward or Vec3.new(0, 0, -1),
                up = Vec3.new(0, 1, 0)
            })
        end
    end
}
```

Usage:
```lua
-- In game code
app.systems.audio.playSfx("jump")
app.systems.audio.playMusic("gameplay")

-- In update loop
app.systems.audio.updateListener()
```

## Audio for Common Events

```lua
-- Player jump
if bestow.input.wasKeyJustPressed(Keys.Space) and state.grounded then
    app.systems.audio.playSfx("jump")
end

-- Taking damage
function takeDamage(entity, amount)
    local health = bestow.entity.getComponent(entity, "Health")
    health.current = health.current - amount
    bestow.entity.setComponent(entity, "Health", health)

    app.systems.audio.playSfx("hit")

    if health.current <= 0 then
        app.systems.audio.playSfx("death")
    end
end

-- Collecting item
function collectItem(player, item)
    app.systems.audio.playSfx("pickup")
    bestow.entity.destroy(item)
end

-- Level transition
function changeLevel(levelName)
    app.systems.audio.playMusic(levelName)
    app.levels[levelName].load()
end
```

## Best Practices

1. **Reference sounds by path** - Never try to load audio data directly
2. **Use channels for persistent sounds** - Music, ambience, UI
3. **Use positional audio for 3D sounds** - Explosions, footsteps, voices
4. **Update listener every frame** - Keep 3D audio synchronized
5. **Add pitch variation to repeated sounds** - Prevents mechanical feel
6. **Fade music transitions** - Smoother than hard cuts
7. **Group related sounds** - Easier volume management
8. **Keep SFX short** - Channel audio replaces, doesn't layer

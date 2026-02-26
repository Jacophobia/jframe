# Audio System

> **Visibility:** Public (Lua + C++ API)
> **Tier:** 3
> **Dependencies:** Types, Assets, Entity
> **Lua Paths:** `bestow.audio` (high-level), `bestow.audio.core` (low-level)

## Purpose

The Audio System manages all sound playback, from fire-and-forget sound effects to full 3D positional audio with listener tracking. The high-level API provides path-based play calls, music management with crossfade, and simple volume control. The low-level API exposes channel-based playback with per-channel control, channel groups for bus mixing, DSP effects, 3D listener configuration, fade/crossfade operations, and runtime performance queries. The system is backed by FMOD Core and processes all audio through the protected IAssetCore interface.

## High-Level API: `IAudioSystem`

The simplified API for common game development tasks. Path-based, fire-and-forget with sensible defaults.

### Playback

| Method | Returns | Description |
|--------|---------|-------------|
| `play(std::string_view soundPath, float volume = 1.0f)` | `Result<SoundHandle>` | Play a sound effect at the given path; returns a handle to control or query it |
| `playAt(std::string_view soundPath, Vec3 position, float volume = 1.0f)` | `Result<SoundHandle>` | Play a positional 3D sound at the given world position |

### Music

| Method | Returns | Description |
|--------|---------|-------------|
| `playMusic(std::string_view musicPath, float fadeIn = 1.0f, bool loop = true)` | `Result<void>` | Start playing background music with a fade-in; crossfades if music is already playing |
| `stopMusic(float fadeOut = 1.0f)` | `Result<void>` | Stop the currently playing music with a fade-out |

### Control

| Method | Returns | Description |
|--------|---------|-------------|
| `stop(SoundHandle handle)` | `Result<void>` | Stop a specific playing sound by its handle |
| `stopAll()` | `void` | Immediately stop all currently playing sounds and music |
| `isPlaying(SoundHandle handle)` | `bool` | Check whether a sound is currently playing |

### Volume

| Method | Returns | Description |
|--------|---------|-------------|
| `setMasterVolume(float volume)` | `void` | Set the master volume affecting all audio output (0.0 to 1.0) |
| `setMusicVolume(float volume)` | `void` | Set the volume for the music channel group (0.0 to 1.0) |
| `setSFXVolume(float volume)` | `void` | Set the volume for the sound effects channel group (0.0 to 1.0) |

## Low-Level API: `IAudioCore`

Full control API. Every configuration knob exposed. Channel-based playback, groups, effects, 3D audio, fading, and performance queries.

### Lifecycle

| Method | Returns | Description |
|--------|---------|-------------|
| `initialize()` | `Result<void>` | Initialize the audio engine and allocate system resources |
| `shutdown()` | `void` | Release all audio resources and shut down the engine |
| `update(DeltaTime dt)` | `void` | Advance the audio engine state; processes 3D updates, fades, and completions |

### Channel-Based Playback

| Method | Returns | Description |
|--------|---------|-------------|
| `playOnChannel(ChannelHandle channel, AssetHandle sound, float volume = 1.0f, bool loop = false)` | `Result<void>` | Play a loaded sound asset on a specific channel with volume and loop control |
| `stopChannel(ChannelHandle channel, float fadeOut = 0)` | `Result<void>` | Stop playback on a channel, optionally with a fade-out duration in seconds |
| `pauseChannel(ChannelHandle channel)` | `Result<void>` | Pause playback on a channel; can be resumed later |
| `resumeChannel(ChannelHandle channel)` | `Result<void>` | Resume a previously paused channel |
| `setChannelVolume(ChannelHandle channel, float volume)` | `Result<void>` | Set the volume of a specific channel (0.0 to 1.0) |
| `setChannelPitch(ChannelHandle channel, float pitch)` | `Result<void>` | Set the pitch multiplier for a channel (1.0 = normal, 2.0 = octave up) |
| `setChannelPan(ChannelHandle channel, float pan)` | `Result<void>` | Set the stereo pan for a channel (-1.0 = full left, 1.0 = full right) |
| `seekChannel(ChannelHandle channel, float position)` | `Result<void>` | Seek to a position in the sound on this channel (in seconds) |
| `getChannelPosition(ChannelHandle channel)` | `float` | Return the current playback position on a channel in seconds |
| `getChannelDuration(ChannelHandle channel)` | `float` | Return the total duration of the sound loaded on this channel in seconds |
| `isChannelPlaying(ChannelHandle channel)` | `bool` | Check whether a channel is currently playing |
| `isChannelPaused(ChannelHandle channel)` | `bool` | Check whether a channel is currently paused |

### Fire-and-Forget Playback

| Method | Returns | Description |
|--------|---------|-------------|
| `playOneShot(AssetHandle sound, float volume = 1.0f)` | `Result<SoundHandle>` | Play a sound once without channel management; returns a handle for early stop |
| `stopOneShot(SoundHandle handle)` | `Result<void>` | Stop a fire-and-forget sound before it completes naturally |

### Positional Audio (3D)

| Method | Returns | Description |
|--------|---------|-------------|
| `playPositional(AssetHandle sound, Vec3 position, float minDist = 1.0f, float maxDist = 100.0f, float volume = 1.0f)` | `Result<SoundHandle>` | Play a 3D positioned sound with distance attenuation |
| `updatePositionalPosition(SoundHandle handle, Vec3 pos)` | `Result<void>` | Update the world position of a currently playing 3D sound |
| `isPositionalPlaying(SoundHandle handle)` | `bool` | Check whether a 3D positional sound is still playing |

### 3D Listener

| Method | Returns | Description |
|--------|---------|-------------|
| `setListenerPosition(Vec3 position)` | `void` | Set the 3D listener position for spatial audio calculations |
| `setListenerOrientation(Vec3 forward, Vec3 up)` | `void` | Set the listener's forward and up vectors for 3D orientation |
| `setListenerVelocity(Vec3 velocity)` | `void` | Set the listener velocity for Doppler effect calculations |
| `setDopplerScale(float scale)` | `void` | Set the global Doppler effect scale (0.0 = disabled, 1.0 = realistic) |
| `setDistanceModel(DistanceModel model)` | `void` | Set the distance attenuation model for all 3D sounds |

### Volume Control

| Method | Returns | Description |
|--------|---------|-------------|
| `setMasterVolume(float volume)` | `void` | Set the master volume that scales all audio output (0.0 to 1.0) |
| `getMasterVolume()` | `float` | Return the current master volume level |

### Channel Groups

| Method | Returns | Description |
|--------|---------|-------------|
| `createGroup(std::string_view name)` | `Result<void>` | Create a named channel group for bus-style mixing |
| `destroyGroup(std::string_view name)` | `Result<void>` | Destroy a named channel group and release its resources |
| `setGroupVolume(std::string_view name, float volume)` | `Result<void>` | Set the volume for all channels in a group (0.0 to 1.0) |
| `getGroupVolume(std::string_view name)` | `float` | Return the current volume of a channel group |
| `assignChannelToGroup(ChannelHandle channel, std::string_view group)` | `Result<void>` | Assign a channel to a named group for collective volume/effect control |
| `muteGroup(std::string_view name)` | `Result<void>` | Mute all channels in a group without changing their volume settings |
| `unmuteGroup(std::string_view name)` | `Result<void>` | Unmute a previously muted channel group |

### Global Controls

| Method | Returns | Description |
|--------|---------|-------------|
| `pauseAll()` | `void` | Pause all currently playing sounds across all channels and groups |
| `resumeAll()` | `void` | Resume all paused sounds across all channels and groups |
| `stopAll(float fadeOut = 0)` | `void` | Stop all sounds, optionally with a global fade-out duration |

### DSP Effects

| Method | Returns | Description |
|--------|---------|-------------|
| `addChannelEffect(ChannelHandle channel, AudioEffect effect)` | `Result<void>` | Add a DSP effect to a specific channel |
| `removeChannelEffects(ChannelHandle channel)` | `Result<void>` | Remove all DSP effects from a channel |
| `addGroupEffect(std::string_view group, AudioEffect effect)` | `Result<void>` | Add a DSP effect to an entire channel group |

### Bus Routing

| Method | Returns | Description |
|--------|---------|-------------|
| `setChannelOutput(ChannelHandle channel, std::string_view busName)` | `Result<void>` | Route a channel's output to a named bus for hierarchical mixing |
| `setGroupOutput(std::string_view group, std::string_view busName)` | `Result<void>` | Route a group's output to a named bus |

### Fade and Crossfade

| Method | Returns | Description |
|--------|---------|-------------|
| `fadeChannel(ChannelHandle channel, float targetVolume, float duration)` | `Result<void>` | Smoothly fade a channel's volume to a target over a duration in seconds |
| `crossfade(ChannelHandle from, ChannelHandle to, float duration)` | `Result<void>` | Crossfade from one channel to another over a duration in seconds |

### Queries

| Method | Returns | Description |
|--------|---------|-------------|
| `getActiveChannelCount()` | `int` | Return the number of channels currently in use |
| `getActiveSoundCount()` | `int` | Return the total number of sounds currently playing |
| `getCPUUsage()` | `float` | Return the audio engine CPU usage as a percentage (0.0 to 100.0) |

## Types

### DistanceModel

Controls how sound volume attenuates with distance from the listener.

```cpp
enum class DistanceModel : std::uint8_t {
    Linear,                  // Linear falloff between min and max distance
    InverseDistance,          // 1/distance falloff (realistic)
    InverseDistanceClamped   // 1/distance clamped at min and max distance
};
```

| Value | Description |
|-------|-------------|
| `Linear` | Volume decreases linearly from 1.0 at minDistance to 0.0 at maxDistance |
| `InverseDistance` | Volume follows inverse distance law; sounds never fully attenuate |
| `InverseDistanceClamped` | Inverse distance clamped to zero at maxDistance |

### AudioEffect

DSP effect definition with type and configuration parameters.

| Field | Type | Default | Description |
|-------|------|---------|-------------|
| `type` | `AudioEffect::Type` | `Reverb` | The DSP effect algorithm to apply |
| `params` | `std::unordered_map<std::string, float>` | `{}` | Named parameters for the effect (e.g., `"wetDry"`, `"decay"`, `"delay"`) |

```cpp
enum class AudioEffect::Type : std::uint8_t {
    Reverb,       // Room reverberation
    Echo,         // Delayed repetition
    Chorus,       // Pitch modulation for thickness
    Flanger,      // Short delay modulation
    Distortion,   // Signal clipping/overdrive
    LowPass,      // Attenuate high frequencies
    HighPass,     // Attenuate low frequencies
    BandPass,     // Pass only a frequency range
    Compressor,   // Dynamic range compression
    Limiter       // Hard volume ceiling
};
```

### ChannelHandle

A strong typed handle identifying a specific audio channel.

```cpp
using ChannelHandle = Handle<struct ChannelTag>;
```

| Field | Type | Description |
|-------|------|-------------|
| `id` | `std::uint64_t` | The unique channel identifier; 0 indicates an invalid handle |

### SoundHandle

A strong typed handle identifying a playing sound instance (fire-and-forget or positional).

```cpp
using SoundHandle = Handle<struct SoundTag>;
```

| Field | Type | Description |
|-------|------|-------------|
| `id` | `std::uint64_t` | The unique sound instance identifier; 0 indicates an invalid handle |

## Lua Examples

```lua
-- High-level: Fire-and-forget sound effects
local sfx, err = bestow.audio.play("sounds/coin.wav")
local sfx3d = bestow.audio.playAt("sounds/explosion.wav", { x = 50, y = 0, z = 100 })

-- High-level: Music with crossfade
bestow.audio.playMusic("music/battle.ogg", 2.0, true)
bestow.audio.stopMusic(1.5)

-- High-level: Volume control
bestow.audio.setMasterVolume(0.8)
bestow.audio.setMusicVolume(0.5)
bestow.audio.setSFXVolume(1.0)

-- High-level: Stop sounds
bestow.audio.stop(sfx)
bestow.audio.stopAll()

-- Low-level: Channel-based playback
bestow.audio.core.playOnChannel(musicChannel, bgmAsset, 0.7, true)
bestow.audio.core.setChannelPitch(musicChannel, 1.2)
bestow.audio.core.seekChannel(musicChannel, 30.0)
local pos = bestow.audio.core.getChannelPosition(musicChannel)
bestow.audio.core.pauseChannel(musicChannel)
bestow.audio.core.resumeChannel(musicChannel)

-- Low-level: Channel groups
bestow.audio.core.createGroup("sfx")
bestow.audio.core.createGroup("music")
bestow.audio.core.assignChannelToGroup(channel, "sfx")
bestow.audio.core.setGroupVolume("sfx", 0.8)
bestow.audio.core.muteGroup("music")

-- Low-level: 3D listener
bestow.audio.core.setListenerPosition({ x = 0, y = 1.7, z = 0 })
bestow.audio.core.setListenerOrientation(
    { x = 0, y = 0, z = -1 },  -- forward
    { x = 0, y = 1, z = 0 }    -- up
)
bestow.audio.core.setDistanceModel("InverseDistanceClamped")

-- Low-level: DSP effects
bestow.audio.core.addChannelEffect(channel, {
    type = "Reverb",
    params = { wetDry = 0.3, decay = 1.5 }
})
bestow.audio.core.addGroupEffect("sfx", {
    type = "LowPass",
    params = { cutoff = 2000 }
})

-- Low-level: Fading
bestow.audio.core.fadeChannel(musicChannel, 0.0, 2.0)
bestow.audio.core.crossfade(oldMusic, newMusic, 1.5)

-- Low-level: Queries
local channels = bestow.audio.core.getActiveChannelCount()
local sounds = bestow.audio.core.getActiveSoundCount()
local cpu = bestow.audio.core.getCPUUsage()
```

## C++ Examples

```cpp
// High-level: Simple playback
auto sfx = audio->play("sounds/jump.wav", 0.8f);
auto positional = audio->playAt("sounds/footstep.wav", playerPosition);

audio->playMusic("music/ambient.ogg", 2.0f, true);
audio->setMasterVolume(0.9f);
audio->setMusicVolume(0.6f);
audio->setSFXVolume(1.0f);

if (audio->isPlaying(sfx.value())) {
    audio->stop(sfx.value());
}

// Low-level: Channel management
audioCore->playOnChannel(musicChannel, bgmAsset, 0.7f, true);
audioCore->setChannelVolume(musicChannel, 0.5f);
audioCore->setChannelPitch(musicChannel, 1.1f);
audioCore->setChannelPan(musicChannel, -0.3f);
audioCore->seekChannel(musicChannel, 60.0f);

float pos = audioCore->getChannelPosition(musicChannel);
float dur = audioCore->getChannelDuration(musicChannel);

// Low-level: Fire-and-forget
auto shot = audioCore->playOneShot(gunSound, 1.0f);

// Low-level: 3D positional audio
auto ambient = audioCore->playPositional(birdSound, Vec3{10, 5, 0}, 2.0f, 50.0f);
audioCore->updatePositionalPosition(ambient.value(), newPosition);

// Low-level: Listener setup
audioCore->setListenerPosition(cameraPosition);
audioCore->setListenerOrientation(cameraForward, cameraUp);
audioCore->setListenerVelocity(cameraVelocity);
audioCore->setDopplerScale(1.0f);
audioCore->setDistanceModel(DistanceModel::InverseDistanceClamped);

// Low-level: Channel groups
audioCore->createGroup("sfx");
audioCore->createGroup("ambient");
audioCore->assignChannelToGroup(sfxChannel, "sfx");
audioCore->setGroupVolume("sfx", 0.8f);
audioCore->muteGroup("ambient");

// Low-level: DSP effects
audioCore->addChannelEffect(musicChannel, AudioEffect{
    .type = AudioEffect::Type::Reverb,
    .params = {{"wetDry", 0.4f}, {"decay", 2.0f}}
});
audioCore->addGroupEffect("sfx", AudioEffect{
    .type = AudioEffect::Type::Compressor,
    .params = {{"threshold", -20.0f}, {"ratio", 4.0f}}
});

// Low-level: Fading
audioCore->fadeChannel(musicChannel, 0.0f, 3.0f);
audioCore->crossfade(oldMusicChannel, newMusicChannel, 2.0f);

// Low-level: Global controls
audioCore->pauseAll();
audioCore->resumeAll();
audioCore->stopAll(1.0f);

// Low-level: Performance queries
int channels = audioCore->getActiveChannelCount();
float cpu = audioCore->getCPUUsage();
```

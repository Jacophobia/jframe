# Bestow Audio System Guide

**A comprehensive guide to the Bestow Audio System powered by FMOD Core API**

## Table of Contents

1. [Overview](#overview)
2. [Core Concepts](#core-concepts)
3. [Getting Started](#getting-started)
4. [API Reference](#api-reference)
5. [Sound Loading](#sound-loading)
6. [Spatial Audio](#spatial-audio)
7. [Best Practices](#best-practices)
8. [Code Examples](#code-examples)

---

## Overview

The Bestow Audio System is a high-level audio engine built on top of FMOD Core API. It provides:

- **Channel-based audio** for music, ambience, UI, and voice
- **Positional 3D audio** with distance attenuation and doppler effects
- **Channel groups** for flexible mixing and volume control
- **Master volume** and global pause/resume controls
- **Asset system integration** for hot-reload support
- **Stub mode** for testing without FMOD (graceful degradation)

### FMOD Features Used

- 512 simultaneous channels (configurable)
- Full 3D audio with listener positioning
- Volume, pitch, and seek controls
- Looping and one-shot playback
- Distance-based attenuation
- Velocity-based doppler shift

---

## Core Concepts

### Channels vs Positional Sounds

The audio system provides two playback models:

#### Channel-Based Audio (Managed)

**Best for:** Background music, UI sounds, narration, ambient loops

```cpp
// Named channels for different purposes
Channels::Music      // Background music tracks
Channels::Ambience   // Ambient soundscapes
Channels::UI         // Button clicks, menu sounds
Channels::Voice      // Dialogue and narration
```

Characteristics:
- **One sound per channel** - Playing a new sound replaces the current one
- **Persistent control** - Pause, resume, seek, and adjust volume after playback starts
- **Global control** - Affected by `pauseAll()`, `resumeAll()`, `stopAll()`
- **Channel groups** - Can be assigned to groups for batch volume control

#### Positional Audio (Fire-and-Forget)

**Best for:** Sound effects tied to game entities, footsteps, gunshots, explosions

```cpp
PositionalSound sound{
    .asset = footstepAsset,
    .position = playerPosition,
    .minDistance = 5.0f,    // Full volume within this radius
    .maxDistance = 50.0f    // Silent beyond this radius
};
SoundHandle handle = audio->playPositional(sound);
```

Characteristics:
- **Multiple simultaneous sounds** - Can play many at once
- **3D spatialization** - Volume/pan based on distance and direction from listener
- **Fire-and-forget** - Typically plays once and cleans up automatically
- **Optional tracking** - Can update position or stop early using the returned handle

### Channel Groups

Channel groups allow you to control multiple channels together:

```cpp
// Create a group and set volume
audio->setGroupVolume("sfx", 0.8f);

// Assign channels to the group
audio->assignChannelToGroup(Channels::UI, "sfx");
audio->assignChannelToGroup(42, "sfx");  // Custom channel ID

// All channels in "sfx" group now play at 80% volume
```

Use cases:
- Settings menu with separate Music/SFX/Voice sliders
- Mute all combat sounds during cutscenes
- Ducking (lowering music volume during dialogue)

### 3D Audio Listener

The listener represents the player's "ears" in the game world:

```cpp
AudioListener listener{
    .position = cameraPosition,
    .forward = cameraForward,    // Where the camera faces
    .up = Vec3{0.0f, 1.0f, 0.0f},
    .velocity = playerVelocity   // For doppler effect
};
audio->setListener(listener);
```

Update the listener every frame for proper 3D audio spatialization.

---

## Getting Started

### Initialization

```cpp
import bestow.audio;
import bestow.audio.impl;

// Get the audio system (through dependency injection)
class MyGame : public IApplication {
public:
    MyGame(IAudioSystem& audio, IAssetSystem& assets)
        : audio_(&audio), assets_(&assets) {}

    void run() override {
        // Initialize audio system
        if (!audio_->initialize()) {
            std::cerr << "Failed to initialize audio system" << std::endl;
            return;
        }

        // Game loop...
    }

private:
    IAudioSystem* audio_;
    IAssetSystem* assets_;
};
```

### Update Loop

Call `update()` every frame to process FMOD events and clean up finished sounds:

```cpp
void update(float deltaTime) {
    audio_->update(deltaTime);  // CRITICAL - must call every frame
}
```

What `update()` does:
- Processes FMOD's internal systems (3D audio, virtual channels, etc.)
- Updates channel states (playing, paused, position, volume)
- Cleans up finished positional sounds
- Handles fade-outs and transitions

### Shutdown

```cpp
void shutdown() {
    audio_->stopAll();      // Stop all playing sounds
    audio_->shutdown();     // Release FMOD resources
}
```

---

## API Reference

### Channel-Based Audio

#### Play Sound on Channel

```cpp
void playOnChannel(Channel channel, const ChannelSound& sound);
```

Plays a sound on the specified channel. If another sound is already playing on that channel, it stops immediately.

**Parameters:**
```cpp
struct ChannelSound {
    AssetHandle asset;              // Sound asset from AssetSystem
    Volume volume = 1.0f;           // 0.0 = silent, 1.0 = full, >1.0 = amplified
    float pitch = 1.0f;             // 1.0 = normal, 2.0 = double speed, 0.5 = half speed
    bool looping = false;           // true = loop forever, false = play once
    float fadeInTime = 0.0f;        // Fade-in duration in seconds (TODO: not yet implemented)
    std::optional<float> startTime; // Start playback at this time offset (seconds)
};
```

**Example:**
```cpp
AssetHandle musicAsset = assets->registerAsset(AssetType::Sound, ":assets:/music/battle_theme.ogg");
assets->loadAsset(musicAsset);

ChannelSound music{
    .asset = musicAsset,
    .volume = 0.7f,
    .looping = true
};
audio->playOnChannel(Channels::Music, music);
```

#### Stop Channel

```cpp
void stopChannel(Channel channel, float fadeOutTime = 0.0f);
```

Stops playback on the specified channel.

**Parameters:**
- `channel` - The channel to stop
- `fadeOutTime` - Duration of fade-out in seconds (TODO: not yet implemented - currently stops immediately)

**Example:**
```cpp
audio->stopChannel(Channels::Music);            // Immediate stop
audio->stopChannel(Channels::Music, 1.5f);      // TODO: 1.5s fade-out (not yet implemented)
```

#### Pause/Resume Channel

```cpp
void pauseChannel(Channel channel);
void resumeChannel(Channel channel);
```

Pauses or resumes a specific channel without stopping it. Playback position is preserved.

**Example:**
```cpp
audio->pauseChannel(Channels::Music);   // Pause music
// ... later ...
audio->resumeChannel(Channels::Music);  // Resume from where it paused
```

#### Channel Volume

```cpp
void setChannelVolume(Channel channel, Volume volume);
```

Sets the volume of a specific channel. Can be changed while playing.

**Parameters:**
- `volume` - Volume multiplier (0.0 = silent, 1.0 = full, >1.0 = amplified)

**Example:**
```cpp
audio->setChannelVolume(Channels::Music, 0.5f);  // 50% volume
```

#### Channel Pitch

```cpp
void setChannelPitch(Channel channel, float pitch);
```

Changes playback speed and pitch. Affects both tempo and frequency.

**Parameters:**
- `pitch` - Pitch multiplier (1.0 = normal, 2.0 = double speed/octave up, 0.5 = half speed/octave down)

**Example:**
```cpp
audio->setChannelPitch(Channels::Voice, 0.8f);  // Slow, deep voice
audio->setChannelPitch(Channels::Voice, 1.5f);  // Fast, high voice
```

#### Seek Channel

```cpp
void seekChannel(Channel channel, float position);
```

Jumps to a specific time position in the playing sound.

**Parameters:**
- `position` - Time offset in seconds

**Example:**
```cpp
audio->seekChannel(Channels::Music, 30.0f);  // Jump to 30 seconds
audio->seekChannel(Channels::Music, 0.0f);   // Rewind to start
```

#### Channel State

```cpp
ChannelState getChannelState(Channel channel) const;
bool isChannelPlaying(Channel channel) const;
```

Query the current state of a channel.

**ChannelState:**
```cpp
struct ChannelState {
    bool isPlaying = false;  // True if sound is playing
    bool isPaused = false;   // True if paused
    float position = 0.0f;   // Current playback position (seconds)
    float length = 0.0f;     // Total sound length (seconds)
    Volume volume = 1.0f;    // Current volume
};
```

**Example:**
```cpp
if (audio->isChannelPlaying(Channels::Music)) {
    ChannelState state = audio->getChannelState(Channels::Music);
    float progress = state.position / state.length;
    std::cout << "Music is " << (progress * 100) << "% complete\n";
}
```

### Positional Audio

#### Play Positional Sound

```cpp
SoundHandle playPositional(const PositionalSound& sound);
```

Plays a 3D positioned sound in the game world. Returns a handle for tracking.

**Parameters:**
```cpp
struct PositionalSound {
    AssetHandle asset;              // Sound asset
    Vec3 position{0.0f};            // World position of sound source
    Volume volume = 1.0f;           // Base volume before distance attenuation
    float pitch = 1.0f;             // Pitch multiplier
    float minDistance = 1.0f;       // Full volume within this radius
    float maxDistance = 100.0f;     // Silent beyond this radius
    std::optional<Vec3> velocity;   // Movement for doppler effect
    std::function<void()> onComplete; // TODO: Callback when finished (not yet implemented)
};
```

**Example:**
```cpp
PositionalSound explosion{
    .asset = explosionAsset,
    .position = Vec3{100.0f, 0.0f, 50.0f},
    .volume = 1.0f,
    .minDistance = 10.0f,
    .maxDistance = 200.0f
};
SoundHandle handle = audio->playPositional(explosion);
```

#### Stop Positional Sound

```cpp
void stopPositional(SoundHandle handle);
```

Stops a specific positional sound before it finishes naturally.

**Example:**
```cpp
SoundHandle loopingEngine = audio->playPositional(engineSound);
// ... later ...
audio->stopPositional(loopingEngine);  // Stop the engine sound
```

#### Update Position

```cpp
void updatePositionalPosition(SoundHandle handle, Vec3 position);
```

Updates the world position of a playing positional sound. Use this for sounds attached to moving entities.

**Example:**
```cpp
// In your update loop
void updateMovingEntity(Entity entity) {
    Vec3 newPos = getEntityPosition(entity);
    audio->updatePositionalPosition(entitySoundHandle, newPos);
}
```

#### Check Positional Status

```cpp
bool isPositionalPlaying(SoundHandle handle) const;
```

Returns true if the sound is still playing.

**Example:**
```cpp
if (!audio->isPositionalPlaying(soundHandle)) {
    std::cout << "Sound finished playing\n";
}
```

### 3D Audio Listener

#### Set Listener

```cpp
void setListener(const AudioListener& listener);
AudioListener getListener() const;
```

Sets the listener's position and orientation. This should match your camera or player position.

**Parameters:**
```cpp
struct AudioListener {
    Vec3 position{0.0f};            // Listener position in world space
    Vec3 forward{0.0f, 0.0f, -1.0f}; // Forward direction (normalized)
    Vec3 up{0.0f, 1.0f, 0.0f};      // Up direction (normalized)
    Vec3 velocity{0.0f};            // Movement velocity for doppler
};
```

**Example:**
```cpp
// Update listener every frame
void updateAudio(const Camera& camera, const Player& player) {
    AudioListener listener{
        .position = camera.position,
        .forward = camera.forward,
        .up = camera.up,
        .velocity = player.velocity
    };
    audio->setListener(listener);
}
```

### Global Controls

#### Master Volume

```cpp
void setMasterVolume(Volume volume);
Volume getMasterVolume() const;
```

Controls the global volume multiplier for all audio.

**Example:**
```cpp
audio->setMasterVolume(0.0f);   // Mute everything
audio->setMasterVolume(0.5f);   // 50% volume
audio->setMasterVolume(1.0f);   // Full volume
```

#### Pause/Resume All

```cpp
void pauseAll();
void resumeAll();
void stopAll();
```

Global controls affecting all active sounds.

**Example:**
```cpp
void togglePause() {
    if (isPaused) {
        audio->resumeAll();
    } else {
        audio->pauseAll();
    }
    isPaused = !isPaused;
}

void returnToMainMenu() {
    audio->stopAll();  // Stop everything (music, SFX, etc.)
}
```

### Channel Groups

#### Set Group Volume

```cpp
void setGroupVolume(const std::string& group, Volume volume);
```

Creates a channel group (if it doesn't exist) and sets its volume. All channels in this group will be affected.

**Example:**
```cpp
audio->setGroupVolume("sfx", 0.8f);     // 80% volume for SFX
audio->setGroupVolume("music", 0.6f);   // 60% volume for music
audio->setGroupVolume("voice", 1.0f);   // 100% volume for dialogue
```

#### Assign Channel to Group

```cpp
void assignChannelToGroup(Channel channel, const std::string& group);
```

Assigns a channel to a group. The channel will inherit the group's volume multiplier.

**Example:**
```cpp
// Set up groups
audio->setGroupVolume("sfx", 0.7f);

// Assign channels
audio->assignChannelToGroup(Channels::UI, "sfx");
audio->assignChannelToGroup(42, "sfx");  // Custom channel for footsteps

// Now changing group volume affects all assigned channels
audio->setGroupVolume("sfx", 0.3f);  // Both UI and channel 42 are now quieter
```

---

## Sound Loading

### Supported Formats

FMOD Core supports many audio formats including:

- **WAV** - Uncompressed, best for short sounds (UI, SFX)
- **OGG Vorbis** - Compressed, best for music and long sounds
- **MP3** - Compressed, widely supported
- **FLAC** - Lossless compression
- **AIFF** - Uncompressed (macOS)

### AssetSystem Integration

The audio system integrates with Bestow's AssetSystem for loading and hot-reload support.

#### Basic Loading

```cpp
// Register the asset
AssetHandle soundAsset = assets->registerAsset(
    AssetType::Sound,
    ":assets:/audio/jump.wav"
);

// Load it (sync or async)
assets->loadAsset(soundAsset);

// Use it
ChannelSound sound{.asset = soundAsset};
audio->playOnChannel(Channels::UI, sound);
```

#### Async Loading

```cpp
AssetHandle musicAsset = assets->registerAsset(
    AssetType::Sound,
    ":assets:/music/battle_theme.ogg"
);

assets->loadAssetAsync(musicAsset, [this](AssetHandle handle, AssetState state) {
    if (state == AssetState::Loaded) {
        std::cout << "Music loaded! Ready to play.\n";
        ChannelSound music{.asset = handle, .looping = true};
        audio->playOnChannel(Channels::Music, music);
    }
});
```

#### Hot Reload Support

```cpp
// Subscribe to asset changes
SubscriptionId subId = assets->subscribe(soundAsset, [this](AssetHandle handle, AssetType type) {
    std::cout << "Sound was reloaded from disk!\n";
    // Audio system automatically invalidates cache
    // Next playback will use the new audio data
});

// Enable hot reload monitoring
assets->enableHotReload(true);
```

### Preloading vs Streaming

#### Preload (Default)

Best for short sounds (SFX, UI):

```cpp
// Sound is fully loaded into memory
AssetHandle sfxAsset = assets->registerAsset(AssetType::Sound, ":assets:/sfx/explosion.wav");
assets->loadAsset(sfxAsset);
```

**Pros:** Instant playback, no disk I/O during gameplay
**Cons:** Uses more memory

#### Streaming (TODO)

Best for long music tracks:

```cpp
// TODO: Streaming is not yet implemented
// For now, all sounds are preloaded
```

**Pros:** Lower memory usage
**Cons:** Disk I/O during playback (may cause hitches on slow storage)

### Asset Paths

Use Bestow's path prefixes for portability:

```cpp
// Assets directory (bundled with game)
":assets:/audio/sfx/jump.wav"
":assets:/music/battle_theme.ogg"

// Library directory (engine resources)
":library:/sounds/ui_click.wav"
```

---

## Spatial Audio

### Distance Attenuation

FMOD uses a linear rolloff model by default:

```
Volume at distance d:
- d < minDistance:  100% volume (no attenuation)
- minDistance < d < maxDistance:  Linear fade from 100% to 0%
- d > maxDistance:  0% volume (silent)
```

**Example:**
```cpp
PositionalSound gunshot{
    .asset = gunshotAsset,
    .position = gunPosition,
    .minDistance = 5.0f,    // Full volume within 5 units
    .maxDistance = 100.0f   // Silent beyond 100 units
};
```

### Panning and Stereo

FMOD automatically handles stereo panning based on sound position relative to the listener:

- Sound to the left → Louder in left channel
- Sound to the right → Louder in right channel
- Sound directly ahead/behind → Equal in both channels

### Doppler Effect

Enable doppler shift by setting the listener and sound velocities:

```cpp
// Set listener velocity (player movement)
AudioListener listener{
    .position = playerPosition,
    .forward = playerForward,
    .velocity = playerVelocity  // e.g., Vec3{10.0f, 0.0f, 0.0f}
};
audio->setListener(listener);

// Set sound velocity (moving car)
PositionalSound carEngine{
    .asset = engineAsset,
    .position = carPosition,
    .velocity = carVelocity  // e.g., Vec3{-20.0f, 0.0f, 0.0f}
};
```

Sound pitch will shift based on relative velocity (higher pitch when approaching, lower when receding).

### Listener Orientation

The listener's `forward` and `up` vectors affect which sounds you hear clearly:

```cpp
AudioListener listener{
    .position = Vec3{0.0f, 0.0f, 0.0f},
    .forward = Vec3{1.0f, 0.0f, 0.0f},   // Facing right
    .up = Vec3{0.0f, 1.0f, 0.0f}         // Standard up
};
```

Sounds in front of the listener may be emphasized over sounds behind (depending on FMOD 3D settings).

---

## Best Practices

### Sound Organization

#### File Structure

```
assets/
  audio/
    music/
      menu_theme.ogg
      battle_theme.ogg
      victory_fanfare.ogg
    sfx/
      ui/
        button_click.wav
        menu_hover.wav
      player/
        jump.wav
        land.wav
        footstep_01.wav
        footstep_02.wav
      enemies/
        enemy_hurt.wav
        enemy_death.wav
    voice/
      narrator_intro.ogg
      dialogue_01.ogg
```

#### Asset Registration

Create an audio manager to centralize asset registration:

```cpp
class AudioManager {
public:
    void loadAllSounds(IAssetSystem& assets) {
        // UI sounds
        uiClick = assets.registerAsset(AssetType::Sound, ":assets:/audio/sfx/ui/button_click.wav");
        uiHover = assets.registerAsset(AssetType::Sound, ":assets:/audio/sfx/ui/menu_hover.wav");

        // Player sounds
        playerJump = assets.registerAsset(AssetType::Sound, ":assets:/audio/sfx/player/jump.wav");
        playerLand = assets.registerAsset(AssetType::Sound, ":assets:/audio/sfx/player/land.wav");

        // Music
        menuTheme = assets.registerAsset(AssetType::Sound, ":assets:/audio/music/menu_theme.ogg");
        battleTheme = assets.registerAsset(AssetType::Sound, ":assets:/audio/music/battle_theme.ogg");

        // Load critical sounds immediately
        assets.loadAsset(uiClick);
        assets.loadAsset(playerJump);

        // Load music asynchronously
        assets.loadAssetAsync(menuTheme, onMusicLoaded);
    }

    AssetHandle uiClick, uiHover;
    AssetHandle playerJump, playerLand;
    AssetHandle menuTheme, battleTheme;
};
```

### Memory Management

#### Sound Caching

The audio system automatically caches loaded sounds for reuse:

```cpp
// First play loads from AssetSystem into FMOD
audio->playOnChannel(Channels::UI, ChannelSound{.asset = clickSound});

// Second play reuses cached FMOD sound (no reload)
audio->playOnChannel(Channels::UI, ChannelSound{.asset = clickSound});
```

The cache is cleared when:
- Audio system shuts down
- `invalidateSoundCache()` is called (e.g., after hot reload)

#### Unloading Sounds

To free memory, unload sounds through the AssetSystem:

```cpp
assets->unloadAsset(soundAsset);  // Frees file data
audio->invalidateSoundCache();    // Frees FMOD sound objects
```

### Audio Mixing

#### Volume Hierarchy

Audio volumes are multiplicative:

```
Final Volume = Master × Group × Channel × Distance Attenuation
```

**Example:**
```cpp
audio->setMasterVolume(0.8f);        // 80% master
audio->setGroupVolume("sfx", 0.5f);  // 50% group
audio->assignChannelToGroup(Channels::UI, "sfx");
audio->setChannelVolume(Channels::UI, 0.6f);  // 60% channel

// Final volume: 0.8 × 0.5 × 0.6 = 0.24 (24%)
```

#### Ducking Music During Dialogue

```cpp
void startDialogue(AssetHandle voiceAsset) {
    // Lower music volume
    audio->setChannelVolume(Channels::Music, 0.3f);

    // Play dialogue
    audio->playOnChannel(Channels::Voice, ChannelSound{.asset = voiceAsset});
}

void endDialogue() {
    // Restore music volume
    audio->setChannelVolume(Channels::Music, 1.0f);
}
```

#### Crossfading Music

```cpp
void crossfadeMusic(AssetHandle newMusic, float duration) {
    // TODO: Fade-out is not yet implemented
    // Current workaround: immediate switch

    audio->stopChannel(Channels::Music);
    audio->playOnChannel(Channels::Music, ChannelSound{
        .asset = newMusic,
        .looping = true,
        .fadeInTime = duration  // TODO: Not yet implemented
    });
}
```

### Performance Tips

#### Limit Simultaneous Sounds

FMOD has a channel limit (512 by default). Prioritize important sounds:

```cpp
// Don't do this:
for (int i = 0; i < 1000; i++) {
    audio->playPositional(bulletSound);  // Will hit channel limit!
}

// Do this instead:
if (activeSounds < 100) {  // Custom limit
    audio->playPositional(bulletSound);
    activeSounds++;
}
```

#### Distance Culling

Don't play sounds that are too far away:

```cpp
void playEntitySound(Vec3 entityPos, AssetHandle sound) {
    Vec3 listenerPos = audio->getListener().position;
    float distance = glm::length(entityPos - listenerPos);

    if (distance < 150.0f) {  // Max audible distance
        audio->playPositional(PositionalSound{
            .asset = sound,
            .position = entityPos,
            .maxDistance = 100.0f
        });
    }
}
```

#### Avoid Frequent Position Updates

Only update positional sound positions when they move significantly:

```cpp
void updateMovingSound(SoundHandle handle, Vec3 newPos) {
    float distMoved = glm::length(newPos - lastSoundPosition);

    if (distMoved > 1.0f) {  // Update every 1 unit of movement
        audio->updatePositionalPosition(handle, newPos);
        lastSoundPosition = newPos;
    }
}
```

#### Use Appropriate File Formats

| Use Case | Recommended Format | Why |
|----------|-------------------|-----|
| UI sounds (clicks, beeps) | WAV (16-bit, 22kHz) | Instant playback, small files |
| Short SFX (<5s) | WAV (16-bit, 44kHz) | No decode overhead |
| Looping ambience | OGG Vorbis | Good compression, seamless loops |
| Background music | OGG Vorbis | Small file size, good quality |
| Dialogue | OGG Vorbis | Balance of quality and size |

---

## Code Examples

### Example 1: Simple Background Music

```cpp
class MusicPlayer {
public:
    MusicPlayer(IAudioSystem& audio, IAssetSystem& assets)
        : audio_(&audio), assets_(&assets) {}

    void init() {
        // Load music tracks
        menuMusic_ = assets_->registerAsset(AssetType::Sound, ":assets:/music/menu.ogg");
        gameMusic_ = assets_->registerAsset(AssetType::Sound, ":assets:/music/game.ogg");

        assets_->loadAsset(menuMusic_);
        assets_->loadAsset(gameMusic_);
    }

    void playMenuMusic() {
        ChannelSound music{
            .asset = menuMusic_,
            .volume = 0.7f,
            .looping = true
        };
        audio_->playOnChannel(Channels::Music, music);
    }

    void playGameMusic() {
        ChannelSound music{
            .asset = gameMusic_,
            .volume = 0.6f,
            .looping = true
        };
        audio_->playOnChannel(Channels::Music, music);
    }

    void stopMusic() {
        audio_->stopChannel(Channels::Music);
    }

private:
    IAudioSystem* audio_;
    IAssetSystem* assets_;
    AssetHandle menuMusic_;
    AssetHandle gameMusic_;
};
```

### Example 2: Player Footsteps with Variation

```cpp
class FootstepSystem {
public:
    FootstepSystem(IAudioSystem& audio, IAssetSystem& assets)
        : audio_(&audio), assets_(&assets) {}

    void init() {
        // Load multiple footstep variations
        for (int i = 0; i < 5; i++) {
            std::string path = std::format(":assets:/sfx/footstep_{:02d}.wav", i);
            AssetHandle asset = assets_->registerAsset(AssetType::Sound, path);
            assets_->loadAsset(asset);
            footsteps_.push_back(asset);
        }
    }

    void playFootstep(Vec3 playerPosition) {
        // Random variation
        int index = rand() % footsteps_.size();

        // Slight random pitch variation for more realism
        float pitchVariation = 0.9f + (rand() % 20) / 100.0f;  // 0.9 to 1.1

        PositionalSound footstep{
            .asset = footsteps_[index],
            .position = playerPosition,
            .volume = 0.5f,
            .pitch = pitchVariation,
            .minDistance = 2.0f,
            .maxDistance = 30.0f
        };

        audio_->playPositional(footstep);
    }

private:
    IAudioSystem* audio_;
    IAssetSystem* assets_;
    std::vector<AssetHandle> footsteps_;
};
```

### Example 3: Music Crossfade with State Machine

```cpp
enum class MusicState {
    Menu,
    Exploration,
    Combat,
    Victory
};

class AdaptiveMusicSystem {
public:
    AdaptiveMusicSystem(IAudioSystem& audio, IAssetSystem& assets)
        : audio_(&audio), assets_(&assets) {}

    void init() {
        // Load all music tracks
        tracks_[MusicState::Menu] = loadMusic("menu_theme.ogg");
        tracks_[MusicState::Exploration] = loadMusic("exploration.ogg");
        tracks_[MusicState::Combat] = loadMusic("combat.ogg");
        tracks_[MusicState::Victory] = loadMusic("victory.ogg");

        // Start with menu music
        transitionTo(MusicState::Menu);
    }

    void transitionTo(MusicState newState) {
        if (currentState_ == newState) return;

        currentState_ = newState;

        // TODO: Proper crossfade when implemented
        // For now: immediate transition
        ChannelSound music{
            .asset = tracks_[newState],
            .volume = 0.7f,
            .looping = true
        };
        audio_->playOnChannel(Channels::Music, music);
    }

    void update(float deltaTime) {
        // Example: Auto-transition based on game state
        if (isInCombat() && currentState_ != MusicState::Combat) {
            transitionTo(MusicState::Combat);
        } else if (!isInCombat() && currentState_ == MusicState::Combat) {
            transitionTo(MusicState::Exploration);
        }
    }

private:
    AssetHandle loadMusic(const std::string& filename) {
        std::string path = ":assets:/music/" + filename;
        AssetHandle asset = assets_->registerAsset(AssetType::Sound, path);
        assets_->loadAsset(asset);
        return asset;
    }

    bool isInCombat() {
        // Game-specific logic
        return false;
    }

    IAudioSystem* audio_;
    IAssetSystem* assets_;
    MusicState currentState_ = MusicState::Menu;
    std::map<MusicState, AssetHandle> tracks_;
};
```

### Example 4: 3D Ambient Soundscape

```cpp
class AmbientSoundManager {
public:
    AmbientSoundManager(IAudioSystem& audio, IAssetSystem& assets)
        : audio_(&audio), assets_(&assets) {}

    void init() {
        // Load ambient sounds
        windAsset_ = assets_->registerAsset(AssetType::Sound, ":assets:/ambient/wind.ogg");
        waterAsset_ = assets_->registerAsset(AssetType::Sound, ":assets:/ambient/water.ogg");
        birdsAsset_ = assets_->registerAsset(AssetType::Sound, ":assets:/ambient/birds.ogg");

        assets_->loadAsset(windAsset_);
        assets_->loadAsset(waterAsset_);
        assets_->loadAsset(birdsAsset_);
    }

    void createForest(Vec3 centerPosition) {
        // Wind - covers large area
        PositionalSound wind{
            .asset = windAsset_,
            .position = centerPosition,
            .volume = 0.3f,
            .minDistance = 50.0f,
            .maxDistance = 200.0f
        };
        windHandle_ = audio_->playPositional(wind);

        // Water - near river
        PositionalSound water{
            .asset = waterAsset_,
            .position = centerPosition + Vec3{30.0f, 0.0f, 0.0f},
            .volume = 0.5f,
            .minDistance = 10.0f,
            .maxDistance = 50.0f
        };
        waterHandle_ = audio_->playPositional(water);

        // Birds - scattered around
        for (int i = 0; i < 3; i++) {
            Vec3 offset{
                (rand() % 60) - 30.0f,
                (rand() % 20) - 10.0f,
                (rand() % 60) - 30.0f
            };

            PositionalSound birds{
                .asset = birdsAsset_,
                .position = centerPosition + offset,
                .volume = 0.4f,
                .minDistance = 15.0f,
                .maxDistance = 80.0f
            };
            birdHandles_.push_back(audio_->playPositional(birds));
        }
    }

    void cleanup() {
        audio_->stopPositional(windHandle_);
        audio_->stopPositional(waterHandle_);
        for (auto handle : birdHandles_) {
            audio_->stopPositional(handle);
        }
        birdHandles_.clear();
    }

private:
    IAudioSystem* audio_;
    IAssetSystem* assets_;
    AssetHandle windAsset_, waterAsset_, birdsAsset_;
    SoundHandle windHandle_, waterHandle_;
    std::vector<SoundHandle> birdHandles_;
};
```

### Example 5: UI Sound Manager

```cpp
class UISoundManager {
public:
    UISoundManager(IAudioSystem& audio, IAssetSystem& assets)
        : audio_(&audio), assets_(&assets) {}

    void init() {
        // Load UI sounds
        clickSound_ = assets_->registerAsset(AssetType::Sound, ":assets:/ui/click.wav");
        hoverSound_ = assets_->registerAsset(AssetType::Sound, ":assets:/ui/hover.wav");
        errorSound_ = assets_->registerAsset(AssetType::Sound, ":assets:/ui/error.wav");

        assets_->loadAsset(clickSound_);
        assets_->loadAsset(hoverSound_);
        assets_->loadAsset(errorSound_);

        // Create UI group for volume control
        audio_->setGroupVolume("ui", 0.7f);
        audio_->assignChannelToGroup(Channels::UI, "ui");
    }

    void playClick() {
        ChannelSound sound{.asset = clickSound_, .volume = 0.8f};
        audio_->playOnChannel(Channels::UI, sound);
    }

    void playHover() {
        ChannelSound sound{.asset = hoverSound_, .volume = 0.5f};
        audio_->playOnChannel(Channels::UI, sound);
    }

    void playError() {
        ChannelSound sound{.asset = errorSound_, .volume = 1.0f, .pitch = 0.9f};
        audio_->playOnChannel(Channels::UI, sound);
    }

    void setUIVolume(float volume) {
        audio_->setGroupVolume("ui", volume);
    }

private:
    IAudioSystem* audio_;
    IAssetSystem* assets_;
    AssetHandle clickSound_, hoverSound_, errorSound_;
};
```

### Example 6: Complete Game Integration

```cpp
class MyPlatformerGame : public IApplication {
public:
    MyPlatformerGame(IAudioSystem& audio, IAssetSystem& assets, IEntitySystem& entities)
        : audio_(&audio), assets_(&assets), entities_(&entities) {}

    void run() override {
        // Initialize audio
        if (!audio_->initialize()) {
            std::cerr << "Failed to initialize audio\n";
            return;
        }

        // Load sounds
        jumpSound_ = assets_->registerAsset(AssetType::Sound, ":assets:/sfx/jump.wav");
        coinSound_ = assets_->registerAsset(AssetType::Sound, ":assets:/sfx/coin.wav");
        musicAsset_ = assets_->registerAsset(AssetType::Sound, ":assets:/music/level1.ogg");

        assets_->loadAsset(jumpSound_);
        assets_->loadAsset(coinSound_);
        assets_->loadAsset(musicAsset_);

        // Start background music
        ChannelSound music{.asset = musicAsset_, .volume = 0.6f, .looping = true};
        audio_->playOnChannel(Channels::Music, music);

        // Game loop
        while (running_) {
            float deltaTime = calculateDeltaTime();
            update(deltaTime);
            render();
        }

        // Cleanup
        audio_->stopAll();
        audio_->shutdown();
    }

    void update(float deltaTime) {
        // CRITICAL: Update audio system every frame
        audio_->update(deltaTime);

        // Update listener position (follow player)
        Vec3 playerPos = getPlayerPosition();
        AudioListener listener{
            .position = playerPos,
            .forward = Vec3{0.0f, 0.0f, -1.0f},
            .up = Vec3{0.0f, 1.0f, 0.0f}
        };
        audio_->setListener(listener);

        // Handle game events
        processGameEvents();
    }

    void onPlayerJump(Vec3 position) {
        PositionalSound jump{
            .asset = jumpSound_,
            .position = position,
            .volume = 0.7f,
            .minDistance = 5.0f,
            .maxDistance = 50.0f
        };
        audio_->playPositional(jump);
    }

    void onCoinCollected(Vec3 coinPosition) {
        PositionalSound coin{
            .asset = coinSound_,
            .position = coinPosition,
            .pitch = 1.0f + (rand() % 20) / 100.0f,  // Slight variation
            .minDistance = 3.0f,
            .maxDistance = 30.0f
        };
        audio_->playPositional(coin);
    }

    void onPauseToggle() {
        if (paused_) {
            audio_->resumeAll();
        } else {
            audio_->pauseAll();
        }
        paused_ = !paused_;
    }

private:
    Vec3 getPlayerPosition() {
        // Get player entity position
        return Vec3{0.0f};  // Placeholder
    }

    void processGameEvents() {
        // Process game logic and trigger audio events
    }

    float calculateDeltaTime() {
        // Calculate time since last frame
        return 0.016f;  // ~60 FPS
    }

    void render() {
        // Render game
    }

    IAudioSystem* audio_;
    IAssetSystem* assets_;
    IEntitySystem* entities_;

    AssetHandle jumpSound_;
    AssetHandle coinSound_;
    AssetHandle musicAsset_;

    bool running_ = true;
    bool paused_ = false;
};
```

---

## Additional Resources

### FMOD Documentation

For advanced usage, consult the FMOD Core API documentation:
- [FMOD Core API Guide](https://fmod.com/docs/2.02/api/core-guide.html)
- [FMOD Core API Reference](https://fmod.com/docs/2.02/api/core-api.html)

### Bestow System Integration

Related Bestow systems:
- **AssetSystem** - See `ASSET-SYSTEM.md` for asset loading and hot-reload
- **EntitySystem** - See `ENTITY-SYSTEM.md` for attaching sounds to entities
- **EventSystem** - See `EVENT-SYSTEM.md` for event-driven audio triggers

### Common Issues

#### No Sound Playing

1. Check FMOD initialization: `audio->initialize()` returned true?
2. Check asset loading: `assets->getAsset<SoundData>(handle)` returns valid data?
3. Check volume: Is master/group/channel volume > 0?
4. Check FMOD: Is FMOD installed and linked correctly?

#### Sounds Stop Abruptly

1. Call `audio->update()` every frame (required!)
2. Check if channel limit (512) is reached
3. Check if sounds are being stopped externally

#### 3D Audio Not Working

1. Ensure listener is set: `audio->setListener(...)`
2. Update listener position every frame
3. Check min/max distance values are reasonable
4. Verify sound is played with `playPositional()`, not `playOnChannel()`

#### Memory Leaks

1. Call `audio->shutdown()` on exit
2. Unload unused assets: `assets->unloadAsset(handle)`
3. Stop all sounds before shutdown: `audio->stopAll()`

---

**Happy audio programming!**

For questions or issues, consult the Bestow documentation or FMOD Core API reference.

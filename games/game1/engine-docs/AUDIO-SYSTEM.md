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

- **Channel-based audio** for music, ambience, UI, and voice (managed playback)
- **Positional 3D audio** with distance attenuation (fire-and-forget sound effects)
- **Master volume** and global pause/resume/stop controls
- **Channel groups** for categorical volume control
- **Asset system integration** for loading sounds
- **3D listener** for spatial audio positioning

### Key Concepts

**Channels** (Managed Playback):
- Pre-defined channels: Music (0), Ambience (1), UI (2), Voice (3)
- One sound per channel at a time
- Full control: pause, resume, seek, volume, pitch
- Can be assigned to channel groups for categorical volume control
- Use for: background music, UI sounds, narration, ambient loops

**Positional Sounds** (3D Sound Effects):
- Play many simultaneously
- Automatic 3D spatialization based on distance and listener position
- Fire-and-forget (or track with returned handle)
- Use for: footsteps, gunshots, explosions, entity sounds

---

## Core Concepts

### Channels vs Positional Sounds

The audio system provides two playback models:

#### Channel-Based Audio (Managed Playback)

**Best for:** Background music, UI sounds, narration, ambient loops

```cpp
// Pre-defined channels (use these constants)
Channels::Music      // Channel 0 - Background music tracks
Channels::Ambience   // Channel 1 - Ambient soundscapes
Channels::UI         // Channel 2 - Button clicks, menu sounds
Channels::Voice      // Channel 3 - Dialogue and narration
```

**Example:**
```cpp
ChannelSound music{
    .asset = musicAsset,
    .volume = 0.7f,
    .looping = true
};
audio->playOnChannel(Channels::Music, music);
```

Characteristics:
- **One sound per channel** - Playing a new sound on a channel stops the previous sound
- **Persistent control** - Pause, resume, seek, volume, pitch can be changed after playback starts
- **Global control** - Affected by `pauseAll()`, `resumeAll()`, `stopAll()`
- **Query state** - Check if playing, paused, position, etc. via `getChannelState()`

#### Positional Audio (3D Sound Effects)

**Best for:** Sound effects tied to game entities, footsteps, gunshots, explosions

```cpp
PositionalSound explosion{
    .asset = explosionAsset,
    .position = Vec3{100.0f, 50.0f, 0.0f},
    .volume = 1.0f,
    .minDistance = 10.0f,   // Full volume within this radius
    .maxDistance = 200.0f   // Silent beyond this radius
};
SoundHandle handle = audio->playPositional(explosion);
```

Characteristics:
- **Multiple simultaneous sounds** - Can play many at once
- **3D spatialization** - Volume and panning based on distance and direction from listener
- **Fire-and-forget** - Typically plays once and cleans up automatically
- **Optional tracking** - Update position with `updatePositionalPosition(handle, newPos)`
- **Check state** - Query if still playing with `isPositionalPlaying(handle)`

### 3D Audio Listener

The listener represents the player's "ears" in the game world. Update it every frame:

```cpp
AudioListener listener{
    .position = cameraPosition,      // Where the listener is
    .forward = cameraForward,        // Which direction they're facing
    .up = Vec3{0.0f, 1.0f, 0.0f},   // Up direction (usually Y-up)
    .velocity = cameraVelocity       // Optional: for Doppler effect
};
audio->setListener(listener);
```

The listener position determines how positional sounds are heard (volume, panning, distance attenuation).

---

## Getting Started

### Basic Usage

The audio system is injected via dependency injection. Here's a minimal example:

```cpp
import std;
import bestow;
import bestow.core;

class MyGame : public bestow::core::Application {
public:
    bool initialize(bestow::core::Engine& engine) override {
        engine_ = &engine;
        auto& sys = engine.systems();

        // Load a sound asset
        AssetHandle musicAsset = sys.assets->registerAsset(
            AssetType::Sound,
            ":assets:/music/theme.ogg"
        );
        sys.assets->loadAsset(musicAsset);

        // Play background music on the Music channel
        ChannelSound music{
            .asset = musicAsset,
            .volume = 0.7f,
            .looping = true
        };
        sys.audio->playOnChannel(Channels::Music, music);

        return true;
    }

    void updateFixed(DeltaTime dt) override {
        auto& sys = engine_->systems();

        // IMPORTANT: Call update() every frame for FMOD to process
        sys.audio->update(dt);

        handleInput();
        // ... other updates
    }

    void shutdown() override {
        auto& sys = engine_->systems();
        sys.audio->stopAll();
    }

private:
    bestow::core::Engine* engine_ = nullptr;
};
```

### Important Notes

- **Call `update(dt)` every frame** - The audio system requires regular updates for FMOD to process events
- **Use AssetSystem for loading** - Always load sounds through `IAssetSystem`
- **Call `stopAll()` on exit** - Clean up playing sounds before shutdown

---

## API Reference

### Lifecycle

```cpp
bool initialize();
void shutdown();
void update(DeltaTime dt);
```

**IMPORTANT:** Call `update(dt)` every frame in your game loop. This processes FMOD events, updates streaming, and handles internal bookkeeping.

### Channel-Based Audio

#### Play Sound on Channel

```cpp
void playOnChannel(Channel channel, const ChannelSound& sound);
```

Plays a sound on the specified channel. If another sound is already playing on that channel, it stops immediately.

**Parameters:**
```cpp
struct ChannelSound {
    AssetHandle asset;         // Sound asset from AssetSystem
    Volume volume = 1.0f;      // 0.0 = silent, 1.0 = full, >1.0 = amplified
    float pitch = 1.0f;        // 1.0 = normal, 2.0 = double speed, 0.5 = half speed
    bool looping = false;      // true = loop forever, false = play once
    float fadeInTime = 0.0f;   // Seconds to fade in from silence
    std::optional<float> startTime;  // Optional: Start at specific time offset
};
```

**Example:**
```cpp
AssetHandle musicAsset = sys.assets->registerAsset(
    AssetType::Sound, ":assets:/music/battle_theme.ogg"
);
sys.assets->loadAsset(musicAsset);

ChannelSound music{
    .asset = musicAsset,
    .volume = 0.7f,
    .looping = true,
    .fadeInTime = 2.0f  // Fade in over 2 seconds
};
sys.audio->playOnChannel(Channels::Music, music);
```

#### Stop Channel

```cpp
void stopChannel(Channel channel, float fadeOutTime = 0.0f);
```

Stops playback on the specified channel. Optionally fades out over the specified time.

**Parameters:**
- `channel` - The channel to stop (Channels::Music, etc.)
- `fadeOutTime` - Seconds to fade out (default: 0.0 = immediate stop)

**Example:**
```cpp
sys.audio->stopChannel(Channels::Music);              // Stop immediately
sys.audio->stopChannel(Channels::Music, 3.0f);        // Fade out over 3 seconds
```

#### Pause/Resume Channel

```cpp
void pauseChannel(Channel channel);
void resumeChannel(Channel channel);
```

Pauses or resumes a specific channel without stopping it. Playback position is preserved.

**Example:**
```cpp
sys.audio->pauseChannel(Channels::Music);   // Pause music
// ... later ...
sys.audio->resumeChannel(Channels::Music);  // Resume from where it paused
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
sys.audio->setChannelVolume(Channels::Music, 0.5f);  // 50% volume
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
sys.audio->setChannelPitch(Channels::Voice, 0.8f);  // Slow, deep voice
sys.audio->setChannelPitch(Channels::Voice, 1.5f);  // Fast, high voice
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
sys.audio->seekChannel(Channels::Music, 30.0f);  // Jump to 30 seconds
sys.audio->seekChannel(Channels::Music, 0.0f);   // Rewind to start
```

#### Channel State

```cpp
ChannelState getChannelState(Channel channel) const;
bool isChannelPlaying(Channel channel) const;
```

Query the current state of a channel.

**ChannelState struct:**
```cpp
struct ChannelState {
    bool isPlaying = false;  // Currently playing (not paused/stopped)
    bool isPaused = false;   // Currently paused
    float position = 0.0f;   // Current playback position (seconds)
    float length = 0.0f;     // Total sound length (seconds)
    Volume volume = 1.0f;    // Current volume
};
```

**Example:**
```cpp
// Simple check
if (sys.audio->isChannelPlaying(Channels::Music)) {
    std::cout << "Music is playing\n";
}

// Detailed state
auto state = sys.audio->getChannelState(Channels::Music);
if (state.isPlaying) {
    float progress = state.position / state.length;
    std::cout << "Music: " << (progress * 100.0f) << "% complete\n";
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
    AssetHandle asset;          // Sound asset
    Vec3 position{0.0f};        // World position of sound source
    Volume volume = 1.0f;       // Base volume before distance attenuation
    float pitch = 1.0f;         // Pitch multiplier
    float minDistance = 1.0f;   // Full volume within this radius
    float maxDistance = 100.0f; // Silent beyond this radius
    std::optional<Vec3> velocity; // Optional: for Doppler effect
    std::function<void()> onComplete; // Optional: Callback when finished
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
SoundHandle handle = sys.audio->playPositional(explosion);
```

#### Stop Positional Sound

```cpp
void stopPositional(SoundHandle handle);
```

Stops a specific positional sound before it finishes naturally.

**Example:**
```cpp
SoundHandle loopingEngine = sys.audio->playPositional(engineSound);
// ... later ...
sys.audio->stopPositional(loopingEngine);  // Stop the engine sound
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
    sys.audio->updatePositionalPosition(entitySoundHandle, newPos);
}
```

#### Check Positional Status

```cpp
bool isPositionalPlaying(SoundHandle handle) const;
```

Checks if a positional sound is still playing.

**Example:**
```cpp
if (sys.audio->isPositionalPlaying(footstepHandle)) {
    // Footstep sound is still playing, don't play another yet
} else {
    // Safe to play next footstep
}
```

### 3D Audio Listener

#### Set Listener

```cpp
void setListener(const AudioListener& listener);
AudioListener getListener() const;
```

Sets the listener's position and orientation. This should match your camera or player position. Update every frame for proper 3D audio spatialization.

**Parameters:**
```cpp
struct AudioListener {
    Vec3 position{0.0f};             // Listener position in world space
    Vec3 forward{0.0f, 0.0f, -1.0f}; // Forward direction (normalized)
    Vec3 up{0.0f, 1.0f, 0.0f};       // Up direction (normalized)
    Vec3 velocity{0.0f};             // Optional: for Doppler effect
};
```

**Example:**
```cpp
// Update listener every frame to match camera/player
void updateAudio(const Camera& camera) {
    AudioListener listener{
        .position = camera.position,
        .forward = camera.forward,
        .up = camera.up
    };
    sys.audio->setListener(listener);
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
sys.audio->setMasterVolume(0.0f);   // Mute everything
sys.audio->setMasterVolume(0.5f);   // 50% volume
sys.audio->setMasterVolume(1.0f);   // Full volume
```

#### Pause/Resume/Stop All

```cpp
void pauseAll();
void resumeAll();
void stopAll();
```

Global controls affecting all active sounds (both channels and positional).

**Example:**
```cpp
void togglePause() {
    if (isPaused_) {
        sys.audio->resumeAll();
    } else {
        sys.audio->pauseAll();
    }
    isPaused_ = !isPaused_;
}

void returnToMainMenu() {
    sys.audio->stopAll();  // Stop everything (music, SFX, etc.)
}
```

### Channel Groups

Channel groups allow you to control volume for categories of sounds (e.g., "Music", "SFX", "Voice").

#### Set Group Volume

```cpp
void setGroupVolume(const std::string& group, Volume volume);
```

Sets the volume for all channels assigned to a group.

**Example:**
```cpp
// Create groups and set volumes
sys.audio->setGroupVolume("Music", 0.7f);
sys.audio->setGroupVolume("SFX", 0.9f);
sys.audio->setGroupVolume("Voice", 1.0f);
```

#### Assign Channel to Group

```cpp
void assignChannelToGroup(Channel channel, const std::string& group);
```

Assigns a channel to a group for categorical volume control.

**Example:**
```cpp
// Organize channels into groups
sys.audio->assignChannelToGroup(Channels::Music, "Music");
sys.audio->assignChannelToGroup(Channels::Ambience, "Music");
sys.audio->assignChannelToGroup(Channels::UI, "SFX");
sys.audio->assignChannelToGroup(Channels::Voice, "Voice");

// Now volume controls affect entire categories
sys.audio->setGroupVolume("Music", 0.5f);  // Affects both Music and Ambience channels
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
AssetHandle soundAsset = sys.assets->registerAsset(
    AssetType::Sound,
    ":assets:/audio/jump.wav"
);

// Load it (sync or async)
sys.assets->loadAsset(soundAsset);

// Use it
ChannelSound sound{.asset = soundAsset};
sys.audio->playOnChannel(Channels::UI, sound);
```

#### Async Loading

```cpp
AssetHandle musicAsset = sys.assets->registerAsset(
    AssetType::Sound,
    ":assets:/music/battle_theme.ogg"
);

sys.assets->loadAssetAsync(musicAsset, [this](AssetHandle handle, AssetState state) {
    if (state == AssetState::Loaded) {
        std::cout << "Music loaded! Ready to play.\n";
        ChannelSound music{.asset = handle, .looping = true};
        sys.audio->playOnChannel(Channels::Music, music);
    }
});
```

#### Hot Reload Support

```cpp
// Subscribe to asset changes
SubscriptionId subId = sys.assets->subscribe(soundAsset, [this](AssetHandle handle, AssetType type) {
    std::cout << "Sound was reloaded from disk!\n";
    // Audio system automatically invalidates cache
    // Next playback will use the new audio data
});

// Enable hot reload monitoring
sys.assets->enableHotReload(true);
```

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

### Listener Orientation

The listener's `forward` and `up` vectors determine the orientation for 3D audio spatialization:

```cpp
AudioListener listener{
    .position = Vec3{0.0f, 0.0f, 0.0f},
    .forward = Vec3{1.0f, 0.0f, 0.0f},   // Facing right
    .up = Vec3{0.0f, 1.0f, 0.0f}         // Standard up (Y-up)
};
sys.audio->setListener(listener);
```

**Important:** Update the listener every frame to match your camera or player position for accurate 3D audio.

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
        uiClick_ = assets.registerAsset(AssetType::Sound, ":assets:/audio/sfx/ui/button_click.wav");
        uiHover_ = assets.registerAsset(AssetType::Sound, ":assets:/audio/sfx/ui/menu_hover.wav");

        // Player sounds
        playerJump_ = assets.registerAsset(AssetType::Sound, ":assets:/audio/sfx/player/jump.wav");
        playerLand_ = assets.registerAsset(AssetType::Sound, ":assets:/audio/sfx/player/land.wav");

        // Music
        menuTheme_ = assets.registerAsset(AssetType::Sound, ":assets:/audio/music/menu_theme.ogg");
        battleTheme_ = assets.registerAsset(AssetType::Sound, ":assets:/audio/music/battle_theme.ogg");

        // Load critical sounds immediately
        assets.loadAsset(uiClick_);
        assets.loadAsset(playerJump_);

        // Load music asynchronously
        assets.loadAssetAsync(menuTheme_, [](AssetHandle, AssetState) {});
    }

    AssetHandle uiClick() const { return uiClick_; }
    AssetHandle playerJump() const { return playerJump_; }
    // ... getters for other sounds

private:
    AssetHandle uiClick_, uiHover_;
    AssetHandle playerJump_, playerLand_;
    AssetHandle menuTheme_, battleTheme_;
};
```

### Audio Mixing

#### Volume Hierarchy

Audio volumes are multiplicative:

```
Final Volume = Master × Group × Channel × Distance Attenuation (for positional)
```

**Example:**
```cpp
sys.audio->setMasterVolume(0.8f);                   // 80% master
sys.audio->setGroupVolume("Music", 0.75f);          // 75% group
sys.audio->setChannelVolume(Channels::Music, 0.6f); // 60% channel

// Music plays at: 0.8 × 0.75 × 0.6 = 0.36 (36% volume)
```

#### Ducking Music During Dialogue

```cpp
void startDialogue(AssetHandle voiceAsset) {
    // Lower music volume
    sys.audio->setChannelVolume(Channels::Music, 0.3f);

    // Play dialogue
    sys.audio->playOnChannel(Channels::Voice, ChannelSound{.asset = voiceAsset});
}

void endDialogue() {
    // Restore music volume
    sys.audio->setChannelVolume(Channels::Music, 1.0f);
}
```

#### Smooth Music Transitions

```cpp
void changeMusic(AssetHandle newMusic) {
    // Fade out current music
    sys.audio->stopChannel(Channels::Music, 2.0f);  // 2 second fade out

    // Wait for fade out to complete (in practice, use a timer)
    // Then play new music with fade in
    ChannelSound music{
        .asset = newMusic,
        .volume = 0.7f,
        .looping = true,
        .fadeInTime = 2.0f  // 2 second fade in
    };
    sys.audio->playOnChannel(Channels::Music, music);
}
```

### Performance Tips

#### Limit Simultaneous Sounds

FMOD has a channel limit (512 by default). Prioritize important sounds:

```cpp
// Don't do this:
for (int i = 0; i < 1000; i++) {
    sys.audio->playPositional(bulletSound);  // Will hit channel limit!
}

// Do this instead:
if (activeSounds_ < 100) {  // Custom limit
    sys.audio->playPositional(bulletSound);
    activeSounds_++;
}
```

#### Distance Culling

Don't play sounds that are too far away:

```cpp
void playEntitySound(Vec3 entityPos, AssetHandle sound) {
    Vec3 listenerPos = sys.audio->getListener().position;
    float distance = glm::length(entityPos - listenerPos);

    if (distance < 150.0f) {  // Max audible distance
        sys.audio->playPositional(PositionalSound{
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
    float distMoved = glm::length(newPos - lastSoundPosition_);

    if (distMoved > 1.0f) {  // Update every 1 unit of movement
        sys.audio->updatePositionalPosition(handle, newPos);
        lastSoundPosition_ = newPos;
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
            .looping = true,
            .fadeInTime = 1.5f
        };
        audio_->playOnChannel(Channels::Music, music);
    }

    void playGameMusic() {
        // Fade out menu music
        audio_->stopChannel(Channels::Music, 2.0f);

        // Play game music after delay
        ChannelSound music{
            .asset = gameMusic_,
            .volume = 0.6f,
            .looping = true,
            .fadeInTime = 2.0f
        };
        audio_->playOnChannel(Channels::Music, music);
    }

    void stopMusic() {
        audio_->stopChannel(Channels::Music, 1.0f);  // Fade out over 1 second
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

### Example 3: 3D Ambient Soundscape

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

### Example 4: UI Sound Manager with Channel Groups

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

        // Set up channel group for UI sounds
        audio_->assignChannelToGroup(Channels::UI, "UI");
        audio_->setGroupVolume("UI", 0.7f);
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
        audio_->setGroupVolume("UI", volume);
    }

private:
    IAudioSystem* audio_;
    IAssetSystem* assets_;
    AssetHandle clickSound_, hoverSound_, errorSound_;
};
```

### Example 5: Complete Game Integration

```cpp
class MyPlatformerGame : public bestow::core::Application {
public:
    bool initialize(bestow::core::Engine& engine) override {
        engine_ = &engine;
        auto& sys = engine.systems();

        // Load sounds
        jumpSound_ = sys.assets->registerAsset(AssetType::Sound, ":assets:/sfx/jump.wav");
        coinSound_ = sys.assets->registerAsset(AssetType::Sound, ":assets:/sfx/coin.wav");
        musicAsset_ = sys.assets->registerAsset(AssetType::Sound, ":assets:/music/level1.ogg");

        sys.assets->loadAsset(jumpSound_);
        sys.assets->loadAsset(coinSound_);
        sys.assets->loadAsset(musicAsset_);

        // Set up channel groups
        sys.audio->assignChannelToGroup(Channels::Music, "Music");
        sys.audio->assignChannelToGroup(Channels::Ambience, "Music");
        sys.audio->setGroupVolume("Music", 0.7f);

        // Start background music
        ChannelSound music{
            .asset = musicAsset_,
            .volume = 0.6f,
            .looping = true,
            .fadeInTime = 2.0f
        };
        sys.audio->playOnChannel(Channels::Music, music);

        return true;
    }

    void updateFixed(DeltaTime dt) override {
        auto& sys = engine_->systems();

        // CRITICAL: Update audio system every frame
        sys.audio->update(dt);

        // Update listener position every frame (follow player)
        Vec3 playerPos = getPlayerPosition();
        AudioListener listener{
            .position = playerPos,
            .forward = Vec3{0.0f, 0.0f, -1.0f},
            .up = Vec3{0.0f, 1.0f, 0.0f}
        };
        sys.audio->setListener(listener);

        // Handle game events
        processGameEvents();
    }

    void onPlayerJump(Vec3 position) {
        auto& sys = engine_->systems();

        PositionalSound jump{
            .asset = jumpSound_,
            .position = position,
            .volume = 0.7f,
            .minDistance = 5.0f,
            .maxDistance = 50.0f
        };
        sys.audio->playPositional(jump);
    }

    void onCoinCollected(Vec3 coinPosition) {
        auto& sys = engine_->systems();

        PositionalSound coin{
            .asset = coinSound_,
            .position = coinPosition,
            .pitch = 1.0f + (rand() % 20) / 100.0f,  // Slight variation
            .minDistance = 3.0f,
            .maxDistance = 30.0f
        };
        sys.audio->playPositional(coin);
    }

    void onPauseToggle() {
        auto& sys = engine_->systems();

        if (paused_) {
            sys.audio->resumeAll();
        } else {
            sys.audio->pauseAll();
        }
        paused_ = !paused_;
    }

    void shutdown() override {
        auto& sys = engine_->systems();
        sys.audio->stopAll();
    }

private:
    Vec3 getPlayerPosition() {
        // Get player entity position
        return Vec3{0.0f};  // Placeholder
    }

    void processGameEvents() {
        // Process game logic and trigger audio events
    }

    bestow::core::Engine* engine_ = nullptr;
    AssetHandle jumpSound_;
    AssetHandle coinSound_;
    AssetHandle musicAsset_;
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

1. Check `update()` is called: Audio system requires `update(dt)` every frame
2. Check asset loading: `assets->getAsset<SoundData>(handle)` returns valid data?
3. Check volume: Is master volume > 0? Is channel/group volume > 0?
4. Check asset path: Is the file in the correct location?
5. Check FMOD: Is FMOD properly installed and linked?

#### 3D Audio Not Working

1. Ensure listener is set: `audio->setListener(...)`
2. Update listener position every frame in your update loop
3. Check min/max distance values are reasonable (not too small/large)
4. Verify sound is played with `playPositional()`, not `playOnChannel()`
5. Check listener is in range of the sound (within maxDistance)

#### Channel Playback Issues

1. Only one sound per channel - new sounds replace old ones
2. Check you're using the correct channel constant (Channels::Music, etc.)
3. Verify asset is loaded before calling `playOnChannel()`
4. Use `isChannelPlaying()` to check current state

#### Memory Issues

1. Unload unused assets: `assets->unloadAsset(handle)`
2. Stop all sounds before shutdown: `audio->stopAll()`
3. Keep sound files reasonably sized (compress music to OGG)
4. Limit simultaneous positional sounds (FMOD has 512 channel limit)

---

**Happy audio programming!**

For questions or issues, consult the Bestow documentation or FMOD Core API reference.

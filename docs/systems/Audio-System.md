# JFrame Audio System

## Overview

The JFrame Audio System provides comprehensive audio playback capabilities through FMOD integration, supporting both channel-based audio management and positional 3D sound effects. The system is designed for game audio needs including music playback, sound effects, UI audio, and spatial audio for immersive gameplay.

### Key Features

- **Channel-based audio management**: Dedicated channels for music, ambience, UI, and voice
- **Positional 3D audio**: Spatial sound effects with distance attenuation and Doppler effect
- **Volume and pitch control**: Per-channel and per-sound audio manipulation
- **Channel groups**: Organize channels into groups with shared volume control
- **Looping and one-shot sounds**: Support for both continuous and single-play audio
- **Fade in/out support**: Smooth audio transitions (fade-in implemented, fade-out planned)
- **Hot reload support**: Dynamic audio asset reloading during development
- **Stub mode operation**: Works without FMOD for testing and headless environments

### Architecture

The Audio System follows the interface-based design pattern:

```cpp
jframe.audio           // Interface module (IAudioSystem)
jframe.audio.impl      // FMOD implementation (FMODAudioSystem)
```

The system integrates with:
- **Asset System**: Loads audio files via `IAssetSystem`
- **FMOD Core API**: Low-level audio engine for playback and 3D positioning

## Getting Started

### Initialization

```cpp
import jframe;
import jframe.audio.impl;

// Create and initialize the audio system
auto audio = createAudioSystem();
auto* fmodAudio = dynamic_cast<FMODAudioSystem*>(audio.get());
if (fmodAudio && !fmodAudio->initialize()) {
    std::cerr << "Failed to initialize audio system" << std::endl;
}
```

### Update Loop

The audio system must be updated every frame to process FMOD events:

```cpp
void gameLoop(DeltaTime dt) {
    audio->update(dt);
    // ... rest of game logic
}
```

## Core Types

### Channel

A `Channel` represents a dedicated audio playback slot. Channels are identified by `uint32_t` values:

```cpp
namespace Channels {
    inline constexpr Channel Music = 0;     // Background music
    inline constexpr Channel Ambience = 1;  // Environmental sounds
    inline constexpr Channel UI = 2;        // User interface sounds
    inline constexpr Channel Voice = 3;     // Character dialogue/voice
}

// Custom channels
constexpr Channel PlayerFootsteps = 10;
constexpr Channel EnemyAttacks = 11;
```

### ChannelSound

Defines a sound to play on a channel:

```cpp
struct ChannelSound {
    AssetHandle asset;              // Sound asset to play
    Volume volume = 1.0f;           // Volume (0.0 to 1.0+)
    float pitch = 1.0f;             // Pitch multiplier (1.0 = normal)
    bool looping = false;           // Loop continuously?
    float fadeInTime = 0.0f;        // Fade in duration (seconds)
    std::optional<float> startTime; // Start position (seconds)
};
```

### ChannelState

Represents the current state of a channel:

```cpp
struct ChannelState {
    bool isPlaying = false;   // Is audio currently playing?
    bool isPaused = false;    // Is audio paused?
    float position = 0.0f;    // Current playback position (seconds)
    float length = 0.0f;      // Total duration (seconds)
    Volume volume = 1.0f;     // Current volume
};
```

### PositionalSound

Defines a 3D sound effect:

```cpp
struct PositionalSound {
    AssetHandle asset;                   // Sound asset
    Vec3 position{0.0f};                 // 3D position
    Volume volume = 1.0f;                // Volume
    float pitch = 1.0f;                  // Pitch multiplier
    float minDistance = 1.0f;            // Full volume radius
    float maxDistance = 100.0f;          // Silence distance
    std::optional<Vec3> velocity;        // For Doppler effect
    std::function<void()> onComplete;    // Callback when finished
};
```

### AudioListener

Represents the 3D audio listener (typically the player/camera):

```cpp
struct AudioListener {
    Vec3 position{0.0f};              // Listener position
    Vec3 forward{0.0f, 0.0f, -1.0f};  // Forward direction
    Vec3 up{0.0f, 1.0f, 0.0f};        // Up direction
    Vec3 velocity{0.0f};              // For Doppler effect
};
```

## Playing Sounds

### One-Shot Sounds (Fire and Forget)

For sound effects that play once and don't need control:

```cpp
// Play a UI click sound
AssetHandle clickSound = assets->registerAsset(AssetType::Sound, "audio/ui_click.wav");
ChannelSound sound{
    .asset = clickSound,
    .volume = 0.8f
};
audio->playOnChannel(Channels::UI, sound);
```

### Looping Sounds

For continuous sounds like music or ambience:

```cpp
// Play background music on loop
AssetHandle musicAsset = assets->registerAsset(AssetType::Music, "audio/level1_music.ogg");
ChannelSound music{
    .asset = musicAsset,
    .volume = 0.6f,
    .looping = true
};
audio->playOnChannel(Channels::Music, music);
```

### Sound with Fade-In

Smoothly fade in audio over time:

```cpp
// Fade in ambience over 2 seconds
ChannelSound ambience{
    .asset = ambienceAsset,
    .volume = 0.7f,
    .looping = true,
    .fadeInTime = 2.0f
};
audio->playOnChannel(Channels::Ambience, ambience);
```

### Starting at a Specific Position

Resume playback from a specific timestamp:

```cpp
// Start music 30 seconds in
ChannelSound music{
    .asset = musicAsset,
    .looping = true,
    .startTime = 30.0f
};
audio->playOnChannel(Channels::Music, music);
```

## Playing Music

### Basic Music Playback

```cpp
class MusicManager {
    IAudioSystem* audio_;
    AssetHandle currentTrack_;

public:
    void playTrack(AssetHandle track) {
        currentTrack_ = track;
        ChannelSound music{
            .asset = track,
            .volume = 0.8f,
            .looping = true
        };
        audio_->playOnChannel(Channels::Music, music);
    }

    void stopMusic() {
        audio_->stopChannel(Channels::Music);
    }
};
```

### Music Crossfade

Smoothly transition between two music tracks:

```cpp
void crossfadeMusic(AssetHandle newTrack, float fadeTime) {
    // Stop current music with fade out
    audio->stopChannel(Channels::Music, fadeTime);

    // Note: Fade-out is not fully implemented yet
    // For now, use a timed callback to start new music

    // Start new music after fade out completes
    // In a real implementation, schedule this with a timer
    ChannelSound newMusic{
        .asset = newTrack,
        .volume = 0.8f,
        .looping = true,
        .fadeInTime = fadeTime
    };
    audio->playOnChannel(Channels::Music, newMusic);
}
```

### Dynamic Music System

Adapt music intensity based on gameplay:

```cpp
class DynamicMusicSystem {
    IAudioSystem* audio_;
    AssetHandle calmTrack_;
    AssetHandle intenseTrack_;
    bool isIntense_ = false;

public:
    void update(bool combatActive) {
        if (combatActive && !isIntense_) {
            transitionToIntense();
        } else if (!combatActive && isIntense_) {
            transitionToCalm();
        }
    }

private:
    void transitionToIntense() {
        isIntense_ = true;
        ChannelSound music{
            .asset = intenseTrack_,
            .volume = 0.9f,
            .looping = true,
            .fadeInTime = 1.0f
        };
        audio_->playOnChannel(Channels::Music, music);
    }

    void transitionToCalm() {
        isIntense_ = false;
        ChannelSound music{
            .asset = calmTrack_,
            .volume = 0.6f,
            .looping = true,
            .fadeInTime = 2.0f
        };
        audio_->playOnChannel(Channels::Music, music);
    }
};
```

## Volume and Pitch Control

### Channel Volume Control

```cpp
// Set music volume to 50%
audio->setChannelVolume(Channels::Music, 0.5f);

// Mute a channel
audio->setChannelVolume(Channels::Voice, 0.0f);

// Boost volume above 1.0 (may cause clipping)
audio->setChannelVolume(Channels::UI, 1.5f);
```

### Master Volume Control

Affects all audio globally:

```cpp
// Set master volume to 70%
audio->setMasterVolume(0.7f);

// Get current master volume
Volume currentVolume = audio->getMasterVolume();
```

### Pitch Control

Change the playback speed and pitch:

```cpp
// Play at normal pitch
audio->setChannelPitch(Channels::Voice, 1.0f);

// Play 50% slower and lower pitch
audio->setChannelPitch(Channels::Voice, 0.5f);

// Play 2x faster and higher pitch
audio->setChannelPitch(Channels::Voice, 2.0f);

// Simulate slow-motion effect
audio->setChannelPitch(Channels::Ambience, 0.3f);
```

### Volume Ramping Example

Gradually adjust volume over time:

```cpp
class VolumeController {
    IAudioSystem* audio_;
    Channel channel_;
    float targetVolume_;
    float currentVolume_;
    float rampSpeed_;

public:
    void update(DeltaTime dt) {
        if (std::abs(currentVolume_ - targetVolume_) > 0.01f) {
            float step = rampSpeed_ * dt;
            if (currentVolume_ < targetVolume_) {
                currentVolume_ = std::min(currentVolume_ + step, targetVolume_);
            } else {
                currentVolume_ = std::max(currentVolume_ - step, targetVolume_);
            }
            audio_->setChannelVolume(channel_, currentVolume_);
        }
    }

    void fadeTo(float targetVolume, float duration) {
        targetVolume_ = targetVolume;
        rampSpeed_ = std::abs(targetVolume - currentVolume_) / duration;
    }
};
```

## Channel Management

### Pausing and Resuming Channels

```cpp
// Pause music
audio->pauseChannel(Channels::Music);

// Check if paused
ChannelState state = audio->getChannelState(Channels::Music);
if (state.isPaused) {
    std::cout << "Music is paused" << std::endl;
}

// Resume playback
audio->resumeChannel(Channels::Music);
```

### Stopping Channels

```cpp
// Stop immediately
audio->stopChannel(Channels::Ambience);

// Stop with fade out (planned feature)
audio->stopChannel(Channels::Music, 2.0f);  // 2 second fade out
```

### Seeking in Audio

Jump to a specific position in the audio:

```cpp
// Seek to 1 minute into the track
audio->seekChannel(Channels::Music, 60.0f);

// Get current position
ChannelState state = audio->getChannelState(Channels::Music);
float currentTime = state.position;
```

### Checking Channel State

```cpp
// Quick check if playing
if (audio->isChannelPlaying(Channels::Music)) {
    std::cout << "Music is playing" << std::endl;
}

// Get detailed state
ChannelState state = audio->getChannelState(Channels::Music);
if (state.isPlaying && !state.isPaused) {
    std::cout << "Position: " << state.position << " / " << state.length << std::endl;
    std::cout << "Volume: " << state.volume << std::endl;
}
```

## Channel Groups

Channel groups allow you to control multiple channels together, useful for settings like "SFX Volume" or "Music Volume" in options menus.

### Creating and Using Groups

```cpp
// Create SFX group and set volume to 80%
audio->setGroupVolume("sfx", 0.8f);

// Assign multiple channels to the group
audio->assignChannelToGroup(10, "sfx");  // PlayerFootsteps
audio->assignChannelToGroup(11, "sfx");  // EnemyAttacks
audio->assignChannelToGroup(12, "sfx");  // Explosions

// All channels in "sfx" group now play at 80% of their individual volume
```

### Settings Menu Integration

```cpp
class AudioSettings {
    IAudioSystem* audio_;

public:
    void setMusicVolume(float volume) {
        audio_->setGroupVolume("music", volume);
    }

    void setSFXVolume(float volume) {
        audio_->setGroupVolume("sfx", volume);
    }

    void setVoiceVolume(float volume) {
        audio_->setGroupVolume("voice", volume);
    }

    void applySettings() {
        // Assign standard channels to groups
        audio_->assignChannelToGroup(Channels::Music, "music");
        audio_->assignChannelToGroup(Channels::Ambience, "music");
        audio_->assignChannelToGroup(Channels::UI, "sfx");
        audio_->assignChannelToGroup(Channels::Voice, "voice");
    }
};
```

## Positional 3D Audio

### Basic 3D Sound

Play a sound at a specific position in 3D space:

```cpp
// Play footstep sound at player position
AssetHandle footstepSound = assets->registerAsset(AssetType::Sound, "audio/footstep.wav");
PositionalSound sound{
    .asset = footstepSound,
    .position = Vec3{100.0f, 0.0f, 50.0f},
    .volume = 0.9f,
    .minDistance = 5.0f,    // Full volume within 5 units
    .maxDistance = 50.0f    // Silent beyond 50 units
};
SoundHandle handle = audio->playPositional(sound);
```

### Updating Sound Position

For moving sound sources (e.g., enemy footsteps):

```cpp
class MovingSoundSource {
    IAudioSystem* audio_;
    SoundHandle soundHandle_;
    Vec3 position_;

public:
    void playLoopingSound(AssetHandle asset) {
        PositionalSound sound{
            .asset = asset,
            .position = position_,
            .minDistance = 10.0f,
            .maxDistance = 100.0f
        };
        soundHandle_ = audio_->playPositional(sound);
    }

    void update(Vec3 newPosition) {
        position_ = newPosition;
        if (audio_->isPositionalPlaying(soundHandle_)) {
            audio_->updatePositionalPosition(soundHandle_, position_);
        }
    }

    void stop() {
        audio_->stopPositional(soundHandle_);
    }
};
```

### Doppler Effect

Add velocity for Doppler effect (pitch changes based on movement):

```cpp
// Fast-moving object with Doppler
PositionalSound projectileSound{
    .asset = projectileAsset,
    .position = projectilePos,
    .velocity = Vec3{velocityX, velocityY, 0.0f},
    .minDistance = 2.0f,
    .maxDistance = 30.0f
};
audio->playPositional(projectileSound);
```

### Audio Listener (Player/Camera)

Update the listener position each frame:

```cpp
void updateAudioListener(Vec3 playerPos, Vec3 playerVelocity, float cameraAngle) {
    AudioListener listener{
        .position = playerPos,
        .forward = Vec3{std::cos(cameraAngle), 0.0f, std::sin(cameraAngle)},
        .up = Vec3{0.0f, 1.0f, 0.0f},
        .velocity = playerVelocity
    };
    audio->setListener(listener);
}

// In game loop
void update(DeltaTime dt) {
    Vec3 playerPos = getPlayerPosition();
    Vec3 playerVel = getPlayerVelocity();
    float angle = getCameraAngle();

    updateAudioListener(playerPos, playerVel, angle);
    audio->update(dt);
}
```

### Distance-Based Volume

The distance between the sound source and listener affects volume:

- **minDistance**: Distance at which volume is at maximum
- **maxDistance**: Distance at which volume reaches zero
- Between min and max, volume falls off logarithmically (FMOD default)

```cpp
// Close-range UI sound
PositionalSound uiBeep{
    .asset = beepAsset,
    .position = buttonPosition,
    .minDistance = 1.0f,
    .maxDistance = 10.0f
};

// Long-range explosion
PositionalSound explosion{
    .asset = explosionAsset,
    .position = explosionPoint,
    .volume = 2.0f,  // Boost volume
    .minDistance = 20.0f,
    .maxDistance = 200.0f
};
```

## Global Controls

### Pause All Audio

Useful for pause menus:

```cpp
void onPauseGame() {
    audio->pauseAll();
}

void onResumeGame() {
    audio->resumeAll();
}
```

### Stop All Audio

Clear all playing sounds (e.g., when changing levels):

```cpp
void onLevelTransition() {
    audio->stopAll();
}
```

## Practical Examples

### Jump Sound Effect

```cpp
void onPlayerJump() {
    AssetHandle jumpSound = assets->getAsset("audio/jump.wav");
    ChannelSound sound{
        .asset = jumpSound,
        .volume = 0.7f,
        .pitch = 1.0f + (rand() % 20 - 10) * 0.01f  // Vary pitch slightly
    };
    audio->playOnChannel(Channels::UI, sound);
}
```

### Enemy Attack Sound (3D)

```cpp
void onEnemyAttack(Vec3 enemyPosition) {
    AssetHandle attackSound = assets->getAsset("audio/enemy_attack.wav");
    PositionalSound sound{
        .asset = attackSound,
        .position = enemyPosition,
        .volume = 1.0f,
        .minDistance = 10.0f,
        .maxDistance = 80.0f
    };
    audio->playPositional(sound);
}
```

### Collectible Pickup with Pitch Variation

```cpp
class CollectibleAudio {
    IAudioSystem* audio_;
    AssetHandle pickupSound_;
    int comboCount_ = 0;

public:
    void onCollect() {
        comboCount_++;
        float pitchMultiplier = 1.0f + (comboCount_ * 0.1f);
        pitchMultiplier = std::min(pitchMultiplier, 2.0f);

        ChannelSound sound{
            .asset = pickupSound_,
            .volume = 0.8f,
            .pitch = pitchMultiplier
        };
        audio_->playOnChannel(Channels::UI, sound);
    }

    void resetCombo() {
        comboCount_ = 0;
    }
};
```

### Environmental Ambience System

```cpp
class AmbienceSystem {
    IAudioSystem* audio_;
    std::unordered_map<std::string, AssetHandle> ambienceMap_;
    std::string currentZone_;

public:
    void registerZone(const std::string& zone, AssetHandle ambience) {
        ambienceMap_[zone] = ambience;
    }

    void enterZone(const std::string& zone) {
        if (zone == currentZone_) return;

        auto it = ambienceMap_.find(zone);
        if (it != ambienceMap_.end()) {
            ChannelSound ambience{
                .asset = it->second,
                .volume = 0.5f,
                .looping = true,
                .fadeInTime = 2.0f
            };
            audio_->playOnChannel(Channels::Ambience, ambience);
            currentZone_ = zone;
        }
    }
};
```

### Dynamic Footstep System

```cpp
class FootstepSystem {
    IAudioSystem* audio_;
    std::vector<AssetHandle> grassSounds_;
    std::vector<AssetHandle> stoneSounds_;
    size_t footstepIndex_ = 0;

public:
    void playFootstep(const std::string& surface, Vec3 position) {
        std::vector<AssetHandle>* sounds = nullptr;

        if (surface == "grass") {
            sounds = &grassSounds_;
        } else if (surface == "stone") {
            sounds = &stoneSounds_;
        }

        if (sounds && !sounds->empty()) {
            AssetHandle sound = (*sounds)[footstepIndex_ % sounds->size()];
            footstepIndex_++;

            PositionalSound footstep{
                .asset = sound,
                .position = position,
                .volume = 0.6f,
                .pitch = 0.9f + (rand() % 20) * 0.01f,  // Random pitch 0.9-1.1
                .minDistance = 5.0f,
                .maxDistance = 30.0f
            };
            audio_->playPositional(footstep);
        }
    }
};
```

### Boss Music System

```cpp
class BossMusicSystem {
    IAudioSystem* audio_;
    AssetHandle bossIntro_;
    AssetHandle bossLoop_;
    bool introPlayed_ = false;

public:
    void startBossFight() {
        introPlayed_ = false;
        ChannelSound intro{
            .asset = bossIntro_,
            .volume = 0.9f,
            .looping = false
        };
        audio_->playOnChannel(Channels::Music, intro);
    }

    void update() {
        if (!introPlayed_ && !audio_->isChannelPlaying(Channels::Music)) {
            // Intro finished, start loop
            ChannelSound loop{
                .asset = bossLoop_,
                .volume = 0.9f,
                .looping = true,
                .fadeInTime = 0.5f
            };
            audio_->playOnChannel(Channels::Music, loop);
            introPlayed_ = true;
        }
    }
};
```

## Best Practices

### Asset Management

1. **Register sounds once, reuse handles**:
   ```cpp
   // Good: Register once at startup
   AssetHandle jumpSound = assets->registerAsset(AssetType::Sound, "audio/jump.wav");

   // Use the handle many times
   for (int i = 0; i < 10; i++) {
       ChannelSound sound{.asset = jumpSound};
       audio->playOnChannel(Channels::UI, sound);
   }
   ```

2. **Preload frequently-used sounds** to avoid loading delays during gameplay.

### Channel Usage

1. **Reserve channels for specific purposes**:
   - Channel 0-9: System channels (music, ambience, UI, voice)
   - Channel 10-99: Game-specific channels
   - Channel 100+: Dynamic/temporary channels

2. **Don't play music on SFX channels** or vice versa - keep audio types separated for easier mixing.

### Performance

1. **Limit positional sounds**: Too many 3D sounds can impact performance. Use 2D sounds for UI and non-spatial effects.

2. **Clean up sound handles**: Positional sounds automatically clean up when finished, but if you stop them early, make sure to call `stopPositional()`.

3. **Use appropriate min/max distances**: Smaller ranges reduce CPU overhead for distance calculations.

### Volume Levels

1. **Keep base volumes below 1.0**: Leave headroom for mixing and master volume control.

2. **Volume guidelines**:
   - Music: 0.6 - 0.8
   - Ambience: 0.4 - 0.6
   - SFX: 0.7 - 0.9
   - UI: 0.5 - 0.7

3. **Master volume should default to 1.0** and be user-adjustable.

### Looping

1. **Use looping for music and continuous ambience**:
   ```cpp
   ChannelSound music{.asset = musicAsset, .looping = true};
   ```

2. **Don't loop one-shot sounds** like footsteps or UI clicks - they should be retriggered as needed.

### Testing Without Audio

The system works in stub mode when FMOD is not available:

```cpp
#ifndef JFRAME_HAS_FMOD
    // Code runs without audio hardware or FMOD library
    // State tracking still works for testing
#endif
```

## Troubleshooting

### No Sound Playing

1. Check FMOD initialization:
   ```cpp
   if (!fmodAudio->initialize()) {
       // FMOD failed to initialize
   }
   ```

2. Verify asset is loaded:
   ```cpp
   AssetState state = assets->getAssetState(soundAsset);
   if (state != AssetState::Loaded) {
       // Asset not ready
   }
   ```

3. Check channel state:
   ```cpp
   ChannelState state = audio->getChannelState(channel);
   std::cout << "Playing: " << state.isPlaying << ", Paused: " << state.isPaused << std::endl;
   ```

### 3D Sound Not Working

1. Ensure listener is set:
   ```cpp
   AudioListener listener{.position = playerPosition};
   audio->setListener(listener);
   ```

2. Check min/max distances are reasonable for your world scale.

3. Verify sound position is within range of listener.

### Volume Too Quiet

1. Check master volume:
   ```cpp
   std::cout << "Master volume: " << audio->getMasterVolume() << std::endl;
   ```

2. Check channel volume and group volume.

3. Verify original audio file isn't too quiet.

## Future Enhancements

- **Fade-out implementation**: Currently planned but not fully implemented
- **Audio compression settings**: FMOD format selection per asset
- **Audio ducking**: Automatically lower music when dialogue plays
- **Reverb zones**: Environmental audio effects
- **Audio pools**: Reusable sound instances for performance
- **Audio events**: Callbacks when sounds finish playing

## References

- Interface: `/Users/jaaaacob/Documents/GameDev/jframe/jframe-contract/src/jframe.audio.cppm`
- Implementation: `/Users/jaaaacob/Documents/GameDev/jframe/jframe-audio/src/FMODAudioSystem.cpp`
- Tests: `/Users/jaaaacob/Documents/GameDev/jframe/tests/unit/AudioSystemTests.cpp`
- Types: `/Users/jaaaacob/Documents/GameDev/jframe/jframe-contract/src/jframe.types.cppm`
- [FMOD Core API Documentation](https://www.fmod.com/docs/2.02/api/core-api.html)

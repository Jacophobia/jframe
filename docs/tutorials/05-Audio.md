# Tutorial 5: Audio

This tutorial covers JFrame's audio system, which is built on FMOD Core API. You'll learn how to play sounds, music, and use positional 3D audio.

## Audio System Overview

JFrame's audio system provides:

- **Channel-based audio** - Managed playback for music and looping sounds
- **Positional audio** - Fire-and-forget 3D sound effects
- **Volume control** - Master volume, channel groups, per-sound volume
- **Audio listener** - 3D audio relative to camera/player position

## Audio Asset Types

JFrame supports two types of audio assets:

### Sound Assets

Short sound effects loaded entirely into memory:

```cpp
// Register a sound effect
jframe::AssetHandle jumpSound = sys.assets->registerAsset(
    jframe::AssetType::Sound,
    "data/audio/jump.wav"
);
sys.assets->loadAsset(jumpSound);
```

### Music Assets

Longer audio files streamed from disk:

```cpp
// Register background music
jframe::AssetHandle bgMusic = sys.assets->registerAsset(
    jframe::AssetType::Music,
    "data/audio/level1_music.ogg"
);
sys.assets->loadAsset(bgMusic);
```

Supported formats: WAV, OGG, MP3

## Channel-Based Audio

Channels are for managed, long-running sounds like music and ambient loops.

### Predefined Channels

```cpp
namespace jframe::Channels {
    inline constexpr Channel Music = 0;
    inline constexpr Channel Ambient = 1;
    inline constexpr Channel UI = 2;
    inline constexpr Channel Voice = 3;
    inline constexpr Channel SFX1 = 4;
    inline constexpr Channel SFX2 = 5;
    inline constexpr Channel SFX3 = 6;
    inline constexpr Channel SFX4 = 7;
}
```

### Playing Music

```cpp
void startBackgroundMusic() {
    auto& sys = engine_->systems();

    // Play on the Music channel
    sys.audio->playOnChannel(jframe::Channels::Music, jframe::ChannelSound{
        .asset = musicAsset_,
        .volume = 0.7f,
        .looping = true,
        .fadeInTime = 2.0f  // Fade in over 2 seconds
    });
}
```

### Channel Controls

```cpp
// Stop music (with optional fade out)
sys.audio->stopChannel(jframe::Channels::Music, 1.5f);  // Fade out over 1.5s

// Pause and resume
sys.audio->pauseChannel(jframe::Channels::Music);
sys.audio->resumeChannel(jframe::Channels::Music);

// Change volume
sys.audio->setChannelVolume(jframe::Channels::Music, 0.5f);

// Change pitch (speed)
sys.audio->setChannelPitch(jframe::Channels::Music, 1.2f);  // 20% faster

// Seek to position
sys.audio->seekChannel(jframe::Channels::Music, 30.0f);  // Skip to 30 seconds
```

### Channel State

```cpp
// Check if a channel is playing
if (sys.audio->isChannelPlaying(jframe::Channels::Music)) {
    jframe::core::logInfo("Music is playing");
}

// Get detailed channel state
jframe::ChannelState state = sys.audio->getChannelState(jframe::Channels::Music);

if (state.isPlaying) {
    jframe::core::logInfo(std::format("Position: {:.2f}s / {:.2f}s",
        state.position, state.length));
    jframe::core::logInfo(std::format("Volume: {:.2f}", state.volume));
}

if (state.isPaused) {
    jframe::core::logInfo("Music is paused");
}
```

## Positional Audio

Positional audio is for one-shot 3D sound effects. The audio system manages the lifetime automatically.

### Playing Positional Sounds

```cpp
void playFootstep(jframe::Vec2 playerPos) {
    auto& sys = engine_->systems();

    // Play a 3D sound at the player's position
    jframe::SoundHandle handle = sys.audio->playPositional(jframe::PositionalSound{
        .asset = footstepSound_,
        .position = {playerPos.x, playerPos.y, 0.0f},
        .volume = 0.8f,
        .minDistance = 50.0f,   // Full volume within 50 units
        .maxDistance = 500.0f   // Silent beyond 500 units
    });

    // Fire and forget - no need to manage the handle
}
```

### Updating Positional Sounds

For sounds that follow a moving object:

```cpp
// Store the handle
jframe::SoundHandle engineSound_;

void startEngineSound(jframe::Vec2 carPos) {
    engineSound_ = sys.audio->playPositional(jframe::PositionalSound{
        .asset = engineSound_,
        .position = {carPos.x, carPos.y, 0.0f},
        .volume = 1.0f,
        .looping = true
    });
}

void updateEngineSound(jframe::Vec2 carPos) {
    if (sys.audio->isPositionalPlaying(engineSound_)) {
        // Update position as the car moves
        sys.audio->updatePositionalPosition(engineSound_, {carPos.x, carPos.y, 0.0f});
    }
}

void stopEngineSound() {
    sys.audio->stopPositional(engineSound_);
}
```

## 3D Audio Listener

The listener represents the player's ears. Positional sounds are heard relative to the listener.

### Setting the Listener

```cpp
void updateAudioListener(jframe::Vec2 cameraPos) {
    auto& sys = engine_->systems();

    // Set listener to camera/player position
    sys.audio->setListener(jframe::AudioListener{
        .position = {cameraPos.x, cameraPos.y, 0.0f},
        .forward = {0.0f, 0.0f, -1.0f},  // Looking into screen
        .up = {0.0f, 1.0f, 0.0f}         // Y-axis is up
    });
}
```

### Audio Falloff

Positional sounds get quieter with distance:

```
Volume
  ^
  |  Full volume
  |  |------------|
  |               \
  |                \
  |                 \_________ Silent
  |
  +-----|--------|-----------|---> Distance
     minDist  midpoint    maxDist
```

- **minDistance**: Full volume within this radius
- **maxDistance**: Silent beyond this distance
- Between min and max: Volume fades linearly

## Volume Control

### Master Volume

```cpp
// Set global volume (0.0 to 1.0)
sys.audio->setMasterVolume(0.8f);

// Get current master volume
float volume = sys.audio->getMasterVolume();
```

### Volume Groups

Group channels together for easier volume control:

```cpp
// Create volume groups
sys.audio->assignChannelToGroup(jframe::Channels::Music, "music");
sys.audio->assignChannelToGroup(jframe::Channels::Ambient, "music");

sys.audio->assignChannelToGroup(jframe::Channels::UI, "sfx");
sys.audio->assignChannelToGroup(jframe::Channels::SFX1, "sfx");
sys.audio->assignChannelToGroup(jframe::Channels::SFX2, "sfx");

// Control all music channels at once
sys.audio->setGroupVolume("music", 0.5f);

// Control all SFX channels at once
sys.audio->setGroupVolume("sfx", 0.9f);
```

### Volume Settings Menu

```cpp
class AudioSettingsMenu {
public:
    void applySettings() {
        sys.audio->setMasterVolume(masterVolume_);
        sys.audio->setGroupVolume("music", musicVolume_);
        sys.audio->setGroupVolume("sfx", sfxVolume_);
    }

    void saveSettings() {
        nlohmann::json settings;
        settings["masterVolume"] = masterVolume_;
        settings["musicVolume"] = musicVolume_;
        settings["sfxVolume"] = sfxVolume_;

        std::ofstream file("settings/audio.json");
        file << settings.dump(4);
    }

    void loadSettings() {
        std::ifstream file("settings/audio.json");
        if (!file.is_open()) return;

        nlohmann::json settings;
        file >> settings;

        masterVolume_ = settings["masterVolume"].get<float>();
        musicVolume_ = settings["musicVolume"].get<float>();
        sfxVolume_ = settings["sfxVolume"].get<float>();

        applySettings();
    }

private:
    float masterVolume_ = 1.0f;
    float musicVolume_ = 0.7f;
    float sfxVolume_ = 1.0f;
};
```

## Common Audio Patterns

### Footstep System

```cpp
struct Footsteps {
    float stepInterval = 0.5f;  // Seconds between steps
    float stepTimer = 0.0f;
    jframe::AssetHandle stepSound;
};

void updateFootsteps(jframe::DeltaTime dt) {
    auto view = sys.entities->getRegistry().view<Footsteps>();

    for (auto [entity, footsteps] : view.each()) {
        // Only play when moving
        jframe::Vec2 vel = sys.physics->getVelocity(entity);
        bool isMoving = std::abs(vel.x) > 10.0f;

        if (isMoving) {
            footsteps.stepTimer += dt;

            if (footsteps.stepTimer >= footsteps.stepInterval) {
                footsteps.stepTimer = 0.0f;

                jframe::Vec2 pos = sys.physics->getPosition(entity);
                sys.audio->playPositional(jframe::PositionalSound{
                    .asset = footsteps.stepSound,
                    .position = {pos.x, pos.y, 0.0f},
                    .volume = 0.6f,
                    .minDistance = 30.0f,
                    .maxDistance = 200.0f
                });
            }
        } else {
            footsteps.stepTimer = 0.0f;
        }
    }
}
```

### Impact Sounds

```cpp
void onCollision(const jframe::CollisionEvent& collision) {
    // Only play on collision start
    if (!collision.isBegin) return;

    // Get relative velocity
    jframe::Vec2 velA = sys.physics->getVelocity(collision.entityA);
    jframe::Vec2 velB = sys.physics->getVelocity(collision.entityB);

    float impactSpeed = glm::length(velA - velB);

    // Only play sound for strong impacts
    if (impactSpeed > 100.0f) {
        float volume = std::clamp(impactSpeed / 500.0f, 0.0f, 1.0f);

        sys.audio->playPositional(jframe::PositionalSound{
            .asset = impactSound_,
            .position = {collision.contactPoint.x, collision.contactPoint.y, 0.0f},
            .volume = volume,
            .minDistance = 50.0f,
            .maxDistance = 300.0f
        });
    }
}
```

### Ambient Sound Zones

```cpp
struct AmbientZone {
    jframe::Rect bounds;
    jframe::AssetHandle ambientSound;
    jframe::Channel channel;
    float targetVolume = 0.5f;
    float currentVolume = 0.0f;
};

void updateAmbientZones(jframe::DeltaTime dt, jframe::Vec2 playerPos) {
    for (auto& zone : ambientZones_) {
        // Check if player is in zone
        bool inZone = zone.bounds.contains(playerPos);

        // Fade in/out
        float fadeSpeed = 2.0f;
        if (inZone) {
            zone.currentVolume = std::min(
                zone.currentVolume + dt * fadeSpeed,
                zone.targetVolume
            );

            if (!sys.audio->isChannelPlaying(zone.channel)) {
                sys.audio->playOnChannel(zone.channel, jframe::ChannelSound{
                    .asset = zone.ambientSound,
                    .volume = zone.currentVolume,
                    .looping = true
                });
            }
        } else {
            zone.currentVolume = std::max(
                zone.currentVolume - dt * fadeSpeed,
                0.0f
            );
        }

        sys.audio->setChannelVolume(zone.channel, zone.currentVolume);

        if (zone.currentVolume == 0.0f && sys.audio->isChannelPlaying(zone.channel)) {
            sys.audio->stopChannel(zone.channel);
        }
    }
}
```

### UI Sound Effects

```cpp
class UIAudio {
public:
    void playHover() {
        sys.audio->playOnChannel(jframe::Channels::UI, jframe::ChannelSound{
            .asset = hoverSound_,
            .volume = 0.3f
        });
    }

    void playClick() {
        sys.audio->playOnChannel(jframe::Channels::UI, jframe::ChannelSound{
            .asset = clickSound_,
            .volume = 0.5f
        });
    }

    void playError() {
        sys.audio->playOnChannel(jframe::Channels::UI, jframe::ChannelSound{
            .asset = errorSound_,
            .volume = 0.7f
        });
    }

private:
    jframe::AssetHandle hoverSound_;
    jframe::AssetHandle clickSound_;
    jframe::AssetHandle errorSound_;
};
```

### Music Crossfade

```cpp
void crossfadeMusic(jframe::AssetHandle newMusic, float fadeTime = 2.0f) {
    // Fade out current music
    if (sys.audio->isChannelPlaying(jframe::Channels::Music)) {
        sys.audio->stopChannel(jframe::Channels::Music, fadeTime);
    }

    // Wait for fade out, then start new music
    // (In a real implementation, you'd use a coroutine or timer)
    std::thread([this, newMusic, fadeTime]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(
            static_cast<int>(fadeTime * 1000)));

        sys.audio->playOnChannel(jframe::Channels::Music, jframe::ChannelSound{
            .asset = newMusic,
            .volume = 0.7f,
            .looping = true,
            .fadeInTime = fadeTime
        });
    }).detach();
}
```

## Pause Menu Audio

```cpp
void onPauseGame() {
    // Pause all game audio
    sys.audio->pauseAll();

    // Play pause menu music
    sys.audio->playOnChannel(jframe::Channels::Music, jframe::ChannelSound{
        .asset = pauseMenuMusic_,
        .volume = 0.5f,
        .looping = true
    });
}

void onResumeGame() {
    // Stop pause menu music
    sys.audio->stopChannel(jframe::Channels::Music);

    // Resume all game audio
    sys.audio->resumeAll();
}
```

## Organizing Audio Assets

### Folder Structure

```
data/audio/
├── music/
│   ├── menu_theme.ogg
│   ├── level1_theme.ogg
│   ├── level2_theme.ogg
│   └── boss_theme.ogg
├── sfx/
│   ├── player/
│   │   ├── jump.wav
│   │   ├── land.wav
│   │   ├── hurt.wav
│   │   └── attack.wav
│   ├── enemies/
│   │   ├── enemy_hit.wav
│   │   └── enemy_death.wav
│   ├── items/
│   │   ├── coin_pickup.wav
│   │   ├── health_pickup.wav
│   │   └── powerup.wav
│   └── ambient/
│       ├── wind.wav
│       ├── water.wav
│       └── fire.wav
└── ui/
    ├── button_hover.wav
    ├── button_click.wav
    └── menu_open.wav
```

### Loading Audio in Lua

```lua
-- data/config/audio.lua
return {
    music = {
        menuTheme = "audio/music/menu_theme.ogg",
        level1Theme = "audio/music/level1_theme.ogg",
        bossTheme = "audio/music/boss_theme.ogg"
    },

    sfx = {
        player = {
            jump = "audio/sfx/player/jump.wav",
            land = "audio/sfx/player/land.wav",
            hurt = "audio/sfx/player/hurt.wav",
            attack = "audio/sfx/player/attack.wav"
        },

        items = {
            coin = "audio/sfx/items/coin_pickup.wav",
            health = "audio/sfx/items/health_pickup.wav"
        },

        ui = {
            hover = "audio/ui/button_hover.wav",
            click = "audio/ui/button_click.wav"
        }
    },

    volumes = {
        master = 0.8,
        music = 0.7,
        sfx = 1.0,
        ui = 0.9
    }
}
```

Load in C++:

```cpp
void loadAudioConfig() {
    sol::state lua;
    lua.open_libraries(sol::lib::base);

    auto result = lua.script_file("data/config/audio.lua");
    sol::table config = result;

    // Load music
    sol::table music = config["music"];
    menuThemeAsset_ = sys.assets->registerAsset(
        jframe::AssetType::Music,
        music["menuTheme"].get<std::string>()
    );

    // Load SFX
    sol::table sfx = config["sfx"];
    sol::table playerSfx = sfx["player"];
    jumpSoundAsset_ = sys.assets->registerAsset(
        jframe::AssetType::Sound,
        playerSfx["jump"].get<std::string>()
    );

    // Apply volumes
    sol::table volumes = config["volumes"];
    sys.audio->setMasterVolume(volumes["master"].get<float>());
    sys.audio->setGroupVolume("music", volumes["music"].get<float>());
    sys.audio->setGroupVolume("sfx", volumes["sfx"].get<float>());
}
```

## Best Practices

### 1. Use Appropriate Asset Types

- **Sound**: Short effects (< 1 second) - loaded into memory
- **Music**: Long tracks - streamed from disk

### 2. Volume Mixing

Balance your audio levels:

```cpp
// Music should be quieter than SFX
sys.audio->playOnChannel(Music, {..., .volume = 0.6f});

// SFX at full volume
sys.audio->playPositional({..., .volume = 1.0f});

// UI sounds quieter to avoid annoying the player
sys.audio->playOnChannel(UI, {..., .volume = 0.4f});
```

### 3. Distance Attenuation

Set appropriate min/max distances:

```cpp
// Quiet sounds (footsteps)
minDistance = 20.0f;
maxDistance = 100.0f;

// Medium sounds (item pickup)
minDistance = 50.0f;
maxDistance = 300.0f;

// Loud sounds (explosions)
minDistance = 100.0f;
maxDistance = 1000.0f;
```

### 4. Limit Simultaneous Sounds

Don't spam the audio system:

```cpp
// Bad: Play coin sound for every coin in a grid
for (int i = 0; i < 100; i++) {
    playSound(coinSound);  // Plays 100 sounds at once!
}

// Good: Play once when collecting multiple coins
if (coinsCollected > 0) {
    playSound(coinSound);
}
```

### 5. Update Listener Every Frame

```cpp
void updateFixed(jframe::DeltaTime dt) {
    // Update listener to follow player/camera
    jframe::Vec2 playerPos = sys.physics->getPosition(player_);
    sys.audio->setListener(jframe::AudioListener{
        .position = {playerPos.x, playerPos.y, 0.0f}
    });

    // Update other systems...
}
```

## Troubleshooting

**No sound playing**
- Check that FMOD is properly installed (see `docs/LLVM20-SETUP.md`)
- Verify audio assets are loaded: `sys.assets->getAssetState(audioAsset)`
- Check master volume: `sys.audio->getMasterVolume()`
- Verify sound files exist and are in a supported format

**Positional audio not working**
- Make sure you're calling `sys.audio->setListener()` each frame
- Check min/max distance values
- Verify listener position is correct

**Music cuts off abruptly**
- Use fade out: `sys.audio->stopChannel(channel, fadeTime)`
- Check for channel conflicts (two sounds on same channel)

**Sound too loud/quiet**
- Adjust master volume
- Use volume groups for easier control
- Check per-sound volume settings

**Crackling or distortion**
- Audio file may be corrupted
- Try a different format (WAV is most reliable)
- Check CPU usage - audio system may be overloaded

## Performance Tips

### Asset Management

```cpp
// Preload frequently used sounds
void preloadAudio() {
    // Load immediately on startup
    sys.assets->loadAsset(jumpSound_);
    sys.assets->loadAsset(coinSound_);

    // Music can be loaded on-demand
    sys.assets->loadAssetAsync(level1Music_);
}
```

### Pooling Sound Handles

```cpp
// Reuse handles for repeating sounds
class SoundPool {
public:
    jframe::SoundHandle playPooled(jframe::PositionalSound sound) {
        // Find an available handle
        for (auto& handle : pool_) {
            if (!sys.audio->isPositionalPlaying(handle)) {
                // Reuse this handle
                handle = sys.audio->playPositional(sound);
                return handle;
            }
        }

        // All handles in use - add a new one
        auto handle = sys.audio->playPositional(sound);
        pool_.push_back(handle);
        return handle;
    }

private:
    std::vector<jframe::SoundHandle> pool_;
};
```

## Conclusion

You now know how to use JFrame's audio system! Key takeaways:

- Use channels for managed audio (music, ambient loops)
- Use positional audio for one-shot 3D sound effects
- Update the listener position every frame
- Organize audio assets in a clear folder structure
- Use volume groups for easy control

## Further Reading

- FMOD Core API: https://fmod.com/docs/2.02/api/core-api.html
- JFrame Audio System API: `docs/systems/audio.md`
- Example: `examples/platformer/src/Game.cpp`

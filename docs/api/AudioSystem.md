# AudioSystem API

The `AudioSystem` provides audio playback with FMOD, supporting channel-based audio and 3D positional sound.

## Overview

```cpp
auto& audio = sys.audio;

// Load sound assets
AssetHandle sfx = assets->registerAsset(AssetType::Sound, "sounds/jump.wav");
AssetHandle music = assets->registerAsset(AssetType::Music, "music/bgm.ogg");
assets->loadAsset(sfx);
assets->loadAsset(music);

// Play on channel
audio->playOnChannel(Channels::Music, {
    .asset = music,
    .volume = 0.8f,
    .looping = true
});

// Play positional sound
audio->playPositional({
    .asset = sfx,
    .position = {100.0f, 200.0f, 0.0f},
    .volume = 1.0f
});

// Update listener
audio->update(dt);
```

## Lifecycle

### update(DeltaTime dt)

```cpp
void update(DeltaTime dt);
```

Updates the FMOD audio system. Call once per frame.

**Example:**

```cpp
void updateFixed(DeltaTime dt) override {
    audio->update(dt);
}
```

---

## Channel-Based Audio (Managed)

Channels are long-lived audio streams for music, ambient sounds, and UI audio.

### playOnChannel(Channel channel, const ChannelSound& sound)

```cpp
void playOnChannel(Channel channel, const ChannelSound& sound);
```

Plays a sound on the specified channel. Replaces any currently playing sound on that channel.

**ChannelSound Structure:**

```cpp
struct ChannelSound {
    AssetHandle asset;
    Volume volume = 1.0f;              // 0.0 = silent, 1.0 = full volume
    float pitch = 1.0f;                // 0.5 = half speed, 2.0 = double speed
    bool looping = false;
    float fadeInTime = 0.0f;           // Fade in duration (seconds)
    std::optional<float> startTime;    // Start position (seconds)
};
```

**Example:**

```cpp
// Background music with loop
audio->playOnChannel(Channels::Music, {
    .asset = musicHandle,
    .volume = 0.7f,
    .looping = true,
    .fadeInTime = 2.0f
});

// One-shot UI sound
audio->playOnChannel(Channels::UI, {
    .asset = clickSound,
    .volume = 1.0f
});
```

---

### Predefined Channels

```cpp
namespace Channels {
    inline constexpr Channel Music = 0;
    inline constexpr Channel Ambience = 1;
    inline constexpr Channel UI = 2;
    inline constexpr Channel Voice = 3;
}
```

You can use any `uint32_t` value as a channel ID.

---

### stopChannel(Channel channel, float fadeOutTime)

```cpp
void stopChannel(Channel channel, float fadeOutTime = 0.0f);
```

Stops playback on a channel.

**Example:**

```cpp
// Instant stop
audio->stopChannel(Channels::Music);

// Fade out over 2 seconds
audio->stopChannel(Channels::Music, 2.0f);
```

---

### pauseChannel(Channel channel)

```cpp
void pauseChannel(Channel channel);
```

Pauses playback on a channel.

---

### resumeChannel(Channel channel)

```cpp
void resumeChannel(Channel channel);
```

Resumes playback on a paused channel.

**Example:**

```cpp
// Pause menu
if (gamePaused) {
    audio->pauseChannel(Channels::Music);
} else {
    audio->resumeChannel(Channels::Music);
}
```

---

### setChannelVolume(Channel channel, Volume volume)

```cpp
void setChannelVolume(Channel channel, Volume volume);
```

Sets the volume of a channel (0.0 to 1.0).

---

### setChannelPitch(Channel channel, float pitch)

```cpp
void setChannelPitch(Channel channel, float pitch);
```

Sets the playback speed/pitch of a channel.

**Example:**

```cpp
// Slow motion effect
audio->setChannelPitch(Channels::Music, 0.5f);
```

---

### seekChannel(Channel channel, float position)

```cpp
void seekChannel(Channel channel, float position);
```

Seeks to a specific position in seconds.

---

### getChannelState(Channel channel)

```cpp
ChannelState getChannelState(Channel channel) const;
```

Returns the current state of a channel.

**ChannelState Structure:**

```cpp
struct ChannelState {
    bool isPlaying = false;
    bool isPaused = false;
    float position = 0.0f;  // Current playback position (seconds)
    float length = 0.0f;    // Total length (seconds)
    Volume volume = 1.0f;
};
```

---

### isChannelPlaying(Channel channel)

```cpp
bool isChannelPlaying(Channel channel) const;
```

Returns `true` if the channel is currently playing.

---

## Positional Audio (Fire & Forget)

Positional sounds are one-shot 3D audio events that automatically clean up when finished.

### playPositional(const PositionalSound& sound)

```cpp
SoundHandle playPositional(const PositionalSound& sound);
```

Plays a 3D positional sound and returns a handle.

**PositionalSound Structure:**

```cpp
struct PositionalSound {
    AssetHandle asset;
    Vec3 position{0.0f};
    Volume volume = 1.0f;
    float pitch = 1.0f;
    float minDistance = 1.0f;   // Full volume within this radius
    float maxDistance = 100.0f; // Inaudible beyond this radius
    std::optional<Vec3> velocity;
    std::function<void()> onComplete;
};
```

**Example:**

```cpp
// Explosion sound
Vec2 explosionPos2D = physics->getPosition(bomb);
audio->playPositional({
    .asset = explosionSound,
    .position = {explosionPos2D.x, explosionPos2D.y, 0.0f},
    .volume = 1.0f,
    .minDistance = 50.0f,
    .maxDistance = 500.0f
});

// Footstep with velocity (Doppler effect)
audio->playPositional({
    .asset = footstepSound,
    .position = {playerPos.x, playerPos.y, 0.0f},
    .velocity = Vec3{vel.x, vel.y, 0.0f},
    .minDistance = 10.0f,
    .maxDistance = 100.0f,
    .onComplete = []() { /* Footstep finished */ }
});
```

---

### stopPositional(SoundHandle handle)

```cpp
void stopPositional(SoundHandle handle);
```

Stops a positional sound.

---

### updatePositionalPosition(SoundHandle handle, Vec3 position)

```cpp
void updatePositionalPosition(SoundHandle handle, Vec3 position);
```

Updates the position of a positional sound (for moving sources).

**Example:**

```cpp
// Moving vehicle sound
SoundHandle engineSound = audio->playPositional({
    .asset = engineLoop,
    .position = {carPos.x, carPos.y, 0.0f},
    .looping = true
});

// Update loop
while (carMoving) {
    Vec2 carPos = physics->getPosition(car);
    audio->updatePositionalPosition(engineSound,
        {carPos.x, carPos.y, 0.0f}
    );
    audio->update(dt);
}

audio->stopPositional(engineSound);
```

---

### isPositionalPlaying(SoundHandle handle)

```cpp
bool isPositionalPlaying(SoundHandle handle) const;
```

Returns `true` if the positional sound is still playing.

---

## 3D Audio Listener

The listener represents the player's ears in 3D space.

### setListener(const AudioListener& listener)

```cpp
void setListener(const AudioListener& listener);
AudioListener getListener() const;
```

Sets the 3D audio listener position and orientation.

**AudioListener Structure:**

```cpp
struct AudioListener {
    Vec3 position{0.0f};
    Vec3 forward{0.0f, 0.0f, -1.0f};
    Vec3 up{0.0f, 1.0f, 0.0f};
    Vec3 velocity{0.0f};
};
```

**Example:**

```cpp
// Update listener to follow player
void updateAudioListener() {
    Vec2 playerPos = physics->getPosition(player);
    Vec2 playerVel = physics->getVelocity(player);

    audio->setListener({
        .position = {playerPos.x, playerPos.y, 0.0f},
        .forward = {0.0f, 0.0f, -1.0f},  // Into screen
        .up = {0.0f, -1.0f, 0.0f},       // Y-down (screen space)
        .velocity = {playerVel.x, playerVel.y, 0.0f}
    });
}
```

---

## Global Controls

### setMasterVolume(Volume volume)

```cpp
void setMasterVolume(Volume volume);
Volume getMasterVolume() const;
```

Sets the global volume (0.0 to 1.0). Affects all audio.

**Example:**

```cpp
// Settings menu
audio->setMasterVolume(settingsVolume);
```

---

### pauseAll()

```cpp
void pauseAll();
```

Pauses all channels and positional sounds.

---

### resumeAll()

```cpp
void resumeAll();
```

Resumes all paused audio.

---

### stopAll()

```cpp
void stopAll();
```

Stops all audio immediately.

---

## Channel Groups

Channel groups allow controlling volume for categories of audio.

### setGroupVolume(const std::string& group, Volume volume)

```cpp
void setGroupVolume(const std::string& group, Volume volume);
```

Sets the volume for a channel group.

**Example:**

```cpp
// Create groups
audio->setGroupVolume("SFX", 0.8f);
audio->setGroupVolume("Music", 0.6f);
audio->setGroupVolume("Voice", 1.0f);
```

---

### assignChannelToGroup(Channel channel, const std::string& group)

```cpp
void assignChannelToGroup(Channel channel, const std::string& group);
```

Assigns a channel to a group.

**Example:**

```cpp
// Assign and set volume
audio->assignChannelToGroup(Channels::Music, "Music");
audio->setGroupVolume("Music", 0.5f);  // Affects all music channels
```

---

## Common Patterns

### Background Music with Crossfade

```cpp
void crossfadeMusic(AssetHandle newMusic, float fadeTime = 2.0f) {
    // Fade out current music
    audio->stopChannel(Channels::Music, fadeTime);

    // Wait for fade to complete (in update loop)
    // Then start new music
    audio->playOnChannel(Channels::Music, {
        .asset = newMusic,
        .volume = 0.7f,
        .looping = true,
        .fadeInTime = fadeTime
    });
}
```

---

### Adaptive Music (Layer System)

```cpp
struct AdaptiveMusicLayers {
    Channel drums = 10;
    Channel bass = 11;
    Channel melody = 12;
    Channel harmony = 13;
};

void setMusicIntensity(float intensity) {
    // intensity: 0.0 (calm) to 1.0 (intense)

    if (intensity < 0.25f) {
        // Minimal - drums only
        audio->setChannelVolume(layers.drums, 1.0f);
        audio->setChannelVolume(layers.bass, 0.0f);
        audio->setChannelVolume(layers.melody, 0.0f);
        audio->setChannelVolume(layers.harmony, 0.0f);
    } else if (intensity < 0.5f) {
        // Build - drums + bass
        audio->setChannelVolume(layers.drums, 1.0f);
        audio->setChannelVolume(layers.bass, 1.0f);
        audio->setChannelVolume(layers.melody, 0.0f);
        audio->setChannelVolume(layers.harmony, 0.0f);
    } else if (intensity < 0.75f) {
        // Combat - all but harmony
        audio->setChannelVolume(layers.drums, 1.0f);
        audio->setChannelVolume(layers.bass, 1.0f);
        audio->setChannelVolume(layers.melody, 1.0f);
        audio->setChannelVolume(layers.harmony, 0.0f);
    } else {
        // Full intensity
        audio->setChannelVolume(layers.drums, 1.0f);
        audio->setChannelVolume(layers.bass, 1.0f);
        audio->setChannelVolume(layers.melody, 1.0f);
        audio->setChannelVolume(layers.harmony, 1.0f);
    }
}
```

---

### Sound Pool (Multiple Footsteps)

```cpp
struct SoundPool {
    std::vector<AssetHandle> sounds;
    std::mt19937 rng{std::random_device{}()};

    AssetHandle getRandomSound() {
        std::uniform_int_distribution<size_t> dist(0, sounds.size() - 1);
        return sounds[dist(rng)];
    }
};

SoundPool footsteps;
footsteps.sounds = {footstep1, footstep2, footstep3, footstep4};

// Play random footstep
Vec2 pos = physics->getPosition(player);
audio->playPositional({
    .asset = footsteps.getRandomSound(),
    .position = {pos.x, pos.y, 0.0f},
    .volume = 0.8f
});
```

---

### Looping Ambient Sound

```cpp
// Per-level ambient loop
void startAmbientSound(AssetHandle ambient) {
    audio->playOnChannel(Channels::Ambience, {
        .asset = ambient,
        .volume = 0.5f,
        .looping = true,
        .fadeInTime = 3.0f
    });
}

void stopAmbientSound() {
    audio->stopChannel(Channels::Ambience, 3.0f);
}
```

---

### Distance-Based Sound Trigger

```cpp
void updateProximitySounds(Entity player, Entity npc) {
    Vec2 playerPos = physics->getPosition(player);
    Vec2 npcPos = physics->getPosition(npc);

    float dist = glm::distance(playerPos, npcPos);

    if (dist < 200.0f && !npcTalking) {
        // Start dialogue when close
        audio->playOnChannel(Channels::Voice, {
            .asset = dialogueSound,
            .volume = 1.0f
        });
        npcTalking = true;
    }
}
```

---

## Asset Types

Use the appropriate asset type when registering audio:

```cpp
// Short sounds (loaded into memory)
AssetHandle jump = assets->registerAsset(AssetType::Sound, "sounds/jump.wav");

// Long sounds/music (streamed from disk)
AssetHandle bgm = assets->registerAsset(AssetType::Music, "music/level1.ogg");
```

**Sound:** Fully loaded into RAM (fast, use for SFX)
**Music:** Streamed from disk (low memory, use for long tracks)

---

## Performance Tips

1. **Use `AssetType::Music` for long tracks** - Streams instead of loading fully
2. **Limit concurrent positional sounds** - FMOD has a channel limit
3. **Use channel groups** - More efficient than per-channel volume changes
4. **Preload frequently used sounds** - Load SFX during level load
5. **Unload unused music** - Free memory when changing levels

## See Also

- [AssetSystem](AssetSystem.md) - Loading audio files
- [PhysicsSystem](PhysicsSystem.md) - Getting entity positions for 3D audio
- [EventSystem](EventSystem.md) - Triggering audio on events

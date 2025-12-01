# Audio System Demo

Comprehensive demonstration of the JFrame Audio System (`IAudioSystem`) interface.

## Overview

This demo exercises **every method** of the Audio System API, demonstrating:

- **Channel-based audio** for managed music, ambience, UI, and voice
- **Positional 3D audio** with distance attenuation and Doppler effect
- **3D audio listener** for player position and orientation
- **Global controls** for master volume and pause/resume
- **Channel groups** for category-based volume mixing
- **Volume hierarchy** (Master → Group → Channel → Asset)
- **Smooth fading** for professional audio transitions

## Building

```bash
# Configure the project
cmake --preset macos-debug

# Build the demo
cmake --build --preset macos-debug --target audio-demo

# Run the demo
./build/macos-debug/examples/system-demos/audio-demo/audio-demo
```

## What's Demonstrated

### Section 1: Asset Registration
- Registering audio assets via `IAssetSystem`

### Section 2: Channel-Based Audio
- `playOnChannel()` - Play sounds on managed channels
- `ChannelSound` structure with volume, pitch, looping, and fade-in

### Section 3: Channel Control
- `setChannelVolume()` - Dynamic volume control
- `setChannelPitch()` - Pitch shifting (slow-motion effects)
- `pauseChannel()` / `resumeChannel()` - Pause/resume playback
- `seekChannel()` - Jump to specific playback position
- `stopChannel()` - Stop with optional fade-out

### Section 4: Channel State Queries
- `isChannelPlaying()` - Quick playing check
- `getChannelState()` - Detailed state (position, length, volume)

### Section 5: Positional (3D) Audio
- `playPositional()` - Fire-and-forget 3D sounds
- `PositionalSound` with min/max distance, velocity, callbacks
- `updatePositionalPosition()` - Move sound sources
- `stopPositional()` - Stop specific positional sound
- Doppler effect demonstration

### Section 6: 3D Audio Listener
- `setListener()` - Update player position and orientation
- `getListener()` - Query current listener state
- Doppler effect from listener movement

### Section 7: Global Audio Controls
- `setMasterVolume()` / `getMasterVolume()` - Global volume
- `pauseAll()` - Pause all audio
- `resumeAll()` - Resume all audio
- `stopAll()` - Stop all audio immediately

### Section 8: Channel Groups
- `assignChannelToGroup()` - Organize channels into categories
- `setGroupVolume()` - Control volume by category (Music, SFX, Dialogue)
- Volume hierarchy demonstration

### Section 9: Lifecycle Management
- `update()` - Per-frame audio processing

### Section 10: Practical Usage Scenarios
- Music crossfading
- UI sound effects
- Combat with positional audio
- Pause menu handling
- Dynamic music intensity
- Footstep sounds following player
- Environmental audio zones
- Settings menu sound test

### Section 11: API Coverage Summary
- Complete checklist of all demonstrated features

## API Coverage

This demo demonstrates **100% of the IAudioSystem interface**:

### Lifecycle (1 method)
- ✅ `update(DeltaTime dt)`

### Channel-Based Audio (9 methods)
- ✅ `playOnChannel(Channel, ChannelSound)`
- ✅ `stopChannel(Channel, fadeOutTime)`
- ✅ `pauseChannel(Channel)`
- ✅ `resumeChannel(Channel)`
- ✅ `setChannelVolume(Channel, Volume)`
- ✅ `setChannelPitch(Channel, pitch)`
- ✅ `seekChannel(Channel, position)`
- ✅ `getChannelState(Channel) -> ChannelState`
- ✅ `isChannelPlaying(Channel) -> bool`

### Positional Audio (4 methods)
- ✅ `playPositional(PositionalSound) -> SoundHandle`
- ✅ `stopPositional(SoundHandle)`
- ✅ `updatePositionalPosition(SoundHandle, Vec3)`
- ✅ `isPositionalPlaying(SoundHandle) -> bool`

### 3D Audio Listener (2 methods)
- ✅ `setListener(AudioListener)`
- ✅ `getListener() -> AudioListener`

### Global Controls (5 methods)
- ✅ `setMasterVolume(Volume)`
- ✅ `getMasterVolume() -> Volume`
- ✅ `pauseAll()`
- ✅ `resumeAll()`
- ✅ `stopAll()`

### Channel Groups (2 methods)
- ✅ `setGroupVolume(groupName, Volume)`
- ✅ `assignChannelToGroup(Channel, groupName)`

**Total: 23/23 methods demonstrated (100% coverage)**

## Types Demonstrated

All audio-related types from `jframe.types`:
- ✅ `Channel` and `Channels` namespace constants
- ✅ `Volume` (float alias)
- ✅ `SoundHandle` (uint64_t alias)
- ✅ `ChannelSound` struct
- ✅ `PositionalSound` struct
- ✅ `AudioListener` struct
- ✅ `ChannelState` struct

## Notes

- This is a demonstration of the **API interface**, not a playback example
- Actual audio playback requires integration with `jframe-audio` implementation (FMOD-based)
- The demo shows typical usage patterns and best practices
- All API calls are documented with clear examples

## Related Files

- **Interface**: `/jframe-contract/src/jframe.audio.cppm`
- **Types**: `/jframe-contract/src/jframe.types.cppm`
- **Implementation**: `/jframe-audio/` (FMOD-based)

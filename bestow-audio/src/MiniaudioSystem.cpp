// bestow-audio/src/MiniaudioSystem.cpp
// Miniaudio-based audio system implementation (fallback when FMOD is unavailable)

module;

#ifdef BESTOW_HAS_MINIAUDIO
// Include stb_vorbis declarations (implementation in stb_vorbis_wrapper.c)
#define STB_VORBIS_HEADER_ONLY
#include "stb_vorbis.c"
#undef STB_VORBIS_HEADER_ONLY

#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio.h"
#endif

#include <spdlog/spdlog.h>

module bestow.audio.impl;

import std;
import bestow.assets;

namespace bestow {

#ifdef BESTOW_HAS_MINIAUDIO

// ============================================================================
// Implementation struct (pimpl)
// ============================================================================

struct MiniaudioSystem::Impl {
    struct FadeOutData {
        float targetTime = 0.0f;
        float currentTime = 0.0f;
        float startVolume = 1.0f;
        bool active = false;
    };

    struct ChannelData {
        ChannelState state;
        std::string group;
        FadeOutData fadeOut;
        ma_sound* sound = nullptr;  // Owned, must be freed
    };

    struct PositionalSoundData {
        Vec3 position;
        bool isPlaying = false;
        ma_sound* sound = nullptr;  // Owned, must be freed
        std::function<void()> onComplete;
    };

    ma_engine engine{};
    bool initialized = false;

    std::unordered_map<Channel, ChannelData> channels;
    std::unordered_map<SoundHandle, PositionalSoundData> positionalSounds;
    std::unordered_map<AssetHandle, std::string, AssetHandleHash> assetPaths;
    AudioListener listener;
    Volume masterVolume = 1.0f;
    std::unordered_map<std::string, Volume> groupVolumes;
    SoundHandle nextSoundHandle = 1;
    bool isPaused = false;

    ~Impl() {
        // Clean up all sounds
        for (auto& [channel, data] : channels) {
            if (data.sound) {
                ma_sound_uninit(data.sound);
                delete data.sound;
            }
        }
        channels.clear();

        for (auto& [handle, data] : positionalSounds) {
            if (data.sound) {
                ma_sound_uninit(data.sound);
                delete data.sound;
            }
        }
        positionalSounds.clear();

        if (initialized) {
            ma_engine_uninit(&engine);
        }
    }

    float getEffectiveVolume(const std::string& group) const {
        float groupVol = 1.0f;
        if (!group.empty()) {
            auto it = groupVolumes.find(group);
            if (it != groupVolumes.end()) {
                groupVol = it->second;
            }
        }
        return masterVolume * groupVol;
    }

    ma_sound* createSoundFromPath(const std::string& path, bool is3D, bool looping) {
        if (path.empty()) return nullptr;

        auto* sound = new ma_sound();
        ma_uint32 flags = MA_SOUND_FLAG_DECODE;
        if (!is3D) {
            flags |= MA_SOUND_FLAG_NO_SPATIALIZATION;
        }

        ma_result result = ma_sound_init_from_file(
            &engine,
            path.c_str(),
            flags,
            nullptr,  // No sound group
            nullptr,  // No fence
            sound
        );

        if (result != MA_SUCCESS) {
            spdlog::warn("MiniaudioSystem: Failed to load sound from {}: {}", path, static_cast<int>(result));
            delete sound;
            return nullptr;
        }

        ma_sound_set_looping(sound, looping ? MA_TRUE : MA_FALSE);
        return sound;
    }
};

// ============================================================================
// MiniaudioSystem implementation
// ============================================================================

MiniaudioSystem::MiniaudioSystem(IAssetSystem& assetSystem)
    : impl_(std::make_unique<Impl>()), assetSystem_(&assetSystem) {
}

MiniaudioSystem::~MiniaudioSystem() {
    shutdown();
}

bool MiniaudioSystem::initialize() {
    if (!impl_) {
        impl_ = std::make_unique<Impl>();
    }

    ma_engine_config engineConfig = ma_engine_config_init();
    engineConfig.channels = 2;
    engineConfig.sampleRate = 44100;
    engineConfig.listenerCount = 1;

    ma_result result = ma_engine_init(&engineConfig, &impl_->engine);
    if (result != MA_SUCCESS) {
        spdlog::error("MiniaudioSystem: Failed to initialize audio engine: {}", static_cast<int>(result));
        return false;
    }

    impl_->initialized = true;
    spdlog::info("MiniaudioSystem: Audio engine initialized successfully");
    return true;
}

void MiniaudioSystem::shutdown() {
    if (!impl_ || !impl_->initialized) return;

    // Clean up all sounds
    for (auto& [channel, data] : impl_->channels) {
        if (data.sound) {
            ma_sound_uninit(data.sound);
            delete data.sound;
            data.sound = nullptr;
        }
    }
    impl_->channels.clear();

    for (auto& [handle, data] : impl_->positionalSounds) {
        if (data.sound) {
            ma_sound_uninit(data.sound);
            delete data.sound;
            data.sound = nullptr;
        }
    }
    impl_->positionalSounds.clear();

    ma_engine_uninit(&impl_->engine);
    impl_->initialized = false;
    spdlog::info("MiniaudioSystem: Audio engine shut down");
}

void MiniaudioSystem::update(DeltaTime dt) {
    if (!impl_ || !impl_->initialized) return;

    // Process fade-outs and update channel states
    for (auto& [channel, data] : impl_->channels) {
        if (data.fadeOut.active) {
            data.fadeOut.currentTime += dt;

            if (data.fadeOut.currentTime >= data.fadeOut.targetTime) {
                if (data.sound) {
                    ma_sound_stop(data.sound);
                }
                data.state.isPlaying = false;
                data.state.volume = 0.0f;
                data.fadeOut.active = false;
            } else {
                float progress = data.fadeOut.currentTime / data.fadeOut.targetTime;
                float newVolume = data.fadeOut.startVolume * (1.0f - progress);
                if (data.sound) {
                    ma_sound_set_volume(data.sound, newVolume * impl_->masterVolume);
                }
                data.state.volume = newVolume;
            }
        }

        // Update channel state from sound
        if (data.sound && !data.fadeOut.active) {
            data.state.isPlaying = ma_sound_is_playing(data.sound) != 0;
            if (data.state.isPlaying) {
                ma_uint64 cursorInFrames;
                ma_sound_get_cursor_in_pcm_frames(data.sound, &cursorInFrames);
                ma_uint32 sampleRate = ma_engine_get_sample_rate(&impl_->engine);
                data.state.position = static_cast<float>(cursorInFrames) / static_cast<float>(sampleRate);
            }
        }
    }

    // Clean up finished positional sounds
    std::vector<SoundHandle> toRemove;
    for (auto& [handle, data] : impl_->positionalSounds) {
        if (data.sound) {
            if (!ma_sound_is_playing(data.sound)) {
                toRemove.push_back(handle);
            }
        }
    }
    for (auto handle : toRemove) {
        stopPositional(handle);
    }
}

void MiniaudioSystem::playOnChannel(Channel channel, const ChannelSound& sound) {
    if (!impl_) return;

    if (!impl_->initialized) {
        // Stub mode fallback
        auto& data = impl_->channels[channel];
        data.state.isPlaying = true;
        data.state.isPaused = false;
        data.state.volume = sound.volume;
        data.fadeOut.active = false;
        return;
    }

    auto& data = impl_->channels[channel];
    data.fadeOut.active = false;

    // Stop and clean up existing sound
    if (data.sound) {
        ma_sound_uninit(data.sound);
        delete data.sound;
        data.sound = nullptr;
    }

    // Get asset path
    std::string path;
    if (assetSystem_) {
        auto metadata = assetSystem_->getAssetMetadata(sound.asset);
        if (!metadata.sourcePath.empty()) {
            path = metadata.sourcePath.string();
            impl_->assetPaths[sound.asset] = path;
        }
    }

    // Fallback to cached path
    if (path.empty()) {
        auto it = impl_->assetPaths.find(sound.asset);
        if (it != impl_->assetPaths.end()) {
            path = it->second;
        }
    }

    // Create new sound
    data.sound = impl_->createSoundFromPath(path, false, sound.looping);
    if (!data.sound) {
        // Track state even if sound creation failed
        data.state.isPlaying = true;
        data.state.isPaused = false;
        data.state.volume = sound.volume;
        spdlog::warn("MiniaudioSystem: Failed to create sound for channel {}", channel);
        return;
    }

    // Configure sound
    ma_sound_set_volume(data.sound, sound.volume * impl_->getEffectiveVolume(data.group));
    ma_sound_set_pitch(data.sound, sound.pitch);

    // Set start position if specified
    if (sound.startTime.has_value()) {
        ma_uint32 sampleRate = ma_engine_get_sample_rate(&impl_->engine);
        ma_uint64 frameOffset = static_cast<ma_uint64>(sound.startTime.value() * sampleRate);
        ma_sound_seek_to_pcm_frame(data.sound, frameOffset);
    }

    // Start playback
    ma_sound_start(data.sound);

    // Get sound length
    ma_uint64 lengthInFrames;
    ma_sound_get_length_in_pcm_frames(data.sound, &lengthInFrames);
    ma_uint32 sampleRate = ma_engine_get_sample_rate(&impl_->engine);
    data.state.length = static_cast<float>(lengthInFrames) / static_cast<float>(sampleRate);

    // Update state
    data.state.isPlaying = true;
    data.state.isPaused = false;
    data.state.volume = sound.volume;
}

void MiniaudioSystem::stopChannel(Channel channel, float fadeOutTime) {
    if (!impl_) return;

    auto it = impl_->channels.find(channel);
    if (it == impl_->channels.end()) return;

    if (fadeOutTime > 0.0f && it->second.state.isPlaying) {
        it->second.fadeOut.active = true;
        it->second.fadeOut.targetTime = fadeOutTime;
        it->second.fadeOut.currentTime = 0.0f;
        it->second.fadeOut.startVolume = it->second.state.volume;
    } else {
        if (it->second.sound) {
            ma_sound_stop(it->second.sound);
        }
        it->second.state.isPlaying = false;
    }
}

void MiniaudioSystem::pauseChannel(Channel channel) {
    if (!impl_) return;

    auto& data = impl_->channels[channel];
    if (data.sound) {
        ma_sound_stop(data.sound);
    }
    data.state.isPaused = true;
}

void MiniaudioSystem::resumeChannel(Channel channel) {
    if (!impl_) return;

    auto& data = impl_->channels[channel];
    if (data.sound && data.state.isPaused) {
        ma_sound_start(data.sound);
    }
    data.state.isPaused = false;
}

void MiniaudioSystem::setChannelVolume(Channel channel, Volume volume) {
    if (!impl_) return;

    auto& data = impl_->channels[channel];
    data.state.volume = volume;
    if (data.sound) {
        ma_sound_set_volume(data.sound, volume * impl_->getEffectiveVolume(data.group));
    }
}

void MiniaudioSystem::setChannelPitch(Channel channel, float pitch) {
    if (!impl_) return;

    auto it = impl_->channels.find(channel);
    if (it != impl_->channels.end() && it->second.sound) {
        ma_sound_set_pitch(it->second.sound, pitch);
    }
}

void MiniaudioSystem::seekChannel(Channel channel, float position) {
    if (!impl_) return;

    auto& data = impl_->channels[channel];
    data.state.position = position;
    if (data.sound) {
        ma_uint32 sampleRate = ma_engine_get_sample_rate(&impl_->engine);
        ma_uint64 frameOffset = static_cast<ma_uint64>(position * sampleRate);
        ma_sound_seek_to_pcm_frame(data.sound, frameOffset);
    }
}

ChannelState MiniaudioSystem::getChannelState(Channel channel) const {
    if (!impl_) return {};

    auto it = impl_->channels.find(channel);
    if (it != impl_->channels.end()) {
        return it->second.state;
    }
    return {};
}

bool MiniaudioSystem::isChannelPlaying(Channel channel) const {
    if (!impl_) return false;

    auto it = impl_->channels.find(channel);
    if (it != impl_->channels.end()) {
        return it->second.state.isPlaying && !it->second.state.isPaused;
    }
    return false;
}

SoundHandle MiniaudioSystem::playPositional(const PositionalSound& sound) {
    if (!impl_) return 0;

    SoundHandle handle = impl_->nextSoundHandle++;

    if (!impl_->initialized) {
        // Stub mode
        impl_->positionalSounds[handle].position = sound.position;
        impl_->positionalSounds[handle].isPlaying = true;
        return handle;
    }

    auto& data = impl_->positionalSounds[handle];
    data.position = sound.position;

    // Get asset path
    std::string path;
    if (assetSystem_) {
        auto metadata = assetSystem_->getAssetMetadata(sound.asset);
        if (!metadata.sourcePath.empty()) {
            path = metadata.sourcePath.string();
            impl_->assetPaths[sound.asset] = path;
        }
    }

    if (path.empty()) {
        auto it = impl_->assetPaths.find(sound.asset);
        if (it != impl_->assetPaths.end()) {
            path = it->second;
        }
    }

    // Create 3D sound
    data.sound = impl_->createSoundFromPath(path, true, false);
    if (!data.sound) {
        data.isPlaying = true;  // Track in stub mode
        return handle;
    }

    // Configure 3D properties
    ma_sound_set_position(data.sound, sound.position.x, sound.position.y, sound.position.z);
    ma_sound_set_volume(data.sound, sound.volume * impl_->masterVolume);
    ma_sound_set_pitch(data.sound, sound.pitch);
    ma_sound_set_min_distance(data.sound, sound.minDistance);
    ma_sound_set_max_distance(data.sound, sound.maxDistance);
    ma_sound_set_attenuation_model(data.sound, ma_attenuation_model_inverse);

    if (sound.velocity.has_value()) {
        ma_sound_set_velocity(data.sound, sound.velocity->x, sound.velocity->y, sound.velocity->z);
    }

    // Start playback
    ma_sound_start(data.sound);
    data.isPlaying = true;
    data.onComplete = sound.onComplete;

    return handle;
}

void MiniaudioSystem::stopPositional(SoundHandle handle) {
    if (!impl_) return;

    auto it = impl_->positionalSounds.find(handle);
    if (it == impl_->positionalSounds.end()) return;

    if (it->second.sound) {
        ma_sound_stop(it->second.sound);
        ma_sound_uninit(it->second.sound);
        delete it->second.sound;
    }

    // Call completion callback if set
    if (it->second.onComplete) {
        it->second.onComplete();
    }

    impl_->positionalSounds.erase(it);
}

void MiniaudioSystem::updatePositionalPosition(SoundHandle handle, Vec3 position) {
    if (!impl_) return;

    auto it = impl_->positionalSounds.find(handle);
    if (it == impl_->positionalSounds.end()) return;

    it->second.position = position;
    if (it->second.sound) {
        ma_sound_set_position(it->second.sound, position.x, position.y, position.z);
    }
}

bool MiniaudioSystem::isPositionalPlaying(SoundHandle handle) const {
    if (!impl_) return false;

    auto it = impl_->positionalSounds.find(handle);
    if (it == impl_->positionalSounds.end()) return false;

    if (it->second.sound) {
        return ma_sound_is_playing(it->second.sound) != 0;
    }
    return it->second.isPlaying;
}

void MiniaudioSystem::setListener(const AudioListener& listener) {
    if (!impl_) return;

    impl_->listener = listener;

    if (!impl_->initialized) return;

    ma_engine_listener_set_position(&impl_->engine, 0,
                                    listener.position.x, listener.position.y, listener.position.z);
    ma_engine_listener_set_direction(&impl_->engine, 0,
                                     listener.forward.x, listener.forward.y, listener.forward.z);
    ma_engine_listener_set_world_up(&impl_->engine, 0,
                                    listener.up.x, listener.up.y, listener.up.z);
    ma_engine_listener_set_velocity(&impl_->engine, 0,
                                    listener.velocity.x, listener.velocity.y, listener.velocity.z);
}

AudioListener MiniaudioSystem::getListener() const {
    if (!impl_) return {};
    return impl_->listener;
}

void MiniaudioSystem::setMasterVolume(Volume volume) {
    if (!impl_) return;

    impl_->masterVolume = volume;

    if (impl_->initialized) {
        ma_engine_set_volume(&impl_->engine, volume);
    }
}

Volume MiniaudioSystem::getMasterVolume() const {
    if (!impl_) return 1.0f;
    return impl_->masterVolume;
}

void MiniaudioSystem::pauseAll() {
    if (!impl_) return;

    impl_->isPaused = true;

    for (auto& [channel, data] : impl_->channels) {
        if (data.state.isPlaying && data.sound) {
            ma_sound_stop(data.sound);
        }
        if (data.state.isPlaying) {
            data.state.isPaused = true;
        }
    }

    for (auto& [handle, data] : impl_->positionalSounds) {
        if (data.sound) {
            ma_sound_stop(data.sound);
        }
    }
}

void MiniaudioSystem::resumeAll() {
    if (!impl_) return;

    impl_->isPaused = false;

    for (auto& [channel, data] : impl_->channels) {
        if (data.state.isPlaying && data.state.isPaused && data.sound) {
            ma_sound_start(data.sound);
        }
        if (data.state.isPlaying) {
            data.state.isPaused = false;
        }
    }

    for (auto& [handle, data] : impl_->positionalSounds) {
        if (data.sound && data.isPlaying) {
            ma_sound_start(data.sound);
        }
    }
}

void MiniaudioSystem::stopAll() {
    if (!impl_) return;

    // Stop all channel sounds
    for (auto& [channel, data] : impl_->channels) {
        if (data.sound) {
            ma_sound_stop(data.sound);
            ma_sound_uninit(data.sound);
            delete data.sound;
            data.sound = nullptr;
        }
    }
    impl_->channels.clear();

    // Stop all positional sounds
    for (auto& [handle, data] : impl_->positionalSounds) {
        if (data.sound) {
            ma_sound_stop(data.sound);
            ma_sound_uninit(data.sound);
            delete data.sound;
            data.sound = nullptr;
        }
    }
    impl_->positionalSounds.clear();
}

void MiniaudioSystem::setGroupVolume(const std::string& group, Volume volume) {
    if (!impl_) return;

    impl_->groupVolumes[group] = volume;

    // Update all channels in this group
    for (auto& [channel, data] : impl_->channels) {
        if (data.group == group && data.sound) {
            ma_sound_set_volume(data.sound, data.state.volume * impl_->getEffectiveVolume(group));
        }
    }
}

void MiniaudioSystem::assignChannelToGroup(Channel channel, const std::string& group) {
    if (!impl_) return;

    auto& data = impl_->channels[channel];
    data.group = group;

    // Update volume if sound is active
    if (data.sound) {
        ma_sound_set_volume(data.sound, data.state.volume * impl_->getEffectiveVolume(group));
    }
}

void MiniaudioSystem::invalidateSoundCache() {
    if (!impl_) return;

    stopAll();
    impl_->assetPaths.clear();
}

#endif  // BESTOW_HAS_MINIAUDIO

}  // namespace bestow

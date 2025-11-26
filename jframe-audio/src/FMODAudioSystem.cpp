// jframe-audio/src/FMODAudioSystem.cpp
// FMOD audio system implementation

module;

#ifdef JFRAME_HAS_FMOD
#include <fmod.h>
#include <fmod_errors.h>
#endif

module jframe.audio.impl;

import std;

namespace jframe {

namespace {
#ifdef JFRAME_HAS_FMOD
    // Helper to check FMOD results and log errors
    bool checkFMODResult(FMOD_RESULT result, const char* context) {
        if (result != FMOD_OK) {
            std::cerr << "FMOD Error [" << context << "]: "
                      << FMOD_ErrorString(result) << std::endl;
            return false;
        }
        return true;
    }

    // Convert Vec3 to FMOD_VECTOR
    FMOD_VECTOR toFMODVector(const Vec3& v) {
        return {v.x, v.y, v.z};
    }
#endif
}  // anonymous namespace

FMODAudioSystem::~FMODAudioSystem() {
#ifdef JFRAME_HAS_FMOD
    if (fmodSystem_) {
        // Release all channel groups
        for (auto& [name, group] : fmodGroups_) {
            if (group) {
                FMOD_ChannelGroup_Release(group);
            }
        }
        fmodGroups_.clear();

        // Release all sounds
        for (auto& [channel, data] : channels_) {
            if (data.fmodSound) {
                FMOD_Sound_Release(data.fmodSound);
            }
        }
        for (auto& [handle, data] : positionalSounds_) {
            if (data.fmodSound) {
                FMOD_Sound_Release(data.fmodSound);
            }
        }

        // Close and release the system
        FMOD_System_Close(fmodSystem_);
        FMOD_System_Release(fmodSystem_);
        fmodSystem_ = nullptr;
    }
#endif
}

bool FMODAudioSystem::initialize() {
#ifdef JFRAME_HAS_FMOD
    // Create FMOD system
    FMOD_RESULT result = FMOD_System_Create(&fmodSystem_, FMOD_VERSION);
    if (!checkFMODResult(result, "FMOD_System_Create")) {
        return false;
    }

    // Initialize with 512 channels, default flags, and no extra driver data
    result = FMOD_System_Init(fmodSystem_, 512, FMOD_INIT_NORMAL, nullptr);
    if (!checkFMODResult(result, "FMOD_System_Init")) {
        FMOD_System_Release(fmodSystem_);
        fmodSystem_ = nullptr;
        return false;
    }

    // Get the master channel group
    result = FMOD_System_GetMasterChannelGroup(fmodSystem_, &masterGroup_);
    if (!checkFMODResult(result, "FMOD_System_GetMasterChannelGroup")) {
        return false;
    }

    // Set up 3D settings
    result = FMOD_System_Set3DSettings(fmodSystem_, 1.0f, 1.0f, 1.0f);
    checkFMODResult(result, "FMOD_System_Set3DSettings");

    std::cout << "FMOD Audio System initialized successfully" << std::endl;
    return true;
#else
    std::cout << "FMOD not available - Audio system running in stub mode" << std::endl;
    return true;
#endif
}

void FMODAudioSystem::update(DeltaTime dt) {
#ifdef JFRAME_HAS_FMOD
    if (fmodSystem_) {
        // Update FMOD system (processes 3D audio, virtual channels, etc.)
        FMOD_System_Update(fmodSystem_);

        // Update channel states by querying FMOD
        for (auto& [channel, data] : channels_) {
            if (data.fmodChannel) {
                FMOD_BOOL isPlaying = 0;
                FMOD_Channel_IsPlaying(data.fmodChannel, &isPlaying);
                data.state.isPlaying = (isPlaying != 0);

                if (isPlaying) {
                    FMOD_BOOL isPaused = 0;
                    FMOD_Channel_GetPaused(data.fmodChannel, &isPaused);
                    data.state.isPaused = (isPaused != 0);

                    unsigned int position = 0;
                    FMOD_Channel_GetPosition(data.fmodChannel, &position, FMOD_TIMEUNIT_MS);
                    data.state.position = static_cast<float>(position) / 1000.0f;

                    float volume = 0.0f;
                    FMOD_Channel_GetVolume(data.fmodChannel, &volume);
                    data.state.volume = volume;
                }
            }
        }

        // Clean up finished positional sounds
        std::vector<SoundHandle> toRemove;
        for (auto& [handle, data] : positionalSounds_) {
            if (data.fmodChannel) {
                FMOD_BOOL isPlaying = 0;
                FMOD_Channel_IsPlaying(data.fmodChannel, &isPlaying);
                if (!isPlaying) {
                    toRemove.push_back(handle);
                }
            }
        }
        for (auto handle : toRemove) {
            stopPositional(handle);
        }
    }
#endif
}

void FMODAudioSystem::playOnChannel(Channel channel, const ChannelSound& sound) {
#ifdef JFRAME_HAS_FMOD
    if (!fmodSystem_) {
        return;
    }

    auto& channelData = channels_[channel];

    // Stop any currently playing sound on this channel
    if (channelData.fmodChannel) {
        FMOD_Channel_Stop(channelData.fmodChannel);
        channelData.fmodChannel = nullptr;
    }

    // Release old sound if exists
    if (channelData.fmodSound) {
        FMOD_Sound_Release(channelData.fmodSound);
        channelData.fmodSound = nullptr;
    }

    // For now, we need asset paths to be registered separately
    // In a real implementation, this would come from the asset system
    // TODO(agent): Need asset path lookup from asset system
    auto pathIt = assetPaths_.find(sound.asset);
    if (pathIt == assetPaths_.end()) {
        std::cerr << "Audio asset " << sound.asset << " not registered" << std::endl;
        return;
    }

    // Create sound
    FMOD_MODE mode = FMOD_DEFAULT;
    if (sound.looping) {
        mode |= FMOD_LOOP_NORMAL;
    } else {
        mode |= FMOD_LOOP_OFF;
    }

    FMOD_RESULT result = FMOD_System_CreateSound(
        fmodSystem_,
        pathIt->second.c_str(),
        mode,
        nullptr,
        &channelData.fmodSound
    );

    if (!checkFMODResult(result, "FMOD_System_CreateSound")) {
        return;
    }

    // Play the sound
    result = FMOD_System_PlaySound(
        fmodSystem_,
        channelData.fmodSound,
        nullptr,  // No channel group yet
        false,    // Don't start paused
        &channelData.fmodChannel
    );

    if (!checkFMODResult(result, "FMOD_System_PlaySound")) {
        FMOD_Sound_Release(channelData.fmodSound);
        channelData.fmodSound = nullptr;
        return;
    }

    // Set volume
    FMOD_Channel_SetVolume(channelData.fmodChannel, sound.volume);

    // Set pitch
    FMOD_Channel_SetPitch(channelData.fmodChannel, sound.pitch);

    // Set start time if specified
    if (sound.startTime.has_value()) {
        unsigned int positionMs = static_cast<unsigned int>(sound.startTime.value() * 1000.0f);
        FMOD_Channel_SetPosition(channelData.fmodChannel, positionMs, FMOD_TIMEUNIT_MS);
    }

    // Assign to group if already assigned
    if (!channelData.group.empty()) {
        auto groupIt = fmodGroups_.find(channelData.group);
        if (groupIt != fmodGroups_.end() && groupIt->second) {
            FMOD_Channel_SetChannelGroup(channelData.fmodChannel, groupIt->second);
        }
    }

    // Update state
    channelData.state.isPlaying = true;
    channelData.state.isPaused = false;
    channelData.state.volume = sound.volume;
#else
    channels_[channel].state.isPlaying = true;
    channels_[channel].state.volume = sound.volume;
#endif
}

void FMODAudioSystem::stopChannel(Channel channel, float fadeOutTime) {
#ifdef JFRAME_HAS_FMOD
    if (auto it = channels_.find(channel); it != channels_.end()) {
        if (it->second.fmodChannel) {
            if (fadeOutTime > 0.0f) {
                // TODO: Implement fade out using FMOD DSP or volume ramping
                // For now, just stop immediately
                FMOD_Channel_Stop(it->second.fmodChannel);
            } else {
                FMOD_Channel_Stop(it->second.fmodChannel);
            }
            it->second.fmodChannel = nullptr;
        }
        it->second.state.isPlaying = false;
    }
#else
    if (auto it = channels_.find(channel); it != channels_.end()) {
        it->second.state.isPlaying = false;
    }
#endif
}

void FMODAudioSystem::pauseChannel(Channel channel) {
#ifdef JFRAME_HAS_FMOD
    if (auto it = channels_.find(channel); it != channels_.end()) {
        if (it->second.fmodChannel) {
            FMOD_Channel_SetPaused(it->second.fmodChannel, true);
        }
        it->second.state.isPaused = true;
    }
#else
    if (auto it = channels_.find(channel); it != channels_.end()) {
        it->second.state.isPaused = true;
    }
#endif
}

void FMODAudioSystem::resumeChannel(Channel channel) {
#ifdef JFRAME_HAS_FMOD
    if (auto it = channels_.find(channel); it != channels_.end()) {
        if (it->second.fmodChannel) {
            FMOD_Channel_SetPaused(it->second.fmodChannel, false);
        }
        it->second.state.isPaused = false;
    }
#else
    if (auto it = channels_.find(channel); it != channels_.end()) {
        it->second.state.isPaused = false;
    }
#endif
}

void FMODAudioSystem::setChannelVolume(Channel channel, Volume volume) {
#ifdef JFRAME_HAS_FMOD
    auto& channelData = channels_[channel];
    channelData.state.volume = volume;
    if (channelData.fmodChannel) {
        FMOD_Channel_SetVolume(channelData.fmodChannel, volume);
    }
#else
    channels_[channel].state.volume = volume;
#endif
}

void FMODAudioSystem::setChannelPitch(Channel channel, float pitch) {
#ifdef JFRAME_HAS_FMOD
    if (auto it = channels_.find(channel); it != channels_.end()) {
        if (it->second.fmodChannel) {
            FMOD_Channel_SetPitch(it->second.fmodChannel, pitch);
        }
    }
#endif
}

void FMODAudioSystem::seekChannel(Channel channel, float position) {
#ifdef JFRAME_HAS_FMOD
    auto& channelData = channels_[channel];
    channelData.state.position = position;
    if (channelData.fmodChannel) {
        unsigned int positionMs = static_cast<unsigned int>(position * 1000.0f);
        FMOD_Channel_SetPosition(channelData.fmodChannel, positionMs, FMOD_TIMEUNIT_MS);
    }
#else
    channels_[channel].state.position = position;
#endif
}

ChannelState FMODAudioSystem::getChannelState(Channel channel) const {
    if (auto it = channels_.find(channel); it != channels_.end()) {
        return it->second.state;
    }
    return {};
}

bool FMODAudioSystem::isChannelPlaying(Channel channel) const {
    if (auto it = channels_.find(channel); it != channels_.end()) {
        return it->second.state.isPlaying && !it->second.state.isPaused;
    }
    return false;
}

SoundHandle FMODAudioSystem::playPositional(const PositionalSound& sound) {
    SoundHandle handle = nextSoundHandle_++;

#ifdef JFRAME_HAS_FMOD
    if (!fmodSystem_) {
        return handle;
    }

    auto& soundData = positionalSounds_[handle];
    soundData.position = sound.position;

    // Get asset path
    auto pathIt = assetPaths_.find(sound.asset);
    if (pathIt == assetPaths_.end()) {
        std::cerr << "Audio asset " << sound.asset << " not registered" << std::endl;
        return handle;
    }

    // Create 3D sound
    FMOD_RESULT result = FMOD_System_CreateSound(
        fmodSystem_,
        pathIt->second.c_str(),
        FMOD_3D | FMOD_LOOP_OFF,
        nullptr,
        &soundData.fmodSound
    );

    if (!checkFMODResult(result, "FMOD_System_CreateSound (3D)")) {
        positionalSounds_.erase(handle);
        return 0;
    }

    // Set 3D min/max distance
    FMOD_Sound_Set3DMinMaxDistance(soundData.fmodSound, sound.minDistance, sound.maxDistance);

    // Play the sound
    result = FMOD_System_PlaySound(
        fmodSystem_,
        soundData.fmodSound,
        nullptr,
        false,
        &soundData.fmodChannel
    );

    if (!checkFMODResult(result, "FMOD_System_PlaySound (3D)")) {
        FMOD_Sound_Release(soundData.fmodSound);
        positionalSounds_.erase(handle);
        return 0;
    }

    // Set volume and pitch
    FMOD_Channel_SetVolume(soundData.fmodChannel, sound.volume);
    FMOD_Channel_SetPitch(soundData.fmodChannel, sound.pitch);

    // Set 3D position
    FMOD_VECTOR pos = toFMODVector(sound.position);
    FMOD_VECTOR vel = {0, 0, 0};
    if (sound.velocity.has_value()) {
        vel = toFMODVector(sound.velocity.value());
    }
    FMOD_Channel_Set3DAttributes(soundData.fmodChannel, &pos, &vel);

#else
    positionalSounds_[handle].position = sound.position;
#endif

    return handle;
}

void FMODAudioSystem::stopPositional(SoundHandle handle) {
#ifdef JFRAME_HAS_FMOD
    if (auto it = positionalSounds_.find(handle); it != positionalSounds_.end()) {
        if (it->second.fmodChannel) {
            FMOD_Channel_Stop(it->second.fmodChannel);
        }
        if (it->second.fmodSound) {
            FMOD_Sound_Release(it->second.fmodSound);
        }
        positionalSounds_.erase(it);
    }
#else
    positionalSounds_.erase(handle);
#endif
}

void FMODAudioSystem::updatePositionalPosition(SoundHandle handle, Vec3 position) {
#ifdef JFRAME_HAS_FMOD
    if (auto it = positionalSounds_.find(handle); it != positionalSounds_.end()) {
        it->second.position = position;
        if (it->second.fmodChannel) {
            FMOD_VECTOR pos = toFMODVector(position);
            FMOD_VECTOR vel = {0, 0, 0};
            FMOD_Channel_Set3DAttributes(it->second.fmodChannel, &pos, &vel);
        }
    }
#else
    if (auto it = positionalSounds_.find(handle); it != positionalSounds_.end()) {
        it->second.position = position;
    }
#endif
}

bool FMODAudioSystem::isPositionalPlaying(SoundHandle handle) const {
#ifdef JFRAME_HAS_FMOD
    if (auto it = positionalSounds_.find(handle); it != positionalSounds_.end()) {
        if (it->second.fmodChannel) {
            FMOD_BOOL isPlaying = 0;
            FMOD_Channel_IsPlaying(it->second.fmodChannel, &isPlaying);
            return isPlaying != 0;
        }
    }
    return false;
#else
    return positionalSounds_.contains(handle);
#endif
}

void FMODAudioSystem::setListener(const AudioListener& listener) {
    listener_ = listener;

#ifdef JFRAME_HAS_FMOD
    if (fmodSystem_) {
        FMOD_VECTOR pos = toFMODVector(listener.position);
        FMOD_VECTOR vel = toFMODVector(listener.velocity);
        FMOD_VECTOR forward = toFMODVector(listener.forward);
        FMOD_VECTOR up = toFMODVector(listener.up);

        FMOD_System_Set3DListenerAttributes(fmodSystem_, 0, &pos, &vel, &forward, &up);
    }
#endif
}

AudioListener FMODAudioSystem::getListener() const {
    return listener_;
}

void FMODAudioSystem::setMasterVolume(Volume volume) {
    masterVolume_ = volume;

#ifdef JFRAME_HAS_FMOD
    if (masterGroup_) {
        FMOD_ChannelGroup_SetVolume(masterGroup_, volume);
    }
#endif
}

Volume FMODAudioSystem::getMasterVolume() const {
    return masterVolume_;
}

void FMODAudioSystem::pauseAll() {
    isPaused_ = true;

#ifdef JFRAME_HAS_FMOD
    if (masterGroup_) {
        FMOD_ChannelGroup_SetPaused(masterGroup_, true);
    }
#endif
}

void FMODAudioSystem::resumeAll() {
    isPaused_ = false;

#ifdef JFRAME_HAS_FMOD
    if (masterGroup_) {
        FMOD_ChannelGroup_SetPaused(masterGroup_, false);
    }
#endif
}

void FMODAudioSystem::stopAll() {
#ifdef JFRAME_HAS_FMOD
    // Stop all channel-based sounds
    for (auto& [channel, data] : channels_) {
        if (data.fmodChannel) {
            FMOD_Channel_Stop(data.fmodChannel);
        }
        if (data.fmodSound) {
            FMOD_Sound_Release(data.fmodSound);
        }
    }

    // Stop all positional sounds
    for (auto& [handle, data] : positionalSounds_) {
        if (data.fmodChannel) {
            FMOD_Channel_Stop(data.fmodChannel);
        }
        if (data.fmodSound) {
            FMOD_Sound_Release(data.fmodSound);
        }
    }
#endif

    channels_.clear();
    positionalSounds_.clear();
}

void FMODAudioSystem::setGroupVolume(const std::string& group, Volume volume) {
    groupVolumes_[group] = volume;

#ifdef JFRAME_HAS_FMOD
    // Create group if it doesn't exist
    auto& fmodGroup = fmodGroups_[group];
    if (!fmodGroup && fmodSystem_) {
        FMOD_RESULT result = FMOD_System_CreateChannelGroup(fmodSystem_, group.c_str(), &fmodGroup);
        if (checkFMODResult(result, "FMOD_System_CreateChannelGroup")) {
            // Add to master group
            if (masterGroup_) {
                FMOD_ChannelGroup_AddGroup(masterGroup_, fmodGroup, false, nullptr);
            }
        }
    }

    // Set volume
    if (fmodGroup) {
        FMOD_ChannelGroup_SetVolume(fmodGroup, volume);
    }
#endif
}

void FMODAudioSystem::assignChannelToGroup(Channel channel, const std::string& group) {
    auto& channelData = channels_[channel];
    channelData.group = group;

#ifdef JFRAME_HAS_FMOD
    // Create group if it doesn't exist
    auto& fmodGroup = fmodGroups_[group];
    if (!fmodGroup && fmodSystem_) {
        FMOD_RESULT result = FMOD_System_CreateChannelGroup(fmodSystem_, group.c_str(), &fmodGroup);
        if (checkFMODResult(result, "FMOD_System_CreateChannelGroup")) {
            // Add to master group
            if (masterGroup_) {
                FMOD_ChannelGroup_AddGroup(masterGroup_, fmodGroup, false, nullptr);
            }
            // Set volume if already specified
            auto volIt = groupVolumes_.find(group);
            if (volIt != groupVolumes_.end()) {
                FMOD_ChannelGroup_SetVolume(fmodGroup, volIt->second);
            }
        }
    }

    // Assign channel to group if it's currently playing
    if (channelData.fmodChannel && fmodGroup) {
        FMOD_Channel_SetChannelGroup(channelData.fmodChannel, fmodGroup);
    }
#endif
}

}  // namespace jframe

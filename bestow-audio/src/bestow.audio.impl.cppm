// bestow-audio/src/bestow.audio.impl.cppm
// Audio system implementation using FMOD

module;

#include <kangaru/kangaru.hpp>

#ifdef BESTOW_HAS_FMOD
#include <fmod.h>
#endif

export module bestow.audio.impl;

import std;
import bestow.audio;
import bestow.assets;
import bestow.types;
import bestow.services;

export namespace bestow {

class FMODAudioSystem : public IAudioSystem {
public:
    explicit FMODAudioSystem(IAssetSystem* assetSystem = nullptr);
    ~FMODAudioSystem() override;

    bool initialize();

    void update(DeltaTime dt) override;

    // Channel-based audio
    void playOnChannel(Channel channel, const ChannelSound& sound) override;
    void stopChannel(Channel channel, float fadeOutTime = 0.0f) override;
    void pauseChannel(Channel channel) override;
    void resumeChannel(Channel channel) override;
    void setChannelVolume(Channel channel, Volume volume) override;
    void setChannelPitch(Channel channel, float pitch) override;
    void seekChannel(Channel channel, float position) override;
    ChannelState getChannelState(Channel channel) const override;
    bool isChannelPlaying(Channel channel) const override;

    // Positional audio
    SoundHandle playPositional(const PositionalSound& sound) override;
    void stopPositional(SoundHandle handle) override;
    void updatePositionalPosition(SoundHandle handle, Vec3 position) override;
    bool isPositionalPlaying(SoundHandle handle) const override;

    // Listener
    void setListener(const AudioListener& listener) override;
    AudioListener getListener() const override;

    // Global controls
    void setMasterVolume(Volume volume) override;
    Volume getMasterVolume() const override;
    void pauseAll() override;
    void resumeAll() override;
    void stopAll() override;

    // Channel groups
    void setGroupVolume(const std::string& group, Volume volume) override;
    void assignChannelToGroup(Channel channel, const std::string& group) override;

    // Hot reload support
    void invalidateSoundCache();

private:
    struct ChannelData {
        ChannelState state;
        std::string group;
#ifdef BESTOW_HAS_FMOD
        FMOD_CHANNEL* fmodChannel = nullptr;
#endif
    };

    struct PositionalSoundData {
        Vec3 position;
        bool isPlaying = false;
#ifdef BESTOW_HAS_FMOD
        FMOD_CHANNEL* fmodChannel = nullptr;
#endif
    };

#ifdef BESTOW_HAS_FMOD
    // Helper to get or create FMOD sound from asset system
    FMOD_SOUND* getOrCreateSound(AssetHandle handle, FMOD_MODE mode);
#endif

    std::unordered_map<Channel, ChannelData> channels_;
    std::unordered_map<SoundHandle, PositionalSoundData> positionalSounds_;
    std::unordered_map<AssetHandle, std::string, AssetHandleHash> assetPaths_;  // Fallback for file paths
    AudioListener listener_;
    Volume masterVolume_ = 1.0f;
    std::unordered_map<std::string, Volume> groupVolumes_;
    SoundHandle nextSoundHandle_ = 1;
    bool isPaused_ = false;
    IAssetSystem* assetSystem_ = nullptr;

#ifdef BESTOW_HAS_FMOD
    FMOD_SYSTEM* fmodSystem_ = nullptr;
    FMOD_CHANNELGROUP* masterGroup_ = nullptr;
    std::unordered_map<std::string, FMOD_CHANNELGROUP*> fmodGroups_;
    std::unordered_map<AssetHandle, FMOD_SOUND*, AssetHandleHash> soundCache_;  // Cached FMOD sounds
#endif
};

// Kangaru service definitions
// Concrete service that provides FMODAudioSystem as IAudioSystem
struct AudioSystemService : kgr::single_service<FMODAudioSystem>, kgr::overrides<IAudioSystemService> {};

}  // namespace bestow

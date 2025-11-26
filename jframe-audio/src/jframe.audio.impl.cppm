// jframe-audio/src/jframe.audio.impl.cppm
// Audio system implementation using FMOD

module;

#ifdef JFRAME_HAS_FMOD
#include <fmod.h>
#endif

export module jframe.audio.impl;

import std;
import jframe.audio;
import jframe.types;

export namespace jframe {

class FMODAudioSystem : public IAudioSystem {
public:
    FMODAudioSystem() = default;
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

    // Asset registration (temporary until asset system integration)
    // TODO(agent): Remove when asset system provides path lookup
    void registerAudioAsset(AssetHandle handle, const std::string& filepath) {
        assetPaths_[handle] = filepath;
    }

private:
    struct ChannelData {
        ChannelState state;
        std::string group;
#ifdef JFRAME_HAS_FMOD
        FMOD_CHANNEL* fmodChannel = nullptr;
        FMOD_SOUND* fmodSound = nullptr;
#endif
    };

    struct PositionalSoundData {
        Vec3 position;
#ifdef JFRAME_HAS_FMOD
        FMOD_CHANNEL* fmodChannel = nullptr;
        FMOD_SOUND* fmodSound = nullptr;
#endif
    };

    std::unordered_map<Channel, ChannelData> channels_;
    std::unordered_map<SoundHandle, PositionalSoundData> positionalSounds_;
    std::unordered_map<AssetHandle, std::string, AssetHandleHash> assetPaths_;
    AudioListener listener_;
    Volume masterVolume_ = 1.0f;
    std::unordered_map<std::string, Volume> groupVolumes_;
    SoundHandle nextSoundHandle_ = 1;
    bool isPaused_ = false;

#ifdef JFRAME_HAS_FMOD
    FMOD_SYSTEM* fmodSystem_ = nullptr;
    FMOD_CHANNELGROUP* masterGroup_ = nullptr;
    std::unordered_map<std::string, FMOD_CHANNELGROUP*> fmodGroups_;
#endif
};

// Factory function (exported via namespace)
inline std::unique_ptr<IAudioSystem> createAudioSystem() {
    return std::make_unique<FMODAudioSystem>();
}

}  // namespace jframe

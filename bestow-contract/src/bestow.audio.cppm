// bestow-contract/src/bestow.audio.cppm
// Audio system interface

module;

#include <string>

export module bestow.audio;

import bestow.types;

export namespace bestow {

struct ChannelState {
    bool isPlaying = false;
    bool isPaused = false;
    float position = 0.0f;
    float length = 0.0f;
    Volume volume = 1.0f;
};

class IAudioSystem {
public:
    virtual ~IAudioSystem() = default;

    //======================================================================
    // Lifecycle
    //======================================================================

    virtual bool initialize() = 0;
    virtual void shutdown() = 0;
    virtual void update(DeltaTime dt) = 0;

    //======================================================================
    // Channel-Based Audio (Managed)
    //======================================================================

    virtual void playOnChannel(Channel channel, const ChannelSound& sound) = 0;
    virtual void stopChannel(Channel channel, float fadeOutTime = 0.0f) = 0;
    virtual void pauseChannel(Channel channel) = 0;
    virtual void resumeChannel(Channel channel) = 0;

    virtual void setChannelVolume(Channel channel, Volume volume) = 0;
    virtual void setChannelPitch(Channel channel, float pitch) = 0;
    virtual void seekChannel(Channel channel, float position) = 0;

    virtual ChannelState getChannelState(Channel channel) const = 0;
    virtual bool isChannelPlaying(Channel channel) const = 0;

    //======================================================================
    // Positional Audio (Fire & Forget)
    //======================================================================

    virtual SoundHandle playPositional(const PositionalSound& sound) = 0;
    virtual void stopPositional(SoundHandle handle) = 0;
    virtual void updatePositionalPosition(SoundHandle handle, Vec3 position) = 0;
    virtual bool isPositionalPlaying(SoundHandle handle) const = 0;

    //======================================================================
    // 3D Audio Listener
    //======================================================================

    virtual void setListener(const AudioListener& listener) = 0;
    virtual AudioListener getListener() const = 0;

    //======================================================================
    // Global Controls
    //======================================================================

    virtual void setMasterVolume(Volume volume) = 0;
    virtual Volume getMasterVolume() const = 0;

    virtual void pauseAll() = 0;
    virtual void resumeAll() = 0;
    virtual void stopAll() = 0;

    //======================================================================
    // Channel Groups
    //======================================================================

    virtual void setGroupVolume(const std::string& group, Volume volume) = 0;
    virtual void assignChannelToGroup(Channel channel, const std::string& group) = 0;
};

}  // namespace bestow

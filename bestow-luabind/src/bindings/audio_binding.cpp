// bestow-luabind/src/bindings/audio_binding.cpp
// Audio system Lua bindings

module;

#include <bestow/sol2_compat.hpp>
#include <spdlog/spdlog.h>

module bestow.luabind;

import std;

namespace bestow {

void bindAudioSystem(sol::state& lua, IAudioSystem& audio) {
    //=========================================================================
    // Audio-related types
    //=========================================================================

    // ChannelSound struct
    lua.new_usertype<ChannelSound>("ChannelSound",
        sol::constructors<ChannelSound()>(),
        "asset", &ChannelSound::asset,
        "volume", &ChannelSound::volume,
        "pitch", &ChannelSound::pitch,
        "looping", &ChannelSound::looping,
        "fadeInTime", &ChannelSound::fadeInTime
        // Note: startTime is std::optional, needs special handling
    );

    // PositionalSound struct
    lua.new_usertype<PositionalSound>("PositionalSound",
        sol::constructors<PositionalSound()>(),
        "asset", &PositionalSound::asset,
        "position", &PositionalSound::position,
        "volume", &PositionalSound::volume,
        "pitch", &PositionalSound::pitch,
        "minDistance", &PositionalSound::minDistance,
        "maxDistance", &PositionalSound::maxDistance
        // Note: velocity and onComplete need special handling
    );

    // AudioListener struct
    lua.new_usertype<AudioListener>("AudioListener",
        sol::constructors<AudioListener()>(),
        "position", &AudioListener::position,
        "forward", &AudioListener::forward,
        "up", &AudioListener::up,
        "velocity", &AudioListener::velocity
    );

    // ChannelState struct
    lua.new_usertype<ChannelState>("ChannelState",
        sol::constructors<ChannelState()>(),
        "isPlaying", &ChannelState::isPlaying,
        "isPaused", &ChannelState::isPaused,
        "position", &ChannelState::position,
        "length", &ChannelState::length,
        "volume", &ChannelState::volume
    );

    //=========================================================================
    // bestow.audio table
    //=========================================================================

    sol::table bestow = lua["bestow"];
    sol::table audioTable = lua.create_table();

    //-------------------------------------------------------------------------
    // Channel-Based Audio
    //-------------------------------------------------------------------------

    audioTable["playOnChannel"] = [&audio](Channel channel, const ChannelSound& sound) {
        audio.playOnChannel(channel, sound);
    };

    audioTable["stopChannel"] = sol::overload(
        [&audio](Channel channel) {
            audio.stopChannel(channel);
        },
        [&audio](Channel channel, float fadeOutTime) {
            audio.stopChannel(channel, fadeOutTime);
        }
    );

    audioTable["pauseChannel"] = [&audio](Channel channel) {
        audio.pauseChannel(channel);
    };

    audioTable["resumeChannel"] = [&audio](Channel channel) {
        audio.resumeChannel(channel);
    };

    audioTable["setChannelVolume"] = [&audio](Channel channel, Volume volume) {
        audio.setChannelVolume(channel, volume);
    };

    audioTable["setChannelPitch"] = [&audio](Channel channel, float pitch) {
        audio.setChannelPitch(channel, pitch);
    };

    audioTable["seekChannel"] = [&audio](Channel channel, float position) {
        audio.seekChannel(channel, position);
    };

    audioTable["getChannelState"] = [&audio](Channel channel) {
        return audio.getChannelState(channel);
    };

    audioTable["isChannelPlaying"] = [&audio](Channel channel) {
        return audio.isChannelPlaying(channel);
    };

    //-------------------------------------------------------------------------
    // Positional Audio
    //-------------------------------------------------------------------------

    audioTable["playPositional"] = [&audio](const PositionalSound& sound) {
        return audio.playPositional(sound);
    };

    audioTable["stopPositional"] = [&audio](SoundHandle handle) {
        audio.stopPositional(handle);
    };

    audioTable["updatePositionalPosition"] = [&audio](SoundHandle handle, const Vec3& position) {
        audio.updatePositionalPosition(handle, position);
    };

    audioTable["isPositionalPlaying"] = [&audio](SoundHandle handle) {
        return audio.isPositionalPlaying(handle);
    };

    //-------------------------------------------------------------------------
    // 3D Audio Listener
    //-------------------------------------------------------------------------

    audioTable["setListener"] = [&audio](const AudioListener& listener) {
        audio.setListener(listener);
    };

    audioTable["getListener"] = [&audio]() {
        return audio.getListener();
    };

    //-------------------------------------------------------------------------
    // Global Controls
    //-------------------------------------------------------------------------

    audioTable["setMasterVolume"] = [&audio](Volume volume) {
        audio.setMasterVolume(volume);
    };

    audioTable["getMasterVolume"] = [&audio]() {
        return audio.getMasterVolume();
    };

    audioTable["pauseAll"] = [&audio]() {
        audio.pauseAll();
    };

    audioTable["resumeAll"] = [&audio]() {
        audio.resumeAll();
    };

    audioTable["stopAll"] = [&audio]() {
        audio.stopAll();
    };

    //-------------------------------------------------------------------------
    // Channel Groups
    //-------------------------------------------------------------------------

    audioTable["setGroupVolume"] = [&audio](const std::string& group, Volume volume) {
        audio.setGroupVolume(group, volume);
    };

    audioTable["assignChannelToGroup"] = [&audio](Channel channel, const std::string& group) {
        audio.assignChannelToGroup(channel, group);
    };

    //-------------------------------------------------------------------------
    // Predefined Channels
    //-------------------------------------------------------------------------

    sol::table channels = lua.create_table();
    channels["Music"] = Channels::Music;
    channels["Ambience"] = Channels::Ambience;
    channels["UI"] = Channels::UI;
    // Note: Additional channels from Channels namespace could be added here

    audioTable["Channel"] = channels;

    bestow["audio"] = audioTable;
}

}  // namespace bestow

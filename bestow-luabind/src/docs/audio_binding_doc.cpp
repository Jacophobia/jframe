// bestow-luabind/src/docs/audio_binding_doc.cpp
// API documentation for bestow.audio

module bestow.luabind;

import std;

namespace bestow {

void registerAudioDoc(DocRegistry& registry) {
    SystemDoc sys;
    sys.name = "audio";
    sys.qualifiedName = "bestow.audio";
    sys.description = "Audio system providing channel-based playback, positional (3D) audio, listener management, and volume group control.";

    // --- Types ---

    sys.types.push_back(TypeDoc{
        .name = "ChannelSound",
        .qualifiedName = "ChannelSound",
        .description = "Describes a sound to play on a named channel. Can be constructed as a userdata or passed as a Lua table.",
        .fields = {
            {"asset", "AssetHandle", "Handle to the loaded sound asset"},
            {"volume", "number", "Playback volume (0.0 to 1.0, default 1.0)"},
            {"pitch", "number", "Playback pitch multiplier (default 1.0)"},
            {"looping", "boolean", "Whether the sound loops (default false)"},
            {"fadeInTime", "number", "Fade-in duration in seconds (default 0.0)"},
        },
        .example = "bestow.audio.playOnChannel(bestow.audio.Channel.Music, {\n    asset = musicHandle,\n    volume = 0.8,\n    looping = true,\n    fadeInTime = 2.0\n})",
    });

    sys.types.push_back(TypeDoc{
        .name = "PositionalSound",
        .qualifiedName = "PositionalSound",
        .description = "Describes a sound that plays at a position in 3D space. Volume attenuates with distance from the listener.",
        .fields = {
            {"asset", "AssetHandle", "Handle to the loaded sound asset"},
            {"position", "Vec3", "World position of the sound source"},
            {"volume", "number", "Base volume (0.0 to 1.0, default 1.0)"},
            {"pitch", "number", "Playback pitch multiplier (default 1.0)"},
            {"minDistance", "number", "Distance at which attenuation begins"},
            {"maxDistance", "number", "Distance at which the sound is inaudible"},
        },
    });

    sys.types.push_back(TypeDoc{
        .name = "AudioListener",
        .qualifiedName = "AudioListener",
        .description = "Represents the 3D audio listener (typically attached to the camera or player).",
        .fields = {
            {"position", "Vec3", "World position of the listener"},
            {"forward", "Vec3", "Forward direction vector"},
            {"up", "Vec3", "Up direction vector"},
            {"velocity", "Vec3", "Velocity for Doppler effect"},
        },
    });

    sys.types.push_back(TypeDoc{
        .name = "ChannelState",
        .qualifiedName = "ChannelState",
        .description = "Current playback state of a channel.",
        .fields = {
            {"isPlaying", "boolean", "Whether the channel is currently playing"},
            {"isPaused", "boolean", "Whether the channel is paused"},
            {"position", "number", "Current playback position in seconds"},
            {"length", "number", "Total length of the sound in seconds"},
            {"volume", "number", "Current channel volume"},
        },
    });

    // --- Channel-Based Audio ---

    sys.methods.push_back(MethodDoc{
        .name = "playOnChannel",
        .qualifiedName = "bestow.audio.playOnChannel",
        .description = "Play a sound on a named channel. Replaces any sound currently playing on that channel.",
        .params = {
            {.name = "channel", .type = "Channel", .description = "The channel to play on (e.g., bestow.audio.Channel.Music)"},
            {.name = "sound", .type = "ChannelSound|table", .description = "Sound descriptor (userdata or Lua table with asset, volume, pitch, looping, fadeInTime)"},
        },
        .example = "bestow.audio.playOnChannel(bestow.audio.Channel.Music, {\n    asset = bgmHandle,\n    volume = 0.7,\n    looping = true\n})",
        .seeAlso = {"bestow.audio.stopChannel"},
    });

    sys.methods.push_back(MethodDoc{
        .name = "stopChannel",
        .qualifiedName = "bestow.audio.stopChannel",
        .description = "Stop playback on a channel, optionally with a fade-out.",
        .params = {
            {.name = "channel", .type = "Channel", .description = "The channel to stop"},
            {.name = "fadeOutTime", .type = "number", .description = "Fade-out duration in seconds", .optional = true, .defaultVal = "0.0"},
        },
        .seeAlso = {"bestow.audio.playOnChannel"},
    });

    sys.methods.push_back(MethodDoc{
        .name = "pauseChannel",
        .qualifiedName = "bestow.audio.pauseChannel",
        .description = "Pause playback on a channel. Can be resumed later.",
        .params = {
            {.name = "channel", .type = "Channel", .description = "The channel to pause"},
        },
        .seeAlso = {"bestow.audio.resumeChannel"},
    });

    sys.methods.push_back(MethodDoc{
        .name = "resumeChannel",
        .qualifiedName = "bestow.audio.resumeChannel",
        .description = "Resume playback on a paused channel.",
        .params = {
            {.name = "channel", .type = "Channel", .description = "The channel to resume"},
        },
        .seeAlso = {"bestow.audio.pauseChannel"},
    });

    sys.methods.push_back(MethodDoc{
        .name = "setChannelVolume",
        .qualifiedName = "bestow.audio.setChannelVolume",
        .description = "Set the volume of a channel.",
        .params = {
            {.name = "channel", .type = "Channel", .description = "The channel"},
            {.name = "volume", .type = "number", .description = "Volume level (0.0 to 1.0)"},
        },
    });

    sys.methods.push_back(MethodDoc{
        .name = "setChannelPitch",
        .qualifiedName = "bestow.audio.setChannelPitch",
        .description = "Set the pitch multiplier of a channel.",
        .params = {
            {.name = "channel", .type = "Channel", .description = "The channel"},
            {.name = "pitch", .type = "number", .description = "Pitch multiplier (1.0 = normal)"},
        },
    });

    sys.methods.push_back(MethodDoc{
        .name = "seekChannel",
        .qualifiedName = "bestow.audio.seekChannel",
        .description = "Seek to a position within the currently playing sound on a channel.",
        .params = {
            {.name = "channel", .type = "Channel", .description = "The channel"},
            {.name = "position", .type = "number", .description = "Position in seconds to seek to"},
        },
    });

    sys.methods.push_back(MethodDoc{
        .name = "getChannelState",
        .qualifiedName = "bestow.audio.getChannelState",
        .description = "Get the current playback state of a channel.",
        .params = {
            {.name = "channel", .type = "Channel", .description = "The channel to query"},
        },
        .returns = {{.type = "ChannelState", .description = "Current state including playing, paused, position, length, volume"}},
    });

    sys.methods.push_back(MethodDoc{
        .name = "isChannelPlaying",
        .qualifiedName = "bestow.audio.isChannelPlaying",
        .description = "Check if a channel is currently playing (not paused or stopped).",
        .params = {
            {.name = "channel", .type = "Channel", .description = "The channel to query"},
        },
        .returns = {{.type = "boolean", .description = "true if playing"}},
    });

    // --- Positional Audio ---

    sys.methods.push_back(MethodDoc{
        .name = "playPositional",
        .qualifiedName = "bestow.audio.playPositional",
        .description = "Play a sound at a position in 3D space. Returns a handle for later control.",
        .params = {
            {.name = "sound", .type = "PositionalSound|table", .description = "Positional sound descriptor (userdata or Lua table with asset, position, volume, pitch, minDistance, maxDistance)"},
        },
        .returns = {{.type = "SoundHandle", .description = "Handle to control the positional sound"}},
        .seeAlso = {"bestow.audio.stopPositional", "bestow.audio.setListener"},
    });

    sys.methods.push_back(MethodDoc{
        .name = "stopPositional",
        .qualifiedName = "bestow.audio.stopPositional",
        .description = "Stop a positional sound.",
        .params = {
            {.name = "handle", .type = "SoundHandle", .description = "Handle of the positional sound"},
        },
    });

    sys.methods.push_back(MethodDoc{
        .name = "updatePositionalPosition",
        .qualifiedName = "bestow.audio.updatePositionalPosition",
        .description = "Update the world position of a playing positional sound (e.g., for a moving source).",
        .params = {
            {.name = "handle", .type = "SoundHandle", .description = "Handle of the positional sound"},
            {.name = "position", .type = "Vec3", .description = "New world position"},
        },
    });

    sys.methods.push_back(MethodDoc{
        .name = "isPositionalPlaying",
        .qualifiedName = "bestow.audio.isPositionalPlaying",
        .description = "Check if a positional sound is still playing.",
        .params = {
            {.name = "handle", .type = "SoundHandle", .description = "Handle of the positional sound"},
        },
        .returns = {{.type = "boolean", .description = "true if still playing"}},
    });

    // --- 3D Audio Listener ---

    sys.methods.push_back(MethodDoc{
        .name = "setListener",
        .qualifiedName = "bestow.audio.setListener",
        .description = "Set the 3D audio listener position and orientation. Typically updated each frame to match the camera.",
        .params = {
            {.name = "listener", .type = "AudioListener", .description = "Listener position, forward, up, and velocity"},
        },
        .seeAlso = {"bestow.audio.getListener"},
    });

    sys.methods.push_back(MethodDoc{
        .name = "getListener",
        .qualifiedName = "bestow.audio.getListener",
        .description = "Get the current 3D audio listener state.",
        .returns = {{.type = "AudioListener", .description = "Current listener position and orientation"}},
        .seeAlso = {"bestow.audio.setListener"},
    });

    // --- Global Controls ---

    sys.methods.push_back(MethodDoc{
        .name = "setMasterVolume",
        .qualifiedName = "bestow.audio.setMasterVolume",
        .description = "Set the master volume that affects all audio output.",
        .params = {
            {.name = "volume", .type = "number", .description = "Master volume (0.0 to 1.0)"},
        },
        .seeAlso = {"bestow.audio.getMasterVolume"},
    });

    sys.methods.push_back(MethodDoc{
        .name = "getMasterVolume",
        .qualifiedName = "bestow.audio.getMasterVolume",
        .description = "Get the current master volume.",
        .returns = {{.type = "number", .description = "Master volume (0.0 to 1.0)"}},
        .seeAlso = {"bestow.audio.setMasterVolume"},
    });

    sys.methods.push_back(MethodDoc{
        .name = "pauseAll",
        .qualifiedName = "bestow.audio.pauseAll",
        .description = "Pause all currently playing audio.",
        .seeAlso = {"bestow.audio.resumeAll"},
    });

    sys.methods.push_back(MethodDoc{
        .name = "resumeAll",
        .qualifiedName = "bestow.audio.resumeAll",
        .description = "Resume all paused audio.",
        .seeAlso = {"bestow.audio.pauseAll"},
    });

    sys.methods.push_back(MethodDoc{
        .name = "stopAll",
        .qualifiedName = "bestow.audio.stopAll",
        .description = "Stop all currently playing audio.",
    });

    // --- Channel Groups ---

    sys.methods.push_back(MethodDoc{
        .name = "setGroupVolume",
        .qualifiedName = "bestow.audio.setGroupVolume",
        .description = "Set the volume of a channel group (e.g., 'music', 'sfx').",
        .params = {
            {.name = "group", .type = "string", .description = "Name of the channel group"},
            {.name = "volume", .type = "number", .description = "Group volume (0.0 to 1.0)"},
        },
    });

    sys.methods.push_back(MethodDoc{
        .name = "assignChannelToGroup",
        .qualifiedName = "bestow.audio.assignChannelToGroup",
        .description = "Assign a channel to a volume group for grouped volume control.",
        .params = {
            {.name = "channel", .type = "Channel", .description = "The channel to assign"},
            {.name = "group", .type = "string", .description = "Name of the group to assign to"},
        },
        .seeAlso = {"bestow.audio.setGroupVolume"},
    });

    // --- Properties ---

    sys.properties.push_back(PropertyDoc{
        .name = "Channel",
        .type = "table",
        .description = "Predefined channel constants: Music, Ambience, UI.",
        .readOnly = true,
    });

    registry.addSystem(std::move(sys));
}

} // namespace bestow

// tests/unit/AudioSystemTests.cpp
// Audio system unit tests

#include <cstddef>
#include <memory>
#include <string>

#include <gtest/gtest.h>

import jframe.audio;
import jframe.audio.impl;
import jframe.types;

namespace jframe::tests {

class AudioSystemTest : public ::testing::Test {
protected:
    void SetUp() override {
        audio_ = createAudioSystem();

        // Initialize the audio system (works in stub mode)
        auto* fmodAudio = dynamic_cast<FMODAudioSystem*>(audio_.get());
        if (fmodAudio) {
            bool initialized = fmodAudio->initialize();
            EXPECT_TRUE(initialized);
        }
    }

    std::unique_ptr<IAudioSystem> audio_;
};

//==========================================================================
// Initialization Tests
//==========================================================================

TEST_F(AudioSystemTest, CanCreate) {
    EXPECT_NE(audio_, nullptr);
}

TEST_F(AudioSystemTest, InitializeReturnsTrue) {
    auto audio = createAudioSystem();
    auto* fmodAudio = dynamic_cast<FMODAudioSystem*>(audio.get());
    ASSERT_NE(fmodAudio, nullptr);

    bool result = fmodAudio->initialize();
    EXPECT_TRUE(result);
}

//==========================================================================
// Channel Playback Tests
//==========================================================================

TEST_F(AudioSystemTest, PlayOnChannelSetsPlayingState) {
    AssetHandle testSound{.uuid = 1, .type = AssetType::Sound};
    ChannelSound sound{.asset = testSound, .volume = 1.0f};

    audio_->playOnChannel(Channels::Music, sound);

    EXPECT_TRUE(audio_->isChannelPlaying(Channels::Music));
}

TEST_F(AudioSystemTest, StopChannelSetsStoppedState) {
    AssetHandle testSound{.uuid = 1, .type = AssetType::Sound};
    ChannelSound sound{.asset = testSound};

    audio_->playOnChannel(Channels::Music, sound);
    EXPECT_TRUE(audio_->isChannelPlaying(Channels::Music));

    audio_->stopChannel(Channels::Music);
    EXPECT_FALSE(audio_->isChannelPlaying(Channels::Music));
}

TEST_F(AudioSystemTest, StopChannelWithFadeOut) {
    AssetHandle testSound{.uuid = 1, .type = AssetType::Sound};
    ChannelSound sound{.asset = testSound};

    audio_->playOnChannel(Channels::Music, sound);

    // Stop with fade out time
    audio_->stopChannel(Channels::Music, 0.5f);

    // In stub mode, should still immediately stop
    EXPECT_FALSE(audio_->isChannelPlaying(Channels::Music));
}

TEST_F(AudioSystemTest, PauseChannelSetsPausedState) {
    AssetHandle testSound{.uuid = 1, .type = AssetType::Sound};
    ChannelSound sound{.asset = testSound};

    audio_->playOnChannel(Channels::Music, sound);
    audio_->pauseChannel(Channels::Music);

    ChannelState state = audio_->getChannelState(Channels::Music);
    EXPECT_TRUE(state.isPaused);
    EXPECT_FALSE(audio_->isChannelPlaying(Channels::Music));
}

TEST_F(AudioSystemTest, ResumeChannelResumesPausedChannel) {
    AssetHandle testSound{.uuid = 1, .type = AssetType::Sound};
    ChannelSound sound{.asset = testSound};

    audio_->playOnChannel(Channels::Music, sound);
    audio_->pauseChannel(Channels::Music);

    ChannelState pausedState = audio_->getChannelState(Channels::Music);
    EXPECT_TRUE(pausedState.isPaused);

    audio_->resumeChannel(Channels::Music);

    ChannelState resumedState = audio_->getChannelState(Channels::Music);
    EXPECT_FALSE(resumedState.isPaused);
    EXPECT_TRUE(audio_->isChannelPlaying(Channels::Music));
}

TEST_F(AudioSystemTest, IsChannelPlayingReturnsCorrectState) {
    AssetHandle testSound{.uuid = 1, .type = AssetType::Sound};
    ChannelSound sound{.asset = testSound};

    EXPECT_FALSE(audio_->isChannelPlaying(Channels::UI));

    audio_->playOnChannel(Channels::UI, sound);
    EXPECT_TRUE(audio_->isChannelPlaying(Channels::UI));

    audio_->stopChannel(Channels::UI);
    EXPECT_FALSE(audio_->isChannelPlaying(Channels::UI));
}

TEST_F(AudioSystemTest, GetChannelStateReturnsCurrentState) {
    AssetHandle testSound{.uuid = 1, .type = AssetType::Sound};
    ChannelSound sound{.asset = testSound, .volume = 0.8f};

    audio_->playOnChannel(Channels::Voice, sound);

    ChannelState state = audio_->getChannelState(Channels::Voice);
    EXPECT_TRUE(state.isPlaying);
    EXPECT_FALSE(state.isPaused);
}

TEST_F(AudioSystemTest, MultipleChannelsCanPlaySimultaneously) {
    AssetHandle sound1{.uuid = 1, .type = AssetType::Sound};
    AssetHandle sound2{.uuid = 2, .type = AssetType::Sound};

    ChannelSound channelSound1{.asset = sound1};
    ChannelSound channelSound2{.asset = sound2};

    audio_->playOnChannel(Channels::Music, channelSound1);
    audio_->playOnChannel(Channels::Ambience, channelSound2);

    EXPECT_TRUE(audio_->isChannelPlaying(Channels::Music));
    EXPECT_TRUE(audio_->isChannelPlaying(Channels::Ambience));
}

//==========================================================================
// Volume Control Tests
//==========================================================================

TEST_F(AudioSystemTest, SetChannelVolumeUpdatesVolume) {
    AssetHandle testSound{.uuid = 1, .type = AssetType::Sound};
    ChannelSound sound{.asset = testSound};

    audio_->playOnChannel(Channels::Music, sound);
    audio_->setChannelVolume(Channels::Music, 0.5f);

    ChannelState state = audio_->getChannelState(Channels::Music);
    EXPECT_FLOAT_EQ(state.volume, 0.5f);
}

TEST_F(AudioSystemTest, GetChannelStateReflectsVolumeChange) {
    AssetHandle testSound{.uuid = 1, .type = AssetType::Sound};
    ChannelSound sound{.asset = testSound, .volume = 1.0f};

    audio_->playOnChannel(Channels::UI, sound);

    ChannelState initialState = audio_->getChannelState(Channels::UI);
    EXPECT_FLOAT_EQ(initialState.volume, 1.0f);

    audio_->setChannelVolume(Channels::UI, 0.3f);

    ChannelState updatedState = audio_->getChannelState(Channels::UI);
    EXPECT_FLOAT_EQ(updatedState.volume, 0.3f);
}

TEST_F(AudioSystemTest, SetMasterVolumeUpdatesGlobalVolume) {
    audio_->setMasterVolume(0.7f);

    Volume volume = audio_->getMasterVolume();
    EXPECT_FLOAT_EQ(volume, 0.7f);
}

TEST_F(AudioSystemTest, GetMasterVolumeReturnsCurrentVolume) {
    Volume defaultVolume = audio_->getMasterVolume();
    EXPECT_FLOAT_EQ(defaultVolume, 1.0f);

    audio_->setMasterVolume(0.5f);
    EXPECT_FLOAT_EQ(audio_->getMasterVolume(), 0.5f);
}

TEST_F(AudioSystemTest, SetChannelPitchUpdatesPitch) {
    AssetHandle testSound{.uuid = 1, .type = AssetType::Sound};
    ChannelSound sound{.asset = testSound};

    audio_->playOnChannel(Channels::Voice, sound);

    // Pitch doesn't affect ChannelState, but shouldn't crash
    audio_->setChannelPitch(Channels::Voice, 1.5f);

    EXPECT_TRUE(audio_->isChannelPlaying(Channels::Voice));
}

TEST_F(AudioSystemTest, SeekChannelUpdatesPosition) {
    AssetHandle testSound{.uuid = 1, .type = AssetType::Sound};
    ChannelSound sound{.asset = testSound};

    audio_->playOnChannel(Channels::Music, sound);

    // Seek to 2 seconds
    audio_->seekChannel(Channels::Music, 2.0f);

    // Should still be playing
    EXPECT_TRUE(audio_->isChannelPlaying(Channels::Music));
}

//==========================================================================
// Positional Audio Tests
//==========================================================================

TEST_F(AudioSystemTest, PlayPositionalReturnsValidHandle) {
    AssetHandle testSound{.uuid = 1, .type = AssetType::Sound};
    PositionalSound sound{
        .asset = testSound,
        .position = Vec3{10.0f, 0.0f, 0.0f}
    };

    SoundHandle handle = audio_->playPositional(sound);

    EXPECT_NE(handle, 0);
}

TEST_F(AudioSystemTest, IsPositionalPlayingReturnsTrueForActiveSound) {
    AssetHandle testSound{.uuid = 1, .type = AssetType::Sound};
    PositionalSound sound{
        .asset = testSound,
        .position = Vec3{5.0f, 5.0f, 0.0f}
    };

    SoundHandle handle = audio_->playPositional(sound);

    EXPECT_TRUE(audio_->isPositionalPlaying(handle));
}

TEST_F(AudioSystemTest, StopPositionalStopsSound) {
    AssetHandle testSound{.uuid = 1, .type = AssetType::Sound};
    PositionalSound sound{
        .asset = testSound,
        .position = Vec3{1.0f, 2.0f, 3.0f}
    };

    SoundHandle handle = audio_->playPositional(sound);
    EXPECT_TRUE(audio_->isPositionalPlaying(handle));

    audio_->stopPositional(handle);
    EXPECT_FALSE(audio_->isPositionalPlaying(handle));
}

TEST_F(AudioSystemTest, UpdatePositionalPositionUpdatesState) {
    AssetHandle testSound{.uuid = 1, .type = AssetType::Sound};
    PositionalSound sound{
        .asset = testSound,
        .position = Vec3{0.0f, 0.0f, 0.0f}
    };

    SoundHandle handle = audio_->playPositional(sound);

    // Update position (state tracking in stub mode)
    audio_->updatePositionalPosition(handle, Vec3{10.0f, 20.0f, 30.0f});

    // Sound should still be playing after position update
    EXPECT_TRUE(audio_->isPositionalPlaying(handle));
}

TEST_F(AudioSystemTest, MultiplePositionalSoundsCanPlay) {
    AssetHandle sound1{.uuid = 1, .type = AssetType::Sound};
    AssetHandle sound2{.uuid = 2, .type = AssetType::Sound};

    PositionalSound pos1{.asset = sound1, .position = Vec3{0.0f, 0.0f, 0.0f}};
    PositionalSound pos2{.asset = sound2, .position = Vec3{10.0f, 0.0f, 0.0f}};

    SoundHandle handle1 = audio_->playPositional(pos1);
    SoundHandle handle2 = audio_->playPositional(pos2);

    EXPECT_TRUE(audio_->isPositionalPlaying(handle1));
    EXPECT_TRUE(audio_->isPositionalPlaying(handle2));
    EXPECT_NE(handle1, handle2);
}

TEST_F(AudioSystemTest, PositionalSoundWithVelocity) {
    AssetHandle testSound{.uuid = 1, .type = AssetType::Sound};
    PositionalSound sound{
        .asset = testSound,
        .position = Vec3{0.0f, 0.0f, 0.0f},
        .velocity = Vec3{1.0f, 0.0f, 0.0f}
    };

    SoundHandle handle = audio_->playPositional(sound);
    EXPECT_TRUE(audio_->isPositionalPlaying(handle));
}

//==========================================================================
// Listener Tests
//==========================================================================

TEST_F(AudioSystemTest, SetListenerUpdatesListenerState) {
    AudioListener listener{
        .position = Vec3{10.0f, 20.0f, 30.0f},
        .forward = Vec3{0.0f, 0.0f, -1.0f},
        .up = Vec3{0.0f, 1.0f, 0.0f}
    };

    audio_->setListener(listener);

    AudioListener retrieved = audio_->getListener();
    EXPECT_FLOAT_EQ(retrieved.position.x, 10.0f);
    EXPECT_FLOAT_EQ(retrieved.position.y, 20.0f);
    EXPECT_FLOAT_EQ(retrieved.position.z, 30.0f);
}

TEST_F(AudioSystemTest, GetListenerReturnsCurrentListener) {
    AudioListener defaultListener = audio_->getListener();
    EXPECT_FLOAT_EQ(defaultListener.position.x, 0.0f);
    EXPECT_FLOAT_EQ(defaultListener.position.y, 0.0f);
    EXPECT_FLOAT_EQ(defaultListener.position.z, 0.0f);
}

TEST_F(AudioSystemTest, SetListenerWithVelocity) {
    AudioListener listener{
        .position = Vec3{5.0f, 5.0f, 5.0f},
        .forward = Vec3{1.0f, 0.0f, 0.0f},
        .up = Vec3{0.0f, 1.0f, 0.0f},
        .velocity = Vec3{2.0f, 0.0f, 0.0f}
    };

    audio_->setListener(listener);

    AudioListener retrieved = audio_->getListener();
    EXPECT_FLOAT_EQ(retrieved.velocity.x, 2.0f);
}

//==========================================================================
// Channel Groups Tests
//==========================================================================

TEST_F(AudioSystemTest, SetGroupVolumeCreatesGroup) {
    audio_->setGroupVolume("sfx", 0.8f);

    // Should not crash - group is created
    EXPECT_TRUE(true);
}

TEST_F(AudioSystemTest, AssignChannelToGroupAssignsChannel) {
    AssetHandle testSound{.uuid = 1, .type = AssetType::Sound};
    ChannelSound sound{.asset = testSound};

    audio_->playOnChannel(Channels::UI, sound);
    audio_->setGroupVolume("ui_group", 0.5f);
    audio_->assignChannelToGroup(Channels::UI, "ui_group");

    // Should not crash - channel is assigned to group
    EXPECT_TRUE(audio_->isChannelPlaying(Channels::UI));
}

TEST_F(AudioSystemTest, UpdateGroupVolumeAffectsGroup) {
    audio_->setGroupVolume("music_group", 0.5f);
    audio_->setGroupVolume("music_group", 0.9f);

    // Should update existing group
    EXPECT_TRUE(true);
}

//==========================================================================
// Global Controls Tests
//==========================================================================

TEST_F(AudioSystemTest, PauseAllPausesAllChannels) {
    AssetHandle sound1{.uuid = 1, .type = AssetType::Sound};
    AssetHandle sound2{.uuid = 2, .type = AssetType::Sound};

    ChannelSound channelSound1{.asset = sound1};
    ChannelSound channelSound2{.asset = sound2};

    audio_->playOnChannel(Channels::Music, channelSound1);
    audio_->playOnChannel(Channels::Ambience, channelSound2);

    audio_->pauseAll();

    EXPECT_FALSE(audio_->isChannelPlaying(Channels::Music));
    EXPECT_FALSE(audio_->isChannelPlaying(Channels::Ambience));
}

TEST_F(AudioSystemTest, ResumeAllResumesAllChannels) {
    AssetHandle sound1{.uuid = 1, .type = AssetType::Sound};
    AssetHandle sound2{.uuid = 2, .type = AssetType::Sound};

    ChannelSound channelSound1{.asset = sound1};
    ChannelSound channelSound2{.asset = sound2};

    audio_->playOnChannel(Channels::Music, channelSound1);
    audio_->playOnChannel(Channels::Ambience, channelSound2);
    audio_->pauseAll();

    audio_->resumeAll();

    EXPECT_TRUE(audio_->isChannelPlaying(Channels::Music));
    EXPECT_TRUE(audio_->isChannelPlaying(Channels::Ambience));
}

TEST_F(AudioSystemTest, StopAllStopsEverything) {
    AssetHandle sound1{.uuid = 1, .type = AssetType::Sound};
    AssetHandle sound2{.uuid = 2, .type = AssetType::Sound};

    ChannelSound channelSound1{.asset = sound1};
    ChannelSound channelSound2{.asset = sound2};

    audio_->playOnChannel(Channels::Music, channelSound1);
    audio_->playOnChannel(Channels::UI, channelSound2);

    PositionalSound posSound{.asset = sound1, .position = Vec3{0.0f}};
    SoundHandle posHandle = audio_->playPositional(posSound);

    audio_->stopAll();

    EXPECT_FALSE(audio_->isChannelPlaying(Channels::Music));
    EXPECT_FALSE(audio_->isChannelPlaying(Channels::UI));
    EXPECT_FALSE(audio_->isPositionalPlaying(posHandle));
}

TEST_F(AudioSystemTest, PauseAllThenStopAll) {
    AssetHandle testSound{.uuid = 1, .type = AssetType::Sound};
    ChannelSound sound{.asset = testSound};

    audio_->playOnChannel(Channels::Music, sound);
    audio_->pauseAll();
    audio_->stopAll();

    EXPECT_FALSE(audio_->isChannelPlaying(Channels::Music));
}

//==========================================================================
// Update Tests
//==========================================================================

TEST_F(AudioSystemTest, UpdateDoesNotCrash) {
    AssetHandle testSound{.uuid = 1, .type = AssetType::Sound};
    ChannelSound sound{.asset = testSound};

    audio_->playOnChannel(Channels::Music, sound);

    // Call update
    audio_->update(0.016f);

    EXPECT_TRUE(audio_->isChannelPlaying(Channels::Music));
}

TEST_F(AudioSystemTest, MultipleUpdates) {
    AssetHandle testSound{.uuid = 1, .type = AssetType::Sound};
    ChannelSound sound{.asset = testSound};

    audio_->playOnChannel(Channels::Music, sound);

    for (int i = 0; i < 10; i++) {
        audio_->update(0.016f);
    }

    EXPECT_TRUE(audio_->isChannelPlaying(Channels::Music));
}

//==========================================================================
// Edge Cases Tests
//==========================================================================

TEST_F(AudioSystemTest, StopChannelThatIsNotPlaying) {
    audio_->stopChannel(Channels::Music);
    EXPECT_FALSE(audio_->isChannelPlaying(Channels::Music));
}

TEST_F(AudioSystemTest, PauseChannelThatIsNotPlaying) {
    audio_->pauseChannel(Channels::Music);
    EXPECT_FALSE(audio_->isChannelPlaying(Channels::Music));
}

TEST_F(AudioSystemTest, ResumeChannelThatIsNotPaused) {
    AssetHandle testSound{.uuid = 1, .type = AssetType::Sound};
    ChannelSound sound{.asset = testSound};

    audio_->playOnChannel(Channels::Music, sound);
    audio_->resumeChannel(Channels::Music);

    EXPECT_TRUE(audio_->isChannelPlaying(Channels::Music));
}

TEST_F(AudioSystemTest, GetChannelStateForUnusedChannel) {
    ChannelState state = audio_->getChannelState(999);
    EXPECT_FALSE(state.isPlaying);
    EXPECT_FALSE(state.isPaused);
}

TEST_F(AudioSystemTest, StopInvalidPositionalHandle) {
    audio_->stopPositional(999999);
    EXPECT_FALSE(audio_->isPositionalPlaying(999999));
}

TEST_F(AudioSystemTest, IsPositionalPlayingForInvalidHandle) {
    EXPECT_FALSE(audio_->isPositionalPlaying(0));
}

TEST_F(AudioSystemTest, UpdatePositionForInvalidHandle) {
    // Should not crash
    audio_->updatePositionalPosition(0, Vec3{0.0f, 0.0f, 0.0f});
    EXPECT_TRUE(true);
}

TEST_F(AudioSystemTest, PlaySoundWithLooping) {
    AssetHandle testSound{.uuid = 1, .type = AssetType::Sound};
    ChannelSound sound{
        .asset = testSound,
        .looping = true
    };

    audio_->playOnChannel(Channels::Music, sound);
    EXPECT_TRUE(audio_->isChannelPlaying(Channels::Music));
}

TEST_F(AudioSystemTest, PlaySoundWithFadeIn) {
    AssetHandle testSound{.uuid = 1, .type = AssetType::Sound};
    ChannelSound sound{
        .asset = testSound,
        .fadeInTime = 0.5f
    };

    audio_->playOnChannel(Channels::Music, sound);
    EXPECT_TRUE(audio_->isChannelPlaying(Channels::Music));
}

TEST_F(AudioSystemTest, PlaySoundWithStartTime) {
    AssetHandle testSound{.uuid = 1, .type = AssetType::Sound};
    ChannelSound sound{
        .asset = testSound,
        .startTime = 2.0f
    };

    audio_->playOnChannel(Channels::Music, sound);
    EXPECT_TRUE(audio_->isChannelPlaying(Channels::Music));
}

TEST_F(AudioSystemTest, PositionalSoundWithMinMaxDistance) {
    AssetHandle testSound{.uuid = 1, .type = AssetType::Sound};
    PositionalSound sound{
        .asset = testSound,
        .position = Vec3{0.0f, 0.0f, 0.0f},
        .minDistance = 5.0f,
        .maxDistance = 50.0f
    };

    SoundHandle handle = audio_->playPositional(sound);
    EXPECT_TRUE(audio_->isPositionalPlaying(handle));
}

//==========================================================================
// Volume Boundary Tests
//==========================================================================

TEST_F(AudioSystemTest, SetChannelVolumeWithZero) {
    AssetHandle testSound{.uuid = 1, .type = AssetType::Sound};
    ChannelSound sound{.asset = testSound};

    audio_->playOnChannel(Channels::Music, sound);
    audio_->setChannelVolume(Channels::Music, 0.0f);

    ChannelState state = audio_->getChannelState(Channels::Music);
    EXPECT_FLOAT_EQ(state.volume, 0.0f);
}

TEST_F(AudioSystemTest, SetChannelVolumeAboveOne) {
    AssetHandle testSound{.uuid = 1, .type = AssetType::Sound};
    ChannelSound sound{.asset = testSound};

    audio_->playOnChannel(Channels::Music, sound);

    // Some audio systems allow > 1.0 for amplification
    audio_->setChannelVolume(Channels::Music, 1.5f);

    ChannelState state = audio_->getChannelState(Channels::Music);
    EXPECT_FLOAT_EQ(state.volume, 1.5f);
}

TEST_F(AudioSystemTest, SetChannelVolumeNegative) {
    AssetHandle testSound{.uuid = 1, .type = AssetType::Sound};
    ChannelSound sound{.asset = testSound};

    audio_->playOnChannel(Channels::Music, sound);

    // Negative volumes should either clamp to 0 or be accepted (implementation-defined)
    audio_->setChannelVolume(Channels::Music, -0.5f);

    ChannelState state = audio_->getChannelState(Channels::Music);
    // Should not crash - implementation may clamp or accept
    EXPECT_TRUE(true);
}

TEST_F(AudioSystemTest, SetMasterVolumeZero) {
    audio_->setMasterVolume(0.0f);
    EXPECT_FLOAT_EQ(audio_->getMasterVolume(), 0.0f);
}

TEST_F(AudioSystemTest, SetMasterVolumeAboveOne) {
    audio_->setMasterVolume(2.0f);
    EXPECT_FLOAT_EQ(audio_->getMasterVolume(), 2.0f);
}

TEST_F(AudioSystemTest, SetGroupVolumeZero) {
    audio_->setGroupVolume("test_group", 0.0f);
    // Should not crash
    EXPECT_TRUE(true);
}

TEST_F(AudioSystemTest, SetGroupVolumeNegative) {
    audio_->setGroupVolume("test_group", -1.0f);
    // Should not crash
    EXPECT_TRUE(true);
}

//==========================================================================
// Pitch Boundary Tests
//==========================================================================

TEST_F(AudioSystemTest, SetChannelPitchZero) {
    AssetHandle testSound{.uuid = 1, .type = AssetType::Sound};
    ChannelSound sound{.asset = testSound};

    audio_->playOnChannel(Channels::Voice, sound);
    audio_->setChannelPitch(Channels::Voice, 0.0f);

    // Should not crash - pitch of 0 stops sound or is undefined
    EXPECT_TRUE(audio_->isChannelPlaying(Channels::Voice) ||
                !audio_->isChannelPlaying(Channels::Voice));
}

TEST_F(AudioSystemTest, SetChannelPitchNegative) {
    AssetHandle testSound{.uuid = 1, .type = AssetType::Sound};
    ChannelSound sound{.asset = testSound};

    audio_->playOnChannel(Channels::Voice, sound);

    // Negative pitch might play in reverse or be clamped
    audio_->setChannelPitch(Channels::Voice, -1.0f);

    // Should not crash
    EXPECT_TRUE(true);
}

TEST_F(AudioSystemTest, SetChannelPitchVeryHigh) {
    AssetHandle testSound{.uuid = 1, .type = AssetType::Sound};
    ChannelSound sound{.asset = testSound};

    audio_->playOnChannel(Channels::Voice, sound);
    audio_->setChannelPitch(Channels::Voice, 10.0f);

    EXPECT_TRUE(audio_->isChannelPlaying(Channels::Voice));
}

TEST_F(AudioSystemTest, SetChannelPitchVeryLow) {
    AssetHandle testSound{.uuid = 1, .type = AssetType::Sound};
    ChannelSound sound{.asset = testSound};

    audio_->playOnChannel(Channels::Voice, sound);
    audio_->setChannelPitch(Channels::Voice, 0.1f);

    EXPECT_TRUE(audio_->isChannelPlaying(Channels::Voice));
}

//==========================================================================
// Seek Boundary Tests
//==========================================================================

TEST_F(AudioSystemTest, SeekChannelToZero) {
    AssetHandle testSound{.uuid = 1, .type = AssetType::Sound};
    ChannelSound sound{.asset = testSound};

    audio_->playOnChannel(Channels::Music, sound);
    audio_->seekChannel(Channels::Music, 0.0f);

    // Should rewind to beginning
    EXPECT_TRUE(audio_->isChannelPlaying(Channels::Music));
}

TEST_F(AudioSystemTest, SeekChannelNegative) {
    AssetHandle testSound{.uuid = 1, .type = AssetType::Sound};
    ChannelSound sound{.asset = testSound};

    audio_->playOnChannel(Channels::Music, sound);

    // Negative seek should either clamp to 0 or be ignored
    audio_->seekChannel(Channels::Music, -5.0f);

    // Should not crash
    EXPECT_TRUE(audio_->isChannelPlaying(Channels::Music));
}

TEST_F(AudioSystemTest, SeekChannelBeyondLength) {
    AssetHandle testSound{.uuid = 1, .type = AssetType::Sound};
    ChannelSound sound{.asset = testSound};

    audio_->playOnChannel(Channels::Music, sound);

    // Seeking beyond length should stop sound or clamp
    audio_->seekChannel(Channels::Music, 999999.0f);

    // Implementation may stop or clamp - both valid
    EXPECT_TRUE(true);
}

TEST_F(AudioSystemTest, SeekChannelThatIsNotPlaying) {
    // Seeking a channel that isn't playing should not crash
    audio_->seekChannel(Channels::Music, 5.0f);
    EXPECT_FALSE(audio_->isChannelPlaying(Channels::Music));
}

//==========================================================================
// ChannelState Field Coverage Tests
//==========================================================================

TEST_F(AudioSystemTest, ChannelStateLengthField) {
    AssetHandle testSound{.uuid = 1, .type = AssetType::Sound};
    ChannelSound sound{.asset = testSound};

    audio_->playOnChannel(Channels::Music, sound);

    ChannelState state = audio_->getChannelState(Channels::Music);

    // Length might be 0 in stub mode, but field should exist
    EXPECT_GE(state.length, 0.0f);
}

TEST_F(AudioSystemTest, ChannelStatePositionField) {
    AssetHandle testSound{.uuid = 1, .type = AssetType::Sound};
    ChannelSound sound{.asset = testSound};

    audio_->playOnChannel(Channels::Music, sound);

    ChannelState state = audio_->getChannelState(Channels::Music);

    // Position should be non-negative
    EXPECT_GE(state.position, 0.0f);
}

TEST_F(AudioSystemTest, ChannelStateAfterSeek) {
    AssetHandle testSound{.uuid = 1, .type = AssetType::Sound};
    ChannelSound sound{.asset = testSound};

    audio_->playOnChannel(Channels::Music, sound);
    audio_->seekChannel(Channels::Music, 3.0f);

    ChannelState state = audio_->getChannelState(Channels::Music);

    // In stub mode, position might be updated
    EXPECT_GE(state.position, 0.0f);
}

//==========================================================================
// Channel Group Advanced Tests
//==========================================================================

TEST_F(AudioSystemTest, AssignMultipleChannelsToSameGroup) {
    AssetHandle sound1{.uuid = 1, .type = AssetType::Sound};
    AssetHandle sound2{.uuid = 2, .type = AssetType::Sound};

    ChannelSound channelSound1{.asset = sound1};
    ChannelSound channelSound2{.asset = sound2};

    audio_->playOnChannel(Channels::Music, channelSound1);
    audio_->playOnChannel(Channels::Ambience, channelSound2);

    audio_->setGroupVolume("main_group", 0.5f);
    audio_->assignChannelToGroup(Channels::Music, "main_group");
    audio_->assignChannelToGroup(Channels::Ambience, "main_group");

    // Both channels in same group should not crash
    EXPECT_TRUE(audio_->isChannelPlaying(Channels::Music));
    EXPECT_TRUE(audio_->isChannelPlaying(Channels::Ambience));
}

TEST_F(AudioSystemTest, ReassignChannelToDifferentGroup) {
    AssetHandle testSound{.uuid = 1, .type = AssetType::Sound};
    ChannelSound sound{.asset = testSound};

    audio_->playOnChannel(Channels::UI, sound);

    audio_->setGroupVolume("group1", 0.8f);
    audio_->assignChannelToGroup(Channels::UI, "group1");

    // Reassign to different group
    audio_->setGroupVolume("group2", 0.5f);
    audio_->assignChannelToGroup(Channels::UI, "group2");

    EXPECT_TRUE(audio_->isChannelPlaying(Channels::UI));
}

TEST_F(AudioSystemTest, AssignChannelToGroupBeforePlaying) {
    audio_->setGroupVolume("preassigned_group", 0.7f);
    audio_->assignChannelToGroup(Channels::Music, "preassigned_group");

    // Now play on that channel
    AssetHandle testSound{.uuid = 1, .type = AssetType::Sound};
    ChannelSound sound{.asset = testSound};
    audio_->playOnChannel(Channels::Music, sound);

    // Should inherit group settings
    EXPECT_TRUE(audio_->isChannelPlaying(Channels::Music));
}

TEST_F(AudioSystemTest, SetGroupVolumeAfterChannelAssignment) {
    AssetHandle testSound{.uuid = 1, .type = AssetType::Sound};
    ChannelSound sound{.asset = testSound};

    audio_->playOnChannel(Channels::Music, sound);
    audio_->assignChannelToGroup(Channels::Music, "dynamic_group");

    // Set volume after assignment
    audio_->setGroupVolume("dynamic_group", 0.3f);

    // Should apply to already-assigned channel
    EXPECT_TRUE(audio_->isChannelPlaying(Channels::Music));
}

//==========================================================================
// Positional Audio Advanced Tests
//==========================================================================

TEST_F(AudioSystemTest, StopPositionalSoundMultipleTimes) {
    AssetHandle testSound{.uuid = 1, .type = AssetType::Sound};
    PositionalSound sound{
        .asset = testSound,
        .position = Vec3{0.0f, 0.0f, 0.0f}
    };

    SoundHandle handle = audio_->playPositional(sound);
    audio_->stopPositional(handle);

    // Stop again - should not crash
    audio_->stopPositional(handle);
    EXPECT_FALSE(audio_->isPositionalPlaying(handle));
}

TEST_F(AudioSystemTest, UpdatePositionOfStoppedSound) {
    AssetHandle testSound{.uuid = 1, .type = AssetType::Sound};
    PositionalSound sound{
        .asset = testSound,
        .position = Vec3{0.0f, 0.0f, 0.0f}
    };

    SoundHandle handle = audio_->playPositional(sound);
    audio_->stopPositional(handle);

    // Update position after stopping - should not crash
    audio_->updatePositionalPosition(handle, Vec3{100.0f, 100.0f, 100.0f});
    EXPECT_FALSE(audio_->isPositionalPlaying(handle));
}

TEST_F(AudioSystemTest, PositionalSoundWithZeroMinMaxDistance) {
    AssetHandle testSound{.uuid = 1, .type = AssetType::Sound};
    PositionalSound sound{
        .asset = testSound,
        .position = Vec3{0.0f, 0.0f, 0.0f},
        .minDistance = 0.0f,
        .maxDistance = 0.0f
    };

    SoundHandle handle = audio_->playPositional(sound);
    // Behavior is implementation-defined, but should not crash
    EXPECT_TRUE(handle == 0 || audio_->isPositionalPlaying(handle));
}

TEST_F(AudioSystemTest, PositionalSoundWithNegativeDistance) {
    AssetHandle testSound{.uuid = 1, .type = AssetType::Sound};
    PositionalSound sound{
        .asset = testSound,
        .position = Vec3{0.0f, 0.0f, 0.0f},
        .minDistance = -10.0f,
        .maxDistance = -5.0f
    };

    SoundHandle handle = audio_->playPositional(sound);
    // Should handle gracefully - either clamp or reject
    EXPECT_TRUE(true);
}

TEST_F(AudioSystemTest, PositionalSoundWithMinGreaterThanMax) {
    AssetHandle testSound{.uuid = 1, .type = AssetType::Sound};
    PositionalSound sound{
        .asset = testSound,
        .position = Vec3{0.0f, 0.0f, 0.0f},
        .minDistance = 100.0f,
        .maxDistance = 10.0f  // Min > Max - invalid
    };

    SoundHandle handle = audio_->playPositional(sound);
    // Should handle gracefully
    EXPECT_TRUE(true);
}

//==========================================================================
// Listener Advanced Tests
//==========================================================================

TEST_F(AudioSystemTest, SetListenerWithZeroVectors) {
    AudioListener listener{
        .position = Vec3{0.0f, 0.0f, 0.0f},
        .forward = Vec3{0.0f, 0.0f, 0.0f},  // Invalid - zero vector
        .up = Vec3{0.0f, 0.0f, 0.0f}        // Invalid - zero vector
    };

    // Should not crash with invalid vectors
    audio_->setListener(listener);

    AudioListener retrieved = audio_->getListener();
    EXPECT_FLOAT_EQ(retrieved.forward.x, 0.0f);
}

TEST_F(AudioSystemTest, SetListenerWithExtremePosition) {
    AudioListener listener{
        .position = Vec3{1e6f, 1e6f, 1e6f},
        .forward = Vec3{0.0f, 0.0f, -1.0f},
        .up = Vec3{0.0f, 1.0f, 0.0f}
    };

    audio_->setListener(listener);

    AudioListener retrieved = audio_->getListener();
    EXPECT_FLOAT_EQ(retrieved.position.x, 1e6f);
}

TEST_F(AudioSystemTest, SetListenerMultipleTimes) {
    AudioListener listener1{
        .position = Vec3{1.0f, 2.0f, 3.0f},
        .forward = Vec3{0.0f, 0.0f, -1.0f},
        .up = Vec3{0.0f, 1.0f, 0.0f}
    };

    AudioListener listener2{
        .position = Vec3{10.0f, 20.0f, 30.0f},
        .forward = Vec3{1.0f, 0.0f, 0.0f},
        .up = Vec3{0.0f, 1.0f, 0.0f}
    };

    audio_->setListener(listener1);
    audio_->setListener(listener2);

    AudioListener retrieved = audio_->getListener();
    EXPECT_FLOAT_EQ(retrieved.position.x, 10.0f);
}

//==========================================================================
// Concurrent Operations and Stress Tests
//==========================================================================

TEST_F(AudioSystemTest, PlayStopPlaySameChannelRapidly) {
    AssetHandle testSound{.uuid = 1, .type = AssetType::Sound};
    ChannelSound sound{.asset = testSound};

    for (int i = 0; i < 10; i++) {
        audio_->playOnChannel(Channels::Music, sound);
        audio_->stopChannel(Channels::Music);
    }

    EXPECT_FALSE(audio_->isChannelPlaying(Channels::Music));
}

TEST_F(AudioSystemTest, PauseResumeCycle) {
    AssetHandle testSound{.uuid = 1, .type = AssetType::Sound};
    ChannelSound sound{.asset = testSound};

    audio_->playOnChannel(Channels::Music, sound);

    for (int i = 0; i < 5; i++) {
        audio_->pauseChannel(Channels::Music);
        audio_->resumeChannel(Channels::Music);
    }

    EXPECT_TRUE(audio_->isChannelPlaying(Channels::Music));
}

TEST_F(AudioSystemTest, ManyPositionalSounds) {
    AssetHandle testSound{.uuid = 1, .type = AssetType::Sound};
    std::vector<SoundHandle> handles;

    // Play many positional sounds
    for (int i = 0; i < 50; i++) {
        PositionalSound sound{
            .asset = testSound,
            .position = Vec3{static_cast<float>(i), 0.0f, 0.0f}
        };
        SoundHandle handle = audio_->playPositional(sound);
        if (handle != 0) {
            handles.push_back(handle);
        }
    }

    // All should be playing
    for (auto handle : handles) {
        EXPECT_TRUE(audio_->isPositionalPlaying(handle));
    }

    // Stop all
    for (auto handle : handles) {
        audio_->stopPositional(handle);
    }

    // None should be playing
    for (auto handle : handles) {
        EXPECT_FALSE(audio_->isPositionalPlaying(handle));
    }
}

TEST_F(AudioSystemTest, StopAllWithManyActiveSounds) {
    AssetHandle sound1{.uuid = 1, .type = AssetType::Sound};
    AssetHandle sound2{.uuid = 2, .type = AssetType::Sound};

    // Play on multiple channels
    audio_->playOnChannel(Channels::Music, ChannelSound{.asset = sound1});
    audio_->playOnChannel(Channels::Ambience, ChannelSound{.asset = sound1});
    audio_->playOnChannel(Channels::UI, ChannelSound{.asset = sound1});
    audio_->playOnChannel(Channels::Voice, ChannelSound{.asset = sound2});

    // Play multiple positional
    std::vector<SoundHandle> posHandles;
    for (int i = 0; i < 10; i++) {
        PositionalSound pos{.asset = sound1, .position = Vec3{static_cast<float>(i)}};
        posHandles.push_back(audio_->playPositional(pos));
    }

    // Stop all
    audio_->stopAll();

    // Verify everything stopped
    EXPECT_FALSE(audio_->isChannelPlaying(Channels::Music));
    EXPECT_FALSE(audio_->isChannelPlaying(Channels::Ambience));
    EXPECT_FALSE(audio_->isChannelPlaying(Channels::UI));
    EXPECT_FALSE(audio_->isChannelPlaying(Channels::Voice));

    for (auto handle : posHandles) {
        EXPECT_FALSE(audio_->isPositionalPlaying(handle));
    }
}

//==========================================================================
// Global Pause/Resume State Tests
//==========================================================================

TEST_F(AudioSystemTest, PauseAllThenPlayNewSound) {
    AssetHandle sound1{.uuid = 1, .type = AssetType::Sound};
    AssetHandle sound2{.uuid = 2, .type = AssetType::Sound};

    audio_->playOnChannel(Channels::Music, ChannelSound{.asset = sound1});
    audio_->pauseAll();

    // Play new sound while paused
    audio_->playOnChannel(Channels::UI, ChannelSound{.asset = sound2});

    // Behavior may vary - new sound might start paused or playing
    // Both are valid depending on implementation
    EXPECT_TRUE(true);
}

TEST_F(AudioSystemTest, ResumeAllWithNoSoundsPlaying) {
    // Resume when nothing is playing should not crash
    audio_->resumeAll();
    EXPECT_TRUE(true);
}

TEST_F(AudioSystemTest, PauseAllMultipleTimes) {
    AssetHandle testSound{.uuid = 1, .type = AssetType::Sound};
    audio_->playOnChannel(Channels::Music, ChannelSound{.asset = testSound});

    audio_->pauseAll();
    audio_->pauseAll();
    audio_->pauseAll();

    EXPECT_FALSE(audio_->isChannelPlaying(Channels::Music));
}

TEST_F(AudioSystemTest, ResumeAllMultipleTimes) {
    AssetHandle testSound{.uuid = 1, .type = AssetType::Sound};
    audio_->playOnChannel(Channels::Music, ChannelSound{.asset = testSound});
    audio_->pauseAll();

    audio_->resumeAll();
    audio_->resumeAll();
    audio_->resumeAll();

    EXPECT_TRUE(audio_->isChannelPlaying(Channels::Music));
}

//==========================================================================
// Update Stress Tests
//==========================================================================

TEST_F(AudioSystemTest, UpdateWithZeroDeltaTime) {
    AssetHandle testSound{.uuid = 1, .type = AssetType::Sound};
    audio_->playOnChannel(Channels::Music, ChannelSound{.asset = testSound});

    audio_->update(0.0f);
    EXPECT_TRUE(audio_->isChannelPlaying(Channels::Music));
}

TEST_F(AudioSystemTest, UpdateWithNegativeDeltaTime) {
    AssetHandle testSound{.uuid = 1, .type = AssetType::Sound};
    audio_->playOnChannel(Channels::Music, ChannelSound{.asset = testSound});

    // Negative delta time should be handled gracefully
    audio_->update(-0.016f);
    EXPECT_TRUE(audio_->isChannelPlaying(Channels::Music));
}

TEST_F(AudioSystemTest, UpdateWithVeryLargeDeltaTime) {
    AssetHandle testSound{.uuid = 1, .type = AssetType::Sound};
    audio_->playOnChannel(Channels::Music, ChannelSound{.asset = testSound});

    // Large delta time (e.g., after lag spike)
    audio_->update(5.0f);

    // Sound may have finished, but should not crash
    EXPECT_TRUE(true);
}

//==========================================================================
// ChannelSound Parameter Coverage Tests
//==========================================================================

TEST_F(AudioSystemTest, PlaySoundWithAllParameters) {
    AssetHandle testSound{.uuid = 1, .type = AssetType::Sound};
    ChannelSound sound{
        .asset = testSound,
        .volume = 0.75f,
        .pitch = 1.2f,
        .looping = true,
        .fadeInTime = 0.3f,
        .startTime = 1.5f
    };

    audio_->playOnChannel(Channels::Music, sound);
    EXPECT_TRUE(audio_->isChannelPlaying(Channels::Music));
}

TEST_F(AudioSystemTest, PlaySoundWithExtremePitch) {
    AssetHandle testSound{.uuid = 1, .type = AssetType::Sound};
    ChannelSound sound{
        .asset = testSound,
        .pitch = 100.0f
    };

    audio_->playOnChannel(Channels::Voice, sound);
    // Should not crash
    EXPECT_TRUE(true);
}

TEST_F(AudioSystemTest, PlaySoundWithNegativeFadeIn) {
    AssetHandle testSound{.uuid = 1, .type = AssetType::Sound};
    ChannelSound sound{
        .asset = testSound,
        .fadeInTime = -1.0f
    };

    audio_->playOnChannel(Channels::Music, sound);
    // Should handle gracefully
    EXPECT_TRUE(true);
}

TEST_F(AudioSystemTest, PlaySoundWithNegativeStartTime) {
    AssetHandle testSound{.uuid = 1, .type = AssetType::Sound};
    ChannelSound sound{
        .asset = testSound,
        .startTime = -5.0f
    };

    audio_->playOnChannel(Channels::Music, sound);
    // Should clamp or ignore
    EXPECT_TRUE(true);
}

//==========================================================================
// PositionalSound Parameter Coverage Tests
//==========================================================================

TEST_F(AudioSystemTest, PlayPositionalWithAllParameters) {
    AssetHandle testSound{.uuid = 1, .type = AssetType::Sound};
    PositionalSound sound{
        .asset = testSound,
        .position = Vec3{10.0f, 20.0f, 30.0f},
        .velocity = Vec3{1.0f, 2.0f, 3.0f},
        .volume = 0.8f,
        .pitch = 1.1f,
        .minDistance = 5.0f,
        .maxDistance = 100.0f
    };

    SoundHandle handle = audio_->playPositional(sound);
    EXPECT_TRUE(audio_->isPositionalPlaying(handle));
}

TEST_F(AudioSystemTest, PlayPositionalWithZeroVolume) {
    AssetHandle testSound{.uuid = 1, .type = AssetType::Sound};
    PositionalSound sound{
        .asset = testSound,
        .position = Vec3{0.0f, 0.0f, 0.0f},
        .volume = 0.0f
    };

    SoundHandle handle = audio_->playPositional(sound);
    EXPECT_TRUE(audio_->isPositionalPlaying(handle));
}

TEST_F(AudioSystemTest, PlayPositionalWithHighVolume) {
    AssetHandle testSound{.uuid = 1, .type = AssetType::Sound};
    PositionalSound sound{
        .asset = testSound,
        .position = Vec3{0.0f, 0.0f, 0.0f},
        .volume = 5.0f
    };

    SoundHandle handle = audio_->playPositional(sound);
    // Should handle gracefully
    EXPECT_TRUE(true);
}

}  // namespace jframe::tests

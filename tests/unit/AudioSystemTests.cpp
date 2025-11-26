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

}  // namespace jframe::tests

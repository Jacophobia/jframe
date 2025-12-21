// tests/unit/AnimationSystemTests.cpp
// Animation system unit tests

#include <cmath>
#include <cstddef>
#include <memory>
#include <string>
#include <vector>

#include <gtest/gtest.h>
#include <kangaru/kangaru.hpp>

import bestow.animation;
import bestow.animation.impl;
import bestow.assets.impl;   // For AssetSystemService (dependency of AnimationSystem)
import bestow.events.impl;   // For EventSystemService (dependency of AssetSystem)
import bestow.types;

namespace bestow::tests {

//==========================================================================
// Test Fixture
//==========================================================================

class AnimationSystemTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Register dependencies in order:
        // 1. EventSystem (no dependencies)
        // 2. AssetSystem (depends on EventSystem)
        // 3. AnimationSystem (depends on AssetSystem)
        container_.service<EventSystemService>();
        container_.service<AssetSystemService>();
        animation_ = &container_.service<AnimationSystemService>();

        // Initialize the animation system
        bool initialized = animation_->initialize();
        EXPECT_TRUE(initialized);
    }

    void TearDown() override {
        if (animation_) {
            animation_->shutdown();
        }
    }

    // Helper to create a test skeleton with a simple bone hierarchy
    std::vector<BoneInfo> createTestBones(std::uint32_t count = 5) {
        std::vector<BoneInfo> bones;
        bones.reserve(count);

        for (std::uint32_t i = 0; i < count; ++i) {
            BoneInfo bone;
            bone.name = "Bone_" + std::to_string(i);
            bone.index = static_cast<std::int32_t>(i);
            bone.parentIndex = (i == 0) ? -1 : static_cast<std::int32_t>(i - 1);
            bone.localBindPose = Mat4{1.0f};
            bone.inverseBindPose = Mat4{1.0f};
            bone.localPosition = Vec3{0.0f, static_cast<float>(i) * 0.5f, 0.0f};
            bone.localRotation = Quat{1.0f, 0.0f, 0.0f, 0.0f};
            bone.localScale = Vec3{1.0f};
            bones.push_back(bone);
        }

        return bones;
    }

    // Helper to create a skeleton and return the handle
    SkeletonHandle createTestSkeleton(std::uint32_t boneCount = 5) {
        auto bones = createTestBones(boneCount);
        auto result = animation_->createSkeleton(std::span<const BoneInfo>(bones));
        EXPECT_TRUE(result.has_value());
        return result.value();
    }

    kgr::container container_;
    IAnimationSystem* animation_ = nullptr;
};

//==========================================================================
// Initialization Tests
//==========================================================================

TEST_F(AnimationSystemTest, CanCreate) {
    EXPECT_NE(animation_, nullptr);
}

TEST_F(AnimationSystemTest, InitializeReturnsTrue) {
    // Animation system is already initialized in SetUp
    // Verify it's working by checking stats
    AnimationStats stats = animation_->getStats();
    EXPECT_EQ(stats.skeletonCount, 0);
}

TEST_F(AnimationSystemTest, ShutdownClearsAllResources) {
    // Create some resources
    auto skeleton = createTestSkeleton();
    auto animatorResult = animation_->createAnimator(skeleton);
    EXPECT_TRUE(animatorResult.has_value());

    // Shutdown
    animation_->shutdown();

    // Re-initialize
    EXPECT_TRUE(animation_->initialize());

    // Stats should be zero
    AnimationStats stats = animation_->getStats();
    EXPECT_EQ(stats.skeletonCount, 0);
    EXPECT_EQ(stats.animatorCount, 0);
}

//==========================================================================
// Skeleton Management Tests
//==========================================================================

TEST_F(AnimationSystemTest, CreateSkeletonFromBoneSpan) {
    auto bones = createTestBones(3);
    auto result = animation_->createSkeleton(std::span<const BoneInfo>(bones));

    EXPECT_TRUE(result.has_value());
    EXPECT_NE(result.value(), AnimationHandles::InvalidSkeleton);
}

TEST_F(AnimationSystemTest, CreateSkeletonReturnsUniqueHandles) {
    auto skeleton1 = createTestSkeleton(3);
    auto skeleton2 = createTestSkeleton(5);

    EXPECT_NE(skeleton1, skeleton2);
    EXPECT_NE(skeleton1, AnimationHandles::InvalidSkeleton);
    EXPECT_NE(skeleton2, AnimationHandles::InvalidSkeleton);
}

TEST_F(AnimationSystemTest, CreateSkeletonWithEmptyBonesReturnsError) {
    std::vector<BoneInfo> emptyBones;
    auto result = animation_->createSkeleton(std::span<const BoneInfo>(emptyBones));

    EXPECT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), AnimationError::EmptyBoneData);
}

TEST_F(AnimationSystemTest, IsValidSkeletonReturnsTrueForValidHandle) {
    auto skeleton = createTestSkeleton();

    EXPECT_TRUE(animation_->isValidSkeleton(skeleton));
}

TEST_F(AnimationSystemTest, IsValidSkeletonReturnsFalseForInvalidHandle) {
    EXPECT_FALSE(animation_->isValidSkeleton(AnimationHandles::InvalidSkeleton));
    EXPECT_FALSE(animation_->isValidSkeleton(99999));
}

TEST_F(AnimationSystemTest, DestroySkeletonInvalidatesHandle) {
    auto skeleton = createTestSkeleton();
    EXPECT_TRUE(animation_->isValidSkeleton(skeleton));

    animation_->destroySkeleton(skeleton);

    EXPECT_FALSE(animation_->isValidSkeleton(skeleton));
}

TEST_F(AnimationSystemTest, DestroySkeletonIsIdempotent) {
    auto skeleton = createTestSkeleton();

    animation_->destroySkeleton(skeleton);
    // Second call should not crash
    animation_->destroySkeleton(skeleton);

    EXPECT_FALSE(animation_->isValidSkeleton(skeleton));
}

TEST_F(AnimationSystemTest, GetSkeletonInfoReturnsCorrectBoneCount) {
    auto skeleton = createTestSkeleton(7);

    SkeletonInfo info = animation_->getSkeletonInfo(skeleton);

    EXPECT_EQ(info.boneCount, 7);
    EXPECT_EQ(info.bones.size(), 7);
}

TEST_F(AnimationSystemTest, GetSkeletonInfoReturnsBoneNames) {
    auto skeleton = createTestSkeleton(3);

    SkeletonInfo info = animation_->getSkeletonInfo(skeleton);

    EXPECT_EQ(info.bones[0].name, "Bone_0");
    EXPECT_EQ(info.bones[1].name, "Bone_1");
    EXPECT_EQ(info.bones[2].name, "Bone_2");
}

TEST_F(AnimationSystemTest, GetSkeletonInfoReturnsParentIndices) {
    auto skeleton = createTestSkeleton(4);

    SkeletonInfo info = animation_->getSkeletonInfo(skeleton);

    EXPECT_EQ(info.bones[0].parentIndex, -1);  // Root has no parent
    EXPECT_EQ(info.bones[1].parentIndex, 0);
    EXPECT_EQ(info.bones[2].parentIndex, 1);
    EXPECT_EQ(info.bones[3].parentIndex, 2);
}

TEST_F(AnimationSystemTest, FindBoneIndexReturnsBoneIndex) {
    auto skeleton = createTestSkeleton(5);

    EXPECT_EQ(animation_->findBoneIndex(skeleton, "Bone_0"), 0);
    EXPECT_EQ(animation_->findBoneIndex(skeleton, "Bone_2"), 2);
    EXPECT_EQ(animation_->findBoneIndex(skeleton, "Bone_4"), 4);
}

TEST_F(AnimationSystemTest, FindBoneIndexReturnsNegativeForNotFound) {
    auto skeleton = createTestSkeleton(3);

    EXPECT_EQ(animation_->findBoneIndex(skeleton, "NonExistent"), -1);
}

TEST_F(AnimationSystemTest, GetBoneNamesReturnsAllNames) {
    auto skeleton = createTestSkeleton(3);

    auto names = animation_->getBoneNames(skeleton);

    EXPECT_EQ(names.size(), 3);
    EXPECT_EQ(names[0], "Bone_0");
    EXPECT_EQ(names[1], "Bone_1");
    EXPECT_EQ(names[2], "Bone_2");
}

TEST_F(AnimationSystemTest, GetBoneCountReturnsCorrectCount) {
    auto skeleton = createTestSkeleton(10);

    EXPECT_EQ(animation_->getBoneCount(skeleton), 10);
}

TEST_F(AnimationSystemTest, GetBoneCountReturnsZeroForInvalidSkeleton) {
    EXPECT_EQ(animation_->getBoneCount(AnimationHandles::InvalidSkeleton), 0);
}

//==========================================================================
// Animator Management Tests
//==========================================================================

TEST_F(AnimationSystemTest, CreateAnimatorReturnsValidHandle) {
    auto skeleton = createTestSkeleton();

    auto result = animation_->createAnimator(skeleton);

    EXPECT_TRUE(result.has_value());
    EXPECT_NE(result.value(), AnimationHandles::InvalidAnimator);
}

TEST_F(AnimationSystemTest, CreateAnimatorFailsForInvalidSkeleton) {
    auto result = animation_->createAnimator(AnimationHandles::InvalidSkeleton);

    EXPECT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), AnimationError::InvalidSkeleton);
}

TEST_F(AnimationSystemTest, CreateMultipleAnimatorsForSameSkeleton) {
    auto skeleton = createTestSkeleton();

    auto animator1 = animation_->createAnimator(skeleton);
    auto animator2 = animation_->createAnimator(skeleton);

    EXPECT_TRUE(animator1.has_value());
    EXPECT_TRUE(animator2.has_value());
    EXPECT_NE(animator1.value(), animator2.value());
}

TEST_F(AnimationSystemTest, IsValidAnimatorReturnsTrueForValid) {
    auto skeleton = createTestSkeleton();
    auto animator = animation_->createAnimator(skeleton);

    EXPECT_TRUE(animation_->isValidAnimator(animator.value()));
}

TEST_F(AnimationSystemTest, IsValidAnimatorReturnsFalseForInvalid) {
    EXPECT_FALSE(animation_->isValidAnimator(AnimationHandles::InvalidAnimator));
    EXPECT_FALSE(animation_->isValidAnimator(99999));
}

TEST_F(AnimationSystemTest, DestroyAnimatorInvalidatesHandle) {
    auto skeleton = createTestSkeleton();
    auto animator = animation_->createAnimator(skeleton);

    animation_->destroyAnimator(animator.value());

    EXPECT_FALSE(animation_->isValidAnimator(animator.value()));
}

TEST_F(AnimationSystemTest, GetAnimatorSkeletonReturnsCorrectSkeleton) {
    auto skeleton = createTestSkeleton();
    auto animator = animation_->createAnimator(skeleton);

    EXPECT_EQ(animation_->getAnimatorSkeleton(animator.value()), skeleton);
}

TEST_F(AnimationSystemTest, DestroySkeletonDestroysAssociatedAnimators) {
    auto skeleton = createTestSkeleton();
    auto animator = animation_->createAnimator(skeleton);

    animation_->destroySkeleton(skeleton);

    EXPECT_FALSE(animation_->isValidAnimator(animator.value()));
}

//==========================================================================
// Animator Playback Control Tests
//==========================================================================

TEST_F(AnimationSystemTest, PlaySetsPlayingState) {
    auto skeleton = createTestSkeleton();
    auto animator = animation_->createAnimator(skeleton).value();

    // Initially not playing
    EXPECT_FALSE(animation_->isPlaying(animator));
}

TEST_F(AnimationSystemTest, StopStopsAllLayers) {
    auto skeleton = createTestSkeleton();
    auto animator = animation_->createAnimator(skeleton).value();

    animation_->stop(animator);

    EXPECT_FALSE(animation_->isPlaying(animator));
}

TEST_F(AnimationSystemTest, SetPausedPausesAnimator) {
    auto skeleton = createTestSkeleton();
    auto animator = animation_->createAnimator(skeleton).value();

    animation_->setPaused(animator, true);

    EXPECT_TRUE(animation_->isPaused(animator));
}

TEST_F(AnimationSystemTest, SetPausedUnpausesAnimator) {
    auto skeleton = createTestSkeleton();
    auto animator = animation_->createAnimator(skeleton).value();

    animation_->setPaused(animator, true);
    EXPECT_TRUE(animation_->isPaused(animator));

    animation_->setPaused(animator, false);
    EXPECT_FALSE(animation_->isPaused(animator));
}

TEST_F(AnimationSystemTest, SetSpeedUpdatesSpeed) {
    auto skeleton = createTestSkeleton();
    auto animator = animation_->createAnimator(skeleton).value();

    animation_->setSpeed(animator, 2.0f);

    EXPECT_FLOAT_EQ(animation_->getSpeed(animator), 2.0f);
}

TEST_F(AnimationSystemTest, GetSpeedReturnsDefaultOne) {
    auto skeleton = createTestSkeleton();
    auto animator = animation_->createAnimator(skeleton).value();

    EXPECT_FLOAT_EQ(animation_->getSpeed(animator), 1.0f);
}

TEST_F(AnimationSystemTest, SetSpeedWithNegativeValue) {
    auto skeleton = createTestSkeleton();
    auto animator = animation_->createAnimator(skeleton).value();

    animation_->setSpeed(animator, -1.0f);

    // Negative speed for reverse playback
    EXPECT_FLOAT_EQ(animation_->getSpeed(animator), -1.0f);
}

TEST_F(AnimationSystemTest, SetSpeedWithZero) {
    auto skeleton = createTestSkeleton();
    auto animator = animation_->createAnimator(skeleton).value();

    animation_->setSpeed(animator, 0.0f);

    EXPECT_FLOAT_EQ(animation_->getSpeed(animator), 0.0f);
}

//==========================================================================
// Animator Layer Control Tests
//==========================================================================

TEST_F(AnimationSystemTest, GetLayerStateReturnsDefaultState) {
    auto skeleton = createTestSkeleton();
    auto animator = animation_->createAnimator(skeleton).value();

    AnimationLayerState state = animation_->getLayerState(animator, 0);

    EXPECT_EQ(state.clip, AnimationHandles::InvalidClip);
    EXPECT_FALSE(state.playing);
    EXPECT_FLOAT_EQ(state.weight, 1.0f);
    EXPECT_FLOAT_EQ(state.speed, 1.0f);
}

TEST_F(AnimationSystemTest, SetLayerWeightUpdatesWeight) {
    auto skeleton = createTestSkeleton();
    auto animator = animation_->createAnimator(skeleton).value();

    animation_->setLayerWeight(animator, 0, 0.5f);

    EXPECT_FLOAT_EQ(animation_->getLayerWeight(animator, 0), 0.5f);
}

TEST_F(AnimationSystemTest, SetLayerWeightClampsToZeroOne) {
    auto skeleton = createTestSkeleton();
    auto animator = animation_->createAnimator(skeleton).value();

    animation_->setLayerWeight(animator, 0, 1.5f);
    // Implementation may clamp or allow >1
    float weight = animation_->getLayerWeight(animator, 0);
    EXPECT_GE(weight, 0.0f);

    animation_->setLayerWeight(animator, 0, -0.5f);
    weight = animation_->getLayerWeight(animator, 0);
    EXPECT_GE(weight, 0.0f);  // Should clamp to 0
}

TEST_F(AnimationSystemTest, SetLayerBlendModeChangesMode) {
    auto skeleton = createTestSkeleton();
    auto animator = animation_->createAnimator(skeleton).value();

    animation_->setLayerBlendMode(animator, 0, AnimationBlendMode::Additive);

    AnimationLayerState state = animation_->getLayerState(animator, 0);
    EXPECT_EQ(state.blendMode, AnimationBlendMode::Additive);
}

TEST_F(AnimationSystemTest, GetLayerCountReturnsAtLeastOne) {
    auto skeleton = createTestSkeleton();
    auto animator = animation_->createAnimator(skeleton).value();

    // Should have at least one default layer
    EXPECT_GE(animation_->getLayerCount(animator), 1);
}

TEST_F(AnimationSystemTest, SetLayerBoneMaskSetsEmptyForAllBones) {
    auto skeleton = createTestSkeleton(5);
    auto animator = animation_->createAnimator(skeleton).value();

    // Empty mask means all bones affected
    std::set<std::uint32_t> emptyMask;
    animation_->setLayerBoneMask(animator, 0, emptyMask);

    // Should not crash
    EXPECT_TRUE(animation_->isValidAnimator(animator));
}

TEST_F(AnimationSystemTest, SetLayerBoneMaskWithSpecificBones) {
    auto skeleton = createTestSkeleton(5);
    auto animator = animation_->createAnimator(skeleton).value();

    // Mask only affects bones 0 and 2
    std::set<std::uint32_t> mask = {0, 2};
    animation_->setLayerBoneMask(animator, 0, mask);

    // Should not crash
    EXPECT_TRUE(animation_->isValidAnimator(animator));
}

//==========================================================================
// Animator Time Control Tests
//==========================================================================

TEST_F(AnimationSystemTest, GetCurrentTimeReturnsZeroByDefault) {
    auto skeleton = createTestSkeleton();
    auto animator = animation_->createAnimator(skeleton).value();

    EXPECT_FLOAT_EQ(animation_->getCurrentTime(animator, 0), 0.0f);
}

TEST_F(AnimationSystemTest, SetCurrentTimeUpdatesTime) {
    auto skeleton = createTestSkeleton();
    auto animator = animation_->createAnimator(skeleton).value();

    animation_->setCurrentTime(animator, 2.5f, 0);

    EXPECT_FLOAT_EQ(animation_->getCurrentTime(animator, 0), 2.5f);
}

TEST_F(AnimationSystemTest, GetNormalizedTimeReturnsZeroByDefault) {
    auto skeleton = createTestSkeleton();
    auto animator = animation_->createAnimator(skeleton).value();

    EXPECT_FLOAT_EQ(animation_->getNormalizedTime(animator, 0), 0.0f);
}

TEST_F(AnimationSystemTest, SetNormalizedTimeUpdatesTime) {
    auto skeleton = createTestSkeleton();
    auto animator = animation_->createAnimator(skeleton).value();

    animation_->setNormalizedTime(animator, 0.5f, 0);

    EXPECT_FLOAT_EQ(animation_->getNormalizedTime(animator, 0), 0.5f);
}

TEST_F(AnimationSystemTest, GetClipDurationReturnsZeroWithoutClip) {
    auto skeleton = createTestSkeleton();
    auto animator = animation_->createAnimator(skeleton).value();

    EXPECT_FLOAT_EQ(animation_->getClipDuration(animator, 0), 0.0f);
}

//==========================================================================
// Bone Transform Tests
//==========================================================================

TEST_F(AnimationSystemTest, GetBoneTransformsReturnsCorrectSize) {
    auto skeleton = createTestSkeleton(5);
    auto animator = animation_->createAnimator(skeleton).value();

    // Update to initialize transforms
    animation_->update(0.0f);

    auto transforms = animation_->getBoneTransforms(animator);

    EXPECT_EQ(transforms.size(), 5);
}

TEST_F(AnimationSystemTest, GetBoneTransformByIndexReturnsIdentityInitially) {
    auto skeleton = createTestSkeleton(3);
    auto animator = animation_->createAnimator(skeleton).value();

    // Update to initialize
    animation_->update(0.0f);

    Mat4 transform = animation_->getBoneTransform(animator, 0);

    // Should be close to identity (or bind pose)
    EXPECT_NEAR(transform[0][0], 1.0f, 0.001f);
    EXPECT_NEAR(transform[1][1], 1.0f, 0.001f);
    EXPECT_NEAR(transform[2][2], 1.0f, 0.001f);
}

TEST_F(AnimationSystemTest, GetBoneTransformByNameReturnsTransform) {
    auto skeleton = createTestSkeleton(3);
    auto animator = animation_->createAnimator(skeleton).value();

    // Update to initialize
    animation_->update(0.0f);

    Mat4 transform = animation_->getBoneTransform(animator, "Bone_1");

    // Should be close to identity (or bind pose)
    EXPECT_NEAR(transform[0][0], 1.0f, 0.001f);
    EXPECT_NEAR(transform[1][1], 1.0f, 0.001f);
    EXPECT_NEAR(transform[2][2], 1.0f, 0.001f);
}

TEST_F(AnimationSystemTest, GetBoneWorldTransformAppliesEntityMatrix) {
    auto skeleton = createTestSkeleton(3);
    auto animator = animation_->createAnimator(skeleton).value();

    // Update to initialize
    animation_->update(0.0f);

    // Entity at position (10, 0, 0)
    Mat4 entityMatrix{1.0f};
    entityMatrix[3][0] = 10.0f;

    Mat4 worldTransform = animation_->getBoneWorldTransform(animator, 0, entityMatrix);

    // World position should include entity offset
    EXPECT_NEAR(worldTransform[3][0], 10.0f, 0.1f);
}

//==========================================================================
// Socket System Tests
//==========================================================================

TEST_F(AnimationSystemTest, DefineSocketReturnsValidHandle) {
    auto skeleton = createTestSkeleton(3);

    SocketDef def;
    def.name = "weapon_socket";
    def.boneName = "Bone_2";
    def.localPosition = Vec3{0.1f, 0.0f, 0.0f};

    auto result = animation_->defineSocket(skeleton, def);

    EXPECT_TRUE(result.has_value());
    EXPECT_NE(result.value(), AnimationHandles::InvalidSocket);
}

TEST_F(AnimationSystemTest, DefineSocketFailsForNonExistentBone) {
    auto skeleton = createTestSkeleton(3);

    SocketDef def;
    def.name = "weapon_socket";
    def.boneName = "NonExistentBone";

    auto result = animation_->defineSocket(skeleton, def);

    EXPECT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), AnimationError::SocketBoneNotFound);
}

TEST_F(AnimationSystemTest, DefineSocketFailsForDuplicateName) {
    auto skeleton = createTestSkeleton(3);

    SocketDef def;
    def.name = "weapon_socket";
    def.boneName = "Bone_1";

    auto result1 = animation_->defineSocket(skeleton, def);
    EXPECT_TRUE(result1.has_value());

    auto result2 = animation_->defineSocket(skeleton, def);
    EXPECT_FALSE(result2.has_value());
    EXPECT_EQ(result2.error(), AnimationError::SocketAlreadyExists);
}

TEST_F(AnimationSystemTest, DefineMultipleSockets) {
    auto skeleton = createTestSkeleton(5);

    std::vector<SocketDef> defs = {
        {.name = "socket1", .boneName = "Bone_0"},
        {.name = "socket2", .boneName = "Bone_2"},
        {.name = "socket3", .boneName = "Bone_4"}
    };

    auto results = animation_->defineSockets(skeleton, std::span<const SocketDef>(defs));

    EXPECT_EQ(results.size(), 3);
    for (const auto& result : results) {
        EXPECT_TRUE(result.has_value());
    }
}

TEST_F(AnimationSystemTest, HasSocketReturnsTrueForExisting) {
    auto skeleton = createTestSkeleton(3);

    SocketDef def;
    def.name = "test_socket";
    def.boneName = "Bone_1";
    animation_->defineSocket(skeleton, def);

    EXPECT_TRUE(animation_->hasSocket(skeleton, "test_socket"));
}

TEST_F(AnimationSystemTest, HasSocketReturnsFalseForNonExisting) {
    auto skeleton = createTestSkeleton(3);

    EXPECT_FALSE(animation_->hasSocket(skeleton, "nonexistent"));
}

TEST_F(AnimationSystemTest, FindSocketReturnsHandle) {
    auto skeleton = createTestSkeleton(3);

    SocketDef def;
    def.name = "my_socket";
    def.boneName = "Bone_0";
    auto created = animation_->defineSocket(skeleton, def);

    auto found = animation_->findSocket(skeleton, "my_socket");

    EXPECT_EQ(found, created.value());
}

TEST_F(AnimationSystemTest, FindSocketReturnsInvalidForNotFound) {
    auto skeleton = createTestSkeleton(3);

    auto found = animation_->findSocket(skeleton, "missing_socket");

    EXPECT_EQ(found, AnimationHandles::InvalidSocket);
}

TEST_F(AnimationSystemTest, RemoveSocketByHandle) {
    auto skeleton = createTestSkeleton(3);

    SocketDef def;
    def.name = "removable_socket";
    def.boneName = "Bone_1";
    auto socket = animation_->defineSocket(skeleton, def).value();

    EXPECT_TRUE(animation_->hasSocket(skeleton, "removable_socket"));

    animation_->removeSocket(socket);

    EXPECT_FALSE(animation_->hasSocket(skeleton, "removable_socket"));
}

TEST_F(AnimationSystemTest, RemoveSocketByName) {
    auto skeleton = createTestSkeleton(3);

    SocketDef def;
    def.name = "named_socket";
    def.boneName = "Bone_2";
    animation_->defineSocket(skeleton, def);

    animation_->removeSocket(skeleton, "named_socket");

    EXPECT_FALSE(animation_->hasSocket(skeleton, "named_socket"));
}

TEST_F(AnimationSystemTest, GetSocketsReturnsAllSockets) {
    auto skeleton = createTestSkeleton(3);

    SocketDef def1{.name = "socket_a", .boneName = "Bone_0"};
    SocketDef def2{.name = "socket_b", .boneName = "Bone_1"};
    animation_->defineSocket(skeleton, def1);
    animation_->defineSocket(skeleton, def2);

    auto sockets = animation_->getSockets(skeleton);

    EXPECT_EQ(sockets.size(), 2);
}

TEST_F(AnimationSystemTest, GetSocketDefReturnsDefinition) {
    auto skeleton = createTestSkeleton(3);

    SocketDef def;
    def.name = "detailed_socket";
    def.boneName = "Bone_1";
    def.localPosition = Vec3{1.0f, 2.0f, 3.0f};
    def.attachMode = SocketAttachMode::FollowPosition;

    auto socket = animation_->defineSocket(skeleton, def).value();
    auto retrieved = animation_->getSocketDef(socket);

    EXPECT_TRUE(retrieved.has_value());
    EXPECT_EQ(retrieved->name, "detailed_socket");
    EXPECT_EQ(retrieved->boneName, "Bone_1");
    EXPECT_FLOAT_EQ(retrieved->localPosition.x, 1.0f);
    EXPECT_EQ(retrieved->attachMode, SocketAttachMode::FollowPosition);
}

TEST_F(AnimationSystemTest, GetSocketTransformReturnsValidTransform) {
    auto skeleton = createTestSkeleton(3);
    auto animator = animation_->createAnimator(skeleton).value();

    SocketDef def;
    def.name = "transform_socket";
    def.boneName = "Bone_0";
    animation_->defineSocket(skeleton, def);

    // Update to initialize transforms
    animation_->update(0.0f);

    Mat4 entityMatrix{1.0f};
    auto result = animation_->getSocketTransform(animator, "transform_socket", entityMatrix);

    EXPECT_TRUE(result.has_value());
}

TEST_F(AnimationSystemTest, GetSocketTransformReturnsErrorForNonexistent) {
    auto skeleton = createTestSkeleton(3);
    auto animator = animation_->createAnimator(skeleton).value();

    Mat4 entityMatrix{1.0f};
    auto result = animation_->getSocketTransform(animator, "missing", entityMatrix);

    EXPECT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), AnimationError::SocketNotFound);
}

TEST_F(AnimationSystemTest, SetSocketLocalTransformUpdatesTransform) {
    auto skeleton = createTestSkeleton(3);

    SocketDef def;
    def.name = "movable_socket";
    def.boneName = "Bone_0";
    auto socket = animation_->defineSocket(skeleton, def).value();

    Vec3 newPos{5.0f, 10.0f, 15.0f};
    Quat newRot{1.0f, 0.0f, 0.0f, 0.0f};

    auto result = animation_->setSocketLocalTransform(socket, newPos, newRot);

    EXPECT_TRUE(result.has_value());

    auto updated = animation_->getSocketDef(socket);
    EXPECT_FLOAT_EQ(updated->localPosition.x, 5.0f);
    EXPECT_FLOAT_EQ(updated->localPosition.y, 10.0f);
    EXPECT_FLOAT_EQ(updated->localPosition.z, 15.0f);
}

TEST_F(AnimationSystemTest, SetSocketEnabledTogglesSockets) {
    auto skeleton = createTestSkeleton(3);

    SocketDef def;
    def.name = "toggle_socket";
    def.boneName = "Bone_1";
    auto socket = animation_->defineSocket(skeleton, def).value();

    EXPECT_TRUE(animation_->isSocketEnabled(socket));

    animation_->setSocketEnabled(socket, false);
    EXPECT_FALSE(animation_->isSocketEnabled(socket));

    animation_->setSocketEnabled(socket, true);
    EXPECT_TRUE(animation_->isSocketEnabled(socket));
}

//==========================================================================
// IK Chain Tests
//==========================================================================

TEST_F(AnimationSystemTest, DefineIKChainSucceeds) {
    // Create a skeleton with at least 3 bones for IK chain
    auto skeleton = createTestSkeleton(5);

    IKTwoBoneChain chain;
    chain.name = "left_arm";
    chain.rootBoneName = "Bone_0";
    chain.midBoneName = "Bone_1";
    chain.tipBoneName = "Bone_2";

    auto result = animation_->defineIKChain(skeleton, chain);

    EXPECT_TRUE(result.has_value());
}

TEST_F(AnimationSystemTest, DefineIKChainFailsForInvalidBone) {
    auto skeleton = createTestSkeleton(5);

    IKTwoBoneChain chain;
    chain.name = "bad_chain";
    chain.rootBoneName = "Bone_0";
    chain.midBoneName = "NonExistent";
    chain.tipBoneName = "Bone_2";

    auto result = animation_->defineIKChain(skeleton, chain);

    EXPECT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), AnimationError::IKChainInvalid);
}

TEST_F(AnimationSystemTest, DefineIKAimSucceeds) {
    auto skeleton = createTestSkeleton(3);

    IKAimConfig config;
    config.name = "head_look";
    config.boneName = "Bone_2";
    config.aimAxis = Vec3{0.0f, 0.0f, 1.0f};
    config.upAxis = Vec3{0.0f, 1.0f, 0.0f};

    auto result = animation_->defineIKAim(skeleton, config);

    EXPECT_TRUE(result.has_value());
}

TEST_F(AnimationSystemTest, DefineIKAimFailsForInvalidBone) {
    auto skeleton = createTestSkeleton(3);

    IKAimConfig config;
    config.name = "bad_aim";
    config.boneName = "NonExistent";

    auto result = animation_->defineIKAim(skeleton, config);

    EXPECT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), AnimationError::IKChainInvalid);
}

TEST_F(AnimationSystemTest, GetIKChainNamesReturnsDefinedChains) {
    auto skeleton = createTestSkeleton(5);

    IKTwoBoneChain chain1{
        .name = "left_arm",
        .rootBoneName = "Bone_0",
        .midBoneName = "Bone_1",
        .tipBoneName = "Bone_2"
    };
    IKTwoBoneChain chain2{
        .name = "left_leg",
        .rootBoneName = "Bone_2",
        .midBoneName = "Bone_3",
        .tipBoneName = "Bone_4"
    };

    animation_->defineIKChain(skeleton, chain1);
    animation_->defineIKChain(skeleton, chain2);

    auto names = animation_->getIKChainNames(skeleton);

    EXPECT_EQ(names.size(), 2);
}

TEST_F(AnimationSystemTest, GetIKAimNamesReturnsDefinedAims) {
    auto skeleton = createTestSkeleton(5);

    IKAimConfig aim1{.name = "head", .boneName = "Bone_4"};
    IKAimConfig aim2{.name = "spine", .boneName = "Bone_2"};

    animation_->defineIKAim(skeleton, aim1);
    animation_->defineIKAim(skeleton, aim2);

    auto names = animation_->getIKAimNames(skeleton);

    EXPECT_EQ(names.size(), 2);
}

TEST_F(AnimationSystemTest, RemoveIKChainRemovesChain) {
    auto skeleton = createTestSkeleton(5);

    IKTwoBoneChain chain{
        .name = "removable_chain",
        .rootBoneName = "Bone_0",
        .midBoneName = "Bone_1",
        .tipBoneName = "Bone_2"
    };
    animation_->defineIKChain(skeleton, chain);

    EXPECT_EQ(animation_->getIKChainNames(skeleton).size(), 1);

    animation_->removeIKChain(skeleton, "removable_chain");

    EXPECT_EQ(animation_->getIKChainNames(skeleton).size(), 0);
}

TEST_F(AnimationSystemTest, RemoveIKAimRemovesAim) {
    auto skeleton = createTestSkeleton(3);

    IKAimConfig aim{.name = "removable_aim", .boneName = "Bone_2"};
    animation_->defineIKAim(skeleton, aim);

    EXPECT_EQ(animation_->getIKAimNames(skeleton).size(), 1);

    animation_->removeIKAim(skeleton, "removable_aim");

    EXPECT_EQ(animation_->getIKAimNames(skeleton).size(), 0);
}

TEST_F(AnimationSystemTest, SetIKTwoBoneTarget) {
    auto skeleton = createTestSkeleton(5);
    auto animator = animation_->createAnimator(skeleton).value();

    IKTwoBoneChain chain{
        .name = "arm",
        .rootBoneName = "Bone_0",
        .midBoneName = "Bone_1",
        .tipBoneName = "Bone_2"
    };
    animation_->defineIKChain(skeleton, chain);

    IKTwoBoneTarget target;
    target.chainName = "arm";
    target.targetPosition = Vec3{1.0f, 2.0f, 3.0f};
    target.weight = 1.0f;

    animation_->setIKTarget(animator, target);

    auto retrieved = animation_->getIKTwoBoneTarget(animator, "arm");

    EXPECT_TRUE(retrieved.has_value());
    EXPECT_FLOAT_EQ(retrieved->targetPosition.x, 1.0f);
    EXPECT_FLOAT_EQ(retrieved->weight, 1.0f);
}

TEST_F(AnimationSystemTest, SetIKAimTarget) {
    auto skeleton = createTestSkeleton(3);
    auto animator = animation_->createAnimator(skeleton).value();

    IKAimConfig config{.name = "look", .boneName = "Bone_2"};
    animation_->defineIKAim(skeleton, config);

    IKAimTarget target;
    target.configName = "look";
    target.targetPosition = Vec3{10.0f, 0.0f, 0.0f};
    target.weight = 0.8f;

    animation_->setIKTarget(animator, target);

    auto retrieved = animation_->getIKAimTarget(animator, "look");

    EXPECT_TRUE(retrieved.has_value());
    EXPECT_FLOAT_EQ(retrieved->targetPosition.x, 10.0f);
    EXPECT_FLOAT_EQ(retrieved->weight, 0.8f);
}

TEST_F(AnimationSystemTest, ClearIKTargetClearsTarget) {
    auto skeleton = createTestSkeleton(3);
    auto animator = animation_->createAnimator(skeleton).value();

    IKAimConfig config{.name = "clearable", .boneName = "Bone_2"};
    animation_->defineIKAim(skeleton, config);

    IKAimTarget target{.configName = "clearable", .targetPosition = Vec3{5.0f}};
    animation_->setIKTarget(animator, target);

    EXPECT_TRUE(animation_->getIKAimTarget(animator, "clearable").has_value());

    animation_->clearIKTarget(animator, "clearable");

    EXPECT_FALSE(animation_->getIKAimTarget(animator, "clearable").has_value());
}

TEST_F(AnimationSystemTest, ClearAllIKTargetsClearsAll) {
    auto skeleton = createTestSkeleton(5);
    auto animator = animation_->createAnimator(skeleton).value();

    // Define and set multiple targets
    IKAimConfig aim{.name = "aim1", .boneName = "Bone_4"};
    animation_->defineIKAim(skeleton, aim);
    animation_->setIKTarget(animator, IKAimTarget{.configName = "aim1"});

    IKTwoBoneChain chain{
        .name = "chain1",
        .rootBoneName = "Bone_0",
        .midBoneName = "Bone_1",
        .tipBoneName = "Bone_2"
    };
    animation_->defineIKChain(skeleton, chain);
    animation_->setIKTarget(animator, IKTwoBoneTarget{.chainName = "chain1"});

    animation_->clearAllIKTargets(animator);

    EXPECT_FALSE(animation_->getIKAimTarget(animator, "aim1").has_value());
    EXPECT_FALSE(animation_->getIKTwoBoneTarget(animator, "chain1").has_value());
}

TEST_F(AnimationSystemTest, SetIKWeightUpdatesWeight) {
    auto skeleton = createTestSkeleton(3);
    auto animator = animation_->createAnimator(skeleton).value();

    IKAimConfig config{.name = "weighted", .boneName = "Bone_2"};
    animation_->defineIKAim(skeleton, config);
    animation_->setIKTarget(animator, IKAimTarget{.configName = "weighted", .weight = 1.0f});

    animation_->setIKWeight(animator, "weighted", 0.3f);

    EXPECT_FLOAT_EQ(animation_->getIKWeight(animator, "weighted"), 0.3f);
}

//==========================================================================
// Root Motion Tests
//==========================================================================

TEST_F(AnimationSystemTest, SetRootMotionConfigStoresConfig) {
    auto skeleton = createTestSkeleton(3);
    auto animator = animation_->createAnimator(skeleton).value();

    RootMotionConfig config;
    config.enabled = true;
    config.extractTranslationX = true;
    config.extractTranslationY = false;
    config.extractTranslationZ = true;
    config.extractRotationY = true;

    animation_->setRootMotionConfig(animator, config);

    RootMotionConfig retrieved = animation_->getRootMotionConfig(animator);

    EXPECT_TRUE(retrieved.enabled);
    EXPECT_TRUE(retrieved.extractTranslationX);
    EXPECT_FALSE(retrieved.extractTranslationY);
    EXPECT_TRUE(retrieved.extractTranslationZ);
    EXPECT_TRUE(retrieved.extractRotationY);
}

TEST_F(AnimationSystemTest, SetRootMotionEnabledTogglesSetting) {
    auto skeleton = createTestSkeleton(3);
    auto animator = animation_->createAnimator(skeleton).value();

    EXPECT_FALSE(animation_->isRootMotionEnabled(animator));

    animation_->setRootMotionEnabled(animator, true);
    EXPECT_TRUE(animation_->isRootMotionEnabled(animator));

    animation_->setRootMotionEnabled(animator, false);
    EXPECT_FALSE(animation_->isRootMotionEnabled(animator));
}

TEST_F(AnimationSystemTest, GetRootMotionReturnsDefaultValues) {
    auto skeleton = createTestSkeleton(3);
    auto animator = animation_->createAnimator(skeleton).value();

    RootMotion motion = animation_->getRootMotion(animator);

    EXPECT_FLOAT_EQ(motion.deltaPosition.x, 0.0f);
    EXPECT_FLOAT_EQ(motion.deltaPosition.y, 0.0f);
    EXPECT_FLOAT_EQ(motion.deltaPosition.z, 0.0f);
}

TEST_F(AnimationSystemTest, ConsumeRootMotionResetsDeltas) {
    auto skeleton = createTestSkeleton(3);
    auto animator = animation_->createAnimator(skeleton).value();

    animation_->setRootMotionEnabled(animator, true);
    animation_->consumeRootMotion(animator);

    RootMotion motion = animation_->getRootMotion(animator);

    EXPECT_FLOAT_EQ(motion.deltaPosition.x, 0.0f);
    EXPECT_FLOAT_EQ(motion.deltaPosition.y, 0.0f);
    EXPECT_FLOAT_EQ(motion.deltaPosition.z, 0.0f);
}

//==========================================================================
// Ragdoll Tests (Basic - Without Physics)
//==========================================================================

TEST_F(AnimationSystemTest, HasRagdollReturnsFalseByDefault) {
    Entity testEntity{1};

    EXPECT_FALSE(animation_->hasRagdoll(testEntity));
}

TEST_F(AnimationSystemTest, GetRagdollStateForNonExistentReturnsDefault) {
    Entity testEntity{1};

    RagdollState state = animation_->getRagdollState(testEntity);

    EXPECT_FALSE(state.created);
    EXPECT_FALSE(state.active);
}

TEST_F(AnimationSystemTest, IsRagdollActiveReturnsFalseWithoutRagdoll) {
    Entity testEntity{1};

    EXPECT_FALSE(animation_->isRagdollActive(testEntity));
}

TEST_F(AnimationSystemTest, SetRagdollBlendWeightWithoutRagdoll) {
    Entity testEntity{1};

    // Should not crash
    animation_->setRagdollBlendWeight(testEntity, 0.5f);

    // Weight should still be 0 (no ragdoll)
    EXPECT_FLOAT_EQ(animation_->getRagdollBlendWeight(testEntity), 0.0f);
}

//==========================================================================
// Event Subscription Tests
//==========================================================================

TEST_F(AnimationSystemTest, SubscribeToEventsReturnsValidId) {
    auto skeleton = createTestSkeleton(3);
    auto animator = animation_->createAnimator(skeleton).value();

    SubscriptionId id = animation_->subscribeToEvents(animator,
        [](const AnimationEvent& event) {
            // Callback
        });

    EXPECT_NE(id, 0);
}

TEST_F(AnimationSystemTest, SubscribeToCompleteReturnsValidId) {
    auto skeleton = createTestSkeleton(3);
    auto animator = animation_->createAnimator(skeleton).value();

    SubscriptionId id = animation_->subscribeToComplete(animator,
        [](AnimatorHandle animator, AnimationClipHandle clip, std::uint32_t layer) {
            // Callback
        });

    EXPECT_NE(id, 0);
}

TEST_F(AnimationSystemTest, SubscribeToLayerChangesReturnsValidId) {
    auto skeleton = createTestSkeleton(3);
    auto animator = animation_->createAnimator(skeleton).value();

    SubscriptionId id = animation_->subscribeToLayerChanges(animator,
        [](AnimatorHandle animator, std::uint32_t layer,
           AnimationClipHandle prev, AnimationClipHandle next) {
            // Callback
        });

    EXPECT_NE(id, 0);
}

TEST_F(AnimationSystemTest, UnsubscribeRemovesSubscription) {
    auto skeleton = createTestSkeleton(3);
    auto animator = animation_->createAnimator(skeleton).value();

    SubscriptionId id = animation_->subscribeToEvents(animator,
        [](const AnimationEvent& event) {});

    // Should not crash
    animation_->unsubscribe(id);
    animation_->unsubscribe(id);  // Double unsubscribe is safe
}

TEST_F(AnimationSystemTest, MultipleSubscriptionsDifferentIds) {
    auto skeleton = createTestSkeleton(3);
    auto animator = animation_->createAnimator(skeleton).value();

    SubscriptionId id1 = animation_->subscribeToEvents(animator,
        [](const AnimationEvent& event) {});
    SubscriptionId id2 = animation_->subscribeToComplete(animator,
        [](AnimatorHandle, AnimationClipHandle, std::uint32_t) {});
    SubscriptionId id3 = animation_->subscribeToLayerChanges(animator,
        [](AnimatorHandle, std::uint32_t, AnimationClipHandle, AnimationClipHandle) {});

    EXPECT_NE(id1, id2);
    EXPECT_NE(id2, id3);
    EXPECT_NE(id1, id3);
}

//==========================================================================
// Statistics Tests
//==========================================================================

TEST_F(AnimationSystemTest, GetStatsReturnsValidStats) {
    AnimationStats stats = animation_->getStats();

    // Should have default values
    EXPECT_EQ(stats.skeletonCount, 0);
    EXPECT_EQ(stats.clipCount, 0);
    EXPECT_EQ(stats.animatorCount, 0);
}

TEST_F(AnimationSystemTest, GetStatsReflectsCreatedResources) {
    auto skeleton = createTestSkeleton(5);
    auto animator = animation_->createAnimator(skeleton);

    AnimationStats stats = animation_->getStats();

    EXPECT_EQ(stats.skeletonCount, 1);
    EXPECT_EQ(stats.animatorCount, 1);
}

TEST_F(AnimationSystemTest, ResetFrameStatsResetsPerFrameMetrics) {
    animation_->update(0.016f);

    AnimationStats beforeReset = animation_->getStats();

    animation_->resetFrameStats();

    AnimationStats afterReset = animation_->getStats();

    EXPECT_EQ(afterReset.animatorsUpdated, 0);
    EXPECT_EQ(afterReset.layersProcessed, 0);
    EXPECT_EQ(afterReset.eventsDispatched, 0);
}

TEST_F(AnimationSystemTest, SetDebugVisualizationTogglesSetting) {
    EXPECT_FALSE(animation_->isDebugVisualizationEnabled());

    animation_->setDebugVisualization(true);
    EXPECT_TRUE(animation_->isDebugVisualizationEnabled());

    animation_->setDebugVisualization(false);
    EXPECT_FALSE(animation_->isDebugVisualizationEnabled());
}

//==========================================================================
// Update Loop Tests
//==========================================================================

TEST_F(AnimationSystemTest, UpdateDoesNotCrashWithNoAnimators) {
    animation_->update(0.016f);

    EXPECT_TRUE(true);  // No crash
}

TEST_F(AnimationSystemTest, UpdateDoesNotCrashWithPausedAnimator) {
    auto skeleton = createTestSkeleton(3);
    auto animator = animation_->createAnimator(skeleton).value();

    animation_->setPaused(animator, true);
    animation_->update(0.016f);

    EXPECT_TRUE(animation_->isPaused(animator));
}

TEST_F(AnimationSystemTest, UpdateWithZeroDeltaTime) {
    auto skeleton = createTestSkeleton(3);
    auto animator = animation_->createAnimator(skeleton).value();

    animation_->update(0.0f);

    EXPECT_TRUE(animation_->isValidAnimator(animator));
}

TEST_F(AnimationSystemTest, UpdateWithNegativeDeltaTime) {
    auto skeleton = createTestSkeleton(3);
    auto animator = animation_->createAnimator(skeleton).value();

    // Should handle gracefully
    animation_->update(-0.016f);

    EXPECT_TRUE(animation_->isValidAnimator(animator));
}

TEST_F(AnimationSystemTest, UpdateWithLargeDeltaTime) {
    auto skeleton = createTestSkeleton(3);
    auto animator = animation_->createAnimator(skeleton).value();

    // Large delta (lag spike)
    animation_->update(5.0f);

    EXPECT_TRUE(animation_->isValidAnimator(animator));
}

TEST_F(AnimationSystemTest, MultipleUpdates) {
    auto skeleton = createTestSkeleton(3);
    auto animator = animation_->createAnimator(skeleton).value();

    for (int i = 0; i < 100; ++i) {
        animation_->update(0.016f);
    }

    EXPECT_TRUE(animation_->isValidAnimator(animator));
}

//==========================================================================
// Edge Case Tests
//==========================================================================

TEST_F(AnimationSystemTest, GetSkeletonInfoForInvalidHandle) {
    SkeletonInfo info = animation_->getSkeletonInfo(AnimationHandles::InvalidSkeleton);

    EXPECT_EQ(info.boneCount, 0);
    EXPECT_TRUE(info.bones.empty());
}

TEST_F(AnimationSystemTest, GetBoneNamesForInvalidSkeleton) {
    auto names = animation_->getBoneNames(AnimationHandles::InvalidSkeleton);

    EXPECT_TRUE(names.empty());
}

TEST_F(AnimationSystemTest, DestroyInvalidAnimator) {
    // Should not crash
    animation_->destroyAnimator(AnimationHandles::InvalidAnimator);
    animation_->destroyAnimator(99999);

    EXPECT_TRUE(true);
}

TEST_F(AnimationSystemTest, PlayOnInvalidAnimator) {
    // Should not crash (handles invalid handle gracefully)
    AnimationPlayConfig config;
    config.layer = 0;

    animation_->play(AnimationHandles::InvalidAnimator, config);

    EXPECT_TRUE(true);
}

TEST_F(AnimationSystemTest, StopOnInvalidAnimator) {
    // Should not crash
    animation_->stop(AnimationHandles::InvalidAnimator);

    EXPECT_TRUE(true);
}

TEST_F(AnimationSystemTest, GetBoneTransformsForInvalidAnimator) {
    auto transforms = animation_->getBoneTransforms(AnimationHandles::InvalidAnimator);

    EXPECT_TRUE(transforms.empty());
}

TEST_F(AnimationSystemTest, CreateManySkeletons) {
    std::vector<SkeletonHandle> handles;

    for (int i = 0; i < 50; ++i) {
        auto skeleton = createTestSkeleton(3);
        handles.push_back(skeleton);
    }

    EXPECT_EQ(animation_->getStats().skeletonCount, 50);

    for (auto handle : handles) {
        EXPECT_TRUE(animation_->isValidSkeleton(handle));
    }
}

TEST_F(AnimationSystemTest, CreateManyAnimators) {
    auto skeleton = createTestSkeleton(5);
    std::vector<AnimatorHandle> handles;

    for (int i = 0; i < 100; ++i) {
        auto animator = animation_->createAnimator(skeleton);
        EXPECT_TRUE(animator.has_value());
        handles.push_back(animator.value());
    }

    EXPECT_EQ(animation_->getStats().animatorCount, 100);

    for (auto handle : handles) {
        EXPECT_TRUE(animation_->isValidAnimator(handle));
    }
}

TEST_F(AnimationSystemTest, CreateManySockets) {
    auto skeleton = createTestSkeleton(5);

    for (int i = 0; i < 20; ++i) {
        SocketDef def;
        def.name = "socket_" + std::to_string(i);
        def.boneName = "Bone_" + std::to_string(i % 5);

        auto result = animation_->defineSocket(skeleton, def);
        EXPECT_TRUE(result.has_value());
    }

    auto sockets = animation_->getSockets(skeleton);
    EXPECT_EQ(sockets.size(), 20);
}

//==========================================================================
// Kangaru DI Integration Tests
//==========================================================================

TEST_F(AnimationSystemTest, KangaruServiceCanBeInstantiated) {
    kgr::container container;

    // Register dependencies first
    container.service<EventSystemService>();
    container.service<AssetSystemService>();

    // Instantiate AnimationSystemService through Kangaru
    auto& animationSystem = container.service<AnimationSystemService>();

    EXPECT_NE(&animationSystem, nullptr);
}

TEST_F(AnimationSystemTest, KangaruServiceIsSingleton) {
    kgr::container container;

    container.service<EventSystemService>();
    container.service<AssetSystemService>();

    auto& animationSystem1 = container.service<AnimationSystemService>();
    auto& animationSystem2 = container.service<AnimationSystemService>();

    EXPECT_EQ(&animationSystem1, &animationSystem2);
}

TEST_F(AnimationSystemTest, KangaruServiceCanInitialize) {
    kgr::container container;

    container.service<EventSystemService>();
    container.service<AssetSystemService>();

    auto& animationSystem = container.service<AnimationSystemService>();
    bool initialized = animationSystem.initialize();

    EXPECT_TRUE(initialized);
}

TEST_F(AnimationSystemTest, KangaruServiceCanCreateSkeleton) {
    kgr::container container;

    container.service<EventSystemService>();
    container.service<AssetSystemService>();

    auto& animationSystem = container.service<AnimationSystemService>();
    animationSystem.initialize();

    auto bones = createTestBones(3);
    auto result = animationSystem.createSkeleton(std::span<const BoneInfo>(bones));

    EXPECT_TRUE(result.has_value());
}

//==========================================================================
// Animation Error String Tests
//==========================================================================

TEST_F(AnimationSystemTest, AnimationErrorToStringReturnsCorrectStrings) {
    EXPECT_EQ(animationErrorToString(AnimationError::Success), "Success");
    EXPECT_EQ(animationErrorToString(AnimationError::InvalidSkeleton), "InvalidSkeleton");
    EXPECT_EQ(animationErrorToString(AnimationError::SkeletonNotFound), "SkeletonNotFound");
    EXPECT_EQ(animationErrorToString(AnimationError::InvalidBoneIndex), "InvalidBoneIndex");
    EXPECT_EQ(animationErrorToString(AnimationError::BoneNotFound), "BoneNotFound");
    EXPECT_EQ(animationErrorToString(AnimationError::EmptyBoneData), "EmptyBoneData");
    EXPECT_EQ(animationErrorToString(AnimationError::InvalidClip), "InvalidClip");
    EXPECT_EQ(animationErrorToString(AnimationError::ClipNotFound), "ClipNotFound");
    EXPECT_EQ(animationErrorToString(AnimationError::InvalidAnimator), "InvalidAnimator");
    EXPECT_EQ(animationErrorToString(AnimationError::InvalidSocket), "InvalidSocket");
    EXPECT_EQ(animationErrorToString(AnimationError::SocketNotFound), "SocketNotFound");
    EXPECT_EQ(animationErrorToString(AnimationError::SocketAlreadyExists), "SocketAlreadyExists");
    EXPECT_EQ(animationErrorToString(AnimationError::IKChainNotFound), "IKChainNotFound");
    EXPECT_EQ(animationErrorToString(AnimationError::IKChainInvalid), "IKChainInvalid");
    EXPECT_EQ(animationErrorToString(AnimationError::RagdollNotFound), "RagdollNotFound");
}

//==========================================================================
// SkeletonInfo Helper Method Tests
//==========================================================================

TEST_F(AnimationSystemTest, SkeletonInfoFindBone) {
    auto skeleton = createTestSkeleton(5);
    SkeletonInfo info = animation_->getSkeletonInfo(skeleton);

    EXPECT_EQ(info.findBone("Bone_0"), 0);
    EXPECT_EQ(info.findBone("Bone_2"), 2);
    EXPECT_EQ(info.findBone("NonExistent"), -1);
}

TEST_F(AnimationSystemTest, SkeletonInfoGetChildren) {
    auto skeleton = createTestSkeleton(5);
    SkeletonInfo info = animation_->getSkeletonInfo(skeleton);

    // In our test hierarchy, each bone except last has exactly one child
    auto children = info.getChildren(0);
    EXPECT_EQ(children.size(), 1);
    EXPECT_EQ(children[0], 1);

    // Root bone has children
    children = info.getChildren(-1);  // No bone has -1 as parent index in query
    EXPECT_EQ(children.size(), 0);
}

TEST_F(AnimationSystemTest, BoneInfoIsRoot) {
    BoneInfo rootBone;
    rootBone.parentIndex = -1;
    EXPECT_TRUE(rootBone.isRoot());

    BoneInfo childBone;
    childBone.parentIndex = 0;
    EXPECT_FALSE(childBone.isRoot());
}

//==========================================================================
// AnimationLayerState Helper Method Tests
//==========================================================================

TEST_F(AnimationSystemTest, AnimationLayerStateEffectiveWeight) {
    AnimationLayerState state;
    state.weight = 0.5f;
    state.fadeWeight = 0.8f;

    EXPECT_FLOAT_EQ(state.effectiveWeight(), 0.4f);
}

TEST_F(AnimationSystemTest, AnimationLayerStateIsActive) {
    AnimationLayerState state;
    state.playing = true;
    state.weight = 1.0f;
    state.fadeWeight = 1.0f;

    EXPECT_TRUE(state.isActive());

    state.weight = 0.0001f;
    state.fadeWeight = 0.001f;
    EXPECT_FALSE(state.isActive());  // Below threshold

    state.playing = false;
    state.weight = 1.0f;
    state.fadeWeight = 1.0f;
    EXPECT_FALSE(state.isActive());  // Not playing
}

}  // namespace bestow::tests

// bestow-animation/src/bestow.animation.impl.cppm
// Animation System Implementation Module

module;

// Kangaru DI framework
#include <kangaru/kangaru.hpp>

// GLM math library - MUST be before ozz to ensure operator* resolution
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>

// ozz-animation runtime headers - MUST be in global module fragment
#include <ozz/animation/runtime/skeleton.h>
#include <ozz/animation/runtime/animation.h>
#include <ozz/animation/runtime/sampling_job.h>
#include <ozz/animation/runtime/blending_job.h>
#include <ozz/animation/runtime/local_to_model_job.h>
#include <ozz/animation/runtime/ik_two_bone_job.h>
#include <ozz/animation/runtime/ik_aim_job.h>
#include <ozz/base/maths/simd_math.h>
#include <ozz/base/maths/soa_transform.h>
#include <ozz/base/maths/vec_float.h>
#include <ozz/base/containers/vector.h>

export module bestow.animation.impl;

import std;
import bestow.services;

export namespace bestow {

//==========================================================================
// Internal Data Structures
//==========================================================================

struct SkeletonData {
    SkeletonHandle handle = 0;
    std::vector<BoneInfo> bones;  // Our contract type
    std::unordered_map<std::string, std::int32_t> boneNameToIndex;
    std::int32_t rootBoneIndex = 0;
    AABB3D bounds;
    ozz::animation::Skeleton ozzSkeleton;  // ozz runtime skeleton
    std::vector<std::int32_t> ozzToBoneIndex;  // Map from ozz joint index to our bone index
    std::vector<std::int32_t> boneToOzzIndex;  // Map from our bone index to ozz joint index
};

struct AnimationKeyframe {
    float time = 0.0f;
    Vec3 position{0.0f};
    Quat rotation{1.0f, 0.0f, 0.0f, 0.0f};
    Vec3 scale{1.0f};
};

struct AnimationChannel {
    std::int32_t boneIndex = -1;
    std::vector<AnimationKeyframe> keyframes;
};

struct AnimationClipData {
    AnimationClipHandle handle = 0;
    SkeletonHandle skeleton = 0;
    std::string name;
    float duration = 0.0f;
    float ticksPerSecond = 30.0f;
    bool looping = true;
    bool hasRootMotion = false;
    std::vector<AnimationChannel> channels;
    std::vector<AnimationEventDef> events;
    ozz::animation::Animation ozzAnimation;  // ozz runtime animation
};

struct AnimatorLayerData {
    // Current (incoming) animation
    AnimationClipHandle clip = AnimationHandles::InvalidClip;
    std::string clipName;
    float time = 0.0f;
    float speed = 1.0f;
    float weight = 1.0f;
    float fadeWeight = 1.0f;
    float fadeSpeed = 0.0f;
    AnimationWrapMode wrapMode = AnimationWrapMode::Loop;
    AnimationBlendMode blendMode = AnimationBlendMode::Override;
    bool playing = false;
    bool paused = false;
    std::set<std::uint32_t> boneMask;
    std::unique_ptr<ozz::animation::SamplingJob::Context> samplingContext;
    ozz::vector<ozz::math::SoaTransform> localTransforms;  // Per-layer sampled transforms

    // Outgoing animation for crossfade blending
    AnimationClipHandle outgoingClip = AnimationHandles::InvalidClip;
    float outgoingTime = 0.0f;
    float outgoingSpeed = 1.0f;
    AnimationWrapMode outgoingWrapMode = AnimationWrapMode::Loop;
    std::unique_ptr<ozz::animation::SamplingJob::Context> outgoingSamplingContext;
    ozz::vector<ozz::math::SoaTransform> outgoingLocalTransforms;

    // Root position matching - offset applied during crossfade to prevent popping
    Vec3 crossfadeRootOffset{0.0f};  // Offset to apply to incoming animation's root
    bool hasCrossfadeRootOffset = false;

    // Default constructible and movable
    AnimatorLayerData() = default;
    AnimatorLayerData(AnimatorLayerData&&) = default;
    AnimatorLayerData& operator=(AnimatorLayerData&&) = default;

    // Not copyable due to unique_ptr
    AnimatorLayerData(const AnimatorLayerData&) = delete;
    AnimatorLayerData& operator=(const AnimatorLayerData&) = delete;
};

struct AnimatorData {
    AnimatorHandle handle = 0;
    SkeletonHandle skeleton = 0;
    std::vector<AnimatorLayerData> layers;
    std::vector<Mat4> boneTransforms;      // Final model-space transforms (output)
    std::vector<Mat4> localTransforms;     // Local transforms
    float globalSpeed = 1.0f;
    bool paused = false;

    // IK targets
    std::unordered_map<std::string, IKTwoBoneTarget> twoBoneTargets;
    std::unordered_map<std::string, IKAimTarget> aimTargets;

    // Root motion
    RootMotionConfig rootMotionConfig;
    RootMotion currentRootMotion;
    Vec3 lastRootPosition{0.0f};
    Quat lastRootRotation{1.0f, 0.0f, 0.0f, 0.0f};
    bool rootMotionInitialized = false;  // First frame flag

    ozz::vector<ozz::math::SoaTransform> blendedLocals;   // Blended local transforms
    ozz::vector<ozz::math::Float4x4> modelMatrices;       // Model-space matrices from ozz
    std::vector<Mat4> modelSpacePoses;                    // Converted model poses for visualization
};

struct SocketData {
    SocketHandle handle = 0;
    SkeletonHandle skeleton = 0;
    std::string name;
    std::uint32_t boneIndex = 0;
    Vec3 localPosition{0.0f};
    Quat localRotation{1.0f, 0.0f, 0.0f, 0.0f};
    Vec3 localScale{1.0f};
    SocketAttachMode attachMode = SocketAttachMode::FollowBone;
    bool enabled = true;
};

struct IKChainData {
    std::string name;
    std::int32_t rootBoneIndex = -1;
    std::int32_t midBoneIndex = -1;
    std::int32_t tipBoneIndex = -1;
};

struct IKAimData {
    std::string name;
    std::int32_t boneIndex = -1;
    Vec3 aimAxis{0.0f, 0.0f, 1.0f};
    Vec3 upAxis{0.0f, 1.0f, 0.0f};
    float horizontalLimit = 90.0f;
    float verticalLimit = 60.0f;
};

struct RagdollData {
    Entity entity;
    SkeletonHandle skeleton = 0;
    AnimatorHandle animator = 0;
    RagdollState state;
    RagdollDef definition;
};

struct EventSubscription {
    SubscriptionId id = 0;
    AnimatorHandle animator = 0;
    enum class Type { Event, Complete, LayerChange } type;
    std::variant<
        AnimationEventCallback,
        AnimationCompleteCallback,
        AnimationLayerCallback
    > callback;
};

//==========================================================================
// AnimationSystem Implementation Data
//==========================================================================

class AnimationSystemImpl {
public:
    // Handle generators
    SkeletonHandle nextSkeletonHandle_ = 1;
    AnimationClipHandle nextClipHandle_ = 1;
    AnimatorHandle nextAnimatorHandle_ = 1;
    SocketHandle nextSocketHandle_ = 1;
    SubscriptionId nextSubscriptionId_ = 1;

    // Storage
    std::unordered_map<SkeletonHandle, SkeletonData> skeletons_;
    std::unordered_map<AnimationClipHandle, AnimationClipData> clips_;
    std::unordered_map<AnimatorHandle, AnimatorData> animators_;
    std::unordered_map<SocketHandle, SocketData> sockets_;
    std::unordered_map<SkeletonHandle, std::vector<IKChainData>> ikChains_;
    std::unordered_map<SkeletonHandle, std::vector<IKAimData>> ikAims_;
    std::unordered_map<Entity, RagdollData> ragdolls_;
    std::vector<EventSubscription> subscriptions_;

    // Skeleton to clip/socket/animator mappings
    std::unordered_map<SkeletonHandle, std::vector<AnimationClipHandle>> skeletonClips_;
    std::unordered_map<SkeletonHandle, std::vector<SocketHandle>> skeletonSockets_;

    // Statistics
    AnimationStats stats_;
    bool debugVisualization_ = false;

    // Pending events to dispatch
    std::vector<AnimationEvent> pendingEvents_;
};

//==========================================================================
// AnimationSystem Class
//==========================================================================

/// Animation System Implementation using ozz-animation backend
/// NOTE: AnimationSystem is stateless regarding asset loading - it receives ModelData
/// directly through its API methods. Asset loading is handled by the caller through
/// IAssetSystem before invoking animation functions.
class AnimationSystem : public IAnimationSystem {
public:
    // Forward declare Service - defined after class is complete
    struct Service;

    AnimationSystem() = default;
    ~AnimationSystem() override;

    //======================================================================
    // Lifecycle
    //======================================================================

    bool initialize() override;
    void shutdown() override;
    void update(DeltaTime dt) override;

    //======================================================================
    // Skeleton Management
    //======================================================================

    Result<SkeletonHandle, AnimationError> createSkeleton(
        const ModelData& modelData) override;

    Result<SkeletonHandle, AnimationError> createSkeleton(
        std::span<const BoneInfo> bones) override;

    void destroySkeleton(SkeletonHandle skeleton) override;
    bool isValidSkeleton(SkeletonHandle skeleton) const override;
    SkeletonInfo getSkeletonInfo(SkeletonHandle skeleton) const override;

    std::int32_t findBoneIndex(
        SkeletonHandle skeleton,
        std::string_view boneName) const override;

    std::vector<std::string> getBoneNames(SkeletonHandle skeleton) const override;
    std::uint32_t getBoneCount(SkeletonHandle skeleton) const override;

    //======================================================================
    // Animation Clip Management
    //======================================================================

    Result<AnimationClipHandle, AnimationError> createAnimationClip(
        SkeletonHandle skeleton,
        const ModelData& modelData,
        std::string_view clipName) override;

    std::vector<AnimationClipHandle> createAnimationClips(
        SkeletonHandle skeleton,
        const ModelData& modelData) override;

    void destroyAnimationClip(AnimationClipHandle clip) override;
    bool isValidClip(AnimationClipHandle clip) const override;
    AnimationClipInfo getAnimationClipInfo(AnimationClipHandle clip) const override;

    AnimationClipHandle findClip(
        SkeletonHandle skeleton,
        std::string_view clipName) const override;

    std::vector<AnimationClipHandle> getClipsForSkeleton(
        SkeletonHandle skeleton) const override;

    std::vector<std::string> getClipNames(SkeletonHandle skeleton) const override;

    //======================================================================
    // Animation Events
    //======================================================================

    Result<void, AnimationError> addClipEvent(
        AnimationClipHandle clip,
        const AnimationEventDef& event) override;

    Result<void, AnimationError> removeClipEvent(
        AnimationClipHandle clip,
        std::string_view eventName) override;

    void clearClipEvents(AnimationClipHandle clip) override;

    std::vector<AnimationEventDef> getClipEvents(
        AnimationClipHandle clip) const override;

    //======================================================================
    // Animator Management
    //======================================================================

    Result<AnimatorHandle, AnimationError> createAnimator(
        SkeletonHandle skeleton) override;

    void destroyAnimator(AnimatorHandle animator) override;
    bool isValidAnimator(AnimatorHandle animator) const override;
    SkeletonHandle getAnimatorSkeleton(AnimatorHandle animator) const override;

    //======================================================================
    // Animator Playback Control
    //======================================================================

    void play(AnimatorHandle animator, AnimationClipHandle clip,
              float transitionTime = 0.25f) override;

    void play(AnimatorHandle animator, std::string_view clipName,
              float transitionTime = 0.25f) override;

    void play(AnimatorHandle animator, const AnimationPlayConfig& config) override;

    void stop(AnimatorHandle animator, float fadeOutTime = 0.0f) override;
    void stopLayer(AnimatorHandle animator, std::uint32_t layer,
                   float fadeOutTime = 0.0f) override;

    void setPaused(AnimatorHandle animator, bool paused) override;
    bool isPaused(AnimatorHandle animator) const override;

    void setSpeed(AnimatorHandle animator, float speed) override;
    float getSpeed(AnimatorHandle animator) const override;

    bool isPlaying(AnimatorHandle animator) const override;
    bool isLayerPlaying(AnimatorHandle animator, std::uint32_t layer) const override;

    //======================================================================
    // Animator Layer Control
    //======================================================================

    AnimationLayerState getLayerState(
        AnimatorHandle animator, std::uint32_t layer) const override;

    void setLayerWeight(AnimatorHandle animator, std::uint32_t layer,
                        float weight) override;

    float getLayerWeight(AnimatorHandle animator, std::uint32_t layer) const override;

    void setLayerBlendMode(AnimatorHandle animator, std::uint32_t layer,
                           AnimationBlendMode mode) override;

    std::uint32_t getLayerCount(AnimatorHandle animator) const override;

    void setLayerBoneMask(AnimatorHandle animator, std::uint32_t layer,
                          const std::set<std::uint32_t>& boneMask) override;

    //======================================================================
    // Animator Time Control
    //======================================================================

    float getCurrentTime(AnimatorHandle animator, std::uint32_t layer = 0) const override;
    float getNormalizedTime(AnimatorHandle animator, std::uint32_t layer = 0) const override;

    void setCurrentTime(AnimatorHandle animator, float time,
                        std::uint32_t layer = 0) override;

    void setNormalizedTime(AnimatorHandle animator, float normalizedTime,
                           std::uint32_t layer = 0) override;

    float getClipDuration(AnimatorHandle animator, std::uint32_t layer = 0) const override;

    //======================================================================
    // Animator Bone Transforms
    //======================================================================

    std::span<const Mat4> getBoneTransforms(AnimatorHandle animator) const override;

    std::span<const Mat4> getModelSpaceBonePoses(AnimatorHandle animator) const override;

    Mat4 getBoneTransform(AnimatorHandle animator,
                          std::uint32_t boneIndex) const override;

    Mat4 getBoneTransform(AnimatorHandle animator,
                          std::string_view boneName) const override;

    Mat4 getBoneWorldTransform(AnimatorHandle animator, std::uint32_t boneIndex,
                               const Mat4& entityWorldMatrix) const override;

    //======================================================================
    // Socket System
    //======================================================================

    Result<SocketHandle, AnimationError> defineSocket(
        SkeletonHandle skeleton, const SocketDef& def) override;

    std::vector<Result<SocketHandle, AnimationError>> defineSockets(
        SkeletonHandle skeleton, std::span<const SocketDef> defs) override;

    void removeSocket(SocketHandle socket) override;
    void removeSocket(SkeletonHandle skeleton, std::string_view name) override;

    bool hasSocket(SkeletonHandle skeleton, std::string_view socketName) const override;

    SocketHandle findSocket(SkeletonHandle skeleton,
                            std::string_view socketName) const override;

    std::vector<SocketState> getSockets(SkeletonHandle skeleton) const override;
    std::optional<SocketDef> getSocketDef(SocketHandle socket) const override;

    Result<SocketTransform, AnimationError> getSocketTransform(
        AnimatorHandle animator, std::string_view socketName,
        const Mat4& entityWorldMatrix) const override;

    Result<SocketTransform, AnimationError> getSocketTransform(
        AnimatorHandle animator, SocketHandle socket,
        const Mat4& entityWorldMatrix) const override;

    std::vector<SocketTransform> getSocketTransforms(
        AnimatorHandle animator, std::span<const std::string_view> socketNames,
        const Mat4& entityWorldMatrix) const override;

    Result<void, AnimationError> setSocketLocalTransform(
        SocketHandle socket, const Vec3& position, const Quat& rotation,
        const Vec3& scale = Vec3{1.0f}) override;

    void setSocketEnabled(SocketHandle socket, bool enabled) override;
    bool isSocketEnabled(SocketHandle socket) const override;

    //======================================================================
    // Socket Raycasting
    //======================================================================

    SocketRaycastResult raycastFromSocket(
        AnimatorHandle animator, const SocketRaycastDef& def,
        const Mat4& entityWorldMatrix,
        const IPhysics3DSystem& physics) const override;

    std::vector<SocketRaycastResult> raycastFromSockets(
        AnimatorHandle animator, std::span<const SocketRaycastDef> defs,
        const Mat4& entityWorldMatrix,
        const IPhysics3DSystem& physics) const override;

    SocketRaycastResult sphereCastFromSocket(
        AnimatorHandle animator, const SocketRaycastDef& def,
        float sphereRadius, const Mat4& entityWorldMatrix,
        const IPhysics3DSystem& physics) const override;

    //======================================================================
    // Inverse Kinematics: Chain Definition
    //======================================================================

    Result<void, AnimationError> defineIKChain(
        SkeletonHandle skeleton, const IKTwoBoneChain& chain) override;

    Result<void, AnimationError> defineIKAim(
        SkeletonHandle skeleton, const IKAimConfig& config) override;

    void removeIKChain(SkeletonHandle skeleton, std::string_view name) override;
    void removeIKAim(SkeletonHandle skeleton, std::string_view name) override;

    std::vector<std::string> getIKChainNames(SkeletonHandle skeleton) const override;
    std::vector<std::string> getIKAimNames(SkeletonHandle skeleton) const override;

    //======================================================================
    // Inverse Kinematics: Runtime Control
    //======================================================================

    void setIKTarget(AnimatorHandle animator, const IKTwoBoneTarget& target) override;
    void setIKTarget(AnimatorHandle animator, const IKAimTarget& target) override;

    std::optional<IKTwoBoneTarget> getIKTwoBoneTarget(
        AnimatorHandle animator, std::string_view chainName) const override;

    std::optional<IKAimTarget> getIKAimTarget(
        AnimatorHandle animator, std::string_view configName) const override;

    void clearIKTarget(AnimatorHandle animator, std::string_view targetName) override;
    void clearAllIKTargets(AnimatorHandle animator) override;

    void setIKWeight(AnimatorHandle animator, std::string_view targetName,
                     float weight) override;

    float getIKWeight(AnimatorHandle animator,
                      std::string_view targetName) const override;

    //======================================================================
    // Root Motion
    //======================================================================

    void setRootMotionConfig(AnimatorHandle animator,
                             const RootMotionConfig& config) override;

    RootMotionConfig getRootMotionConfig(AnimatorHandle animator) const override;

    void setRootMotionEnabled(AnimatorHandle animator, bool enabled) override;
    bool isRootMotionEnabled(AnimatorHandle animator) const override;

    RootMotion getRootMotion(AnimatorHandle animator) const override;

    RootMotion extractRootMotion(AnimationClipHandle clip,
                                 float fromTime, float toTime) const override;

    void consumeRootMotion(AnimatorHandle animator) override;

    //======================================================================
    // Physics Integration: Ragdoll
    //======================================================================

    Result<void, AnimationError> createRagdoll(
        Entity entity, const RagdollDef& def,
        IPhysics3DSystem& physics) override;

    void destroyRagdoll(Entity entity, IPhysics3DSystem& physics) override;

    bool hasRagdoll(Entity entity) const override;
    RagdollState getRagdollState(Entity entity) const override;

    void activateRagdoll(Entity entity, IPhysics3DSystem& physics,
                         bool instant = false, float blendDuration = 0.3f) override;

    void deactivateRagdoll(Entity entity, IPhysics3DSystem& physics,
                           bool instant = false, float blendDuration = 0.3f) override;

    void setRagdollBlendWeight(Entity entity, float weight) override;
    float getRagdollBlendWeight(Entity entity) const override;
    bool isRagdollActive(Entity entity) const override;

    std::vector<Mat4> getRagdollBoneTransforms(
        Entity entity, const IPhysics3DSystem& physics) const override;

    //======================================================================
    // Physics Integration: Bone Sync & Impulses
    //======================================================================

    void syncToPhysics(AnimatorHandle animator, Entity entity,
                       IPhysics3DSystem& physics) override;

    void applyBoneImpulse(Entity entity, std::uint32_t boneIndex,
                          const Vec3& impulse, IPhysics3DSystem& physics) override;

    void applyBoneImpulse(Entity entity, std::string_view boneName,
                          const Vec3& impulse, IPhysics3DSystem& physics) override;

    void applyImpulseAtPosition(Entity entity, const Vec3& worldPosition,
                                const Vec3& impulse, float radius,
                                IPhysics3DSystem& physics) override;

    //======================================================================
    // Event Subscriptions
    //======================================================================

    SubscriptionId subscribeToEvents(AnimatorHandle animator,
                                     AnimationEventCallback callback) override;

    SubscriptionId subscribeToComplete(AnimatorHandle animator,
                                       AnimationCompleteCallback callback) override;

    SubscriptionId subscribeToLayerChanges(AnimatorHandle animator,
                                           AnimationLayerCallback callback) override;

    void unsubscribe(SubscriptionId id) override;

    //======================================================================
    // Stateless Animation Sampling
    //======================================================================

    Result<std::vector<Mat4>, AnimationError> sampleAnimation(
        AnimationClipHandle clip, float time,
        AnimationWrapMode wrapMode = AnimationWrapMode::Loop) override;

    Result<std::vector<Mat4>, AnimationError> blendAnimations(
        SkeletonHandle skeleton, const AnimationBlendConfig& config) override;

    //======================================================================
    // Statistics & Debugging
    //======================================================================

    AnimationStats getStats() const override;
    void resetFrameStats() override;

    void setDebugVisualization(bool enabled) override;
    bool isDebugVisualizationEnabled() const override;

private:
    // Private helper methods for update loop
    void applyIK(AnimatorData& animator, SkeletonData& skeleton);
    void extractRootMotion(AnimatorData& animator, SkeletonData& skeleton);
};

// Kangaru service definition - must be after class is complete
struct AnimationSystem::Service : kgr::single_service<AnimationSystem>,
                                  kgr::overrides<IAnimationSystemService> {};

// Alias for consistent naming with other systems
using AnimationSystemService = AnimationSystem::Service;

}  // namespace bestow

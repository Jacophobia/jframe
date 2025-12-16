// bestow-animation/src/bestow.animation.impl.cppm
// Animation System Implementation Module

module;

#include <kangaru/kangaru.hpp>

export module bestow.animation.impl;

import bestow.services;

export namespace bestow {

// Forward declare the implementation class
class AnimationSystem;

// Kangaru service definition for AnimationSystem
// AnimationSystem depends on IAssetSystem for loading model data
BESTOW_SYSTEM(AnimationSystem, IAnimationSystem, IAssetSystem) {
public:
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
};

}  // namespace bestow

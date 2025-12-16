// bestow-contract/src/bestow.animation.cppm
// Skeletal Animation System Interface
//
// This module provides a renderer-agnostic animation system for skeletal animation,
// including sampling, blending, IK solvers, socket attachments, and physics integration.

module;

#include <functional>
#include <optional>
#include <vector>
#include <span>
#include <string>
#include <string_view>
#include <cstdint>
#include <set>

export module bestow.animation;

import bestow.types;
import bestow.assets;    // For ModelData
import bestow.physics3d; // For IPhysics3DSystem (optional dependency)

export namespace bestow {

//==========================================================================
// Handle Types
//==========================================================================

/// Opaque handle to a loaded skeleton
using SkeletonHandle = std::uint64_t;

/// Opaque handle to an animation clip
using AnimationClipHandle = std::uint64_t;

/// Opaque handle to an animator instance (per-entity animation state)
using AnimatorHandle = std::uint64_t;

/// Opaque handle to a socket definition
using SocketHandle = std::uint64_t;

namespace AnimationHandles {
    inline constexpr SkeletonHandle InvalidSkeleton = 0;
    inline constexpr AnimationClipHandle InvalidClip = 0;
    inline constexpr AnimatorHandle InvalidAnimator = 0;
    inline constexpr SocketHandle InvalidSocket = 0;
}

//==========================================================================
// Error Types
//==========================================================================

enum class AnimationError : std::uint8_t {
    Success = 0,

    // Skeleton errors
    InvalidSkeleton,
    SkeletonNotFound,
    InvalidBoneIndex,
    BoneNotFound,
    EmptyBoneData,

    // Clip errors
    InvalidClip,
    ClipNotFound,
    IncompatibleSkeleton,
    NoAnimationData,
    DuplicateClipName,

    // Animator errors
    InvalidAnimator,
    AnimatorNotFound,
    AnimatorSkeletonMismatch,

    // Socket errors
    InvalidSocket,
    SocketNotFound,
    SocketAlreadyExists,
    SocketBoneNotFound,

    // IK errors
    IKChainNotFound,
    IKChainInvalid,
    IKSolveFailed,

    // Operation errors
    SamplingFailed,
    BlendingFailed,
    InvalidTimeRange,
    InvalidWeight,

    // Physics integration errors
    PhysicsSystemRequired,
    RagdollCreationFailed,
    RagdollNotFound,
    RagdollAlreadyExists,
    RagdollInactive,

    // Asset errors
    AssetLoadFailed,
    InvalidModelData
};

/// Convert AnimationError to string for debugging
inline constexpr std::string_view animationErrorToString(AnimationError error) {
    switch (error) {
        case AnimationError::Success: return "Success";
        case AnimationError::InvalidSkeleton: return "InvalidSkeleton";
        case AnimationError::SkeletonNotFound: return "SkeletonNotFound";
        case AnimationError::InvalidBoneIndex: return "InvalidBoneIndex";
        case AnimationError::BoneNotFound: return "BoneNotFound";
        case AnimationError::EmptyBoneData: return "EmptyBoneData";
        case AnimationError::InvalidClip: return "InvalidClip";
        case AnimationError::ClipNotFound: return "ClipNotFound";
        case AnimationError::IncompatibleSkeleton: return "IncompatibleSkeleton";
        case AnimationError::NoAnimationData: return "NoAnimationData";
        case AnimationError::DuplicateClipName: return "DuplicateClipName";
        case AnimationError::InvalidAnimator: return "InvalidAnimator";
        case AnimationError::AnimatorNotFound: return "AnimatorNotFound";
        case AnimationError::AnimatorSkeletonMismatch: return "AnimatorSkeletonMismatch";
        case AnimationError::InvalidSocket: return "InvalidSocket";
        case AnimationError::SocketNotFound: return "SocketNotFound";
        case AnimationError::SocketAlreadyExists: return "SocketAlreadyExists";
        case AnimationError::SocketBoneNotFound: return "SocketBoneNotFound";
        case AnimationError::IKChainNotFound: return "IKChainNotFound";
        case AnimationError::IKChainInvalid: return "IKChainInvalid";
        case AnimationError::IKSolveFailed: return "IKSolveFailed";
        case AnimationError::SamplingFailed: return "SamplingFailed";
        case AnimationError::BlendingFailed: return "BlendingFailed";
        case AnimationError::InvalidTimeRange: return "InvalidTimeRange";
        case AnimationError::InvalidWeight: return "InvalidWeight";
        case AnimationError::PhysicsSystemRequired: return "PhysicsSystemRequired";
        case AnimationError::RagdollCreationFailed: return "RagdollCreationFailed";
        case AnimationError::RagdollNotFound: return "RagdollNotFound";
        case AnimationError::RagdollAlreadyExists: return "RagdollAlreadyExists";
        case AnimationError::RagdollInactive: return "RagdollInactive";
        case AnimationError::AssetLoadFailed: return "AssetLoadFailed";
        case AnimationError::InvalidModelData: return "InvalidModelData";
        default: return "Unknown";
    }
}

//==========================================================================
// Enumerations
//==========================================================================

/// How animation behaves when reaching its end
enum class AnimationWrapMode : std::uint8_t {
    Once,           // Play once, stop at last frame
    Loop,           // Restart from beginning
    PingPong,       // Reverse direction at ends
    ClampForever    // Hold last frame indefinitely
};

/// How animation layers combine with layers below
enum class AnimationBlendMode : std::uint8_t {
    Override,       // Replace lower layers completely
    Additive        // Add deltas to lower layers (for overlays)
};

/// Interpolation method for keyframes
enum class AnimationInterpolation : std::uint8_t {
    Step,           // No interpolation, snap to keyframes
    Linear,         // Linear interpolation (lerp/slerp)
    Cubic           // Cubic spline interpolation (smoother)
};

/// Socket attachment behavior
enum class SocketAttachMode : std::uint8_t {
    FollowBone,         // Socket follows bone transform exactly
    FollowPosition,     // Only follow bone position, keep world rotation
    FollowRotation,     // Only follow bone rotation, keep world position
    WorldSpace          // Fixed world transform (detached from bone)
};

//==========================================================================
// Skeleton Data Structures
//==========================================================================

/// Information about a single bone in a skeleton
struct BoneInfo {
    std::string name;
    std::int32_t index = -1;
    std::int32_t parentIndex = -1;      // -1 for root bones
    Mat4 inverseBindPose{1.0f};         // Transforms from model to bone space
    Mat4 localBindPose{1.0f};           // Default local transform relative to parent
    Vec3 localPosition{0.0f};           // Extracted position
    Quat localRotation{1.0f, 0.0f, 0.0f, 0.0f}; // Extracted rotation (w,x,y,z)
    Vec3 localScale{1.0f};              // Extracted scale

    bool isRoot() const { return parentIndex < 0; }
};

/// Complete skeleton information
struct SkeletonInfo {
    std::uint32_t boneCount = 0;
    std::vector<BoneInfo> bones;
    std::int32_t rootBoneIndex = 0;     // Index of the root bone
    AABB3D bounds;                      // Skeleton bounds in bind pose

    /// Find bone by name, returns -1 if not found
    std::int32_t findBone(std::string_view name) const {
        for (std::uint32_t i = 0; i < boneCount; ++i) {
            if (bones[i].name == name) return static_cast<std::int32_t>(i);
        }
        return -1;
    }

    /// Get all children of a bone
    std::vector<std::int32_t> getChildren(std::int32_t boneIndex) const {
        std::vector<std::int32_t> children;
        for (std::uint32_t i = 0; i < boneCount; ++i) {
            if (bones[i].parentIndex == boneIndex) {
                children.push_back(static_cast<std::int32_t>(i));
            }
        }
        return children;
    }
};

//==========================================================================
// Animation Clip Data Structures
//==========================================================================

/// Information about an animation clip
struct AnimationClipInfo {
    std::string name;
    SkeletonHandle skeleton = AnimationHandles::InvalidSkeleton;
    float duration = 0.0f;              // Total duration in seconds
    float ticksPerSecond = 30.0f;       // Sample rate
    AnimationWrapMode defaultWrapMode = AnimationWrapMode::Loop;
    std::uint32_t channelCount = 0;     // Number of animated bones
    std::uint32_t keyframeCount = 0;    // Total keyframes across all channels
    bool hasRootMotion = false;         // Whether clip contains root bone motion
    bool looping = true;                // Whether clip is designed to loop
};

/// Animation event definition (stored in clip)
struct AnimationEventDef {
    std::string name;                   // Event identifier (e.g., "footstep_left")
    float time = 0.0f;                  // Time in clip when event fires (seconds)
    std::string stringParam;            // Optional string data
    float floatParam = 0.0f;            // Optional float data
    std::int32_t intParam = 0;          // Optional int data
};

/// Animation event fired during playback
struct AnimationEvent {
    AnimatorHandle animator = AnimationHandles::InvalidAnimator;
    AnimationClipHandle clip = AnimationHandles::InvalidClip;
    std::uint32_t layer = 0;            // Layer that triggered event
    std::string name;                   // Event name
    float clipTime = 0.0f;              // Absolute time in clip (seconds)
    float normalizedTime = 0.0f;        // Progress through clip [0, 1]
    std::string stringParam;            // Optional string data
    float floatParam = 0.0f;            // Optional float data
    std::int32_t intParam = 0;          // Optional int data
};

//==========================================================================
// Socket Data Structures
//==========================================================================

/// Socket definition - an attachment point on a bone
struct SocketDef {
    std::string name;                   // Unique socket identifier
    std::string boneName;               // Parent bone name
    Vec3 localPosition{0.0f};           // Offset from bone origin
    Quat localRotation{1.0f, 0.0f, 0.0f, 0.0f}; // Rotation relative to bone
    Vec3 localScale{1.0f};              // Scale (usually 1,1,1)
    SocketAttachMode attachMode = SocketAttachMode::FollowBone;
};

/// Runtime socket state
struct SocketState {
    SocketHandle handle = AnimationHandles::InvalidSocket;
    std::string name;
    std::uint32_t boneIndex = 0;
    Mat4 localTransform{1.0f};          // Socket-to-bone transform
    Mat4 worldTransform{1.0f};          // Computed world transform (after update)
    SocketAttachMode attachMode = SocketAttachMode::FollowBone;
    bool enabled = true;
};

/// Socket query result with decomposed transform
struct SocketTransform {
    Mat4 worldMatrix{1.0f};             // Full 4x4 transform matrix
    Vec3 position{0.0f};                // World position
    Quat rotation{1.0f, 0.0f, 0.0f, 0.0f}; // World rotation
    Vec3 scale{1.0f};                   // World scale
    Vec3 forward{0.0f, 0.0f, -1.0f};    // Forward direction (-Z in socket space)
    Vec3 up{0.0f, 1.0f, 0.0f};          // Up direction (+Y in socket space)
    Vec3 right{1.0f, 0.0f, 0.0f};       // Right direction (+X in socket space)
};

/// Configuration for raycasting from a socket
struct SocketRaycastDef {
    std::string socketName;             // Socket to cast from
    Vec3 direction{0.0f, 0.0f, -1.0f};  // Direction in socket local space
    float maxDistance = 100.0f;         // Maximum ray length
    CollisionMask3D collisionMask = 0xFFFF; // Physics collision layers to test
    bool ignoreBackfaces = true;        // Ignore backfacing triangles
};

/// Result of a socket raycast
struct SocketRaycastResult {
    bool hit = false;
    Vec3 origin{0.0f};                  // Ray origin (socket world position)
    Vec3 direction{0.0f};               // Ray direction (world space)
    Vec3 hitPoint{0.0f};                // World position of hit
    Vec3 hitNormal{0.0f};               // Surface normal at hit
    float distance = 0.0f;              // Distance from origin to hit
    Entity hitEntity;                   // Entity that was hit
    std::uint32_t hitShapeIndex = 0;    // Shape index on hit entity
    std::string socketName;             // Socket used for cast
};

//==========================================================================
// Animator State Structures
//==========================================================================

/// State of a single animation layer
struct AnimationLayerState {
    AnimationClipHandle clip = AnimationHandles::InvalidClip;
    std::string clipName;               // Cached clip name
    float time = 0.0f;                  // Current playback time (seconds)
    float normalizedTime = 0.0f;        // Progress through clip [0, 1]
    float speed = 1.0f;                 // Playback speed multiplier
    float weight = 1.0f;                // Blend weight [0, 1]
    float fadeWeight = 1.0f;            // Crossfade weight (multiplied with weight)
    AnimationWrapMode wrapMode = AnimationWrapMode::Loop;
    AnimationBlendMode blendMode = AnimationBlendMode::Override;
    bool playing = false;
    bool paused = false;
    bool finished = false;              // True when non-looping clip reaches end

    /// Get effective weight (weight * fadeWeight)
    float effectiveWeight() const { return weight * fadeWeight; }

    /// Check if this layer is actively contributing
    bool isActive() const { return playing && effectiveWeight() > 0.001f; }
};

/// Configuration for playing an animation
struct AnimationPlayConfig {
    AnimationClipHandle clip = AnimationHandles::InvalidClip;
    std::string clipName;               // Alternative: specify by name
    float startTime = 0.0f;             // Start from this time in clip
    float speed = 1.0f;                 // Playback speed
    float weight = 1.0f;                // Layer weight
    float blendInTime = 0.25f;          // Crossfade duration from current animation
    float blendOutTime = 0.25f;         // Crossfade duration when transitioning away
    AnimationWrapMode wrapMode = AnimationWrapMode::Loop;
    std::uint32_t layer = 0;            // Which layer to play on
    AnimationBlendMode blendMode = AnimationBlendMode::Override;
    bool restartIfSame = false;         // Restart if same clip is already playing
};

/// Multi-layer blend configuration for stateless blending
struct AnimationBlendConfig {
    struct LayerConfig {
        AnimationClipHandle clip = AnimationHandles::InvalidClip;
        float time = 0.0f;
        float weight = 1.0f;
        AnimationWrapMode wrapMode = AnimationWrapMode::Loop;
        AnimationBlendMode blendMode = AnimationBlendMode::Override;
    };
    std::vector<LayerConfig> layers;
    float masterWeight = 1.0f;          // Global weight multiplier
};

//==========================================================================
// Inverse Kinematics Structures
//==========================================================================

/// Two-bone IK chain definition (for arms, legs)
struct IKTwoBoneChain {
    std::string name;                   // Chain identifier (e.g., "left_arm")
    std::string rootBoneName;           // First bone (e.g., "LeftUpperArm")
    std::string midBoneName;            // Middle bone (e.g., "LeftLowerArm")
    std::string tipBoneName;            // End bone (e.g., "LeftHand")

    // Cached bone indices (set by system)
    std::int32_t rootBoneIndex = -1;
    std::int32_t midBoneIndex = -1;
    std::int32_t tipBoneIndex = -1;
};

/// Two-bone IK target (runtime)
struct IKTwoBoneTarget {
    std::string chainName;              // Which chain to solve
    Vec3 targetPosition{0.0f};          // World-space target for tip bone
    Vec3 poleVector{0.0f, 0.0f, 1.0f};  // Controls mid-joint bend direction
    float weight = 1.0f;                // IK influence [0, 1]
    bool enabled = true;
};

/// Aim/look-at IK configuration
struct IKAimConfig {
    std::string name;                   // Config identifier (e.g., "head_look")
    std::string boneName;               // Bone to aim (e.g., "Head")
    Vec3 aimAxis{0.0f, 0.0f, 1.0f};     // Local axis that should point at target
    Vec3 upAxis{0.0f, 1.0f, 0.0f};      // Local up axis for orientation
    float horizontalLimit = 90.0f;      // Max horizontal rotation (degrees)
    float verticalLimit = 60.0f;        // Max vertical rotation (degrees)

    // Cached bone index (set by system)
    std::int32_t boneIndex = -1;
};

/// Aim IK target (runtime)
struct IKAimTarget {
    std::string configName;             // Which aim config to use
    Vec3 targetPosition{0.0f};          // World-space look target
    Vec3 worldUp{0.0f, 1.0f, 0.0f};     // World up direction
    float weight = 1.0f;                // IK influence [0, 1]
    bool enabled = true;
};

//==========================================================================
// Root Motion Structures
//==========================================================================

/// Root motion extracted from animation
struct RootMotion {
    Vec3 deltaPosition{0.0f};           // Position change this frame
    Quat deltaRotation{1.0f, 0.0f, 0.0f, 0.0f}; // Rotation change this frame
    Vec3 totalPosition{0.0f};           // Cumulative position from clip start
    Quat totalRotation{1.0f, 0.0f, 0.0f, 0.0f}; // Cumulative rotation from clip start
    bool hasTranslation = false;        // Whether clip has root translation
    bool hasRotation = false;           // Whether clip has root rotation
};

/// Root motion extraction configuration
struct RootMotionConfig {
    bool enabled = false;
    bool extractTranslationX = true;
    bool extractTranslationY = false;   // Usually false (vertical handled by physics)
    bool extractTranslationZ = true;
    bool extractRotationY = true;       // Usually only Y (turn) rotation
    bool extractRotationXZ = false;     // Pitch/roll usually not extracted
    std::string rootBoneName;           // Override root bone (empty = use skeleton root)
    std::int32_t rootBoneIndex = -1;    // Cached index
};

//==========================================================================
// Physics Integration: Ragdoll Structures
//==========================================================================

/// Per-bone physics body configuration for ragdoll
struct RagdollBoneDef {
    std::uint32_t boneIndex = 0;
    std::string boneName;               // For reference

    // Collision shape
    enum class ShapeType : std::uint8_t {
        Capsule,
        Box,
        Sphere
    } shape = ShapeType::Capsule;

    Vec3 shapeSize{0.05f, 0.2f, 0.05f}; // Capsule: radius, halfHeight, radius
                                         // Box: halfExtents
                                         // Sphere: radius, 0, 0
    Vec3 shapeOffset{0.0f};             // Offset from bone origin
    Quat shapeRotation{1.0f, 0.0f, 0.0f, 0.0f}; // Rotation relative to bone

    // Physics properties
    float mass = 5.0f;
    float friction = 0.5f;
    float restitution = 0.0f;
    float linearDamping = 0.1f;
    float angularDamping = 0.1f;

    // Constraint to parent bone
    bool hasConstraint = true;
    enum class ConstraintType : std::uint8_t {
        Cone,           // Ball-and-socket with cone limit
        Hinge,          // Single axis rotation
        Fixed,          // No relative motion
        BallSocket      // Free rotation (no limits)
    } constraintType = ConstraintType::Cone;

    Vec3 constraintAxis{0.0f, 1.0f, 0.0f};  // Hinge axis or cone axis
    Vec3 constraintPivot{0.0f};              // Pivot point in bone space
    float constraintSwingLimit = 0.5f;       // Swing angle limit (radians)
    float constraintTwistLimit = 0.3f;       // Twist limit (radians)
    float constraintHingeMin = -1.57f;       // Min hinge angle (radians)
    float constraintHingeMax = 1.57f;        // Max hinge angle (radians)
};

/// Complete ragdoll definition
struct RagdollDef {
    SkeletonHandle skeleton = AnimationHandles::InvalidSkeleton;
    std::vector<RagdollBoneDef> bones;
    bool selfCollision = false;         // Whether ragdoll bones collide with each other
    CollisionLayer3D collisionLayer = CollisionLayers3D::Dynamic;
    CollisionMask3D collisionMask = 0xFFFF;

    // Optional: bones to exclude from ragdoll (remain animated)
    std::set<std::string> excludedBones;
};

/// Ragdoll instance runtime state
struct RagdollState {
    bool created = false;               // Ragdoll bodies exist
    bool active = false;                // Physics drives bones (true) vs animation (false)
    float blendWeight = 0.0f;           // 0 = animation, 1 = physics
    float blendTarget = 0.0f;           // Target blend weight
    float blendDuration = 0.0f;         // Transition duration
    float blendTime = 0.0f;             // Current transition progress
    std::vector<Entity> boneEntities;   // Physics body entities per bone
    std::vector<std::uint32_t> boneIndices; // Which skeleton bones have bodies
};

//==========================================================================
// Callback Types
//==========================================================================

/// Called when animation event fires during playback
using AnimationEventCallback = std::function<void(const AnimationEvent& event)>;

/// Called when a non-looping animation completes
using AnimationCompleteCallback = std::function<void(
    AnimatorHandle animator,
    AnimationClipHandle clip,
    std::uint32_t layer)>;

/// Called when animation changes on a layer
using AnimationLayerCallback = std::function<void(
    AnimatorHandle animator,
    std::uint32_t layer,
    AnimationClipHandle previousClip,
    AnimationClipHandle newClip)>;

//==========================================================================
// Statistics
//==========================================================================

struct AnimationStats {
    // Resource counts
    std::uint32_t skeletonCount = 0;
    std::uint32_t clipCount = 0;
    std::uint32_t animatorCount = 0;
    std::uint32_t socketDefCount = 0;
    std::uint32_t ikChainCount = 0;
    std::uint32_t ragdollCount = 0;

    // Per-frame metrics
    std::uint32_t animatorsUpdated = 0;
    std::uint32_t layersProcessed = 0;
    std::uint32_t samplingJobs = 0;
    std::uint32_t blendingJobs = 0;
    std::uint32_t ikSolves = 0;
    std::uint32_t socketQueries = 0;
    std::uint32_t eventsDispatched = 0;
    std::uint32_t ragdollSyncs = 0;

    // Timing (milliseconds)
    float updateTimeMs = 0.0f;
    float samplingTimeMs = 0.0f;
    float blendingTimeMs = 0.0f;
    float ikTimeMs = 0.0f;
    float ragdollSyncTimeMs = 0.0f;

    // Memory estimates (bytes)
    std::size_t skeletonMemoryBytes = 0;
    std::size_t clipMemoryBytes = 0;
    std::size_t animatorMemoryBytes = 0;
    std::size_t totalMemoryBytes = 0;
};

//==========================================================================
// IAnimationSystem Interface
//==========================================================================

class IAnimationSystem {
public:
    virtual ~IAnimationSystem() = default;

    //======================================================================
    // Lifecycle
    //======================================================================

    /// Initialize the animation system
    /// @return true on success, false on failure
    virtual bool initialize() = 0;

    /// Shutdown and release all resources
    virtual void shutdown() = 0;

    /// Update all active animators (call once per frame)
    /// @param dt Delta time in seconds
    virtual void update(DeltaTime dt) = 0;

    //======================================================================
    // Skeleton Management
    //======================================================================

    /// Create skeleton from model data loaded via AssetSystem
    /// @param modelData Model containing bone hierarchy and animations
    /// @return Skeleton handle or error
    virtual Result<SkeletonHandle, AnimationError> createSkeleton(
        const ModelData& modelData) = 0;

    /// Create skeleton from explicit bone data
    /// @param bones Array of bone information
    /// @return Skeleton handle or error
    virtual Result<SkeletonHandle, AnimationError> createSkeleton(
        std::span<const BoneInfo> bones) = 0;

    /// Destroy skeleton and all associated clips, animators, sockets
    /// @param skeleton Handle to destroy
    virtual void destroySkeleton(SkeletonHandle skeleton) = 0;

    /// Check if skeleton handle is valid
    virtual bool isValidSkeleton(SkeletonHandle skeleton) const = 0;

    /// Get skeleton information
    virtual SkeletonInfo getSkeletonInfo(SkeletonHandle skeleton) const = 0;

    /// Find bone index by name
    /// @return Bone index or -1 if not found
    virtual std::int32_t findBoneIndex(
        SkeletonHandle skeleton,
        std::string_view boneName) const = 0;

    /// Get all bone names for a skeleton
    virtual std::vector<std::string> getBoneNames(
        SkeletonHandle skeleton) const = 0;

    /// Get bone count for a skeleton
    virtual std::uint32_t getBoneCount(SkeletonHandle skeleton) const = 0;

    //======================================================================
    // Animation Clip Management
    //======================================================================

    /// Create animation clip from model data
    /// @param skeleton Target skeleton (bone structure must match)
    /// @param modelData Model containing animation data
    /// @param clipName Name of animation to extract from model
    /// @return Clip handle or error
    virtual Result<AnimationClipHandle, AnimationError> createAnimationClip(
        SkeletonHandle skeleton,
        const ModelData& modelData,
        std::string_view clipName) = 0;

    /// Create all animation clips from model data
    /// @param skeleton Target skeleton
    /// @param modelData Model containing animations
    /// @return Vector of clip handles (empty on failure)
    virtual std::vector<AnimationClipHandle> createAnimationClips(
        SkeletonHandle skeleton,
        const ModelData& modelData) = 0;

    /// Destroy animation clip
    virtual void destroyAnimationClip(AnimationClipHandle clip) = 0;

    /// Check if clip handle is valid
    virtual bool isValidClip(AnimationClipHandle clip) const = 0;

    /// Get clip information
    virtual AnimationClipInfo getAnimationClipInfo(
        AnimationClipHandle clip) const = 0;

    /// Find clip by name for a skeleton
    /// @return Clip handle or InvalidClip if not found
    virtual AnimationClipHandle findClip(
        SkeletonHandle skeleton,
        std::string_view clipName) const = 0;

    /// Get all clips for a skeleton
    virtual std::vector<AnimationClipHandle> getClipsForSkeleton(
        SkeletonHandle skeleton) const = 0;

    /// Get all clip names for a skeleton
    virtual std::vector<std::string> getClipNames(
        SkeletonHandle skeleton) const = 0;

    //======================================================================
    // Animation Events (Clip-level)
    //======================================================================

    /// Add event to animation clip (will fire at specified time during playback)
    /// @param clip Target clip
    /// @param event Event definition
    /// @return Success or error
    virtual Result<void, AnimationError> addClipEvent(
        AnimationClipHandle clip,
        const AnimationEventDef& event) = 0;

    /// Remove event from clip by name
    virtual Result<void, AnimationError> removeClipEvent(
        AnimationClipHandle clip,
        std::string_view eventName) = 0;

    /// Remove all events from clip
    virtual void clearClipEvents(AnimationClipHandle clip) = 0;

    /// Get all events defined for a clip
    virtual std::vector<AnimationEventDef> getClipEvents(
        AnimationClipHandle clip) const = 0;

    //======================================================================
    // Animator Management
    //======================================================================

    /// Create animator instance (per-entity animation state)
    /// @param skeleton Skeleton to animate
    /// @return Animator handle or error
    virtual Result<AnimatorHandle, AnimationError> createAnimator(
        SkeletonHandle skeleton) = 0;

    /// Destroy animator instance
    virtual void destroyAnimator(AnimatorHandle animator) = 0;

    /// Check if animator handle is valid
    virtual bool isValidAnimator(AnimatorHandle animator) const = 0;

    /// Get skeleton associated with animator
    virtual SkeletonHandle getAnimatorSkeleton(
        AnimatorHandle animator) const = 0;

    //======================================================================
    // Animator Playback Control
    //======================================================================

    /// Play animation (simple API - uses layer 0, default transition)
    /// @param animator Target animator
    /// @param clip Animation to play
    /// @param transitionTime Crossfade duration in seconds (0 = instant)
    virtual void play(
        AnimatorHandle animator,
        AnimationClipHandle clip,
        float transitionTime = 0.25f) = 0;

    /// Play animation by name
    virtual void play(
        AnimatorHandle animator,
        std::string_view clipName,
        float transitionTime = 0.25f) = 0;

    /// Play animation with full configuration
    virtual void play(
        AnimatorHandle animator,
        const AnimationPlayConfig& config) = 0;

    /// Stop all animations on animator
    /// @param fadeOutTime Fade out duration (0 = instant stop)
    virtual void stop(AnimatorHandle animator, float fadeOutTime = 0.0f) = 0;

    /// Stop specific layer
    virtual void stopLayer(
        AnimatorHandle animator,
        std::uint32_t layer,
        float fadeOutTime = 0.0f) = 0;

    /// Pause/resume entire animator
    virtual void setPaused(AnimatorHandle animator, bool paused) = 0;
    virtual bool isPaused(AnimatorHandle animator) const = 0;

    /// Set global playback speed multiplier
    virtual void setSpeed(AnimatorHandle animator, float speed) = 0;
    virtual float getSpeed(AnimatorHandle animator) const = 0;

    /// Check if animator has any playing animations
    virtual bool isPlaying(AnimatorHandle animator) const = 0;

    /// Check if specific layer is playing
    virtual bool isLayerPlaying(
        AnimatorHandle animator,
        std::uint32_t layer) const = 0;

    //======================================================================
    // Animator Layer Control
    //======================================================================

    /// Get current state of a layer
    virtual AnimationLayerState getLayerState(
        AnimatorHandle animator,
        std::uint32_t layer) const = 0;

    /// Set layer weight
    virtual void setLayerWeight(
        AnimatorHandle animator,
        std::uint32_t layer,
        float weight) = 0;

    /// Get layer weight
    virtual float getLayerWeight(
        AnimatorHandle animator,
        std::uint32_t layer) const = 0;

    /// Set layer blend mode
    virtual void setLayerBlendMode(
        AnimatorHandle animator,
        std::uint32_t layer,
        AnimationBlendMode mode) = 0;

    /// Get number of active layers
    virtual std::uint32_t getLayerCount(AnimatorHandle animator) const = 0;

    /// Set mask for which bones are affected by a layer
    /// @param boneMask Set of bone indices affected (empty = all bones)
    virtual void setLayerBoneMask(
        AnimatorHandle animator,
        std::uint32_t layer,
        const std::set<std::uint32_t>& boneMask) = 0;

    //======================================================================
    // Animator Time Control
    //======================================================================

    /// Get current playback time on layer (seconds)
    virtual float getCurrentTime(
        AnimatorHandle animator,
        std::uint32_t layer = 0) const = 0;

    /// Get normalized time [0, 1] on layer
    virtual float getNormalizedTime(
        AnimatorHandle animator,
        std::uint32_t layer = 0) const = 0;

    /// Set playback time (seconds)
    virtual void setCurrentTime(
        AnimatorHandle animator,
        float time,
        std::uint32_t layer = 0) = 0;

    /// Set normalized time [0, 1]
    virtual void setNormalizedTime(
        AnimatorHandle animator,
        float normalizedTime,
        std::uint32_t layer = 0) = 0;

    /// Get clip duration on layer (seconds)
    virtual float getClipDuration(
        AnimatorHandle animator,
        std::uint32_t layer = 0) const = 0;

    //======================================================================
    // Animator Bone Transforms (Output for Rendering)
    //======================================================================

    /// Get final bone transforms for GPU skinning
    /// @return Span of model-space transforms (valid until next update)
    virtual std::span<const Mat4> getBoneTransforms(
        AnimatorHandle animator) const = 0;

    /// Get specific bone's transform by index
    virtual Mat4 getBoneTransform(
        AnimatorHandle animator,
        std::uint32_t boneIndex) const = 0;

    /// Get specific bone's transform by name
    virtual Mat4 getBoneTransform(
        AnimatorHandle animator,
        std::string_view boneName) const = 0;

    /// Get bone world transform (bone transform * entity world matrix)
    virtual Mat4 getBoneWorldTransform(
        AnimatorHandle animator,
        std::uint32_t boneIndex,
        const Mat4& entityWorldMatrix) const = 0;

    //======================================================================
    // Socket System
    //======================================================================

    /// Define a socket attachment point on skeleton
    /// @param skeleton Target skeleton
    /// @param def Socket definition
    /// @return Socket handle or error
    virtual Result<SocketHandle, AnimationError> defineSocket(
        SkeletonHandle skeleton,
        const SocketDef& def) = 0;

    /// Define multiple sockets at once
    virtual std::vector<Result<SocketHandle, AnimationError>> defineSockets(
        SkeletonHandle skeleton,
        std::span<const SocketDef> defs) = 0;

    /// Remove socket definition
    virtual void removeSocket(SocketHandle socket) = 0;

    /// Remove socket by name
    virtual void removeSocket(SkeletonHandle skeleton, std::string_view name) = 0;

    /// Check if socket exists on skeleton
    virtual bool hasSocket(
        SkeletonHandle skeleton,
        std::string_view socketName) const = 0;

    /// Find socket handle by name
    virtual SocketHandle findSocket(
        SkeletonHandle skeleton,
        std::string_view socketName) const = 0;

    /// Get all sockets for skeleton
    virtual std::vector<SocketState> getSockets(
        SkeletonHandle skeleton) const = 0;

    /// Get socket definition
    virtual std::optional<SocketDef> getSocketDef(SocketHandle socket) const = 0;

    /// Get socket world transform for an animator
    /// @param animator Active animator instance
    /// @param socketName Socket to query
    /// @param entityWorldMatrix Entity's world transform
    /// @return Socket transform with position, rotation, directions
    virtual Result<SocketTransform, AnimationError> getSocketTransform(
        AnimatorHandle animator,
        std::string_view socketName,
        const Mat4& entityWorldMatrix) const = 0;

    /// Get socket transform by handle
    virtual Result<SocketTransform, AnimationError> getSocketTransform(
        AnimatorHandle animator,
        SocketHandle socket,
        const Mat4& entityWorldMatrix) const = 0;

    /// Get multiple socket transforms efficiently
    virtual std::vector<SocketTransform> getSocketTransforms(
        AnimatorHandle animator,
        std::span<const std::string_view> socketNames,
        const Mat4& entityWorldMatrix) const = 0;

    /// Update socket's local transform
    virtual Result<void, AnimationError> setSocketLocalTransform(
        SocketHandle socket,
        const Vec3& position,
        const Quat& rotation,
        const Vec3& scale = Vec3{1.0f}) = 0;

    /// Enable/disable socket
    virtual void setSocketEnabled(SocketHandle socket, bool enabled) = 0;
    virtual bool isSocketEnabled(SocketHandle socket) const = 0;

    //======================================================================
    // Socket Raycasting (Requires Physics System)
    //======================================================================

    /// Perform raycast from socket position in socket's forward direction
    /// @param animator Source animator
    /// @param def Raycast configuration
    /// @param entityWorldMatrix Entity's world transform
    /// @param physics Physics system reference for raycasting
    /// @return Raycast result with hit information
    virtual SocketRaycastResult raycastFromSocket(
        AnimatorHandle animator,
        const SocketRaycastDef& def,
        const Mat4& entityWorldMatrix,
        const IPhysics3DSystem& physics) const = 0;

    /// Perform multiple raycasts from different sockets
    virtual std::vector<SocketRaycastResult> raycastFromSockets(
        AnimatorHandle animator,
        std::span<const SocketRaycastDef> defs,
        const Mat4& entityWorldMatrix,
        const IPhysics3DSystem& physics) const = 0;

    /// Perform sphere cast from socket
    virtual SocketRaycastResult sphereCastFromSocket(
        AnimatorHandle animator,
        const SocketRaycastDef& def,
        float sphereRadius,
        const Mat4& entityWorldMatrix,
        const IPhysics3DSystem& physics) const = 0;

    //======================================================================
    // Inverse Kinematics: Chain Definition
    //======================================================================

    /// Define two-bone IK chain on skeleton (arms, legs)
    /// @param skeleton Target skeleton
    /// @param chain Chain definition with bone names
    /// @return Success or error
    virtual Result<void, AnimationError> defineIKChain(
        SkeletonHandle skeleton,
        const IKTwoBoneChain& chain) = 0;

    /// Define aim IK configuration on skeleton (head, spine)
    virtual Result<void, AnimationError> defineIKAim(
        SkeletonHandle skeleton,
        const IKAimConfig& config) = 0;

    /// Remove IK chain by name
    virtual void removeIKChain(SkeletonHandle skeleton, std::string_view name) = 0;

    /// Remove aim IK config by name
    virtual void removeIKAim(SkeletonHandle skeleton, std::string_view name) = 0;

    /// Get all IK chain names for skeleton
    virtual std::vector<std::string> getIKChainNames(
        SkeletonHandle skeleton) const = 0;

    /// Get all aim IK config names for skeleton
    virtual std::vector<std::string> getIKAimNames(
        SkeletonHandle skeleton) const = 0;

    //======================================================================
    // Inverse Kinematics: Runtime Control
    //======================================================================

    /// Set two-bone IK target for animator
    virtual void setIKTarget(
        AnimatorHandle animator,
        const IKTwoBoneTarget& target) = 0;

    /// Set aim IK target for animator
    virtual void setIKTarget(
        AnimatorHandle animator,
        const IKAimTarget& target) = 0;

    /// Get current IK target (two-bone)
    virtual std::optional<IKTwoBoneTarget> getIKTwoBoneTarget(
        AnimatorHandle animator,
        std::string_view chainName) const = 0;

    /// Get current IK target (aim)
    virtual std::optional<IKAimTarget> getIKAimTarget(
        AnimatorHandle animator,
        std::string_view configName) const = 0;

    /// Clear specific IK target
    virtual void clearIKTarget(
        AnimatorHandle animator,
        std::string_view targetName) = 0;

    /// Clear all IK targets for animator
    virtual void clearAllIKTargets(AnimatorHandle animator) = 0;

    /// Set IK target weight (for smooth enable/disable)
    virtual void setIKWeight(
        AnimatorHandle animator,
        std::string_view targetName,
        float weight) = 0;

    /// Get current IK target weight
    virtual float getIKWeight(
        AnimatorHandle animator,
        std::string_view targetName) const = 0;

    //======================================================================
    // Root Motion
    //======================================================================

    /// Configure root motion extraction for animator
    virtual void setRootMotionConfig(
        AnimatorHandle animator,
        const RootMotionConfig& config) = 0;

    /// Get current root motion configuration
    virtual RootMotionConfig getRootMotionConfig(
        AnimatorHandle animator) const = 0;

    /// Enable/disable root motion extraction (shorthand)
    virtual void setRootMotionEnabled(AnimatorHandle animator, bool enabled) = 0;
    virtual bool isRootMotionEnabled(AnimatorHandle animator) const = 0;

    /// Get root motion delta from last update
    /// @return Delta position/rotation since last frame
    virtual RootMotion getRootMotion(AnimatorHandle animator) const = 0;

    /// Get root motion from clip between two times (stateless)
    virtual RootMotion extractRootMotion(
        AnimationClipHandle clip,
        float fromTime,
        float toTime) const = 0;

    /// Consume and reset root motion delta (call after applying)
    virtual void consumeRootMotion(AnimatorHandle animator) = 0;

    //======================================================================
    // Physics Integration: Ragdoll
    //======================================================================

    /// Create ragdoll physics bodies for skeleton
    /// @param entity Entity to attach ragdoll to
    /// @param def Ragdoll configuration
    /// @param physics Physics system for body creation
    /// @return Success or error
    virtual Result<void, AnimationError> createRagdoll(
        Entity entity,
        const RagdollDef& def,
        IPhysics3DSystem& physics) = 0;

    /// Destroy ragdoll physics bodies
    virtual void destroyRagdoll(
        Entity entity,
        IPhysics3DSystem& physics) = 0;

    /// Check if entity has a ragdoll
    virtual bool hasRagdoll(Entity entity) const = 0;

    /// Get ragdoll state
    virtual RagdollState getRagdollState(Entity entity) const = 0;

    /// Activate ragdoll (physics drives bones)
    /// @param instant If true, switch instantly; if false, blend smoothly
    /// @param blendDuration Duration of blend transition (if not instant)
    virtual void activateRagdoll(
        Entity entity,
        IPhysics3DSystem& physics,
        bool instant = false,
        float blendDuration = 0.3f) = 0;

    /// Deactivate ragdoll (animation drives bones)
    virtual void deactivateRagdoll(
        Entity entity,
        IPhysics3DSystem& physics,
        bool instant = false,
        float blendDuration = 0.3f) = 0;

    /// Set ragdoll blend weight manually [0=animation, 1=physics]
    virtual void setRagdollBlendWeight(Entity entity, float weight) = 0;

    /// Get ragdoll blend weight
    virtual float getRagdollBlendWeight(Entity entity) const = 0;

    /// Check if ragdoll is active (physics driving)
    virtual bool isRagdollActive(Entity entity) const = 0;

    /// Get bone transforms from ragdoll physics bodies
    virtual std::vector<Mat4> getRagdollBoneTransforms(
        Entity entity,
        const IPhysics3DSystem& physics) const = 0;

    //======================================================================
    // Physics Integration: Bone Sync & Impulses
    //======================================================================

    /// Sync animator bone transforms to kinematic physics bodies
    /// Call after update() to push animation poses to physics for collision
    virtual void syncToPhysics(
        AnimatorHandle animator,
        Entity entity,
        IPhysics3DSystem& physics) = 0;

    /// Apply impulse to specific ragdoll bone by index
    virtual void applyBoneImpulse(
        Entity entity,
        std::uint32_t boneIndex,
        const Vec3& impulse,
        IPhysics3DSystem& physics) = 0;

    /// Apply impulse to ragdoll bone by name
    virtual void applyBoneImpulse(
        Entity entity,
        std::string_view boneName,
        const Vec3& impulse,
        IPhysics3DSystem& physics) = 0;

    /// Apply impulse at world position (affects nearest bone)
    virtual void applyImpulseAtPosition(
        Entity entity,
        const Vec3& worldPosition,
        const Vec3& impulse,
        float radius,
        IPhysics3DSystem& physics) = 0;

    //======================================================================
    // Event Subscriptions
    //======================================================================

    /// Subscribe to animation events for an animator
    virtual SubscriptionId subscribeToEvents(
        AnimatorHandle animator,
        AnimationEventCallback callback) = 0;

    /// Subscribe to animation completion (non-looping clips only)
    virtual SubscriptionId subscribeToComplete(
        AnimatorHandle animator,
        AnimationCompleteCallback callback) = 0;

    /// Subscribe to layer animation changes
    virtual SubscriptionId subscribeToLayerChanges(
        AnimatorHandle animator,
        AnimationLayerCallback callback) = 0;

    /// Unsubscribe from events
    virtual void unsubscribe(SubscriptionId id) = 0;

    //======================================================================
    // Stateless Animation Sampling (Low-level API)
    //======================================================================

    /// Sample animation at specific time without animator state
    /// @return Bone transforms in model space
    virtual Result<std::vector<Mat4>, AnimationError> sampleAnimation(
        AnimationClipHandle clip,
        float time,
        AnimationWrapMode wrapMode = AnimationWrapMode::Loop) = 0;

    /// Blend multiple animations without animator state
    virtual Result<std::vector<Mat4>, AnimationError> blendAnimations(
        SkeletonHandle skeleton,
        const AnimationBlendConfig& config) = 0;

    //======================================================================
    // Statistics & Debugging
    //======================================================================

    /// Get animation system statistics
    virtual AnimationStats getStats() const = 0;

    /// Reset per-frame statistics counters
    virtual void resetFrameStats() = 0;

    /// Enable/disable debug visualization (if supported)
    virtual void setDebugVisualization(bool enabled) = 0;
    virtual bool isDebugVisualizationEnabled() const = 0;
};

}  // namespace bestow

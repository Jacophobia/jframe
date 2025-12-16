# Animation System Proposal

**Document Version:** 1.0
**Status:** Draft
**Author:** Claude
**Module:** `bestow.animation`

---

## Table of Contents

1. [Executive Summary](#executive-summary)
2. [Motivation](#motivation)
3. [Current State Analysis](#current-state-analysis)
4. [Proposed Architecture](#proposed-architecture)
5. [Contract Definition](#contract-definition)
6. [Socket System](#socket-system)
7. [Physics Integration](#physics-integration)
8. [Library Recommendations](#library-recommendations)
9. [Migration Path](#migration-path)
10. [File Structure](#file-structure)
11. [Example Usage](#example-usage)

---

## Executive Summary

This proposal introduces a dedicated `IAnimationSystem` to handle skeletal animation, decoupled from the graphics pipeline. The system provides:

- **Skeletal animation sampling and blending** - Runtime playback with multi-layer support
- **Inverse Kinematics (IK)** - Two-bone and aim solvers
- **Socket system** - Named attachment points on bones for weapons, effects, and raycasting
- **Physics integration** - Ragdoll support, root motion, bone-to-physics synchronization
- **Renderer-agnostic design** - Outputs `Mat4[]` transforms consumable by any graphics backend

The animation system follows Bestow's contract-based architecture, enabling complete implementation interchangeability while maintaining clean separation from rendering and physics concerns.

---

## Motivation

### Why Separate Animation from Graphics?

The current `IGraphics3DSystem` contains skeletal animation methods (`createSkeleton`, `sampleAnimation`, `blendAnimations`). This coupling creates several problems:

1. **Renderer lock-in** - Animation logic is duplicated across Vulkan and OpenGL backends
2. **Non-rendering use cases blocked** - Animation data needed for gameplay (hit detection, footsteps, IK targets) requires graphics system access
3. **Testing difficulty** - Cannot unit test animation logic without graphics context
4. **Violation of Single Responsibility** - Graphics3D already handles meshes, materials, lights, shadows, post-processing, debug rendering

### Key Use Cases Enabled by Separation

| Use Case | Current Limitation | With IAnimationSystem |
|----------|-------------------|----------------------|
| **Melee hit detection** | Must raycast in graphics system | Socket positions + physics raycasts |
| **Footstep audio** | Manual frame counting | Animation events at exact times |
| **Ragdoll death** | No transition support | Smooth blend to physics |
| **Root motion locomotion** | Not implemented | Extract motion, drive character |
| **Weapon attachment** | Manual offset math | Socket system with transforms |
| **Server-side animation** | Requires graphics | Headless animation sampling |

---

## Current State Analysis

### Existing Animation Infrastructure

#### Data Structures (in `bestow.assets.cppm`)

```cpp
// Already defined - no changes needed
struct Vertex3DData {
    float boneIndices[4];    // ✅ Exists
    float boneWeights[4];    // ✅ Exists
};

struct ModelData {
    struct Bone { ... };              // ✅ Exists
    struct AnimationKeyframe { ... }; // ✅ Exists
    struct AnimationChannel { ... };  // ✅ Exists
    struct Animation { ... };         // ✅ Exists
    std::vector<Bone> bones;          // ✅ Exists
    std::vector<Animation> animations;// ✅ Exists
};
```

#### Graphics3D Animation API (to be deprecated)

```cpp
// In bestow.graphics3d.cppm - WILL BE REMOVED
virtual Result<SkeletonHandle> createSkeleton(const ModelData&) = 0;
virtual Result<AnimationClipHandle> createAnimationClip(...) = 0;
virtual void destroySkeleton(SkeletonHandle) = 0;
virtual void destroyAnimationClip(AnimationClipHandle) = 0;
virtual std::vector<Mat4> sampleAnimation(...) = 0;
virtual std::vector<Mat4> blendAnimations(...) = 0;

// WILL BE KEPT - only needs bone transforms
virtual void drawSkinnedMesh(MeshHandle, MaterialHandle, Mat4, std::span<const Mat4>) = 0;
```

#### Implementation Status

| Backend | Skeleton | Sampling | Blending | IK | Ragdoll |
|---------|----------|----------|----------|----|---------|
| OpenGL | ✅ Full | ✅ Full | ✅ Full | ❌ | ❌ |
| Vulkan | ⚠️ Stub | ⚠️ Stub | ⚠️ Stub | ❌ | ❌ |

---

## Proposed Architecture

### System Dependency Graph

```
                         IAssetSystem
                              │
                              │ loads ModelData
                              ▼
┌─────────────────────────────────────────────────────────────┐
│                      IAnimationSystem                        │
│                                                              │
│  ┌─────────────┐  ┌─────────────┐  ┌─────────────────────┐  │
│  │  Skeletons  │  │    Clips    │  │     Animators       │  │
│  │             │  │             │  │                     │  │
│  │ • Bones     │  │ • Keyframes │  │ • Playback state    │  │
│  │ • Hierarchy │  │ • Duration  │  │ • Layer blending    │  │
│  │ • Sockets   │  │ • Events    │  │ • IK solvers        │  │
│  └─────────────┘  └─────────────┘  └─────────────────────┘  │
│                                                              │
│  ┌─────────────────────────────────────────────────────────┐ │
│  │                   Physics Integration                    │ │
│  │  • Ragdoll creation    • Root motion extraction         │ │
│  │  • Bone sync           • Impulse application            │ │
│  └─────────────────────────────────────────────────────────┘ │
└──────────────────────────┬──────────────────────────────────┘
                           │
           ┌───────────────┼───────────────┐
           │               │               │
           ▼               ▼               ▼
   IGraphics3DSystem  IPhysics3DSystem  IEntitySystem
   (drawSkinnedMesh)  (ragdoll bodies)  (components)
```

### Data Flow

```
Frame Update:
1. IAnimationSystem::update(dt)
   ├─ Advance all animator playback times
   ├─ Sample keyframes for active clips
   ├─ Blend layers together
   ├─ Apply IK solvers
   ├─ Fire animation events
   └─ Cache final bone transforms

2. Game code reads transforms:
   └─ anim->getBoneTransforms(animator) → Mat4[]

3. Game code queries sockets:
   └─ anim->getSocketTransform(animator, "weapon_r") → Mat4

4. Graphics renders:
   └─ gfx->drawSkinnedMesh(mesh, material, world, boneTransforms)

5. Physics sync (optional):
   └─ anim->syncToPhysics(animator, entity, physics)
```

---

## Contract Definition

### Module: `bestow.animation.cppm`

```cpp
module;

export module bestow.animation;

import bestow.types;
import std;

export namespace bestow {

//==========================================================================
// Forward Declarations
//==========================================================================

class IPhysics3DSystem;  // Optional dependency for physics integration

//==========================================================================
// Handle Types
//==========================================================================

/// Opaque handle to a loaded skeleton
using SkeletonHandle = std::uint64_t;

/// Opaque handle to an animation clip
using AnimationClipHandle = std::uint64_t;

/// Opaque handle to an animator instance
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

    // Clip errors
    InvalidClip,
    ClipNotFound,
    IncompatibleSkeleton,
    NoAnimationData,

    // Animator errors
    InvalidAnimator,
    AnimatorNotFound,

    // Socket errors
    InvalidSocket,
    SocketNotFound,
    SocketAlreadyExists,

    // Operation errors
    SamplingFailed,
    BlendingFailed,
    IKSolveFailed,

    // Physics integration errors
    PhysicsSystemRequired,
    RagdollCreationFailed,
    RagdollNotFound,

    // Asset errors
    AssetLoadFailed,
    InvalidModelData
};

/// Detailed error information
struct AnimationErrorInfo {
    AnimationError error = AnimationError::Success;
    std::string message;
    std::string context;  // e.g., bone name, clip name
};

//==========================================================================
// Enumerations
//==========================================================================

/// How animation behaves at boundaries
enum class AnimationWrapMode : std::uint8_t {
    Once,           // Play once, stop at last frame
    Loop,           // Restart from beginning
    PingPong,       // Reverse direction at ends
    ClampForever    // Hold last frame indefinitely
};

/// How animation layers combine
enum class AnimationBlendMode : std::uint8_t {
    Override,       // Replace lower layers completely
    Additive        // Add to lower layers (for overlays)
};

/// Interpolation method for keyframes
enum class AnimationInterpolation : std::uint8_t {
    Step,           // No interpolation, snap to keyframes
    Linear,         // Linear interpolation
    Cubic           // Cubic spline interpolation
};

/// Socket attachment mode
enum class SocketAttachMode : std::uint8_t {
    FollowBone,         // Socket follows bone transform exactly
    FollowPosition,     // Only position, world rotation
    FollowRotation,     // Only rotation, world position
    WorldSpace          // Fixed world transform (for detached sockets)
};

//==========================================================================
// Skeleton Data Structures
//==========================================================================

/// Information about a single bone
struct BoneInfo {
    std::string name;
    std::int32_t parentIndex = -1;  // -1 for root bones
    Mat4 inverseBindPose;           // Transforms from model to bone space
    Mat4 localBindPose;             // Default local transform
};

/// Complete skeleton information
struct SkeletonInfo {
    std::uint32_t boneCount = 0;
    std::vector<BoneInfo> bones;
    std::vector<std::string> boneNames;      // Quick lookup
    std::vector<std::int32_t> parentIndices; // Quick lookup
    AABB3D bounds;                           // Skeleton bounds in bind pose
};

//==========================================================================
// Animation Clip Data Structures
//==========================================================================

/// Information about an animation clip
struct AnimationClipInfo {
    std::string name;
    float duration = 0.0f;              // Total duration in seconds
    float ticksPerSecond = 30.0f;       // Sample rate
    AnimationWrapMode defaultWrapMode = AnimationWrapMode::Loop;
    std::uint32_t channelCount = 0;     // Number of animated bones
    std::uint32_t keyframeCount = 0;    // Total keyframes across all channels
    bool hasRootMotion = false;         // Whether clip contains root motion
    std::vector<std::string> eventNames;// Animation events in this clip
};

/// Animation event definition
struct AnimationEventDef {
    std::string name;           // Event identifier (e.g., "footstep_left")
    float time = 0.0f;          // Time in clip when event fires
    std::string stringParam;    // Optional string data
    float floatParam = 0.0f;    // Optional float data
    std::int32_t intParam = 0;  // Optional int data
};

/// Animation event fired during playback
struct AnimationEvent {
    AnimatorHandle animator;
    AnimationClipHandle clip;
    std::string name;
    float clipTime;             // Time in clip
    float normalizedTime;       // 0-1 progress through clip
    std::string stringParam;
    float floatParam;
    std::int32_t intParam;
};

//==========================================================================
// Socket Data Structures
//==========================================================================

/// Socket definition - an attachment point on a bone
struct SocketDef {
    std::string name;                   // Unique socket identifier
    std::string boneName;               // Parent bone name
    Vec3 localPosition{0.0f};           // Offset from bone origin
    Quat localRotation{0, 0, 0, 1};     // Rotation relative to bone
    Vec3 localScale{1.0f};              // Scale (usually 1,1,1)
    SocketAttachMode attachMode = SocketAttachMode::FollowBone;
};

/// Runtime socket state
struct SocketState {
    SocketHandle handle;
    std::string name;
    std::uint32_t boneIndex;
    Mat4 localTransform;        // Socket-to-bone transform
    Mat4 worldTransform;        // Computed world transform (after update)
    SocketAttachMode attachMode;
    bool enabled = true;
};

/// Socket query result
struct SocketTransform {
    Mat4 worldMatrix;           // Full 4x4 transform matrix
    Vec3 position;              // World position
    Quat rotation;              // World rotation
    Vec3 forward;               // Forward direction (-Z in socket space)
    Vec3 up;                    // Up direction (+Y in socket space)
    Vec3 right;                 // Right direction (+X in socket space)
};

/// Raycast from socket configuration
struct SocketRaycastDef {
    std::string socketName;             // Socket to cast from
    Vec3 direction{0, 0, -1};           // Direction in socket local space
    float maxDistance = 100.0f;         // Maximum ray length
    std::uint32_t collisionMask = ~0u;  // Physics collision layers
    bool ignoreOwner = true;            // Ignore entity that owns animator
};

/// Result of socket raycast
struct SocketRaycastResult {
    bool hit = false;
    Vec3 hitPoint{0.0f};
    Vec3 hitNormal{0.0f};
    float distance = 0.0f;
    Entity hitEntity;
    std::uint32_t hitShapeIndex = 0;
    std::string socketName;
};

//==========================================================================
// Animator State Structures
//==========================================================================

/// State of a single animation layer
struct AnimationLayerState {
    AnimationClipHandle clip = AnimationHandles::InvalidClip;
    float time = 0.0f;                  // Current playback time (seconds)
    float normalizedTime = 0.0f;        // 0-1 progress through clip
    float speed = 1.0f;                 // Playback speed multiplier
    float weight = 1.0f;                // Blend weight [0, 1]
    AnimationWrapMode wrapMode = AnimationWrapMode::Loop;
    AnimationBlendMode blendMode = AnimationBlendMode::Override;
    bool playing = false;
    bool finished = false;              // True when non-looping clip ends
};

/// Configuration for playing an animation
struct AnimationPlayConfig {
    AnimationClipHandle clip = AnimationHandles::InvalidClip;
    float startTime = 0.0f;             // Start from this time in clip
    float speed = 1.0f;
    float blendInTime = 0.25f;          // Crossfade from current animation
    float blendOutTime = 0.25f;         // Crossfade when transitioning away
    AnimationWrapMode wrapMode = AnimationWrapMode::Loop;
    std::uint32_t layer = 0;            // Which layer to play on
    AnimationBlendMode blendMode = AnimationBlendMode::Override;
};

/// Multi-layer blend configuration
struct AnimationBlendConfig {
    std::vector<AnimationLayerState> layers;
    float masterSpeed = 1.0f;           // Global speed multiplier
};

//==========================================================================
// Inverse Kinematics Structures
//==========================================================================

/// Two-bone IK chain (arms, legs)
struct IKTwoBoneChain {
    std::string name;                   // Chain identifier
    std::string rootBoneName;           // e.g., "LeftUpperArm"
    std::string midBoneName;            // e.g., "LeftLowerArm"
    std::string tipBoneName;            // e.g., "LeftHand"
};

/// Two-bone IK target
struct IKTwoBoneTarget {
    std::string chainName;              // Which chain to solve
    Vec3 targetPosition;                // World-space target for tip
    Vec3 poleVector{0, 0, 1};           // Controls mid-joint bend direction
    float weight = 1.0f;                // IK influence [0, 1]
    bool enabled = true;
};

/// Aim/look-at IK configuration
struct IKAimConfig {
    std::string name;                   // Config identifier
    std::string boneName;               // Bone to aim (e.g., "Head")
    Vec3 aimAxis{0, 0, 1};              // Local axis that points at target
    Vec3 upAxis{0, 1, 0};               // Local up axis
    Vec2 angleLimits{90.0f, 90.0f};     // Horizontal/vertical limits (degrees)
};

/// Aim IK target
struct IKAimTarget {
    std::string configName;             // Which aim config to use
    Vec3 targetPosition;                // World-space look target
    Vec3 worldUp{0, 1, 0};              // World up direction
    float weight = 1.0f;                // IK influence [0, 1]
    bool enabled = true;
};

//==========================================================================
// Root Motion Structures
//==========================================================================

/// Root motion extracted from animation
struct RootMotion {
    Vec3 deltaPosition{0.0f};           // Position change this frame
    Quat deltaRotation{0, 0, 0, 1};     // Rotation change this frame
    Vec3 totalPosition{0.0f};           // Total position from clip start
    Quat totalRotation{0, 0, 0, 1};     // Total rotation from clip start
    bool hasTranslation = false;
    bool hasRotation = false;
};

/// Root motion configuration
struct RootMotionConfig {
    bool extractTranslationX = true;
    bool extractTranslationY = false;   // Usually false (vertical handled by physics)
    bool extractTranslationZ = true;
    bool extractRotation = true;
    std::string rootBoneName;           // Override root bone (empty = use skeleton root)
};

//==========================================================================
// Physics Integration Structures
//==========================================================================

/// Per-bone physics body configuration
struct RagdollBoneDef {
    std::uint32_t boneIndex = 0;
    std::string boneName;               // For reference

    // Collision shape
    enum class ShapeType : std::uint8_t { Capsule, Box, Sphere } shape = ShapeType::Capsule;
    Vec3 shapeSize{0.05f, 0.2f, 0.05f}; // Capsule: radius, halfHeight, radius
    Vec3 shapeOffset{0.0f};             // Offset from bone origin
    Quat shapeRotation{0, 0, 0, 1};     // Rotation from bone space

    // Physics properties
    float mass = 5.0f;
    float friction = 0.5f;
    float restitution = 0.0f;

    // Constraint to parent
    bool hasConstraint = true;
    enum class ConstraintType : std::uint8_t {
        Cone, Hinge, Fixed, BallSocket
    } constraintType = ConstraintType::Cone;
    Vec3 constraintAxis{0, 1, 0};       // Hinge axis or cone axis
    float constraintLimit = 0.5f;       // Radians for angular limits
    float constraintTwist = 0.1f;       // Twist limit for cone constraints
};

/// Complete ragdoll definition
struct RagdollDef {
    SkeletonHandle skeleton = AnimationHandles::InvalidSkeleton;
    std::vector<RagdollBoneDef> bones;
    bool selfCollision = false;         // Whether ragdoll bones collide with each other
    std::uint32_t collisionLayer = 0;   // Physics collision layer
    std::uint32_t collisionMask = ~0u;  // What layers to collide with

    /// Create default ragdoll from skeleton (bipedal humanoid assumed)
    static RagdollDef createBipedDefault(SkeletonHandle skeleton);

    /// Create from bone name patterns
    static RagdollDef createFromPattern(
        SkeletonHandle skeleton,
        const std::vector<std::string>& bonePatterns,
        RagdollBoneDef defaultConfig);
};

/// Ragdoll instance state
struct RagdollState {
    bool active = false;                // True = physics drives bones
    float blendWeight = 0.0f;           // 0 = animation, 1 = physics
    float blendDuration = 0.0f;         // Transition duration
    float blendTime = 0.0f;             // Current transition progress
    std::vector<Entity> boneEntities;   // Physics body entities per bone
};

//==========================================================================
// Callback Types
//==========================================================================

/// Called when animation event fires
using AnimationEventCallback = std::function<void(const AnimationEvent& event)>;

/// Called when animation completes (non-looping only)
using AnimationCompleteCallback = std::function<void(
    AnimatorHandle animator,
    AnimationClipHandle clip,
    std::uint32_t layer)>;

/// Called when animation layer changes
using AnimationLayerCallback = std::function<void(
    AnimatorHandle animator,
    std::uint32_t layer,
    AnimationClipHandle oldClip,
    AnimationClipHandle newClip)>;

//==========================================================================
// Statistics
//==========================================================================

struct AnimationStats {
    // Resource counts
    std::uint32_t skeletonCount = 0;
    std::uint32_t clipCount = 0;
    std::uint32_t animatorCount = 0;
    std::uint32_t socketCount = 0;
    std::uint32_t ragdollCount = 0;

    // Per-frame metrics
    std::uint32_t animatorsUpdated = 0;
    std::uint32_t samplingJobs = 0;
    std::uint32_t blendingJobs = 0;
    std::uint32_t ikSolves = 0;
    std::uint32_t socketUpdates = 0;
    std::uint32_t eventsDispatched = 0;

    // Timing
    float updateTimeMs = 0.0f;
    float samplingTimeMs = 0.0f;
    float blendingTimeMs = 0.0f;
    float ikTimeMs = 0.0f;

    // Memory
    std::size_t skeletonMemoryBytes = 0;
    std::size_t clipMemoryBytes = 0;
    std::size_t animatorMemoryBytes = 0;
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

    /// Update all active animators
    /// @param dt Delta time in seconds
    virtual void update(DeltaTime dt) = 0;

    //======================================================================
    // Skeleton Management
    //======================================================================

    /// Create skeleton from model data loaded via AssetSystem
    /// @param modelData Model containing bone hierarchy
    /// @return Skeleton handle or error
    virtual Result<SkeletonHandle, AnimationError> createSkeleton(
        const ModelData& modelData) = 0;

    /// Create skeleton from explicit bone data
    /// @param bones Bone information array
    /// @return Skeleton handle or error
    virtual Result<SkeletonHandle, AnimationError> createSkeleton(
        std::span<const BoneInfo> bones) = 0;

    /// Destroy skeleton and all associated clips/animators
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

    /// Get all bone names
    virtual std::vector<std::string> getBoneNames(
        SkeletonHandle skeleton) const = 0;

    //======================================================================
    // Animation Clip Management
    //======================================================================

    /// Create animation clip from model data
    /// @param skeleton Target skeleton (must match bone structure)
    /// @param modelData Model containing animation data
    /// @param clipName Name of animation in model
    /// @return Clip handle or error
    virtual Result<AnimationClipHandle, AnimationError> createAnimationClip(
        SkeletonHandle skeleton,
        const ModelData& modelData,
        std::string_view clipName) = 0;

    /// Create all animation clips from model data
    /// @param skeleton Target skeleton
    /// @param modelData Model containing animations
    /// @return Vector of clip handles (empty clips are skipped)
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

    /// Get clip names for a skeleton
    virtual std::vector<std::string> getClipNames(
        SkeletonHandle skeleton) const = 0;

    //======================================================================
    // Animation Events
    //======================================================================

    /// Add event to animation clip
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

    /// Get all events for a clip
    virtual std::vector<AnimationEventDef> getClipEvents(
        AnimationClipHandle clip) const = 0;

    //======================================================================
    // Animator Management
    //======================================================================

    /// Create animator instance for an entity
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

    /// Play animation on animator (simple API)
    /// @param animator Target animator
    /// @param clip Animation to play
    /// @param transitionTime Crossfade duration (0 = instant)
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
    /// @param animator Target animator
    /// @param fadeOut Fade out duration (0 = instant stop)
    virtual void stop(AnimatorHandle animator, float fadeOut = 0.0f) = 0;

    /// Stop specific layer
    virtual void stopLayer(
        AnimatorHandle animator,
        std::uint32_t layer,
        float fadeOut = 0.0f) = 0;

    /// Pause/resume animator
    virtual void setPaused(AnimatorHandle animator, bool paused) = 0;
    virtual bool isPaused(AnimatorHandle animator) const = 0;

    /// Set global playback speed
    virtual void setSpeed(AnimatorHandle animator, float speed) = 0;
    virtual float getSpeed(AnimatorHandle animator) const = 0;

    /// Check if animator is playing any animation
    virtual bool isPlaying(AnimatorHandle animator) const = 0;

    /// Check if specific layer is playing
    virtual bool isLayerPlaying(
        AnimatorHandle animator,
        std::uint32_t layer) const = 0;

    //======================================================================
    // Animator Layer Control
    //======================================================================

    /// Get current layer state
    virtual AnimationLayerState getLayerState(
        AnimatorHandle animator,
        std::uint32_t layer) const = 0;

    /// Set layer weight
    virtual void setLayerWeight(
        AnimatorHandle animator,
        std::uint32_t layer,
        float weight) = 0;

    /// Set layer blend mode
    virtual void setLayerBlendMode(
        AnimatorHandle animator,
        std::uint32_t layer,
        AnimationBlendMode mode) = 0;

    /// Get number of active layers
    virtual std::uint32_t getLayerCount(AnimatorHandle animator) const = 0;

    //======================================================================
    // Animator Time Control
    //======================================================================

    /// Get current playback time on layer
    virtual float getCurrentTime(
        AnimatorHandle animator,
        std::uint32_t layer = 0) const = 0;

    /// Get normalized time (0-1) on layer
    virtual float getNormalizedTime(
        AnimatorHandle animator,
        std::uint32_t layer = 0) const = 0;

    /// Set playback time
    virtual void setCurrentTime(
        AnimatorHandle animator,
        float time,
        std::uint32_t layer = 0) = 0;

    /// Set normalized time (0-1)
    virtual void setNormalizedTime(
        AnimatorHandle animator,
        float normalizedTime,
        std::uint32_t layer = 0) = 0;

    //======================================================================
    // Animator Bone Transforms (Output)
    //======================================================================

    /// Get final bone transforms for rendering
    /// @return Span of model-space transforms (valid until next update)
    virtual std::span<const Mat4> getBoneTransforms(
        AnimatorHandle animator) const = 0;

    /// Get specific bone transform
    virtual Mat4 getBoneTransform(
        AnimatorHandle animator,
        std::uint32_t boneIndex) const = 0;

    /// Get bone transform by name
    virtual Mat4 getBoneTransform(
        AnimatorHandle animator,
        std::string_view boneName) const = 0;

    /// Get bone world transform (with entity transform applied)
    virtual Mat4 getBoneWorldTransform(
        AnimatorHandle animator,
        std::uint32_t boneIndex,
        const Mat4& entityWorldMatrix) const = 0;

    //======================================================================
    // Socket System
    //======================================================================

    /// Define a socket on a skeleton
    /// @param skeleton Target skeleton
    /// @param def Socket definition
    /// @return Socket handle or error
    virtual Result<SocketHandle, AnimationError> defineSocket(
        SkeletonHandle skeleton,
        const SocketDef& def) = 0;

    /// Define multiple sockets at once
    virtual Result<std::vector<SocketHandle>, AnimationError> defineSockets(
        SkeletonHandle skeleton,
        std::span<const SocketDef> defs) = 0;

    /// Remove socket definition
    virtual void removeSocket(SocketHandle socket) = 0;

    /// Check if socket exists
    virtual bool hasSocket(
        SkeletonHandle skeleton,
        std::string_view socketName) const = 0;

    /// Find socket by name
    virtual SocketHandle findSocket(
        SkeletonHandle skeleton,
        std::string_view socketName) const = 0;

    /// Get all sockets for skeleton
    virtual std::vector<SocketState> getSockets(
        SkeletonHandle skeleton) const = 0;

    /// Get socket transform for an animator
    /// @param animator Active animator
    /// @param socketName Socket to query
    /// @param entityWorldMatrix Entity's world transform
    /// @return Socket transform or error
    virtual Result<SocketTransform, AnimationError> getSocketTransform(
        AnimatorHandle animator,
        std::string_view socketName,
        const Mat4& entityWorldMatrix) const = 0;

    /// Get socket transform by handle
    virtual Result<SocketTransform, AnimationError> getSocketTransform(
        AnimatorHandle animator,
        SocketHandle socket,
        const Mat4& entityWorldMatrix) const = 0;

    /// Get multiple socket transforms
    virtual std::vector<SocketTransform> getSocketTransforms(
        AnimatorHandle animator,
        std::span<const std::string_view> socketNames,
        const Mat4& entityWorldMatrix) const = 0;

    /// Update socket local transform
    virtual Result<void, AnimationError> setSocketLocalTransform(
        SocketHandle socket,
        const Vec3& position,
        const Quat& rotation,
        const Vec3& scale = Vec3{1.0f}) = 0;

    /// Enable/disable socket
    virtual void setSocketEnabled(SocketHandle socket, bool enabled) = 0;

    //======================================================================
    // Socket Raycasting (Requires Physics System)
    //======================================================================

    /// Perform raycast from socket position
    /// @param animator Source animator
    /// @param def Raycast configuration
    /// @param entityWorldMatrix Entity's world transform
    /// @param physics Physics system for raycasting
    /// @return Raycast result
    virtual SocketRaycastResult raycastFromSocket(
        AnimatorHandle animator,
        const SocketRaycastDef& def,
        const Mat4& entityWorldMatrix,
        const IPhysics3DSystem& physics) const = 0;

    /// Perform multiple raycasts from sockets
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
    // Inverse Kinematics
    //======================================================================

    /// Define two-bone IK chain on skeleton
    virtual Result<void, AnimationError> defineIKChain(
        SkeletonHandle skeleton,
        const IKTwoBoneChain& chain) = 0;

    /// Define aim IK configuration
    virtual Result<void, AnimationError> defineIKAim(
        SkeletonHandle skeleton,
        const IKAimConfig& config) = 0;

    /// Set two-bone IK target for animator
    virtual void setIKTarget(
        AnimatorHandle animator,
        const IKTwoBoneTarget& target) = 0;

    /// Set aim IK target for animator
    virtual void setIKTarget(
        AnimatorHandle animator,
        const IKAimTarget& target) = 0;

    /// Clear specific IK target
    virtual void clearIKTarget(
        AnimatorHandle animator,
        std::string_view targetName) = 0;

    /// Clear all IK targets
    virtual void clearAllIKTargets(AnimatorHandle animator) = 0;

    /// Get current IK weight
    virtual float getIKWeight(
        AnimatorHandle animator,
        std::string_view targetName) const = 0;

    //======================================================================
    // Root Motion
    //======================================================================

    /// Enable root motion extraction for animator
    virtual void setRootMotionEnabled(
        AnimatorHandle animator,
        bool enabled,
        const RootMotionConfig& config = {}) = 0;

    /// Check if root motion is enabled
    virtual bool isRootMotionEnabled(AnimatorHandle animator) const = 0;

    /// Get root motion delta from last update
    /// @return Delta position/rotation since last frame
    virtual RootMotion getRootMotion(AnimatorHandle animator) const = 0;

    /// Get root motion from clip between two times
    virtual RootMotion extractRootMotion(
        AnimationClipHandle clip,
        float fromTime,
        float toTime) const = 0;

    /// Apply root motion to character controller
    virtual void applyRootMotion(
        AnimatorHandle animator,
        Entity characterEntity,
        IPhysics3DSystem& physics) = 0;

    //======================================================================
    // Physics Integration: Ragdoll
    //======================================================================

    /// Create ragdoll from definition
    /// @param rootEntity Entity to attach ragdoll to
    /// @param def Ragdoll configuration
    /// @param physics Physics system for body creation
    /// @return Success or error
    virtual Result<void, AnimationError> createRagdoll(
        Entity rootEntity,
        const RagdollDef& def,
        IPhysics3DSystem& physics) = 0;

    /// Destroy ragdoll
    virtual void destroyRagdoll(
        Entity rootEntity,
        IPhysics3DSystem& physics) = 0;

    /// Check if entity has ragdoll
    virtual bool hasRagdoll(Entity rootEntity) const = 0;

    /// Get ragdoll state
    virtual RagdollState getRagdollState(Entity rootEntity) const = 0;

    /// Activate ragdoll (physics drives bones)
    /// @param instant If true, switch instantly; if false, blend over time
    virtual void activateRagdoll(
        Entity rootEntity,
        IPhysics3DSystem& physics,
        bool instant = false,
        float blendDuration = 0.3f) = 0;

    /// Deactivate ragdoll (animation drives bones)
    virtual void deactivateRagdoll(
        Entity rootEntity,
        IPhysics3DSystem& physics,
        bool instant = false,
        float blendDuration = 0.3f) = 0;

    /// Set ragdoll blend weight manually
    virtual void setRagdollBlendWeight(
        Entity rootEntity,
        float weight) = 0;

    /// Get bone transforms from ragdoll physics bodies
    virtual std::vector<Mat4> getRagdollBoneTransforms(
        Entity rootEntity,
        const IPhysics3DSystem& physics) const = 0;

    //======================================================================
    // Physics Integration: Bone Sync
    //======================================================================

    /// Sync animator bone transforms to kinematic physics bodies
    /// Call after update() to push animation poses to physics
    virtual void syncToPhysics(
        AnimatorHandle animator,
        Entity rootEntity,
        IPhysics3DSystem& physics) = 0;

    /// Apply impulse to ragdoll bone
    virtual void applyBoneImpulse(
        Entity rootEntity,
        std::uint32_t boneIndex,
        const Vec3& impulse,
        IPhysics3DSystem& physics) = 0;

    /// Apply impulse to ragdoll bone by name
    virtual void applyBoneImpulse(
        Entity rootEntity,
        std::string_view boneName,
        const Vec3& impulse,
        IPhysics3DSystem& physics) = 0;

    /// Apply impulse at world position (finds nearest bone)
    virtual void applyImpulseAtPosition(
        Entity rootEntity,
        const Vec3& worldPosition,
        const Vec3& impulse,
        IPhysics3DSystem& physics) = 0;

    //======================================================================
    // Event Subscriptions
    //======================================================================

    /// Subscribe to animation events for an animator
    virtual SubscriptionId subscribeToEvents(
        AnimatorHandle animator,
        AnimationEventCallback callback) = 0;

    /// Subscribe to animation completion
    virtual SubscriptionId subscribeToComplete(
        AnimatorHandle animator,
        AnimationCompleteCallback callback) = 0;

    /// Subscribe to layer changes
    virtual SubscriptionId subscribeToLayerChanges(
        AnimatorHandle animator,
        AnimationLayerCallback callback) = 0;

    /// Unsubscribe from events
    virtual void unsubscribe(SubscriptionId id) = 0;

    //======================================================================
    // Stateless Sampling (Low-level API)
    //======================================================================

    /// Sample animation at specific time without animator
    /// @return Bone transforms in model space
    virtual Result<std::vector<Mat4>, AnimationError> sampleAnimation(
        AnimationClipHandle clip,
        float time,
        AnimationWrapMode wrapMode = AnimationWrapMode::Loop) = 0;

    /// Blend multiple animations without animator
    virtual Result<std::vector<Mat4>, AnimationError> blendAnimations(
        SkeletonHandle skeleton,
        const AnimationBlendConfig& config) = 0;

    //======================================================================
    // Statistics
    //======================================================================

    /// Get animation system statistics
    virtual AnimationStats getStats() const = 0;

    /// Reset per-frame statistics
    virtual void resetFrameStats() = 0;
};

}  // namespace bestow
```

---

## Socket System

### Overview

Sockets are named attachment points on bones, used for:

1. **Weapon attachment** - Sword in hand, shield on back
2. **Effect spawning** - Muzzle flash at gun barrel, footstep dust at feet
3. **Hit detection raycasting** - Sword slash traces, punch detection
4. **IK targets** - Hand positions for grabbing objects
5. **Audio positioning** - Voice from mouth socket, footsteps from feet

### Socket Definition

Sockets are defined per-skeleton and shared by all animators using that skeleton:

```cpp
// Define sockets when loading character
auto skeleton = anim->createSkeleton(modelData).value();

// Weapon sockets
anim->defineSocket(skeleton, {
    .name = "weapon_r",
    .boneName = "RightHand",
    .localPosition = {0.1f, 0.0f, 0.0f},
    .localRotation = Quat::fromEuler(0, 90, 0)  // Sword points forward
});

anim->defineSocket(skeleton, {
    .name = "weapon_l",
    .boneName = "LeftHand",
    .localPosition = {0.1f, 0.0f, 0.0f}
});

// Shield socket on back
anim->defineSocket(skeleton, {
    .name = "shield_back",
    .boneName = "Spine2",
    .localPosition = {0.0f, 0.0f, -0.3f},
    .localRotation = Quat::fromEuler(0, 180, 0)
});

// Combat hit detection sockets
anim->defineSocket(skeleton, {
    .name = "sword_tip",
    .boneName = "RightHand",
    .localPosition = {0.0f, 0.0f, 1.2f}  // 1.2m sword length
});

anim->defineSocket(skeleton, {
    .name = "sword_base",
    .boneName = "RightHand",
    .localPosition = {0.0f, 0.0f, 0.1f}
});

// Effect sockets
anim->defineSocket(skeleton, {
    .name = "foot_l",
    .boneName = "LeftFoot",
    .localPosition = {0.0f, -0.1f, 0.0f}
});

anim->defineSocket(skeleton, {
    .name = "foot_r",
    .boneName = "RightFoot",
    .localPosition = {0.0f, -0.1f, 0.0f}
});
```

### Socket Queries

```cpp
// Get socket world position for attachment
auto transform = anim->getSocketTransform(animator, "weapon_r", entityWorld);
if (transform) {
    // Position sword mesh at socket
    swordEntity.setTransform(transform->worldMatrix);
}

// Spawn effect at socket
auto footTransform = anim->getSocketTransform(animator, "foot_l", entityWorld);
if (footTransform) {
    spawnDustEffect(footTransform->position);
}
```

### Socket Raycasting for Hit Detection

The primary use case for socket raycasting is melee combat hit detection:

```cpp
class MeleeWeaponSystem {
    IAnimationSystem* anim_;
    IPhysics3DSystem* physics_;

    struct WeaponTrace {
        Entity owner;
        AnimatorHandle animator;
        Vec3 lastTipPos;
        Vec3 lastBasePos;
        bool tracing = false;
    };

    std::vector<WeaponTrace> activeTraces_;

public:
    void startWeaponTrace(Entity entity, AnimatorHandle animator) {
        // Get initial socket positions
        auto entityWorld = getWorldMatrix(entity);
        auto tipTransform = anim_->getSocketTransform(
            animator, "sword_tip", entityWorld);
        auto baseTransform = anim_->getSocketTransform(
            animator, "sword_base", entityWorld);

        activeTraces_.push_back({
            .owner = entity,
            .animator = animator,
            .lastTipPos = tipTransform->position,
            .lastBasePos = baseTransform->position,
            .tracing = true
        });
    }

    void update() {
        for (auto& trace : activeTraces_) {
            if (!trace.tracing) continue;

            auto entityWorld = getWorldMatrix(trace.owner);

            // Get current socket positions
            auto tipTransform = anim_->getSocketTransform(
                trace.animator, "sword_tip", entityWorld);
            auto baseTransform = anim_->getSocketTransform(
                trace.animator, "sword_base", entityWorld);

            Vec3 currentTip = tipTransform->position;
            Vec3 currentBase = baseTransform->position;

            // Raycast from last position to current position
            // This catches fast swings that might skip frames

            // Tip trace
            auto tipHit = physics_->raycast(
                trace.lastTipPos,
                currentTip - trace.lastTipPos,
                glm::length(currentTip - trace.lastTipPos),
                CollisionLayers::Hittable
            );

            // Base trace
            auto baseHit = physics_->raycast(
                trace.lastBasePos,
                currentBase - trace.lastBasePos,
                glm::length(currentBase - trace.lastBasePos),
                CollisionLayers::Hittable
            );

            // Also cast along the blade (perpendicular)
            auto bladeHit = physics_->raycast(
                currentBase,
                currentTip - currentBase,
                glm::length(currentTip - currentBase),
                CollisionLayers::Hittable
            );

            // Process hits (deduplicate by entity)
            processHits({tipHit, baseHit, bladeHit}, trace.owner);

            // Store for next frame
            trace.lastTipPos = currentTip;
            trace.lastBasePos = currentBase;
        }
    }

    void stopWeaponTrace(Entity entity) {
        for (auto& trace : activeTraces_) {
            if (trace.owner == entity) {
                trace.tracing = false;
            }
        }
    }
};
```

### Animation Event Integration

Socket raycasting is typically triggered by animation events:

```cpp
// Add events to attack animation
anim->addClipEvent(attackClip, {
    .name = "melee_trace_start",
    .time = 0.15f  // Start tracing at frame 15
});

anim->addClipEvent(attackClip, {
    .name = "melee_trace_end",
    .time = 0.45f  // Stop tracing at frame 45
});

// Subscribe to events
anim->subscribeToEvents(animator, [this](const AnimationEvent& event) {
    if (event.name == "melee_trace_start") {
        meleeSystem.startWeaponTrace(entity, animator);
    } else if (event.name == "melee_trace_end") {
        meleeSystem.stopWeaponTrace(entity);
    }
});
```

### Socket-Based Aim Direction

For ranged weapons, sockets provide aim direction:

```cpp
// Get muzzle socket for shooting
auto muzzle = anim->getSocketTransform(animator, "muzzle", entityWorld);
if (muzzle) {
    // Raycast in socket's forward direction
    auto hit = physics->raycast(
        muzzle->position,
        muzzle->forward,  // -Z in socket space
        1000.0f,          // Max range
        CollisionLayers::Shootable
    );

    if (hit) {
        applyDamage(hit.entity, weaponDamage);
        spawnImpactEffect(hit.point, hit.normal);
    }

    // Spawn muzzle flash at socket
    spawnMuzzleFlash(muzzle->position, muzzle->rotation);
}
```

---

## Physics Integration

### Ragdoll System

```cpp
// Create ragdoll when character spawns
auto ragdollDef = RagdollDef::createBipedDefault(skeleton);
anim->createRagdoll(characterEntity, ragdollDef, *physics);

// On death: activate ragdoll
void onCharacterDeath(Entity entity, Vec3 killDirection) {
    // Blend to ragdoll over 0.2 seconds
    anim->activateRagdoll(entity, *physics, false, 0.2f);

    // Apply death impulse
    anim->applyBoneImpulse(entity, "Spine1", killDirection * 500.0f, *physics);
}

// Update: choose animation or physics transforms
void render(Entity entity) {
    std::span<const Mat4> boneTransforms;

    auto ragdollState = anim->getRagdollState(entity);
    if (ragdollState.active || ragdollState.blendWeight > 0.0f) {
        // Physics-driven (or blending)
        auto physicsTransforms = anim->getRagdollBoneTransforms(entity, *physics);

        if (ragdollState.blendWeight < 1.0f) {
            // Blend animation and physics
            auto animTransforms = anim->getBoneTransforms(animator);
            boneTransforms = blendTransforms(
                animTransforms,
                physicsTransforms,
                ragdollState.blendWeight
            );
        } else {
            boneTransforms = physicsTransforms;
        }
    } else {
        // Animation-driven
        boneTransforms = anim->getBoneTransforms(animator);
    }

    graphics->drawSkinnedMesh(mesh, material, worldMatrix, boneTransforms);
}
```

### Root Motion

```cpp
// Enable root motion on animator
anim->setRootMotionEnabled(animator, true, {
    .extractTranslationX = true,
    .extractTranslationY = false,  // Physics handles vertical
    .extractTranslationZ = true,
    .extractRotation = true
});

// In update loop
void update(float dt) {
    anim->update(dt);

    // Get root motion delta
    auto rootMotion = anim->getRootMotion(animator);

    if (rootMotion.hasTranslation) {
        // Apply to character controller
        physics->moveCharacter(
            characterEntity,
            rootMotion.deltaPosition / dt,  // Convert to velocity
            dt
        );
    }

    if (rootMotion.hasRotation) {
        // Apply rotation
        auto currentRot = entity.getRotation();
        entity.setRotation(rootMotion.deltaRotation * currentRot);
    }
}
```

### Kinematic Bone Sync

For animated objects with physics interaction:

```cpp
// Moving platform with animation
void updateAnimatedPlatform() {
    // Play animation
    anim->update(dt);

    // Sync bone colliders to physics
    // (platform collision follows animation)
    anim->syncToPhysics(animator, platformEntity, *physics);
}
```

---

## Library Recommendations

### Animation Runtime: ozz-animation

| Aspect | Details |
|--------|---------|
| **License** | MIT |
| **Version** | 0.16.0 (January 2025) |
| **Features** | Sampling, blending, two-bone IK, aim IK, SIMD optimization |
| **Integration** | Available via vcpkg |
| **Pros** | Production-quality, actively maintained, renderer-agnostic |

### Model Loading: Assimp + tinygltf

| Library | Use Case | License |
|---------|----------|---------|
| **tinygltf** | glTF 2.0 files (recommended format) | MIT |
| **Assimp** | FBX, COLLADA, OBJ, and 40+ other formats | BSD-3 |

**Recommendation:** Use tinygltf for glTF files (cleaner, faster), Assimp for legacy formats.

### Proposed Dependencies (vcpkg.json additions)

```json
{
  "dependencies": [
    "ozz-animation",
    "assimp"
  ]
}
```

Note: tinygltf is already included in the project.

---

## Migration Path

### Phase 1: Create Animation Contract Module

1. Create `/home/user/bestow/bestow-contract/src/bestow.animation.cppm` with full interface
2. Add `IAnimationSystemService` to `bestow.services.cppm`
3. Add `ServiceFor<IAnimationSystem>` mapping
4. Export from `bestow.services.cppm` via `export import bestow.animation`

### Phase 2: Create Animation System Implementation

1. Create `/home/user/bestow/bestow-animation/` directory structure
2. Create `CMakeLists.txt` with ozz-animation dependency
3. Implement `AnimationSystem` class using ozz-animation
4. Implement skeleton management (ozz::animation::Skeleton wrapper)
5. Implement clip management (ozz::animation::Animation wrapper)
6. Implement animator with layer support
7. Implement sampling jobs (ozz::animation::SamplingJob)
8. Implement blending jobs (ozz::animation::BlendingJob)
9. Implement socket system
10. Implement IK solvers (ozz::animation::IKTwoBoneJob, ozz::animation::IKAimJob)

### Phase 3: Add Physics Integration

1. Implement ragdoll creation from skeleton
2. Implement ragdoll activation/deactivation
3. Implement bone transform sync to kinematic bodies
4. Implement physics-to-animation transform reading
5. Implement root motion extraction
6. Implement bone impulse application

### Phase 4: Add Asset Loading Support

1. Create animation asset loader in bestow-assets
2. Support glTF animation loading via tinygltf
3. Support FBX animation loading via Assimp (optional)
4. Support ozz binary format for optimized runtime loading
5. Add hot reload support for animation files

### Phase 5: Add ECS Components

1. Add `SkeletalAnimation` component to `bestow.components`
2. Add `AnimationController` component for state machine
3. Add `RagdollComponent` for physics integration
4. Add `SocketAttachment` component for attached objects

### Phase 6: Deprecate Graphics3D Animation API

1. Mark Graphics3D animation methods as `[[deprecated]]`
2. Update all internal uses to use IAnimationSystem
3. Update documentation
4. Update examples

### Phase 7: Remove Graphics3D Animation API

1. Remove skeleton/clip creation from IGraphics3DSystem
2. Remove sampling/blending methods
3. Keep only `drawSkinnedMesh(mesh, material, world, boneTransforms)`
4. Remove animation code from OpenGL and Vulkan backends

### Phase 8: Create Examples

1. Create animated character example with locomotion
2. Create melee combat example with socket raycasting
3. Create ragdoll physics example
4. Create IK example (foot placement, look-at)

### Phase 9: Documentation

1. Write API documentation for bestow.animation module
2. Write tutorial for basic animation playback
3. Write tutorial for socket-based hit detection
4. Write tutorial for ragdoll integration
5. Update Architecture.md with animation system

---

## File Structure

```
bestow/
├── bestow-contract/
│   └── src/
│       ├── bestow.animation.cppm      # NEW: Animation contract
│       └── bestow.services.cppm       # MODIFIED: Add animation service
│
├── bestow-animation/                   # NEW: Animation implementation
│   ├── CMakeLists.txt
│   ├── src/
│   │   ├── bestow.animation.impl.cppm # Main implementation
│   │   ├── Skeleton.cpp               # Skeleton management
│   │   ├── AnimationClip.cpp          # Clip management
│   │   ├── Animator.cpp               # Animator with layers
│   │   ├── Sampler.cpp                # ozz sampling wrapper
│   │   ├── Blender.cpp                # ozz blending wrapper
│   │   ├── Socket.cpp                 # Socket system
│   │   ├── IKSolver.cpp               # IK implementations
│   │   ├── RootMotion.cpp             # Root motion extraction
│   │   ├── Ragdoll.cpp                # Physics integration
│   │   └── AnimationSystem.cpp        # Main system class
│   └── tests/
│       ├── test_skeleton.cpp
│       ├── test_sampling.cpp
│       ├── test_blending.cpp
│       ├── test_socket.cpp
│       └── test_ragdoll.cpp
│
├── bestow-assets/
│   └── src/
│       └── loaders/
│           ├── AnimationLoader.cpp    # NEW: Animation asset loader
│           └── ModelLoader.cpp        # MODIFIED: Include animation data
│
├── bestow-components/
│   └── src/
│       └── bestow.components.cppm     # MODIFIED: Add animation components
│
├── examples/
│   ├── animated-character/            # NEW: Basic animation example
│   ├── melee-combat/                  # NEW: Socket raycast example
│   └── ragdoll-physics/               # NEW: Ragdoll example
│
└── docs/
    └── systems/
        └── Animation-System.md        # NEW: Animation documentation
```

---

## Example Usage

### Complete Character Controller

```cpp
#include <bestow/bestow.hpp>

class CharacterController {
    // Systems
    IAnimationSystem* anim_;
    IPhysics3DSystem* physics_;
    IGraphics3DSystem* graphics_;
    IInputSystem* input_;

    // Resources
    SkeletonHandle skeleton_;
    AnimatorHandle animator_;
    MeshHandle mesh_;
    MaterialHandle material_;

    // State
    Entity entity_;
    bool isDead_ = false;
    bool isAttacking_ = false;

public:
    void initialize() {
        // Load model with animations
        auto modelData = loadModel("characters/warrior.gltf");

        // Create rendering resources
        mesh_ = graphics_->createMeshFromData(modelData.meshes[0]).value();
        material_ = graphics_->createPBRMaterial(/* ... */).value();

        // Create animation resources
        skeleton_ = anim_->createSkeleton(modelData).value();
        anim_->createAnimationClips(skeleton_, modelData);

        // Define sockets
        anim_->defineSockets(skeleton_, {
            {.name = "weapon_r", .boneName = "RightHand",
             .localPosition = {0.1f, 0, 0}},
            {.name = "sword_tip", .boneName = "RightHand",
             .localPosition = {0, 0, 1.2f}},
            {.name = "sword_base", .boneName = "RightHand",
             .localPosition = {0, 0, 0.1f}},
            {.name = "foot_l", .boneName = "LeftFoot",
             .localPosition = {0, -0.1f, 0}},
            {.name = "foot_r", .boneName = "RightFoot",
             .localPosition = {0, -0.1f, 0}}
        });

        // Define IK chains
        anim_->defineIKChain(skeleton_, {
            .name = "left_arm",
            .rootBoneName = "LeftUpperArm",
            .midBoneName = "LeftLowerArm",
            .tipBoneName = "LeftHand"
        });

        anim_->defineIKAim(skeleton_, {
            .name = "head_look",
            .boneName = "Head",
            .aimAxis = {0, 0, 1},
            .angleLimits = {70.0f, 45.0f}
        });

        // Create animator
        animator_ = anim_->createAnimator(skeleton_).value();

        // Enable root motion
        anim_->setRootMotionEnabled(animator_, true);

        // Create ragdoll (starts inactive)
        auto ragdollDef = RagdollDef::createBipedDefault(skeleton_);
        anim_->createRagdoll(entity_, ragdollDef, *physics_);

        // Subscribe to animation events
        anim_->subscribeToEvents(animator_, [this](const AnimationEvent& e) {
            onAnimationEvent(e);
        });

        // Start with idle animation
        anim_->play(animator_, "idle");
    }

    void update(float dt) {
        if (isDead_) {
            // Physics drives ragdoll, nothing to update
            return;
        }

        handleInput();

        // Update animations
        anim_->update(dt);

        // Apply root motion to physics
        auto rootMotion = anim_->getRootMotion(animator_);
        if (rootMotion.hasTranslation) {
            physics_->moveCharacter(entity_,
                rootMotion.deltaPosition / dt, dt);
        }

        // Update look-at IK
        if (auto target = findLookTarget()) {
            anim_->setIKTarget(animator_, IKAimTarget{
                .configName = "head_look",
                .targetPosition = *target,
                .weight = 0.7f
            });
        } else {
            anim_->clearIKTarget(animator_, "head_look");
        }

        // Handle active weapon trace
        if (isAttacking_) {
            updateWeaponTrace();
        }
    }

    void render() {
        Mat4 worldMatrix = entity_.getWorldMatrix();
        std::span<const Mat4> boneTransforms;

        auto ragdollState = anim_->getRagdollState(entity_);
        if (ragdollState.active) {
            boneTransforms = anim_->getRagdollBoneTransforms(
                entity_, *physics_);
        } else {
            boneTransforms = anim_->getBoneTransforms(animator_);
        }

        graphics_->drawSkinnedMesh(mesh_, material_,
            worldMatrix, boneTransforms);

        // Draw weapon at socket
        auto weaponSocket = anim_->getSocketTransform(
            animator_, "weapon_r", worldMatrix);
        if (weaponSocket) {
            graphics_->drawMesh(swordMesh_, swordMaterial_,
                weaponSocket->worldMatrix);
        }
    }

private:
    void handleInput() {
        // Movement
        Vec2 moveInput = input_->getLeftStick();
        if (glm::length(moveInput) > 0.1f) {
            if (!isAttacking_) {
                anim_->play(animator_, "run", 0.2f);
            }
        } else {
            if (!isAttacking_) {
                anim_->play(animator_, "idle", 0.3f);
            }
        }

        // Attack
        if (input_->isButtonPressed(Button::X) && !isAttacking_) {
            anim_->play(animator_, "attack_slash", 0.1f);
            isAttacking_ = true;
        }
    }

    void onAnimationEvent(const AnimationEvent& event) {
        if (event.name == "footstep") {
            auto foot = event.stringParam;  // "left" or "right"
            auto socketName = foot == "left" ? "foot_l" : "foot_r";
            auto transform = anim_->getSocketTransform(
                animator_, socketName, entity_.getWorldMatrix());
            if (transform) {
                playFootstepSound(transform->position);
                spawnDustEffect(transform->position);
            }
        }
        else if (event.name == "attack_trace_start") {
            startWeaponTrace();
        }
        else if (event.name == "attack_trace_end") {
            stopWeaponTrace();
            isAttacking_ = false;
        }
    }

    void startWeaponTrace() {
        auto world = entity_.getWorldMatrix();
        lastTipPos_ = anim_->getSocketTransform(
            animator_, "sword_tip", world)->position;
        lastBasePos_ = anim_->getSocketTransform(
            animator_, "sword_base", world)->position;
        tracedEntities_.clear();
    }

    void updateWeaponTrace() {
        auto world = entity_.getWorldMatrix();
        Vec3 currentTip = anim_->getSocketTransform(
            animator_, "sword_tip", world)->position;
        Vec3 currentBase = anim_->getSocketTransform(
            animator_, "sword_base", world)->position;

        // Raycast along sword movement
        auto hits = {
            physics_->raycast(lastTipPos_, currentTip - lastTipPos_,
                glm::length(currentTip - lastTipPos_)),
            physics_->raycast(lastBasePos_, currentBase - lastBasePos_,
                glm::length(currentBase - lastBasePos_)),
            physics_->raycast(currentBase, currentTip - currentBase,
                glm::length(currentTip - currentBase))
        };

        for (auto& hit : hits) {
            if (hit && hit.entity != entity_ &&
                tracedEntities_.find(hit.entity) == tracedEntities_.end()) {
                tracedEntities_.insert(hit.entity);
                applyDamage(hit.entity, 25, hit.point, hit.normal);
            }
        }

        lastTipPos_ = currentTip;
        lastBasePos_ = currentBase;
    }

    void stopWeaponTrace() {
        // Trace complete
    }

public:
    void takeDamage(int amount, Vec3 hitDirection) {
        health_ -= amount;

        if (health_ <= 0) {
            die(hitDirection);
        } else {
            // Play hit reaction
            anim_->play(animator_, "hit_react", 0.1f);
        }
    }

    void die(Vec3 killDirection) {
        isDead_ = true;

        // Activate ragdoll with smooth blend
        anim_->activateRagdoll(entity_, *physics_, false, 0.2f);

        // Apply death impulse to spine
        anim_->applyBoneImpulse(entity_, "Spine1",
            killDirection * 800.0f, *physics_);
    }

private:
    Vec3 lastTipPos_, lastBasePos_;
    std::set<Entity> tracedEntities_;
    int health_ = 100;
};
```

---

## Appendix: Mixamo Integration Notes

Mixamo exports characters and animations that work directly with this system:

1. **Export format:** Use glTF 2.0 (`.glb`) for best compatibility
2. **Skeleton:** Mixamo rigs map cleanly to humanoid bone conventions
3. **Animations:** Download as "Without Skin" to get animation-only files
4. **Sockets:** Common Mixamo bone names:
   - `mixamorig:RightHand` → weapon_r socket
   - `mixamorig:LeftHand` → weapon_l socket
   - `mixamorig:Head` → head IK target
   - `mixamorig:RightFoot`, `mixamorig:LeftFoot` → footstep sockets

---

## References

- [ozz-animation Documentation](https://guillaumeblanc.github.io/ozz-animation/)
- [glTF 2.0 Specification](https://www.khronos.org/gltf/)
- [Mixamo](https://www.mixamo.com/)
- [Game Animation Programming (O'Reilly)](https://www.oreilly.com/library/view/game-animation-programming/9781800202047/)

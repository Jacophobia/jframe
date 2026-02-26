# Animation System

> **Visibility:** Public (Lua + C++ API)
> **Tier:** 4
> **Dependencies:** Types, Entity, Assets
> **Lua Paths:** `bestow.animation` (high-level), `bestow.animation.core` (low-level), `bestow.animation.fsm` (state machine)

## Purpose

The Animation System provides skeletal animation for 3D entities including clip playback, blending, inverse kinematics, socket attachments, ragdoll physics integration, and root motion extraction. The high-level API offers simple entity-centric play/stop commands with automatic model setup, while the low-level API exposes full skeleton and clip handle management, multi-layer blending, blend trees, IK solvers, and bone-level control. A dedicated Animation State Machine sub-contract (`bestow.animation.fsm`) provides a parameter-driven state machine with a fluent builder pattern for managing complex animation graphs.

## High-Level API: `IAnimationSystem`

The simplified API for common animation tasks. Entity-centric operations that accept clip names as strings and manage internal handles automatically. No lifecycle methods -- the engine manages those internally.

### Playback

| Method | Returns | Description |
|--------|---------|-------------|
| `play(Entity entity, std::string_view clipName, float blendTime = 0.25f, bool loop = true)` | `Result<void>` | Play a named animation clip on the entity with optional blend-in time and looping |
| `stop(Entity entity, float blendTime = 0)` | `Result<void>` | Stop the current animation on the entity with an optional blend-out time |
| `isPlaying(Entity entity)` | `bool` | Return whether the entity is currently playing any animation |

### Speed

| Method | Returns | Description |
|--------|---------|-------------|
| `setSpeed(Entity entity, float speed)` | `Result<void>` | Set the playback speed multiplier for the entity's animation (1.0 = normal, negative = reverse) |

### Queries

| Method | Returns | Description |
|--------|---------|-------------|
| `getTime(Entity entity)` | `float` | Return the current playback time in seconds for the entity's animation |
| `getNormalizedTime(Entity entity)` | `float` | Return the current playback progress as a normalized value in the range [0, 1] |
| `getCurrentClipName(Entity entity)` | `std::string_view` | Return the name of the currently playing animation clip |

### Setup

| Method | Returns | Description |
|--------|---------|-------------|
| `setupFromModel(Entity entity, std::string_view modelPath)` | `Result<void>` | Load a model asset, create skeleton and animator for the entity, and register all embedded clips |

### Events

| Method | Returns | Description |
|--------|---------|-------------|
| `onAnimationComplete(std::function<void(Entity, std::string_view clipName)> cb)` | `SubscriptionId` | Subscribe to animation completion events; called when a non-looping clip finishes |
| `unsubscribe(SubscriptionId id)` | `void` | Remove a previously registered animation event subscription |

## Low-Level API: `IAnimationCore`

Full control API. Exposes handle-based skeleton and clip management, animator instances, multi-layer blending, blend trees, pose sampling, IK solvers, socket attachments, ragdoll integration, animation events, and root motion.

### Lifecycle

| Method | Returns | Description |
|--------|---------|-------------|
| `update(DeltaTime dt)` | `void` | Advance all active animators by the given delta time |

### Skeleton Management

| Method | Returns | Description |
|--------|---------|-------------|
| `createSkeleton(const ModelData& model)` | `Result<SkeletonHandle>` | Create a skeleton from model data and return its handle |
| `destroySkeleton(SkeletonHandle handle)` | `Result<void>` | Destroy a skeleton and free its resources |
| `getBoneCount(SkeletonHandle skeleton)` | `int` | Return the number of bones in the skeleton |
| `findBone(SkeletonHandle skeleton, std::string_view name)` | `std::optional<int>` | Find a bone index by name, or nullopt if not found |
| `getBoneName(SkeletonHandle skeleton, int index)` | `std::string_view` | Return the name of the bone at the given index |

### Clip Management

| Method | Returns | Description |
|--------|---------|-------------|
| `createClip(std::string_view name, const ModelData::Animation& anim, SkeletonHandle skeleton)` | `Result<AnimClipHandle>` | Create an animation clip from in-memory animation data |
| `loadClip(std::string_view name, AssetHandle asset, SkeletonHandle skeleton)` | `Result<AnimClipHandle>` | Load an animation clip from an asset file |
| `destroyClip(AnimClipHandle handle)` | `Result<void>` | Destroy an animation clip |
| `getClipDuration(AnimClipHandle clip)` | `float` | Return the clip duration in seconds |

### Animator Instances

| Method | Returns | Description |
|--------|---------|-------------|
| `createAnimator(Entity entity, SkeletonHandle skeleton)` | `Result<AnimatorHandle>` | Create an animator instance bound to an entity and skeleton |
| `destroyAnimator(AnimatorHandle handle)` | `Result<void>` | Destroy an animator instance |
| `getAnimator(Entity entity)` | `std::optional<AnimatorHandle>` | Get the animator handle for an entity, or nullopt if none exists |

### Playback Control

| Method | Returns | Description |
|--------|---------|-------------|
| `play(AnimatorHandle anim, AnimClipHandle clip, float blendTime = 0.25f, bool loop = true)` | `Result<void>` | Play a clip on the animator with blend-in time and loop control |
| `stop(AnimatorHandle anim, float blendTime = 0)` | `Result<void>` | Stop the current animation with optional blend-out |
| `pause(AnimatorHandle anim)` | `Result<void>` | Pause the animator at the current time |
| `resume(AnimatorHandle anim)` | `Result<void>` | Resume a paused animator |
| `setPlaybackSpeed(AnimatorHandle anim, float speed)` | `Result<void>` | Set the playback speed multiplier |
| `setTime(AnimatorHandle anim, float time)` | `Result<void>` | Seek to a specific time in the current clip |
| `getTime(AnimatorHandle anim)` | `float` | Return the current playback time in seconds |
| `getNormalizedTime(AnimatorHandle anim)` | `float` | Return the normalized playback progress [0, 1] |
| `isPlaying(AnimatorHandle anim)` | `bool` | Return whether the animator is currently playing |

### Blending

| Method | Returns | Description |
|--------|---------|-------------|
| `crossfade(AnimatorHandle anim, AnimClipHandle toClip, float blendTime)` | `Result<void>` | Crossfade from the current clip to a new clip over the given duration |
| `setLayerWeight(AnimatorHandle anim, int layer, float weight)` | `Result<void>` | Set the blend weight for an animation layer (0.0 to 1.0) |
| `setLayerMask(AnimatorHandle anim, int layer, std::span<const int> boneIndices)` | `Result<void>` | Restrict a layer to affect only the specified bones |
| `setBlendTree(AnimatorHandle anim, const BlendTreeDef& tree)` | `Result<void>` | Apply a blend tree definition for parameter-driven multi-clip blending |

### Sampling

| Method | Returns | Description |
|--------|---------|-------------|
| `samplePose(AnimatorHandle anim)` | `std::vector<Mat4>` | Sample the final blended bone matrices for the current animator state |
| `sampleClip(AnimClipHandle clip, float time, bool loop = true)` | `std::vector<Mat4>` | Sample bone matrices from a specific clip at a given time |
| `blendPoses(std::span<const Mat4> a, std::span<const Mat4> b, float weight)` | `std::vector<Mat4>` | Linearly blend two pose arrays by weight (0.0 = a, 1.0 = b) |

### IK Solvers

| Method | Returns | Description |
|--------|---------|-------------|
| `setIKTarget(AnimatorHandle anim, std::string_view chainName, Vec3 target, float weight = 1.0f)` | `Result<void>` | Set the world-space IK target for a named IK chain |
| `clearIKTarget(AnimatorHandle anim, std::string_view chainName)` | `Result<void>` | Remove the IK target for a chain, reverting to animation-driven poses |
| `addIKChain(AnimatorHandle anim, std::string_view name, int startBone, int endBone, int maxIterations = 10)` | `Result<void>` | Define an IK chain between two bones with a maximum iteration count |

### Socket Attachments

| Method | Returns | Description |
|--------|---------|-------------|
| `createSocket(AnimatorHandle anim, std::string_view socketName, int boneIndex, Vec3 offset = {}, Quat rotation = {})` | `Result<void>` | Create a named socket attached to a bone with a local offset and rotation |
| `getSocketTransform(AnimatorHandle anim, std::string_view socketName)` | `std::optional<Mat4>` | Get the current world-space transform of a socket, or nullopt if not found |

### Ragdoll

| Method | Returns | Description |
|--------|---------|-------------|
| `enableRagdoll(AnimatorHandle anim)` | `Result<void>` | Enable ragdoll physics for the animator, transitioning from animation to simulation |
| `disableRagdoll(AnimatorHandle anim)` | `Result<void>` | Disable ragdoll and return to animation-driven bone poses |
| `isRagdollActive(AnimatorHandle anim)` | `bool` | Return whether ragdoll physics is currently active for the animator |

### Events

| Method | Returns | Description |
|--------|---------|-------------|
| `onAnimationEvent(std::function<void(AnimatorHandle, std::string_view eventName)> cb)` | `SubscriptionId` | Subscribe to named animation events embedded in clips (e.g., footstep triggers) |
| `onAnimationComplete(std::function<void(AnimatorHandle, AnimClipHandle)> cb)` | `SubscriptionId` | Subscribe to clip completion events for non-looping animations |
| `unsubscribe(SubscriptionId id)` | `void` | Remove a previously registered event subscription |

### Root Motion

| Method | Returns | Description |
|--------|---------|-------------|
| `enableRootMotion(AnimatorHandle anim, bool enabled)` | `Result<void>` | Enable or disable root motion extraction from the animation |
| `getRootMotionDelta(AnimatorHandle anim)` | `Vec3` | Return the root bone position delta since the last frame |
| `getRootMotionRotationDelta(AnimatorHandle anim)` | `Quat` | Return the root bone rotation delta since the last frame |

## Animation State Machine: `IAnimationStateMachine`

Accessed via `bestow.animation.fsm` in Lua. Provides a parameter-driven animation state machine that automatically evaluates transitions and manages blending. Instances are created per-entity from a definition built with the fluent builder pattern.

### Lifecycle

| Method | Returns | Description |
|--------|---------|-------------|
| `update(DeltaTime dt)` | `void` | Advance the state machine by delta time, evaluating transitions |
| `reset()` | `void` | Reset the state machine to the default state |

### State Queries

| Method | Returns | Description |
|--------|---------|-------------|
| `getState()` | `AnimStateMachineState` | Return the full runtime state including current state, transition status, and timing |
| `getCurrentStateId()` | `AnimStateId` | Return the current state identifier |
| `getCurrentStateName()` | `std::string_view` | Return the current state name |
| `getStateConfig(AnimStateId id)` | `const AnimationStateConfig*` | Return the configuration for a state by ID, or nullptr if not found |
| `findState(std::string_view name)` | `AnimStateId` | Look up a state ID by name |
| `isInState(AnimStateId id)` | `bool` | Check whether the machine is currently in the specified state by ID |
| `isInState(std::string_view name)` | `bool` | Check whether the machine is currently in the specified state by name |
| `isTransitioning()` | `bool` | Return whether a transition is currently in progress |
| `getStateTime()` | `float` | Return the time spent in the current state in seconds |
| `getNormalizedTime()` | `float` | Return the normalized animation progress [0, 1] for the current state |

### Manual State Control

| Method | Returns | Description |
|--------|---------|-------------|
| `forceState(AnimStateId state, float blendTime = -1.0f)` | `void` | Force an immediate transition to the target state by ID, bypassing conditions (-1 uses default blend time) |
| `forceState(std::string_view stateName, float blendTime = -1.0f)` | `void` | Force an immediate transition to the target state by name |

### Parameter Control

| Method | Returns | Description |
|--------|---------|-------------|
| `setBool(std::string_view name, bool value)` | `void` | Set a boolean parameter value |
| `getBool(std::string_view name)` | `bool` | Get the current value of a boolean parameter |
| `setInt(std::string_view name, int value)` | `void` | Set an integer parameter value |
| `getInt(std::string_view name)` | `int` | Get the current value of an integer parameter |
| `setFloat(std::string_view name, float value)` | `void` | Set a float parameter value |
| `getFloat(std::string_view name)` | `float` | Get the current value of a float parameter |
| `setTrigger(std::string_view name)` | `void` | Set a trigger parameter; auto-resets to false after being consumed by a transition |
| `resetTrigger(std::string_view name)` | `void` | Manually reset a trigger parameter to false |

### Animation Access

| Method | Returns | Description |
|--------|---------|-------------|
| `getAnimator()` | `AnimatorHandle` | Return the animator handle associated with this state machine |

### Callbacks

| Method | Returns | Description |
|--------|---------|-------------|
| `setOnStateEnter(StateEnterCallback callback)` | `void` | Set a callback invoked when entering any state; receives (stateId, fromStateId) |
| `setOnStateExit(StateExitCallback callback)` | `void` | Set a callback invoked when exiting any state; receives (stateId, toStateId) |
| `setOnTransition(TransitionCallback callback)` | `void` | Set a callback invoked when a transition begins; receives (fromId, toId, blendTime) |

### Builder Pattern: `AnimationStateMachineBuilder`

| Method | Returns | Description |
|--------|---------|-------------|
| `AnimationStateMachineBuilder(std::string name)` | *(constructor)* | Create a new builder with the given state machine name |
| `addState(AnimStateId id, std::string name, std::string clipName, AnimationWrapMode wrapMode = Loop, float blendTime = 0.25f)` | `Builder&` | Add a state with its animation clip, wrap mode, and default blend-in time |
| `setDefaultState(AnimStateId id)` | `Builder&` | Set which state the machine starts in |
| `addTransition(AnimStateId from, AnimStateId to, std::vector<TransitionCondition> conditions = {}, float blendTime = -1.0f, int priority = 0)` | `Builder&` | Add a transition between states with conditions, blend time override, and priority |
| `addParameter(std::string name, AnimParamType type, AnimParamValue defaultValue = false)` | `Builder&` | Add a named parameter that can drive transitions |
| `build()` | `AnimationStateMachineDef` | Finalize and return the state machine definition |

### Condition Helpers

| Function | Returns | Description |
|----------|---------|-------------|
| `onAnimationEnd()` | `TransitionCondition` | Create a condition that triggers when the current non-looping animation finishes |
| `afterTime(float seconds)` | `TransitionCondition` | Create a condition that triggers after spending the specified time in the current state |
| `paramGreater(std::string name, float value)` | `TransitionCondition` | Create a condition that triggers when a float parameter exceeds the given value |
| `paramLess(std::string name, float value)` | `TransitionCondition` | Create a condition that triggers when a float parameter is below the given value |
| `paramEquals(std::string name, bool value)` | `TransitionCondition` | Create a condition that triggers when a bool parameter matches the given value |
| `onTrigger(std::string name)` | `TransitionCondition` | Create a condition that triggers when the named trigger parameter is set |

### Factory Function

| Function | Returns | Description |
|----------|---------|-------------|
| `createAnimationStateMachine(IAnimationSystem* animSystem, AnimatorHandle animator, AnimationStateMachineDef definition)` | `std::unique_ptr<IAnimationStateMachine>` | Create a new state machine instance from a definition |

## Types

### BlendTreeDef

| Field | Type | Default | Description |
|-------|------|---------|-------------|
| `clips` | `std::vector<AnimClipHandle>` | `{}` | Animation clips to blend between |
| `parameter` | `std::string` | `""` | Name of the float parameter that drives the blend |
| `thresholds` | `std::vector<float>` | `{}` | Parameter value thresholds for each clip (must match clips count) |

### AnimationWrapMode

| Value | Description |
|-------|-------------|
| `Once` | Play once and stop at the last frame |
| `Loop` | Restart from the beginning when the clip ends |
| `PingPong` | Reverse playback direction at each end |
| `ClampForever` | Hold the last frame indefinitely |

### AnimStateId

`std::uint32_t` -- Unique identifier for animation states within a state machine. `InvalidAnimState` (0) represents no state.

### AnimParamType

| Value | Description |
|-------|-------------|
| `Bool` | Boolean parameter |
| `Int` | Integer parameter |
| `Float` | Floating-point parameter |
| `Trigger` | Boolean that auto-resets to false after consumption by a transition |

### TransitionCondition

| Field | Type | Default | Description |
|-------|------|---------|-------------|
| `type` | `TransitionConditionType` | `Immediate` | Type of condition (Immediate, OnAnimationEnd, AfterTime, OnParameter) |
| `timeThreshold` | `float` | `0.0f` | Time threshold for AfterTime conditions (seconds) |
| `parameterName` | `std::string` | `""` | Parameter name for OnParameter conditions |
| `comparison` | `ParameterComparison` | `Equal` | Comparison operator for parameter conditions |
| `parameterValue` | `AnimParamValue` | `false` | Value to compare against |

### TransitionConditionType

| Value | Description |
|-------|-------------|
| `Immediate` | Always true; transition as soon as evaluated |
| `OnAnimationEnd` | Wait for the current non-looping animation to finish |
| `AfterTime` | Trigger after spending a specified time in the current state |
| `OnParameter` | Trigger when a parameter condition is met |

### ParameterComparison

| Value | Description |
|-------|-------------|
| `Equal` | Parameter equals the threshold value |
| `NotEqual` | Parameter does not equal the threshold value |
| `Greater` | Parameter is greater than the threshold |
| `Less` | Parameter is less than the threshold |
| `GreaterOrEqual` | Parameter is greater than or equal to the threshold |
| `LessOrEqual` | Parameter is less than or equal to the threshold |

### AnimationStateConfig

| Field | Type | Default | Description |
|-------|------|---------|-------------|
| `id` | `AnimStateId` | `InvalidAnimState` | Unique state identifier |
| `name` | `std::string` | `""` | Human-readable state name |
| `clip` | `AnimationClipHandle` | `InvalidClip` | Animation clip handle for this state |
| `clipName` | `std::string` | `""` | Alternative: look up clip by name instead of handle |
| `wrapMode` | `AnimationWrapMode` | `Loop` | How the animation behaves when reaching its end |
| `speed` | `float` | `1.0f` | Playback speed multiplier |
| `defaultBlendTime` | `float` | `0.25f` | Default transition blend time into this state |
| `interruptible` | `bool` | `true` | Whether this state can be interrupted mid-playback |

### AnimationTransitionDef

| Field | Type | Default | Description |
|-------|------|---------|-------------|
| `fromState` | `AnimStateId` | `InvalidAnimState` | Source state (InvalidAnimState means "from any state") |
| `toState` | `AnimStateId` | `InvalidAnimState` | Target state |
| `conditions` | `std::vector<TransitionCondition>` | `{}` | All conditions that must be true (AND logic) for the transition to fire |
| `blendTime` | `float` | `0.25f` | Transition blend duration in seconds |
| `hasBlendTimeOverride` | `bool` | `false` | Whether blendTime was explicitly set (vs. using the target state's default) |
| `priority` | `int` | `0` | Higher-priority transitions are evaluated first |

### AnimationStateMachineDef

| Field | Type | Default | Description |
|-------|------|---------|-------------|
| `name` | `std::string` | `""` | State machine name for debugging |
| `states` | `std::vector<AnimationStateConfig>` | `{}` | All states in the machine |
| `defaultState` | `AnimStateId` | `InvalidAnimState` | The initial state entered on creation |
| `transitions` | `std::vector<AnimationTransitionDef>` | `{}` | All transitions between states |
| `parameters` | `std::vector<AnimParameterDef>` | `{}` | Parameters that drive transition conditions |

### AnimStateMachineState

| Field | Type | Default | Description |
|-------|------|---------|-------------|
| `currentState` | `AnimStateId` | `InvalidAnimState` | Currently active state |
| `previousState` | `AnimStateId` | `InvalidAnimState` | State before the last transition |
| `targetState` | `AnimStateId` | `InvalidAnimState` | Target state during an active transition |
| `stateTime` | `float` | `0.0f` | Time spent in the current state in seconds |
| `normalizedTime` | `float` | `0.0f` | Normalized animation progress [0, 1] |
| `inTransition` | `bool` | `false` | Whether a transition is in progress |
| `animationComplete` | `bool` | `false` | Whether a non-looping animation has finished |

## Lua Examples

```lua
-- High-level: Simple animation playback
local _, err = bestow.animation.setupFromModel(player, "models/hero.glb")
if err then print("Setup failed: " .. err.message) return end

bestow.animation.play(player, "idle")
bestow.animation.setSpeed(player, 1.0)

-- Switch to running
bestow.animation.play(player, "run", 0.3, true)

-- Check state
if bestow.animation.isPlaying(player) then
    local t = bestow.animation.getNormalizedTime(player)
    local name = bestow.animation.getCurrentClipName(player)
    print("Playing: " .. name .. " at " .. t)
end

-- Subscribe to completion
local subId = bestow.animation.onAnimationComplete(function(entity, clipName)
    if clipName == "attack" then
        bestow.animation.play(entity, "idle")
    end
end)

-- Low-level: Blend trees and IK
bestow.animation.core.addIKChain(animator, "leftHand", startBone, endBone, 10)
bestow.animation.core.setIKTarget(animator, "leftHand", {2.0, 1.5, 0.0}, 0.8)

-- Create a socket for weapon attachment
bestow.animation.core.createSocket(animator, "weaponSocket", rightHandBone, {0, 0.1, 0})
local socketTransform = bestow.animation.core.getSocketTransform(animator, "weaponSocket")

-- State machine
local IDLE, RUN, JUMP, ATTACK = 1, 2, 3, 4

local def = bestow.animation.fsm.builder("PlayerFSM")
    :addState(IDLE, "idle", "idle_clip", "Loop", 0.2)
    :addState(RUN, "run", "run_clip", "Loop", 0.3)
    :addState(JUMP, "jump", "jump_clip", "Once", 0.15)
    :addState(ATTACK, "attack", "slash_clip", "Once", 0.1)
    :setDefaultState(IDLE)
    :addParameter("speed", "Float", 0.0)
    :addParameter("grounded", "Bool", true)
    :addParameter("attack", "Trigger")
    :addTransition(IDLE, RUN, {bestow.animation.fsm.paramGreater("speed", 0.1)})
    :addTransition(RUN, IDLE, {bestow.animation.fsm.paramLess("speed", 0.1)})
    :addTransition(0, JUMP, {bestow.animation.fsm.paramEquals("grounded", false)})
    :addTransition(JUMP, IDLE, {bestow.animation.fsm.onAnimationEnd()})
    :addTransition(0, ATTACK, {bestow.animation.fsm.onTrigger("attack")}, 0.1, 10)
    :addTransition(ATTACK, IDLE, {bestow.animation.fsm.onAnimationEnd()})
    :build()

local fsm = bestow.animation.fsm.create(animSystem, animator, def)

-- In update loop
fsm:setFloat("speed", currentSpeed)
fsm:setBool("grounded", onGround)
fsm:update(dt)

-- Trigger attack
fsm:setTrigger("attack")
```

## C++ Examples

```cpp
// High-level: Entity-centric animation
auto result = animation->setupFromModel(player, "models/hero.glb");
if (!result) { spdlog::error("{}", result.error().message); return; }

animation->play(player, "idle");
animation->setSpeed(player, 1.0f);

auto subId = animation->onAnimationComplete(
    [&](Entity e, std::string_view clip) {
        if (clip == "death") animation->play(e, "death_idle", 0.5f, false);
    });

// Low-level: Full skeleton + clip pipeline
auto skeleton = animCore->createSkeleton(modelData);
auto clip = animCore->createClip("walk", modelData.animations[0], *skeleton);
auto animator = animCore->createAnimator(playerEntity, *skeleton);

animCore->play(*animator, *clip, 0.25f, true);

// Crossfade to a new clip
animCore->crossfade(*animator, runClip, 0.3f);

// Multi-layer blending (upper body override)
animCore->setLayerWeight(*animator, 1, 0.8f);
animCore->setLayerMask(*animator, 1, upperBodyBones);

// IK for foot placement
animCore->addIKChain(*animator, "leftFoot", hipBone, leftFootBone, 10);
animCore->setIKTarget(*animator, "leftFoot", groundContactPoint, 1.0f);

// Socket for weapon
animCore->createSocket(*animator, "sword", rightHandBone, {0, 0.05f, 0});
auto swordXform = animCore->getSocketTransform(*animator, "sword");

// Ragdoll on death
animCore->enableRagdoll(*animator);

// Root motion
animCore->enableRootMotion(*animator, true);
Vec3 delta = animCore->getRootMotionDelta(*animator);
Quat rotDelta = animCore->getRootMotionRotationDelta(*animator);

// State machine builder
constexpr AnimStateId IDLE = 1, RUN = 2, JUMP = 3;

auto def = AnimationStateMachineBuilder("PlayerFSM")
    .addState(IDLE, "idle", "idle_clip", AnimationWrapMode::Loop, 0.2f)
    .addState(RUN, "run", "run_clip", AnimationWrapMode::Loop, 0.3f)
    .addState(JUMP, "jump", "jump_clip", AnimationWrapMode::Once, 0.15f)
    .setDefaultState(IDLE)
    .addParameter("speed", AnimParamType::Float, 0.0f)
    .addParameter("grounded", AnimParamType::Bool, true)
    .addTransition(IDLE, RUN, {paramGreater("speed", 0.1f)})
    .addTransition(RUN, IDLE, {paramLess("speed", 0.1f)})
    .addTransition(InvalidAnimState, JUMP, {paramEquals("grounded", false)})
    .addTransition(JUMP, IDLE, {onAnimationEnd()})
    .build();

auto fsm = createAnimationStateMachine(animation, animator, std::move(def));

// In update loop
fsm->setFloat("speed", playerSpeed);
fsm->setBool("grounded", isOnGround);
fsm->update(dt);
```

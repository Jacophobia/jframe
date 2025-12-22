// bestow-contract/src/bestow.animation.statemachine.cppm
// Animation State Machine Interface
//
// Provides a state machine layer on top of IAnimationSystem for managing
// animation states, transitions, and blending. The state machine handles
// transition evaluation and blending automatically.

module;

#include <functional>
#include <optional>
#include <vector>
#include <span>
#include <string>
#include <string_view>
#include <cstdint>
#include <variant>
#include <memory>
#include <unordered_map>

export module bestow.animation.statemachine;

import bestow.types;
import bestow.animation;

export namespace bestow {

//==========================================================================
// Handle Types
//==========================================================================

/// Unique identifier for animation states within a state machine
using AnimStateId = std::uint32_t;

/// Invalid state constant
inline constexpr AnimStateId InvalidAnimState = 0;

//==========================================================================
// Parameter Types
//==========================================================================

/// Types of parameters that can control transitions
enum class AnimParamType : std::uint8_t {
    Bool,
    Int,
    Float,
    Trigger     // Auto-resets to false after consumption
};

/// Parameter value variant
using AnimParamValue = std::variant<bool, int, float>;

/// Parameter definition
struct AnimParameterDef {
    std::string name;
    AnimParamType type = AnimParamType::Bool;
    AnimParamValue defaultValue = false;
};

//==========================================================================
// Condition Types
//==========================================================================

/// Types of transition conditions
enum class TransitionConditionType : std::uint8_t {
    Immediate,          // Always true (use for trigger-based or unconditional)
    OnAnimationEnd,     // Wait for current animation to finish (non-looping)
    AfterTime,          // After specified time in current state
    OnParameter         // When a parameter condition is met
};

/// Comparison operators for parameter conditions
enum class ParameterComparison : std::uint8_t {
    Equal,
    NotEqual,
    Greater,
    Less,
    GreaterOrEqual,
    LessOrEqual
};

/// A single condition for a transition
struct TransitionCondition {
    TransitionConditionType type = TransitionConditionType::Immediate;

    // For AfterTime
    float timeThreshold = 0.0f;

    // For OnParameter
    std::string parameterName;
    ParameterComparison comparison = ParameterComparison::Equal;
    AnimParamValue parameterValue = false;
};

//==========================================================================
// State Configuration
//==========================================================================

/// Defines how an animation state behaves
struct AnimationStateConfig {
    AnimStateId id = InvalidAnimState;
    std::string name;
    AnimationClipHandle clip = AnimationHandles::InvalidClip;
    std::string clipName;               // Alternative: look up by name

    // Playback behavior
    AnimationWrapMode wrapMode = AnimationWrapMode::Loop;
    float speed = 1.0f;                 // Playback speed multiplier

    // Transition defaults
    float defaultBlendTime = 0.25f;     // Default transition INTO this state

    // Flags
    bool interruptible = true;          // Can be interrupted mid-playback
};

//==========================================================================
// Transition Configuration
//==========================================================================

/// Defines a transition between two states
struct AnimationTransitionDef {
    AnimStateId fromState = InvalidAnimState;   // Source state (InvalidAnimState = any state)
    AnimStateId toState = InvalidAnimState;     // Target state

    std::vector<TransitionCondition> conditions;// All must be true (AND logic)

    float blendTime = 0.25f;                    // Transition blend time
    bool hasBlendTimeOverride = false;          // Whether blendTime was explicitly set

    int priority = 0;                           // Higher = checked first
};

//==========================================================================
// State Machine Definition
//==========================================================================

/// Complete state machine definition
struct AnimationStateMachineDef {
    std::string name;

    // States
    std::vector<AnimationStateConfig> states;
    AnimStateId defaultState = InvalidAnimState;

    // Transitions
    std::vector<AnimationTransitionDef> transitions;

    // Parameters
    std::vector<AnimParameterDef> parameters;
};

//==========================================================================
// Runtime State
//==========================================================================

/// Current state of an animation state machine instance
struct AnimStateMachineState {
    AnimStateId currentState = InvalidAnimState;
    AnimStateId previousState = InvalidAnimState;
    AnimStateId targetState = InvalidAnimState;     // During transitions

    float stateTime = 0.0f;                         // Time in current state (seconds)
    float normalizedTime = 0.0f;                    // [0-1] progress through animation

    bool inTransition = false;
    bool animationComplete = false;                 // For non-looping states
};

//==========================================================================
// Callbacks
//==========================================================================

/// Called when entering a state
using StateEnterCallback = std::function<void(AnimStateId state, AnimStateId fromState)>;

/// Called when exiting a state
using StateExitCallback = std::function<void(AnimStateId state, AnimStateId toState)>;

/// Called when a transition starts
using TransitionCallback = std::function<void(AnimStateId from, AnimStateId to, float blendTime)>;

//==========================================================================
// IAnimationStateMachine Interface
//==========================================================================

/// Interface for animation state machine instance
class IAnimationStateMachine {
public:
    virtual ~IAnimationStateMachine() = default;

    //======================================================================
    // Lifecycle
    //======================================================================

    /// Update state machine (call each frame)
    virtual void update(DeltaTime dt) = 0;

    /// Reset to default state
    virtual void reset() = 0;

    //======================================================================
    // State Queries
    //======================================================================

    /// Get current runtime state
    virtual AnimStateMachineState getState() const = 0;

    /// Get current state ID
    virtual AnimStateId getCurrentStateId() const = 0;

    /// Get current state name
    virtual std::string_view getCurrentStateName() const = 0;

    /// Get state config by ID
    virtual const AnimationStateConfig* getStateConfig(AnimStateId id) const = 0;

    /// Get state ID by name
    virtual AnimStateId findState(std::string_view name) const = 0;

    /// Check if currently in a specific state
    virtual bool isInState(AnimStateId id) const = 0;
    virtual bool isInState(std::string_view name) const = 0;

    /// Check if currently transitioning
    virtual bool isTransitioning() const = 0;

    /// Get time in current state (seconds)
    virtual float getStateTime() const = 0;

    /// Get normalized animation time [0-1]
    virtual float getNormalizedTime() const = 0;

    //======================================================================
    // Manual State Control
    //======================================================================

    /// Force immediate transition to state (bypasses conditions)
    virtual void forceState(AnimStateId state, float blendTime = -1.0f) = 0;
    virtual void forceState(std::string_view stateName, float blendTime = -1.0f) = 0;

    //======================================================================
    // Parameter Control
    //======================================================================

    /// Set boolean parameter
    virtual void setBool(std::string_view name, bool value) = 0;
    virtual bool getBool(std::string_view name) const = 0;

    /// Set integer parameter
    virtual void setInt(std::string_view name, int value) = 0;
    virtual int getInt(std::string_view name) const = 0;

    /// Set float parameter
    virtual void setFloat(std::string_view name, float value) = 0;
    virtual float getFloat(std::string_view name) const = 0;

    /// Set trigger (auto-resets after consumed by a transition)
    virtual void setTrigger(std::string_view name) = 0;
    virtual void resetTrigger(std::string_view name) = 0;

    //======================================================================
    // Animation Access
    //======================================================================

    /// Get associated animator handle
    virtual AnimatorHandle getAnimator() const = 0;

    //======================================================================
    // Callbacks
    //======================================================================

    virtual void setOnStateEnter(StateEnterCallback callback) = 0;
    virtual void setOnStateExit(StateExitCallback callback) = 0;
    virtual void setOnTransition(TransitionCallback callback) = 0;
};

//==========================================================================
// Builder Pattern for State Machine Definition
//==========================================================================

/// Fluent builder for creating state machine definitions
class AnimationStateMachineBuilder {
public:
    AnimationStateMachineBuilder(std::string name) {
        def_.name = std::move(name);
    }

    /// Add a state to the state machine
    AnimationStateMachineBuilder& addState(AnimStateId id, std::string name,
                                           std::string clipName,
                                           AnimationWrapMode wrapMode = AnimationWrapMode::Loop,
                                           float blendTime = 0.25f) {
        AnimationStateConfig state;
        state.id = id;
        state.name = std::move(name);
        state.clipName = std::move(clipName);
        state.wrapMode = wrapMode;
        state.defaultBlendTime = blendTime;
        def_.states.push_back(std::move(state));
        return *this;
    }

    /// Set the default/initial state
    AnimationStateMachineBuilder& setDefaultState(AnimStateId id) {
        def_.defaultState = id;
        return *this;
    }

    /// Add a transition between states
    AnimationStateMachineBuilder& addTransition(AnimStateId from, AnimStateId to,
                                                 std::vector<TransitionCondition> conditions = {},
                                                 float blendTime = -1.0f,
                                                 int priority = 0) {
        AnimationTransitionDef trans;
        trans.fromState = from;
        trans.toState = to;
        trans.conditions = std::move(conditions);
        trans.priority = priority;
        if (blendTime >= 0.0f) {
            trans.blendTime = blendTime;
            trans.hasBlendTimeOverride = true;
        }
        def_.transitions.push_back(std::move(trans));
        return *this;
    }

    /// Add a parameter
    AnimationStateMachineBuilder& addParameter(std::string name, AnimParamType type,
                                                AnimParamValue defaultValue = false) {
        AnimParameterDef param;
        param.name = std::move(name);
        param.type = type;
        param.defaultValue = defaultValue;
        def_.parameters.push_back(std::move(param));
        return *this;
    }

    /// Build the definition
    AnimationStateMachineDef build() {
        return std::move(def_);
    }

private:
    AnimationStateMachineDef def_;
};

//==========================================================================
// Helper Functions for Creating Conditions
//==========================================================================

/// Create an "on animation end" condition
inline TransitionCondition onAnimationEnd() {
    return TransitionCondition{.type = TransitionConditionType::OnAnimationEnd};
}

/// Create an "after time" condition
inline TransitionCondition afterTime(float seconds) {
    return TransitionCondition{
        .type = TransitionConditionType::AfterTime,
        .timeThreshold = seconds
    };
}

/// Create a parameter condition (float greater than)
inline TransitionCondition paramGreater(std::string name, float value) {
    return TransitionCondition{
        .type = TransitionConditionType::OnParameter,
        .parameterName = std::move(name),
        .comparison = ParameterComparison::Greater,
        .parameterValue = value
    };
}

/// Create a parameter condition (float less than)
inline TransitionCondition paramLess(std::string name, float value) {
    return TransitionCondition{
        .type = TransitionConditionType::OnParameter,
        .parameterName = std::move(name),
        .comparison = ParameterComparison::Less,
        .parameterValue = value
    };
}

/// Create a parameter condition (bool equals)
inline TransitionCondition paramEquals(std::string name, bool value) {
    return TransitionCondition{
        .type = TransitionConditionType::OnParameter,
        .parameterName = std::move(name),
        .comparison = ParameterComparison::Equal,
        .parameterValue = value
    };
}

/// Create a trigger condition (trigger is set)
inline TransitionCondition onTrigger(std::string name) {
    return TransitionCondition{
        .type = TransitionConditionType::OnParameter,
        .parameterName = std::move(name),
        .comparison = ParameterComparison::Equal,
        .parameterValue = true
    };
}

//==========================================================================
// Factory Function (declared here, defined in implementation)
//==========================================================================

/// Create an animation state machine instance
/// Note: IAnimationSystem is imported from bestow.animation module
std::unique_ptr<IAnimationStateMachine> createAnimationStateMachine(
    IAnimationSystem* animSystem,
    AnimatorHandle animator,
    AnimationStateMachineDef definition);

}  // namespace bestow

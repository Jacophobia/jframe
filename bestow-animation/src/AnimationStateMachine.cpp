// bestow-animation/src/AnimationStateMachine.cpp
// Animation State Machine Implementation
//
// This is a module implementation unit for bestow.animation.statemachine.
// The factory function createAnimationStateMachine is declared in the
// module interface and defined here.

module;

#include <algorithm>
#include <unordered_map>

module bestow.animation.statemachine;

import std;
import bestow.animation;

namespace bestow {

//==========================================================================
// AnimationStateMachine Implementation
//==========================================================================

class AnimationStateMachineImpl : public IAnimationStateMachine {
public:
    AnimationStateMachineImpl(IAnimationSystem* animSystem,
                              AnimatorHandle animator,
                              AnimationStateMachineDef definition)
        : animation_(animSystem)
        , animator_(animator)
        , definition_(std::move(definition))
    {
        // Build lookup tables
        for (std::size_t i = 0; i < definition_.states.size(); ++i) {
            const auto& state = definition_.states[i];
            stateIdToIndex_[state.id] = i;
            stateNameToId_[state.name] = state.id;
        }

        // Organize transitions by source state and sort by priority
        for (const auto& trans : definition_.transitions) {
            if (trans.fromState == InvalidAnimState) {
                anyStateTransitions_.push_back(&trans);
            } else {
                transitionsFrom_[trans.fromState].push_back(&trans);
            }
        }

        // Sort transitions by priority (descending)
        auto sortByPriority = [](const AnimationTransitionDef* a, const AnimationTransitionDef* b) {
            return a->priority > b->priority;
        };
        std::sort(anyStateTransitions_.begin(), anyStateTransitions_.end(), sortByPriority);
        for (auto& [stateId, transitions] : transitionsFrom_) {
            std::sort(transitions.begin(), transitions.end(), sortByPriority);
        }

        // Initialize parameters
        for (const auto& paramDef : definition_.parameters) {
            parameters_[paramDef.name] = {paramDef.type, paramDef.defaultValue, false};
        }

        // Subscribe to animation completion
        // Only set animationComplete when the EXPECTED clip completes (not during crossfades)
        if (animation_) {
            completeSubscription_ = animation_->subscribeToComplete(
                animator_,
                [this](AnimatorHandle, AnimationClipHandle clip, std::uint32_t) {
                    // Only mark complete if this is the clip we're expecting
                    if (clip == expectedCompleteClip_) {
                        state_.animationComplete = true;
                    }
                });
        }

        // Start in default state
        if (definition_.defaultState != InvalidAnimState) {
            enterState(definition_.defaultState, 0.0f);
        }
    }

    ~AnimationStateMachineImpl() override {
        if (animation_ && completeSubscription_ != 0) {
            animation_->unsubscribe(completeSubscription_);
        }
    }

    //======================================================================
    // Lifecycle
    //======================================================================

    void update(DeltaTime dt) override {
        if (state_.currentState == InvalidAnimState) return;

        // Update timers
        state_.stateTime += dt;

        // Get animation state from animator
        if (animation_ && animator_ != AnimationHandles::InvalidAnimator) {
            auto layerState = animation_->getLayerState(animator_, 0);
            state_.normalizedTime = layerState.normalizedTime;

            // Check if transition completed (fadeWeight reached 1.0)
            if (state_.inTransition && layerState.fadeWeight >= 0.999f) {
                completeTransition();
            }
        }

        // Check for valid transitions (if not already transitioning)
        if (!state_.inTransition) {
            evaluateTransitions();
        }

        // Reset consumed triggers at end of frame
        for (auto& [name, param] : parameters_) {
            if (param.type == AnimParamType::Trigger && param.consumed) {
                param.value = false;
                param.consumed = false;
            }
        }
    }

    void reset() override {
        state_ = AnimStateMachineState{};

        // Reset parameters to defaults
        for (const auto& paramDef : definition_.parameters) {
            parameters_[paramDef.name] = {paramDef.type, paramDef.defaultValue, false};
        }

        // Enter default state
        if (definition_.defaultState != InvalidAnimState) {
            enterState(definition_.defaultState, 0.0f);
        }
    }

    //======================================================================
    // State Queries
    //======================================================================

    AnimStateMachineState getState() const override {
        return state_;
    }

    AnimStateId getCurrentStateId() const override {
        return state_.currentState;
    }

    std::string_view getCurrentStateName() const override {
        if (auto* config = getStateConfig(state_.currentState)) {
            return config->name;
        }
        return "";
    }

    const AnimationStateConfig* getStateConfig(AnimStateId id) const override {
        auto it = stateIdToIndex_.find(id);
        if (it != stateIdToIndex_.end() && it->second < definition_.states.size()) {
            return &definition_.states[it->second];
        }
        return nullptr;
    }

    AnimStateId findState(std::string_view name) const override {
        auto it = stateNameToId_.find(std::string(name));
        if (it != stateNameToId_.end()) {
            return it->second;
        }
        return InvalidAnimState;
    }

    bool isInState(AnimStateId id) const override {
        return state_.currentState == id && !state_.inTransition;
    }

    bool isInState(std::string_view name) const override {
        return isInState(findState(name));
    }

    bool isTransitioning() const override {
        return state_.inTransition;
    }

    float getStateTime() const override {
        return state_.stateTime;
    }

    float getNormalizedTime() const override {
        return state_.normalizedTime;
    }

    //======================================================================
    // Manual State Control
    //======================================================================

    void forceState(AnimStateId stateId, float blendTime) override {
        if (stateId == InvalidAnimState) return;
        auto* config = getStateConfig(stateId);
        if (!config) return;

        float actualBlendTime = (blendTime >= 0.0f) ? blendTime : config->defaultBlendTime;
        startTransition(stateId, actualBlendTime);
    }

    void forceState(std::string_view stateName, float blendTime) override {
        forceState(findState(stateName), blendTime);
    }

    //======================================================================
    // Parameter Control
    //======================================================================

    void setBool(std::string_view name, bool value) override {
        auto it = parameters_.find(std::string(name));
        if (it != parameters_.end() && it->second.type == AnimParamType::Bool) {
            it->second.value = value;
        }
    }

    bool getBool(std::string_view name) const override {
        auto it = parameters_.find(std::string(name));
        if (it != parameters_.end()) {
            if (auto* val = std::get_if<bool>(&it->second.value)) {
                return *val;
            }
        }
        return false;
    }

    void setInt(std::string_view name, int value) override {
        auto it = parameters_.find(std::string(name));
        if (it != parameters_.end() && it->second.type == AnimParamType::Int) {
            it->second.value = value;
        }
    }

    int getInt(std::string_view name) const override {
        auto it = parameters_.find(std::string(name));
        if (it != parameters_.end()) {
            if (auto* val = std::get_if<int>(&it->second.value)) {
                return *val;
            }
        }
        return 0;
    }

    void setFloat(std::string_view name, float value) override {
        auto it = parameters_.find(std::string(name));
        if (it != parameters_.end() && it->second.type == AnimParamType::Float) {
            it->second.value = value;
        }
    }

    float getFloat(std::string_view name) const override {
        auto it = parameters_.find(std::string(name));
        if (it != parameters_.end()) {
            if (auto* val = std::get_if<float>(&it->second.value)) {
                return *val;
            }
        }
        return 0.0f;
    }

    void setTrigger(std::string_view name) override {
        auto it = parameters_.find(std::string(name));
        if (it != parameters_.end() && it->second.type == AnimParamType::Trigger) {
            it->second.value = true;
            it->second.consumed = false;
        }
    }

    void resetTrigger(std::string_view name) override {
        auto it = parameters_.find(std::string(name));
        if (it != parameters_.end() && it->second.type == AnimParamType::Trigger) {
            it->second.value = false;
            it->second.consumed = false;
        }
    }

    //======================================================================
    // Animation Access
    //======================================================================

    AnimatorHandle getAnimator() const override {
        return animator_;
    }

    //======================================================================
    // Callbacks
    //======================================================================

    void setOnStateEnter(StateEnterCallback callback) override {
        onEnter_ = std::move(callback);
    }

    void setOnStateExit(StateExitCallback callback) override {
        onExit_ = std::move(callback);
    }

    void setOnTransition(TransitionCallback callback) override {
        onTransition_ = std::move(callback);
    }

private:
    //======================================================================
    // Internal Parameter Structure
    //======================================================================

    struct ParameterState {
        AnimParamType type = AnimParamType::Bool;
        AnimParamValue value = false;
        bool consumed = false;  // For triggers
    };

    //======================================================================
    // Transition Logic
    //======================================================================

    void evaluateTransitions() {
        // Check any-state transitions first (highest priority)
        for (const auto* transition : anyStateTransitions_) {
            if (canTransition(*transition)) {
                float blendTime = getTransitionBlendTime(*transition);
                startTransition(transition->toState, blendTime);
                consumeTriggersForTransition(*transition);
                return;
            }
        }

        // Check transitions from current state
        auto it = transitionsFrom_.find(state_.currentState);
        if (it != transitionsFrom_.end()) {
            for (const auto* transition : it->second) {
                if (canTransition(*transition)) {
                    float blendTime = getTransitionBlendTime(*transition);
                    startTransition(transition->toState, blendTime);
                    consumeTriggersForTransition(*transition);
                    return;
                }
            }
        }
    }

    bool canTransition(const AnimationTransitionDef& transition) {
        // Check if target state exists and is different (or allows self-transition)
        auto* targetConfig = getStateConfig(transition.toState);
        if (!targetConfig) return false;

        // Check all conditions (AND logic)
        for (const auto& condition : transition.conditions) {
            if (!evaluateCondition(condition)) {
                return false;
            }
        }

        return true;
    }

    bool evaluateCondition(const TransitionCondition& condition) {
        switch (condition.type) {
            case TransitionConditionType::Immediate:
                return true;

            case TransitionConditionType::OnAnimationEnd:
                return state_.animationComplete;

            case TransitionConditionType::AfterTime:
                return state_.stateTime >= condition.timeThreshold;

            case TransitionConditionType::OnParameter:
                return evaluateParameterCondition(condition);
        }
        return false;
    }

    bool evaluateParameterCondition(const TransitionCondition& condition) {
        auto it = parameters_.find(condition.parameterName);
        if (it == parameters_.end()) return false;

        const auto& param = it->second;

        // Handle different parameter types
        if (auto* boolVal = std::get_if<bool>(&param.value)) {
            auto* condVal = std::get_if<bool>(&condition.parameterValue);
            if (!condVal) return false;

            switch (condition.comparison) {
                case ParameterComparison::Equal: return *boolVal == *condVal;
                case ParameterComparison::NotEqual: return *boolVal != *condVal;
                default: return false;
            }
        }
        else if (auto* intVal = std::get_if<int>(&param.value)) {
            auto* condVal = std::get_if<int>(&condition.parameterValue);
            if (!condVal) return false;

            switch (condition.comparison) {
                case ParameterComparison::Equal: return *intVal == *condVal;
                case ParameterComparison::NotEqual: return *intVal != *condVal;
                case ParameterComparison::Greater: return *intVal > *condVal;
                case ParameterComparison::Less: return *intVal < *condVal;
                case ParameterComparison::GreaterOrEqual: return *intVal >= *condVal;
                case ParameterComparison::LessOrEqual: return *intVal <= *condVal;
            }
        }
        else if (auto* floatVal = std::get_if<float>(&param.value)) {
            // For float conditions, also try int comparison value
            float condFloat = 0.0f;
            if (auto* fv = std::get_if<float>(&condition.parameterValue)) {
                condFloat = *fv;
            } else if (auto* iv = std::get_if<int>(&condition.parameterValue)) {
                condFloat = static_cast<float>(*iv);
            } else {
                return false;
            }

            switch (condition.comparison) {
                case ParameterComparison::Equal: return std::abs(*floatVal - condFloat) < 0.0001f;
                case ParameterComparison::NotEqual: return std::abs(*floatVal - condFloat) >= 0.0001f;
                case ParameterComparison::Greater: return *floatVal > condFloat;
                case ParameterComparison::Less: return *floatVal < condFloat;
                case ParameterComparison::GreaterOrEqual: return *floatVal >= condFloat;
                case ParameterComparison::LessOrEqual: return *floatVal <= condFloat;
            }
        }

        return false;
    }

    float getTransitionBlendTime(const AnimationTransitionDef& transition) {
        if (transition.hasBlendTimeOverride) {
            return transition.blendTime;
        }
        if (auto* config = getStateConfig(transition.toState)) {
            return config->defaultBlendTime;
        }
        return 0.25f;
    }

    void consumeTriggersForTransition(const AnimationTransitionDef& transition) {
        for (const auto& condition : transition.conditions) {
            if (condition.type == TransitionConditionType::OnParameter) {
                auto it = parameters_.find(condition.parameterName);
                if (it != parameters_.end() && it->second.type == AnimParamType::Trigger) {
                    it->second.consumed = true;
                }
            }
        }
    }

    void startTransition(AnimStateId targetState, float blendTime) {
        auto* targetConfig = getStateConfig(targetState);
        if (!targetConfig) return;

        // Call exit callback
        if (onExit_) {
            onExit_(state_.currentState, targetState);
        }

        // Start animation crossfade
        if (animation_) {
            AnimationPlayConfig playConfig;
            playConfig.clip = targetConfig->clip;
            playConfig.clipName = targetConfig->clipName;
            playConfig.speed = targetConfig->speed;
            playConfig.blendInTime = blendTime;
            playConfig.wrapMode = targetConfig->wrapMode;
            playConfig.layer = 0;

            animation_->play(animator_, playConfig);

            // Track which clip we expect to complete (for OnAnimationEnd transitions)
            expectedCompleteClip_ = targetConfig->clip;
        }

        // Update state
        state_.previousState = state_.currentState;
        state_.targetState = targetState;
        state_.inTransition = true;
        state_.animationComplete = false;

        if (onTransition_) {
            onTransition_(state_.currentState, targetState, blendTime);
        }
    }

    void enterState(AnimStateId stateId, float blendTime) {
        auto* config = getStateConfig(stateId);
        if (!config) return;

        // Play the animation
        if (animation_) {
            AnimationPlayConfig playConfig;
            playConfig.clip = config->clip;
            playConfig.clipName = config->clipName;
            playConfig.speed = config->speed;
            playConfig.blendInTime = blendTime;
            playConfig.wrapMode = config->wrapMode;
            playConfig.layer = 0;

            animation_->play(animator_, playConfig);

            // Track which clip we expect to complete (for OnAnimationEnd transitions)
            expectedCompleteClip_ = config->clip;
        }

        state_.currentState = stateId;
        state_.stateTime = 0.0f;
        state_.normalizedTime = 0.0f;
        state_.animationComplete = false;
        state_.inTransition = false;

        if (onEnter_) {
            onEnter_(stateId, state_.previousState);
        }
    }

    void completeTransition() {
        AnimStateId enteredState = state_.targetState;
        state_.currentState = state_.targetState;
        state_.targetState = InvalidAnimState;
        state_.inTransition = false;
        state_.stateTime = 0.0f;
        state_.animationComplete = false;

        if (onEnter_) {
            onEnter_(enteredState, state_.previousState);
        }
    }

    //======================================================================
    // Member Variables
    //======================================================================

    IAnimationSystem* animation_ = nullptr;
    AnimatorHandle animator_ = AnimationHandles::InvalidAnimator;
    AnimationStateMachineDef definition_;
    AnimStateMachineState state_;

    // Lookup tables
    std::unordered_map<AnimStateId, std::size_t> stateIdToIndex_;
    std::unordered_map<std::string, AnimStateId> stateNameToId_;

    // Sorted transitions by priority (per source state)
    std::unordered_map<AnimStateId, std::vector<const AnimationTransitionDef*>> transitionsFrom_;
    std::vector<const AnimationTransitionDef*> anyStateTransitions_;

    // Parameters
    std::unordered_map<std::string, ParameterState> parameters_;

    // Callbacks
    StateEnterCallback onEnter_;
    StateExitCallback onExit_;
    TransitionCallback onTransition_;

    // Subscriptions
    SubscriptionId completeSubscription_ = 0;

    // Track which clip should trigger animation completion
    AnimationClipHandle expectedCompleteClip_ = AnimationHandles::InvalidClip;
};

//==========================================================================
// Factory Function
//==========================================================================

std::unique_ptr<IAnimationStateMachine> createAnimationStateMachine(
    IAnimationSystem* animSystem,
    AnimatorHandle animator,
    AnimationStateMachineDef definition)
{
    return std::make_unique<AnimationStateMachineImpl>(
        animSystem, animator, std::move(definition));
}

}  // namespace bestow

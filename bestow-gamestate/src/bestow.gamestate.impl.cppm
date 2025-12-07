// bestow-gamestate/src/bestow.gamestate.impl.cppm
// Game State System implementation

module;

export module bestow.gamestate.impl;

import std;
import bestow.gamestate;
import bestow.types;

export namespace bestow {

//==========================================================================
// Stack-Based Game State System Implementation
//==========================================================================

class GameStateSystemImpl : public IGameStateSystem {
public:
    GameStateSystemImpl() = default;
    ~GameStateSystemImpl() override {
        clearStates();
    }

    //======================================================================
    // Stack Operations
    //======================================================================

    void pushState(
        std::unique_ptr<IGameState> state,
        StateTransition transition) override {

        if (!state) return;

        // Pause current state if exists
        if (!stateStack_.empty()) {
            stateStack_.back()->onPause();
        }

        // Enter new state
        IGameState* newState = state.get();
        stateStack_.push_back(std::move(state));
        newState->onEnter();

        // Notify callback
        if (stateChangeCallback_) {
            IGameState* oldState = stateStack_.size() > 1 ?
                stateStack_[stateStack_.size() - 2].get() : nullptr;
            stateChangeCallback_(oldState, newState);
        }

        // Handle transition
        if (transition.type != TransitionType::None) {
            startTransition(transition, false);
        }
    }

    void popState(StateTransition transition) override {
        if (stateStack_.empty()) return;

        // Start transition if needed
        if (transition.type != TransitionType::None) {
            startTransition(transition, true);
            pendingPop_ = true;
            return;
        }

        performPop();
    }

    void replaceState(
        std::unique_ptr<IGameState> state,
        StateTransition transition) override {

        if (!state) return;

        // Clear all states
        clearStates();

        // Push new state
        pushState(std::move(state), transition);
    }

    void popUntilPredicate(
        std::function<bool(const IGameState*)> predicate,
        StateTransition transition) override {

        while (!stateStack_.empty()) {
            if (predicate(stateStack_.back().get())) {
                // Found the target state, resume it
                stateStack_.back()->onResume();
                break;
            }
            performPop();
        }
    }

    void clearStates() override {
        while (!stateStack_.empty()) {
            stateStack_.back()->onExit();
            stateStack_.pop_back();
        }

        if (stateChangeCallback_) {
            stateChangeCallback_(nullptr, nullptr);
        }
    }

    //======================================================================
    // State Access
    //======================================================================

    IGameState* getCurrentState() override {
        return stateStack_.empty() ? nullptr : stateStack_.back().get();
    }

    const IGameState* getCurrentState() const override {
        return stateStack_.empty() ? nullptr : stateStack_.back().get();
    }

    IGameState* getStateAt(std::size_t depth) override {
        if (depth >= stateStack_.size()) return nullptr;
        return stateStack_[stateStack_.size() - 1 - depth].get();
    }

    std::size_t getStateCount() const override {
        return stateStack_.size();
    }

    bool isEmpty() const override {
        return stateStack_.empty();
    }

    bool isTransitioning() const override {
        return transitionActive_;
    }

    //======================================================================
    // Main Loop Integration
    //======================================================================

    void update(DeltaTime dt) override {
        if (stateStack_.empty()) return;

        // Update transition
        if (transitionActive_) {
            updateTransition(dt);
        }

        // Update states from bottom to top, respecting UpdateBelow flag
        bool shouldUpdate = true;

        for (auto it = stateStack_.rbegin(); it != stateStack_.rend(); ++it) {
            if (shouldUpdate) {
                (*it)->update(dt);
            }

            // Check if we should continue updating states below
            GameStateFlags flags = (*it)->getFlags();
            if (!hasFlag(flags, GameStateFlags::UpdateBelow)) {
                break;  // Don't update states below this one
            }
        }

        // Handle pending pop after transition completes
        if (pendingPop_ && !transitionActive_) {
            performPop();
            pendingPop_ = false;
        }
    }

    void render() override {
        if (stateStack_.empty()) return;

        // Find the lowest state that should be rendered
        std::size_t startIndex = stateStack_.size() - 1;

        for (std::size_t i = stateStack_.size(); i > 0; --i) {
            std::size_t idx = i - 1;
            GameStateFlags flags = stateStack_[idx]->getFlags();

            if (hasFlag(flags, GameStateFlags::RenderBelow)) {
                startIndex = idx > 0 ? idx - 1 : 0;
            } else {
                startIndex = idx;
                break;
            }
        }

        // Render from bottom to top
        for (std::size_t i = startIndex; i < stateStack_.size(); ++i) {
            stateStack_[i]->render();
        }

        // Render transition effect
        if (transitionActive_) {
            renderTransition();
        }
    }

    bool handleInput(const StateInputEvent& event) override {
        if (stateStack_.empty()) return false;

        // Process input from top to bottom, stopping when consumed or blocked
        for (auto it = stateStack_.rbegin(); it != stateStack_.rend(); ++it) {
            if ((*it)->handleInput(event)) {
                return true;  // Input was consumed
            }

            GameStateFlags flags = (*it)->getFlags();
            if (hasFlag(flags, GameStateFlags::BlockInput)) {
                return true;  // Input blocked from reaching states below
            }
        }

        return false;
    }

    //======================================================================
    // State Registration
    //======================================================================

    void registerStateFactory(
        const std::string& stateName,
        StateFactory factory) override {
        stateFactories_[stateName] = std::move(factory);
    }

    std::unique_ptr<IGameState> createState(const std::string& stateName) override {
        auto it = stateFactories_.find(stateName);
        if (it != stateFactories_.end()) {
            return it->second();
        }
        return nullptr;
    }

    //======================================================================
    // Callbacks
    //======================================================================

    void setStateChangeCallback(StateChangeCallback callback) override {
        stateChangeCallback_ = std::move(callback);
    }

private:
    std::vector<std::unique_ptr<IGameState>> stateStack_;
    std::unordered_map<std::string, StateFactory> stateFactories_;
    StateChangeCallback stateChangeCallback_;

    // Transition state
    bool transitionActive_ = false;
    bool pendingPop_ = false;
    StateTransition currentTransition_;
    float transitionProgress_ = 0.0f;
    bool transitionOut_ = false;

    void performPop() {
        if (stateStack_.empty()) return;

        IGameState* oldState = stateStack_.back().get();
        oldState->onExit();
        stateStack_.pop_back();

        // Resume state below if exists
        IGameState* newState = nullptr;
        if (!stateStack_.empty()) {
            newState = stateStack_.back().get();
            newState->onResume();
        }

        if (stateChangeCallback_) {
            stateChangeCallback_(oldState, newState);
        }
    }

    void startTransition(const StateTransition& transition, bool isOutTransition) {
        currentTransition_ = transition;
        transitionActive_ = true;
        transitionProgress_ = 0.0f;
        transitionOut_ = isOutTransition;
    }

    void updateTransition(DeltaTime dt) {
        if (currentTransition_.duration <= 0.0f) {
            transitionActive_ = false;
            return;
        }

        transitionProgress_ += dt / currentTransition_.duration;

        if (transitionProgress_ >= 1.0f) {
            transitionProgress_ = 1.0f;
            transitionActive_ = false;
        }
    }

    void renderTransition() {
        // Calculate alpha for fade transition
        float alpha = transitionOut_ ?
            transitionProgress_ :          // Fade out: 0 -> 1 (transparent to black)
            1.0f - transitionProgress_;    // Fade in: 1 -> 0 (black to transparent)

        // TODO: Render transition overlay
        // This would typically draw a full-screen quad with the appropriate effect
        // For now, this is a stub that integrates with the graphics system

        switch (currentTransition_.type) {
            case TransitionType::Fade:
                // Draw black overlay with alpha
                break;
            case TransitionType::Slide:
                // Apply slide transform to render
                break;
            case TransitionType::Custom:
                // Call custom transition renderer
                break;
            default:
                break;
        }
    }
};

//==========================================================================
// Factory Function
//==========================================================================

inline std::unique_ptr<IGameStateSystem> createGameStateSystem() {
    return std::make_unique<GameStateSystemImpl>();
}

}  // namespace bestow

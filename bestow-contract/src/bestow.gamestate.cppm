// bestow-contract/src/bestow.gamestate.cppm
// Game State System interface for managing game states (menus, gameplay, pause, etc.)

module;

#include <functional>
#include <optional>
#include <vector>
#include <memory>

export module bestow.gamestate;

import bestow.types;

export namespace bestow {

//==========================================================================
// State Flags (bitmask for state behavior)
//==========================================================================

enum class GameStateFlags : std::uint8_t {
    None           = 0,
    UpdateBelow    = 1 << 0,  // Continue updating states below this one
    RenderBelow    = 1 << 1,  // Continue rendering states below this one
    BlockInput     = 1 << 2,  // Block input from reaching states below
    Transparent    = 1 << 3,  // This state has transparent background (render below)
    Overlay        = UpdateBelow | RenderBelow | BlockInput,  // Typical overlay/pause menu
    Popup          = RenderBelow | BlockInput,  // Popup dialog (pause game, show behind)
};

constexpr GameStateFlags operator|(GameStateFlags a, GameStateFlags b) {
    return static_cast<GameStateFlags>(
        static_cast<std::uint8_t>(a) | static_cast<std::uint8_t>(b));
}

constexpr GameStateFlags operator&(GameStateFlags a, GameStateFlags b) {
    return static_cast<GameStateFlags>(
        static_cast<std::uint8_t>(a) & static_cast<std::uint8_t>(b));
}

constexpr bool hasFlag(GameStateFlags flags, GameStateFlags flag) {
    return (static_cast<std::uint8_t>(flags) & static_cast<std::uint8_t>(flag)) != 0;
}

//==========================================================================
// Input Event (simplified for state handling)
//==========================================================================

struct StateInputEvent {
    enum class Type : std::uint8_t {
        KeyPressed,
        KeyReleased,
        MousePressed,
        MouseReleased,
        MouseMoved,
        MouseScrolled,
        GamepadButton,
        GamepadAxis
    };

    Type type;
    int code = 0;       // Key code, mouse button, or gamepad button
    float value = 0.0f; // Axis value or scroll amount
    int x = 0;          // Mouse X
    int y = 0;          // Mouse Y
    int modifiers = 0;  // Shift, Ctrl, Alt
};

//==========================================================================
// State Transition Types
//==========================================================================

enum class TransitionType : std::uint8_t {
    None,       // Instant transition
    Fade,       // Fade out old, fade in new
    Slide,      // Slide transition
    Custom      // User-defined transition
};

struct StateTransition {
    TransitionType type = TransitionType::None;
    float duration = 0.3f;  // Transition duration in seconds

    static StateTransition instant() { return {TransitionType::None, 0.0f}; }
    static StateTransition fade(float duration = 0.3f) { return {TransitionType::Fade, duration}; }
    static StateTransition slide(float duration = 0.3f) { return {TransitionType::Slide, duration}; }
};

//==========================================================================
// IGameState Interface
//==========================================================================

class IGameState {
public:
    virtual ~IGameState() = default;

    //======================================================================
    // Lifecycle Callbacks
    //======================================================================

    /// Called when state is first pushed onto the stack
    virtual void onEnter() = 0;

    /// Called when state is popped from the stack
    virtual void onExit() = 0;

    /// Called when another state is pushed on top of this one
    virtual void onPause() {}

    /// Called when a state above this one is popped, and this becomes active again
    virtual void onResume() {}

    //======================================================================
    // Per-Frame Updates
    //======================================================================

    /// Update game logic
    /// @param dt Delta time since last frame
    virtual void update(DeltaTime dt) = 0;

    /// Render the state
    virtual void render() = 0;

    //======================================================================
    // Input Handling
    //======================================================================

    /// Handle input events
    /// @return true if the event was consumed (don't pass to states below)
    virtual bool handleInput(const StateInputEvent& event) { return false; }

    //======================================================================
    // State Properties
    //======================================================================

    /// Get the behavior flags for this state
    virtual GameStateFlags getFlags() const { return GameStateFlags::None; }

    /// Get the name of this state (for debugging)
    virtual std::string_view getName() const { return "unnamed"; }
};

//==========================================================================
// State Factory (for serialization/deserialization of states)
//==========================================================================

using StateFactory = std::function<std::unique_ptr<IGameState>()>;

//==========================================================================
// IGameStateSystem Interface
//==========================================================================

class IGameStateSystem {
public:
    virtual ~IGameStateSystem() = default;

    //======================================================================
    // Stack Operations
    //======================================================================

    /// Push a new state onto the stack
    /// The new state becomes the active state
    virtual void pushState(
        std::unique_ptr<IGameState> state,
        StateTransition transition = StateTransition::instant()) = 0;

    /// Pop the current state from the stack
    /// The state below becomes active (or empty if no states remain)
    virtual void popState(StateTransition transition = StateTransition::instant()) = 0;

    /// Pop all states and push a new one
    /// Useful for "quit to main menu" scenarios
    virtual void replaceState(
        std::unique_ptr<IGameState> state,
        StateTransition transition = StateTransition::instant()) = 0;

    /// Pop states until a specific state type is reached
    /// Useful for "return to game" from nested menus
    template<typename T>
    void popUntil(StateTransition transition = StateTransition::instant()) {
        popUntilPredicate([](const IGameState* s) {
            return dynamic_cast<const T*>(s) != nullptr;
        }, transition);
    }

    /// Pop states until predicate returns true
    virtual void popUntilPredicate(
        std::function<bool(const IGameState*)> predicate,
        StateTransition transition = StateTransition::instant()) = 0;

    /// Clear all states from the stack
    virtual void clearStates() = 0;

    //======================================================================
    // State Access
    //======================================================================

    /// Get the current (topmost) active state
    virtual IGameState* getCurrentState() = 0;
    virtual const IGameState* getCurrentState() const = 0;

    /// Get state at a specific depth (0 = top, 1 = below top, etc.)
    virtual IGameState* getStateAt(std::size_t depth) = 0;

    /// Get the number of states on the stack
    virtual std::size_t getStateCount() const = 0;

    /// Check if the stack is empty
    virtual bool isEmpty() const = 0;

    /// Check if a transition is currently in progress
    virtual bool isTransitioning() const = 0;

    //======================================================================
    // Main Loop Integration
    //======================================================================

    /// Update all active states (respecting UpdateBelow flags)
    virtual void update(DeltaTime dt) = 0;

    /// Render all visible states (respecting RenderBelow flags)
    virtual void render() = 0;

    /// Process input through the state stack
    /// @return true if input was consumed by any state
    virtual bool handleInput(const StateInputEvent& event) = 0;

    //======================================================================
    // State Registration (for dynamic state creation)
    //======================================================================

    /// Register a state factory by name
    virtual void registerStateFactory(
        const std::string& stateName,
        StateFactory factory) = 0;

    /// Create a state by registered name
    virtual std::unique_ptr<IGameState> createState(const std::string& stateName) = 0;

    //======================================================================
    // Callbacks
    //======================================================================

    using StateChangeCallback = std::function<void(IGameState* oldState, IGameState* newState)>;

    /// Register callback for when active state changes
    virtual void setStateChangeCallback(StateChangeCallback callback) = 0;
};

//==========================================================================
// Common State Base Classes (optional helpers)
//==========================================================================

/// Base class for simple states with common defaults
class SimpleGameState : public IGameState {
public:
    void onEnter() override {}
    void onExit() override {}
    void update(DeltaTime dt) override {}
    void render() override {}
};

/// Base class for overlay states (pause menus, dialogs)
class OverlayState : public IGameState {
public:
    GameStateFlags getFlags() const override {
        return GameStateFlags::Overlay;
    }
};

/// Base class for popup dialogs (blocks input, renders behind, pauses game)
class PopupState : public IGameState {
public:
    GameStateFlags getFlags() const override {
        return GameStateFlags::Popup;
    }
};

}  // namespace bestow

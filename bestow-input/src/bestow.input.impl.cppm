// bestow-input/src/bestow.input.impl.cppm
// Input system implementation using GLFW and SDL2
// Supports event-driven input with phases and action mappings

module;

#include <kangaru/kangaru.hpp>
#include <bestow/kangaru_macros.hpp>
#include <GLFW/glfw3.h>
#include <SDL.h>

export module bestow.input.impl;

import std;
import bestow.services;  // Re-exports all contracts including IInputSystem, IEventSystem, IAssetSystem

export namespace bestow {

//==========================================================================
// Platform Mapping Utilities (KeyCode <-> GLFW, GamepadButton <-> SDL, etc.)
//==========================================================================

/// Convert platform-agnostic KeyCode to GLFW key code
int keyCodeToGLFW(KeyCode key);

/// Convert GLFW key code to platform-agnostic KeyCode
KeyCode glfwToKeyCode(int glfwKey);

/// Convert platform-agnostic GamepadButton to SDL controller button
SDL_GameControllerButton gamepadButtonToSDL(GamepadButton button);

/// Convert SDL controller button to platform-agnostic GamepadButton
GamepadButton sdlToGamepadButton(SDL_GameControllerButton button);

/// Convert platform-agnostic GamepadAxis to SDL controller axis
SDL_GameControllerAxis gamepadAxisToSDL(GamepadAxis axis);

/// Convert SDL controller axis to platform-agnostic GamepadAxis
GamepadAxis sdlToGamepadAxis(SDL_GameControllerAxis axis);

/// Convert platform-agnostic MouseButton to GLFW mouse button
int mouseButtonToGLFW(MouseButton button);

/// Convert GLFW mouse button to platform-agnostic MouseButton
MouseButton glfwToMouseButton(int glfwButton);

//==========================================================================
// PhaseTree - Utility for phase hierarchy operations
//==========================================================================

namespace PhaseTree {
    /// Check if childPhase is the same as or a descendant of parentPhase
    /// e.g., "game.melee.combo" is descendant of "game" and "game.melee"
    bool isDescendantOrSame(std::string_view childPhase, std::string_view parentPhase);

    /// Get the parent phase (e.g., "game.melee" -> "game", "game" -> "")
    std::string getParent(std::string_view phase);

    /// Get all ancestors including self (e.g., "game.melee.combo" -> ["game.melee.combo", "game.melee", "game"])
    std::vector<std::string> getAncestors(std::string_view phase);

    /// Validate phase string format (non-empty, valid characters)
    bool isValidPhase(std::string_view phase);
}

//==========================================================================
// InputSystem Implementation
//==========================================================================

class InputSystem : public IInputSystem {
public:
    explicit InputSystem(IEventSystem* pIEventSystem = nullptr, IAssetSystem* pIAssetSystem = nullptr)
        : pIEventSystem_(pIEventSystem), pIAssetSystem_(pIAssetSystem) {}
    ~InputSystem() override;

    //======================================================================
    // Lifecycle
    //======================================================================

    bool initialize(void* nativeWindow) override;
    void shutdown() override;
    void update() override;

    //======================================================================
    // Phase Management (Event-Driven Input System)
    //======================================================================

    std::string getCurrentPhase() const override;
    std::vector<std::string> getPhaseStack() const override;
    void pushPhase(const std::string& phase) override;
    void popPhase() override;
    void changePhase(const std::string& phase) override;
    bool isPhaseActive(const std::string& phase) const override;
    bool hasPhaseBeenSet() const override;

    //======================================================================
    // Action Registration (Event-Driven Input System)
    //======================================================================

    void registerAction(const ActionRegistration& registration) override;
    void unregisterAction(const std::string& actionName) override;
    void unregisterPhaseActions(const std::string& phase) override;
    void clearActions() override;
    std::vector<ActionRegistration> getActions() const override;

    //======================================================================
    // Input State Queries (Platform-Agnostic)
    //======================================================================

    InputState getInputState(const InputBinding& binding) const override;
    float getInputHoldDuration(const InputBinding& binding) const override;
    void setDefaultHoldThreshold(float seconds) override;
    float getDefaultHoldThreshold() const override;

    //======================================================================
    // Configuration Loading
    //======================================================================

    bool loadInputConfig(const std::string& path) override;
    bool reloadInputConfig() override;

    //======================================================================
    // Legacy Mapping Management (Deprecated)
    //======================================================================

    void registerMapping(const InputMapping& mapping) override;
    void removeMapping(const InputBinding& binding) override;
    void clearMappings() override;
    std::vector<InputMapping> getMappings() const override;

    //======================================================================
    // Legacy Action State Queries (Deprecated)
    //======================================================================

    ActionState getActionState(const Action& action) const override;
    std::vector<ActionState> getAllActionStates() const override;
    bool isActionActive(const Action& action) const override;
    bool wasActionJustPressed(const Action& action) const override;
    bool wasActionJustReleased(const Action& action) const override;
    float getActionValue(const Action& action) const override;

    //======================================================================
    // Raw Input (for Rebinding UI)
    //======================================================================

    std::optional<InputBinding> getLastInput() const override;
    bool isListeningForInput() const override;
    void startListeningForInput() override;
    void stopListeningForInput() override;

    //======================================================================
    // Mouse State (Platform-Agnostic)
    //======================================================================

    Vec2 getMousePosition() const override;
    Vec2 getMouseDelta() const override;
    bool isMouseButtonDown(MouseButton button) const override;
    bool wasMouseButtonJustPressed(MouseButton button) const override;
    bool wasMouseButtonJustReleased(MouseButton button) const override;

    //======================================================================
    // Modifier Keys
    //======================================================================

    ModifierKey getModifierState() const override;
    bool isModifierPressed(ModifierKey mod) const override;
    bool isShiftPressed() const override;
    bool isCtrlPressed() const override;
    bool isAltPressed() const override;
    bool isSuperPressed() const override;

    //======================================================================
    // Direct Keyboard State (Platform-Agnostic)
    //======================================================================

    bool isKeyDown(KeyCode key) const override;
    bool wasKeyJustPressed(KeyCode key) const override;
    bool wasKeyJustReleased(KeyCode key) const override;

    //======================================================================
    // Gamepad State (Platform-Agnostic)
    //======================================================================

    bool isGamepadButtonDown(GamepadButton button, int gamepadIndex = 0) const override;
    bool wasGamepadButtonJustPressed(GamepadButton button, int gamepadIndex = 0) const override;
    bool wasGamepadButtonJustReleased(GamepadButton button, int gamepadIndex = 0) const override;
    float getGamepadAxisValue(GamepadAxis axis, int gamepadIndex = 0) const override;
    Vec2 getLeftStick(int gamepadIndex = 0) const override;
    Vec2 getRightStick(int gamepadIndex = 0) const override;

    //======================================================================
    // Scroll Wheel
    //======================================================================

    Vec2 getScrollDelta() const override;

    //======================================================================
    // Text Input
    //======================================================================

    void enableTextInput() override;
    void disableTextInput() override;
    bool isTextInputEnabled() const override;
    std::string getTextInput() const override;
    void clearTextInput() override;

    //======================================================================
    // Controller
    //======================================================================

    int getConnectedControllerCount() const override;
    bool isControllerConnected(int index) const override;
    std::string getControllerName(int index) const override;

    //======================================================================
    // Cursor Control
    //======================================================================

    void showMouseCursor() override;
    void hideMouseCursor() override;
    bool isMouseCursorVisible() const override;
    void setCursorMode(CursorMode mode) override;
    CursorMode getCursorMode() const override;

    //======================================================================
    // GLFW Callbacks
    //======================================================================

    void onScrollCallback(double xoffset, double yoffset);
    void onCharCallback(unsigned int codepoint);

private:
    //======================================================================
    // Update Helpers
    //======================================================================

    void updateKeyboardState();
    void updateMouseState();
    void updateControllerState();
    void updateActionStates();
    void updateModifierState();
    void updateInputStateTracking(float dt);
    void processActionRegistrations();

    //======================================================================
    // Input State Tracking Helpers
    //======================================================================

    /// Generate a unique key for an InputBinding (for state tracking maps)
    std::string serializeBinding(const InputBinding& binding) const;

    /// Get raw input value for a binding (0.0 or 1.0 for buttons, -1 to 1 for axes)
    float getRawInputValue(const InputBinding& binding) const;

    /// Check if a single condition is satisfied
    bool checkCondition(const ActionCondition& condition) const;

    /// Check if all conditions in a registration are satisfied
    bool checkAllConditions(const ActionRegistration& reg) const;

    /// Execute effects for a triggered action
    void executeEffects(const ActionRegistration& reg);

    /// Emit an action event through EventSystem
    void emitActionEvent(const std::string& actionName, InputSource source,
                         float duration, const Vec2& axis);

    //======================================================================
    // Platform State (GLFW/SDL)
    //======================================================================

    GLFWwindow* window_ = nullptr;
    std::array<SDL_GameController*, 4> controllers_{};
    bool sdlInitialized_ = false;

    //======================================================================
    // Phase System State
    //======================================================================

    std::vector<std::string> phaseStack_;
    bool phaseHasBeenSet_ = false;

    //======================================================================
    // Action Registration State
    //======================================================================

    std::vector<ActionRegistration> actionRegistrations_;
    std::string inputConfigPath_;

    //======================================================================
    // Input State Tracking (for state machine: NotPressed -> JustPressed -> Pressed -> Held -> JustReleased)
    //======================================================================

    /// Tracks current InputState for each binding
    std::unordered_map<std::string, InputState> inputStates_;

    /// Tracks how long each input has been held (seconds)
    std::unordered_map<std::string, float> holdDurations_;

    /// Tracks which discrete registrations have fired (to prevent re-firing until released)
    std::unordered_set<const ActionRegistration*> discreteFired_;

    /// Default threshold for transitioning to Held state (seconds)
    float defaultHoldThreshold_ = 0.5f;

    /// Delta time for current frame (set in update())
    float deltaTime_ = 0.0f;

    //======================================================================
    // Legacy Action State (for deprecated API)
    //======================================================================

    std::vector<InputMapping> mappings_;
    std::unordered_map<Action, ActionState> actionStates_;
    std::unordered_map<Action, ActionState> prevActionStates_;

    //======================================================================
    // Raw Input State
    //======================================================================

    Vec2 mousePosition_{0, 0};
    Vec2 prevMousePosition_{0, 0};
    bool mouseButtons_[8] = {};
    bool prevMouseButtons_[8] = {};

    std::unordered_map<int, bool> keyStates_;     // GLFW key code -> pressed
    std::unordered_map<int, bool> prevKeyStates_;

    // Gamepad button state per controller [controller_index][button] -> pressed
    std::array<std::array<bool, static_cast<size_t>(GamepadButton::Count)>, 4> gamepadButtons_{};
    std::array<std::array<bool, static_cast<size_t>(GamepadButton::Count)>, 4> prevGamepadButtons_{};

    // Gamepad axis state per controller [controller_index][axis] -> value (-1 to 1)
    std::array<std::array<float, static_cast<size_t>(GamepadAxis::Count)>, 4> gamepadAxes_{};

    Vec2 scrollDelta_{0, 0};
    ModifierKey currentModifiers_ = ModifierKey::None;

    bool textInputEnabled_ = false;
    std::string textInputBuffer_;

    bool isListening_ = false;
    std::optional<InputBinding> lastInput_;

    //======================================================================
    // Cursor State
    //======================================================================

    CursorMode cursorMode_ = CursorMode::Normal;

    //======================================================================
    // Injected Dependencies
    //======================================================================

    IEventSystem* pIEventSystem_ = nullptr;
    IAssetSystem* pIAssetSystem_ = nullptr;

public:
    struct Service;
};

//==========================================================================
// Kangaru Service Registration
//==========================================================================

struct InputSystem::Service : kgr::single_service<InputSystem>, kgr::overrides<IInputSystemService> {
    static auto construct(kgr::inject_t<IEventSystemService> d1, kgr::inject_t<IAssetSystemService> d2)
        -> kgr::inject_result<IEventSystem*, IAssetSystem*> {
        return kgr::inject(&d1.forward(), &d2.forward());
    }
};

// Backwards compatibility alias
using InputSystemService = InputSystem::Service;

}  // namespace bestow

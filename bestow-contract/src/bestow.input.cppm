// bestow-contract/src/bestow.input.cppm
// Input system interface

module;

#include <optional>
#include <string>
#include <vector>

export module bestow.input;

import bestow.types;

export namespace bestow {

class IInputSystem {
public:
    virtual ~IInputSystem() = default;

    //======================================================================
    // Lifecycle
    //======================================================================

    virtual bool initialize(void* nativeWindow) = 0;
    virtual void shutdown() = 0;
    virtual void update() = 0;

    //======================================================================
    // Phase Management (Event-Driven Input System)
    //======================================================================

    /// Get the current active phase (top of stack)
    virtual std::string getCurrentPhase() const = 0;

    /// Get the full phase stack
    virtual std::vector<std::string> getPhaseStack() const = 0;

    /// Push a phase onto the stack (enables that phase's actions)
    virtual void pushPhase(const std::string& phase) = 0;

    /// Pop the top phase from the stack (returns to previous phase)
    virtual void popPhase() = 0;

    /// Replace entire stack with a single phase
    virtual void changePhase(const std::string& phase) = 0;

    /// Check if a phase (or any of its ancestors) is currently active
    virtual bool isPhaseActive(const std::string& phase) const = 0;

    /// Check if any phase has been set (for startup validation)
    virtual bool hasPhaseBeenSet() const = 0;

    //======================================================================
    // Action Registration (Event-Driven Input System)
    //======================================================================

    /// Register a complete action registration (from builder API)
    virtual void registerAction(const ActionRegistration& registration) = 0;

    /// Unregister all actions with a specific name
    virtual void unregisterAction(const std::string& actionName) = 0;

    /// Unregister all actions for a specific phase
    virtual void unregisterPhaseActions(const std::string& phase) = 0;

    /// Clear all action registrations
    virtual void clearActions() = 0;

    /// Get all registered actions
    virtual std::vector<ActionRegistration> getActions() const = 0;

    //======================================================================
    // Input State Queries (Platform-Agnostic)
    //======================================================================

    /// Get the current state of an input binding
    virtual InputState getInputState(const InputBinding& binding) const = 0;

    /// Get how long an input has been held (0 if not held)
    virtual float getInputHoldDuration(const InputBinding& binding) const = 0;

    /// Set the default hold threshold for HELD state transition
    virtual void setDefaultHoldThreshold(float seconds) = 0;

    /// Get the default hold threshold
    virtual float getDefaultHoldThreshold() const = 0;

    //======================================================================
    // Configuration Loading
    //======================================================================

    /// Load action registrations from inputs.lua file
    /// Called automatically during initialization if file exists
    virtual bool loadInputConfig(const std::string& path) = 0;

    /// Reload input configuration (for hot reload)
    virtual bool reloadInputConfig() = 0;

    //======================================================================
    // Mapping Management (Legacy - for backwards compatibility)
    //======================================================================

    [[deprecated("Use registerAction() with ActionRegistration instead")]]
    virtual void registerMapping(const InputMapping& mapping) = 0;

    [[deprecated("Use unregisterAction() instead")]]
    virtual void removeMapping(const InputBinding& binding) = 0;

    [[deprecated("Use clearActions() instead")]]
    virtual void clearMappings() = 0;

    virtual std::vector<InputMapping> getMappings() const = 0;

    //======================================================================
    // Action State Queries (Deprecated - use event subscriptions)
    //======================================================================

    [[deprecated("Use event subscriptions instead: events->subscribe(Events::ActionTriggered, ...)")]]
    virtual ActionState getActionState(const Action& action) const = 0;

    [[deprecated("Use event subscriptions instead")]]
    virtual std::vector<ActionState> getAllActionStates() const = 0;

    [[deprecated("Use event subscriptions instead")]]
    virtual bool isActionActive(const Action& action) const = 0;

    [[deprecated("Use event subscriptions instead")]]
    virtual bool wasActionJustPressed(const Action& action) const = 0;

    [[deprecated("Use event subscriptions instead")]]
    virtual bool wasActionJustReleased(const Action& action) const = 0;

    [[deprecated("Use event subscriptions instead")]]
    virtual float getActionValue(const Action& action) const = 0;

    //======================================================================
    // Raw Input (for Rebinding UI)
    //======================================================================

    virtual std::optional<InputBinding> getLastInput() const = 0;
    virtual bool isListeningForInput() const = 0;
    virtual void startListeningForInput() = 0;
    virtual void stopListeningForInput() = 0;

    //======================================================================
    // Mouse State (Platform-Agnostic)
    //======================================================================

    virtual Vec2 getMousePosition() const = 0;
    virtual Vec2 getMouseDelta() const = 0;

    /// Check if a mouse button is currently down
    virtual bool isMouseButtonDown(MouseButton button) const = 0;

    /// Check if a mouse button was just pressed this frame
    virtual bool wasMouseButtonJustPressed(MouseButton button) const = 0;

    /// Check if a mouse button was just released this frame
    virtual bool wasMouseButtonJustReleased(MouseButton button) const = 0;

    //======================================================================
    // Modifier Keys
    //======================================================================

    /// Get the current state of all modifier keys as a bitmask
    virtual ModifierKey getModifierState() const = 0;

    /// Check if a specific modifier (or combination) is currently pressed
    virtual bool isModifierPressed(ModifierKey mod) const = 0;

    /// Convenience methods for common modifier checks
    virtual bool isShiftPressed() const = 0;
    virtual bool isCtrlPressed() const = 0;
    virtual bool isAltPressed() const = 0;
    virtual bool isSuperPressed() const = 0;  // Windows/Command key

    //======================================================================
    // Direct Keyboard State (Platform-Agnostic)
    //======================================================================

    /// Check if a specific key is currently pressed
    virtual bool isKeyDown(KeyCode key) const = 0;

    /// Check if a key was just pressed this frame
    virtual bool wasKeyJustPressed(KeyCode key) const = 0;

    /// Check if a key was just released this frame
    virtual bool wasKeyJustReleased(KeyCode key) const = 0;

    //======================================================================
    // Gamepad State (Platform-Agnostic)
    //======================================================================

    /// Check if a gamepad button is currently down
    virtual bool isGamepadButtonDown(GamepadButton button, int gamepadIndex = 0) const = 0;

    /// Check if a gamepad button was just pressed this frame
    virtual bool wasGamepadButtonJustPressed(GamepadButton button, int gamepadIndex = 0) const = 0;

    /// Check if a gamepad button was just released this frame
    virtual bool wasGamepadButtonJustReleased(GamepadButton button, int gamepadIndex = 0) const = 0;

    /// Get the value of a gamepad axis (-1 to 1 for sticks, 0 to 1 for triggers)
    virtual float getGamepadAxisValue(GamepadAxis axis, int gamepadIndex = 0) const = 0;

    /// Get left stick as Vec2
    virtual Vec2 getLeftStick(int gamepadIndex = 0) const = 0;

    /// Get right stick as Vec2
    virtual Vec2 getRightStick(int gamepadIndex = 0) const = 0;

    //======================================================================
    // Scroll Wheel
    //======================================================================

    virtual Vec2 getScrollDelta() const = 0;

    //======================================================================
    // Text Input
    //======================================================================

    virtual void enableTextInput() = 0;
    virtual void disableTextInput() = 0;
    virtual bool isTextInputEnabled() const = 0;
    virtual std::string getTextInput() const = 0;
    virtual void clearTextInput() = 0;

    //======================================================================
    // Controller
    //======================================================================

    virtual int getConnectedControllerCount() const = 0;
    virtual bool isControllerConnected(int index) const = 0;
    virtual std::string getControllerName(int index) const = 0;
};

}  // namespace bestow

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
    // Mapping Management
    //======================================================================

    virtual void registerMapping(const InputMapping& mapping) = 0;
    virtual void removeMapping(const InputBinding& binding) = 0;
    virtual void clearMappings() = 0;
    virtual std::vector<InputMapping> getMappings() const = 0;

    //======================================================================
    // Action State Queries
    //======================================================================

    virtual ActionState getActionState(const Action& action) const = 0;
    virtual std::vector<ActionState> getAllActionStates() const = 0;

    virtual bool isActionActive(const Action& action) const = 0;
    virtual bool wasActionJustPressed(const Action& action) const = 0;
    virtual bool wasActionJustReleased(const Action& action) const = 0;
    virtual float getActionValue(const Action& action) const = 0;

    //======================================================================
    // Raw Input (for Rebinding UI)
    //======================================================================

    virtual std::optional<InputBinding> getLastInput() const = 0;
    virtual bool isListeningForInput() const = 0;
    virtual void startListeningForInput() = 0;
    virtual void stopListeningForInput() = 0;

    //======================================================================
    // Mouse State
    //======================================================================

    virtual Vec2 getMousePosition() const = 0;
    virtual Vec2 getMouseDelta() const = 0;
    virtual bool isMouseButtonDown(int button) const = 0;

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
    // Direct Keyboard State
    //======================================================================

    /// Check if a specific key is currently pressed (by GLFW key code)
    virtual bool isKeyDown(int keyCode) const = 0;

    /// Check if a key was just pressed this frame
    virtual bool wasKeyJustPressed(int keyCode) const = 0;

    /// Check if a key was just released this frame
    virtual bool wasKeyJustReleased(int keyCode) const = 0;

    //======================================================================
    // Direct Mouse Button State
    //======================================================================

    /// Check if a mouse button was just pressed this frame
    virtual bool wasMouseButtonJustPressed(int button) const = 0;

    /// Check if a mouse button was just released this frame
    virtual bool wasMouseButtonJustReleased(int button) const = 0;

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

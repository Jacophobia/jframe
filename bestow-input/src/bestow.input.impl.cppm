// bestow-input/src/bestow.input.impl.cppm
// Input system implementation using GLFW and SDL2

module;

#include <kangaru/kangaru.hpp>
#include <bestow/kangaru_macros.hpp>
#include <GLFW/glfw3.h>
#include <SDL.h>

export module bestow.input.impl;

import std;
import bestow.services;  // Re-exports all contracts including bestow.input, bestow.assets, bestow.types

export namespace bestow {

class InputSystem : public IInputSystem {
public:
    explicit InputSystem(IAssetSystem* pIAssetSystem = nullptr)
        : pIAssetSystem_(pIAssetSystem) {}
    ~InputSystem() override;

    bool initialize(void* nativeWindow) override;
    void shutdown() override;

    void update() override;

    // Mapping management
    void registerMapping(const InputMapping& mapping) override;
    void removeMapping(const InputBinding& binding) override;
    void clearMappings() override;
    std::vector<InputMapping> getMappings() const override;

    // Action state queries
    ActionState getActionState(const Action& action) const override;
    std::vector<ActionState> getAllActionStates() const override;
    bool isActionActive(const Action& action) const override;
    bool wasActionJustPressed(const Action& action) const override;
    bool wasActionJustReleased(const Action& action) const override;
    float getActionValue(const Action& action) const override;

    // Raw input
    std::optional<InputBinding> getLastInput() const override;
    bool isListeningForInput() const override;
    void startListeningForInput() override;
    void stopListeningForInput() override;

    // Mouse state
    Vec2 getMousePosition() const override;
    Vec2 getMouseDelta() const override;
    bool isMouseButtonDown(int button) const override;

    // Modifier keys
    ModifierKey getModifierState() const override;
    bool isModifierPressed(ModifierKey mod) const override;
    bool isShiftPressed() const override;
    bool isCtrlPressed() const override;
    bool isAltPressed() const override;
    bool isSuperPressed() const override;

    // Direct keyboard state
    bool isKeyDown(int keyCode) const override;
    bool wasKeyJustPressed(int keyCode) const override;
    bool wasKeyJustReleased(int keyCode) const override;

    // Direct mouse button state
    bool wasMouseButtonJustPressed(int button) const override;
    bool wasMouseButtonJustReleased(int button) const override;

    // Scroll wheel
    Vec2 getScrollDelta() const override;

    // Text input
    void enableTextInput() override;
    void disableTextInput() override;
    bool isTextInputEnabled() const override;
    std::string getTextInput() const override;
    void clearTextInput() override;

    // Controller
    int getConnectedControllerCount() const override;
    bool isControllerConnected(int index) const override;
    std::string getControllerName(int index) const override;

    // GLFW callbacks (need to be called by external scroll/char callback setters)
    void onScrollCallback(double xoffset, double yoffset);
    void onCharCallback(unsigned int codepoint);

private:
    void updateKeyboardState();
    void updateMouseState();
    void updateControllerState();
    void updateActionStates();

    GLFWwindow* window_ = nullptr;
    std::vector<InputMapping> mappings_;
    std::unordered_map<Action, ActionState> actionStates_;
    std::unordered_map<Action, ActionState> prevActionStates_;

    Vec2 mousePosition_{0, 0};
    Vec2 prevMousePosition_{0, 0};
    bool mouseButtons_[8] = {};
    bool prevMouseButtons_[8] = {};

    // Track key states for just pressed/released detection
    // Using a map since GLFW key codes are sparse (GLFW_KEY_SPACE=32 to GLFW_KEY_LAST=348)
    std::unordered_map<int, bool> keyStates_;
    std::unordered_map<int, bool> prevKeyStates_;

    std::array<SDL_GameController*, 4> controllers_{};
    bool isListening_ = false;
    std::optional<InputBinding> lastInput_;

    // Scroll wheel
    Vec2 scrollDelta_{0, 0};

    // Modifier keys
    ModifierKey currentModifiers_ = ModifierKey::None;

    // Text input
    bool textInputEnabled_ = false;
    std::string textInputBuffer_;

    bool sdlInitialized_ = false;

    // Injected dependencies
    IAssetSystem* pIAssetSystem_ = nullptr;

    // Helper to update modifier state from GLFW
    void updateModifierState();
};

// Service definition - must be after class is complete
BESTOW_SERVICE(InputSystem, InputSystem, AssetSystem);

}  // namespace bestow

// bestow-input/src/bestow.input.impl.cppm
// Input system implementation using GLFW and SDL2

module;

#include <GLFW/glfw3.h>
#include <SDL.h>

export module bestow.input.impl;

import std;
import bestow.input;
import bestow.types;

export namespace bestow {

class InputSystem : public IInputSystem {
public:
    InputSystem() = default;
    ~InputSystem() override;

    bool initialize(GLFWwindow* window);

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

    // Controller
    int getConnectedControllerCount() const override;
    bool isControllerConnected(int index) const override;
    std::string getControllerName(int index) const override;

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

    std::array<SDL_GameController*, 4> controllers_{};
    bool isListening_ = false;
    std::optional<InputBinding> lastInput_;

    bool sdlInitialized_ = false;
};

// Factory function (exported via namespace)
inline std::unique_ptr<IInputSystem> createInputSystem() {
    return std::make_unique<InputSystem>();
}

}  // namespace bestow

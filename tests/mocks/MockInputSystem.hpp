// tests/mocks/MockInputSystem.hpp
// Shared mock input system for testing

#pragma once

import std;
import bestow;
import bestow.types;

namespace bestow::tests {

class MockInputSystem : public IInputSystem {
public:
    // Tracking state
    std::vector<std::string> phaseStack_;
    std::vector<InputMapping> registeredMappings;
    std::vector<ActionRegistration> registeredActions;

    // Delegates
    std::function<bool(void*)> onInitialize = [](void*) { return true; };
    std::function<void()> onShutdown = [] {};
    std::function<void()> onUpdate = [] {};

    std::function<std::string()> onGetCurrentPhase =
        [this] { return phaseStack_.empty() ? "" : phaseStack_.back(); };
    std::function<std::vector<std::string>()> onGetPhaseStack =
        [this] { return phaseStack_; };
    std::function<void(const std::string&)> onPushPhase =
        [this](const std::string& phase) { phaseStack_.push_back(phase); };
    std::function<void()> onPopPhase =
        [this] { if (!phaseStack_.empty()) phaseStack_.pop_back(); };
    std::function<void(const std::string&)> onChangePhase =
        [this](const std::string& phase) {
            phaseStack_.clear();
            phaseStack_.push_back(phase);
        };
    std::function<bool(const std::string&)> onIsPhaseActive =
        [this](const std::string& phase) {
            return !phaseStack_.empty() && phaseStack_.back() == phase;
        };
    std::function<bool()> onHasPhaseBeenSet =
        [this] { return !phaseStack_.empty(); };

    std::function<void(const ActionRegistration&)> onRegisterAction =
        [this](const ActionRegistration& reg) { registeredActions.push_back(reg); };
    std::function<void(const std::string&)> onUnregisterAction = [](const std::string&) {};
    std::function<void(const std::string&)> onUnregisterPhaseActions = [](const std::string&) {};
    std::function<void()> onClearActions = [this] { registeredActions.clear(); };
    std::function<std::vector<ActionRegistration>()> onGetActions =
        [this] { return registeredActions; };

    std::function<InputState(const InputBinding&)> onGetInputState =
        [](const InputBinding&) { return InputState{}; };
    std::function<float(const InputBinding&)> onGetInputHoldDuration =
        [](const InputBinding&) { return 0.0f; };
    std::function<void(float)> onSetDefaultHoldThreshold = [](float) {};
    std::function<float()> onGetDefaultHoldThreshold = [] { return 0.5f; };

    std::function<bool(const std::string&)> onLoadInputConfig =
        [](const std::string&) { return true; };
    std::function<bool()> onReloadInputConfig = [] { return true; };

    // IInputSystem overrides
    bool initialize(void* nativeWindow) override { return onInitialize(nativeWindow); }
    void shutdown() override { onShutdown(); }
    void update() override { onUpdate(); }

    std::string getCurrentPhase() const override { return onGetCurrentPhase(); }
    std::vector<std::string> getPhaseStack() const override { return onGetPhaseStack(); }
    void pushPhase(const std::string& phase) override { onPushPhase(phase); }
    void popPhase() override { onPopPhase(); }
    void changePhase(const std::string& phase) override { onChangePhase(phase); }
    bool isPhaseActive(const std::string& phase) const override { return onIsPhaseActive(phase); }
    bool hasPhaseBeenSet() const override { return onHasPhaseBeenSet(); }

    void registerAction(const ActionRegistration& reg) override { onRegisterAction(reg); }
    void unregisterAction(const std::string& name) override { onUnregisterAction(name); }
    void unregisterPhaseActions(const std::string& phase) override {
        onUnregisterPhaseActions(phase);
    }
    void clearActions() override { onClearActions(); }
    std::vector<ActionRegistration> getActions() const override { return onGetActions(); }

    InputState getInputState(const InputBinding& binding) const override {
        return onGetInputState(binding);
    }
    float getInputHoldDuration(const InputBinding& binding) const override {
        return onGetInputHoldDuration(binding);
    }
    void setDefaultHoldThreshold(float seconds) override { onSetDefaultHoldThreshold(seconds); }
    float getDefaultHoldThreshold() const override { return onGetDefaultHoldThreshold(); }

    bool loadInputConfig(const std::string& path) override { return onLoadInputConfig(path); }
    bool reloadInputConfig() override { return onReloadInputConfig(); }

    // Legacy mapping (deprecated) — tracks for builder tests
    void registerMapping(const InputMapping& mapping) override {
        registeredMappings.push_back(mapping);
    }
    void removeMapping(const InputBinding&) override {}
    void clearMappings() override {}
    std::vector<InputMapping> getMappings() const override { return registeredMappings; }

    // Legacy action state queries (deprecated) — simple stubs
    ActionState getActionState(const Action&) const override { return {}; }
    std::vector<ActionState> getAllActionStates() const override { return {}; }
    bool isActionActive(const Action&) const override { return false; }
    bool wasActionJustPressed(const Action&) const override { return false; }
    bool wasActionJustReleased(const Action&) const override { return false; }
    float getActionValue(const Action&) const override { return 0.0f; }

    // Raw input
    std::optional<InputBinding> getLastInput() const override { return std::nullopt; }
    bool isListeningForInput() const override { return false; }
    void startListeningForInput() override {}
    void stopListeningForInput() override {}

    // Mouse state
    Vec2 getMousePosition() const override { return {}; }
    Vec2 getMouseDelta() const override { return {}; }
    bool isMouseButtonDown(MouseButton) const override { return false; }
    bool wasMouseButtonJustPressed(MouseButton) const override { return false; }
    bool wasMouseButtonJustReleased(MouseButton) const override { return false; }

    // Modifiers
    ModifierKey getModifierState() const override { return ModifierKey::None; }
    bool isModifierPressed(ModifierKey) const override { return false; }
    bool isShiftPressed() const override { return false; }
    bool isCtrlPressed() const override { return false; }
    bool isAltPressed() const override { return false; }
    bool isSuperPressed() const override { return false; }

    // Keyboard
    bool isKeyDown(KeyCode) const override { return false; }
    bool wasKeyJustPressed(KeyCode) const override { return false; }
    bool wasKeyJustReleased(KeyCode) const override { return false; }

    // Gamepad
    bool isGamepadButtonDown(GamepadButton, int) const override { return false; }
    bool wasGamepadButtonJustPressed(GamepadButton, int) const override { return false; }
    bool wasGamepadButtonJustReleased(GamepadButton, int) const override { return false; }
    float getGamepadAxisValue(GamepadAxis, int) const override { return 0.0f; }
    Vec2 getLeftStick(int) const override { return {}; }
    Vec2 getRightStick(int) const override { return {}; }

    // Scroll
    Vec2 getScrollDelta() const override { return {}; }

    // Text input
    void enableTextInput() override {}
    void disableTextInput() override {}
    bool isTextInputEnabled() const override { return false; }
    std::string getTextInput() const override { return ""; }
    void clearTextInput() override {}

    // Controller
    int getConnectedControllerCount() const override { return 0; }
    bool isControllerConnected(int) const override { return false; }
    std::string getControllerName(int) const override { return ""; }

    // Cursor
    void showMouseCursor() override {}
    void hideMouseCursor() override {}
    bool isMouseCursorVisible() const override { return true; }
    void setCursorMode(CursorMode) override {}
    CursorMode getCursorMode() const override { return CursorMode::Normal; }
};

}  // namespace bestow::tests

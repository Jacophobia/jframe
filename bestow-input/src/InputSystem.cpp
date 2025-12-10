// bestow-input/src/InputSystem.cpp
// Input system implementation

module;

#include <array>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include <glm/glm.hpp>

#include <GLFW/glfw3.h>
#include <SDL.h>

module bestow.input.impl;

namespace bestow {


InputSystem::~InputSystem() {
    for (auto* controller : controllers_) {
        if (controller) {
            SDL_GameControllerClose(controller);
        }
    }
    if (sdlInitialized_) {
        SDL_QuitSubSystem(SDL_INIT_GAMECONTROLLER);
    }
}

bool InputSystem::initialize(void* nativeWindow) {
    window_ = static_cast<GLFWwindow*>(nativeWindow);

    // Initialize SDL2 for game controllers only
    if (SDL_InitSubSystem(SDL_INIT_GAMECONTROLLER) < 0) {
        return false;
    }
    sdlInitialized_ = true;

    // Load controller mappings from AssetSystem if available
    if (assetSystem_) {
        AssetHandle mappingsHandle = assetSystem_->registerAsset(
            AssetType::Data,
            "data/config/gamecontrollerdb.txt"
        );
        assetSystem_->loadAsset(mappingsHandle);

        if (assetSystem_->isLoaded(mappingsHandle)) {
            const DataAsset* data = static_cast<const DataAsset*>(
                assetSystem_->getRawAsset(mappingsHandle)
            );
            if (data && !data->rawText.empty()) {
                // Use SDL_RWFromMem to load mappings from memory instead of file
                SDL_RWops* rw = SDL_RWFromMem(
                    (void*)data->rawText.data(),
                    static_cast<int>(data->rawText.size())
                );
                if (rw) {
                    SDL_GameControllerAddMappingsFromRW(rw, 1);  // 1 = free RW after reading
                }
            }
        }
    }

    // Enumerate existing controllers
    int numJoysticks = SDL_NumJoysticks();
    for (int i = 0; i < numJoysticks && i < 4; ++i) {
        if (SDL_IsGameController(i)) {
            controllers_[i] = SDL_GameControllerOpen(i);
        }
    }

    return true;
}

void InputSystem::shutdown() {
    // Close all open controllers
    for (auto& controller : controllers_) {
        if (controller) {
            SDL_GameControllerClose(controller);
            controller = nullptr;
        }
    }
    
    // Shutdown SDL game controller subsystem
    if (sdlInitialized_) {
        SDL_QuitSubSystem(SDL_INIT_GAMECONTROLLER);
        sdlInitialized_ = false;
    }
    
    window_ = nullptr;
}

void InputSystem::setAssetSystem(IAssetSystem* assets) {
    assetSystem_ = assets;
}

void InputSystem::update() {
    prevActionStates_ = actionStates_;
    prevMousePosition_ = mousePosition_;

    // Reset scroll delta (it accumulates from callbacks)
    scrollDelta_ = Vec2{0.0f, 0.0f};

    updateKeyboardState();
    updateMouseState();
    updateControllerState();
    updateActionStates();
}

void InputSystem::updateKeyboardState() {
    if (!window_) return;

    // If listening for input, capture any key press
    if (isListening_) {
        for (int key = GLFW_KEY_SPACE; key <= GLFW_KEY_LAST; ++key) {
            if (glfwGetKey(window_, key) == GLFW_PRESS) {
                lastInput_ = InputBinding{
                    .deviceType = InputDeviceType::Keyboard,
                    .deviceIndex = 0,
                    .keyCode = key,
                    .scale = 1.0f,
                    .deadzone = 0.0f
                };
                isListening_ = false;
                return;
            }
        }
    }
}

void InputSystem::updateMouseState() {
    if (!window_) return;

    double x, y;
    glfwGetCursorPos(window_, &x, &y);
    mousePosition_ = Vec2{static_cast<float>(x), static_cast<float>(y)};

    for (int i = 0; i < 8; ++i) {
        bool wasDown = mouseButtons_[i];
        mouseButtons_[i] = glfwGetMouseButton(window_, i) == GLFW_PRESS;

        // If listening for input, capture mouse button press
        if (isListening_ && mouseButtons_[i] && !wasDown) {
            lastInput_ = InputBinding{
                .deviceType = InputDeviceType::Mouse,
                .deviceIndex = 0,
                .keyCode = i,
                .scale = 1.0f,
                .deadzone = 0.0f
            };
            isListening_ = false;
            return;
        }
    }
}

void InputSystem::updateControllerState() {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        switch (event.type) {
            case SDL_CONTROLLERDEVICEADDED: {
                int index = event.cdevice.which;
                if (index < 4 && !controllers_[index]) {
                    controllers_[index] = SDL_GameControllerOpen(index);
                }
                break;
            }
            case SDL_CONTROLLERDEVICEREMOVED: {
                for (int i = 0; i < 4; ++i) {
                    if (controllers_[i] &&
                        SDL_JoystickInstanceID(SDL_GameControllerGetJoystick(controllers_[i])) ==
                            event.cdevice.which) {
                        SDL_GameControllerClose(controllers_[i]);
                        controllers_[i] = nullptr;
                        break;
                    }
                }
                break;
            }
            case SDL_CONTROLLERBUTTONDOWN: {
                if (isListening_) {
                    // Find which controller index this is
                    int controllerIndex = -1;
                    for (int i = 0; i < 4; ++i) {
                        if (controllers_[i] &&
                            SDL_JoystickInstanceID(SDL_GameControllerGetJoystick(controllers_[i])) ==
                                event.cbutton.which) {
                            controllerIndex = i;
                            break;
                        }
                    }
                    if (controllerIndex >= 0) {
                        lastInput_ = InputBinding{
                            .deviceType = InputDeviceType::Controller,
                            .deviceIndex = controllerIndex,
                            .keyCode = event.cbutton.button,
                            .scale = 1.0f,
                            .deadzone = 0.0f
                        };
                        isListening_ = false;
                    }
                }
                break;
            }
            case SDL_CONTROLLERAXISMOTION: {
                if (isListening_) {
                    // Only capture significant axis movement (past 50% threshold)
                    constexpr Sint16 LISTEN_THRESHOLD = 16384;
                    if (std::abs(event.caxis.value) > LISTEN_THRESHOLD) {
                        int controllerIndex = -1;
                        for (int i = 0; i < 4; ++i) {
                            if (controllers_[i] &&
                                SDL_JoystickInstanceID(SDL_GameControllerGetJoystick(controllers_[i])) ==
                                    event.caxis.which) {
                                controllerIndex = i;
                                break;
                            }
                        }
                        if (controllerIndex >= 0) {
                            // Encode axis as button code + SDL_CONTROLLER_BUTTON_MAX
                            lastInput_ = InputBinding{
                                .deviceType = InputDeviceType::Controller,
                                .deviceIndex = controllerIndex,
                                .keyCode = event.caxis.axis + SDL_CONTROLLER_BUTTON_MAX,
                                .scale = event.caxis.value > 0 ? 1.0f : -1.0f,
                                .deadzone = 0.2f
                            };
                            isListening_ = false;
                        }
                    }
                }
                break;
            }
        }
    }
}

void InputSystem::updateActionStates() {
    // First, save previous states and reset current values
    std::unordered_map<Action, bool> wasActiveMap;
    for (auto& [action, state] : actionStates_) {
        wasActiveMap[action] = state.active;
        state.value = 0.0f;
        state.active = false;
    }

    // Accumulate values from all mappings
    for (const auto& mapping : mappings_) {
        auto& state = actionStates_[mapping.action];
        state.action = mapping.action;

        float bindingValue = 0.0f;

        switch (mapping.binding.deviceType) {
            case InputDeviceType::Keyboard:
                if (window_ && glfwGetKey(window_, mapping.binding.keyCode) == GLFW_PRESS) {
                    bindingValue = mapping.binding.scale;
                }
                break;

            case InputDeviceType::Mouse:
                if (window_ && mapping.binding.keyCode < 8 && mouseButtons_[mapping.binding.keyCode]) {
                    bindingValue = mapping.binding.scale;
                }
                break;

            case InputDeviceType::Controller:
                if (mapping.binding.deviceIndex < 4 &&
                    controllers_[mapping.binding.deviceIndex]) {
                    auto* controller = controllers_[mapping.binding.deviceIndex];
                    // Check if it's a button or axis
                    if (mapping.binding.keyCode < SDL_CONTROLLER_BUTTON_MAX) {
                        if (SDL_GameControllerGetButton(controller,
                                static_cast<SDL_GameControllerButton>(mapping.binding.keyCode))) {
                            bindingValue = mapping.binding.scale;
                        }
                    } else {
                        int axis = mapping.binding.keyCode - SDL_CONTROLLER_BUTTON_MAX;
                        if (axis < SDL_CONTROLLER_AXIS_MAX) {
                            float axisValue = SDL_GameControllerGetAxis(controller,
                                static_cast<SDL_GameControllerAxis>(axis)) / 32767.0f;
                            if (std::abs(axisValue) > mapping.binding.deadzone) {
                                bindingValue = axisValue * mapping.binding.scale;
                            }
                        }
                    }
                }
                break;
        }

        // Accumulate: use max absolute value to handle multiple bindings
        // This allows multiple keys to contribute to the same action
        if (std::abs(bindingValue) > std::abs(state.value)) {
            state.value = bindingValue;
        }
    }

    // Update active/pressed/released states
    for (auto& [action, state] : actionStates_) {
        bool wasActive = wasActiveMap.count(action) ? wasActiveMap[action] : false;
        state.active = std::abs(state.value) > 0.01f;
        state.justPressed = state.active && !wasActive;
        state.justReleased = !state.active && wasActive;
    }
}

void InputSystem::registerMapping(const InputMapping& mapping) {
    mappings_.push_back(mapping);
}

void InputSystem::removeMapping(const InputBinding& binding) {
    mappings_.erase(
        std::remove_if(mappings_.begin(), mappings_.end(),
            [&](const InputMapping& m) {
                return m.binding.deviceType == binding.deviceType &&
                       m.binding.keyCode == binding.keyCode &&
                       m.binding.deviceIndex == binding.deviceIndex;
            }),
        mappings_.end());
}

void InputSystem::clearMappings() {
    mappings_.clear();
    actionStates_.clear();
}

std::vector<InputMapping> InputSystem::getMappings() const {
    return mappings_;
}

ActionState InputSystem::getActionState(const Action& action) const {
    if (auto it = actionStates_.find(action); it != actionStates_.end()) {
        return it->second;
    }
    return {.action = action};
}

std::vector<ActionState> InputSystem::getAllActionStates() const {
    std::vector<ActionState> states;
    for (const auto& [action, state] : actionStates_) {
        states.push_back(state);
    }
    return states;
}

bool InputSystem::isActionActive(const Action& action) const {
    return getActionState(action).active;
}

bool InputSystem::wasActionJustPressed(const Action& action) const {
    return getActionState(action).justPressed;
}

bool InputSystem::wasActionJustReleased(const Action& action) const {
    return getActionState(action).justReleased;
}

float InputSystem::getActionValue(const Action& action) const {
    return getActionState(action).value;
}

std::optional<InputBinding> InputSystem::getLastInput() const {
    return lastInput_;
}

bool InputSystem::isListeningForInput() const {
    return isListening_;
}

void InputSystem::startListeningForInput() {
    isListening_ = true;
    lastInput_ = std::nullopt;
}

void InputSystem::stopListeningForInput() {
    isListening_ = false;
}

Vec2 InputSystem::getMousePosition() const {
    return mousePosition_;
}

Vec2 InputSystem::getMouseDelta() const {
    return mousePosition_ - prevMousePosition_;
}

bool InputSystem::isMouseButtonDown(int button) const {
    return button >= 0 && button < 8 && mouseButtons_[button];
}

int InputSystem::getConnectedControllerCount() const {
    int count = 0;
    for (const auto* controller : controllers_) {
        if (controller) ++count;
    }
    return count;
}

bool InputSystem::isControllerConnected(int index) const {
    return index >= 0 && index < 4 && controllers_[index] != nullptr;
}

std::string InputSystem::getControllerName(int index) const {
    if (isControllerConnected(index)) {
        const char* name = SDL_GameControllerName(controllers_[index]);
        return name ? name : "Unknown Controller";
    }
    return "";
}

Vec2 InputSystem::getScrollDelta() const {
    return scrollDelta_;
}

void InputSystem::enableTextInput() {
    textInputEnabled_ = true;
}

void InputSystem::disableTextInput() {
    textInputEnabled_ = false;
}

bool InputSystem::isTextInputEnabled() const {
    return textInputEnabled_;
}

std::string InputSystem::getTextInput() const {
    return textInputBuffer_;
}

void InputSystem::clearTextInput() {
    textInputBuffer_.clear();
}

void InputSystem::onScrollCallback(double xoffset, double yoffset) {
    // Accumulate scroll delta (will be reset at start of next update())
    scrollDelta_.x += static_cast<float>(xoffset);
    scrollDelta_.y += static_cast<float>(yoffset);
}

void InputSystem::onCharCallback(unsigned int codepoint) {
    if (!textInputEnabled_) return;

    // Convert codepoint to UTF-8 and append to buffer
    if (codepoint < 0x80) {
        textInputBuffer_ += static_cast<char>(codepoint);
    } else if (codepoint < 0x800) {
        textInputBuffer_ += static_cast<char>(0xC0 | (codepoint >> 6));
        textInputBuffer_ += static_cast<char>(0x80 | (codepoint & 0x3F));
    } else if (codepoint < 0x10000) {
        textInputBuffer_ += static_cast<char>(0xE0 | (codepoint >> 12));
        textInputBuffer_ += static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F));
        textInputBuffer_ += static_cast<char>(0x80 | (codepoint & 0x3F));
    } else {
        textInputBuffer_ += static_cast<char>(0xF0 | (codepoint >> 18));
        textInputBuffer_ += static_cast<char>(0x80 | ((codepoint >> 12) & 0x3F));
        textInputBuffer_ += static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F));
        textInputBuffer_ += static_cast<char>(0x80 | (codepoint & 0x3F));
    }
}

}  // namespace bestow

// bestow-input/src/InputSystem.cpp
// Input system implementation with event-driven phases and action mappings

module;

#include <array>
#include <optional>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <glm/glm.hpp>
#include <spdlog/spdlog.h>

#include <GLFW/glfw3.h>
#include <SDL.h>

module bestow.input.impl;

namespace bestow {

//==========================================================================
// Platform Mapping Functions
//==========================================================================

int keyCodeToGLFW(KeyCode key) {
    // KeyCode values were designed to match GLFW key codes where possible
    // Direct mapping for most keys
    switch (key) {
        // Special keys
        case KeyCode::Space: return GLFW_KEY_SPACE;
        case KeyCode::Apostrophe: return GLFW_KEY_APOSTROPHE;
        case KeyCode::Comma: return GLFW_KEY_COMMA;
        case KeyCode::Minus: return GLFW_KEY_MINUS;
        case KeyCode::Period: return GLFW_KEY_PERIOD;
        case KeyCode::Slash: return GLFW_KEY_SLASH;
        case KeyCode::Semicolon: return GLFW_KEY_SEMICOLON;
        case KeyCode::Equal: return GLFW_KEY_EQUAL;
        case KeyCode::LeftBracket: return GLFW_KEY_LEFT_BRACKET;
        case KeyCode::Backslash: return GLFW_KEY_BACKSLASH;
        case KeyCode::RightBracket: return GLFW_KEY_RIGHT_BRACKET;
        case KeyCode::GraveAccent: return GLFW_KEY_GRAVE_ACCENT;

        // Numbers
        case KeyCode::Num0: return GLFW_KEY_0;
        case KeyCode::Num1: return GLFW_KEY_1;
        case KeyCode::Num2: return GLFW_KEY_2;
        case KeyCode::Num3: return GLFW_KEY_3;
        case KeyCode::Num4: return GLFW_KEY_4;
        case KeyCode::Num5: return GLFW_KEY_5;
        case KeyCode::Num6: return GLFW_KEY_6;
        case KeyCode::Num7: return GLFW_KEY_7;
        case KeyCode::Num8: return GLFW_KEY_8;
        case KeyCode::Num9: return GLFW_KEY_9;

        // Letters
        case KeyCode::A: return GLFW_KEY_A;
        case KeyCode::B: return GLFW_KEY_B;
        case KeyCode::C: return GLFW_KEY_C;
        case KeyCode::D: return GLFW_KEY_D;
        case KeyCode::E: return GLFW_KEY_E;
        case KeyCode::F: return GLFW_KEY_F;
        case KeyCode::G: return GLFW_KEY_G;
        case KeyCode::H: return GLFW_KEY_H;
        case KeyCode::I: return GLFW_KEY_I;
        case KeyCode::J: return GLFW_KEY_J;
        case KeyCode::K: return GLFW_KEY_K;
        case KeyCode::L: return GLFW_KEY_L;
        case KeyCode::M: return GLFW_KEY_M;
        case KeyCode::N: return GLFW_KEY_N;
        case KeyCode::O: return GLFW_KEY_O;
        case KeyCode::P: return GLFW_KEY_P;
        case KeyCode::Q: return GLFW_KEY_Q;
        case KeyCode::R: return GLFW_KEY_R;
        case KeyCode::S: return GLFW_KEY_S;
        case KeyCode::T: return GLFW_KEY_T;
        case KeyCode::U: return GLFW_KEY_U;
        case KeyCode::V: return GLFW_KEY_V;
        case KeyCode::W: return GLFW_KEY_W;
        case KeyCode::X: return GLFW_KEY_X;
        case KeyCode::Y: return GLFW_KEY_Y;
        case KeyCode::Z: return GLFW_KEY_Z;

        // Function keys
        case KeyCode::F1: return GLFW_KEY_F1;
        case KeyCode::F2: return GLFW_KEY_F2;
        case KeyCode::F3: return GLFW_KEY_F3;
        case KeyCode::F4: return GLFW_KEY_F4;
        case KeyCode::F5: return GLFW_KEY_F5;
        case KeyCode::F6: return GLFW_KEY_F6;
        case KeyCode::F7: return GLFW_KEY_F7;
        case KeyCode::F8: return GLFW_KEY_F8;
        case KeyCode::F9: return GLFW_KEY_F9;
        case KeyCode::F10: return GLFW_KEY_F10;
        case KeyCode::F11: return GLFW_KEY_F11;
        case KeyCode::F12: return GLFW_KEY_F12;

        // Navigation
        case KeyCode::Escape: return GLFW_KEY_ESCAPE;
        case KeyCode::Enter: return GLFW_KEY_ENTER;
        case KeyCode::Tab: return GLFW_KEY_TAB;
        case KeyCode::Backspace: return GLFW_KEY_BACKSPACE;
        case KeyCode::Insert: return GLFW_KEY_INSERT;
        case KeyCode::Delete: return GLFW_KEY_DELETE;
        case KeyCode::Right: return GLFW_KEY_RIGHT;
        case KeyCode::Left: return GLFW_KEY_LEFT;
        case KeyCode::Down: return GLFW_KEY_DOWN;
        case KeyCode::Up: return GLFW_KEY_UP;
        case KeyCode::PageUp: return GLFW_KEY_PAGE_UP;
        case KeyCode::PageDown: return GLFW_KEY_PAGE_DOWN;
        case KeyCode::Home: return GLFW_KEY_HOME;
        case KeyCode::End: return GLFW_KEY_END;

        // Lock keys
        case KeyCode::CapsLock: return GLFW_KEY_CAPS_LOCK;
        case KeyCode::ScrollLock: return GLFW_KEY_SCROLL_LOCK;
        case KeyCode::NumLock: return GLFW_KEY_NUM_LOCK;
        case KeyCode::PrintScreen: return GLFW_KEY_PRINT_SCREEN;
        case KeyCode::Pause: return GLFW_KEY_PAUSE;

        // Numpad
        case KeyCode::KP0: return GLFW_KEY_KP_0;
        case KeyCode::KP1: return GLFW_KEY_KP_1;
        case KeyCode::KP2: return GLFW_KEY_KP_2;
        case KeyCode::KP3: return GLFW_KEY_KP_3;
        case KeyCode::KP4: return GLFW_KEY_KP_4;
        case KeyCode::KP5: return GLFW_KEY_KP_5;
        case KeyCode::KP6: return GLFW_KEY_KP_6;
        case KeyCode::KP7: return GLFW_KEY_KP_7;
        case KeyCode::KP8: return GLFW_KEY_KP_8;
        case KeyCode::KP9: return GLFW_KEY_KP_9;
        case KeyCode::KPDecimal: return GLFW_KEY_KP_DECIMAL;
        case KeyCode::KPDivide: return GLFW_KEY_KP_DIVIDE;
        case KeyCode::KPMultiply: return GLFW_KEY_KP_MULTIPLY;
        case KeyCode::KPSubtract: return GLFW_KEY_KP_SUBTRACT;
        case KeyCode::KPAdd: return GLFW_KEY_KP_ADD;
        case KeyCode::KPEnter: return GLFW_KEY_KP_ENTER;
        case KeyCode::KPEqual: return GLFW_KEY_KP_EQUAL;

        // Modifiers
        case KeyCode::LeftShift: return GLFW_KEY_LEFT_SHIFT;
        case KeyCode::LeftCtrl: return GLFW_KEY_LEFT_CONTROL;
        case KeyCode::LeftAlt: return GLFW_KEY_LEFT_ALT;
        case KeyCode::LeftSuper: return GLFW_KEY_LEFT_SUPER;
        case KeyCode::RightShift: return GLFW_KEY_RIGHT_SHIFT;
        case KeyCode::RightCtrl: return GLFW_KEY_RIGHT_CONTROL;
        case KeyCode::RightAlt: return GLFW_KEY_RIGHT_ALT;
        case KeyCode::RightSuper: return GLFW_KEY_RIGHT_SUPER;
        case KeyCode::Menu: return GLFW_KEY_MENU;

        default: return GLFW_KEY_UNKNOWN;
    }
}

KeyCode glfwToKeyCode(int glfwKey) {
    // Reverse mapping
    switch (glfwKey) {
        case GLFW_KEY_SPACE: return KeyCode::Space;
        case GLFW_KEY_APOSTROPHE: return KeyCode::Apostrophe;
        case GLFW_KEY_COMMA: return KeyCode::Comma;
        case GLFW_KEY_MINUS: return KeyCode::Minus;
        case GLFW_KEY_PERIOD: return KeyCode::Period;
        case GLFW_KEY_SLASH: return KeyCode::Slash;
        case GLFW_KEY_SEMICOLON: return KeyCode::Semicolon;
        case GLFW_KEY_EQUAL: return KeyCode::Equal;
        case GLFW_KEY_LEFT_BRACKET: return KeyCode::LeftBracket;
        case GLFW_KEY_BACKSLASH: return KeyCode::Backslash;
        case GLFW_KEY_RIGHT_BRACKET: return KeyCode::RightBracket;
        case GLFW_KEY_GRAVE_ACCENT: return KeyCode::GraveAccent;

        case GLFW_KEY_0: return KeyCode::Num0;
        case GLFW_KEY_1: return KeyCode::Num1;
        case GLFW_KEY_2: return KeyCode::Num2;
        case GLFW_KEY_3: return KeyCode::Num3;
        case GLFW_KEY_4: return KeyCode::Num4;
        case GLFW_KEY_5: return KeyCode::Num5;
        case GLFW_KEY_6: return KeyCode::Num6;
        case GLFW_KEY_7: return KeyCode::Num7;
        case GLFW_KEY_8: return KeyCode::Num8;
        case GLFW_KEY_9: return KeyCode::Num9;

        case GLFW_KEY_A: return KeyCode::A;
        case GLFW_KEY_B: return KeyCode::B;
        case GLFW_KEY_C: return KeyCode::C;
        case GLFW_KEY_D: return KeyCode::D;
        case GLFW_KEY_E: return KeyCode::E;
        case GLFW_KEY_F: return KeyCode::F;
        case GLFW_KEY_G: return KeyCode::G;
        case GLFW_KEY_H: return KeyCode::H;
        case GLFW_KEY_I: return KeyCode::I;
        case GLFW_KEY_J: return KeyCode::J;
        case GLFW_KEY_K: return KeyCode::K;
        case GLFW_KEY_L: return KeyCode::L;
        case GLFW_KEY_M: return KeyCode::M;
        case GLFW_KEY_N: return KeyCode::N;
        case GLFW_KEY_O: return KeyCode::O;
        case GLFW_KEY_P: return KeyCode::P;
        case GLFW_KEY_Q: return KeyCode::Q;
        case GLFW_KEY_R: return KeyCode::R;
        case GLFW_KEY_S: return KeyCode::S;
        case GLFW_KEY_T: return KeyCode::T;
        case GLFW_KEY_U: return KeyCode::U;
        case GLFW_KEY_V: return KeyCode::V;
        case GLFW_KEY_W: return KeyCode::W;
        case GLFW_KEY_X: return KeyCode::X;
        case GLFW_KEY_Y: return KeyCode::Y;
        case GLFW_KEY_Z: return KeyCode::Z;

        case GLFW_KEY_F1: return KeyCode::F1;
        case GLFW_KEY_F2: return KeyCode::F2;
        case GLFW_KEY_F3: return KeyCode::F3;
        case GLFW_KEY_F4: return KeyCode::F4;
        case GLFW_KEY_F5: return KeyCode::F5;
        case GLFW_KEY_F6: return KeyCode::F6;
        case GLFW_KEY_F7: return KeyCode::F7;
        case GLFW_KEY_F8: return KeyCode::F8;
        case GLFW_KEY_F9: return KeyCode::F9;
        case GLFW_KEY_F10: return KeyCode::F10;
        case GLFW_KEY_F11: return KeyCode::F11;
        case GLFW_KEY_F12: return KeyCode::F12;

        case GLFW_KEY_ESCAPE: return KeyCode::Escape;
        case GLFW_KEY_ENTER: return KeyCode::Enter;
        case GLFW_KEY_TAB: return KeyCode::Tab;
        case GLFW_KEY_BACKSPACE: return KeyCode::Backspace;
        case GLFW_KEY_INSERT: return KeyCode::Insert;
        case GLFW_KEY_DELETE: return KeyCode::Delete;
        case GLFW_KEY_RIGHT: return KeyCode::Right;
        case GLFW_KEY_LEFT: return KeyCode::Left;
        case GLFW_KEY_DOWN: return KeyCode::Down;
        case GLFW_KEY_UP: return KeyCode::Up;
        case GLFW_KEY_PAGE_UP: return KeyCode::PageUp;
        case GLFW_KEY_PAGE_DOWN: return KeyCode::PageDown;
        case GLFW_KEY_HOME: return KeyCode::Home;
        case GLFW_KEY_END: return KeyCode::End;

        case GLFW_KEY_CAPS_LOCK: return KeyCode::CapsLock;
        case GLFW_KEY_SCROLL_LOCK: return KeyCode::ScrollLock;
        case GLFW_KEY_NUM_LOCK: return KeyCode::NumLock;
        case GLFW_KEY_PRINT_SCREEN: return KeyCode::PrintScreen;
        case GLFW_KEY_PAUSE: return KeyCode::Pause;

        case GLFW_KEY_KP_0: return KeyCode::KP0;
        case GLFW_KEY_KP_1: return KeyCode::KP1;
        case GLFW_KEY_KP_2: return KeyCode::KP2;
        case GLFW_KEY_KP_3: return KeyCode::KP3;
        case GLFW_KEY_KP_4: return KeyCode::KP4;
        case GLFW_KEY_KP_5: return KeyCode::KP5;
        case GLFW_KEY_KP_6: return KeyCode::KP6;
        case GLFW_KEY_KP_7: return KeyCode::KP7;
        case GLFW_KEY_KP_8: return KeyCode::KP8;
        case GLFW_KEY_KP_9: return KeyCode::KP9;
        case GLFW_KEY_KP_DECIMAL: return KeyCode::KPDecimal;
        case GLFW_KEY_KP_DIVIDE: return KeyCode::KPDivide;
        case GLFW_KEY_KP_MULTIPLY: return KeyCode::KPMultiply;
        case GLFW_KEY_KP_SUBTRACT: return KeyCode::KPSubtract;
        case GLFW_KEY_KP_ADD: return KeyCode::KPAdd;
        case GLFW_KEY_KP_ENTER: return KeyCode::KPEnter;
        case GLFW_KEY_KP_EQUAL: return KeyCode::KPEqual;

        case GLFW_KEY_LEFT_SHIFT: return KeyCode::LeftShift;
        case GLFW_KEY_LEFT_CONTROL: return KeyCode::LeftCtrl;
        case GLFW_KEY_LEFT_ALT: return KeyCode::LeftAlt;
        case GLFW_KEY_LEFT_SUPER: return KeyCode::LeftSuper;
        case GLFW_KEY_RIGHT_SHIFT: return KeyCode::RightShift;
        case GLFW_KEY_RIGHT_CONTROL: return KeyCode::RightCtrl;
        case GLFW_KEY_RIGHT_ALT: return KeyCode::RightAlt;
        case GLFW_KEY_RIGHT_SUPER: return KeyCode::RightSuper;
        case GLFW_KEY_MENU: return KeyCode::Menu;

        default: return KeyCode::Unknown;
    }
}

SDL_GameControllerButton gamepadButtonToSDL(GamepadButton button) {
    switch (button) {
        case GamepadButton::A: return SDL_CONTROLLER_BUTTON_A;
        case GamepadButton::B: return SDL_CONTROLLER_BUTTON_B;
        case GamepadButton::X: return SDL_CONTROLLER_BUTTON_X;
        case GamepadButton::Y: return SDL_CONTROLLER_BUTTON_Y;
        case GamepadButton::LeftBumper: return SDL_CONTROLLER_BUTTON_LEFTSHOULDER;
        case GamepadButton::RightBumper: return SDL_CONTROLLER_BUTTON_RIGHTSHOULDER;
        case GamepadButton::Back: return SDL_CONTROLLER_BUTTON_BACK;
        case GamepadButton::Start: return SDL_CONTROLLER_BUTTON_START;
        case GamepadButton::Guide: return SDL_CONTROLLER_BUTTON_GUIDE;
        case GamepadButton::LeftThumb: return SDL_CONTROLLER_BUTTON_LEFTSTICK;
        case GamepadButton::RightThumb: return SDL_CONTROLLER_BUTTON_RIGHTSTICK;
        case GamepadButton::DPadUp: return SDL_CONTROLLER_BUTTON_DPAD_UP;
        case GamepadButton::DPadRight: return SDL_CONTROLLER_BUTTON_DPAD_RIGHT;
        case GamepadButton::DPadDown: return SDL_CONTROLLER_BUTTON_DPAD_DOWN;
        case GamepadButton::DPadLeft: return SDL_CONTROLLER_BUTTON_DPAD_LEFT;
        default: return SDL_CONTROLLER_BUTTON_INVALID;
    }
}

GamepadButton sdlToGamepadButton(SDL_GameControllerButton button) {
    switch (button) {
        case SDL_CONTROLLER_BUTTON_A: return GamepadButton::A;
        case SDL_CONTROLLER_BUTTON_B: return GamepadButton::B;
        case SDL_CONTROLLER_BUTTON_X: return GamepadButton::X;
        case SDL_CONTROLLER_BUTTON_Y: return GamepadButton::Y;
        case SDL_CONTROLLER_BUTTON_LEFTSHOULDER: return GamepadButton::LeftBumper;
        case SDL_CONTROLLER_BUTTON_RIGHTSHOULDER: return GamepadButton::RightBumper;
        case SDL_CONTROLLER_BUTTON_BACK: return GamepadButton::Back;
        case SDL_CONTROLLER_BUTTON_START: return GamepadButton::Start;
        case SDL_CONTROLLER_BUTTON_GUIDE: return GamepadButton::Guide;
        case SDL_CONTROLLER_BUTTON_LEFTSTICK: return GamepadButton::LeftThumb;
        case SDL_CONTROLLER_BUTTON_RIGHTSTICK: return GamepadButton::RightThumb;
        case SDL_CONTROLLER_BUTTON_DPAD_UP: return GamepadButton::DPadUp;
        case SDL_CONTROLLER_BUTTON_DPAD_RIGHT: return GamepadButton::DPadRight;
        case SDL_CONTROLLER_BUTTON_DPAD_DOWN: return GamepadButton::DPadDown;
        case SDL_CONTROLLER_BUTTON_DPAD_LEFT: return GamepadButton::DPadLeft;
        default: return GamepadButton::A;  // Fallback
    }
}

SDL_GameControllerAxis gamepadAxisToSDL(GamepadAxis axis) {
    switch (axis) {
        case GamepadAxis::LeftX: return SDL_CONTROLLER_AXIS_LEFTX;
        case GamepadAxis::LeftY: return SDL_CONTROLLER_AXIS_LEFTY;
        case GamepadAxis::RightX: return SDL_CONTROLLER_AXIS_RIGHTX;
        case GamepadAxis::RightY: return SDL_CONTROLLER_AXIS_RIGHTY;
        case GamepadAxis::LeftTrigger: return SDL_CONTROLLER_AXIS_TRIGGERLEFT;
        case GamepadAxis::RightTrigger: return SDL_CONTROLLER_AXIS_TRIGGERRIGHT;
        default: return SDL_CONTROLLER_AXIS_INVALID;
    }
}

GamepadAxis sdlToGamepadAxis(SDL_GameControllerAxis axis) {
    switch (axis) {
        case SDL_CONTROLLER_AXIS_LEFTX: return GamepadAxis::LeftX;
        case SDL_CONTROLLER_AXIS_LEFTY: return GamepadAxis::LeftY;
        case SDL_CONTROLLER_AXIS_RIGHTX: return GamepadAxis::RightX;
        case SDL_CONTROLLER_AXIS_RIGHTY: return GamepadAxis::RightY;
        case SDL_CONTROLLER_AXIS_TRIGGERLEFT: return GamepadAxis::LeftTrigger;
        case SDL_CONTROLLER_AXIS_TRIGGERRIGHT: return GamepadAxis::RightTrigger;
        default: return GamepadAxis::LeftX;  // Fallback
    }
}

int mouseButtonToGLFW(MouseButton button) {
    return static_cast<int>(button);  // Direct mapping (GLFW_MOUSE_BUTTON_LEFT = 0, etc.)
}

MouseButton glfwToMouseButton(int glfwButton) {
    if (glfwButton >= 0 && glfwButton < static_cast<int>(MouseButton::Count)) {
        return static_cast<MouseButton>(glfwButton);
    }
    return MouseButton::Left;  // Fallback
}

//==========================================================================
// PhaseTree Implementation
//==========================================================================

namespace PhaseTree {

bool isDescendantOrSame(std::string_view childPhase, std::string_view parentPhase) {
    if (childPhase == parentPhase) return true;
    if (parentPhase.empty()) return true;  // Root is ancestor of all

    // Check if childPhase starts with parentPhase followed by a dot
    if (childPhase.size() > parentPhase.size() &&
        childPhase.substr(0, parentPhase.size()) == parentPhase &&
        childPhase[parentPhase.size()] == '.') {
        return true;
    }
    return false;
}

std::string getParent(std::string_view phase) {
    auto lastDot = phase.rfind('.');
    if (lastDot == std::string_view::npos) {
        return "";  // No parent (root level)
    }
    return std::string(phase.substr(0, lastDot));
}

std::vector<std::string> getAncestors(std::string_view phase) {
    std::vector<std::string> ancestors;
    std::string current(phase);

    while (!current.empty()) {
        ancestors.push_back(current);
        current = getParent(current);
    }

    return ancestors;
}

bool isValidPhase(std::string_view phase) {
    if (phase.empty()) return false;

    // Check for valid characters (alphanumeric, underscore, dot)
    for (char c : phase) {
        if (!std::isalnum(static_cast<unsigned char>(c)) && c != '_' && c != '.') {
            return false;
        }
    }

    // Check for consecutive dots or leading/trailing dots
    if (phase.front() == '.' || phase.back() == '.') return false;
    if (phase.find("..") != std::string_view::npos) return false;

    return true;
}

}  // namespace PhaseTree

//==========================================================================
// InputSystem Implementation
//==========================================================================

InputSystem::~InputSystem() {
    spdlog::debug("[InputSystem] Destructor called");
    for (auto* controller : controllers_) {
        if (controller) {
            SDL_GameControllerClose(controller);
        }
    }
    spdlog::debug("[InputSystem] Destructor complete");
    if (sdlInitialized_) {
        SDL_QuitSubSystem(SDL_INIT_GAMECONTROLLER);
    }
}

bool InputSystem::initialize(void* nativeWindow) {
    window_ = static_cast<GLFWwindow*>(nativeWindow);

    spdlog::info("[InputSystem] initialize() window_={}", (void*)window_);

    if (window_) {
        // Enable sticky keys so key presses between poll frames aren't lost.
        // With sticky keys, glfwGetKey() returns GLFW_PRESS until polled,
        // even if the key was released before the next poll.
        glfwSetInputMode(window_, GLFW_STICKY_KEYS, GLFW_TRUE);
        spdlog::info("[InputSystem] GLFW_STICKY_KEYS enabled");
    }

    // Initialize SDL2 for game controllers only
    if (SDL_InitSubSystem(SDL_INIT_GAMECONTROLLER) < 0) {
        return false;
    }
    sdlInitialized_ = true;

    // Load controller mappings from AssetSystem if available
    if (pIAssetSystem_) {
        AssetHandle mappingsHandle = pIAssetSystem_->registerAsset(
            AssetType::Data,
            "data/config/gamecontrollerdb.txt"
        );
        pIAssetSystem_->loadAsset(mappingsHandle);

        if (pIAssetSystem_->isLoaded(mappingsHandle)) {
            const DataAsset* data = static_cast<const DataAsset*>(
                pIAssetSystem_->getRawAsset(mappingsHandle)
            );
            if (data && !data->rawText.empty()) {
                SDL_RWops* rw = SDL_RWFromMem(
                    (void*)data->rawText.data(),
                    static_cast<int>(data->rawText.size())
                );
                if (rw) {
                    SDL_GameControllerAddMappingsFromRW(rw, 1);
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
    spdlog::debug("[InputSystem] shutdown() called");
    for (auto& controller : controllers_) {
        if (controller) {
            SDL_GameControllerClose(controller);
            controller = nullptr;
        }
    }

    if (sdlInitialized_) {
        SDL_QuitSubSystem(SDL_INIT_GAMECONTROLLER);
        sdlInitialized_ = false;
    }

    window_ = nullptr;
    spdlog::debug("[InputSystem] shutdown() complete");
}

void InputSystem::update() {
    // Calculate delta time (this should ideally be passed in, but we'll estimate)
    static auto lastTime = std::chrono::high_resolution_clock::now();
    auto now = std::chrono::high_resolution_clock::now();
    deltaTime_ = std::chrono::duration<float>(now - lastTime).count();
    lastTime = now;

    // Clamp delta time to avoid huge jumps
    if (deltaTime_ > 0.1f) deltaTime_ = 0.1f;

    // Save previous states
    prevActionStates_ = actionStates_;
    prevMousePosition_ = mousePosition_;
    prevKeyStates_ = keyStates_;
    for (int i = 0; i < 8; ++i) {
        prevMouseButtons_[i] = mouseButtons_[i];
    }
    for (int g = 0; g < 4; ++g) {
        prevGamepadButtons_[g] = gamepadButtons_[g];
    }

    // Reset frame-specific state
    scrollDelta_ = Vec2{0.0f, 0.0f};

    // Update raw input state
    updateModifierState();
    updateKeyboardState();
    updateMouseState();
    updateControllerState();

    // Update input state machine (JustPressed -> Pressed -> Held, etc.)
    updateInputStateTracking(deltaTime_);

    // Process action registrations (event-driven system)
    processActionRegistrations();

    // Update legacy action states (for backwards compatibility)
    updateActionStates();
}

//==========================================================================
// Phase Management
//==========================================================================

std::string InputSystem::getCurrentPhase() const {
    if (phaseStack_.empty()) {
        return "";
    }
    return phaseStack_.back();
}

std::vector<std::string> InputSystem::getPhaseStack() const {
    return phaseStack_;
}

void InputSystem::pushPhase(const std::string& phase) {
    if (!PhaseTree::isValidPhase(phase)) {
        return;
    }

    std::string oldPhase = getCurrentPhase();
    phaseStack_.push_back(phase);
    phaseHasBeenSet_ = true;

    // Emit phase pushed event
    if (pIEventSystem_) {
        PhaseEventData eventData{
            .oldPhase = oldPhase,
            .newPhase = phase,
            .phaseStack = phaseStack_
        };
        pIEventSystem_->publish(Events::PhasePushed, eventData);
    }
}

void InputSystem::popPhase() {
    // Don't pop if empty or if only the base phase remains
    // (base phase is the one set by changePhase, should not be poppable)
    if (phaseStack_.size() <= 1) {
        return;
    }

    std::string oldPhase = phaseStack_.back();
    phaseStack_.pop_back();
    std::string newPhase = getCurrentPhase();

    // Emit phase popped event
    if (pIEventSystem_) {
        PhaseEventData eventData{
            .oldPhase = oldPhase,
            .newPhase = newPhase,
            .phaseStack = phaseStack_
        };
        pIEventSystem_->publish(Events::PhasePopped, eventData);
    }
}

void InputSystem::changePhase(const std::string& phase) {
    if (!PhaseTree::isValidPhase(phase)) {
        return;
    }

    std::string oldPhase = getCurrentPhase();
    phaseStack_.clear();
    phaseStack_.push_back(phase);
    phaseHasBeenSet_ = true;

    // Clear discrete fired set when phase changes
    discreteFired_.clear();

    // Emit phase changed event
    if (pIEventSystem_) {
        PhaseEventData eventData{
            .oldPhase = oldPhase,
            .newPhase = phase,
            .phaseStack = phaseStack_
        };
        pIEventSystem_->publish(Events::PhaseChanged, eventData);
    }
}

bool InputSystem::isPhaseActive(const std::string& phase) const {
    std::string current = getCurrentPhase();
    if (current.empty()) return false;

    // Check if current phase is a descendant of (or same as) the given phase
    return PhaseTree::isDescendantOrSame(current, phase);
}

bool InputSystem::hasPhaseBeenSet() const {
    return phaseHasBeenSet_;
}

//==========================================================================
// Action Registration
//==========================================================================

void InputSystem::registerAction(const ActionRegistration& registration) {
    actionRegistrations_.push_back(registration);
}

void InputSystem::unregisterAction(const std::string& actionName) {
    actionRegistrations_.erase(
        std::remove_if(actionRegistrations_.begin(), actionRegistrations_.end(),
            [&](const ActionRegistration& reg) {
                for (const auto& effect : reg.effects) {
                    if (effect.type == ActionEffectType::EmitAction &&
                        effect.value == actionName) {
                        return true;
                    }
                }
                return false;
            }),
        actionRegistrations_.end());
}

void InputSystem::unregisterPhaseActions(const std::string& phase) {
    actionRegistrations_.erase(
        std::remove_if(actionRegistrations_.begin(), actionRegistrations_.end(),
            [&](const ActionRegistration& reg) {
                return reg.phase == phase;
            }),
        actionRegistrations_.end());
}

void InputSystem::clearActions() {
    actionRegistrations_.clear();
    discreteFired_.clear();
}

std::vector<ActionRegistration> InputSystem::getActions() const {
    return actionRegistrations_;
}

//==========================================================================
// Input State Queries
//==========================================================================

InputState InputSystem::getInputState(const InputBinding& binding) const {
    std::string key = serializeBinding(binding);
    auto it = inputStates_.find(key);
    if (it != inputStates_.end()) {
        return it->second;
    }
    return InputState::NotPressed;
}

float InputSystem::getInputHoldDuration(const InputBinding& binding) const {
    std::string key = serializeBinding(binding);
    auto it = holdDurations_.find(key);
    if (it != holdDurations_.end()) {
        return it->second;
    }
    return 0.0f;
}

void InputSystem::setDefaultHoldThreshold(float seconds) {
    defaultHoldThreshold_ = seconds;
}

float InputSystem::getDefaultHoldThreshold() const {
    return defaultHoldThreshold_;
}

//==========================================================================
// Configuration Loading
//==========================================================================

bool InputSystem::loadInputConfig(const std::string& path) {
    // Check if the file exists before storing the path
    if (!std::filesystem::exists(path)) {
        return false;
    }

    inputConfigPath_ = path;
    // Note: Actual Lua loading happens in the Lua bindings layer
    // This method just stores the path for potential reload
    return true;
}

bool InputSystem::reloadInputConfig() {
    if (inputConfigPath_.empty()) {
        return false;
    }
    // Clear existing registrations and reload
    clearActions();
    return loadInputConfig(inputConfigPath_);
}

//==========================================================================
// Input State Tracking Helpers
//==========================================================================

std::string InputSystem::serializeBinding(const InputBinding& binding) const {
    std::string result;
    result += std::to_string(static_cast<int>(binding.source));
    result += ":";
    result += std::to_string(binding.deviceIndex);
    result += ":";

    // Serialize the variant
    std::visit([&result](auto&& arg) {
        using T = std::decay_t<decltype(arg)>;
        if constexpr (std::is_same_v<T, KeyCode>) {
            result += "k" + std::to_string(static_cast<int>(arg));
        } else if constexpr (std::is_same_v<T, MouseButton>) {
            result += "m" + std::to_string(static_cast<int>(arg));
        } else if constexpr (std::is_same_v<T, GamepadButton>) {
            result += "b" + std::to_string(static_cast<int>(arg));
        } else if constexpr (std::is_same_v<T, GamepadAxis>) {
            result += "a" + std::to_string(static_cast<int>(arg));
        }
    }, binding.input);

    return result;
}

float InputSystem::getRawInputValue(const InputBinding& binding) const {
    return std::visit([this, &binding](auto&& arg) -> float {
        using T = std::decay_t<decltype(arg)>;

        if constexpr (std::is_same_v<T, KeyCode>) {
            int glfwKey = keyCodeToGLFW(arg);
            auto it = keyStates_.find(glfwKey);
            bool pressed = (it != keyStates_.end() && it->second);

            // Check modifiers
            if (pressed && binding.requiredModifiers != ModifierKey::None) {
                if (!hasModifier(currentModifiers_, binding.requiredModifiers)) {
                    return 0.0f;
                }
            }
            return pressed ? binding.scale : 0.0f;

        } else if constexpr (std::is_same_v<T, MouseButton>) {
            int idx = static_cast<int>(arg);
            if (idx >= 0 && idx < 8) {
                return mouseButtons_[idx] ? binding.scale : 0.0f;
            }
            return 0.0f;

        } else if constexpr (std::is_same_v<T, GamepadButton>) {
            int idx = binding.deviceIndex;
            if (idx >= 0 && idx < 4) {
                int btnIdx = static_cast<int>(arg);
                if (btnIdx >= 0 && btnIdx < static_cast<int>(GamepadButton::Count)) {
                    return gamepadButtons_[idx][btnIdx] ? binding.scale : 0.0f;
                }
            }
            return 0.0f;

        } else if constexpr (std::is_same_v<T, GamepadAxis>) {
            int idx = binding.deviceIndex;
            if (idx >= 0 && idx < 4) {
                int axisIdx = static_cast<int>(arg);
                if (axisIdx >= 0 && axisIdx < static_cast<int>(GamepadAxis::Count)) {
                    float value = gamepadAxes_[idx][axisIdx];
                    // Apply deadzone
                    if (std::abs(value) < binding.deadzone) {
                        return 0.0f;
                    }
                    return value * binding.scale;
                }
            }
            return 0.0f;
        }

        return 0.0f;
    }, binding.input);
}

void InputSystem::updateInputStateTracking(float dt) {
    // Update state machine for all tracked inputs
    // We need to track inputs that are used in action registrations

    std::unordered_set<std::string> activeBindings;

    // Collect all bindings used in registrations
    for (const auto& reg : actionRegistrations_) {
        for (const auto& cond : reg.conditions) {
            activeBindings.insert(serializeBinding(cond.input));
        }
    }

    // Update state for each binding
    for (const auto& bindingKey : activeBindings) {
        // Find the binding in a registration (to get the actual binding)
        InputBinding binding;
        bool foundBinding = false;

        for (const auto& reg : actionRegistrations_) {
            for (const auto& cond : reg.conditions) {
                if (serializeBinding(cond.input) == bindingKey) {
                    binding = cond.input;
                    foundBinding = true;
                    break;
                }
            }
            if (foundBinding) break;
        }

        if (!foundBinding) continue;

        float rawValue = getRawInputValue(binding);
        bool isDown = std::abs(rawValue) > 0.01f;

        InputState& state = inputStates_[bindingKey];
        float& duration = holdDurations_[bindingKey];

        InputState prevState = state;

        if (isDown) {
            switch (state) {
                case InputState::NotPressed:
                case InputState::JustReleased:
                    state = InputState::JustPressed;
                    duration = 0.0f;
                    break;

                case InputState::JustPressed:
                    state = InputState::Pressed;
                    duration += dt;
                    break;

                case InputState::Pressed:
                    duration += dt;
                    if (duration >= defaultHoldThreshold_) {
                        state = InputState::Held;
                    }
                    break;

                case InputState::Held:
                    duration += dt;
                    break;
            }
        } else {
            switch (state) {
                case InputState::JustPressed:
                case InputState::Pressed:
                case InputState::Held:
                    state = InputState::JustReleased;
                    break;

                case InputState::JustReleased:
                    state = InputState::NotPressed;
                    duration = 0.0f;
                    break;

                case InputState::NotPressed:
                    // Stay not pressed
                    break;
            }
        }
    }
}

bool InputSystem::checkCondition(const ActionCondition& condition) const {
    std::string key = serializeBinding(condition.input);
    auto it = inputStates_.find(key);
    InputState state = (it != inputStates_.end()) ? it->second : InputState::NotPressed;

    switch (condition.type) {
        case ActionConditionType::WhenPressed:
            return state == InputState::JustPressed;

        case ActionConditionType::WhenReleased:
            return state == InputState::JustReleased;

        case ActionConditionType::WhenActive:
            return state == InputState::JustPressed ||
                   state == InputState::Pressed ||
                   state == InputState::Held;

        case ActionConditionType::WhenInactive:
            return state == InputState::NotPressed ||
                   state == InputState::JustReleased;

        case ActionConditionType::WhenHeld: {
            // Check if we just transitioned to held
            // Or if we're held and this is continuous
            if (state == InputState::Held) {
                float duration = getInputHoldDuration(condition.input);
                float threshold = condition.holdThreshold.value_or(defaultHoldThreshold_);
                // This is tricky - we want to fire once when duration crosses threshold
                // For now, check if held
                return duration >= threshold;
            }
            return false;
        }
    }

    return false;
}

bool InputSystem::checkAllConditions(const ActionRegistration& reg) const {
    if (reg.conditions.empty()) {
        return false;
    }

    for (const auto& cond : reg.conditions) {
        if (!checkCondition(cond)) {
            return false;
        }
    }
    return true;
}

void InputSystem::executeEffects(const ActionRegistration& reg) {
    for (const auto& effect : reg.effects) {
        switch (effect.type) {
            case ActionEffectType::EmitAction: {
                // Determine source from first condition
                InputSource source = InputSource::Keyboard;
                float duration = 0.0f;
                Vec2 axis{0.0f, 0.0f};

                if (!reg.conditions.empty()) {
                    source = reg.conditions[0].input.source;
                    duration = getInputHoldDuration(reg.conditions[0].input);

                    // For axis inputs, get the value
                    if (std::holds_alternative<GamepadAxis>(reg.conditions[0].input.input)) {
                        auto axisType = std::get<GamepadAxis>(reg.conditions[0].input.input);
                        int idx = reg.conditions[0].input.deviceIndex;
                        float value = getGamepadAxisValue(axisType, idx);

                        // For stick axes, combine X and Y
                        if (axisType == GamepadAxis::LeftX || axisType == GamepadAxis::LeftY) {
                            axis = getLeftStick(idx);
                        } else if (axisType == GamepadAxis::RightX || axisType == GamepadAxis::RightY) {
                            axis = getRightStick(idx);
                        } else {
                            axis = Vec2{value, 0.0f};
                        }
                    }
                }

                emitActionEvent(effect.value, source, duration, axis);
                break;
            }

            case ActionEffectType::PushPhase:
                pushPhase(effect.value);
                break;

            case ActionEffectType::PopPhase:
                popPhase();
                break;

            case ActionEffectType::ChangePhase:
                changePhase(effect.value);
                break;
        }
    }
}

void InputSystem::emitActionEvent(const std::string& actionName, InputSource source,
                                   float duration, const Vec2& axis) {
    if (!pIEventSystem_) return;

    ActionEventData eventData{
        .action = actionName,
        .phase = getCurrentPhase(),
        .source = source,
        .playerIndex = 0,  // TODO: Support multiplayer
        .duration = duration,
        .axis = axis
    };

    // Publish to "action:<Name>" for Lua subscribers (e.g., "action:MenuUp")
    pIEventSystem_->publish("action:" + actionName, eventData);

    // Also publish to generic "action_triggered" for C++ subscribers
    pIEventSystem_->publish(Events::ActionTriggered, eventData);
}

void InputSystem::processActionRegistrations() {
    // Snapshot the phase at frame start. Actions that fire during this loop may
    // push/pop phases (e.g., Pause pushes the pause scene), but we must evaluate
    // ALL actions against the phase that was active when the frame began.
    // Otherwise, an action in the new phase (e.g., Resume) can fire on the same
    // key press that triggered the phase change, immediately undoing it.
    std::string snapshotPhase = getCurrentPhase();
    if (snapshotPhase.empty()) {
        return;  // No phase set, don't process any actions
    }

    for (const auto& reg : actionRegistrations_) {
        // Check if this registration applies to the snapshot phase
        if (!PhaseTree::isDescendantOrSame(snapshotPhase, reg.phase)) {
            continue;
        }

        bool conditionsMet = checkAllConditions(reg);

        if (reg.terminal == ActionTerminal::Discrete) {
            // Discrete: fire once when conditions become true
            const ActionRegistration* regPtr = &reg;

            if (conditionsMet) {
                if (discreteFired_.find(regPtr) == discreteFired_.end()) {
                    // Not yet fired - execute and mark as fired
                    executeEffects(reg);
                    discreteFired_.insert(regPtr);
                }
            } else {
                // Conditions no longer met - allow re-firing
                discreteFired_.erase(regPtr);
            }
        } else {
            // Continuous: fire every frame while conditions are true
            if (conditionsMet) {
                executeEffects(reg);
            }
        }
    }
}

//==========================================================================
// Raw Input Update Methods
//==========================================================================

void InputSystem::updateKeyboardState() {
    if (!window_) return;

    for (int key = GLFW_KEY_SPACE; key <= GLFW_KEY_LAST; ++key) {
        bool pressed = glfwGetKey(window_, key) == GLFW_PRESS;
        keyStates_[key] = pressed;

        if (isListening_ && pressed && !prevKeyStates_[key]) {
            KeyCode keyCode = glfwToKeyCode(key);
            lastInput_ = InputBinding{
                .source = InputSource::Keyboard,
                .deviceIndex = 0,
                .input = keyCode,
                .requiredModifiers = ModifierKey::None,
                .scale = 1.0f,
                .deadzone = 0.0f
            };
            isListening_ = false;
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

        if (isListening_ && mouseButtons_[i] && !wasDown) {
            lastInput_ = InputBinding{
                .source = InputSource::Mouse,
                .deviceIndex = 0,
                .input = glfwToMouseButton(i),
                .requiredModifiers = ModifierKey::None,
                .scale = 1.0f,
                .deadzone = 0.0f
            };
            isListening_ = false;
            return;
        }
    }
}

void InputSystem::updateControllerState() {
    // Poll gamepad state directly for better responsiveness
    for (int g = 0; g < 4; ++g) {
        if (!controllers_[g]) continue;

        // Update buttons
        for (int b = 0; b < static_cast<int>(GamepadButton::Count); ++b) {
            GamepadButton btn = static_cast<GamepadButton>(b);
            SDL_GameControllerButton sdlBtn = gamepadButtonToSDL(btn);

            if (sdlBtn != SDL_CONTROLLER_BUTTON_INVALID) {
                gamepadButtons_[g][b] = SDL_GameControllerGetButton(controllers_[g], sdlBtn) != 0;
            } else {
                // Handle trigger buttons (digital version of analog triggers)
                if (btn == GamepadButton::LeftTrigger) {
                    float triggerValue = SDL_GameControllerGetAxis(controllers_[g], SDL_CONTROLLER_AXIS_TRIGGERLEFT) / 32767.0f;
                    gamepadButtons_[g][b] = triggerValue > 0.5f;
                } else if (btn == GamepadButton::RightTrigger) {
                    float triggerValue = SDL_GameControllerGetAxis(controllers_[g], SDL_CONTROLLER_AXIS_TRIGGERRIGHT) / 32767.0f;
                    gamepadButtons_[g][b] = triggerValue > 0.5f;
                }
            }
        }

        // Update axes
        for (int a = 0; a < static_cast<int>(GamepadAxis::Count); ++a) {
            GamepadAxis axis = static_cast<GamepadAxis>(a);
            SDL_GameControllerAxis sdlAxis = gamepadAxisToSDL(axis);

            if (sdlAxis != SDL_CONTROLLER_AXIS_INVALID) {
                float value = SDL_GameControllerGetAxis(controllers_[g], sdlAxis) / 32767.0f;
                gamepadAxes_[g][a] = value;
            }
        }
    }

    // Process SDL events for device connect/disconnect and input listening
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
                        GamepadButton btn = sdlToGamepadButton(
                            static_cast<SDL_GameControllerButton>(event.cbutton.button));
                        lastInput_ = InputBinding{
                            .source = InputSource::Gamepad,
                            .deviceIndex = controllerIndex,
                            .input = btn,
                            .requiredModifiers = ModifierKey::None,
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
                            GamepadAxis axis = sdlToGamepadAxis(
                                static_cast<SDL_GameControllerAxis>(event.caxis.axis));
                            lastInput_ = InputBinding{
                                .source = InputSource::Gamepad,
                                .deviceIndex = controllerIndex,
                                .input = axis,
                                .requiredModifiers = ModifierKey::None,
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

void InputSystem::updateModifierState() {
    if (!window_) {
        currentModifiers_ = ModifierKey::None;
        return;
    }

    currentModifiers_ = ModifierKey::None;

    if (glfwGetKey(window_, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS ||
        glfwGetKey(window_, GLFW_KEY_RIGHT_SHIFT) == GLFW_PRESS) {
        currentModifiers_ |= ModifierKey::Shift;
    }

    if (glfwGetKey(window_, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS ||
        glfwGetKey(window_, GLFW_KEY_RIGHT_CONTROL) == GLFW_PRESS) {
        currentModifiers_ |= ModifierKey::Ctrl;
    }

    if (glfwGetKey(window_, GLFW_KEY_LEFT_ALT) == GLFW_PRESS ||
        glfwGetKey(window_, GLFW_KEY_RIGHT_ALT) == GLFW_PRESS) {
        currentModifiers_ |= ModifierKey::Alt;
    }

    if (glfwGetKey(window_, GLFW_KEY_LEFT_SUPER) == GLFW_PRESS ||
        glfwGetKey(window_, GLFW_KEY_RIGHT_SUPER) == GLFW_PRESS) {
        currentModifiers_ |= ModifierKey::Super;
    }

    if (glfwGetKey(window_, GLFW_KEY_CAPS_LOCK) == GLFW_PRESS) {
        currentModifiers_ |= ModifierKey::CapsLock;
    }

    if (glfwGetKey(window_, GLFW_KEY_NUM_LOCK) == GLFW_PRESS) {
        currentModifiers_ |= ModifierKey::NumLock;
    }
}

//==========================================================================
// Legacy Action State (Deprecated)
//==========================================================================

void InputSystem::updateActionStates() {
    std::unordered_map<Action, bool> wasActiveMap;
    for (auto& [action, state] : actionStates_) {
        wasActiveMap[action] = state.active;
        state.value = 0.0f;
        state.active = false;
    }

    // Process legacy mappings
    for (const auto& mapping : mappings_) {
        auto& state = actionStates_[mapping.action];
        state.action = mapping.action;

        float bindingValue = getRawInputValue(mapping.binding);

        if (std::abs(bindingValue) > std::abs(state.value)) {
            state.value = bindingValue;
        }
    }

    // Also process new ActionBuilder registrations for isActionActive() compatibility
    std::string currentPhase = getCurrentPhase();
    if (!currentPhase.empty()) {
        for (const auto& reg : actionRegistrations_) {
            // Only process if phase matches
            if (!isPhaseActive(reg.phase)) {
                continue;
            }

            // Find the emitted action name
            std::string actionName;
            for (const auto& effect : reg.effects) {
                if (effect.type == ActionEffectType::EmitAction) {
                    actionName = effect.value;
                    break;
                }
            }
            if (actionName.empty()) continue;

            // Check if conditions are met
            bool conditionsMet = checkAllConditions(reg);

            auto& state = actionStates_[actionName];
            state.action = actionName;

            if (conditionsMet) {
                // Set value to 1.0 for digital inputs, actual value for analog
                float value = 1.0f;
                if (!reg.conditions.empty()) {
                    const auto& binding = reg.conditions[0].input;
                    if (std::holds_alternative<GamepadAxis>(binding.input)) {
                        auto axisType = std::get<GamepadAxis>(binding.input);
                        value = getGamepadAxisValue(axisType, binding.deviceIndex);
                    }
                }
                if (std::abs(value) > std::abs(state.value)) {
                    state.value = value;
                }
            }
        }
    }

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
    std::string targetKey = serializeBinding(binding);
    mappings_.erase(
        std::remove_if(mappings_.begin(), mappings_.end(),
            [&](const InputMapping& m) {
                return serializeBinding(m.binding) == targetKey;
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

//==========================================================================
// Raw Input Queries
//==========================================================================

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

//==========================================================================
// Mouse State (Platform-Agnostic)
//==========================================================================

Vec2 InputSystem::getMousePosition() const {
    return mousePosition_;
}

Vec2 InputSystem::getMouseDelta() const {
    return mousePosition_ - prevMousePosition_;
}

bool InputSystem::isMouseButtonDown(MouseButton button) const {
    int idx = static_cast<int>(button);
    return idx >= 0 && idx < 8 && mouseButtons_[idx];
}

bool InputSystem::wasMouseButtonJustPressed(MouseButton button) const {
    int idx = static_cast<int>(button);
    if (idx < 0 || idx >= 8) return false;
    return mouseButtons_[idx] && !prevMouseButtons_[idx];
}

bool InputSystem::wasMouseButtonJustReleased(MouseButton button) const {
    int idx = static_cast<int>(button);
    if (idx < 0 || idx >= 8) return false;
    return !mouseButtons_[idx] && prevMouseButtons_[idx];
}

//==========================================================================
// Modifier Keys
//==========================================================================

ModifierKey InputSystem::getModifierState() const {
    return currentModifiers_;
}

bool InputSystem::isModifierPressed(ModifierKey mod) const {
    return hasModifier(currentModifiers_, mod);
}

bool InputSystem::isShiftPressed() const {
    return hasModifier(currentModifiers_, ModifierKey::Shift);
}

bool InputSystem::isCtrlPressed() const {
    return hasModifier(currentModifiers_, ModifierKey::Ctrl);
}

bool InputSystem::isAltPressed() const {
    return hasModifier(currentModifiers_, ModifierKey::Alt);
}

bool InputSystem::isSuperPressed() const {
    return hasModifier(currentModifiers_, ModifierKey::Super);
}

//==========================================================================
// Keyboard State (Platform-Agnostic)
//==========================================================================

bool InputSystem::isKeyDown(KeyCode key) const {
    int glfwKey = keyCodeToGLFW(key);
    auto it = keyStates_.find(glfwKey);
    return it != keyStates_.end() && it->second;
}

bool InputSystem::wasKeyJustPressed(KeyCode key) const {
    int glfwKey = keyCodeToGLFW(key);
    auto currIt = keyStates_.find(glfwKey);
    auto prevIt = prevKeyStates_.find(glfwKey);
    bool currentlyDown = currIt != keyStates_.end() && currIt->second;
    bool wasDown = prevIt != prevKeyStates_.end() && prevIt->second;
    return currentlyDown && !wasDown;
}

bool InputSystem::wasKeyJustReleased(KeyCode key) const {
    int glfwKey = keyCodeToGLFW(key);
    auto currIt = keyStates_.find(glfwKey);
    auto prevIt = prevKeyStates_.find(glfwKey);
    bool currentlyDown = currIt != keyStates_.end() && currIt->second;
    bool wasDown = prevIt != prevKeyStates_.end() && prevIt->second;
    return !currentlyDown && wasDown;
}

//==========================================================================
// Gamepad State (Platform-Agnostic)
//==========================================================================

bool InputSystem::isGamepadButtonDown(GamepadButton button, int gamepadIndex) const {
    if (gamepadIndex < 0 || gamepadIndex >= 4) return false;
    int idx = static_cast<int>(button);
    if (idx < 0 || idx >= static_cast<int>(GamepadButton::Count)) return false;
    return gamepadButtons_[gamepadIndex][idx];
}

bool InputSystem::wasGamepadButtonJustPressed(GamepadButton button, int gamepadIndex) const {
    if (gamepadIndex < 0 || gamepadIndex >= 4) return false;
    int idx = static_cast<int>(button);
    if (idx < 0 || idx >= static_cast<int>(GamepadButton::Count)) return false;
    return gamepadButtons_[gamepadIndex][idx] && !prevGamepadButtons_[gamepadIndex][idx];
}

bool InputSystem::wasGamepadButtonJustReleased(GamepadButton button, int gamepadIndex) const {
    if (gamepadIndex < 0 || gamepadIndex >= 4) return false;
    int idx = static_cast<int>(button);
    if (idx < 0 || idx >= static_cast<int>(GamepadButton::Count)) return false;
    return !gamepadButtons_[gamepadIndex][idx] && prevGamepadButtons_[gamepadIndex][idx];
}

float InputSystem::getGamepadAxisValue(GamepadAxis axis, int gamepadIndex) const {
    if (gamepadIndex < 0 || gamepadIndex >= 4) return 0.0f;
    int idx = static_cast<int>(axis);
    if (idx < 0 || idx >= static_cast<int>(GamepadAxis::Count)) return 0.0f;
    return gamepadAxes_[gamepadIndex][idx];
}

Vec2 InputSystem::getLeftStick(int gamepadIndex) const {
    return Vec2{
        getGamepadAxisValue(GamepadAxis::LeftX, gamepadIndex),
        getGamepadAxisValue(GamepadAxis::LeftY, gamepadIndex)
    };
}

Vec2 InputSystem::getRightStick(int gamepadIndex) const {
    return Vec2{
        getGamepadAxisValue(GamepadAxis::RightX, gamepadIndex),
        getGamepadAxisValue(GamepadAxis::RightY, gamepadIndex)
    };
}

//==========================================================================
// Other
//==========================================================================

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
    scrollDelta_.x += static_cast<float>(xoffset);
    scrollDelta_.y += static_cast<float>(yoffset);
}

void InputSystem::onCharCallback(unsigned int codepoint) {
    if (!textInputEnabled_) return;

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

//==========================================================================
// Cursor Control
//==========================================================================

void InputSystem::showMouseCursor() {
    setCursorMode(CursorMode::Normal);
}

void InputSystem::hideMouseCursor() {
    setCursorMode(CursorMode::Hidden);
}

bool InputSystem::isMouseCursorVisible() const {
    return cursorMode_ == CursorMode::Normal;
}

void InputSystem::setCursorMode(CursorMode mode) {
    cursorMode_ = mode;

    // No window means we can't apply to GLFW, but still store the mode
    if (!window_) return;

    int glfwMode = GLFW_CURSOR_NORMAL;
    switch (mode) {
        case CursorMode::Normal:
            glfwMode = GLFW_CURSOR_NORMAL;
            break;
        case CursorMode::Hidden:
            glfwMode = GLFW_CURSOR_HIDDEN;
            break;
        case CursorMode::Disabled:
            glfwMode = GLFW_CURSOR_DISABLED;
            break;
    }

    glfwSetInputMode(window_, GLFW_CURSOR, glfwMode);
}

CursorMode InputSystem::getCursorMode() const {
    return cursorMode_;
}

}  // namespace bestow

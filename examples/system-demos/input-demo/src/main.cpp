// examples/system-demos/input-demo/src/main.cpp
// Comprehensive Input System API demonstration

// Third-party headers MUST come BEFORE 'import std;' for MSVC C++23 module compatibility.
// On Windows, mixing 'import std;' with CRT headers causes type redefinition errors.

// Prevent SDL from redefining main to SDL_main on Windows
// (we're using GLFW for window management, SDL only for controller input)
#define SDL_MAIN_HANDLED

#include <kangaru/kangaru.hpp>
#include <GLFW/glfw3.h>
#include <SDL.h>

import std;
import bestow.types;
import bestow.input;
import bestow.input.impl;

using namespace bestow;

// Demo mode
enum class DemoMode {
    Normal,
    Rebinding
};

// Helper to print input device type
const char* deviceTypeToString(InputDeviceType type) {
    switch (type) {
        case InputDeviceType::Keyboard: return "Keyboard";
        case InputDeviceType::Mouse: return "Mouse";
        case InputDeviceType::Controller: return "Controller";
        default: return "Unknown";
    }
}

// Helper to print key name (simplified)
std::string keyCodeToString(InputDeviceType type, int keyCode) {
    if (type == InputDeviceType::Keyboard) {
        // Common GLFW key codes
        switch (keyCode) {
            case 32: return "Space";
            case 65: return "A";
            case 68: return "D";
            case 83: return "S";
            case 87: return "W";
            case 256: return "Escape";
            case 257: return "Enter";
            case 262: return "Right Arrow";
            case 263: return "Left Arrow";
            case 264: return "Down Arrow";
            case 265: return "Up Arrow";
            case 340: return "Left Shift";
            case 341: return "Left Ctrl";
            default: return "Key " + std::to_string(keyCode);
        }
    } else if (type == InputDeviceType::Mouse) {
        switch (keyCode) {
            case 0: return "Left Mouse";
            case 1: return "Right Mouse";
            case 2: return "Middle Mouse";
            default: return "Mouse " + std::to_string(keyCode);
        }
    } else {
        // Controller buttons
        if (keyCode < SDL_CONTROLLER_BUTTON_MAX) {
            switch (keyCode) {
                case SDL_CONTROLLER_BUTTON_A: return "A Button";
                case SDL_CONTROLLER_BUTTON_B: return "B Button";
                case SDL_CONTROLLER_BUTTON_X: return "X Button";
                case SDL_CONTROLLER_BUTTON_Y: return "Y Button";
                case SDL_CONTROLLER_BUTTON_BACK: return "Back";
                case SDL_CONTROLLER_BUTTON_GUIDE: return "Guide";
                case SDL_CONTROLLER_BUTTON_START: return "Start";
                case SDL_CONTROLLER_BUTTON_LEFTSTICK: return "Left Stick";
                case SDL_CONTROLLER_BUTTON_RIGHTSTICK: return "Right Stick";
                case SDL_CONTROLLER_BUTTON_LEFTSHOULDER: return "Left Bumper";
                case SDL_CONTROLLER_BUTTON_RIGHTSHOULDER: return "Right Bumper";
                case SDL_CONTROLLER_BUTTON_DPAD_UP: return "D-Pad Up";
                case SDL_CONTROLLER_BUTTON_DPAD_DOWN: return "D-Pad Down";
                case SDL_CONTROLLER_BUTTON_DPAD_LEFT: return "D-Pad Left";
                case SDL_CONTROLLER_BUTTON_DPAD_RIGHT: return "D-Pad Right";
                default: return "Button " + std::to_string(keyCode);
            }
        } else {
            // Axis
            int axis = keyCode - SDL_CONTROLLER_BUTTON_MAX;
            switch (axis) {
                case SDL_CONTROLLER_AXIS_LEFTX: return "Left Stick X";
                case SDL_CONTROLLER_AXIS_LEFTY: return "Left Stick Y";
                case SDL_CONTROLLER_AXIS_RIGHTX: return "Right Stick X";
                case SDL_CONTROLLER_AXIS_RIGHTY: return "Right Stick Y";
                case SDL_CONTROLLER_AXIS_TRIGGERLEFT: return "Left Trigger";
                case SDL_CONTROLLER_AXIS_TRIGGERRIGHT: return "Right Trigger";
                default: return "Axis " + std::to_string(axis);
            }
        }
    }
}

// Print all registered mappings
void printMappings(IInputSystem& input) {
    std::cout << "\n=== Registered Input Mappings ===\n";
    auto mappings = input.getMappings();
    if (mappings.empty()) {
        std::cout << "  (No mappings registered)\n";
        return;
    }

    // Group by action
    std::map<Action, std::vector<InputBinding>> actionMap;
    for (const auto& mapping : mappings) {
        actionMap[mapping.action].push_back(mapping.binding);
    }

    for (const auto& [action, bindings] : actionMap) {
        std::cout << "  " << action << ":\n";
        for (const auto& binding : bindings) {
            std::cout << "    - " << deviceTypeToString(binding.deviceType);
            if (binding.deviceType == InputDeviceType::Controller) {
                std::cout << " [#" << binding.deviceIndex << "]";
            }
            std::cout << " " << keyCodeToString(binding.deviceType, binding.keyCode);
            if (binding.scale != 1.0f) {
                std::cout << " (scale: " << binding.scale << ")";
            }
            if (binding.deadzone > 0.0f) {
                std::cout << " (deadzone: " << binding.deadzone << ")";
            }
            std::cout << "\n";
        }
    }
}

// Print current action states
void printActionStates(IInputSystem& input) {
    auto allStates = input.getAllActionStates();
    if (allStates.empty()) {
        return;
    }

    std::cout << "\n=== Action States ===\n";
    for (const auto& state : allStates) {
        if (state.active || state.justPressed || state.justReleased) {
            std::cout << "  " << state.action << ": ";
            if (state.justPressed) {
                std::cout << "[JUST PRESSED] ";
            }
            if (state.justReleased) {
                std::cout << "[JUST RELEASED] ";
            }
            if (state.active) {
                std::cout << "ACTIVE (value: " << state.value << ")";
            }
            std::cout << "\n";
        }
    }
}

// Print mouse state
void printMouseState(IInputSystem& input) {
    auto pos = input.getMousePosition();
    auto delta = input.getMouseDelta();

    std::cout << "\n=== Mouse State ===\n";
    std::cout << "  Position: (" << pos.x << ", " << pos.y << ")\n";
    std::cout << "  Delta: (" << delta.x << ", " << delta.y << ")\n";
    std::cout << "  Buttons: ";

    bool anyPressed = false;
    for (int i = 0; i < 3; ++i) {
        if (input.isMouseButtonDown(i)) {
            if (anyPressed) std::cout << ", ";
            std::cout << keyCodeToString(InputDeviceType::Mouse, i);
            anyPressed = true;
        }
    }
    if (!anyPressed) {
        std::cout << "(none)";
    }
    std::cout << "\n";
}

// Print controller state
void printControllerState(IInputSystem& input) {
    int count = input.getConnectedControllerCount();
    std::cout << "\n=== Controller State ===\n";
    std::cout << "  Connected Controllers: " << count << "\n";

    for (int i = 0; i < 4; ++i) {
        if (input.isControllerConnected(i)) {
            std::cout << "  Controller #" << i << ": " << input.getControllerName(i) << "\n";
        }
    }
}

// Print instructions
void printInstructions() {
    std::cout << "\n";
    std::cout << "========================================\n";
    std::cout << "  Bestow Input System Demo\n";
    std::cout << "========================================\n\n";
    std::cout << "This demo exercises the ENTIRE Input System API.\n\n";
    std::cout << "CONTROLS:\n";
    std::cout << "  Movement:\n";
    std::cout << "    - Arrow Keys or WASD: Move\n";
    std::cout << "    - Controller Left Stick: Move\n";
    std::cout << "  Actions:\n";
    std::cout << "    - Space or Controller A: Jump\n";
    std::cout << "    - Ctrl or Controller X: Attack\n";
    std::cout << "  Menu:\n";
    std::cout << "    - Escape or Controller Start: Menu\n";
    std::cout << "  Mouse:\n";
    std::cout << "    - Left/Right/Middle buttons: Test mouse input\n";
    std::cout << "    - Move mouse: See position and delta\n";
    std::cout << "  Rebinding:\n";
    std::cout << "    - Press 'R': Enter rebinding mode\n";
    std::cout << "    - While in rebinding mode, press any key/button to capture it\n";
    std::cout << "  Other:\n";
    std::cout << "    - 'M': Print all current mappings\n";
    std::cout << "    - 'C': Clear all mappings\n";
    std::cout << "    - 'T': Test action helper methods\n";
    std::cout << "    - 'Q': Quit\n";
    std::cout << "\nThe demo will continuously display active inputs.\n";
    std::cout << "Watch the console for real-time input state updates.\n\n";
}

// Test action helper methods
void testActionHelpers(IInputSystem& input) {
    std::cout << "\n=== Testing Action Helper Methods ===\n";

    std::vector<Action> actions = {"MoveLeft", "MoveRight", "Jump", "Attack", "Menu"};

    for (const auto& action : actions) {
        bool active = input.isActionActive(action);
        bool pressed = input.wasActionJustPressed(action);
        bool released = input.wasActionJustReleased(action);
        float value = input.getActionValue(action);

        std::cout << "  " << action << ":\n";
        std::cout << "    isActionActive(): " << (active ? "true" : "false") << "\n";
        std::cout << "    wasActionJustPressed(): " << (pressed ? "true" : "false") << "\n";
        std::cout << "    wasActionJustReleased(): " << (released ? "true" : "false") << "\n";
        std::cout << "    getActionValue(): " << value << "\n";

        // Also test getActionState
        auto state = input.getActionState(action);
        std::cout << "    getActionState().active: " << (state.active ? "true" : "false") << "\n";
        std::cout << "    getActionState().value: " << state.value << "\n";
    }
}

int main() {
    // Initialize GLFW
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW\n";
        return 1;
    }

    // Create window
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);

    GLFWwindow* window = glfwCreateWindow(800, 600, "Input System Demo", nullptr, nullptr);
    if (!window) {
        std::cerr << "Failed to create GLFW window\n";
        glfwTerminate();
        return 1;
    }

    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);  // VSync

    // Create input system using DI container
    kgr::container container;
    auto& input = container.service<InputSystemService>();

    if (!input.initialize(window)) {
        std::cerr << "Failed to initialize input system\n";
        glfwDestroyWindow(window);
        glfwTerminate();
        return 1;
    }

    printInstructions();

    // Register typical game input mappings
    std::cout << "Registering default input mappings...\n";

    // MoveLeft - Left Arrow, A key, Controller D-Pad Left, Controller Left Stick X (negative)
    input.registerMapping(InputMapping{
        .binding = {.deviceType = InputDeviceType::Keyboard, .deviceIndex = 0, .keyCode = 263, .scale = -1.0f},  // Left Arrow
        .action = "MoveLeft"
    });
    input.registerMapping(InputMapping{
        .binding = {.deviceType = InputDeviceType::Keyboard, .deviceIndex = 0, .keyCode = 65, .scale = -1.0f},   // A
        .action = "MoveLeft"
    });
    input.registerMapping(InputMapping{
        .binding = {.deviceType = InputDeviceType::Controller, .deviceIndex = 0, .keyCode = SDL_CONTROLLER_BUTTON_DPAD_LEFT, .scale = -1.0f},
        .action = "MoveLeft"
    });
    input.registerMapping(InputMapping{
        .binding = {.deviceType = InputDeviceType::Controller, .deviceIndex = 0,
                   .keyCode = SDL_CONTROLLER_AXIS_LEFTX + SDL_CONTROLLER_BUTTON_MAX, .scale = -1.0f, .deadzone = 0.2f},
        .action = "MoveLeft"
    });

    // MoveRight - Right Arrow, D key, Controller D-Pad Right, Controller Left Stick X (positive)
    input.registerMapping(InputMapping{
        .binding = {.deviceType = InputDeviceType::Keyboard, .deviceIndex = 0, .keyCode = 262, .scale = 1.0f},   // Right Arrow
        .action = "MoveRight"
    });
    input.registerMapping(InputMapping{
        .binding = {.deviceType = InputDeviceType::Keyboard, .deviceIndex = 0, .keyCode = 68, .scale = 1.0f},    // D
        .action = "MoveRight"
    });
    input.registerMapping(InputMapping{
        .binding = {.deviceType = InputDeviceType::Controller, .deviceIndex = 0, .keyCode = SDL_CONTROLLER_BUTTON_DPAD_RIGHT, .scale = 1.0f},
        .action = "MoveRight"
    });
    input.registerMapping(InputMapping{
        .binding = {.deviceType = InputDeviceType::Controller, .deviceIndex = 0,
                   .keyCode = SDL_CONTROLLER_AXIS_LEFTX + SDL_CONTROLLER_BUTTON_MAX, .scale = 1.0f, .deadzone = 0.2f},
        .action = "MoveRight"
    });

    // Jump - Space, Controller A button
    input.registerMapping(InputMapping{
        .binding = {.deviceType = InputDeviceType::Keyboard, .deviceIndex = 0, .keyCode = 32, .scale = 1.0f},    // Space
        .action = "Jump"
    });
    input.registerMapping(InputMapping{
        .binding = {.deviceType = InputDeviceType::Controller, .deviceIndex = 0, .keyCode = SDL_CONTROLLER_BUTTON_A, .scale = 1.0f},
        .action = "Jump"
    });

    // Attack - Left Ctrl, Controller X button
    input.registerMapping(InputMapping{
        .binding = {.deviceType = InputDeviceType::Keyboard, .deviceIndex = 0, .keyCode = 341, .scale = 1.0f},   // Left Ctrl
        .action = "Attack"
    });
    input.registerMapping(InputMapping{
        .binding = {.deviceType = InputDeviceType::Controller, .deviceIndex = 0, .keyCode = SDL_CONTROLLER_BUTTON_X, .scale = 1.0f},
        .action = "Attack"
    });

    // Menu - Escape, Controller Start button
    input.registerMapping(InputMapping{
        .binding = {.deviceType = InputDeviceType::Keyboard, .deviceIndex = 0, .keyCode = 256, .scale = 1.0f},   // Escape
        .action = "Menu"
    });
    input.registerMapping(InputMapping{
        .binding = {.deviceType = InputDeviceType::Controller, .deviceIndex = 0, .keyCode = SDL_CONTROLLER_BUTTON_START, .scale = 1.0f},
        .action = "Menu"
    });

    std::cout << "Input mappings registered successfully!\n";
    printMappings(input);

    // Demo state
    DemoMode mode = DemoMode::Normal;
    bool showStats = false;
    int frameCount = 0;

    // Main loop
    while (!glfwWindowShouldClose(window)) {
        // Update input system (CRITICAL: must be called every frame)
        input.update();

        // Poll GLFW events
        glfwPollEvents();

        // Check for quit
        if (input.wasActionJustPressed("Menu")) {
            std::cout << "\nMenu action pressed - exiting demo\n";
            break;
        }

        // Handle rebinding mode
        if (glfwGetKey(window, 82) == GLFW_PRESS && mode == DemoMode::Normal) {  // R key
            std::cout << "\n>>> ENTERING REBINDING MODE <<<\n";
            std::cout << "Press any key, mouse button, or controller button to capture it...\n";
            input.startListeningForInput();
            mode = DemoMode::Rebinding;
        }

        if (mode == DemoMode::Rebinding) {
            if (input.isListeningForInput()) {
                std::cout << "  Listening for input...\r" << std::flush;
            } else {
                // Input was captured
                auto lastInput = input.getLastInput();
                if (lastInput.has_value()) {
                    std::cout << "\n>>> INPUT CAPTURED <<<\n";
                    std::cout << "  Device: " << deviceTypeToString(lastInput->deviceType) << "\n";
                    if (lastInput->deviceType == InputDeviceType::Controller) {
                        std::cout << "  Device Index: " << lastInput->deviceIndex << "\n";
                    }
                    std::cout << "  Key/Button: " << keyCodeToString(lastInput->deviceType, lastInput->keyCode) << "\n";
                    std::cout << "  Scale: " << lastInput->scale << "\n";
                    std::cout << "  Deadzone: " << lastInput->deadzone << "\n";
                    std::cout << "\nYou could now bind this to an action!\n";
                }
                std::cout << ">>> EXITING REBINDING MODE <<<\n\n";
                mode = DemoMode::Normal;
            }
        }

        // Handle other commands (only in normal mode)
        if (mode == DemoMode::Normal) {
            // M - Print mappings
            if (glfwGetKey(window, 77) == GLFW_PRESS) {  // M key
                static bool mKeyWasPressed = false;
                if (!mKeyWasPressed) {
                    printMappings(input);
                    mKeyWasPressed = true;
                }
            } else {
                static bool mKeyWasPressed = false;
                mKeyWasPressed = false;
            }

            // C - Clear mappings
            if (glfwGetKey(window, 67) == GLFW_PRESS) {  // C key
                static bool cKeyWasPressed = false;
                if (!cKeyWasPressed) {
                    std::cout << "\n>>> CLEARING ALL MAPPINGS <<<\n";
                    input.clearMappings();
                    std::cout << "All mappings cleared.\n";
                    cKeyWasPressed = true;
                }
            } else {
                static bool cKeyWasPressed = false;
                cKeyWasPressed = false;
            }

            // T - Test action helpers
            if (glfwGetKey(window, 84) == GLFW_PRESS) {  // T key
                static bool tKeyWasPressed = false;
                if (!tKeyWasPressed) {
                    testActionHelpers(input);
                    tKeyWasPressed = true;
                }
            } else {
                static bool tKeyWasPressed = false;
                tKeyWasPressed = false;
            }

            // Q - Quit
            if (glfwGetKey(window, 81) == GLFW_PRESS) {  // Q key
                std::cout << "\nQ pressed - exiting demo\n";
                break;
            }
        }

        // Print active states periodically (every 30 frames to avoid spam)
        if (mode == DemoMode::Normal && frameCount % 30 == 0) {
            // Only print if something is active
            auto allStates = input.getAllActionStates();
            bool anyActive = std::ranges::any_of(allStates, [](const ActionState& s) {
                return s.active || s.justPressed || s.justReleased;
            });

            bool mouseActive = input.isMouseButtonDown(0) || input.isMouseButtonDown(1) ||
                             input.isMouseButtonDown(2);

            if (anyActive) {
                printActionStates(input);
            }

            if (mouseActive || frameCount == 0) {
                printMouseState(input);
            }

            if (frameCount == 0 || input.getConnectedControllerCount() > 0) {
                printControllerState(input);
            }
        }

        frameCount++;

        // Swap buffers (no rendering needed for this demo)
        glfwSwapBuffers(window);

        // Small delay to prevent excessive CPU usage
        std::this_thread::sleep_for(std::chrono::milliseconds(16));  // ~60 FPS
    }

    std::cout << "\n=== Demo Complete ===\n";
    std::cout << "Final statistics:\n";
    std::cout << "  Total frames: " << frameCount << "\n";
    std::cout << "  Final mapping count: " << input.getMappings().size() << "\n";
    printControllerState(input);

    // Cleanup
    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}

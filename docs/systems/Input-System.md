# JFrame Input System

## Overview

The Input System provides a unified interface for handling keyboard, mouse, and gamepad input in JFrame. It uses an action-based input mapping system that allows you to bind multiple input sources to named actions, making it easy to support different control schemes and rebindable controls.

**Key Features:**
- Action-based input mapping (keyboard, mouse, gamepad)
- Multiple bindings per action (e.g., both WASD and arrow keys)
- State tracking (active, just pressed, just released)
- Analog input support with deadzone and scaling
- Runtime input rebinding support
- Mouse position and delta tracking
- Gamepad connection detection

**Dependencies:**
- GLFW (keyboard and mouse input)
- SDL2 (gamepad support)

## Architecture

### Core Components

```cpp
// Interface (jframe.input)
import jframe.input;
IInputSystem* input = ...;

// Implementation (jframe.input.impl)
import jframe.input.impl;
auto input = jframe::createInputSystem();
```

### Data Types

```cpp
// Input device types
enum class InputDeviceType : std::uint8_t {
    Keyboard,
    Mouse,
    Controller
};

// Input binding definition
struct InputBinding {
    InputDeviceType deviceType = InputDeviceType::Keyboard;
    int deviceIndex = 0;        // For multiple controllers
    int keyCode = 0;            // GLFW key code
    float scale = 1.0f;         // Scaling factor for analog inputs
    float deadzone = 0.1f;      // Deadzone for analog sticks
};

// Action name
using Action = std::string;

// Action state
struct ActionState {
    Action action;
    bool active = false;        // Currently held/active
    float value = 0.0f;         // Analog value (0.0 to 1.0)
    bool justPressed = false;   // Became active this frame
    bool justReleased = false;  // Became inactive this frame
};

// Input mapping (binds a key/button to an action)
struct InputMapping {
    InputBinding binding;
    Action action;
};
```

## Basic Usage

### 1. Register Input Mappings

```cpp
import jframe;

// In your game initialization
void Game::initialize(jframe::core::Engine& engine) {
    auto& input = engine.systems().input;

    // Register keyboard mappings
    input->registerMapping(jframe::InputMapping{
        .binding = {
            .deviceType = jframe::InputDeviceType::Keyboard,
            .keyCode = 32  // Spacebar (GLFW_KEY_SPACE)
        },
        .action = "jump"
    });

    input->registerMapping(jframe::InputMapping{
        .binding = {
            .deviceType = jframe::InputDeviceType::Keyboard,
            .keyCode = 65  // A key (GLFW_KEY_A)
        },
        .action = "move_left"
    });

    input->registerMapping(jframe::InputMapping{
        .binding = {
            .deviceType = jframe::InputDeviceType::Keyboard,
            .keyCode = 68  // D key (GLFW_KEY_D)
        },
        .action = "move_right"
    });
}
```

### 2. Query Input State

```cpp
void Game::update(jframe::DeltaTime dt) {
    auto& input = engine_->systems().input;

    // Check if action is currently active (held down)
    if (input->isActionActive("move_left")) {
        // Move player left
        playerVelocity.x = -moveSpeed;
    }

    // Check if action was just pressed this frame
    if (input->wasActionJustPressed("jump")) {
        // Player jumped
        playerVelocity.y = -jumpForce;
    }

    // Check if action was just released this frame
    if (input->wasActionJustReleased("shoot")) {
        // Release charged shot
        fireWeapon();
    }

    // Get analog value (useful for combining multiple inputs)
    float horizontalInput = 0.0f;
    if (input->isActionActive("move_left")) {
        horizontalInput -= 1.0f;
    }
    if (input->isActionActive("move_right")) {
        horizontalInput += 1.0f;
    }

    playerVelocity.x = horizontalInput * moveSpeed;
}
```

### 3. Update Input System

The input system must be updated each frame to track state changes:

```cpp
void GameLoop::update() {
    // Update input state (called by engine automatically)
    input->update();

    // Now query input for this frame
    game->update(deltaTime);
}
```

## Complete Player Movement Example

```cpp
// PlayerMovementSystem.cpp
import jframe;

class PlayerMovementSystem {
public:
    PlayerMovementSystem(jframe::core::Engine& engine)
        : engine_(engine) {}

    void setupInputMappings() {
        auto& input = engine_.systems().input;

        // Primary controls (WASD)
        input->registerMapping({
            {.deviceType = jframe::InputDeviceType::Keyboard, .keyCode = 65},
            "move_left"
        });
        input->registerMapping({
            {.deviceType = jframe::InputDeviceType::Keyboard, .keyCode = 68},
            "move_right"
        });
        input->registerMapping({
            {.deviceType = jframe::InputDeviceType::Keyboard, .keyCode = 32},
            "jump"
        });

        // Alternative controls (Arrow keys)
        input->registerMapping({
            {.deviceType = jframe::InputDeviceType::Keyboard, .keyCode = 263},
            "move_left"
        });
        input->registerMapping({
            {.deviceType = jframe::InputDeviceType::Keyboard, .keyCode = 262},
            "move_right"
        });

        // Gamepad support
        input->registerMapping({
            {.deviceType = jframe::InputDeviceType::Controller, .keyCode = 0},
            "jump"  // A button
        });
    }

    void update(jframe::DeltaTime dt) {
        auto& sys = engine_.systems();
        auto& input = sys.input;

        // Get player entity
        auto playerView = sys.entities->view<PlayerTag>();
        for (auto entity : playerView) {
            // Get current velocity
            auto velocity = sys.physics->getVelocity(entity);

            // Horizontal movement
            float horizontal = 0.0f;
            if (input->isActionActive("move_left")) {
                horizontal -= 1.0f;
            }
            if (input->isActionActive("move_right")) {
                horizontal += 1.0f;
            }

            velocity.x = horizontal * moveSpeed_;

            // Jump (only if grounded)
            if (input->wasActionJustPressed("jump")) {
                if (auto* groundDetector = sys.entities->tryGet<GroundDetector>(entity)) {
                    if (groundDetector->isGrounded) {
                        velocity.y = -jumpForce_;
                    }
                }
            }

            // Apply velocity
            sys.physics->setVelocity(entity, velocity);
        }
    }

private:
    jframe::core::Engine& engine_;
    float moveSpeed_ = 400.0f;
    float jumpForce_ = 800.0f;
};
```

## Mouse Input

### Mouse Position and Buttons

```cpp
void Game::handleMouseInput() {
    auto& input = engine_->systems().input;

    // Get mouse position (screen coordinates)
    jframe::Vec2 mousePos = input->getMousePosition();

    // Get mouse delta (for camera rotation, etc.)
    jframe::Vec2 mouseDelta = input->getMouseDelta();

    // Check mouse buttons (0 = left, 1 = right, 2 = middle)
    if (input->isMouseButtonDown(0)) {
        // Left mouse button is pressed
        fireWeapon(mousePos);
    }

    if (input->isMouseButtonDown(1)) {
        // Right mouse button is pressed (aim mode)
        aimAt(mousePos);
    }
}
```

### Mouse Action Mappings

You can also map mouse buttons to actions:

```cpp
// Map left mouse button to "attack" action
input->registerMapping(jframe::InputMapping{
    .binding = {
        .deviceType = jframe::InputDeviceType::Mouse,
        .keyCode = 0  // Left mouse button
    },
    .action = "attack"
});

// Now use like any other action
if (input->wasActionJustPressed("attack")) {
    fireWeapon();
}
```

## Gamepad Support

### Detecting Connected Controllers

```cpp
void Game::checkControllers() {
    auto& input = engine_->systems().input;

    // Get number of connected controllers
    int controllerCount = input->getConnectedControllerCount();

    // Check specific controller
    if (input->isControllerConnected(0)) {
        std::string name = input->getControllerName(0);
        jframe::core::logInfo("Controller 0: " + name);
    }
}
```

### Gamepad Button Mappings

SDL2 controller button indices:

```cpp
// Common SDL2 gamepad buttons
// 0  = A (Xbox) / Cross (PlayStation)
// 1  = B (Xbox) / Circle (PlayStation)
// 2  = X (Xbox) / Square (PlayStation)
// 3  = Y (Xbox) / Triangle (PlayStation)
// 4  = Back/Share
// 5  = Guide/Home
// 6  = Start/Options
// 7  = Left Stick Click
// 8  = Right Stick Click
// 9  = Left Shoulder
// 10 = Right Shoulder
// 11 = D-Pad Up
// 12 = D-Pad Down
// 13 = D-Pad Left
// 14 = D-Pad Right

// Example: Map gamepad buttons
input->registerMapping({
    {.deviceType = jframe::InputDeviceType::Controller, .deviceIndex = 0, .keyCode = 0},
    "jump"  // A button
});

input->registerMapping({
    {.deviceType = jframe::InputDeviceType::Controller, .deviceIndex = 0, .keyCode = 1},
    "attack"  // B button
});

input->registerMapping({
    {.deviceType = jframe::InputDeviceType::Controller, .deviceIndex = 0, .keyCode = 9},
    "dash"  // Left shoulder
});
```

### Analog Stick Support

```cpp
// Map analog stick axes with deadzone and scaling
input->registerMapping(jframe::InputMapping{
    .binding = {
        .deviceType = jframe::InputDeviceType::Controller,
        .deviceIndex = 0,
        .keyCode = 0,      // Left stick X axis
        .scale = 1.0f,     // Full range
        .deadzone = 0.15f  // 15% deadzone
    },
    .action = "move_horizontal"
});

// Get analog value
float horizontal = input->getActionValue("move_horizontal");
playerVelocity.x = horizontal * moveSpeed;
```

## Input Mapping Management

### Multiple Bindings Per Action

You can bind multiple inputs to the same action. The action will be active if ANY of its bindings are active:

```cpp
// Keyboard jump
input->registerMapping({
    {.deviceType = jframe::InputDeviceType::Keyboard, .keyCode = 32},
    "jump"
});

// Gamepad jump
input->registerMapping({
    {.deviceType = jframe::InputDeviceType::Controller, .keyCode = 0},
    "jump"
});

// Now both spacebar AND gamepad A button trigger "jump"
if (input->wasActionJustPressed("jump")) {
    player->jump();
}
```

### Removing Mappings

```cpp
// Remove a specific binding
jframe::InputBinding binding{
    .deviceType = jframe::InputDeviceType::Keyboard,
    .keyCode = 65
};
input->removeMapping(binding);

// Clear all mappings
input->clearMappings();
```

### Querying Mappings

```cpp
// Get all current mappings
std::vector<jframe::InputMapping> mappings = input->getMappings();

for (const auto& mapping : mappings) {
    std::cout << "Action: " << mapping.action
              << " bound to keyCode: " << mapping.binding.keyCode
              << std::endl;
}
```

## Runtime Input Rebinding

The input system supports capturing raw input for rebinding UI:

```cpp
class ControlsMenu {
public:
    void startRebinding(const std::string& action) {
        rebindingAction_ = action;
        input_->startListeningForInput();
        showMessage("Press any key...");
    }

    void update() {
        if (input_->isListeningForInput()) {
            if (auto binding = input_->getLastInput()) {
                // User pressed a key, bind it to the action
                input_->registerMapping({*binding, rebindingAction_});
                input_->stopListeningForInput();
                showMessage("Bound to action: " + rebindingAction_);
            }
        }
    }

private:
    jframe::IInputSystem* input_;
    std::string rebindingAction_;
};
```

### Complete Rebinding Example

```cpp
// ControlsMenuSystem.cpp
class ControlsMenuSystem {
public:
    void rebindKey(const std::string& action) {
        // Start listening for input
        input_->startListeningForInput();
        actionToRebind_ = action;

        showPrompt("Press a key to bind to " + action);
    }

    void update() {
        // Check if we're waiting for input
        if (!input_->isListeningForInput()) {
            return;
        }

        // Check if user pressed something
        auto lastInput = input_->getLastInput();
        if (!lastInput.has_value()) {
            return;
        }

        // Remove old binding for this action
        auto mappings = input_->getMappings();
        for (const auto& mapping : mappings) {
            if (mapping.action == actionToRebind_) {
                input_->removeMapping(mapping.binding);
            }
        }

        // Add new binding
        input_->registerMapping({*lastInput, actionToRebind_});

        // Stop listening
        input_->stopListeningForInput();

        showPrompt("Action " + actionToRebind_ + " rebound!");
        saveControlsToConfig();
    }

private:
    jframe::IInputSystem* input_;
    std::string actionToRebind_;
};
```

## Loading Input Mappings from Lua

The recommended way to configure input is via Lua files:

```lua
-- data/config/input.lua
return {
    move_left = {
        primary = 65,     -- A key
        secondary = 263   -- Left arrow
    },
    move_right = {
        primary = 68,     -- D key
        secondary = 262   -- Right arrow
    },
    jump = {
        primary = 32,     -- Space
        controller = 0    -- Gamepad A button
    },
    attack = {
        primary = 74,     -- J key
        mouse = 0         -- Left mouse button
    }
}
```

Then load it in your game:

```cpp
void Game::loadInputMappings() {
    auto& input = engine_->systems().input;

    // Load config
    auto config = createConfigSystem();
    config->loadConfig("data/config/input.lua");

    // Register move_left
    if (int key = config->getIntOr("move_left.primary", 0)) {
        input->registerMapping({
            {.deviceType = jframe::InputDeviceType::Keyboard, .keyCode = key},
            "move_left"
        });
    }
    if (int key = config->getIntOr("move_left.secondary", 0)) {
        input->registerMapping({
            {.deviceType = jframe::InputDeviceType::Keyboard, .keyCode = key},
            "move_left"
        });
    }

    // Register move_right
    if (int key = config->getIntOr("move_right.primary", 0)) {
        input->registerMapping({
            {.deviceType = jframe::InputDeviceType::Keyboard, .keyCode = key},
            "move_right"
        });
    }
    if (int key = config->getIntOr("move_right.secondary", 0)) {
        input->registerMapping({
            {.deviceType = jframe::InputDeviceType::Keyboard, .keyCode = key},
            "move_right"
        });
    }

    // Register jump
    if (int key = config->getIntOr("jump.primary", 0)) {
        input->registerMapping({
            {.deviceType = jframe::InputDeviceType::Keyboard, .keyCode = key},
            "jump"
        });
    }
    if (int button = config->getIntOr("jump.controller", -1); button >= 0) {
        input->registerMapping({
            {.deviceType = jframe::InputDeviceType::Controller, .keyCode = button},
            "jump"
        });
    }
}
```

## GLFW Key Codes Reference

Common GLFW key codes for keyboard input:

### Printable Keys
```cpp
32   Space
39   ' (Apostrophe)
44   , (Comma)
45   - (Minus)
46   . (Period)
47   / (Slash)
48-57  0-9 (Numbers)
59   ; (Semicolon)
61   = (Equal)
65-90  A-Z (Letters)
91   [ (Left Bracket)
92   \ (Backslash)
93   ] (Right Bracket)
96   ` (Grave Accent)
```

### Function Keys
```cpp
256  Escape
257  Enter
258  Tab
259  Backspace
260  Insert
261  Delete
262  Right Arrow
263  Left Arrow
264  Down Arrow
265  Up Arrow
266  Page Up
267  Page Down
268  Home
269  End
280-289  F1-F10
290  F11
291  F12
```

### Modifier Keys
```cpp
340  Left Shift
341  Left Control
342  Left Alt
343  Left Super (Windows/Command)
344  Right Shift
345  Right Control
346  Right Alt
347  Right Super
```

### Example: Common Game Controls
```cpp
// WASD movement
input->registerMapping({{.deviceType = Keyboard, .keyCode = 87}, "move_up"});    // W
input->registerMapping({{.deviceType = Keyboard, .keyCode = 65}, "move_left"});  // A
input->registerMapping({{.deviceType = Keyboard, .keyCode = 83}, "move_down"});  // S
input->registerMapping({{.deviceType = Keyboard, .keyCode = 68}, "move_right"}); // D

// Arrow keys (alternative)
input->registerMapping({{.deviceType = Keyboard, .keyCode = 265}, "move_up"});    // Up
input->registerMapping({{.deviceType = Keyboard, .keyCode = 263}, "move_left"});  // Left
input->registerMapping({{.deviceType = Keyboard, .keyCode = 264}, "move_down"});  // Down
input->registerMapping({{.deviceType = Keyboard, .keyCode = 262}, "move_right"}); // Right

// Common actions
input->registerMapping({{.deviceType = Keyboard, .keyCode = 32}, "jump"});       // Space
input->registerMapping({{.deviceType = Keyboard, .keyCode = 340}, "sprint"});    // Left Shift
input->registerMapping({{.deviceType = Keyboard, .keyCode = 341}, "crouch"});    // Left Ctrl
input->registerMapping({{.deviceType = Keyboard, .keyCode = 69}, "interact"});   // E
input->registerMapping({{.deviceType = Keyboard, .keyCode = 82}, "reload"});     // R
input->registerMapping({{.deviceType = Keyboard, .keyCode = 256}, "menu"});      // Escape
```

## Advanced Usage

### Action State Queries

Get full action state information:

```cpp
// Get complete state for an action
jframe::ActionState state = input->getActionState("jump");

std::cout << "Active: " << state.active << std::endl;
std::cout << "Value: " << state.value << std::endl;
std::cout << "Just Pressed: " << state.justPressed << std::endl;
std::cout << "Just Released: " << state.justReleased << std::endl;

// Get all action states
std::vector<jframe::ActionState> allStates = input->getAllActionStates();
for (const auto& state : allStates) {
    if (state.active) {
        std::cout << "Action " << state.action << " is active" << std::endl;
    }
}
```

### Input Buffering for Tight Controls

```cpp
class InputBuffer {
public:
    void update(jframe::DeltaTime dt) {
        // Update jump buffer
        if (input_->wasActionJustPressed("jump")) {
            jumpBufferTime_ = bufferDuration_;
        } else if (jumpBufferTime_ > 0.0f) {
            jumpBufferTime_ -= dt;
        }
    }

    bool hasBufferedJump() const {
        return jumpBufferTime_ > 0.0f;
    }

    void consumeJump() {
        jumpBufferTime_ = 0.0f;
    }

private:
    jframe::IInputSystem* input_;
    float jumpBufferTime_ = 0.0f;
    float bufferDuration_ = 0.1f;  // 100ms buffer window
};
```

### Input Combination Detection

```cpp
bool checkComboInput(IInputSystem* input) {
    // Check for simultaneous button presses
    return input->isActionActive("attack") &&
           input->isActionActive("special");
}

bool checkSequentialCombo(IInputSystem* input) {
    // Track input sequence
    static std::vector<std::string> sequence;
    static float sequenceTimer = 0.0f;

    // Reset if too much time passed
    if (sequenceTimer > 1.0f) {
        sequence.clear();
    }

    // Add to sequence
    if (input->wasActionJustPressed("up")) {
        sequence.push_back("up");
    }
    if (input->wasActionJustPressed("down")) {
        sequence.push_back("down");
    }

    // Check for Konami code
    if (sequence == std::vector<std::string>{"up", "up", "down", "down"}) {
        sequence.clear();
        return true;
    }

    return false;
}
```

## Best Practices

### 1. Use Action Names Consistently

Define action names as constants to avoid typos:

```cpp
namespace Actions {
    inline constexpr const char* MoveLeft = "move_left";
    inline constexpr const char* MoveRight = "move_right";
    inline constexpr const char* Jump = "jump";
    inline constexpr const char* Attack = "attack";
}

// Use consistently
if (input->isActionActive(Actions::MoveLeft)) {
    // ...
}
```

### 2. Register Mappings Early

Register all input mappings during initialization, not during gameplay:

```cpp
void Game::initialize() {
    setupInputMappings();  // Do this once
}

void Game::update() {
    // Query input, don't register new mappings here
}
```

### 3. Support Multiple Control Schemes

Always provide alternative inputs:

```cpp
// Support both WASD and arrow keys
registerBothSchemes("move_left", 65, 263);   // A and Left Arrow
registerBothSchemes("move_right", 68, 262);  // D and Right Arrow
```

### 4. Handle Disconnected Controllers Gracefully

```cpp
void checkControllerStatus() {
    if (!input->isControllerConnected(0)) {
        // Show "Reconnect Controller" message
        showMessage("Controller disconnected!");
    }
}
```

### 5. Use Configuration Files

Store input mappings in Lua config files for easy customization:

```lua
-- Easy for players to modify
return {
    move_left = 65,
    move_right = 68,
    jump = 32
}
```

## Common Patterns

### Platformer Controls

```cpp
void setupPlatformerInput(IInputSystem* input) {
    // Horizontal movement
    input->registerMapping({{.deviceType = Keyboard, .keyCode = 65}, "move_left"});
    input->registerMapping({{.deviceType = Keyboard, .keyCode = 68}, "move_right"});
    input->registerMapping({{.deviceType = Keyboard, .keyCode = 263}, "move_left"});
    input->registerMapping({{.deviceType = Keyboard, .keyCode = 262}, "move_right"});

    // Jump
    input->registerMapping({{.deviceType = Keyboard, .keyCode = 32}, "jump"});
    input->registerMapping({{.deviceType = Keyboard, .keyCode = 265}, "jump"});

    // Dash/Run
    input->registerMapping({{.deviceType = Keyboard, .keyCode = 340}, "dash"});
}
```

### Top-Down Shooter Controls

```cpp
void setupShooterInput(IInputSystem* input) {
    // WASD movement
    input->registerMapping({{.deviceType = Keyboard, .keyCode = 87}, "move_up"});
    input->registerMapping({{.deviceType = Keyboard, .keyCode = 65}, "move_left"});
    input->registerMapping({{.deviceType = Keyboard, .keyCode = 83}, "move_down"});
    input->registerMapping({{.deviceType = Keyboard, .keyCode = 68}, "move_right"});

    // Mouse aim and shoot
    input->registerMapping({{.deviceType = Mouse, .keyCode = 0}, "shoot"});
    input->registerMapping({{.deviceType = Mouse, .keyCode = 1}, "aim"});

    // Reload and interact
    input->registerMapping({{.deviceType = Keyboard, .keyCode = 82}, "reload"});
    input->registerMapping({{.deviceType = Keyboard, .keyCode = 69}, "interact"});
}
```

### Menu Navigation

```cpp
void setupMenuInput(IInputSystem* input) {
    // Navigation
    input->registerMapping({{.deviceType = Keyboard, .keyCode = 265}, "menu_up"});
    input->registerMapping({{.deviceType = Keyboard, .keyCode = 264}, "menu_down"});
    input->registerMapping({{.deviceType = Keyboard, .keyCode = 263}, "menu_left"});
    input->registerMapping({{.deviceType = Keyboard, .keyCode = 262}, "menu_right"});

    // Confirm/Cancel
    input->registerMapping({{.deviceType = Keyboard, .keyCode = 257}, "menu_confirm"});
    input->registerMapping({{.deviceType = Keyboard, .keyCode = 256}, "menu_cancel"});

    // Gamepad
    input->registerMapping({{.deviceType = Controller, .keyCode = 0}, "menu_confirm"});
    input->registerMapping({{.deviceType = Controller, .keyCode = 1}, "menu_cancel"});
}
```

## Troubleshooting

### Input Not Responding

1. Verify input system is being updated each frame
2. Check that mappings are registered before querying
3. Ensure correct key codes are used (GLFW key codes)
4. Verify GLFW window has focus

### Controller Not Detected

1. Ensure SDL2 is initialized
2. Check controller is connected before starting game
3. Verify controller is recognized by SDL2 (use SDL test utilities)
4. Check controller index (0-3)

### Actions Not Triggering

1. Print current action states to debug
2. Verify action names match exactly (case-sensitive)
3. Check that update() is called before querying input
4. Ensure mappings aren't being cleared accidentally

### Multiple Inputs Conflicting

This is expected behavior - if multiple bindings map to the same action, any of them will activate it. To separate them:

```cpp
// Use different actions for keyboard vs gamepad
input->registerMapping({keyboard_binding, "jump_keyboard"});
input->registerMapping({gamepad_binding, "jump_gamepad"});

// Check both
if (input->wasActionJustPressed("jump_keyboard") ||
    input->wasActionJustPressed("jump_gamepad")) {
    player->jump();
}
```

## API Reference

### Lifecycle

```cpp
// Update input state (call once per frame)
void update();
```

### Mapping Management

```cpp
// Register an input mapping
void registerMapping(const InputMapping& mapping);

// Remove a specific binding
void removeMapping(const InputBinding& binding);

// Clear all mappings
void clearMappings();

// Get all current mappings
std::vector<InputMapping> getMappings() const;
```

### Action State Queries

```cpp
// Get full action state
ActionState getActionState(const Action& action) const;

// Get all action states
std::vector<ActionState> getAllActionStates() const;

// Check if action is currently active
bool isActionActive(const Action& action) const;

// Check if action became active this frame
bool wasActionJustPressed(const Action& action) const;

// Check if action became inactive this frame
bool wasActionJustReleased(const Action& action) const;

// Get analog value (0.0 to 1.0)
float getActionValue(const Action& action) const;
```

### Raw Input (for Rebinding UI)

```cpp
// Get the last input received
std::optional<InputBinding> getLastInput() const;

// Check if listening for input
bool isListeningForInput() const;

// Start capturing raw input
void startListeningForInput();

// Stop capturing raw input
void stopListeningForInput();
```

### Mouse State

```cpp
// Get mouse position (screen coordinates)
Vec2 getMousePosition() const;

// Get mouse delta (movement since last frame)
Vec2 getMouseDelta() const;

// Check if mouse button is pressed
bool isMouseButtonDown(int button) const;
```

### Controller

```cpp
// Get number of connected controllers
int getConnectedControllerCount() const;

// Check if specific controller is connected
bool isControllerConnected(int index) const;

// Get controller name
std::string getControllerName(int index) const;
```

## See Also

- [Events System](Events-System.md) - For input events
- [Physics System](Physics-System.md) - For applying forces based on input
- [Entity System](Entity-System.md) - For managing player entities

# Bestow Input System Guide

**Version:** 2.0
**Last Updated:** 2025-12-14

The Input System provides comprehensive handling for keyboard, mouse, and gamepad input through an **action mapping architecture**. This guide covers everything game developers need to implement responsive, accessible input in their games.

---

## Table of Contents

1. [Overview](#overview)
2. [Core Concepts](#core-concepts)
3. [Action Mapping](#action-mapping)
4. [Mouse Input](#mouse-input)
5. [Controller Support](#controller-support)
6. [Text Input](#text-input)
7. [Input Listening and Rebinding](#input-listening-and-rebinding)
8. [Best Practices](#best-practices)
9. [Complete Examples](#complete-examples)

---

## Overview

The Input System (`IInputSystem`) is the centralized interface for all player input in Bestow. It provides:

- **Action-based input**: Map hardware inputs (keys, buttons) to logical actions
- **Multi-device support**: Keyboard, mouse, and gamepad simultaneously
- **Frame-perfect detection**: Track pressed, held, and released states
- **Live rebinding**: Allow players to remap controls at runtime
- **Dvorak-friendly defaults**: Optimized for Dvorak keyboard layout (,AOE movement)

### Basic Setup

```cpp
import bestow;

class MyGame : public bestow::core::Application {
public:
    bool initialize(bestow::core::Engine& engine) override {
        auto& sys = engine.systems();

        // Register action mappings
        sys.input->registerMapping({
            .binding = {
                .deviceType = InputDeviceType::Keyboard,
                .keyCode = 44,  // Comma (,) - left on Dvorak
            },
            .action = "move_left"
        });

        return true;
    }

    void updateFixed(DeltaTime dt) override {
        auto& sys = engine.systems();

        if (sys.input->isActionActive("move_left")) {
            // Player is moving left
        }
    }
};
```

---

## Core Concepts

### Action-Based Input System

**The Input System is entirely action-based.** You map device inputs (keyboard keys, mouse buttons, gamepad buttons) to named actions, then query those actions in your game code.

**Actions** are logical game commands (e.g., "jump", "fire", "menu_open"). They are:
- Platform-agnostic
- Rebindable at runtime
- Support multiple input sources (keyboard + gamepad)
- Work with analog inputs (joysticks, triggers)

```cpp
// Good: Action-based input
if (input->isActionActive("jump")) {
    player.jump();
}

// The system does NOT expose raw key queries
// Everything goes through action mappings
```

### Input Structures

The system uses these core types:

**InputBinding** - Describes a physical input:
```cpp
struct InputBinding {
    InputDeviceType deviceType;  // Keyboard, Mouse, Controller
    int deviceIndex;             // Controller index (0-3)
    int keyCode;                 // Key/button code
    float scale;                 // Multiplier for analog values
    float deadzone;              // Ignore values below this threshold
};
```

**InputMapping** - Maps a binding to an action:
```cpp
struct InputMapping {
    InputBinding binding;
    Action action;  // std::string
};
```

**ActionState** - Current state of an action:
```cpp
struct ActionState {
    Action action;
    bool active;        // Currently held
    float value;        // Analog value (0.0-1.0)
    bool justPressed;   // Pressed this frame
    bool justReleased;  // Released this frame
};
```

### Input States

Every action has three states you can query:

| State | Method | Description | Use Case |
|-------|--------|-------------|----------|
| **Active** | `isActionActive(action)` | Input is currently held down | Continuous movement, aiming |
| **Just Pressed** | `wasActionJustPressed(action)` | Input pressed this frame | Jump, shoot, open menu |
| **Just Released** | `wasActionJustReleased(action)` | Input released this frame | Charge attacks, drag-and-drop |

```cpp
void updateFixed(DeltaTime dt) {
    auto& sys = engine_->systems();

    // Continuous movement while held
    if (sys.input->isActionActive("move_right")) {
        player.velocity.x = 200.0f;
    }

    // Jump only on the first frame of press
    if (sys.input->wasActionJustPressed("jump")) {
        player.jump();
    }

    // Release a charged shot
    if (sys.input->wasActionJustReleased("charge")) {
        player.releaseChargedShot();
    }
}
```

### Action Values (Analog Input)

Actions support analog values from 0.0 to 1.0 (or -1.0 to +1.0 for axes):

```cpp
// Get analog value (-1.0 to +1.0 for axes, 0.0 to 1.0 for buttons)
float moveAmount = sys.input->getActionValue("move_horizontal");
player.velocity.x = moveAmount * player.maxSpeed;

// Digital check (is value above threshold?)
bool isMoving = sys.input->isActionActive("move_horizontal");
```

### Complete Action State

For comprehensive action information, use `getActionState()`:

```cpp
ActionState state = sys.input->getActionState("jump");

if (state.active) {
    // Action is currently active
}
if (state.justPressed) {
    // Action was just pressed this frame
}
if (state.justReleased) {
    // Action was just released this frame
}
float value = state.value;  // Analog value
```

### Query All Action States

```cpp
// Get state of all registered actions
std::vector<ActionState> allStates = sys.input->getAllActionStates();

for (const auto& state : allStates) {
    std::cout << state.action << ": " << state.value << "\n";
}
```

---

## Action Mapping

Action mapping decouples game logic from hardware inputs. Players can use keyboard, mouse, or gamepad interchangeably.

### Registering Mappings

```cpp
void setupControls() {
    auto& sys = engine_->systems();

    // Keyboard jump (Space)
    sys.input->registerMapping({
        .binding = {.deviceType = InputDeviceType::Keyboard, .keyCode = 32},
        .action = "jump"
    });

    // Controller jump (A button - button code 0)
    sys.input->registerMapping({
        .binding = {.deviceType = InputDeviceType::Controller, .keyCode = 0},
        .action = "jump"
    });

    // Mouse jump (Left click)
    sys.input->registerMapping({
        .binding = {.deviceType = InputDeviceType::Mouse, .keyCode = 0},
        .action = "jump"
    });

    // Now all three inputs trigger the same "jump" action!
}
```

### Key Codes Reference

Bestow uses GLFW key codes for keyboard input. Common keys:

| Key | Code | Dvorak Position | QWERTY Position |
|-----|------|-----------------|-----------------|
| Comma (`,`) | 44 | Home row left | Below M |
| A | 65 | Left hand middle | QWERTY A position |
| O | 79 | Right hand index | QWERTY S position |
| E | 69 | Right hand middle | QWERTY D position |
| Space | 32 | Spacebar | Spacebar |
| Escape | 256 | Escape | Escape |
| Enter | 257 | Enter | Enter |
| Tab | 258 | Tab | Tab |
| Left Shift | 340 | Left Shift | Left Shift |
| Left Ctrl | 341 | Left Ctrl | Left Ctrl |
| Left Arrow | 263 | Arrow key | Arrow key |
| Right Arrow | 262 | Arrow key | Arrow key |
| Up Arrow | 265 | Arrow key | Arrow key |
| Down Arrow | 264 | Arrow key | Arrow key |

**Full reference**: See GLFW documentation for complete key code list.

### Dvorak-Friendly Movement

**IMPORTANT**: The project owner uses Dvorak layout. Default movement should use **,AOE** (Dvorak home row), which corresponds to WASD finger positions on QWERTY keyboards.

```cpp
void setupDvorakMovement() {
    auto& sys = engine_->systems();

    // Left: Comma (,) - Dvorak left hand
    sys.input->registerMapping({
        .binding = {.deviceType = InputDeviceType::Keyboard, .keyCode = 44},
        .action = "move_left"
    });

    // Down: A - Dvorak home row
    sys.input->registerMapping({
        .binding = {.deviceType = InputDeviceType::Keyboard, .keyCode = 65},
        .action = "move_down"
    });

    // Up: O - Dvorak home row
    sys.input->registerMapping({
        .binding = {.deviceType = InputDeviceType::Keyboard, .keyCode = 79},
        .action = "move_up"
    });

    // Right: E - Dvorak right hand
    sys.input->registerMapping({
        .binding = {.deviceType = InputDeviceType::Keyboard, .keyCode = 69},
        .action = "move_right"
    });
}
```

### Multiple Bindings per Action

You can bind multiple inputs to the same action. The system returns the **maximum absolute value** when multiple inputs are active:

```cpp
// Bind both O and Up Arrow to "move_up"
sys.input->registerMapping({
    .binding = {.deviceType = InputDeviceType::Keyboard, .keyCode = 79},
    .action = "move_up"
});
sys.input->registerMapping({
    .binding = {.deviceType = InputDeviceType::Keyboard, .keyCode = 265},  // Up Arrow
    .action = "move_up"
});

// Both keys will activate the same action
if (sys.input->isActionActive("move_up")) {
    // Triggered by either O or Up Arrow
}
```

### Directional Actions with Scale

Use `scale` to create opposing directional actions:

```cpp
void setupHorizontalMovement() {
    auto& sys = engine_->systems();

    // Left = negative scale
    sys.input->registerMapping({
        .binding = {
            .deviceType = InputDeviceType::Keyboard,
            .keyCode = 44,  // Comma
            .scale = -1.0f  // Negative for left
        },
        .action = "move_horizontal"
    });

    // Right = positive scale
    sys.input->registerMapping({
        .binding = {
            .deviceType = InputDeviceType::Keyboard,
            .keyCode = 69,  // E
            .scale = 1.0f   // Positive for right
        },
        .action = "move_horizontal"
    });

    // In gameplay:
    float moveX = sys.input->getActionValue("move_horizontal");
    // Comma = -1.0, E = +1.0
    player.velocity.x = moveX * player.maxSpeed;
}
```

### Querying Mappings

```cpp
// Get all current mappings
std::vector<InputMapping> mappings = sys.input->getMappings();

for (const auto& mapping : mappings) {
    std::cout << "Action: " << mapping.action << "\n";
    std::cout << "Device: " << static_cast<int>(mapping.binding.deviceType) << "\n";
    std::cout << "Key/Button: " << mapping.binding.keyCode << "\n";
}
```

### Removing Mappings

```cpp
// Remove specific mapping by binding
InputBinding toRemove{
    .deviceType = InputDeviceType::Keyboard,
    .keyCode = 32  // Space
};
sys.input->removeMapping(toRemove);

// Remove all mappings
sys.input->clearMappings();
```

---

## Mouse Input

### Mouse Position and Delta

```cpp
// Get current mouse position (screen coordinates)
Vec2 mousePos = sys.input->getMousePosition();
// mousePos.x and mousePos.y are in pixels from top-left corner

// Get mouse movement this frame
Vec2 mouseDelta = sys.input->getMouseDelta();
// mouseDelta.x and mouseDelta.y show change since last frame

// Example: Camera rotation
void updateCamera() {
    Vec2 delta = sys.input->getMouseDelta();
    camera.rotation.y += delta.x * sensitivity;
    camera.rotation.x += delta.y * sensitivity;
}
```

### Mouse Buttons

Mouse buttons are indexed 0-7:

| Button | Index | Constant |
|--------|-------|----------|
| Left | 0 | GLFW_MOUSE_BUTTON_LEFT |
| Right | 1 | GLFW_MOUSE_BUTTON_RIGHT |
| Middle | 2 | GLFW_MOUSE_BUTTON_MIDDLE |
| Back | 3 | GLFW_MOUSE_BUTTON_4 |
| Forward | 4 | GLFW_MOUSE_BUTTON_5 |

```cpp
// Check if mouse button is down
if (sys.input->isMouseButtonDown(0)) {  // Left click
    fireWeapon();
}

// Map mouse buttons to actions
sys.input->registerMapping({
    .binding = {
        .deviceType = InputDeviceType::Mouse,
        .keyCode = 0,  // Left button
    },
    .action = "attack"
});

// Use the action
if (sys.input->wasActionJustPressed("attack")) {
    Vec2 mousePos = sys.input->getMousePosition();
    shootAt(mousePos);
}
```

### Scroll Wheel

```cpp
// Get scroll wheel movement this frame
Vec2 scroll = sys.input->getScrollDelta();
// scroll.y is vertical scroll (most common)
// scroll.x is horizontal scroll (trackpad two-finger swipe)

void updateFixed(DeltaTime dt) {
    Vec2 scroll = sys.input->getScrollDelta();

    if (scroll.y > 0) {
        // Scrolled up
        zoomIn();
    } else if (scroll.y < 0) {
        // Scrolled down
        zoomOut();
    }
}
```

**Important**: Scroll delta is only valid for one frame. It resets to zero each `update()` call.

---

## Controller Support

### Connected Controllers

Bestow supports multiple simultaneous controllers using SDL2's GameController API:

```cpp
// Check how many controllers are connected
int numControllers = sys.input->getConnectedControllerCount();

// Check if specific controller slot is active
if (sys.input->isControllerConnected(0)) {
    std::string name = sys.input->getControllerName(0);
    // name = "Xbox Series X Controller", "DualShock 4", etc.
}
```

### Controller Buttons

SDL2 provides standardized button mappings. Button codes for common buttons:

| Button | Code | Xbox | PlayStation |
|--------|------|------|-------------|
| A / Cross | 0 | A | Cross (X) |
| B / Circle | 1 | B | Circle |
| X / Square | 2 | X | Square |
| Y / Triangle | 3 | Y | Triangle |
| Back / Share | 4 | Back | Share |
| Guide / PS | 5 | Xbox | PlayStation |
| Start / Options | 6 | Start | Options |
| Left Stick | 7 | L3 | L3 |
| Right Stick | 8 | R3 | R3 |
| Left Shoulder | 9 | LB | L1 |
| Right Shoulder | 10 | RB | R1 |
| D-Pad Up | 11 | D-Up | D-Up |
| D-Pad Down | 12 | D-Down | D-Down |
| D-Pad Left | 13 | D-Left | D-Left |
| D-Pad Right | 14 | D-Right | D-Right |

```cpp
// Map controller button to action
sys.input->registerMapping({
    .binding = {
        .deviceType = InputDeviceType::Controller,
        .deviceIndex = 0,  // First controller
        .keyCode = 0,      // A button
    },
    .action = "jump"
});
```

### Controller Axes

Analog sticks and triggers use axes. Note: Axis codes may be offset by SDL_CONTROLLER_BUTTON_MAX in some implementations. Refer to SDL2 documentation for exact codes.

Common axes:
- Left Stick X: Horizontal movement (-1.0 to +1.0)
- Left Stick Y: Vertical movement (-1.0 to +1.0)
- Right Stick X: Aiming/camera horizontal
- Right Stick Y: Aiming/camera vertical
- Left Trigger: 0.0 to +1.0
- Right Trigger: 0.0 to +1.0

```cpp
// Map left stick horizontal axis
sys.input->registerMapping({
    .binding = {
        .deviceType = InputDeviceType::Controller,
        .deviceIndex = 0,
        .keyCode = /* axis code */,
        .scale = 1.0f,
        .deadzone = 0.2f  // Ignore small movements
    },
    .action = "move_horizontal"
});

// In gameplay code
float moveX = sys.input->getActionValue("move_horizontal");
player.velocity.x = moveX * player.maxSpeed;
```

### Deadzone Handling

Analog sticks have **drift** (small unintended movements). Use deadzones to filter noise:

```cpp
InputBinding leftStickX{
    .deviceType = InputDeviceType::Controller,
    .deviceIndex = 0,
    .keyCode = /* axis code */,
    .scale = 1.0f,
    .deadzone = 0.2f  // Ignore values between -0.2 and +0.2
};
```

**Recommended deadzones**:
- Analog sticks: 0.15 - 0.25
- Triggers: 0.0 - 0.1

---

## Text Input

For text entry (chat, player names, console), enable text input mode:

```cpp
class ChatBox {
public:
    void open() {
        auto& sys = engine_->systems();
        sys.input->enableTextInput();
        sys.input->clearTextInput();
    }

    void close() {
        auto& sys = engine_->systems();
        sys.input->disableTextInput();
    }

    void update() {
        auto& sys = engine_->systems();

        if (!sys.input->isTextInputEnabled()) {
            return;
        }

        std::string text = sys.input->getTextInput();
        if (!text.empty()) {
            chatBuffer_ += text;
            sys.input->clearTextInput();  // Clear for next frame
        }

        // Handle backspace (use an action for special keys)
        if (sys.input->wasActionJustPressed("backspace")) {
            if (!chatBuffer_.empty()) {
                chatBuffer_.pop_back();
            }
        }

        // Submit with Enter
        if (sys.input->wasActionJustPressed("submit")) {
            submitChat(chatBuffer_);
            chatBuffer_.clear();
        }
    }

private:
    std::string chatBuffer_;

    void submitChat(const std::string& message) {
        // Send the chat message
    }
};
```

**Important**: Text input captures UTF-8 characters, not key presses. Use actions for special keys (Enter, Backspace, Escape).

---

## Input Listening and Rebinding

Allow players to remap controls at runtime using the **input listening** system.

### Listening for Input

When listening is active, the system captures the next input and makes it available via `getLastInput()`:

```cpp
// Start listening for input
sys.input->startListeningForInput();

// Check if currently listening
bool listening = sys.input->isListeningForInput();

// Get the last input captured (returns std::optional<InputBinding>)
auto maybeInput = sys.input->getLastInput();
if (maybeInput.has_value()) {
    InputBinding newBinding = *maybeInput;
    // Listening automatically stops after capturing input
}

// Stop listening manually
sys.input->stopListeningForInput();
```

### Complete Rebinding Example

```cpp
class RebindUI {
public:
    void startRebinding(const std::string& action) {
        auto& sys = engine_->systems();
        currentAction_ = action;
        sys.input->startListeningForInput();
        // Show UI: "Press any key to bind to 'jump'..."
    }

    void update() {
        auto& sys = engine_->systems();

        if (!sys.input->isListeningForInput()) {
            return;  // Not rebinding
        }

        // Check if player pressed something
        auto maybeInput = sys.input->getLastInput();
        if (maybeInput.has_value()) {
            InputBinding newBinding = *maybeInput;

            // Remove old binding for this action
            auto mappings = sys.input->getMappings();
            for (const auto& mapping : mappings) {
                if (mapping.action == currentAction_) {
                    sys.input->removeMapping(mapping.binding);
                }
            }

            // Add new binding
            sys.input->registerMapping({
                .binding = newBinding,
                .action = currentAction_
            });

            // Listening automatically stopped
            // Update UI: "Jump bound to Space"
        }
    }

private:
    std::string currentAction_;
};
```

### Rebinding Workflow

1. **Start listening**: `sys.input->startListeningForInput()`
2. **Wait for input**: Check `sys.input->getLastInput()` each frame
3. **Capture input**: When `getLastInput()` returns a value, listening automatically stops
4. **Update mapping**: Remove old binding, register new binding
5. **Save to config**: Persist mappings to JSON or Lua config

---

## Best Practices

### 1. Always Use Action Mapping

```cpp
// Good: Action-based
if (sys.input->isActionActive("jump")) {
    player.jump();
}

// Bad: Hard-coded keys (system doesn't even expose this)
// if (isKeyPressed(Key::Space)) { ... }  // NOT POSSIBLE
```

### 2. Call `update()` Once Per Frame

The system needs to update state for "just pressed" and "just released" detection:

```cpp
void updateFixed(DeltaTime dt) {
    // Input update is handled by the engine automatically
    // when you call engine.systems().input methods
    handleInput();
    updatePhysics(dt);
}
```

### 3. Use `wasJustPressed()` for Discrete Actions

```cpp
// Jump: Only on first frame of press
if (sys.input->wasActionJustPressed("jump")) {
    player.jump();
}

// Move: Continuous while held
if (sys.input->isActionActive("move_right")) {
    player.velocity.x = 200.0f;
}
```

### 4. Support All Input Devices

Always provide keyboard, mouse, **and** gamepad bindings:

```cpp
void setupUniversalControls() {
    auto& sys = engine_->systems();

    // Keyboard
    sys.input->registerMapping({
        .binding = {.deviceType = InputDeviceType::Keyboard, .keyCode = 32},
        .action = "jump"
    });

    // Gamepad
    sys.input->registerMapping({
        .binding = {.deviceType = InputDeviceType::Controller, .keyCode = 0},
        .action = "jump"
    });
}
```

### 5. Default to Dvorak-Friendly Bindings

```cpp
// Default movement: ,AOE (Dvorak home row)
void setupDefaultMovement() {
    auto& sys = engine_->systems();

    sys.input->registerMapping({.binding = {.deviceType = InputDeviceType::Keyboard, .keyCode = 44}, .action = "move_left"});   // ,
    sys.input->registerMapping({.binding = {.deviceType = InputDeviceType::Keyboard, .keyCode = 65}, .action = "move_down"});   // A
    sys.input->registerMapping({.binding = {.deviceType = InputDeviceType::Keyboard, .keyCode = 79}, .action = "move_up"});     // O
    sys.input->registerMapping({.binding = {.deviceType = InputDeviceType::Keyboard, .keyCode = 69}, .action = "move_right"});  // E

    // Also support Arrow keys as alternative
    sys.input->registerMapping({.binding = {.deviceType = InputDeviceType::Keyboard, .keyCode = 263}, .action = "move_left"});  // Left Arrow
    sys.input->registerMapping({.binding = {.deviceType = InputDeviceType::Keyboard, .keyCode = 264}, .action = "move_down"});  // Down Arrow
    sys.input->registerMapping({.binding = {.deviceType = InputDeviceType::Keyboard, .keyCode = 265}, .action = "move_up"});    // Up Arrow
    sys.input->registerMapping({.binding = {.deviceType = InputDeviceType::Keyboard, .keyCode = 262}, .action = "move_right"}); // Right Arrow
}
```

### 6. Implement Input Buffering for Responsive Feel

```cpp
class InputBuffer {
public:
    void update(float deltaTime) {
        auto& sys = engine_->systems();

        // Track jump presses with a time window
        if (sys.input->wasActionJustPressed("jump")) {
            jumpBufferTime_ = 0.1f;  // 100ms buffer
        }
        jumpBufferTime_ -= deltaTime;
    }

    bool consumeJumpBuffer() {
        if (jumpBufferTime_ > 0) {
            jumpBufferTime_ = 0;
            return true;
        }
        return false;
    }

private:
    float jumpBufferTime_ = 0;
};

// In game logic:
if (player.isGrounded() && inputBuffer.consumeJumpBuffer()) {
    player.jump();  // Jump even if grounded slightly after press
}
```

### 7. Persist Mappings to Config

Save and load mappings from JSON or Lua config files for persistence.

### 8. Accessibility: Allow Multiple Bindings

Let players bind multiple keys to the same action for accessibility:

```cpp
// Player can jump with Space OR Enter OR Gamepad A
sys.input->registerMapping({.binding = {.deviceType = InputDeviceType::Keyboard, .keyCode = 32}, .action = "jump"});   // Space
sys.input->registerMapping({.binding = {.deviceType = InputDeviceType::Keyboard, .keyCode = 257}, .action = "jump"});  // Enter
sys.input->registerMapping({.binding = {.deviceType = InputDeviceType::Controller, .keyCode = 0}, .action = "jump"});  // A button
```

---

## Complete Examples

### Example 1: Basic 2D Platformer Movement (Dvorak-Friendly)

```cpp
import bestow;
import bestow.core;

class PlatformerGame : public bestow::core::Application {
public:
    bool initialize(bestow::core::Engine& engine) override {
        engine_ = &engine;
        setupDvorakControls();
        player_ = createPlayer();
        return true;
    }

    void updateFixed(DeltaTime dt) override {
        handleMovement(dt);
        handleJump();
    }

    void render(float alpha) override {
        // Rendering logic
    }

    void shutdown() override {}

private:
    void setupDvorakControls() {
        auto& sys = engine_->systems();

        // Dvorak home row movement: ,AOE
        sys.input->registerMapping({
            .binding = {
                .deviceType = InputDeviceType::Keyboard,
                .keyCode = 44,  // Comma (,) - left
                .scale = -1.0f
            },
            .action = "move_horizontal"
        });

        sys.input->registerMapping({
            .binding = {
                .deviceType = InputDeviceType::Keyboard,
                .keyCode = 69,  // E - right
                .scale = 1.0f
            },
            .action = "move_horizontal"
        });

        // Jump: Space or O
        sys.input->registerMapping({
            .binding = {.deviceType = InputDeviceType::Keyboard, .keyCode = 32},
            .action = "jump"
        });
        sys.input->registerMapping({
            .binding = {.deviceType = InputDeviceType::Keyboard, .keyCode = 79},  // O
            .action = "jump"
        });

        // Gamepad support
        sys.input->registerMapping({
            .binding = {
                .deviceType = InputDeviceType::Controller,
                .keyCode = /* left stick X code */,
                .scale = 1.0f,
                .deadzone = 0.2f
            },
            .action = "move_horizontal"
        });
        sys.input->registerMapping({
            .binding = {.deviceType = InputDeviceType::Controller, .keyCode = 0},  // A button
            .action = "jump"
        });
    }

    void handleMovement(DeltaTime dt) {
        auto& sys = engine_->systems();
        float moveInput = sys.input->getActionValue("move_horizontal");

        Vec2 velocity = sys.physics->getLinearVelocity(player_);
        velocity.x = moveInput * moveSpeed_;
        sys.physics->setLinearVelocity(player_, velocity);
    }

    void handleJump() {
        auto& sys = engine_->systems();
        if (sys.input->wasActionJustPressed("jump") && isGrounded()) {
            Vec2 velocity = sys.physics->getLinearVelocity(player_);
            velocity.y = jumpForce_;
            sys.physics->setLinearVelocity(player_, velocity);
        }
    }

    bool isGrounded() {
        // Check if player is touching ground
        return true;  // Implementation varies
    }

    Entity createPlayer() {
        auto& sys = engine_->systems();
        Entity player = sys.entities->createEntity();
        // Setup player components...
        return player;
    }

    bestow::core::Engine* engine_ = nullptr;
    Entity player_;
    float moveSpeed_ = 300.0f;
    float jumpForce_ = -500.0f;
};
```

### Example 2: Controls Rebinding Menu

```cpp
class ControlsMenu {
public:
    ControlsMenu(bestow::core::Engine& engine) : engine_(&engine) {
        actions_ = {"move_left", "move_right", "jump", "attack", "pause"};
    }

    void update() {
        auto& sys = engine_->systems();

        if (isRebinding_) {
            updateRebinding();
        } else {
            updateMenu();
        }
    }

    void render() {
        if (isRebinding_) {
            drawText("Press any key to bind to: " + actions_[selectedIndex_]);
        } else {
            for (size_t i = 0; i < actions_.size(); ++i) {
                std::string binding = getBindingForAction(actions_[i]);
                std::string line = actions_[i] + ": " + binding;

                if (i == selectedIndex_) {
                    line = "> " + line;  // Highlight selected
                }

                drawText(line, i);
            }

            drawText("Press Enter to rebind, Escape to exit");
        }
    }

private:
    void updateMenu() {
        auto& sys = engine_->systems();

        // Navigate menu
        if (sys.input->wasActionJustPressed("move_up")) {
            selectedIndex_ = (selectedIndex_ - 1 + actions_.size()) % actions_.size();
        }
        if (sys.input->wasActionJustPressed("move_down")) {
            selectedIndex_ = (selectedIndex_ + 1) % actions_.size();
        }

        // Start rebinding
        if (sys.input->wasActionJustPressed("confirm")) {
            startRebinding();
        }
    }

    void updateRebinding() {
        auto& sys = engine_->systems();

        auto maybeInput = sys.input->getLastInput();
        if (maybeInput.has_value()) {
            // Got new input!
            InputBinding newBinding = *maybeInput;
            std::string action = actions_[selectedIndex_];

            // Remove old binding
            auto mappings = sys.input->getMappings();
            for (const auto& mapping : mappings) {
                if (mapping.action == action) {
                    sys.input->removeMapping(mapping.binding);
                }
            }

            // Register new binding
            sys.input->registerMapping({
                .binding = newBinding,
                .action = action
            });

            // Listening automatically stopped
            isRebinding_ = false;

            // Save to config
            saveMappingsToFile();
        }

        // Cancel rebinding
        if (sys.input->wasActionJustPressed("cancel")) {
            sys.input->stopListeningForInput();
            isRebinding_ = false;
        }
    }

    void startRebinding() {
        auto& sys = engine_->systems();
        sys.input->startListeningForInput();
        isRebinding_ = true;
    }

    std::string getBindingForAction(const std::string& action) {
        auto& sys = engine_->systems();
        auto mappings = sys.input->getMappings();
        for (const auto& mapping : mappings) {
            if (mapping.action == action) {
                return bindingToString(mapping.binding);
            }
        }
        return "(Unbound)";
    }

    std::string bindingToString(const InputBinding& binding) {
        if (binding.deviceType == InputDeviceType::Keyboard) {
            return "Key " + std::to_string(binding.keyCode);
        } else if (binding.deviceType == InputDeviceType::Mouse) {
            return "Mouse " + std::to_string(binding.keyCode);
        } else {
            return "Controller " + std::to_string(binding.keyCode);
        }
    }

    void saveMappingsToFile() {
        // Save mappings to persistent storage
    }

    void drawText(const std::string& text, int line = 0) {
        // Rendering implementation...
    }

    bestow::core::Engine* engine_;
    std::vector<std::string> actions_;
    size_t selectedIndex_ = 0;
    bool isRebinding_ = false;
};
```

---

## Summary

The Bestow Input System provides:

- **Action-based architecture**: Map hardware inputs to logical actions
- **Multi-device support**: Keyboard, mouse, and gamepads
- **Runtime rebinding**: Let players customize controls
- **Dvorak-first design**: Default to ,AOE movement for Dvorak users
- **Accessibility**: Support multiple bindings and alternative input methods

**Key Takeaways**:
1. All input goes through action mapping - no raw key queries
2. Use `wasJustPressed()` for discrete actions, `isActionActive()` for continuous
3. Default to Dvorak-friendly bindings (,AOE), but support alternatives
4. Support keyboard, mouse, AND gamepad for all gameplay actions
5. Implement input buffering for responsive gameplay
6. Persist mappings to config files for player customization

For questions or issues, refer to the test suite at `/tests/unit/InputSystemTests.cpp`.

---

**Next Steps**:
- Read the [Physics System Guide](PHYSICS-SYSTEM.md) for collision-based input (ground detection)
- Read the [Config System Guide](CONFIG-SYSTEM.md) for persisting input bindings
- See example games in `/examples/` for complete implementations

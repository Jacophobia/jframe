# Bestow Input System Guide

**Version:** 1.0
**Last Updated:** 2025-12-09

The Input System provides comprehensive handling for keyboard, mouse, and gamepad input with support for action mapping, rebinding, and multiple input devices. This guide covers everything game developers need to implement responsive, accessible input in their games.

---

## Table of Contents

1. [Overview](#overview)
2. [Core Concepts](#core-concepts)
3. [Keyboard Input](#keyboard-input)
4. [Mouse Input](#mouse-input)
5. [Gamepad Input](#gamepad-input)
6. [Action Mapping](#action-mapping)
7. [Input Rebinding](#input-rebinding)
8. [Text Input](#text-input)
9. [Best Practices](#best-practices)
10. [Complete Examples](#complete-examples)

---

## Overview

The Input System (`IInputSystem`) is the centralized interface for all player input in Bestow. It provides:

- **Multi-device support**: Keyboard, mouse, and up to 4 gamepads simultaneously
- **Action-based input**: Map hardware inputs to logical actions (e.g., "jump", "shoot")
- **Frame-perfect detection**: Track pressed, held, and released states
- **Live rebinding**: Allow players to remap controls at runtime
- **Dvorak-friendly defaults**: Optimized for Dvorak keyboard layout (,AOE movement)

### Basic Setup

```cpp
import bestow;

class MyGame : public IApplication {
public:
    MyGame(IInputSystem& input) : input_(&input) {}

    void initialize() {
        // Register action mappings
        input_->registerMapping({
            .binding = {
                .deviceType = InputDeviceType::Keyboard,
                .keyCode = 44,  // Comma (,) - left on Dvorak
            },
            .action = "move_left"
        });
    }

    void update() {
        input_->update();  // Call once per frame

        if (input_->isActionActive("move_left")) {
            // Player is moving left
        }
    }

private:
    IInputSystem* input_;
};
```

---

## Core Concepts

### Action vs Raw Input

**Actions** are logical game commands (e.g., "jump", "fire", "menu_open"). They are platform-agnostic and rebindable.

**Raw input** is direct hardware state (e.g., "Space key pressed", "Mouse X = 512"). Use raw input only for debugging or rebinding UI.

```cpp
// Good: Use actions for gameplay
if (input_->isActionActive("jump")) {
    player.jump();
}

// Avoid: Don't hard-code key checks in game logic
if (input_->isKeyPressed(GLFW_KEY_SPACE)) {  // NO! Tightly coupled to hardware
    player.jump();
}
```

### Input States

Every action and input has three states:

| State | Method | Description | Use Case |
|-------|--------|-------------|----------|
| **Active** | `isActionActive()` | Input is currently held down | Continuous movement, aiming |
| **Just Pressed** | `wasActionJustPressed()` | Input pressed this frame | Jump, shoot, open menu |
| **Just Released** | `wasActionJustReleased()` | Input released this frame | Charge attacks, drag-and-drop |

```cpp
void update() {
    // Continuous movement while held
    if (input_->isActionActive("move_right")) {
        player.velocity.x = 200.0f;
    }

    // Jump only on the first frame of press
    if (input_->wasActionJustPressed("jump")) {
        player.jump();
    }

    // Release a charged shot
    if (input_->wasActionJustReleased("charge")) {
        player.releaseChargedShot();
    }
}
```

### Action Values

Actions have both binary (active/inactive) and analog (0.0 to 1.0) states. This supports analog sticks and triggers:

```cpp
// Binary: Is the player moving?
bool isMoving = input_->isActionActive("move_horizontal");

// Analog: How much are they moving?
float moveAmount = input_->getActionValue("move_horizontal");
// moveAmount ranges from -1.0 (full left) to +1.0 (full right)

player.velocity.x = moveAmount * player.maxSpeed;
```

---

## Keyboard Input

### Key Codes

Bestow uses GLFW key codes. Common keys:

| Key | Code | Dvorak Position | QWERTY Position |
|-----|------|-----------------|-----------------|
| Comma (`,`) | 44 | Home row left | QWERTY Q position |
| A | 65 | Left hand middle | QWERTY A position |
| O | 79 | Right hand index | QWERTY S position |
| E | 69 | Right hand middle | QWERTY D position |
| Space | 32 | Spacebar | Spacebar |
| Escape | 256 | Escape | Escape |
| Enter | 257 | Enter | Enter |
| Tab | 258 | Tab | Tab |
| Left Shift | 340 | Left Shift | Left Shift |
| Left Ctrl | 341 | Left Ctrl | Left Ctrl |

**Full reference**: See GLFW documentation for complete key code list.

### Dvorak-Friendly Movement

**IMPORTANT**: The project owner uses Dvorak layout. Default movement should use **,AOE** (Dvorak home row), which maps to WASD positions on QWERTY keyboards.

```cpp
void setupDvorakMovement(IInputSystem* input) {
    // Left: Comma (,) - Dvorak left hand
    input->registerMapping({
        .binding = {.deviceType = InputDeviceType::Keyboard, .keyCode = 44},
        .action = "move_left"
    });

    // Down: A - Dvorak home row
    input->registerMapping({
        .binding = {.deviceType = InputDeviceType::Keyboard, .keyCode = 65},
        .action = "move_down"
    });

    // Up: O - Dvorak home row
    input->registerMapping({
        .binding = {.deviceType = InputDeviceType::Keyboard, .keyCode = 79},
        .action = "move_up"
    });

    // Right: E - Dvorak right hand
    input->registerMapping({
        .binding = {.deviceType = InputDeviceType::Keyboard, .keyCode = 69},
        .action = "move_right"
    });
}
```

### Keyboard State Queries

While action mapping is preferred, you can query raw keyboard state:

```cpp
// Check if a specific key is down (avoid in gameplay code)
bool spacePressed = input_->isKeyPressed(GLFW_KEY_SPACE);  // Not in interface
```

**Note**: The current interface does not expose `isKeyPressed()` directly. Use action mapping instead for all gameplay input.

---

## Mouse Input

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
if (input_->isMouseButtonDown(0)) {  // Left click
    fireWeapon();
}

// Map mouse buttons to actions
input_->registerMapping({
    .binding = {
        .deviceType = InputDeviceType::Mouse,
        .keyCode = 0,  // Left button
    },
    .action = "attack"
});
```

### Mouse Position and Delta

```cpp
// Get current mouse position (screen coordinates)
Vec2 mousePos = input_->getMousePosition();
// mousePos.x and mousePos.y are in pixels from top-left corner

// Get mouse movement this frame
Vec2 mouseDelta = input_->getMouseDelta();
// mouseDelta.x and mouseDelta.y show change since last frame

// Example: Camera rotation
void updateCamera() {
    Vec2 delta = input_->getMouseDelta();
    camera.rotation.y += delta.x * sensitivity;
    camera.rotation.x += delta.y * sensitivity;
}
```

### Scroll Wheel

```cpp
// Get scroll wheel movement this frame
Vec2 scroll = input_->getScrollDelta();
// scroll.y is vertical scroll (most common)
// scroll.x is horizontal scroll (trackpad two-finger swipe)

void update() {
    Vec2 scroll = input_->getScrollDelta();

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

## Gamepad Input

### Controller Support

Bestow supports up to **4 simultaneous controllers** using SDL2's GameController API. Controllers are automatically detected when connected.

```cpp
// Check how many controllers are connected
int numControllers = input_->getConnectedControllerCount();

// Check if specific controller slot is active
if (input_->isControllerConnected(0)) {
    std::string name = input_->getControllerName(0);
    // name = "Xbox Series X Controller", "DualShock 4", etc.
}
```

### Controller Buttons

SDL2 provides standardized button mappings for Xbox/PlayStation-style controllers:

| Button | SDL Code | Xbox | PlayStation |
|--------|----------|------|-------------|
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
input_->registerMapping({
    .binding = {
        .deviceType = InputDeviceType::Controller,
        .deviceIndex = 0,  // First controller
        .keyCode = 0,      // A button
    },
    .action = "jump"
});
```

### Controller Axes

Analog sticks and triggers use axes. Axis codes are offset by `SDL_CONTROLLER_BUTTON_MAX`:

| Axis | SDL Code | Range | Description |
|------|----------|-------|-------------|
| Left Stick X | 0 | -1.0 to +1.0 | Left = -1, Right = +1 |
| Left Stick Y | 1 | -1.0 to +1.0 | Up = -1, Down = +1 |
| Right Stick X | 2 | -1.0 to +1.0 | Left = -1, Right = +1 |
| Right Stick Y | 3 | -1.0 to +1.0 | Up = -1, Down = +1 |
| Left Trigger | 4 | 0.0 to +1.0 | Unpressed = 0, Pressed = +1 |
| Right Trigger | 5 | 0.0 to +1.0 | Unpressed = 0, Pressed = +1 |

```cpp
// Map left stick horizontal axis
input_->registerMapping({
    .binding = {
        .deviceType = InputDeviceType::Controller,
        .deviceIndex = 0,
        .keyCode = SDL_CONTROLLER_BUTTON_MAX + 0,  // Left Stick X
        .scale = 1.0f,
        .deadzone = 0.2f  // Ignore small movements
    },
    .action = "move_horizontal"
});

// In gameplay code
float moveX = input_->getActionValue("move_horizontal");
player.velocity.x = moveX * player.maxSpeed;
```

### Deadzone Handling

Analog sticks have **drift** (small unintended movements). Use deadzones to filter noise:

```cpp
InputBinding leftStickX{
    .deviceType = InputDeviceType::Controller,
    .deviceIndex = 0,
    .keyCode = SDL_CONTROLLER_BUTTON_MAX + 0,  // Left Stick X
    .scale = 1.0f,
    .deadzone = 0.2f  // Ignore values between -0.2 and +0.2
};
```

**Recommended deadzones**:
- Analog sticks: 0.15 - 0.25
- Triggers: 0.0 - 0.1

---

## Action Mapping

Action mapping decouples game logic from hardware inputs. Players can use keyboard, mouse, or gamepad interchangeably.

### Registering Mappings

```cpp
void setupControls(IInputSystem* input) {
    // Keyboard jump (Space)
    input->registerMapping({
        .binding = {.deviceType = InputDeviceType::Keyboard, .keyCode = 32},
        .action = "jump"
    });

    // Controller jump (A button)
    input->registerMapping({
        .binding = {.deviceType = InputDeviceType::Controller, .keyCode = 0},
        .action = "jump"
    });

    // Mouse jump (Left click)
    input->registerMapping({
        .binding = {.deviceType = InputDeviceType::Mouse, .keyCode = 0},
        .action = "jump"
    });

    // Now all three inputs trigger the same "jump" action!
}
```

### Multiple Bindings per Action

You can bind multiple inputs to the same action. The system returns the **maximum absolute value** when multiple inputs are active:

```cpp
// Bind both A and Left Arrow to "move_left"
input->registerMapping({
    .binding = {.deviceType = InputDeviceType::Keyboard, .keyCode = 65},
    .action = "move_left"
});
input->registerMapping({
    .binding = {.deviceType = InputDeviceType::Keyboard, .keyCode = 263},  // Left Arrow
    .action = "move_left"
});

// Both keys will activate the same action
if (input->isActionActive("move_left")) {
    // Triggered by either A or Left Arrow
}
```

### Directional Actions with Scale

Use `scale` to create opposing directional actions:

```cpp
void setupHorizontalMovement(IInputSystem* input) {
    // Left = negative scale
    input->registerMapping({
        .binding = {
            .deviceType = InputDeviceType::Keyboard,
            .keyCode = 44,  // Comma
            .scale = -1.0f  // Negative for left
        },
        .action = "move_horizontal"
    });

    // Right = positive scale
    input->registerMapping({
        .binding = {
            .deviceType = InputDeviceType::Keyboard,
            .keyCode = 69,  // E
            .scale = 1.0f   // Positive for right
        },
        .action = "move_horizontal"
    });

    // Controller left stick (already analog)
    input->registerMapping({
        .binding = {
            .deviceType = InputDeviceType::Controller,
            .keyCode = SDL_CONTROLLER_BUTTON_MAX + 0,  // Left Stick X
            .scale = 1.0f,
            .deadzone = 0.2f
        },
        .action = "move_horizontal"
    });

    // In gameplay:
    float moveX = input->getActionValue("move_horizontal");
    // Comma = -1.0, E = +1.0, stick = -1.0 to +1.0
}
```

### Removing Mappings

```cpp
// Remove specific mapping
InputBinding toRemove{
    .deviceType = InputDeviceType::Keyboard,
    .keyCode = 32  // Space
};
input->removeMapping(toRemove);

// Remove all mappings
input->clearMappings();
```

---

## Input Rebinding

Allow players to remap controls at runtime using the **input listening** system.

### Listening for Input

```cpp
class RebindUI {
public:
    void startRebinding(IInputSystem* input, const std::string& action) {
        currentAction_ = action;
        input->startListeningForInput();
        // Show UI: "Press any key to bind to 'jump'..."
    }

    void update(IInputSystem* input) {
        if (!input->isListeningForInput()) {
            return;  // Not rebinding
        }

        // Check if player pressed something
        auto maybeInput = input->getLastInput();
        if (maybeInput.has_value()) {
            InputBinding newBinding = *maybeInput;

            // Remove old binding for this action
            auto mappings = input->getMappings();
            for (const auto& mapping : mappings) {
                if (mapping.action == currentAction_) {
                    input->removeMapping(mapping.binding);
                }
            }

            // Add new binding
            input->registerMapping({
                .binding = newBinding,
                .action = currentAction_
            });

            // Stop listening
            input->stopListeningForInput();

            // Update UI: "Jump bound to Space"
        }
    }

private:
    std::string currentAction_;
};
```

### Rebinding Workflow

1. **Start listening**: `input->startListeningForInput()`
2. **Wait for input**: Check `input->getLastInput()` each frame
3. **Capture input**: When `getLastInput()` returns a value, listening automatically stops
4. **Update mapping**: Remove old binding, register new binding
5. **Save to config**: Persist mappings to JSON or Lua config

```cpp
// Example: Complete rebind flow
void rebindAction(IInputSystem* input, const std::string& action) {
    input->startListeningForInput();

    while (input->isListeningForInput()) {
        input->update();

        auto maybeInput = input->getLastInput();
        if (maybeInput) {
            // Got input! Update mapping
            input->registerMapping({
                .binding = *maybeInput,
                .action = action
            });
            break;
        }
    }
}
```

---

## Text Input

For text entry (chat, player names, console), enable text input mode:

```cpp
class ChatBox {
public:
    void open(IInputSystem* input) {
        input->enableTextInput();
        input->clearTextInput();
    }

    void close(IInputSystem* input) {
        input->disableTextInput();
    }

    void update(IInputSystem* input) {
        if (!input->isTextInputEnabled()) return;

        std::string text = input->getTextInput();
        if (!text.empty()) {
            chatBuffer_ += text;
            input->clearTextInput();  // Clear for next frame
        }

        // Handle backspace (not part of text input)
        if (input->wasActionJustPressed("backspace")) {
            if (!chatBuffer_.empty()) {
                chatBuffer_.pop_back();
            }
        }
    }

private:
    std::string chatBuffer_;
};
```

**Important**: Text input captures UTF-8 characters, not key presses. Use actions for special keys (Enter, Backspace, Escape).

---

## Best Practices

### 1. Always Use Action Mapping

```cpp
// Good: Action-based
if (input->isActionActive("jump")) {
    player.jump();
}

// Bad: Hard-coded keys
if (/* raw key check */) {
    player.jump();
}
```

### 2. Call `update()` Once Per Frame

```cpp
void gameLoop() {
    while (running) {
        input->update();  // First thing each frame
        handleInput();
        updatePhysics();
        render();
    }
}
```

### 3. Use `wasJustPressed()` for Discrete Actions

```cpp
// Jump: Only on first frame of press
if (input->wasActionJustPressed("jump")) {
    player.jump();
}

// Move: Continuous while held
if (input->isActionActive("move_right")) {
    player.velocity.x = 200.0f;
}
```

### 4. Support All Input Devices

Always provide keyboard, mouse, **and** gamepad bindings:

```cpp
void setupUniversalControls(IInputSystem* input) {
    // Keyboard
    input->registerMapping({
        .binding = {.deviceType = InputDeviceType::Keyboard, .keyCode = 32},
        .action = "jump"
    });

    // Gamepad
    input->registerMapping({
        .binding = {.deviceType = InputDeviceType::Controller, .keyCode = 0},
        .action = "jump"
    });
}
```

### 5. Default to Dvorak-Friendly Bindings

```cpp
// Default movement: ,AOE (Dvorak home row)
void setupDefaultMovement(IInputSystem* input) {
    input->registerMapping({.binding = {.deviceType = InputDeviceType::Keyboard, .keyCode = 44}, .action = "move_left"});   // ,
    input->registerMapping({.binding = {.deviceType = InputDeviceType::Keyboard, .keyCode = 65}, .action = "move_down"});   // A
    input->registerMapping({.binding = {.deviceType = InputDeviceType::Keyboard, .keyCode = 79}, .action = "move_up"});     // O
    input->registerMapping({.binding = {.deviceType = InputDeviceType::Keyboard, .keyCode = 69}, .action = "move_right"});  // E

    // Also support Arrow keys as alternative
    input->registerMapping({.binding = {.deviceType = InputDeviceType::Keyboard, .keyCode = 263}, .action = "move_left"});  // Left Arrow
    input->registerMapping({.binding = {.deviceType = InputDeviceType::Keyboard, .keyCode = 264}, .action = "move_down"});  // Down Arrow
    input->registerMapping({.binding = {.deviceType = InputDeviceType::Keyboard, .keyCode = 265}, .action = "move_up"});    // Up Arrow
    input->registerMapping({.binding = {.deviceType = InputDeviceType::Keyboard, .keyCode = 262}, .action = "move_right"}); // Right Arrow
}
```

### 6. Implement Input Buffering for Responsive Feel

```cpp
class InputBuffer {
public:
    void update(IInputSystem* input, float deltaTime) {
        // Track jump presses with a time window
        if (input->wasActionJustPressed("jump")) {
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

```cpp
// Save mappings to JSON
void saveMappings(IInputSystem* input) {
    nlohmann::json config;
    auto mappings = input->getMappings();

    for (const auto& mapping : mappings) {
        config["mappings"].push_back({
            {"action", mapping.action},
            {"deviceType", static_cast<int>(mapping.binding.deviceType)},
            {"keyCode", mapping.binding.keyCode},
            {"scale", mapping.binding.scale},
            {"deadzone", mapping.binding.deadzone}
        });
    }

    // Write to file...
}

// Load mappings from JSON
void loadMappings(IInputSystem* input, const nlohmann::json& config) {
    input->clearMappings();

    for (const auto& item : config["mappings"]) {
        input->registerMapping({
            .binding = {
                .deviceType = static_cast<InputDeviceType>(item["deviceType"]),
                .keyCode = item["keyCode"],
                .scale = item["scale"],
                .deadzone = item["deadzone"]
            },
            .action = item["action"]
        });
    }
}
```

### 8. Accessibility: Allow Multiple Bindings

Let players bind multiple keys to the same action for accessibility:

```cpp
// Player can jump with Space OR Enter OR Gamepad A
input->registerMapping({.binding = {.deviceType = InputDeviceType::Keyboard, .keyCode = 32}, .action = "jump"});   // Space
input->registerMapping({.binding = {.deviceType = InputDeviceType::Keyboard, .keyCode = 257}, .action = "jump"});  // Enter
input->registerMapping({.binding = {.deviceType = InputDeviceType::Controller, .keyCode = 0}, .action = "jump"});  // A button
```

---

## Complete Examples

### Example 1: Basic 2D Platformer Movement (Dvorak-Friendly)

```cpp
import bestow;

class PlatformerGame : public IApplication {
public:
    PlatformerGame(IInputSystem& input, IPhysicsSystem& physics)
        : input_(&input), physics_(&physics) {}

    void initialize() {
        setupDvorakControls();
        player_ = createPlayer();
    }

    void update(float deltaTime) {
        input_->update();
        handleMovement(deltaTime);
        handleJump();
    }

private:
    void setupDvorakControls() {
        // Dvorak home row movement: ,AOE
        input_->registerMapping({
            .binding = {
                .deviceType = InputDeviceType::Keyboard,
                .keyCode = 44,  // Comma (,) - left
                .scale = -1.0f
            },
            .action = "move_horizontal"
        });

        input_->registerMapping({
            .binding = {
                .deviceType = InputDeviceType::Keyboard,
                .keyCode = 69,  // E - right
                .scale = 1.0f
            },
            .action = "move_horizontal"
        });

        // Jump: Space or O
        input_->registerMapping({
            .binding = {.deviceType = InputDeviceType::Keyboard, .keyCode = 32},
            .action = "jump"
        });
        input_->registerMapping({
            .binding = {.deviceType = InputDeviceType::Keyboard, .keyCode = 79},  // O
            .action = "jump"
        });

        // Gamepad support
        input_->registerMapping({
            .binding = {
                .deviceType = InputDeviceType::Controller,
                .keyCode = SDL_CONTROLLER_BUTTON_MAX + 0,  // Left Stick X
                .scale = 1.0f,
                .deadzone = 0.2f
            },
            .action = "move_horizontal"
        });
        input_->registerMapping({
            .binding = {.deviceType = InputDeviceType::Controller, .keyCode = 0},  // A button
            .action = "jump"
        });
    }

    void handleMovement(float deltaTime) {
        float moveInput = input_->getActionValue("move_horizontal");
        Vec2 velocity = physics_->getLinearVelocity(player_);

        // Immediate direction change for tight platformer feel
        velocity.x = moveInput * moveSpeed_;

        physics_->setLinearVelocity(player_, velocity);
    }

    void handleJump() {
        if (input_->wasActionJustPressed("jump") && isGrounded()) {
            Vec2 velocity = physics_->getLinearVelocity(player_);
            velocity.y = jumpForce_;
            physics_->setLinearVelocity(player_, velocity);
        }
    }

    bool isGrounded() {
        // Check if player is touching ground (implementation varies)
        return physics_->raycast(player_, {0, 1}, 0.1f).has_value();
    }

    Entity createPlayer() {
        // Entity creation logic...
        return Entity{};
    }

    IInputSystem* input_;
    IPhysicsSystem* physics_;
    Entity player_;
    float moveSpeed_ = 300.0f;
    float jumpForce_ = -500.0f;
};
```

### Example 2: Advanced Jump with Coyote Time and Input Buffer

```cpp
class AdvancedPlatformerController {
public:
    void update(IInputSystem* input, IPhysicsSystem* physics, float deltaTime) {
        // Update timers
        if (isGrounded(physics)) {
            coyoteTime_ = coyoteTimeWindow_;
        } else {
            coyoteTime_ -= deltaTime;
        }

        if (input->wasActionJustPressed("jump")) {
            jumpBufferTime_ = jumpBufferWindow_;
        } else {
            jumpBufferTime_ -= deltaTime;
        }

        // Jump if:
        // 1. Player recently left ground (coyote time), AND
        // 2. Player recently pressed jump (input buffer)
        if (coyoteTime_ > 0 && jumpBufferTime_ > 0 && !hasJumped_) {
            performJump(physics);
            coyoteTime_ = 0;
            jumpBufferTime_ = 0;
            hasJumped_ = true;
        }

        // Reset jump flag when grounded
        if (isGrounded(physics)) {
            hasJumped_ = false;
        }

        // Variable jump height: Release jump early for short hop
        if (input->wasActionJustReleased("jump")) {
            Vec2 velocity = physics->getLinearVelocity(player_);
            if (velocity.y < 0) {  // Moving upward (negative Y)
                velocity.y *= 0.5f;  // Cut upward velocity in half
                physics->setLinearVelocity(player_, velocity);
            }
        }
    }

private:
    void performJump(IPhysicsSystem* physics) {
        Vec2 velocity = physics->getLinearVelocity(player_);
        velocity.y = jumpForce_;
        physics->setLinearVelocity(player_, velocity);
    }

    bool isGrounded(IPhysicsSystem* physics) {
        return physics->raycast(player_, {0, 1}, 0.1f).has_value();
    }

    Entity player_;
    float jumpForce_ = -500.0f;

    // Coyote time: Grace period after leaving ground
    float coyoteTime_ = 0;
    float coyoteTimeWindow_ = 0.15f;  // 150ms

    // Input buffer: Accept jump presses slightly before landing
    float jumpBufferTime_ = 0;
    float jumpBufferWindow_ = 0.1f;  // 100ms

    bool hasJumped_ = false;
};
```

### Example 3: Twin-Stick Shooter with Gamepad

```cpp
class TwinStickShooter {
public:
    void initialize(IInputSystem* input) {
        // Movement: Left stick
        input->registerMapping({
            .binding = {
                .deviceType = InputDeviceType::Controller,
                .keyCode = SDL_CONTROLLER_BUTTON_MAX + 0,  // Left Stick X
                .scale = 1.0f,
                .deadzone = 0.2f
            },
            .action = "move_horizontal"
        });
        input->registerMapping({
            .binding = {
                .deviceType = InputDeviceType::Controller,
                .keyCode = SDL_CONTROLLER_BUTTON_MAX + 1,  // Left Stick Y
                .scale = 1.0f,
                .deadzone = 0.2f
            },
            .action = "move_vertical"
        });

        // Aiming: Right stick
        input->registerMapping({
            .binding = {
                .deviceType = InputDeviceType::Controller,
                .keyCode = SDL_CONTROLLER_BUTTON_MAX + 2,  // Right Stick X
                .scale = 1.0f,
                .deadzone = 0.2f
            },
            .action = "aim_horizontal"
        });
        input->registerMapping({
            .binding = {
                .deviceType = InputDeviceType::Controller,
                .keyCode = SDL_CONTROLLER_BUTTON_MAX + 3,  // Right Stick Y
                .scale = 1.0f,
                .deadzone = 0.2f
            },
            .action = "aim_vertical"
        });

        // Keyboard fallback: ,AOE for movement, Arrow keys for aim
        setupKeyboardControls(input);
    }

    void update(IInputSystem* input, float deltaTime) {
        // Movement
        float moveX = input->getActionValue("move_horizontal");
        float moveY = input->getActionValue("move_vertical");
        player_.velocity = Vec2{moveX, moveY} * moveSpeed_;

        // Aiming
        float aimX = input->getActionValue("aim_horizontal");
        float aimY = input->getActionValue("aim_vertical");

        // Only update aim if stick is pushed
        if (std::abs(aimX) > 0.01f || std::abs(aimY) > 0.01f) {
            player_.aimAngle = std::atan2(aimY, aimX);

            // Auto-fire when aiming
            fireWeapon();
        }
    }

private:
    void setupKeyboardControls(IInputSystem* input) {
        // Movement: ,AOE (Dvorak)
        input->registerMapping({.binding = {.deviceType = InputDeviceType::Keyboard, .keyCode = 44, .scale = -1.0f}, .action = "move_horizontal"});  // ,
        input->registerMapping({.binding = {.deviceType = InputDeviceType::Keyboard, .keyCode = 69, .scale = 1.0f}, .action = "move_horizontal"});   // E
        input->registerMapping({.binding = {.deviceType = InputDeviceType::Keyboard, .keyCode = 65, .scale = 1.0f}, .action = "move_vertical"});     // A
        input->registerMapping({.binding = {.deviceType = InputDeviceType::Keyboard, .keyCode = 79, .scale = -1.0f}, .action = "move_vertical"});    // O

        // Aim: Arrow keys
        input->registerMapping({.binding = {.deviceType = InputDeviceType::Keyboard, .keyCode = 263, .scale = -1.0f}, .action = "aim_horizontal"});  // Left
        input->registerMapping({.binding = {.deviceType = InputDeviceType::Keyboard, .keyCode = 262, .scale = 1.0f}, .action = "aim_horizontal"});   // Right
        input->registerMapping({.binding = {.deviceType = InputDeviceType::Keyboard, .keyCode = 264, .scale = 1.0f}, .action = "aim_vertical"});     // Down
        input->registerMapping({.binding = {.deviceType = InputDeviceType::Keyboard, .keyCode = 265, .scale = -1.0f}, .action = "aim_vertical"});    // Up
    }

    void fireWeapon() {
        // Weapon firing logic...
    }

    struct Player {
        Vec2 velocity;
        float aimAngle;
    } player_;

    float moveSpeed_ = 250.0f;
};
```

### Example 4: Controls Rebinding Menu

```cpp
class ControlsMenu {
public:
    ControlsMenu(IInputSystem& input) : input_(&input) {
        // List of rebindable actions
        actions_ = {"move_left", "move_right", "jump", "attack", "pause"};
    }

    void update() {
        input_->update();

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
        // Navigate menu
        if (input_->wasActionJustPressed("move_up")) {
            selectedIndex_ = (selectedIndex_ - 1 + actions_.size()) % actions_.size();
        }
        if (input_->wasActionJustPressed("move_down")) {
            selectedIndex_ = (selectedIndex_ + 1) % actions_.size();
        }

        // Start rebinding
        if (input_->wasActionJustPressed("confirm")) {
            startRebinding();
        }
    }

    void updateRebinding() {
        auto maybeInput = input_->getLastInput();
        if (maybeInput.has_value()) {
            // Got new input!
            InputBinding newBinding = *maybeInput;
            std::string action = actions_[selectedIndex_];

            // Remove old binding
            auto mappings = input_->getMappings();
            for (const auto& mapping : mappings) {
                if (mapping.action == action) {
                    input_->removeMapping(mapping.binding);
                }
            }

            // Register new binding
            input_->registerMapping({
                .binding = newBinding,
                .action = action
            });

            // Stop rebinding
            input_->stopListeningForInput();
            isRebinding_ = false;

            // Save to config
            saveMappingsToFile();
        }

        // Cancel rebinding
        if (input_->wasActionJustPressed("cancel")) {
            input_->stopListeningForInput();
            isRebinding_ = false;
        }
    }

    void startRebinding() {
        input_->startListeningForInput();
        isRebinding_ = true;
    }

    std::string getBindingForAction(const std::string& action) {
        auto mappings = input_->getMappings();
        for (const auto& mapping : mappings) {
            if (mapping.action == action) {
                return bindingToString(mapping.binding);
            }
        }
        return "(Unbound)";
    }

    std::string bindingToString(const InputBinding& binding) {
        // Convert binding to human-readable string
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
        // (See Best Practices section for JSON serialization example)
    }

    void drawText(const std::string& text, int line = 0) {
        // Rendering implementation...
    }

    IInputSystem* input_;
    std::vector<std::string> actions_;
    size_t selectedIndex_ = 0;
    bool isRebinding_ = false;
};
```

---

## Summary

The Bestow Input System provides:

- **Universal input handling**: Keyboard, mouse, and gamepads
- **Action mapping**: Decouple game logic from hardware
- **Runtime rebinding**: Let players customize controls
- **Dvorak-first design**: Default to ,AOE movement for Dvorak users
- **Accessibility**: Support multiple bindings and alternative input methods

**Key Takeaways**:
1. Always use action mapping instead of raw input checks
2. Call `input->update()` once per frame, before gameplay logic
3. Use `wasJustPressed()` for discrete actions, `isActionActive()` for continuous
4. Default to Dvorak-friendly bindings (,AOE), but support alternatives
5. Support keyboard, mouse, AND gamepad for all gameplay actions
6. Implement input buffering and coyote time for responsive gameplay

For questions or issues, refer to the test suite at `/tests/unit/InputSystemTests.cpp`.

---

**Next Steps**:
- Read the [Physics System Guide](PHYSICS-SYSTEM.md) for collision-based input (ground detection)
- Read the [Config System Guide](CONFIG-SYSTEM.md) for persisting input bindings
- See example games in `/examples/` for complete implementations

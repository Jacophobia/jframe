# Tutorial 03: Handling Input

In this tutorial, you'll learn how to handle keyboard, mouse, and gamepad input using Bestow's action mapping system. You'll understand how to create flexible, rebindable controls that work across all input devices.

## What You'll Build

By the end of this tutorial, you'll have:
- Action-based input system (abstract controls)
- Keyboard, mouse, and gamepad support
- Rebindable controls
- Text input for UI fields
- Mouse position and delta tracking
- Dvorak-friendly default bindings

## Prerequisites

Before starting, complete Tutorials 01 and 02 and understand:
- The game loop (updateFixed, render)
- Entity creation and components
- Basic rendering

## Understanding Action Mapping

Bestow uses an **action mapping system** that separates input devices from game logic. Instead of checking "is the A key pressed?", you check "is the move_left action active?".

### Why Action Mapping?

**Without Action Mapping (Bad):**
```cpp
// Tightly coupled to specific keys
if (keyboard.isKeyPressed('A')) {
    moveLeft();
}
if (gamepad.getLeftStickX() < -0.5f) {
    moveLeft();
}
// Hard to rebind, device-specific, lots of code
```

**With Action Mapping (Good):**
```cpp
// Device-agnostic
if (input->isActionActive("move_left")) {
    moveLeft();
}
// Works with any device, easy to rebind, clean code
```

### Key Concepts

- **Action**: Named gameplay command ("jump", "attack", "move_left")
- **Binding**: Maps physical input (key/button/axis) to an action
- **Mapping**: Combines binding with action name
- **Device Type**: Keyboard, Mouse, or Gamepad
- **Scale**: Multiplier for axis values (-1.0, 0.0, 1.0)

## Step 1: Creating Input Mappings

Define your game actions and bind them to physical inputs.

### Register Keyboard Mappings

```cpp
// In Game.cpp - initialize()
auto& sys = engine_->systems();

// Move right - Dvorak-friendly 'E' key
sys.input->registerMapping(bestow::InputMapping{
    .binding = bestow::InputBinding{
        .deviceType = bestow::InputDeviceType::Keyboard,
        .keyCode = 'E',  // Dvorak equivalent of WASD's 'D'
        .scale = 1.0f
    },
    .action = "move_right"
});

// Move left - Dvorak-friendly comma key
sys.input->registerMapping(bestow::InputMapping{
    .binding = bestow::InputBinding{
        .deviceType = bestow::InputDeviceType::Keyboard,
        .keyCode = ',',  // Dvorak equivalent of WASD's 'A'
        .scale = 1.0f
    },
    .action = "move_left"
});

// Move up - Dvorak-friendly 'O' key
sys.input->registerMapping(bestow::InputMapping{
    .binding = bestow::InputBinding{
        .deviceType = bestow::InputDeviceType::Keyboard,
        .keyCode = 'O',  // Dvorak equivalent of WASD's 'W'
        .scale = 1.0f
    },
    .action = "move_up"
});

// Move down - Dvorak-friendly 'A' key
sys.input->registerMapping(bestow::InputMapping{
    .binding = bestow::InputBinding{
        .deviceType = bestow::InputDeviceType::Keyboard,
        .keyCode = 'A',  // Dvorak equivalent of WASD's 'S'
        .scale = 1.0f
    },
    .action = "move_down"
});

// Jump - Spacebar (universal)
sys.input->registerMapping(bestow::InputMapping{
    .binding = bestow::InputBinding{
        .deviceType = bestow::InputDeviceType::Keyboard,
        .keyCode = 32  // Spacebar
    },
    .action = "jump"
});

// Attack - Left Shift
sys.input->registerMapping(bestow::InputMapping{
    .binding = bestow::InputBinding{
        .deviceType = bestow::InputDeviceType::Keyboard,
        .keyCode = 340  // Left Shift
    },
    .action = "attack"
});
```

### Dvorak Layout Note

**CRITICAL:** The project owner uses Dvorak keyboard layout. Default controls should use **,AOE** for movement (Dvorak equivalent of WASD's physical positions), not WASD. This provides the same ergonomic positioning for Dvorak users.

| QWERTY | Dvorak | Action |
|--------|--------|--------|
| W | , (comma) | Move Up |
| A | A | Move Down |
| S | O | Move Left |
| D | E | Move Right |

### Common Key Codes

```cpp
// Letters (use uppercase)
'A' to 'Z'

// Numbers
'0' to '9'

// Special keys
32    // Space
256   // Escape
257   // Enter
258   // Tab
259   // Backspace
340   // Left Shift
341   // Left Control
342   // Left Alt
344   // Right Shift
345   // Right Control
346   // Right Alt

// Arrow keys
262   // Right Arrow
263   // Left Arrow
264   // Down Arrow
265   // Up Arrow

// Function keys
290   // F1
291   // F2
// ... up to F12 (301)
```

## Step 2: Querying Action State

Check action state in your update loop.

### Button-Style Actions (Pressed/Released)

```cpp
// In Game.cpp - updateFixed()
auto& sys = engine_->systems();

// Check if action is currently active (held down)
if (sys.input->isActionActive("jump")) {
    bestow::core::logInfo("Jump button is held");
}

// Check if action was just pressed this frame (useful for jumps)
if (sys.input->wasActionJustPressed("jump")) {
    bestow::core::logInfo("Jump button pressed!");
    applyJumpForce();
}

// Check if action was just released this frame
if (sys.input->wasActionJustReleased("attack")) {
    bestow::core::logInfo("Attack button released");
    executeAttack();
}
```

### Axis-Style Actions (Analog Values)

```cpp
// Get analog value (useful for movement)
float horizontal = sys.input->getActionValue("move_horizontal");
// Returns: -1.0 (left), 0.0 (neutral), 1.0 (right)

float vertical = sys.input->getActionValue("move_vertical");
// Returns: -1.0 (down), 0.0 (neutral), 1.0 (up)

// Apply movement
bestow::Vec2 velocity = {
    horizontal * moveSpeed_,
    vertical * moveSpeed_
};
sys.physics->setVelocity(player_, velocity);
```

### Complete Movement Example

```cpp
void Game::updateFixed(bestow::DeltaTime dt) {
    auto& sys = engine_->systems();

    // Get movement input (keyboard or gamepad)
    float horizontal = 0.0f;
    float vertical = 0.0f;

    if (sys.input->isActionActive("move_right")) horizontal += 1.0f;
    if (sys.input->isActionActive("move_left")) horizontal -= 1.0f;
    if (sys.input->isActionActive("move_up")) vertical += 1.0f;
    if (sys.input->isActionActive("move_down")) vertical -= 1.0f;

    // Normalize diagonal movement
    if (horizontal != 0.0f && vertical != 0.0f) {
        float length = std::sqrt(horizontal * horizontal + vertical * vertical);
        horizontal /= length;
        vertical /= length;
    }

    // Apply velocity
    constexpr float MOVE_SPEED = 200.0f;
    bestow::Vec2 velocity = {
        horizontal * MOVE_SPEED,
        vertical * MOVE_SPEED
    };
    sys.physics->setVelocity(player_, velocity);

    // Jump (only if just pressed)
    if (sys.input->wasActionJustPressed("jump")) {
        if (isGrounded()) {
            applyJumpForce();
        }
    }

    // Attack (charge while held, release to fire)
    if (sys.input->isActionActive("attack")) {
        chargeAttack(dt);
    }
    if (sys.input->wasActionJustReleased("attack")) {
        releaseAttack();
    }
}
```

## Step 3: Mouse Input

Handle mouse position, clicks, and delta movement.

### Mouse Position

```cpp
// In Game.cpp - updateFixed()
auto& sys = engine_->systems();

// Get mouse position in screen coordinates (pixels)
bestow::Vec2 mousePos = sys.input->getMousePosition();
bestow::core::logInfo(std::format("Mouse: ({}, {})", mousePos.x, mousePos.y));

// Convert screen to world coordinates (account for camera)
bestow::Vec2 worldPos = sys.graphics->screenToWorld(mousePos, camera_);

// Aim at mouse position
bestow::Vec2 playerPos = sys.physics->getPosition(player_);
bestow::Vec2 direction = {
    worldPos.x - playerPos.x,
    worldPos.y - playerPos.y
};

// Normalize direction
float length = std::sqrt(direction.x * direction.x + direction.y * direction.y);
if (length > 0.0f) {
    direction.x /= length;
    direction.y /= length;
}

// Set player rotation to face mouse
float angle = std::atan2(direction.y, direction.x) * (180.0f / 3.14159f);
auto& transform = sys.entities->get<bestow::Transform2D>(player_);
transform.rotation = angle;
```

### Mouse Buttons

```cpp
// Register mouse button mappings
sys.input->registerMapping(bestow::InputMapping{
    .binding = bestow::InputBinding{
        .deviceType = bestow::InputDeviceType::Mouse,
        .keyCode = 0  // Left mouse button
    },
    .action = "fire"
});

sys.input->registerMapping(bestow::InputMapping{
    .binding = bestow::InputBinding{
        .deviceType = bestow::InputDeviceType::Mouse,
        .keyCode = 1  // Right mouse button
    },
    .action = "aim"
});

sys.input->registerMapping(bestow::InputMapping{
    .binding = bestow::InputBinding{
        .deviceType = bestow::InputDeviceType::Mouse,
        .keyCode = 2  // Middle mouse button
    },
    .action = "special"
});

// In updateFixed()
if (sys.input->wasActionJustPressed("fire")) {
    // Shoot at mouse position
    shootProjectile(worldPos);
}
```

### Mouse Delta (Camera Control)

```cpp
// Get mouse movement since last frame
bestow::Vec2 mouseDelta = sys.input->getMouseDelta();

// Free-look camera
camera_.rotation += mouseDelta.x * 0.1f;  // Horizontal rotation
camera_.pitch += mouseDelta.y * 0.1f;     // Vertical rotation

// Clamp pitch
camera_.pitch = std::clamp(camera_.pitch, -89.0f, 89.0f);
```

### Mouse Scroll Wheel

```cpp
// Register scroll wheel as zoom action
sys.input->registerMapping(bestow::InputMapping{
    .binding = bestow::InputBinding{
        .deviceType = bestow::InputDeviceType::Mouse,
        .keyCode = -1,  // Scroll wheel (special code)
        .scale = 1.0f
    },
    .action = "zoom"
});

// In updateFixed()
float scrollDelta = sys.input->getActionValue("zoom");
if (scrollDelta != 0.0f) {
    camera_.zoom += scrollDelta * 0.1f;
    camera_.zoom = std::clamp(camera_.zoom, 0.5f, 3.0f);
}
```

## Step 4: Gamepad Support

Handle analog sticks and buttons.

### Gamepad Button Mappings

```cpp
// Register gamepad buttons
sys.input->registerMapping(bestow::InputMapping{
    .binding = bestow::InputBinding{
        .deviceType = bestow::InputDeviceType::Gamepad,
        .keyCode = 0  // A button (Xbox) / Cross (PlayStation)
    },
    .action = "jump"
});

sys.input->registerMapping(bestow::InputMapping{
    .binding = bestow::InputBinding{
        .deviceType = bestow::InputDeviceType::Gamepad,
        .keyCode = 1  // B button (Xbox) / Circle (PlayStation)
    },
    .action = "attack"
});

sys.input->registerMapping(bestow::InputMapping{
    .binding = bestow::InputBinding{
        .deviceType = bestow::InputDeviceType::Gamepad,
        .keyCode = 2  // X button (Xbox) / Square (PlayStation)
    },
    .action = "interact"
});

sys.input->registerMapping(bestow::InputMapping{
    .binding = bestow::InputBinding{
        .deviceType = bestow::InputDeviceType::Gamepad,
        .keyCode = 3  // Y button (Xbox) / Triangle (PlayStation)
    },
    .action = "special"
});
```

### Gamepad Button Reference

| Button | Xbox | PlayStation | Code |
|--------|------|-------------|------|
| A | A | Cross | 0 |
| B | B | Circle | 1 |
| X | X | Square | 2 |
| Y | Y | Triangle | 3 |
| L1/LB | Left Bumper | L1 | 4 |
| R1/RB | Right Bumper | R1 | 5 |
| Back | Back | Select | 6 |
| Start | Start | Start | 7 |
| L3 | Left Stick Click | L3 | 8 |
| R3 | Right Stick Click | R3 | 9 |

### Analog Sticks

```cpp
// Register left stick horizontal axis
sys.input->registerMapping(bestow::InputMapping{
    .binding = bestow::InputBinding{
        .deviceType = bestow::InputDeviceType::Gamepad,
        .axisIndex = 0,  // Left stick X
        .scale = 1.0f
    },
    .action = "move_horizontal"
});

// Register left stick vertical axis
sys.input->registerMapping(bestow::InputMapping{
    .binding = bestow::InputBinding{
        .deviceType = bestow::InputDeviceType::Gamepad,
        .axisIndex = 1,  // Left stick Y
        .scale = 1.0f
    },
    .action = "move_vertical"
});

// Register right stick for aiming
sys.input->registerMapping(bestow::InputMapping{
    .binding = bestow::InputBinding{
        .deviceType = bestow::InputDeviceType::Gamepad,
        .axisIndex = 2,  // Right stick X
        .scale = 1.0f
    },
    .action = "aim_horizontal"
});

sys.input->registerMapping(bestow::InputMapping{
    .binding = bestow::InputBinding{
        .deviceType = bestow::InputDeviceType::Gamepad,
        .axisIndex = 3,  // Right stick Y
        .scale = 1.0f
    },
    .action = "aim_vertical"
});

// In updateFixed()
float moveH = sys.input->getActionValue("move_horizontal");
float moveV = sys.input->getActionValue("move_vertical");

// Apply deadzone (ignore tiny movements)
constexpr float DEADZONE = 0.15f;
if (std::abs(moveH) < DEADZONE) moveH = 0.0f;
if (std::abs(moveV) < DEADZONE) moveV = 0.0f;

// Move character
bestow::Vec2 velocity = {moveH * 200.0f, moveV * 200.0f};
sys.physics->setVelocity(player_, velocity);

// Aim with right stick
float aimH = sys.input->getActionValue("aim_horizontal");
float aimV = sys.input->getActionValue("aim_vertical");

if (std::abs(aimH) > DEADZONE || std::abs(aimV) > DEADZONE) {
    float angle = std::atan2(aimV, aimH) * (180.0f / 3.14159f);
    auto& transform = sys.entities->get<bestow::Transform2D>(player_);
    transform.rotation = angle;
}
```

### Gamepad Axis Reference

| Axis | Description | Index |
|------|-------------|-------|
| Left Stick X | Horizontal (-1.0 left, 1.0 right) | 0 |
| Left Stick Y | Vertical (-1.0 down, 1.0 up) | 1 |
| Right Stick X | Horizontal | 2 |
| Right Stick Y | Vertical | 3 |
| Left Trigger | Analog trigger (0.0 to 1.0) | 4 |
| Right Trigger | Analog trigger (0.0 to 1.0) | 5 |

### Trigger Support

```cpp
// Register triggers
sys.input->registerMapping(bestow::InputMapping{
    .binding = bestow::InputBinding{
        .deviceType = bestow::InputDeviceType::Gamepad,
        .axisIndex = 4,  // Left trigger
        .scale = 1.0f
    },
    .action = "aim_zoom"
});

sys.input->registerMapping(bestow::InputMapping{
    .binding = bestow::InputBinding{
        .deviceType = bestow::InputDeviceType::Gamepad,
        .axisIndex = 5,  // Right trigger
        .scale = 1.0f
    },
    .action = "fire_weapon"
});

// In updateFixed()
float aimPressure = sys.input->getActionValue("aim_zoom");
if (aimPressure > 0.5f) {
    // Zoom in while aiming
    camera_.zoom = 1.5f;
}

float firePressure = sys.input->getActionValue("fire_weapon");
if (firePressure > 0.9f) {
    // Fire when trigger fully pressed
    fireWeapon();
}
```

## Step 5: Multiple Bindings Per Action

Bind the same action to multiple inputs for flexibility.

```cpp
// Bind jump to both spacebar and gamepad A button
sys.input->registerMapping(bestow::InputMapping{
    .binding = bestow::InputBinding{
        .deviceType = bestow::InputDeviceType::Keyboard,
        .keyCode = 32  // Spacebar
    },
    .action = "jump"
});

sys.input->registerMapping(bestow::InputMapping{
    .binding = bestow::InputBinding{
        .deviceType = bestow::InputDeviceType::Gamepad,
        .keyCode = 0  // A button
    },
    .action = "jump"
});

// Now sys.input->isActionActive("jump") works with EITHER input!
```

### Combining Axis and Button Inputs

```cpp
// Horizontal movement from multiple sources
sys.input->registerMapping(bestow::InputMapping{
    .binding = bestow::InputBinding{
        .deviceType = bestow::InputDeviceType::Keyboard,
        .keyCode = 'E',
        .scale = 1.0f  // Positive direction
    },
    .action = "move_horizontal"
});

sys.input->registerMapping(bestow::InputMapping{
    .binding = bestow::InputBinding{
        .deviceType = bestow::InputDeviceType::Keyboard,
        .keyCode = ',',
        .scale = -1.0f  // Negative direction
    },
    .action = "move_horizontal"
});

sys.input->registerMapping(bestow::InputMapping{
    .binding = bestow::InputBinding{
        .deviceType = bestow::InputDeviceType::Gamepad,
        .axisIndex = 0  // Left stick X
    },
    .action = "move_horizontal"
});

// getActionValue("move_horizontal") combines all sources!
```

## Step 6: Rebindable Controls

Implement a settings menu for rebinding controls.

### Save and Load Bindings

```cpp
// In Game.h
struct InputConfig {
    std::vector<bestow::InputMapping> mappings;
};

InputConfig defaultConfig_;
InputConfig currentConfig_;

// In Game.cpp - initialize()
void Game::setupDefaultControls() {
    defaultConfig_.mappings = {
        // Jump
        {bestow::InputBinding{bestow::InputDeviceType::Keyboard, 32}, "jump"},
        // Move right
        {bestow::InputBinding{bestow::InputDeviceType::Keyboard, 'E'}, "move_right"},
        // Move left
        {bestow::InputBinding{bestow::InputDeviceType::Keyboard, ','}, "move_left"},
        // etc...
    };

    // Load from save file or use defaults
    loadInputConfig();

    // Register all mappings
    for (const auto& mapping : currentConfig_.mappings) {
        engine_->systems().input->registerMapping(mapping);
    }
}

void Game::loadInputConfig() {
    // Try to load from JSON save file
    std::ifstream file("settings/input.json");
    if (file.is_open()) {
        // Parse JSON and populate currentConfig_
        // (Use nlohmann::json library)
    } else {
        // Use defaults
        currentConfig_ = defaultConfig_;
    }
}

void Game::saveInputConfig() {
    std::ofstream file("settings/input.json");
    // Serialize currentConfig_ to JSON
    // file << json;
}
```

### Rebinding Interface

```cpp
void Game::rebindAction(const std::string& actionName, bestow::InputBinding newBinding) {
    auto& sys = engine_->systems();

    // Remove old bindings for this action
    currentConfig_.mappings.erase(
        std::remove_if(currentConfig_.mappings.begin(), currentConfig_.mappings.end(),
            [&actionName](const auto& mapping) {
                return mapping.action == actionName;
            }),
        currentConfig_.mappings.end()
    );

    // Add new binding
    currentConfig_.mappings.push_back(bestow::InputMapping{newBinding, actionName});

    // Clear and re-register all mappings
    sys.input->clearMappings();
    for (const auto& mapping : currentConfig_.mappings) {
        sys.input->registerMapping(mapping);
    }

    // Save to disk
    saveInputConfig();
}

// In your settings UI
void SettingsMenu::waitForKeyPress(const std::string& actionToRebind) {
    // Display "Press any key..."
    // When key pressed:
    int keyCode = getLastKeyPressed();

    rebindAction(actionToRebind, bestow::InputBinding{
        .deviceType = bestow::InputDeviceType::Keyboard,
        .keyCode = keyCode
    });
}
```

## Step 7: Text Input

Handle text input for chat boxes, name entry, etc.

```cpp
// In Game.cpp - updateFixed()
auto& sys = engine_->systems();

// Check if text input is available
std::string textInput = sys.input->getTextInput();

if (!textInput.empty()) {
    // Append to current input field
    playerName_ += textInput;
    bestow::core::logInfo("Text input: " + textInput);
}

// Handle backspace
if (sys.input->wasActionJustPressed("backspace")) {
    if (!playerName_.empty()) {
        playerName_.pop_back();
    }
}

// Handle enter
if (sys.input->wasActionJustPressed("enter")) {
    submitName(playerName_);
    playerName_.clear();
}
```

### Input Mode (Game vs UI)

```cpp
// Toggle between gameplay and text entry
enum class InputMode {
    Gameplay,
    TextEntry
};

InputMode currentMode_ = InputMode::Gameplay;

void Game::updateFixed(bestow::DeltaTime dt) {
    auto& sys = engine_->systems();

    if (currentMode_ == InputMode::Gameplay) {
        // Process gameplay input
        if (sys.input->isActionActive("move_right")) {
            // Move player
        }
    } else if (currentMode_ == InputMode::TextEntry) {
        // Process text input only
        std::string text = sys.input->getTextInput();
        chatBuffer_ += text;
    }

    // Toggle mode with Tab
    if (sys.input->wasActionJustPressed("toggle_chat")) {
        currentMode_ = (currentMode_ == InputMode::Gameplay)
            ? InputMode::TextEntry
            : InputMode::Gameplay;
    }
}
```

## Complete Example: Dual Keyboard/Gamepad Character

Here's a complete example supporting both input methods:

```cpp
// Game.h
#pragma once

import bestow;
import bestow.core;

class Game {
public:
    bool initialize(bestow::core::Engine& engine);
    void updateFixed(bestow::DeltaTime dt);
    void render(float alpha);
    void shutdown();

private:
    void setupInputMappings();
    void handleMovement(bestow::DeltaTime dt);
    void handleActions();

    bestow::core::Engine* engine_ = nullptr;
    bestow::Entity player_;
    float moveSpeed_ = 200.0f;
};

// Game.cpp
import std;
import bestow;
import bestow.core;

#include "Game.h"

bool Game::initialize(bestow::core::Engine& engine) {
    engine_ = &engine;
    auto& sys = engine.systems();

    // Create player
    player_ = sys.entities->createEntity();
    sys.physics->createBody(player_, bestow::PhysicsBodyDef{
        .type = bestow::BodyType::Dynamic,
        .transform = {400.0f, 300.0f},
        .size = {32.0f, 32.0f},
        .fixedRotation = true
    });

    setupInputMappings();
    return true;
}

void Game::setupInputMappings() {
    auto& sys = engine_->systems();

    // Dvorak keyboard controls (,AOE layout)
    sys.input->registerMapping(bestow::InputMapping{
        .binding = {bestow::InputDeviceType::Keyboard, 'E'},
        .action = "move_right"
    });
    sys.input->registerMapping(bestow::InputMapping{
        .binding = {bestow::InputDeviceType::Keyboard, ','},
        .action = "move_left"
    });
    sys.input->registerMapping(bestow::InputMapping{
        .binding = {bestow::InputDeviceType::Keyboard, 'O'},
        .action = "move_up"
    });
    sys.input->registerMapping(bestow::InputMapping{
        .binding = {bestow::InputDeviceType::Keyboard, 'A'},
        .action = "move_down"
    });
    sys.input->registerMapping(bestow::InputMapping{
        .binding = {bestow::InputDeviceType::Keyboard, 32},  // Space
        .action = "jump"
    });

    // Gamepad controls
    sys.input->registerMapping(bestow::InputMapping{
        .binding = {
            .deviceType = bestow::InputDeviceType::Gamepad,
            .axisIndex = 0  // Left stick X
        },
        .action = "move_horizontal"
    });
    sys.input->registerMapping(bestow::InputMapping{
        .binding = {
            .deviceType = bestow::InputDeviceType::Gamepad,
            .axisIndex = 1  // Left stick Y
        },
        .action = "move_vertical"
    });
    sys.input->registerMapping(bestow::InputMapping{
        .binding = {
            .deviceType = bestow::InputDeviceType::Gamepad,
            .keyCode = 0  // A button
        },
        .action = "jump"
    });
}

void Game::updateFixed(bestow::DeltaTime dt) {
    handleMovement(dt);
    handleActions();
}

void Game::handleMovement(bestow::DeltaTime dt) {
    auto& sys = engine_->systems();

    // Combine keyboard and gamepad input
    float horizontal = 0.0f;
    float vertical = 0.0f;

    // Keyboard (buttons)
    if (sys.input->isActionActive("move_right")) horizontal += 1.0f;
    if (sys.input->isActionActive("move_left")) horizontal -= 1.0f;
    if (sys.input->isActionActive("move_up")) vertical += 1.0f;
    if (sys.input->isActionActive("move_down")) vertical -= 1.0f;

    // Gamepad (axes) - automatically combined!
    horizontal += sys.input->getActionValue("move_horizontal");
    vertical += sys.input->getActionValue("move_vertical");

    // Deadzone for gamepad
    constexpr float DEADZONE = 0.15f;
    if (std::abs(horizontal) < DEADZONE) horizontal = 0.0f;
    if (std::abs(vertical) < DEADZONE) vertical = 0.0f;

    // Clamp combined input
    horizontal = std::clamp(horizontal, -1.0f, 1.0f);
    vertical = std::clamp(vertical, -1.0f, 1.0f);

    // Normalize diagonal movement
    if (horizontal != 0.0f && vertical != 0.0f) {
        float length = std::sqrt(horizontal * horizontal + vertical * vertical);
        horizontal /= length;
        vertical /= length;
    }

    // Apply velocity
    bestow::Vec2 velocity = {
        horizontal * moveSpeed_,
        vertical * moveSpeed_
    };
    sys.physics->setVelocity(player_, velocity);
}

void Game::handleActions() {
    auto& sys = engine_->systems();

    // Jump (keyboard or gamepad)
    if (sys.input->wasActionJustPressed("jump")) {
        // Check if grounded
        auto groundCheck = sys.physics->checkGrounded(player_);
        if (groundCheck.grounded) {
            sys.physics->applyImpulse(player_, {0.0f, -500.0f});
        }
    }
}

void Game::render(float alpha) {
    auto& sys = engine_->systems();
    bestow::Vec2 pos = sys.physics->getPosition(player_);

    // Draw player
    sys.graphics->drawRect(
        {static_cast<int>(pos.x - 16), static_cast<int>(pos.y - 16), 32, 32},
        bestow::Color::green()
    );
}

void Game::shutdown() {
    bestow::core::logInfo("Shutting down");
}
```

## Best Practices

### 1. Use Action Names, Not Device-Specific Code

**DON'T:**
```cpp
if (keyboard.isKeyPressed('E')) { ... }
```

**DO:**
```cpp
if (input->isActionActive("move_right")) { ... }
```

### 2. Support Multiple Input Devices

Always provide keyboard, mouse, and gamepad options for every action.

### 3. Apply Deadzones to Analog Input

Prevent stick drift by ignoring small values:

```cpp
constexpr float DEADZONE = 0.15f;
if (std::abs(value) < DEADZONE) value = 0.0f;
```

### 4. Use wasJustPressed for One-Time Actions

Use `wasActionJustPressed()` for jumps, attacks, menu selections to avoid repeating actions every frame.

### 5. Normalize Diagonal Movement

Prevent faster diagonal movement:

```cpp
if (horizontal != 0.0f && vertical != 0.0f) {
    float length = std::sqrt(horizontal * horizontal + vertical * vertical);
    horizontal /= length;
    vertical /= length;
}
```

## Next Steps

Now you can handle all input types! Continue learning:

- **Tutorial 04: Physics Basics** - Create physics bodies, apply forces, handle collisions
- **Tutorial 05: Audio** - Add sound effects and music
- **Tutorial 06: 3D Platformer** - Expand your game into 3D

## Troubleshooting

**Input not working**
- Verify you called `.withInput()` in EngineBuilder
- Check action names match exactly (case-sensitive)
- Ensure mappings are registered before checking input

**Gamepad not detected**
- Check console logs for "Gamepad connected" message
- Try unplugging and replugging the controller
- Verify SDL2 is installed correctly

**Movement is too fast diagonally**
- Add diagonal normalization (see Best Practices #5)

**Stick drift (gamepad moves without input)**
- Apply a deadzone (see Best Practices #3)

**Can't rebind controls**
- Ensure you clear old mappings before registering new ones
- Save config to disk after rebinding
- Verify save file format is correct (use JSON)

**Text input not working**
- Check that text input mode is enabled
- Ensure you're calling `getTextInput()` in updateFixed()
- Verify keyboard focus is on the game window

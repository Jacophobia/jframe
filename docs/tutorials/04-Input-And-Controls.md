# Tutorial 4: Input and Controls

This tutorial covers Bestow's action-based input system, which supports keyboard, mouse, and gamepad input with rebindable controls.

## Input System Overview

Bestow uses an action mapping system:

- **Actions** are abstract game commands like "jump" or "fire"
- **Bindings** map physical inputs (keys, buttons) to actions
- **Devices** include keyboard, mouse, and gamepads
- **Scales** allow analog inputs and inverted axes

### Why Action Mapping?

Instead of checking raw keys in your game code:

```cpp
// Bad: Hardcoded keys
if (isKeyPressed(GLFW_KEY_SPACE)) {
    jump();
}
```

You map keys to named actions:

```cpp
// Good: Action-based
if (input->wasActionJustPressed("jump")) {
    jump();
}
```

Benefits:
- **Rebindable** - Players can customize controls
- **Multi-input** - One action can have multiple bindings (Space + Gamepad A)
- **Platform-agnostic** - Game logic doesn't care about physical keys
- **Easier to understand** - "jump" is clearer than "key 32"

## Setting Up Input Mappings

### Basic Keyboard Mapping

**Note:** Bestow uses Dvorak-friendly controls by default (,AOE instead of WASD).

```cpp
void setupInputMappings() {
    auto& input = *engine_->systems().input;

    // Jump with Space
    input.registerMapping(bestow::InputMapping{
        .binding = bestow::InputBinding{
            .deviceType = bestow::InputDeviceType::Keyboard,
            .keyCode = 32  // Space
        },
        .action = "jump"
    });

    // Move right with E (Dvorak equivalent of D)
    input.registerMapping(bestow::InputMapping{
        .binding = bestow::InputBinding{
            .deviceType = bestow::InputDeviceType::Keyboard,
            .keyCode = 69,  // E
            .scale = 1.0f
        },
        .action = "move_horizontal"
    });

    // Move left with Comma (Dvorak equivalent of A)
    input.registerMapping(bestow::InputMapping{
        .binding = bestow::InputBinding{
            .deviceType = bestow::InputDeviceType::Keyboard,
            .keyCode = 44,  // ,
            .scale = -1.0f  // Negative for left
        },
        .action = "move_horizontal"
    });
}
```

### Common Key Codes

```cpp
// Dvorak Movement Keys (,AOE)
keyCode = 44;  // , (Comma - Move left)
keyCode = 65;  // A (Move down in Dvorak)
keyCode = 79;  // O (Move up in Dvorak)
keyCode = 69;  // E (Move right)

// Letters (A-Z): 65-90

// Numbers (0-9): 48-57
keyCode = 48;  // 0
keyCode = 49;  // 1

// Special keys
keyCode = 32;   // Space
keyCode = 256;  // Escape
keyCode = 257;  // Enter
keyCode = 258;  // Tab
keyCode = 259;  // Backspace

// Arrow keys
keyCode = 262;  // Right
keyCode = 263;  // Left
keyCode = 264;  // Down
keyCode = 265;  // Up

// Function keys
keyCode = 290;  // F1
keyCode = 291;  // F2
// ... F12 = 301

// Modifiers
keyCode = 340;  // Left Shift
keyCode = 341;  // Left Control
keyCode = 342;  // Left Alt
```

## Checking Input State

### Digital Actions (Pressed/Released)

```cpp
void updatePlayer(bestow::DeltaTime dt) {
    auto& input = *engine_->systems().input;

    // Check if action is currently held down
    if (input.isActionActive("fire")) {
        // Held - fires continuously
        fireWeapon();
    }

    // Check if action was just pressed this frame
    if (input.wasActionJustPressed("jump")) {
        // Just pressed - jumps once
        jump();
    }

    // Check if action was just released this frame
    if (input.wasActionJustReleased("charge_attack")) {
        // Released - release charged attack
        releaseChargedAttack();
    }
}
```

### Analog Actions (Axes)

```cpp
void updatePlayer(bestow::DeltaTime dt) {
    auto& input = *engine_->systems().input;

    // Get analog value (-1.0 to 1.0)
    float horizontal = input.getActionValue("move_horizontal");
    float vertical = input.getActionValue("move_vertical");

    // Apply movement
    bestow::Vec2 velocity = physics->getVelocity(player_);
    velocity.x = horizontal * 200.0f;  // Move speed
    physics->setVelocity(player_, velocity);
}
```

## Multiple Bindings Per Action

Players can use keyboard OR gamepad:

```cpp
void setupInputMappings() {
    auto& input = *engine_->systems().input;

    // Jump with Space
    input.registerMapping(bestow::InputMapping{
        .binding = bestow::InputBinding{
            .deviceType = bestow::InputDeviceType::Keyboard,
            .keyCode = 32  // Space
        },
        .action = "jump"
    });

    // Jump with Gamepad A button
    input.registerMapping(bestow::InputMapping{
        .binding = bestow::InputBinding{
            .deviceType = bestow::InputDeviceType::Gamepad,
            .gamepadButton = 0  // A button (Xbox), Cross (PS)
        },
        .action = "jump"
    });

    // Now "jump" action works with BOTH Space and Gamepad A
}
```

## Gamepad Support

### Gamepad Buttons

```cpp
// Face buttons (Xbox layout)
gamepadButton = 0;  // A (Xbox), Cross (PS)
gamepadButton = 1;  // B (Xbox), Circle (PS)
gamepadButton = 2;  // X (Xbox), Square (PS)
gamepadButton = 3;  // Y (Xbox), Triangle (PS)

// Shoulder buttons
gamepadButton = 4;  // Left Bumper (LB)
gamepadButton = 5;  // Right Bumper (RB)

// Triggers (as buttons)
gamepadButton = 6;  // Left Trigger (LT)
gamepadButton = 7;  // Right Trigger (RT)

// Menu buttons
gamepadButton = 8;  // Back/Select
gamepadButton = 9;  // Start

// Stick buttons
gamepadButton = 10; // Left Stick Click (L3)
gamepadButton = 11; // Right Stick Click (R3)

// D-Pad
gamepadButton = 12; // D-Pad Up
gamepadButton = 13; // D-Pad Right
gamepadButton = 14; // D-Pad Down
gamepadButton = 15; // D-Pad Left
```

### Gamepad Axes

```cpp
// Left stick
input.registerMapping(bestow::InputMapping{
    .binding = bestow::InputBinding{
        .deviceType = bestow::InputDeviceType::Gamepad,
        .gamepadAxis = 0,  // Left Stick X
        .scale = 1.0f
    },
    .action = "move_horizontal"
});

input.registerMapping(bestow::InputMapping{
    .binding = bestow::InputBinding{
        .deviceType = bestow::InputDeviceType::Gamepad,
        .gamepadAxis = 1,  // Left Stick Y
        .scale = 1.0f
    },
    .action = "move_vertical"
});

// Right stick (camera/aim)
input.registerMapping(bestow::InputMapping{
    .binding = bestow::InputBinding{
        .deviceType = bestow::InputDeviceType::Gamepad,
        .gamepadAxis = 2,  // Right Stick X
        .scale = 1.0f
    },
    .action = "look_horizontal"
});

input.registerMapping(bestow::InputMapping{
    .binding = bestow::InputBinding{
        .deviceType = bestow::InputDeviceType::Gamepad,
        .gamepadAxis = 3,  // Right Stick Y
        .scale = -1.0f  // Inverted Y (common for cameras)
    },
    .action = "look_vertical"
});

// Triggers (as axes)
gamepadAxis = 4;  // Left Trigger (0.0 to 1.0)
gamepadAxis = 5;  // Right Trigger (0.0 to 1.0)
```

### Gamepad Detection

```cpp
// Check if any controllers are connected
int count = input.getConnectedControllerCount();
bestow::core::logInfo(std::format("Controllers connected: {}", count));

// Check specific controller
if (input.isControllerConnected(0)) {
    std::string name = input.getControllerName(0);
    bestow::core::logInfo("Controller 0: " + name);
}
```

## Mouse Input

### Mouse Buttons

```cpp
// Fire with left mouse button
input.registerMapping(bestow::InputMapping{
    .binding = bestow::InputBinding{
        .deviceType = bestow::InputDeviceType::Mouse,
        .mouseButton = 0  // Left button
    },
    .action = "fire"
});

// Mouse button codes
mouseButton = 0;  // Left
mouseButton = 1;  // Right
mouseButton = 2;  // Middle
mouseButton = 3;  // Mouse 4
mouseButton = 4;  // Mouse 5
```

### Mouse Position and Delta

```cpp
// Get mouse position (screen coordinates)
bestow::Vec2 mousePos = input.getMousePosition();
bestow::core::logInfo(std::format("Mouse: ({}, {})", mousePos.x, mousePos.y));

// Get mouse delta (movement since last frame)
bestow::Vec2 mouseDelta = input.getMouseDelta();

// Example: Camera look
cameraYaw += mouseDelta.x * sensitivity;
cameraPitch += mouseDelta.y * sensitivity;

// Example: Check if mouse button is down
if (input.isMouseButtonDown(0)) {
    // Left mouse button held
    chargeShotPower();
}
```

## Advanced Input Patterns

### Composite Actions (,AOE Movement - Dvorak-friendly)

```cpp
void setupMovementInput() {
    auto& input = *engine_->systems().input;

    // Horizontal movement (Dvorak: comma/E)
    input.registerMapping(bestow::InputMapping{
        .binding = {.deviceType = bestow::InputDeviceType::Keyboard, .keyCode = 44, .scale = -1.0f},  // ,
        .action = "move_horizontal"
    });
    input.registerMapping(bestow::InputMapping{
        .binding = {.deviceType = bestow::InputDeviceType::Keyboard, .keyCode = 69, .scale = 1.0f},   // E
        .action = "move_horizontal"
    });

    // Vertical movement (Dvorak: O/A)
    input.registerMapping(bestow::InputMapping{
        .binding = {.deviceType = bestow::InputDeviceType::Keyboard, .keyCode = 79, .scale = 1.0f},   // O (up)
        .action = "move_vertical"
    });
    input.registerMapping(bestow::InputMapping{
        .binding = {.deviceType = bestow::InputDeviceType::Keyboard, .keyCode = 65, .scale = -1.0f},  // A (down)
        .action = "move_vertical"
    });
}

void updatePlayer(bestow::DeltaTime dt) {
    // Get composite movement
    float horizontal = input.getActionValue("move_horizontal");
    float vertical = input.getActionValue("move_vertical");

    // Normalize diagonal movement
    float length = std::sqrt(horizontal * horizontal + vertical * vertical);
    if (length > 1.0f) {
        horizontal /= length;
        vertical /= length;
    }

    // Apply movement
    velocity.x = horizontal * moveSpeed;
    velocity.y = vertical * moveSpeed;
}
```

### Modifier Keys

```cpp
struct ModifiedAction {
    std::string action;
    bool requiresShift = false;
    bool requiresCtrl = false;
    bool requiresAlt = false;
};

bool isModifiedActionPressed(const ModifiedAction& action) {
    auto& input = *engine_->systems().input;

    // Check base action
    if (!input.wasActionJustPressed(action.action)) {
        return false;
    }

    // Check modifiers
    if (action.requiresShift && !input.isMouseButtonDown(340)) return false;
    if (action.requiresCtrl && !input.isMouseButtonDown(341)) return false;
    if (action.requiresAlt && !input.isMouseButtonDown(342)) return false;

    return true;
}

// Example: Quick save with Ctrl+S
if (isModifiedActionPressed({"save", .requiresCtrl = true})) {
    quickSave();
}
```

### Double Tap Detection

```cpp
struct DoubleTapDetector {
    float lastPressTime = -999.0f;
    float doubleTapWindow = 0.3f;  // 300ms window
};

bool checkDoubleTap(DoubleTapDetector& detector, bool justPressed, float currentTime) {
    if (justPressed) {
        if (currentTime - detector.lastPressTime < detector.doubleTapWindow) {
            // Double tap detected
            detector.lastPressTime = -999.0f;  // Reset
            return true;
        }
        detector.lastPressTime = currentTime;
    }
    return false;
}

// Usage
DoubleTapDetector dashDetector_;

void update(bestow::DeltaTime dt) {
    currentTime_ += dt;

    bool forwardPressed = input.wasActionJustPressed("move_forward");
    if (checkDoubleTap(dashDetector_, forwardPressed, currentTime_)) {
        // Double-tap forward to dash
        dash();
    }
}
```

### Hold to Charge

```cpp
struct ChargeAttack {
    float chargeTime = 0.0f;
    float maxCharge = 2.0f;
    bool isCharging = false;
};

void updateChargeAttack(bestow::DeltaTime dt) {
    auto& input = *engine_->systems().input;

    if (input.isActionActive("attack")) {
        // Charging
        if (!chargeAttack_.isCharging) {
            chargeAttack_.isCharging = true;
            chargeAttack_.chargeTime = 0.0f;
        }

        chargeAttack_.chargeTime = std::min(
            chargeAttack_.chargeTime + dt,
            chargeAttack_.maxCharge
        );

        // Show charge indicator
        float chargePercent = chargeAttack_.chargeTime / chargeAttack_.maxCharge;
        showChargeIndicator(chargePercent);

    } else if (chargeAttack_.isCharging) {
        // Released - fire charged attack
        chargeAttack_.isCharging = false;
        fireChargedAttack(chargeAttack_.chargeTime);
    }
}
```

## Input Rebinding

Allow players to rebind controls at runtime.

### Listening for Input

```cpp
class ControlsMenu {
public:
    void startRebinding(const std::string& actionName) {
        rebindingAction_ = actionName;
        input_->startListeningForInput();
        bestow::core::logInfo("Press a key for: " + actionName);
    }

    void update() {
        if (!input_->isListeningForInput()) return;

        auto lastInput = input_->getLastInput();
        if (lastInput) {
            // Player pressed a key/button
            bestow::core::logInfo("Rebinding to: key " +
                std::to_string(lastInput->keyCode));

            // Remove old binding
            auto mappings = input_->getMappings();
            for (const auto& mapping : mappings) {
                if (mapping.action == rebindingAction_) {
                    input_->removeMapping(mapping.binding);
                }
            }

            // Add new binding
            input_->registerMapping(bestow::InputMapping{
                .binding = *lastInput,
                .action = rebindingAction_
            });

            input_->stopListeningForInput();
            rebindingAction_.clear();
        }
    }

private:
    std::string rebindingAction_;
    bestow::IInputSystem* input_;
};
```

### Saving and Loading Bindings

```cpp
void saveBindings(const std::string& filename) {
    auto mappings = input_->getMappings();

    nlohmann::json j;
    for (const auto& mapping : mappings) {
        j[mapping.action].push_back({
            {"deviceType", static_cast<int>(mapping.binding.deviceType)},
            {"keyCode", mapping.binding.keyCode},
            {"scale", mapping.binding.scale}
        });
    }

    std::ofstream file(filename);
    file << j.dump(4);
}

void loadBindings(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) return;

    nlohmann::json j;
    file >> j;

    input_->clearMappings();

    for (auto& [action, bindings] : j.items()) {
        for (auto& binding : bindings) {
            input_->registerMapping(bestow::InputMapping{
                .binding = bestow::InputBinding{
                    .deviceType = static_cast<bestow::InputDeviceType>(
                        binding["deviceType"].get<int>()),
                    .keyCode = binding["keyCode"].get<int>(),
                    .scale = binding["scale"].get<float>()
                },
                .action = action
            });
        }
    }
}
```

## Lua-Based Input Configuration

Define input in Lua for easy editing:

```lua
-- data/config/input.lua
-- Using Dvorak-friendly ,AOE layout for movement
return {
    keyboard = {
        -- Movement (Dvorak: ,AOE)
        {action = "move_horizontal", key = ",", scale = -1.0},  -- Comma for left
        {action = "move_horizontal", key = "E", scale = 1.0},   -- E for right
        {action = "move_vertical", key = "O", scale = 1.0},     -- O for up
        {action = "move_vertical", key = "A", scale = -1.0},    -- A for down

        -- Actions
        {action = "jump", key = "Space"},
        {action = "attack", key = "J"},
        {action = "interact", key = "E"},
        {action = "pause", key = "Escape"}
    },

    gamepad = {
        -- Movement
        {action = "move_horizontal", axis = 0},  -- Left stick X
        {action = "move_vertical", axis = 1},    -- Left stick Y

        -- Actions
        {action = "jump", button = 0},      -- A
        {action = "attack", button = 2},    -- X
        {action = "interact", button = 1},  -- B
        {action = "pause", button = 9}      -- Start
    }
}
```

Load in C++:

```cpp
void loadInputConfigFromLua(const std::string& path) {
    sol::state lua;
    lua.open_libraries(sol::lib::base);

    auto result = lua.script_file(path);
    if (!result.valid()) {
        bestow::core::logError("Failed to load input config");
        return;
    }

    sol::table config = result;

    // Load keyboard mappings
    sol::table keyboard = config["keyboard"];
    for (size_t i = 1; i <= keyboard.size(); ++i) {
        sol::table mapping = keyboard[i];

        std::string action = mapping["action"];
        std::string keyStr = mapping["key"];
        float scale = mapping["scale"].get_or(1.0f);

        int keyCode = getKeyCode(keyStr);  // Convert "A" -> 65

        input_->registerMapping(bestow::InputMapping{
            .binding = {
                .deviceType = bestow::InputDeviceType::Keyboard,
                .keyCode = keyCode,
                .scale = scale
            },
            .action = action
        });
    }

    // Load gamepad mappings (similar)
}
```

## Common Input Patterns

### Platformer Controls

```cpp
void setupPlatformerInput() {
    auto& input = *engine_->systems().input;

    // Horizontal movement (Dvorak: comma/E, Arrow keys, Left Stick)
    input.registerMapping({.binding = {.deviceType = Keyboard, .keyCode = 44, .scale = -1.0f}, .action = "move"});   // ,
    input.registerMapping({.binding = {.deviceType = Keyboard, .keyCode = 69, .scale = 1.0f}, .action = "move"});    // E
    input.registerMapping({.binding = {.deviceType = Keyboard, .keyCode = 263, .scale = -1.0f}, .action = "move"});  // Left Arrow
    input.registerMapping({.binding = {.deviceType = Keyboard, .keyCode = 262, .scale = 1.0f}, .action = "move"});   // Right Arrow
    input.registerMapping({.binding = {.deviceType = Gamepad, .gamepadAxis = 0}, .action = "move"});

    // Jump (Space, O, A button)
    input.registerMapping({.binding = {.deviceType = Keyboard, .keyCode = 32}, .action = "jump"});   // Space
    input.registerMapping({.binding = {.deviceType = Keyboard, .keyCode = 79}, .action = "jump"});   // O (Dvorak up)
    input.registerMapping({.binding = {.deviceType = Gamepad, .gamepadButton = 0}, .action = "jump"});

    // Attack
    input.registerMapping({.binding = {.deviceType = Keyboard, .keyCode = 74}, .action = "attack"});  // J
    input.registerMapping({.binding = {.deviceType = Gamepad, .gamepadButton = 2}, .action = "attack"});  // X
}
```

### Top-Down Controls

```cpp
void setupTopDownInput() {
    // ,AOE (Dvorak-friendly) + Arrow keys for 8-directional movement
    input.registerMapping({.binding = {.deviceType = Keyboard, .keyCode = 44, .scale = -1.0f}, .action = "move_x"});  // ,
    input.registerMapping({.binding = {.deviceType = Keyboard, .keyCode = 69, .scale = 1.0f}, .action = "move_x"});   // E
    input.registerMapping({.binding = {.deviceType = Keyboard, .keyCode = 79, .scale = 1.0f}, .action = "move_y"});   // O (up)
    input.registerMapping({.binding = {.deviceType = Keyboard, .keyCode = 65, .scale = -1.0f}, .action = "move_y"});  // A (down)

    // Gamepad
    input.registerMapping({.binding = {.deviceType = Gamepad, .gamepadAxis = 0}, .action = "move_x"});
    input.registerMapping({.binding = {.deviceType = Gamepad, .gamepadAxis = 1}, .action = "move_y"});
}
```

## Troubleshooting

**Input not responding**
- Check that you called `withInput()` in EngineBuilder
- Verify action names match exactly (case-sensitive)
- Make sure `input->update()` is called each frame (automatic in Engine)

**Gamepad not detected**
- Check `getConnectedControllerCount()`
- Try unplugging and replugging the controller
- Verify controller works in other applications

**Multiple actions triggering**
- Check for duplicate bindings on the same key
- Use `removeMapping()` before adding new bindings

**Analog stick drift**
- Add a dead zone to ignore small values:
  ```cpp
  float value = input.getActionValue("move");
  if (std::abs(value) < 0.1f) value = 0.0f;  // Dead zone
  ```

## Next Steps

Now you understand input handling! Next tutorial:

- **Tutorial 5: Audio** - Adding sounds and music to your game

## Further Reading

- Bestow Input System API: `docs/systems/input.md`
- GLFW Key Codes: https://www.glfw.org/docs/latest/group__keys.html
- Example: `examples/platformer/src/Game.cpp`

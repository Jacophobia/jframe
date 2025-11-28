# InputSystem API

The `InputSystem` provides action-based input mapping for keyboard, mouse, and game controllers.

## Overview

```cpp
auto& input = sys.input;

// Map inputs to actions
input->registerMapping({
    .binding = {
        .deviceType = InputDeviceType::Keyboard,
        .keyCode = GLFW_KEY_SPACE
    },
    .action = "Jump"
});

// Query action state
if (input->wasActionJustPressed("Jump")) {
    physics->applyImpulse(player, Vec2{0, -500});
}

// Get analog input
float move = input->getActionValue("MoveHorizontal");
```

## Lifecycle

### update()

```cpp
void update();
```

Updates input state. Call once per frame before checking input.

**Example:**

```cpp
void run() {
    while (!graphics->shouldClose()) {
        input->update();  // Poll input

        // Check input state
        if (input->wasActionJustPressed("Quit")) {
            break;
        }

        // Game logic...
    }
}
```

---

## Input Mapping

### registerMapping(const InputMapping& mapping)

```cpp
void registerMapping(const InputMapping& mapping);
```

Registers an input binding to an action.

**InputMapping Structure:**

```cpp
struct InputMapping {
    InputBinding binding;
    Action action;  // std::string
};

struct InputBinding {
    InputDeviceType deviceType = InputDeviceType::Keyboard;
    int deviceIndex = 0;
    int keyCode = 0;
    float scale = 1.0f;      // For axes (e.g., -1 for inverted)
    float deadzone = 0.1f;   // For analog sticks
};

enum class InputDeviceType : uint8_t {
    Keyboard,
    Mouse,
    Controller
};
```

**Example:**

```cpp
// Keyboard - Jump
input->registerMapping({
    .binding = {
        .deviceType = InputDeviceType::Keyboard,
        .keyCode = GLFW_KEY_SPACE
    },
    .action = "Jump"
});

// Keyboard - Move Right
input->registerMapping({
    .binding = {
        .deviceType = InputDeviceType::Keyboard,
        .keyCode = GLFW_KEY_D,
        .scale = 1.0f
    },
    .action = "MoveHorizontal"
});

// Keyboard - Move Left (negative)
input->registerMapping({
    .binding = {
        .deviceType = InputDeviceType::Keyboard,
        .keyCode = GLFW_KEY_A,
        .scale = -1.0f
    },
    .action = "MoveHorizontal"
});

// Controller - Jump
input->registerMapping({
    .binding = {
        .deviceType = InputDeviceType::Controller,
        .deviceIndex = 0,
        .keyCode = GLFW_GAMEPAD_BUTTON_A
    },
    .action = "Jump"
});

// Controller - Move (left stick X-axis)
input->registerMapping({
    .binding = {
        .deviceType = InputDeviceType::Controller,
        .deviceIndex = 0,
        .keyCode = GLFW_GAMEPAD_AXIS_LEFT_X,
        .deadzone = 0.15f
    },
    .action = "MoveHorizontal"
});
```

---

### removeMapping(const InputBinding& binding)

```cpp
void removeMapping(const InputBinding& binding);
```

Removes a specific input binding.

---

### clearMappings()

```cpp
void clearMappings();
```

Removes all input mappings.

---

### getMappings()

```cpp
std::vector<InputMapping> getMappings() const;
```

Returns all registered input mappings.

**Use for:** Displaying current controls in UI

---

## Action State Queries

### getActionState(const Action& action)

```cpp
ActionState getActionState(const Action& action) const;
```

Returns the full state of an action.

**ActionState Structure:**

```cpp
struct ActionState {
    Action action;
    bool active = false;        // Currently active
    float value = 0.0f;         // Analog value (-1.0 to 1.0)
    bool justPressed = false;   // Pressed this frame
    bool justReleased = false;  // Released this frame
};
```

---

### isActionActive(const Action& action)

```cpp
bool isActionActive(const Action& action) const;
```

Returns `true` if the action is currently active (held down).

**Example:**

```cpp
if (input->isActionActive("Fire")) {
    // Continuous firing
    fireWeapon();
}
```

---

### wasActionJustPressed(const Action& action)

```cpp
bool wasActionJustPressed(const Action& action) const;
```

Returns `true` if the action was pressed **this frame**.

**Example:**

```cpp
if (input->wasActionJustPressed("Jump")) {
    // Jump on button press
    physics->applyImpulse(player, Vec2{0, -500});
}
```

---

### wasActionJustReleased(const Action& action)

```cpp
bool wasActionJustReleased(const Action& action) const;
```

Returns `true` if the action was released **this frame**.

**Example:**

```cpp
if (input->wasActionJustReleased("ChargeShot")) {
    // Fire charged shot on release
    fireChargedShot(chargeAmount);
}
```

---

### getActionValue(const Action& action)

```cpp
float getActionValue(const Action& action) const;
```

Returns the analog value of an action (-1.0 to 1.0).

**For digital inputs:** Returns 0.0 or 1.0 (or -1.0 if scale is negative)

**For analog inputs:** Returns the axis value with deadzone applied

**Example:**

```cpp
// Horizontal movement
float moveInput = input->getActionValue("MoveHorizontal");
Vec2 vel = physics->getVelocity(player);
vel.x = moveInput * 200.0f;  // Move speed
physics->setVelocity(player, vel);

// Analog aiming
float aimX = input->getActionValue("AimX");
float aimY = input->getActionValue("AimY");
Vec2 aimDir{aimX, aimY};
if (glm::length(aimDir) > 0.1f) {
    aimAngle = std::atan2(aimY, aimX);
}
```

---

### getAllActionStates()

```cpp
std::vector<ActionState> getAllActionStates() const;
```

Returns the state of all registered actions.

**Use for:** Debug UI showing all input states

---

## Raw Input (for Rebinding UI)

### getLastInput()

```cpp
std::optional<InputBinding> getLastInput() const;
```

Returns the last input that was pressed, or `std::nullopt` if none.

**Use for:** Detecting input for key rebinding

---

### startListeningForInput()

```cpp
void startListeningForInput();
```

Begins listening for any input (for rebinding).

---

### stopListeningForInput()

```cpp
void stopListeningForInput();
```

Stops listening for input.

---

### isListeningForInput()

```cpp
bool isListeningForInput() const;
```

Returns `true` if currently listening for input.

**Example:**

```cpp
void rebindControl(const std::string& action) {
    input->startListeningForInput();

    while (input->isListeningForInput()) {
        input->update();

        if (auto binding = input->getLastInput()) {
            // Got new binding
            input->removeMapping(*binding);  // Remove old
            input->registerMapping({
                .binding = *binding,
                .action = action
            });
            input->stopListeningForInput();
            break;
        }

        if (input->wasActionJustPressed("Cancel")) {
            input->stopListeningForInput();
            break;
        }
    }
}
```

---

## Mouse Input

### getMousePosition()

```cpp
Vec2 getMousePosition() const;
```

Returns the mouse position in screen coordinates.

**Example:**

```cpp
Vec2 mouseScreen = input->getMousePosition();
Vec2 mouseWorld = graphics->screenToWorld(mouseScreen);

if (input->isMouseButtonDown(GLFW_MOUSE_BUTTON_LEFT)) {
    // Click to spawn entity
    Entity clickMarker = entities->createEntity();
    entities->emplace<Transform2D>(clickMarker,
        mouseWorld.x, mouseWorld.y
    );
}
```

---

### getMouseDelta()

```cpp
Vec2 getMouseDelta() const;
```

Returns the mouse movement since the last frame.

**Example:**

```cpp
Vec2 delta = input->getMouseDelta();
cameraAngle += delta.x * 0.01f;
```

---

### isMouseButtonDown(int button)

```cpp
bool isMouseButtonDown(int button) const;
```

Returns `true` if the specified mouse button is held down.

**Mouse Buttons:**
- `GLFW_MOUSE_BUTTON_LEFT` (0)
- `GLFW_MOUSE_BUTTON_RIGHT` (1)
- `GLFW_MOUSE_BUTTON_MIDDLE` (2)

---

## Controller Input

### getConnectedControllerCount()

```cpp
int getConnectedControllerCount() const;
```

Returns the number of connected game controllers.

---

### isControllerConnected(int index)

```cpp
bool isControllerConnected(int index) const;
```

Returns `true` if the controller at the specified index is connected.

---

### getControllerName(int index)

```cpp
std::string getControllerName(int index) const;
```

Returns the name of the controller (e.g., "Xbox 360 Controller").

---

## Common Key Codes

**Keyboard:**
- `GLFW_KEY_SPACE`
- `GLFW_KEY_ENTER`
- `GLFW_KEY_ESCAPE`
- `GLFW_KEY_W`, `GLFW_KEY_A`, `GLFW_KEY_S`, `GLFW_KEY_D`
- `GLFW_KEY_UP`, `GLFW_KEY_DOWN`, `GLFW_KEY_LEFT`, `GLFW_KEY_RIGHT`
- `GLFW_KEY_LEFT_SHIFT`, `GLFW_KEY_LEFT_CONTROL`

**Controller Buttons:**
- `GLFW_GAMEPAD_BUTTON_A` (0)
- `GLFW_GAMEPAD_BUTTON_B` (1)
- `GLFW_GAMEPAD_BUTTON_X` (2)
- `GLFW_GAMEPAD_BUTTON_Y` (3)
- `GLFW_GAMEPAD_BUTTON_LEFT_BUMPER` (4)
- `GLFW_GAMEPAD_BUTTON_RIGHT_BUMPER` (5)
- `GLFW_GAMEPAD_BUTTON_START` (7)
- `GLFW_GAMEPAD_BUTTON_DPAD_UP` (10)
- `GLFW_GAMEPAD_BUTTON_DPAD_DOWN` (12)

**Controller Axes:**
- `GLFW_GAMEPAD_AXIS_LEFT_X` (0)
- `GLFW_GAMEPAD_AXIS_LEFT_Y` (1)
- `GLFW_GAMEPAD_AXIS_RIGHT_X` (2)
- `GLFW_GAMEPAD_AXIS_RIGHT_Y` (3)
- `GLFW_GAMEPAD_AXIS_LEFT_TRIGGER` (4)
- `GLFW_GAMEPAD_AXIS_RIGHT_TRIGGER` (5)

See [GLFW documentation](https://www.glfw.org/docs/latest/group__keys.html) for full list.

---

## Common Patterns

### Setting Up Standard Controls

```cpp
void setupControls(IInputSystem* input) {
    // Movement - Keyboard
    input->registerMapping({
        {InputDeviceType::Keyboard, 0, GLFW_KEY_D, 1.0f},
        "MoveHorizontal"
    });
    input->registerMapping({
        {InputDeviceType::Keyboard, 0, GLFW_KEY_A, -1.0f},
        "MoveHorizontal"
    });

    // Movement - Controller
    input->registerMapping({
        {InputDeviceType::Controller, 0, GLFW_GAMEPAD_AXIS_LEFT_X, 1.0f, 0.15f},
        "MoveHorizontal"
    });

    // Jump
    input->registerMapping({
        {InputDeviceType::Keyboard, 0, GLFW_KEY_SPACE},
        "Jump"
    });
    input->registerMapping({
        {InputDeviceType::Controller, 0, GLFW_GAMEPAD_BUTTON_A},
        "Jump"
    });

    // Attack
    input->registerMapping({
        {InputDeviceType::Keyboard, 0, GLFW_KEY_J},
        "Attack"
    });
    input->registerMapping({
        {InputDeviceType::Controller, 0, GLFW_GAMEPAD_BUTTON_X},
        "Attack"
    });
}
```

---

### Platformer Movement

```cpp
void updatePlayerInput(DeltaTime dt) {
    // Horizontal movement
    float moveInput = input->getActionValue("MoveHorizontal");
    Vec2 vel = physics->getVelocity(player);
    vel.x = moveInput * 200.0f;

    // Jump (only if grounded)
    auto ground = physics->checkGrounded(player);
    if (ground.grounded && input->wasActionJustPressed("Jump")) {
        vel.y = -500.0f;
    }

    physics->setVelocity(player, vel);

    // Attack
    if (input->wasActionJustPressed("Attack")) {
        performAttack();
    }
}
```

---

### Mouse Aiming

```cpp
void updateMouseAim() {
    Vec2 playerPos = physics->getPosition(player);
    Vec2 mouseScreen = input->getMousePosition();
    Vec2 mouseWorld = graphics->screenToWorld(mouseScreen);

    // Aim direction
    Vec2 aimDir = mouseWorld - playerPos;
    float aimAngle = std::atan2(aimDir.y, aimDir.x);

    // Update player rotation
    auto& transform = entities->get<Transform2D>(player);
    transform.rotation = aimAngle;

    // Fire on click
    if (input->isMouseButtonDown(GLFW_MOUSE_BUTTON_LEFT)) {
        fireProjectile(playerPos, aimDir);
    }
}
```

---

### Charge Shot

```cpp
float chargeTime = 0.0f;
const float maxCharge = 2.0f;

void updateChargeShotInput(DeltaTime dt) {
    if (input->isActionActive("ChargeShot")) {
        // Charging
        chargeTime = std::min(chargeTime + dt, maxCharge);
    }

    if (input->wasActionJustReleased("ChargeShot")) {
        // Fire with charge
        float power = chargeTime / maxCharge;
        fireChargedShot(power);
        chargeTime = 0.0f;
    }
}
```

---

### Action Buffer (Fighting Game Input)

```cpp
struct InputBuffer {
    std::deque<std::pair<std::string, float>> history;
    const float bufferTime = 0.2f;  // 200ms buffer

    void update(DeltaTime dt, IInputSystem* input) {
        // Age out old inputs
        for (auto& [action, time] : history) {
            time += dt;
        }
        history.erase(
            std::remove_if(history.begin(), history.end(),
                [this](auto& p) { return p.second > bufferTime; }),
            history.end()
        );

        // Add new inputs
        if (input->wasActionJustPressed("Attack")) {
            history.push_back({"Attack", 0.0f});
        }
        if (input->wasActionJustPressed("Special")) {
            history.push_back({"Special", 0.0f});
        }
    }

    bool matchSequence(const std::vector<std::string>& sequence) {
        if (history.size() < sequence.size()) return false;

        for (size_t i = 0; i < sequence.size(); ++i) {
            if (history[i].first != sequence[i]) {
                return false;
            }
        }
        return true;
    }
};

// Usage
if (inputBuffer.matchSequence({"Attack", "Attack", "Special"})) {
    performComboMove();
}
```

---

## Performance Tips

1. **Call `update()` once per frame** - Don't poll multiple times
2. **Use actions, not raw keys** - Easier to rebind and support multiple devices
3. **Cache action values** - Don't call `getActionValue()` multiple times in a frame
4. **Use `wasJustPressed` for single events** - Don't trigger repeatedly
5. **Set appropriate deadzones** - Prevent stick drift on controllers

## See Also

- [PhysicsSystem](PhysicsSystem.md) - Applying input to physics
- [GraphicsSystem](GraphicsSystem.md) - Mouse position for UI
- [EventSystem](EventSystem.md) - Publishing input events

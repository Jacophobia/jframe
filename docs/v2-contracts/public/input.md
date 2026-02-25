# Input System

> **Visibility:** Public (Lua + C++ API)
> **Tier:** 2
> **Dependencies:** Types, Events
> **Lua Paths:** `bestow.input` (high-level), `bestow.input.core` (low-level)

## Purpose

The Input System provides a phase-based, action-driven input abstraction layer. The high-level API exposes phase management, action registration, basic polling, and cursor control for common game input patterns. The low-level API adds the full phase stack, raw input capture for rebinding UIs, complete keyboard/mouse/gamepad state queries, modifier key detection, text input, deadzone configuration, and haptic feedback. All input flows through a phase stack that determines which action mappings are active, enabling clean separation of menu, gameplay, and dialogue input contexts.

## High-Level API: `IInputSystem`

The simplified API for common game development tasks. 8-15 methods, phase-based action registration with sensible defaults.

### Phase Management

| Method | Returns | Description |
|--------|---------|-------------|
| `pushPhase(std::string_view phase)` | `void` | Push a new input phase onto the stack, making it the active phase |
| `popPhase()` | `void` | Pop the current phase from the stack, restoring the previous phase |
| `changePhase(std::string_view phase)` | `void` | Replace the entire phase stack with a single new phase |
| `getCurrentPhase()` | `std::string` | Return the name of the currently active input phase |

### Action Registration

| Method | Returns | Description |
|--------|---------|-------------|
| `registerAction(const ActionRegistration& reg)` | `void` | Register an input action with its bindings, conditions, and effects |
| `unregisterAction(std::string_view name)` | `void` | Remove a previously registered action by name |

### Input Polling

| Method | Returns | Description |
|--------|---------|-------------|
| `isKeyDown(KeyCode key)` | `bool` | Check whether a keyboard key is currently held down |
| `wasKeyJustPressed(KeyCode key)` | `bool` | Check whether a keyboard key was pressed this frame |
| `getMousePosition()` | `Vec2` | Return the current mouse position in screen coordinates |
| `getMouseDelta()` | `Vec2` | Return the mouse movement delta since the last frame |
| `getLeftStick(int index = 0)` | `Vec2` | Return the left analog stick position as a normalized 2D vector |
| `getRightStick(int index = 0)` | `Vec2` | Return the right analog stick position as a normalized 2D vector |

### Cursor

| Method | Returns | Description |
|--------|---------|-------------|
| `setCursorMode(CursorMode mode)` | `void` | Set the cursor visibility and lock mode |

### Configuration

| Method | Returns | Description |
|--------|---------|-------------|
| `loadConfig(std::string_view path)` | `Result<void>` | Load input action mappings from a Lua configuration file |

## Low-Level API: `IInputCore`

Full control API. Every configuration knob exposed. Includes lifecycle management, the complete phase stack, all action management, raw input capture, full keyboard/mouse/gamepad state, modifier keys, text input, deadzone tuning, and haptic feedback.

### Lifecycle

| Method | Returns | Description |
|--------|---------|-------------|
| `initialize(void* nativeWindow)` | `Result<void>` | Initialize the input system with a platform window handle |
| `shutdown()` | `void` | Release all input resources and reset state |
| `update()` | `void` | Poll input devices and update all state; called once per frame in EarlyUpdate |

### Phase Management

| Method | Returns | Description |
|--------|---------|-------------|
| `getCurrentPhase()` | `std::string` | Return the name of the currently active input phase |
| `getPhaseStack()` | `std::vector<std::string>` | Return the complete phase stack from bottom to top |
| `pushPhase(std::string_view phase)` | `void` | Push a new input phase onto the stack |
| `popPhase()` | `void` | Pop the current phase, restoring the previous one |
| `changePhase(std::string_view phase)` | `void` | Replace the entire phase stack with a single new phase |
| `isPhaseActive(std::string_view phase)` | `bool` | Check whether a phase is anywhere in the current stack |

### Action Registration

| Method | Returns | Description |
|--------|---------|-------------|
| `registerAction(const ActionRegistration& reg)` | `void` | Register a complete action with bindings, conditions, and effects |
| `unregisterAction(std::string_view name)` | `void` | Remove a named action from the registry |
| `unregisterPhaseActions(std::string_view phase)` | `void` | Remove all actions associated with a specific phase |
| `clearActions()` | `void` | Remove all registered actions across all phases |
| `getActions()` | `std::vector<ActionRegistration>` | Return all currently registered action definitions |

### Input State Queries

| Method | Returns | Description |
|--------|---------|-------------|
| `getInputState(const InputBinding& binding)` | `InputState` | Return the full state of a specific input binding this frame |
| `getHoldDuration(const InputBinding& binding)` | `float` | Return how long an input has been continuously held in seconds |
| `setDefaultHoldThreshold(float seconds)` | `void` | Set the duration after which a held input transitions to the Held state |
| `getDefaultHoldThreshold()` | `float` | Return the current default hold threshold in seconds |

### Configuration

| Method | Returns | Description |
|--------|---------|-------------|
| `loadConfig(std::string_view path)` | `Result<void>` | Load input configuration from a Lua file |
| `reloadConfig()` | `Result<void>` | Reload the most recently loaded configuration file |

### Raw Input Capture (Rebinding UI)

| Method | Returns | Description |
|--------|---------|-------------|
| `getLastInput()` | `std::optional<InputBinding>` | Return the last raw input detected while listening, or nullopt |
| `isListeningForInput()` | `bool` | Check whether the system is currently capturing raw input for rebinding |
| `startListeningForInput()` | `void` | Begin capturing the next raw input for key/button rebinding |
| `stopListeningForInput()` | `void` | Stop capturing raw input and return to normal operation |

### Keyboard

| Method | Returns | Description |
|--------|---------|-------------|
| `isKeyDown(KeyCode key)` | `bool` | Check whether a keyboard key is currently held down |
| `wasKeyJustPressed(KeyCode key)` | `bool` | Check whether a key transitioned from up to down this frame |
| `wasKeyJustReleased(KeyCode key)` | `bool` | Check whether a key transitioned from down to up this frame |

### Mouse

| Method | Returns | Description |
|--------|---------|-------------|
| `getMousePosition()` | `Vec2` | Return the current mouse cursor position in screen coordinates |
| `getMouseDelta()` | `Vec2` | Return the mouse movement delta since the previous frame |
| `isMouseButtonDown(MouseButton button)` | `bool` | Check whether a mouse button is currently held down |
| `wasMouseButtonJustPressed(MouseButton button)` | `bool` | Check whether a mouse button was pressed this frame |
| `wasMouseButtonJustReleased(MouseButton button)` | `bool` | Check whether a mouse button was released this frame |
| `getScrollDelta()` | `Vec2` | Return the scroll wheel delta since the previous frame (x = horizontal, y = vertical) |

### Modifier Keys

| Method | Returns | Description |
|--------|---------|-------------|
| `getModifierState()` | `ModifierKey` | Return a bitmask of all currently active modifier keys |
| `isShiftPressed()` | `bool` | Check whether either Shift key is currently held |
| `isCtrlPressed()` | `bool` | Check whether either Ctrl key is currently held |
| `isAltPressed()` | `bool` | Check whether either Alt key is currently held |
| `isSuperPressed()` | `bool` | Check whether the Super/Windows/Command key is currently held |

### Gamepad

| Method | Returns | Description |
|--------|---------|-------------|
| `isGamepadButtonDown(GamepadButton btn, int index = 0)` | `bool` | Check whether a gamepad button is currently held |
| `wasGamepadButtonJustPressed(GamepadButton btn, int index = 0)` | `bool` | Check whether a gamepad button was pressed this frame |
| `wasGamepadButtonJustReleased(GamepadButton btn, int index = 0)` | `bool` | Check whether a gamepad button was released this frame |
| `getGamepadAxisValue(GamepadAxis axis, int index = 0)` | `float` | Return the raw axis value for a gamepad axis (-1.0 to 1.0) |
| `getLeftStick(int index = 0)` | `Vec2` | Return the left stick position as a normalized 2D vector |
| `getRightStick(int index = 0)` | `Vec2` | Return the right stick position as a normalized 2D vector |

### Controller Info

| Method | Returns | Description |
|--------|---------|-------------|
| `getConnectedControllerCount()` | `int` | Return the number of currently connected game controllers |
| `isControllerConnected(int index)` | `bool` | Check whether a controller at the given index is connected |
| `getControllerName(int index)` | `std::string` | Return the human-readable name of a connected controller |

### Text Input

| Method | Returns | Description |
|--------|---------|-------------|
| `enableTextInput()` | `void` | Enable text input mode (for chat, name entry, etc.) |
| `disableTextInput()` | `void` | Disable text input mode and return to normal key handling |
| `isTextInputEnabled()` | `bool` | Check whether text input mode is currently active |
| `getTextInput()` | `std::string` | Return the accumulated text input since the last clear |
| `clearTextInput()` | `void` | Clear the accumulated text input buffer |

### Cursor Control

| Method | Returns | Description |
|--------|---------|-------------|
| `showMouseCursor()` | `void` | Make the mouse cursor visible |
| `hideMouseCursor()` | `void` | Hide the mouse cursor |
| `isMouseCursorVisible()` | `bool` | Check whether the mouse cursor is currently visible |
| `setCursorMode(CursorMode mode)` | `void` | Set the cursor visibility and confinement mode |
| `getCursorMode()` | `CursorMode` | Return the current cursor mode |

### Deadzone Configuration

| Method | Returns | Description |
|--------|---------|-------------|
| `setStickDeadzone(float deadzone)` | `void` | Set the deadzone radius for analog sticks (0.0 to 1.0) |
| `getStickDeadzone()` | `float` | Return the current stick deadzone radius |
| `setTriggerDeadzone(float deadzone)` | `void` | Set the deadzone threshold for analog triggers (0.0 to 1.0) |
| `getTriggerDeadzone()` | `float` | Return the current trigger deadzone threshold |

### Haptics

| Method | Returns | Description |
|--------|---------|-------------|
| `setGamepadVibration(int index, float leftMotor, float rightMotor, float duration = 0)` | `Result<void>` | Set gamepad vibration intensity for both motors; duration 0 = until stopped |
| `stopGamepadVibration(int index)` | `void` | Immediately stop all vibration on the specified gamepad |

## Types

### ActionRegistration

Complete action registration from the builder API.

| Field | Type | Default | Description |
|-------|------|---------|-------------|
| `phase` | `std::string` | `""` | The input phase during which this action is active |
| `conditions` | `std::vector<ActionCondition>` | `{}` | All conditions that must be true (AND logic) for the action to fire |
| `effects` | `std::vector<ActionEffect>` | `{}` | Effects to execute when all conditions are met |
| `terminal` | `ActionTerminal` | `Discrete` | Whether the action fires once on transition or continuously while held |
| `deadzone` | `float` | `0.0f` | Deadzone threshold for axis inputs in this action |
| `valid` | `bool` | `false` | Whether the builder validated this registration successfully |
| `validationError` | `std::string` | `""` | Error message if validation failed |

### ActionCondition

A single condition within an action registration.

| Field | Type | Default | Description |
|-------|------|---------|-------------|
| `type` | `ActionConditionType` | `WhenPressed` | The condition type that must be satisfied |
| `input` | `InputBinding` | `{}` | The input binding to evaluate |
| `holdThreshold` | `std::optional<float>` | `nullopt` | Override hold duration for WhenHeld conditions; nullopt uses the system default |

### ActionEffect

A single effect triggered by an action.

| Field | Type | Default | Description |
|-------|------|---------|-------------|
| `type` | `ActionEffectType` | `EmitAction` | The type of effect to execute |
| `value` | `std::string` | `""` | Action name for EmitAction, or phase name for phase effects |

### InputBinding

Represents a specific input that can be bound to an action.

| Field | Type | Default | Description |
|-------|------|---------|-------------|
| `source` | `InputSource` | `Keyboard` | The device type this binding comes from |
| `deviceIndex` | `int` | `0` | Gamepad index (0-3) or 0 for keyboard/mouse |
| `input` | `std::variant<KeyCode, MouseButton, GamepadButton, GamepadAxis>` | `KeyCode::Unknown` | The actual input to evaluate |
| `requiredModifiers` | `ModifierKey` | `None` | Modifier keys that must be held for this binding to activate |
| `scale` | `float` | `1.0f` | Value multiplier for axis inversion or scaling |
| `deadzone` | `float` | `0.1f` | Deadzone threshold for axis inputs |

### InputState

```cpp
enum class InputState : std::uint8_t {
    NotPressed,    // Input is up, has been for multiple frames
    JustPressed,   // Input went down this frame
    Pressed,       // Input is down, under hold threshold
    Held,          // Input is down, exceeded hold threshold
    JustReleased   // Input went up this frame
};
```

| Value | Description |
|-------|-------------|
| `NotPressed` | The input is not active and has been up for at least one frame |
| `JustPressed` | The input transitioned from up to down this frame |
| `Pressed` | The input is held down but has not yet exceeded the hold threshold |
| `Held` | The input has been held down longer than the hold threshold |
| `JustReleased` | The input transitioned from down to up this frame |

### KeyCode

Platform-agnostic keyboard key codes abstracting GLFW/SDL constants.

```cpp
enum class KeyCode : std::uint16_t {
    Unknown = 0,
    A = 65, B, C, D, E, F, G, H, I, J, K, L, M,
    N, O, P, Q, R, S, T, U, V, W, X, Y, Z,
    Num0 = 48, Num1, Num2, Num3, Num4, Num5, Num6, Num7, Num8, Num9,
    F1 = 290, F2, F3, F4, F5, F6, F7, F8, F9, F10, F11, F12,
    Space = 32, Apostrophe = 39, Comma = 44, Minus = 45,
    Period = 46, Slash = 47, Semicolon = 59, Equal = 61,
    LeftBracket = 91, Backslash = 92, RightBracket = 93, GraveAccent = 96,
    Escape = 256, Enter, Tab, Backspace, Insert, Delete,
    Right, Left, Down, Up,
    PageUp, PageDown, Home, End,
    CapsLock = 280, ScrollLock, NumLock, PrintScreen, Pause,
    LeftShift = 340, LeftControl, LeftAlt, LeftSuper,
    RightShift, RightControl, RightAlt, RightSuper, Menu
    // ... additional keys
};
```

### MouseButton

```cpp
enum class MouseButton : std::uint8_t {
    Left = 0,
    Right = 1,
    Middle = 2,
    Button4 = 3,
    Button5 = 4,
    Button6 = 5,
    Button7 = 6,
    Button8 = 7
};
```

### GamepadButton

```cpp
enum class GamepadButton : std::uint8_t {
    A = 0, B, X, Y,
    LeftBumper, RightBumper,
    Back, Start, Guide,
    LeftThumb, RightThumb,
    DPadUp, DPadRight, DPadDown, DPadLeft
};
```

### GamepadAxis

```cpp
enum class GamepadAxis : std::uint8_t {
    LeftX = 0,
    LeftY,
    RightX,
    RightY,
    LeftTrigger,
    RightTrigger
};
```

### ModifierKey

Bitmask flags for keyboard modifier keys.

```cpp
enum class ModifierKey : std::uint8_t {
    None     = 0,
    Shift    = 1,
    Ctrl     = 2,
    Alt      = 4,
    Super    = 8,
    CapsLock = 16,
    NumLock  = 32
};
```

| Value | Description |
|-------|-------------|
| `None` | No modifier keys required |
| `Shift` | Either Shift key |
| `Ctrl` | Either Ctrl key |
| `Alt` | Either Alt key |
| `Super` | Windows/Command/Super key |
| `CapsLock` | Caps Lock active |
| `NumLock` | Num Lock active |

### CursorMode

```cpp
enum class CursorMode : std::uint8_t {
    Normal,     // Cursor visible and free to move
    Hidden,     // Cursor invisible but free to move
    Disabled,   // Cursor invisible and locked to window center (FPS mode)
    Confined    // Cursor visible but confined to the window area
};
```

| Value | Description |
|-------|-------------|
| `Normal` | Cursor is visible and can move freely on screen |
| `Hidden` | Cursor is invisible but mouse movement is still tracked normally |
| `Disabled` | Cursor is hidden and locked to window center; raw mouse deltas are reported |
| `Confined` | Cursor is visible but cannot leave the window boundaries |

### ActionConditionType

```cpp
enum class ActionConditionType : std::uint8_t {
    WhenPressed,     // True on the single frame input enters JustPressed
    WhenReleased,    // True on the single frame input enters JustReleased
    WhenActive,      // True while input is JustPressed, Pressed, or Held
    WhenInactive,    // True while input is NotPressed or JustReleased
    WhenHeld         // True on the single frame input transitions to Held state
};
```

### ActionEffectType

```cpp
enum class ActionEffectType : std::uint8_t {
    EmitAction,      // Emit an action event through the EventSystem
    PushPhase,       // Push a phase onto the phase stack
    PopPhase,        // Pop the top phase from the stack
    ChangePhase      // Replace the entire phase stack with a new phase
};
```

### ActionTerminal

```cpp
enum class ActionTerminal : std::uint8_t {
    Discrete,        // Emit once when conditions transition from false to true
    Continuous       // Emit every frame while conditions remain true
};
```

## Lua Examples

```lua
-- High-level: Phase management
bestow.input.pushPhase("gameplay")
print(bestow.input.getCurrentPhase())  -- "gameplay"

bestow.input.pushPhase("inventory")    -- stack: gameplay, inventory
bestow.input.popPhase()                -- stack: gameplay
bestow.input.changePhase("menu")       -- stack: menu

-- High-level: Register actions (Dvorak movement: ,AOE instead of WASD)
bestow.input.registerAction({
    phase = "gameplay",
    conditions = {{ type = "WhenActive", input = { source = "Keyboard", key = "Comma" } }},
    effects = {{ type = "EmitAction", value = "MoveUp" }},
    terminal = "Continuous"
})

-- High-level: Polling
if bestow.input.isKeyDown("Space") then
    -- jump
end
local mx, my = bestow.input.getMousePosition()
local stick = bestow.input.getLeftStick(0)

bestow.input.setCursorMode("Disabled")

-- High-level: Load config from Lua file
local _, err = bestow.input.loadConfig("config/input.lua")

-- Low-level: Full phase stack inspection
local stack = bestow.input.core.getPhaseStack()
local isActive = bestow.input.core.isPhaseActive("gameplay")

-- Low-level: Raw input capture for rebinding
bestow.input.core.startListeningForInput()
-- ... wait for user to press a key ...
local binding = bestow.input.core.getLastInput()
bestow.input.core.stopListeningForInput()

-- Low-level: Full mouse state
local scrollDelta = bestow.input.core.getScrollDelta()
local isRightDown = bestow.input.core.isMouseButtonDown("Right")

-- Low-level: Gamepad
local connected = bestow.input.core.getConnectedControllerCount()
local name = bestow.input.core.getControllerName(0)
local triggerVal = bestow.input.core.getGamepadAxisValue("LeftTrigger", 0)

-- Low-level: Text input mode
bestow.input.core.enableTextInput()
local text = bestow.input.core.getTextInput()
bestow.input.core.clearTextInput()

-- Low-level: Deadzone config
bestow.input.core.setStickDeadzone(0.15)
bestow.input.core.setTriggerDeadzone(0.1)

-- Low-level: Haptics
bestow.input.core.setGamepadVibration(0, 0.5, 0.8, 0.3)
bestow.input.core.stopGamepadVibration(0)
```

## C++ Examples

```cpp
// High-level: Phase-based action registration
input->pushPhase("gameplay");
input->registerAction(ActionRegistration{
    .phase = "gameplay",
    .conditions = {{ActionConditionType::WhenPressed, InputBinding::key(KeyCode::Space)}},
    .effects = {{ActionEffectType::EmitAction, "Jump"}},
    .terminal = ActionTerminal::Discrete
});

if (input->isKeyDown(KeyCode::Comma)) {  // Dvorak 'W' equivalent
    // move up
}

Vec2 mouse = input->getMousePosition();
Vec2 stick = input->getLeftStick(0);
input->setCursorMode(CursorMode::Disabled);

// Low-level: Full keyboard/mouse/gamepad state
if (inputCore->wasKeyJustReleased(KeyCode::Escape)) {
    inputCore->pushPhase("pause");
}

ModifierKey mods = inputCore->getModifierState();
if (hasModifier(mods, ModifierKey::Ctrl) && inputCore->wasKeyJustPressed(KeyCode::S)) {
    save();
}

Vec2 scroll = inputCore->getScrollDelta();
bool rightClick = inputCore->wasMouseButtonJustPressed(MouseButton::Right);

// Low-level: Gamepad with deadzone
inputCore->setStickDeadzone(0.2f);
float trigger = inputCore->getGamepadAxisValue(GamepadAxis::RightTrigger, 0);

// Low-level: Haptic feedback
inputCore->setGamepadVibration(0, 0.5f, 0.8f, 0.3f);

// Low-level: Raw input capture for rebinding
inputCore->startListeningForInput();
// ... later in update loop ...
if (auto binding = inputCore->getLastInput()) {
    rebindAction("Jump", *binding);
    inputCore->stopListeningForInput();
}

// Low-level: Text input for chat
inputCore->enableTextInput();
std::string typed = inputCore->getTextInput();
inputCore->clearTextInput();
```

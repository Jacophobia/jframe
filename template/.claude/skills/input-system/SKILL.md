---
name: input-system
description: Handle keyboard, mouse, and controller input in Bestow using the Action Builder or direct polling. Use when implementing player controls, input mapping, detecting key presses, mouse movement, cursor control, or controller support.
---

# Input System

## Action Builder (Recommended)

The Action Builder is the modern, event-driven input system. Actions are registered at init time and consumed via polling or events.

### Registering Actions

```lua
local k = KeyCode
local mb = MouseButton
local gb = GamepadButton
local ga = GamepadAxis

-- Movement (Dvorak + QWERTY, keyboard + gamepad)
bestow.action.builder():duringPhase("gameplay"):whenActive(k.Comma):emitAction("MoveForward"):continuously()
bestow.action.builder():duringPhase("gameplay"):whenActive(k.W):emitAction("MoveForward"):continuously()
bestow.action.builder():duringPhase("gameplay"):whenActive(k.O):emitAction("MoveBack"):continuously()
bestow.action.builder():duringPhase("gameplay"):whenActive(k.S):emitAction("MoveBack"):continuously()
bestow.action.builder():duringPhase("gameplay"):whenActive(k.A):emitAction("MoveLeft"):continuously()
bestow.action.builder():duringPhase("gameplay"):whenActive(k.E):emitAction("MoveRight"):continuously()
bestow.action.builder():duringPhase("gameplay"):whenActive(k.D):emitAction("MoveRight"):continuously()

-- Analog stick
bestow.action.builder():duringPhase("gameplay"):whenActive(ga.LeftX):withDeadzone(0.15):emitAction("MoveAxisX"):continuously()
bestow.action.builder():duringPhase("gameplay"):whenActive(ga.LeftY):withDeadzone(0.15):emitAction("MoveAxisY"):continuously()

-- Discrete actions (fire once per press)
bestow.action.builder():duringPhase("gameplay"):whenPressed(k.Space):emitAction("Jump"):discretely()
bestow.action.builder():duringPhase("gameplay"):whenPressed(gb.A):emitAction("Jump"):discretely()
bestow.action.builder():duringPhase("gameplay"):whenPressed(mb.Left):emitAction("Attack"):discretely()

-- Hold detection
bestow.action.builder():duringPhase("gameplay"):whenHeld(k.LeftShift, 0.0):emitAction("Sprint"):continuously()

-- Phase transitions
bestow.action.builder():duringPhase("gameplay"):whenPressed(k.Escape):pushPhase("pause"):discretely()
bestow.action.builder():duringPhase("pause"):whenPressed(k.Escape):popPhase():discretely()
```

### Builder API

```lua
bestow.action.builder()
    :duringPhase(phaseName)          -- Which input phase this binding is active in
    :whenPressed(input)              -- Trigger on press (discrete)
    :whenReleased(input)             -- Trigger on release
    :whenActive(input)               -- Trigger while held (continuous)
    :whenInactive(input)             -- Trigger while NOT held
    :whenHeld(input, threshold?)     -- Trigger after holding for threshold seconds
    :withDeadzone(0.0-1.0)           -- Analog deadzone (for axes)
    :emitAction(actionName)          -- Emit named action
    :pushPhase(phaseName)            -- Push a new input phase
    :popPhase()                      -- Pop current input phase
    :changePhase(phaseName)          -- Replace current input phase
    :discretely()                    -- Fire once per trigger
    :continuously()                  -- Fire every frame while active
```

### Input Types for Builder

```lua
-- Keyboard
KeyCode.Space, KeyCode.Escape, KeyCode.Enter, KeyCode.Tab, KeyCode.Backspace
KeyCode.A through KeyCode.Z
KeyCode.Num0 through KeyCode.Num9
KeyCode.F1 through KeyCode.F25
KeyCode.Up, KeyCode.Down, KeyCode.Left, KeyCode.Right
KeyCode.Comma, KeyCode.Period, KeyCode.Minus, KeyCode.Apostrophe
KeyCode.LeftShift, KeyCode.RightShift, KeyCode.LeftCtrl, KeyCode.RightCtrl
KeyCode.LeftAlt, KeyCode.RightAlt, KeyCode.LeftSuper, KeyCode.RightSuper

-- Mouse
MouseButton.Left, MouseButton.Right, MouseButton.Middle, MouseButton.Button4, MouseButton.Button5

-- Gamepad Buttons
GamepadButton.A, .B, .X, .Y, .Back, .Guide, .Start
GamepadButton.LeftStick, .RightStick, .LeftShoulder, .RightShoulder
GamepadButton.DPadUp, .DPadDown, .DPadLeft, .DPadRight

-- Gamepad Axes (analog, returns -1.0 to 1.0)
GamepadAxis.LeftX, .LeftY, .RightX, .RightY, .TriggerLeft, .TriggerRight
```

### Consuming Actions

```lua
-- In update()
if bestow.input.isActionActive("MoveForward") then
    -- Player is holding forward
end

if bestow.input.wasActionJustPressed("Jump") then
    -- Player just pressed jump this frame
end

local value = bestow.input.getActionValue("MoveAxisX")  -- -1.0 to 1.0
```

### Phase Management

Phases control which actions are active. Only actions registered for the current phase fire.

```lua
-- Push/pop phases for menus, pause, etc.
bestow.input.pushPhase("gameplay")
bestow.input.pushPhase("pause")       -- Pauses gameplay actions
bestow.input.popPhase()                -- Returns to gameplay
bestow.input.changePhase("menu")       -- Replace current phase

-- Query phase state
local phase = bestow.input.getCurrentPhase()
local stack = bestow.input.getPhaseStack()
local active = bestow.input.isPhaseActive("gameplay")
```

## Direct Polling API

> **DISCOURAGED:** Direct polling bypasses the Action Builder's phase system, rebinding support, and multi-device handling. **Always prefer the Action Builder above** for game input. Direct polling should only be used for debug tools, editor UI, or cases where the Action Builder genuinely cannot work.

For raw input state (debug/editor use only):

```lua
-- Keyboard
bestow.input.isKeyDown(KeyCode.Space) -> bool
bestow.input.wasKeyJustPressed(KeyCode.Enter) -> bool
bestow.input.wasKeyJustReleased(KeyCode.Escape) -> bool

-- Mouse position and movement
bestow.input.getMousePosition() -> Vec2
bestow.input.getMouseDelta() -> Vec2
bestow.input.getScrollDelta() -> Vec2

-- Mouse buttons
bestow.input.isMouseButtonDown(MouseButton.Left) -> bool
bestow.input.wasMouseButtonJustPressed(MouseButton.Left) -> bool
bestow.input.wasMouseButtonJustReleased(MouseButton.Left) -> bool

-- Modifier keys
bestow.input.isShiftPressed() -> bool
bestow.input.isCtrlPressed() -> bool
bestow.input.isAltPressed() -> bool
bestow.input.isSuperPressed() -> bool
bestow.input.getModifierState() -> ModifierKey  -- bitmask

-- Gamepad
bestow.input.isGamepadButtonDown(GamepadButton.A, playerIndex?) -> bool
bestow.input.getGamepadAxisValue(GamepadAxis.LeftX, playerIndex?) -> float
bestow.input.getLeftStick(playerIndex?) -> Vec2
bestow.input.getRightStick(playerIndex?) -> Vec2
bestow.input.getConnectedControllerCount() -> int
bestow.input.isControllerConnected(index) -> bool
```

## Cursor Control

```lua
bestow.input.showMouseCursor()
bestow.input.hideMouseCursor()
bestow.input.isMouseCursorVisible() -> bool

-- CursorMode: Normal (visible, free), Hidden (invisible, free), Disabled (invisible, locked)
bestow.input.setCursorMode(CursorMode.Normal)
bestow.input.setCursorMode(CursorMode.Disabled)  -- For FPS camera
bestow.input.getCursorMode() -> CursorMode
```

## Text Input (for UI/Chat)

```lua
bestow.input.enableTextInput()
bestow.input.disableTextInput()
bestow.input.isTextInputEnabled() -> bool
bestow.input.getTextInput() -> string      -- Accumulated text since last clear
bestow.input.clearTextInput()
```

## Input Rebinding (for Settings Screen)

```lua
bestow.input.startListeningForInput()
bestow.input.stopListeningForInput()
bestow.input.isListeningForInput() -> bool
bestow.input.getLastInput() -> InputBinding | nil  -- Returns the binding for whatever was pressed
```

## Global Key Aliases

For convenience, a global `Keys.*` table is also available:
```lua
Keys.A through Keys.Z, Keys.Space, Keys.Escape, Keys.Enter, Keys.Tab
Keys.Up, Keys.Down, Keys.Left, Keys.Right
Keys.Comma, Keys.O, Keys.E  -- Dvorak movement equivalents
Keys.F1 through Keys.F12, Keys.LeftShift, Keys.LeftCtrl, Keys.LeftAlt
```

## Common Patterns

### Full Movement Setup

```lua
-- In init()
local function setupInput()
    local k = KeyCode
    local ga = GamepadAxis
    local gb = GamepadButton

    -- Movement (Dvorak + QWERTY + Gamepad)
    for _, key in ipairs({k.Comma, k.W}) do
        bestow.action.builder():duringPhase("gameplay"):whenActive(key):emitAction("MoveForward"):continuously()
    end
    for _, key in ipairs({k.O, k.S}) do
        bestow.action.builder():duringPhase("gameplay"):whenActive(key):emitAction("MoveBack"):continuously()
    end
    bestow.action.builder():duringPhase("gameplay"):whenActive(k.A):emitAction("MoveLeft"):continuously()
    for _, key in ipairs({k.E, k.D}) do
        bestow.action.builder():duringPhase("gameplay"):whenActive(key):emitAction("MoveRight"):continuously()
    end

    -- Gamepad analog sticks
    bestow.action.builder():duringPhase("gameplay"):whenActive(ga.LeftX):withDeadzone(0.15):emitAction("MoveAxisX"):continuously()
    bestow.action.builder():duringPhase("gameplay"):whenActive(ga.LeftY):withDeadzone(0.15):emitAction("MoveAxisY"):continuously()

    -- Jump
    bestow.action.builder():duringPhase("gameplay"):whenPressed(k.Space):emitAction("Jump"):discretely()
    bestow.action.builder():duringPhase("gameplay"):whenPressed(gb.A):emitAction("Jump"):discretely()

    -- Start in gameplay phase
    bestow.input.pushPhase("gameplay")
end

-- In update()
local function getMovement()
    local moveX, moveZ = 0, 0

    -- Digital input
    if bestow.input.isActionActive("MoveLeft") then moveX = moveX - 1 end
    if bestow.input.isActionActive("MoveRight") then moveX = moveX + 1 end
    if bestow.input.isActionActive("MoveForward") then moveZ = moveZ - 1 end
    if bestow.input.isActionActive("MoveBack") then moveZ = moveZ + 1 end

    -- Analog override
    local stickX = bestow.input.getActionValue("MoveAxisX") or 0
    local stickY = bestow.input.getActionValue("MoveAxisY") or 0
    if math.abs(stickX) > 0.01 then moveX = stickX end
    if math.abs(stickY) > 0.01 then moveZ = stickY end

    return moveX, moveZ
end
```

### FPS Mouse Look

```lua
bestow.input.setCursorMode(CursorMode.Disabled)

local function updateMouseLook(state)
    local delta = bestow.input.getMouseDelta()
    local sensitivity = 0.002
    state.yaw = state.yaw - delta.x * sensitivity
    state.pitch = math.max(-1.5, math.min(1.5, state.pitch - delta.y * sensitivity))
    return Quat.fromEuler(state.pitch, state.yaw, 0)
end
```

## Best Practices

1. **Use Action Builder** for all game input - supports rebinding and multiple devices
2. **Always support Dvorak** - Bind ,AOE alongside WASD
3. **Use phases** to manage input contexts (gameplay, pause, menu, dialogue)
4. **Use `discretely()`** for one-shot actions (jump, attack, interact)
5. **Use `continuously()`** for held actions (movement, sprint, aim)
6. **Gate on UI** - Check `bestow.ui.wantsKeyboardInput()` before processing game input when UI is active

---
name: input-system
description: Handle keyboard, mouse, and controller input in Bestow. Use when implementing player controls, input mapping, detecting key presses, mouse movement, or controller support.
---

# Input System

The input system handles all player input including keyboard, mouse, and game controllers.

## Complete API Reference

### Action-Based Input (Recommended)

Actions are named inputs that can be bound to multiple keys/buttons:

```lua
-- Check if action is currently active (held down)
bestow.input.isActionActive(action: string) -> bool

-- Check if action was just pressed this frame
bestow.input.wasActionJustPressed(action: string) -> bool

-- Check if action was just released this frame
bestow.input.wasActionJustReleased(action: string) -> bool

-- Get analog value (0.0 to 1.0 for buttons, -1.0 to 1.0 for axes)
bestow.input.getActionValue(action: string) -> float

-- Get full action state
bestow.input.getActionState(action: string) -> ActionState
-- Returns: { action: string, active: bool, value: float, justPressed: bool, justReleased: bool }

-- Get all action states
bestow.input.getAllActionStates() -> table<string, ActionState>
```

### Action Mapping

```lua
-- Register a new input mapping
bestow.input.registerMapping(mapping: InputMapping)
-- InputMapping = { binding: InputBinding, action: string }
-- InputBinding = {
--     deviceType: "Keyboard" | "Mouse" | "Controller",
--     deviceIndex: int (default 0),
--     keyCode: int,
--     requiredModifiers: ModifierKey (default None),
--     scale: float (default 1.0),
--     deadzone: float (default 0.0)
-- }

-- Remove a mapping
bestow.input.removeMapping(binding: InputBinding)

-- Clear all mappings
bestow.input.clearMappings()

-- Get all current mappings
bestow.input.getMappings() -> table<InputMapping>
```

### Direct Keyboard Input

```lua
-- Is key currently held down?
bestow.input.isKeyDown(keyCode: int) -> bool

-- Was key just pressed this frame?
bestow.input.wasKeyJustPressed(keyCode: int) -> bool

-- Was key just released this frame?
bestow.input.wasKeyJustReleased(keyCode: int) -> bool
```

### Mouse Input

```lua
-- Get mouse position in screen coordinates
bestow.input.getMousePosition() -> Vec2

-- Get mouse movement since last frame
bestow.input.getMouseDelta() -> Vec2

-- Check mouse button state
bestow.input.isMouseButtonDown(button: int) -> bool
bestow.input.wasMouseButtonJustPressed(button: int) -> bool
bestow.input.wasMouseButtonJustReleased(button: int) -> bool

-- Get scroll wheel delta
bestow.input.getScrollDelta() -> float

-- Mouse button constants
bestow.input.Mouse.LEFT    -- 0
bestow.input.Mouse.RIGHT   -- 1
bestow.input.Mouse.MIDDLE  -- 2
bestow.input.Mouse.BUTTON_4 through BUTTON_8
```

### Modifier Keys

```lua
-- Check modifier state
bestow.input.isShiftPressed() -> bool
bestow.input.isCtrlPressed() -> bool
bestow.input.isAltPressed() -> bool
bestow.input.isSuperPressed() -> bool

-- Get modifier bitmask
bestow.input.getModifierState() -> ModifierKey

-- Check specific modifier in bitmask
bestow.input.isModifierPressed(mod: ModifierKey) -> bool

-- ModifierKey values
ModifierKey.None, .Shift, .Ctrl, .Alt, .Super, .CapsLock, .NumLock
```

### Text Input (for UI)

```lua
-- Enable text input mode (shows virtual keyboard on mobile)
bestow.input.enableTextInput()

-- Disable text input mode
bestow.input.disableTextInput()

-- Check if text input is enabled
bestow.input.isTextInputEnabled() -> bool

-- Get accumulated text input
bestow.input.getTextInput() -> string

-- Clear text input buffer
bestow.input.clearTextInput()
```

### Controller Support

```lua
-- Get number of connected controllers
bestow.input.getConnectedControllerCount() -> int

-- Check if specific controller is connected
bestow.input.isControllerConnected(index: int) -> bool

-- Get controller name
bestow.input.getControllerName(index: int) -> string
```

### Input Rebinding (for Settings UI)

```lua
-- Start listening for any input (for rebinding)
bestow.input.startListeningForInput()

-- Stop listening
bestow.input.stopListeningForInput()

-- Check if listening
bestow.input.isListeningForInput() -> bool

-- Get the last input received (returns nil if none)
bestow.input.getLastInput() -> InputBinding | nil
```

## Key Constants

### Global Keys.* Aliases (Convenient)

```lua
Keys.A through Keys.Z
Keys.Space, Keys.Escape, Keys.Enter, Keys.Tab, Keys.Backspace
Keys.Up, Keys.Down, Keys.Left, Keys.Right
Keys.F1 through Keys.F12
Keys.LeftShift, Keys.RightShift
Keys.LeftCtrl, Keys.RightCtrl
Keys.LeftAlt, Keys.RightAlt
Keys.Comma, Keys.O, Keys.E  -- Dvorak movement equivalents
```

### Full Key Codes (bestow.input.Key.*)

```lua
bestow.input.Key.SPACE       -- 32
bestow.input.Key.A-Z         -- 65-90
bestow.input.Key.LEFT        -- 263
bestow.input.Key.RIGHT       -- 262
bestow.input.Key.UP          -- 265
bestow.input.Key.DOWN        -- 264
bestow.input.Key.ESCAPE      -- 256
bestow.input.Key.ENTER       -- 257
bestow.input.Key.TAB         -- 258
bestow.input.Key.BACKSPACE   -- 259
bestow.input.Key.F1-F12      -- 290-301
bestow.input.Key.LEFT_SHIFT  -- 340
bestow.input.Key.LEFT_CONTROL-- 341
bestow.input.Key.LEFT_ALT    -- 342
bestow.input.Key.COMMA       -- 44
```

## Common Patterns

### Basic Movement (Dvorak + QWERTY)

```lua
local function getMovementInput()
    local moveX = 0
    local moveZ = 0

    -- Left (same on both layouts)
    if bestow.input.isKeyDown(Keys.A) then moveX = moveX - 1 end

    -- Right (Dvorak: E, QWERTY: D)
    if bestow.input.isKeyDown(Keys.E) or bestow.input.isKeyDown(Keys.D) then
        moveX = moveX + 1
    end

    -- Forward (Dvorak: comma, QWERTY: W)
    if bestow.input.isKeyDown(Keys.Comma) or bestow.input.isKeyDown(Keys.W) then
        moveZ = moveZ - 1
    end

    -- Back (Dvorak: O, QWERTY: S)
    if bestow.input.isKeyDown(Keys.O) or bestow.input.isKeyDown(Keys.S) then
        moveZ = moveZ + 1
    end

    return Vec2.new(moveX, moveZ)
end
```

### Action-Based Movement Setup

```lua
-- In init()
local function registerMovementMappings()
    -- Forward (Dvorak + QWERTY)
    bestow.input.registerMapping({
        binding = { deviceType = "Keyboard", keyCode = Keys.Comma },
        action = "move_forward"
    })
    bestow.input.registerMapping({
        binding = { deviceType = "Keyboard", keyCode = Keys.W },
        action = "move_forward"
    })

    -- Back
    bestow.input.registerMapping({
        binding = { deviceType = "Keyboard", keyCode = Keys.O },
        action = "move_back"
    })
    bestow.input.registerMapping({
        binding = { deviceType = "Keyboard", keyCode = Keys.S },
        action = "move_back"
    })

    -- Left
    bestow.input.registerMapping({
        binding = { deviceType = "Keyboard", keyCode = Keys.A },
        action = "move_left"
    })

    -- Right
    bestow.input.registerMapping({
        binding = { deviceType = "Keyboard", keyCode = Keys.E },
        action = "move_right"
    })
    bestow.input.registerMapping({
        binding = { deviceType = "Keyboard", keyCode = Keys.D },
        action = "move_right"
    })

    -- Jump
    bestow.input.registerMapping({
        binding = { deviceType = "Keyboard", keyCode = Keys.Space },
        action = "jump"
    })
end

-- In update()
local function getMovementFromActions()
    local moveX = 0
    local moveZ = 0

    if bestow.input.isActionActive("move_left") then moveX = moveX - 1 end
    if bestow.input.isActionActive("move_right") then moveX = moveX + 1 end
    if bestow.input.isActionActive("move_forward") then moveZ = moveZ - 1 end
    if bestow.input.isActionActive("move_back") then moveZ = moveZ + 1 end

    return Vec2.new(moveX, moveZ)
end
```

### Mouse Look (FPS Camera)

```lua
local sensitivity = 0.002
local yaw = 0
local pitch = 0

local function updateMouseLook()
    local delta = bestow.input.getMouseDelta()

    yaw = yaw - delta.x * sensitivity
    pitch = pitch - delta.y * sensitivity

    -- Clamp pitch to prevent flipping
    pitch = math.max(-1.5, math.min(1.5, pitch))

    -- Create rotation quaternion
    local rotation = Quat.fromEuler(pitch, yaw, 0)
    return rotation
end
```

### Input Rebinding UI

```lua
local waitingForKey = nil  -- Action name we're rebinding

local function startRebind(actionName)
    waitingForKey = actionName
    bestow.input.startListeningForInput()
end

local function checkRebind()
    if not waitingForKey then return end

    local input = bestow.input.getLastInput()
    if input then
        -- Remove old mapping
        bestow.input.removeMapping({ deviceType = "Keyboard", keyCode = oldKey })

        -- Add new mapping
        bestow.input.registerMapping({
            binding = input,
            action = waitingForKey
        })

        -- Done rebinding
        bestow.input.stopListeningForInput()
        waitingForKey = nil
    end
end
```

## Best Practices

1. **Use action-based input** for maintainability and rebinding support
2. **Always support Dvorak** - Use ,AOE in addition to WASD
3. **Check justPressed for one-shot actions** like jumping, shooting
4. **Check isKeyDown for continuous actions** like movement
5. **Use modifiers for alternative actions** like shift+click
6. **Gate input on UI** - Check `bestow.ui.wantsKeyboardInput()` before game input

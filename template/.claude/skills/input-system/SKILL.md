---
name: input-system
description: Handle keyboard, mouse, and controller input in Bestow. Use when detecting key presses, mouse movement, controller buttons, or setting up input mappings.
---

# Input System

The input system provides access to keyboard, mouse, and game controller input.

## Keyboard Input

### Key State Queries

```lua
-- Is key currently held down?
if bestow.input.isKeyDown(Keys.Space) then
    -- Space is pressed
end

-- Was key just pressed this frame?
if bestow.input.wasKeyJustPressed(Keys.Space) then
    -- Space was just pressed (fires once)
end

-- Was key just released this frame?
if bestow.input.wasKeyJustReleased(Keys.Space) then
    -- Space was just released
end
```

### Common Key Constants

```lua
-- Letters
Keys.A, Keys.B, Keys.C, ... Keys.Z

-- Numbers
Keys.Num0, Keys.Num1, ... Keys.Num9

-- Special
Keys.Space, Keys.Escape, Keys.Enter, Keys.Tab
Keys.Backspace, Keys.Delete, Keys.Home, Keys.End

-- Arrows
Keys.Up, Keys.Down, Keys.Left, Keys.Right

-- Modifiers
Keys.LeftShift, Keys.RightShift
Keys.LeftCtrl, Keys.RightCtrl
Keys.LeftAlt, Keys.RightAlt

-- Function keys
Keys.F1, Keys.F2, ... Keys.F12

-- Punctuation (important for Dvorak!)
Keys.Comma      -- ',' (Dvorak W equivalent)
Keys.Period     -- '.'
Keys.Semicolon  -- ';'
```

### Dvorak-Friendly Movement

**IMPORTANT:** The project owner uses Dvorak. Always support both layouts:

```lua
-- Movement that works for both Dvorak and QWERTY
local function getMovementInput()
    local movement = Vec3.new(0, 0, 0)

    -- Forward: comma (Dvorak) or W (QWERTY)
    if bestow.input.isKeyDown(Keys.Comma) or bestow.input.isKeyDown(Keys.W) then
        movement.z = -1
    end

    -- Back: O (Dvorak) or S (QWERTY)
    if bestow.input.isKeyDown(Keys.O) or bestow.input.isKeyDown(Keys.S) then
        movement.z = 1
    end

    -- Left: A (same on both)
    if bestow.input.isKeyDown(Keys.A) then
        movement.x = -1
    end

    -- Right: E (Dvorak) or D (QWERTY)
    if bestow.input.isKeyDown(Keys.E) or bestow.input.isKeyDown(Keys.D) then
        movement.x = 1
    end

    -- Arrow keys work too
    if bestow.input.isKeyDown(Keys.Up) then movement.z = -1 end
    if bestow.input.isKeyDown(Keys.Down) then movement.z = 1 end
    if bestow.input.isKeyDown(Keys.Left) then movement.x = -1 end
    if bestow.input.isKeyDown(Keys.Right) then movement.x = 1 end

    return movement
end
```

### Modifier Keys

```lua
-- Check modifier state
if bestow.input.isShiftPressed() then
    -- Shift is held
end

if bestow.input.isCtrlPressed() then
    -- Ctrl is held
end

if bestow.input.isAltPressed() then
    -- Alt is held
end

-- Combine for shortcuts
if bestow.input.isCtrlPressed() and bestow.input.wasKeyJustPressed(Keys.S) then
    -- Ctrl+S: Save game
    app.systems.save.quickSave()
end
```

## Mouse Input

### Position and Movement

```lua
-- Get mouse position in screen coordinates
local mousePos = bestow.input.getMousePosition()
print("Mouse at: " .. mousePos.x .. ", " .. mousePos.y)

-- Get mouse movement since last frame
local delta = bestow.input.getMouseDelta()
if delta:length() > 0 then
    -- Mouse moved
end

-- Get scroll wheel delta
local scroll = bestow.input.getScrollDelta()
if scroll.y ~= 0 then
    -- Scrolled up (positive) or down (negative)
end
```

### Mouse Buttons

```lua
-- Button constants
-- 0 = Left, 1 = Right, 2 = Middle

-- Is button held?
if bestow.input.isMouseButtonDown(0) then
    -- Left mouse button held
end

-- Was button just clicked?
if bestow.input.wasMouseButtonJustPressed(0) then
    -- Left click this frame
end

-- Was button just released?
if bestow.input.wasMouseButtonJustReleased(1) then
    -- Right button released
end
```

### Screen to World Conversion

```lua
-- Convert mouse position to world coordinates
local mouseScreen = bestow.input.getMousePosition()
local mouseWorld = bestow.camera3d.screenToWorld(mouseScreen)

-- Raycast from camera through mouse position
local camera = bestow.camera3d.getCamera()
local ray = bestow.camera3d.screenToRay(mouseScreen)
local hit = bestow.physics3d.raycast(ray.origin, ray.direction, 1000)
if hit then
    -- Clicked on something at hit.point
end
```

## Text Input

For text fields and chat input:

```lua
-- Enable text input mode (shows virtual keyboard on mobile)
bestow.input.enableTextInput()

-- Get typed characters this frame
local text = bestow.input.getTextInput()
if #text > 0 then
    app.main.state.inputBuffer = app.main.state.inputBuffer .. text
end

-- Clear the buffer
bestow.input.clearTextInput()

-- Disable when done
bestow.input.disableTextInput()
```

## Action System (Input Mapping)

For remappable controls:

```lua
-- Check if action is active (regardless of binding)
if bestow.input.isActionActive("jump") then
    -- Jump action triggered
end

if bestow.input.wasActionJustPressed("attack") then
    -- Attack action just pressed
end

-- Get analog value (-1 to 1)
local moveX = bestow.input.getActionValue("move_horizontal")
local moveY = bestow.input.getActionValue("move_vertical")
```

## Controller Support

```lua
-- Check connected controllers
local count = bestow.input.getConnectedControllerCount()
if count > 0 then
    -- At least one controller connected
end

-- Check specific controller
if bestow.input.isControllerConnected(0) then
    local name = bestow.input.getControllerName(0)
    print("Controller: " .. name)
end
```

## Input Pattern: Movement System

```lua
-- systems/movement.lua
return {
    speed = 5.0,

    update = function(dt)
        local self = app.systems.movement
        local state = app.main.state

        if not state.player then return end

        local movement = Vec3.new(0, 0, 0)

        -- Dvorak/QWERTY movement
        if bestow.input.isKeyDown(Keys.Comma) or bestow.input.isKeyDown(Keys.W) then
            movement.z = -1
        end
        if bestow.input.isKeyDown(Keys.O) or bestow.input.isKeyDown(Keys.S) then
            movement.z = 1
        end
        if bestow.input.isKeyDown(Keys.A) then
            movement.x = -1
        end
        if bestow.input.isKeyDown(Keys.E) or bestow.input.isKeyDown(Keys.D) then
            movement.x = 1
        end

        -- Normalize diagonal movement
        if movement:length() > 0 then
            movement = movement:normalize() * self.speed * dt
            local pos = bestow.entity.getField(state.player, "Transform3D", "position")
            bestow.entity.setField(state.player, "Transform3D", "position", pos + movement)
        end
    end
}
```

## Input Pattern: Pause Menu

```lua
-- In main.lua update()
update = function(dt)
    local state = app.main.state

    -- Escape to toggle pause
    if bestow.input.wasKeyJustPressed(Keys.Escape) then
        if state.phase == "playing" then
            state.phase = "paused"
        elseif state.phase == "paused" then
            state.phase = "playing"
        else
            -- In menu, escape quits
            return false
        end
    end

    -- Only update game when playing
    if state.phase == "playing" then
        app.systems.movement.update(dt)
        app.systems.enemies.update(dt)
    end

    return true  -- Continue running
end
```

## Best Practices

1. **Always support Dvorak AND QWERTY** - Check both key layouts for movement
2. **Use wasKeyJustPressed for actions** - Not isKeyDown (fires every frame)
3. **Use isKeyDown for continuous input** - Movement, holding buttons
4. **Store input config in a system** - Easy to modify key bindings
5. **Provide arrow key fallbacks** - Universal movement keys

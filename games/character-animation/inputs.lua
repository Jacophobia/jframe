--[[
    Input Action Definitions

    Returns a function that registers input actions.
    Called explicitly from main.lua: app.inputs()

    Movement uses Dvorak layout (,AOE) as per user preferences.
    Each action builder creates ONE mapping.
    Multiple inputs for the same action = multiple builders.
]]

return function()

local k = bestow.input.keys
local btn = bestow.input.buttons  -- Gamepad buttons (A, B, X, Y, DPad, etc.)

--------------------------------------------------
-- Movement Actions (continuous while held)
-- Using ,AOE for Dvorak (equivalent to WASD positions)
--------------------------------------------------

-- Forward - Dvorak W (comma key)
bestow.action.builder()
    :duringPhase("game.playing")
    :whenActive(k.Comma)
    :emitAction("MoveForward")
    :continuously()

bestow.action.builder()
    :duringPhase("game.playing")
    :whenActive(k.Up)
    :emitAction("MoveForward")
    :continuously()

-- Gamepad DPad for movement (stick axes require different handling)
bestow.action.builder()
    :duringPhase("game.playing")
    :whenActive(btn.DPadUp)
    :emitAction("MoveForward")
    :continuously()

-- Backward - Dvorak S (O key)
bestow.action.builder()
    :duringPhase("game.playing")
    :whenActive(k.O)
    :emitAction("MoveBackward")
    :continuously()

bestow.action.builder()
    :duringPhase("game.playing")
    :whenActive(k.Down)
    :emitAction("MoveBackward")
    :continuously()

bestow.action.builder()
    :duringPhase("game.playing")
    :whenActive(btn.DPadDown)
    :emitAction("MoveBackward")
    :continuously()

-- Left - Dvorak A (A key, same as QWERTY)
bestow.action.builder()
    :duringPhase("game.playing")
    :whenActive(k.A)
    :emitAction("MoveLeft")
    :continuously()

bestow.action.builder()
    :duringPhase("game.playing")
    :whenActive(k.Left)
    :emitAction("MoveLeft")
    :continuously()

bestow.action.builder()
    :duringPhase("game.playing")
    :whenActive(btn.DPadLeft)
    :emitAction("MoveLeft")
    :continuously()

-- Right - Dvorak D (E key)
bestow.action.builder()
    :duringPhase("game.playing")
    :whenActive(k.E)
    :emitAction("MoveRight")
    :continuously()

bestow.action.builder()
    :duringPhase("game.playing")
    :whenActive(k.Right)
    :emitAction("MoveRight")
    :continuously()

bestow.action.builder()
    :duringPhase("game.playing")
    :whenActive(btn.DPadRight)
    :emitAction("MoveRight")
    :continuously()

--------------------------------------------------
-- Run modifier (hold to run instead of walk)
--------------------------------------------------

bestow.action.builder()
    :duringPhase("game.playing")
    :whenActive(k.LeftShift)
    :emitAction("Run")
    :continuously()

-- Left trigger as run modifier (treated as button when pressed past threshold)
bestow.action.builder()
    :duringPhase("game.playing")
    :whenActive(btn.LeftTrigger)
    :emitAction("Run")
    :continuously()

--------------------------------------------------
-- Jump Action (discrete)
--------------------------------------------------

bestow.action.builder()
    :duringPhase("game.playing")
    :whenPressed(k.Space)
    :emitAction("Jump")
    :discretely()

bestow.action.builder()
    :duringPhase("game.playing")
    :whenPressed(btn.A)
    :emitAction("Jump")
    :discretely()

--------------------------------------------------
-- Quit Action
--------------------------------------------------

bestow.action.builder()
    :duringPhase("game.playing")
    :whenPressed(k.Escape)
    :emitAction("Quit")
    :discretely()

--------------------------------------------------
-- Gamepad Analog Sticks
--------------------------------------------------

local axes = bestow.input.axes

-- Left stick for movement
bestow.action.builder()
    :duringPhase("game.playing")
    :whenActive(axes.LeftX)
    :withDeadzone(0.15)
    :emitAction("MoveAxisX")
    :continuously()

bestow.action.builder()
    :duringPhase("game.playing")
    :whenActive(axes.LeftY)
    :withDeadzone(0.15)
    :emitAction("MoveAxisY")
    :continuously()

-- Right stick for camera
bestow.action.builder()
    :duringPhase("game.playing")
    :whenActive(axes.RightX)
    :withDeadzone(0.15)
    :emitAction("CameraAxisX")
    :continuously()

bestow.action.builder()
    :duringPhase("game.playing")
    :whenActive(axes.RightY)
    :withDeadzone(0.15)
    :emitAction("CameraAxisY")
    :continuously()

end -- return function()

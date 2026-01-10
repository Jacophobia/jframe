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
local gp = bestow.input.gamepad

--------------------------------------------------
-- Movement Actions (continuous while held)
-- Using ,AOE for Dvorak (equivalent to WASD positions)
--------------------------------------------------

-- Forward - Dvorak W (comma key)
bestow.action.builder()
    :duringPhase("game.playing")
    :whileActive(k.Comma)
    :emitAction("MoveForward")
    :continuously()

bestow.action.builder()
    :duringPhase("game.playing")
    :whileActive(k.Up)
    :emitAction("MoveForward")
    :continuously()

bestow.action.builder()
    :duringPhase("game.playing")
    :whileActive(gp.LeftStickUp)
    :emitAction("MoveForward")
    :continuously()

-- Backward - Dvorak S (O key)
bestow.action.builder()
    :duringPhase("game.playing")
    :whileActive(k.O)
    :emitAction("MoveBackward")
    :continuously()

bestow.action.builder()
    :duringPhase("game.playing")
    :whileActive(k.Down)
    :emitAction("MoveBackward")
    :continuously()

bestow.action.builder()
    :duringPhase("game.playing")
    :whileActive(gp.LeftStickDown)
    :emitAction("MoveBackward")
    :continuously()

-- Left - Dvorak A (A key, same as QWERTY)
bestow.action.builder()
    :duringPhase("game.playing")
    :whileActive(k.A)
    :emitAction("MoveLeft")
    :continuously()

bestow.action.builder()
    :duringPhase("game.playing")
    :whileActive(k.Left)
    :emitAction("MoveLeft")
    :continuously()

bestow.action.builder()
    :duringPhase("game.playing")
    :whileActive(gp.LeftStickLeft)
    :emitAction("MoveLeft")
    :continuously()

-- Right - Dvorak D (E key)
bestow.action.builder()
    :duringPhase("game.playing")
    :whileActive(k.E)
    :emitAction("MoveRight")
    :continuously()

bestow.action.builder()
    :duringPhase("game.playing")
    :whileActive(k.Right)
    :emitAction("MoveRight")
    :continuously()

bestow.action.builder()
    :duringPhase("game.playing")
    :whileActive(gp.LeftStickRight)
    :emitAction("MoveRight")
    :continuously()

--------------------------------------------------
-- Run modifier (hold to run instead of walk)
--------------------------------------------------

bestow.action.builder()
    :duringPhase("game.playing")
    :whileActive(k.LeftShift)
    :emitAction("Run")
    :continuously()

bestow.action.builder()
    :duringPhase("game.playing")
    :whileActive(gp.LeftTrigger)
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
    :whenPressed(gp.A)
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

end -- return function()

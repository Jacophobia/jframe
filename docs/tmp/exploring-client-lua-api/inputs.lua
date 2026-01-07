--[[
    Input Action Definitions

    Returns a function that registers input actions.
    Called explicitly from app.lua: app.inputs()

    Each action builder creates ONE mapping.
    Multiple inputs for the same action = multiple builders.

    Movement uses Dvorak layout (,AOE) as per user preferences.
]]

return function()

local k = bestow.input.keys
local m = bestow.input.mouse
local gp = bestow.input.gamepad

--------------------------------------------------
-- Menu Actions
--------------------------------------------------

-- Menu Up - keyboard
bestow.action.builder()
    :duringPhase("menu")
    :whenPressed(k.Up)
    :emitAction("MenuUp")
    :discretely()

-- Menu Up - Dvorak W
bestow.action.builder()
    :duringPhase("menu")
    :whenPressed(k.Comma)
    :emitAction("MenuUp")
    :discretely()

-- Menu Up - gamepad d-pad
bestow.action.builder()
    :duringPhase("menu")
    :whenPressed(gp.DPadUp)
    :emitAction("MenuUp")
    :discretely()

-- Menu Up - gamepad stick
bestow.action.builder()
    :duringPhase("menu")
    :whenPressed(gp.LeftStickUp)
    :emitAction("MenuUp")
    :discretely()

-- Menu Down - keyboard
bestow.action.builder()
    :duringPhase("menu")
    :whenPressed(k.Down)
    :emitAction("MenuDown")
    :discretely()

-- Menu Down - Dvorak S
bestow.action.builder()
    :duringPhase("menu")
    :whenPressed(k.O)
    :emitAction("MenuDown")
    :discretely()

-- Menu Down - gamepad
bestow.action.builder()
    :duringPhase("menu")
    :whenPressed(gp.DPadDown)
    :emitAction("MenuDown")
    :discretely()

-- Menu Confirm
bestow.action.builder()
    :duringPhase("menu")
    :whenPressed(k.Return)
    :emitAction("MenuConfirm")
    :discretely()

bestow.action.builder()
    :duringPhase("menu")
    :whenPressed(k.Space)
    :emitAction("MenuConfirm")
    :discretely()

bestow.action.builder()
    :duringPhase("menu")
    :whenPressed(gp.A)
    :emitAction("MenuConfirm")
    :discretely()

-- Menu Cancel
bestow.action.builder()
    :duringPhase("menu")
    :whenPressed(k.Escape)
    :emitAction("MenuCancel")
    :discretely()

bestow.action.builder()
    :duringPhase("menu")
    :whenPressed(gp.B)
    :emitAction("MenuCancel")
    :discretely()

--------------------------------------------------
-- Game Combat - Movement (continuous while active)
-- Using ,AOE for Dvorak (equivalent to WASD positions)
--------------------------------------------------

-- Forward - Dvorak W (comma key)
bestow.action.builder()
    :duringPhase("game.combat")
    :whileActive(k.Comma)
    :emitAction("MoveForward")
    :continuously()

bestow.action.builder()
    :duringPhase("game.combat")
    :whileActive(gp.LeftStickUp)
    :emitAction("MoveForward")
    :continuously()

-- Backward - Dvorak S (O key)
bestow.action.builder()
    :duringPhase("game.combat")
    :whileActive(k.O)
    :emitAction("MoveBackward")
    :continuously()

bestow.action.builder()
    :duringPhase("game.combat")
    :whileActive(gp.LeftStickDown)
    :emitAction("MoveBackward")
    :continuously()

-- Left - Dvorak A (A key, same as QWERTY)
bestow.action.builder()
    :duringPhase("game.combat")
    :whileActive(k.A)
    :emitAction("MoveLeft")
    :continuously()

bestow.action.builder()
    :duringPhase("game.combat")
    :whileActive(gp.LeftStickLeft)
    :emitAction("MoveLeft")
    :continuously()

-- Right - Dvorak D (E key)
bestow.action.builder()
    :duringPhase("game.combat")
    :whileActive(k.E)
    :emitAction("MoveRight")
    :continuously()

bestow.action.builder()
    :duringPhase("game.combat")
    :whileActive(gp.LeftStickRight)
    :emitAction("MoveRight")
    :continuously()

--------------------------------------------------
-- Game Combat - Actions (discrete)
--------------------------------------------------

-- Jump
bestow.action.builder()
    :duringPhase("game.combat")
    :whenPressed(k.Space)
    :emitAction("Jump")
    :discretely()

bestow.action.builder()
    :duringPhase("game.combat")
    :whenPressed(gp.A)
    :emitAction("Jump")
    :discretely()

-- Attack
bestow.action.builder()
    :duringPhase("game.combat")
    :whenPressed(m.Left)
    :emitAction("Attack")
    :discretely()

bestow.action.builder()
    :duringPhase("game.combat")
    :whenPressed(k.J)
    :emitAction("Attack")
    :discretely()

bestow.action.builder()
    :duringPhase("game.combat")
    :whenPressed(gp.X)
    :emitAction("Attack")
    :discretely()

-- Heavy attack
bestow.action.builder()
    :duringPhase("game.combat")
    :whenPressed(m.Right)
    :emitAction("HeavyAttack")
    :discretely()

bestow.action.builder()
    :duringPhase("game.combat")
    :whenPressed(k.K)
    :emitAction("HeavyAttack")
    :discretely()

bestow.action.builder()
    :duringPhase("game.combat")
    :whenPressed(gp.Y)
    :emitAction("HeavyAttack")
    :discretely()

-- Block (continuous while held)
bestow.action.builder()
    :duringPhase("game.combat")
    :whileActive(k.LeftShift)
    :emitAction("Block")
    :continuously()

bestow.action.builder()
    :duringPhase("game.combat")
    :whileActive(gp.LeftTrigger)
    :emitAction("Block")
    :continuously()

-- Dash
bestow.action.builder()
    :duringPhase("game.combat")
    :whenPressed(k.LeftCtrl)
    :emitAction("Dash")
    :discretely()

bestow.action.builder()
    :duringPhase("game.combat")
    :whenPressed(gp.B)
    :emitAction("Dash")
    :discretely()

-- Interact
bestow.action.builder()
    :duringPhase("game.combat")
    :whenPressed(k.F)
    :emitAction("Interact")
    :discretely()

-- Camera look (continuous axis)
bestow.action.builder()
    :duringPhase("game.combat")
    :whileMoving(m.Delta)
    :emitAction("Look")
    :continuously()

bestow.action.builder()
    :duringPhase("game.combat")
    :whileMoving(gp.RightStick)
    :emitAction("Look")
    :continuously()

--------------------------------------------------
-- UI Actions
--------------------------------------------------

-- Pause
bestow.action.builder()
    :duringPhase("game.combat")
    :whenPressed(k.Escape)
    :emitAction("TogglePause")
    :discretely()

bestow.action.builder()
    :duringPhase("game.combat")
    :whenPressed(gp.Start)
    :emitAction("TogglePause")
    :discretely()

-- Unpause (from paused state)
bestow.action.builder()
    :duringPhase("game.paused")
    :whenPressed(k.Escape)
    :emitAction("TogglePause")
    :discretely()

bestow.action.builder()
    :duringPhase("game.paused")
    :whenPressed(gp.Start)
    :emitAction("TogglePause")
    :discretely()

bestow.action.builder()
    :duringPhase("game.paused")
    :whenPressed(gp.B)
    :emitAction("TogglePause")
    :discretely()

-- Inventory toggle
bestow.action.builder()
    :duringPhase("game.combat")
    :whenPressed(k.I)
    :emitAction("ToggleInventory")
    :discretely()

bestow.action.builder()
    :duringPhase("game.combat")
    :whenPressed(k.Tab)
    :emitAction("ToggleInventory")
    :discretely()

bestow.action.builder()
    :duringPhase("game.inventory")
    :whenPressed(k.I)
    :emitAction("ToggleInventory")
    :discretely()

bestow.action.builder()
    :duringPhase("game.inventory")
    :whenPressed(k.Escape)
    :emitAction("ToggleInventory")
    :discretely()

bestow.action.builder()
    :duringPhase("game.inventory")
    :whenPressed(gp.B)
    :emitAction("ToggleInventory")
    :discretely()

--------------------------------------------------
-- Results screen
--------------------------------------------------

bestow.action.builder()
    :duringPhase("results")
    :whenPressed(k.Return)
    :emitAction("ContinueFromResults")
    :discretely()

bestow.action.builder()
    :duringPhase("results")
    :whenPressed(gp.A)
    :emitAction("ContinueFromResults")
    :discretely()

bestow.action.builder()
    :duringPhase("results")
    :whenPressed(k.Escape)
    :emitAction("QuitToMenu")
    :discretely()

bestow.action.builder()
    :duringPhase("results")
    :whenPressed(gp.B)
    :emitAction("QuitToMenu")
    :discretely()

end -- return function()

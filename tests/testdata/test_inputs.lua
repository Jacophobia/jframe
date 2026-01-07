-- tests/testdata/test_inputs.lua
-- Lua integration tests for event-driven input system
-- This file tests ActionBuilder, phase management, and platform-agnostic input API

local tests = {}
local passed = 0
local failed = 0

local function assert_true(condition, message)
    if condition then
        passed = passed + 1
        return true
    else
        failed = failed + 1
        print("FAIL: " .. (message or "assertion failed"))
        return false
    end
end

local function assert_false(condition, message)
    return assert_true(not condition, message)
end

local function assert_equal(expected, actual, message)
    if expected == actual then
        passed = passed + 1
        return true
    else
        failed = failed + 1
        print("FAIL: " .. (message or "expected " .. tostring(expected) .. " but got " .. tostring(actual)))
        return false
    end
end

local function assert_not_nil(value, message)
    return assert_true(value ~= nil, message or "expected non-nil value")
end

local function test(name, fn)
    local ok, err = pcall(fn)
    if not ok then
        failed = failed + 1
        print("ERROR in " .. name .. ": " .. tostring(err))
    else
        print("PASS: " .. name)
    end
end

--=============================================================================
-- Platform-Agnostic Input Constants Tests
--=============================================================================

test("KeyCode enum exists", function()
    assert_not_nil(bestow.input.keys, "bestow.input.keys should exist")
    assert_not_nil(bestow.input.keys.Space, "bestow.input.keys.Space should exist")
    assert_not_nil(bestow.input.keys.A, "bestow.input.keys.A should exist")
    assert_not_nil(bestow.input.keys.Enter, "bestow.input.keys.Enter should exist")
    assert_not_nil(bestow.input.keys.Escape, "bestow.input.keys.Escape should exist")
end)

test("GamepadButton enum exists", function()
    assert_not_nil(bestow.input.buttons, "bestow.input.buttons should exist")
    assert_not_nil(bestow.input.buttons.A, "bestow.input.buttons.A should exist")
    assert_not_nil(bestow.input.buttons.B, "bestow.input.buttons.B should exist")
    assert_not_nil(bestow.input.buttons.X, "bestow.input.buttons.X should exist")
    assert_not_nil(bestow.input.buttons.Y, "bestow.input.buttons.Y should exist")
    assert_not_nil(bestow.input.buttons.Start, "bestow.input.buttons.Start should exist")
    assert_not_nil(bestow.input.buttons.Back, "bestow.input.buttons.Back should exist")
end)

test("GamepadAxis enum exists", function()
    assert_not_nil(bestow.input.axes, "bestow.input.axes should exist")
    assert_not_nil(bestow.input.axes.LeftX, "bestow.input.axes.LeftX should exist")
    assert_not_nil(bestow.input.axes.LeftY, "bestow.input.axes.LeftY should exist")
    assert_not_nil(bestow.input.axes.RightX, "bestow.input.axes.RightX should exist")
    assert_not_nil(bestow.input.axes.RightY, "bestow.input.axes.RightY should exist")
    assert_not_nil(bestow.input.axes.LeftTrigger, "bestow.input.axes.LeftTrigger should exist")
    assert_not_nil(bestow.input.axes.RightTrigger, "bestow.input.axes.RightTrigger should exist")
end)

test("MouseButton enum exists", function()
    assert_not_nil(bestow.input.mouse, "bestow.input.mouse should exist")
    assert_not_nil(bestow.input.mouse.Left, "bestow.input.mouse.Left should exist")
    assert_not_nil(bestow.input.mouse.Right, "bestow.input.mouse.Right should exist")
    assert_not_nil(bestow.input.mouse.Middle, "bestow.input.mouse.Middle should exist")
end)

test("Button aliases exist", function()
    assert_not_nil(bestow.input.buttons.LB, "LB alias should exist")
    assert_not_nil(bestow.input.buttons.RB, "RB alias should exist")
    assert_not_nil(bestow.input.buttons.LT, "LT alias should exist")
    assert_not_nil(bestow.input.buttons.RT, "RT alias should exist")
end)

test("Axis aliases exist", function()
    assert_not_nil(bestow.input.axes.LT, "LT axis alias should exist")
    assert_not_nil(bestow.input.axes.RT, "RT axis alias should exist")
end)

--=============================================================================
-- Phase Management Tests
--=============================================================================

test("Phase table exists", function()
    assert_not_nil(bestow.phase, "bestow.phase should exist")
    assert_not_nil(bestow.phase.current, "bestow.phase.current should exist")
    assert_not_nil(bestow.phase.push, "bestow.phase.push should exist")
    assert_not_nil(bestow.phase.pop, "bestow.phase.pop should exist")
    assert_not_nil(bestow.phase.change, "bestow.phase.change should exist")
    assert_not_nil(bestow.phase.isActive, "bestow.phase.isActive should exist")
end)

test("Change phase", function()
    bestow.phase.change("test_game")
    assert_equal("test_game", bestow.phase.current(), "Phase should be test_game")
end)

test("Push phase", function()
    bestow.phase.change("game")
    bestow.phase.push("game.melee")
    assert_equal("game.melee", bestow.phase.current(), "Current phase should be game.melee")

    local stack = bestow.phase.stack()
    assert_equal(2, #stack, "Phase stack should have 2 entries")
end)

test("Pop phase", function()
    bestow.phase.change("game")
    bestow.phase.push("game.melee")
    bestow.phase.pop()
    assert_equal("game", bestow.phase.current(), "Current phase should be game after pop")
end)

test("Is phase active - current phase", function()
    bestow.phase.change("game.melee")
    assert_true(bestow.phase.isActive("game.melee"), "game.melee should be active")
end)

test("Is phase active - ancestor phase", function()
    bestow.phase.change("game.melee.combo")
    assert_true(bestow.phase.isActive("game.melee"), "game.melee should be active (ancestor)")
    assert_true(bestow.phase.isActive("game"), "game should be active (ancestor)")
end)

test("Is phase active - inactive phase", function()
    bestow.phase.change("game")
    assert_false(bestow.phase.isActive("menu"), "menu should not be active")
    assert_false(bestow.phase.isActive("game.melee"), "game.melee should not be active")
end)

--=============================================================================
-- ActionBuilder Tests
--=============================================================================

test("ActionBuilder exists", function()
    assert_not_nil(bestow.action, "bestow.action should exist")
    assert_not_nil(bestow.action.builder, "bestow.action.builder should exist")
end)

test("ActionBuilder basic creation", function()
    local builder = bestow.action.builder()
    assert_not_nil(builder, "builder should not be nil")
end)

test("ActionBuilder fluent API - duringPhase", function()
    local builder = bestow.action.builder()
    local result = builder:duringPhase("game")
    assert_equal(builder, result, "duringPhase should return builder for chaining")
end)

test("ActionBuilder fluent API - whenPressed", function()
    local builder = bestow.action.builder()
    local result = builder:whenPressed(bestow.input.keys.Space)
    assert_equal(builder, result, "whenPressed should return builder for chaining")
end)

test("ActionBuilder fluent API - whenReleased", function()
    local builder = bestow.action.builder()
    local result = builder:whenReleased(bestow.input.keys.Space)
    assert_equal(builder, result, "whenReleased should return builder for chaining")
end)

test("ActionBuilder fluent API - whenActive", function()
    local builder = bestow.action.builder()
    local result = builder:whenActive(bestow.input.keys.A)
    assert_equal(builder, result, "whenActive should return builder for chaining")
end)

test("ActionBuilder fluent API - whenInactive", function()
    local builder = bestow.action.builder()
    local result = builder:whenInactive(bestow.input.keys.A)
    assert_equal(builder, result, "whenInactive should return builder for chaining")
end)

test("ActionBuilder fluent API - whenHeld without threshold", function()
    local builder = bestow.action.builder()
    local result = builder:whenHeld(bestow.input.keys.Space)
    assert_equal(builder, result, "whenHeld should return builder for chaining")
end)

test("ActionBuilder fluent API - whenHeld with threshold", function()
    local builder = bestow.action.builder()
    local result = builder:whenHeld(bestow.input.keys.Space, 0.5)
    assert_equal(builder, result, "whenHeld with threshold should return builder for chaining")
end)

test("ActionBuilder fluent API - withDeadzone", function()
    local builder = bestow.action.builder()
    local result = builder:withDeadzone(0.15)
    assert_equal(builder, result, "withDeadzone should return builder for chaining")
end)

test("ActionBuilder fluent API - emitAction", function()
    local builder = bestow.action.builder()
    local result = builder:emitAction("Jump")
    assert_equal(builder, result, "emitAction should return builder for chaining")
end)

test("ActionBuilder fluent API - pushPhase", function()
    local builder = bestow.action.builder()
    local result = builder:pushPhase("game.melee")
    assert_equal(builder, result, "pushPhase should return builder for chaining")
end)

test("ActionBuilder fluent API - popPhase", function()
    local builder = bestow.action.builder()
    local result = builder:popPhase()
    assert_equal(builder, result, "popPhase should return builder for chaining")
end)

test("ActionBuilder fluent API - changePhase", function()
    local builder = bestow.action.builder()
    local result = builder:changePhase("menu")
    assert_equal(builder, result, "changePhase should return builder for chaining")
end)

test("ActionBuilder complete registration - discretely", function()
    -- This should complete without error
    bestow.action.builder()
        :duringPhase("test_discrete")
        :whenPressed(bestow.input.keys.J)
        :emitAction("TestJump")
        :discretely()
    passed = passed + 1
end)

test("ActionBuilder complete registration - continuously", function()
    -- This should complete without error
    bestow.action.builder()
        :duringPhase("test_continuous")
        :whenActive(bestow.input.keys.D)
        :emitAction("TestMove")
        :continuously()
    passed = passed + 1
end)

test("ActionBuilder with keyboard input", function()
    bestow.action.builder()
        :duringPhase("test_keyboard")
        :whenPressed(bestow.input.keys.Space)
        :emitAction("KeyboardJump")
        :discretely()
    passed = passed + 1
end)

test("ActionBuilder with gamepad button", function()
    bestow.action.builder()
        :duringPhase("test_gamepad")
        :whenPressed(bestow.input.buttons.A)
        :emitAction("GamepadJump")
        :discretely()
    passed = passed + 1
end)

test("ActionBuilder with mouse button", function()
    bestow.action.builder()
        :duringPhase("test_mouse")
        :whenPressed(bestow.input.mouse.Left)
        :emitAction("MouseClick")
        :discretely()
    passed = passed + 1
end)

test("ActionBuilder with multiple conditions", function()
    bestow.action.builder()
        :duringPhase("test_multi")
        :whenActive(bestow.input.keys.LeftShift)
        :whenPressed(bestow.input.keys.Space)
        :emitAction("SuperJump")
        :discretely()
    passed = passed + 1
end)

test("ActionBuilder with phase effect", function()
    bestow.action.builder()
        :duringPhase("test_phase_effect")
        :whenPressed(bestow.input.keys.Escape)
        :pushPhase("test_pause")
        :discretely()
    passed = passed + 1
end)

--=============================================================================
-- InputBinding Tests
--=============================================================================

test("InputBinding factory methods exist", function()
    assert_not_nil(InputBinding, "InputBinding should exist")
    assert_not_nil(InputBinding.key, "InputBinding.key should exist")
    assert_not_nil(InputBinding.mouseButton, "InputBinding.mouseButton should exist")
    assert_not_nil(InputBinding.gamepadButton, "InputBinding.gamepadButton should exist")
    assert_not_nil(InputBinding.gamepadAxis, "InputBinding.gamepadAxis should exist")
end)

test("InputBinding.key creates keyboard binding", function()
    local binding = InputBinding.key(bestow.input.keys.A)
    assert_not_nil(binding, "binding should not be nil")
    assert_equal(InputSource.Keyboard, binding.source, "source should be Keyboard")
end)

test("InputBinding.mouseButton creates mouse binding", function()
    local binding = InputBinding.mouseButton(bestow.input.mouse.Left)
    assert_not_nil(binding, "binding should not be nil")
    assert_equal(InputSource.Mouse, binding.source, "source should be Mouse")
end)

test("InputBinding.gamepadButton creates gamepad binding", function()
    local binding = InputBinding.gamepadButton(bestow.input.buttons.A)
    assert_not_nil(binding, "binding should not be nil")
    assert_equal(InputSource.Gamepad, binding.source, "source should be Gamepad")
end)

test("InputBinding.gamepadAxis creates axis binding", function()
    local binding = InputBinding.gamepadAxis(bestow.input.axes.LeftX)
    assert_not_nil(binding, "binding should not be nil")
    assert_equal(InputSource.Gamepad, binding.source, "source should be Gamepad")
end)

test("InputBinding isAxis and isButton", function()
    local keyBinding = InputBinding.key(bestow.input.keys.A)
    local axisBinding = InputBinding.gamepadAxis(bestow.input.axes.LeftX)

    assert_true(keyBinding:isButton(), "key binding should be button")
    assert_false(keyBinding:isAxis(), "key binding should not be axis")
    assert_true(axisBinding:isAxis(), "axis binding should be axis")
    assert_false(axisBinding:isButton(), "axis binding should not be button")
end)

--=============================================================================
-- Input State Query Tests
--=============================================================================

test("Input state query functions exist", function()
    assert_not_nil(bestow.input.isKeyDown, "isKeyDown should exist")
    assert_not_nil(bestow.input.wasKeyJustPressed, "wasKeyJustPressed should exist")
    assert_not_nil(bestow.input.wasKeyJustReleased, "wasKeyJustReleased should exist")
    assert_not_nil(bestow.input.isMouseButtonDown, "isMouseButtonDown should exist")
    assert_not_nil(bestow.input.isGamepadButtonDown, "isGamepadButtonDown should exist")
    assert_not_nil(bestow.input.getGamepadAxisValue, "getGamepadAxisValue should exist")
    assert_not_nil(bestow.input.getLeftStick, "getLeftStick should exist")
    assert_not_nil(bestow.input.getRightStick, "getRightStick should exist")
end)

test("isKeyDown with KeyCode enum", function()
    -- Without actual key press, should return false
    local result = bestow.input.isKeyDown(bestow.input.keys.Space)
    assert_false(result, "Space key should not be pressed")
end)

test("isMouseButtonDown with MouseButton enum", function()
    local result = bestow.input.isMouseButtonDown(bestow.input.mouse.Left)
    assert_false(result, "Left mouse button should not be pressed")
end)

test("isGamepadButtonDown with GamepadButton enum", function()
    local result = bestow.input.isGamepadButtonDown(bestow.input.buttons.A)
    assert_false(result, "Gamepad A button should not be pressed")
end)

test("getGamepadAxisValue with GamepadAxis enum", function()
    local result = bestow.input.getGamepadAxisValue(bestow.input.axes.LeftX)
    assert_equal(0.0, result, "Left stick X should be 0 without controller")
end)

test("getLeftStick returns Vec2", function()
    local stick = bestow.input.getLeftStick()
    assert_not_nil(stick, "getLeftStick should return a value")
    assert_equal(0.0, stick.x, "Left stick X should be 0")
    assert_equal(0.0, stick.y, "Left stick Y should be 0")
end)

test("getRightStick returns Vec2", function()
    local stick = bestow.input.getRightStick()
    assert_not_nil(stick, "getRightStick should return a value")
    assert_equal(0.0, stick.x, "Right stick X should be 0")
    assert_equal(0.0, stick.y, "Right stick Y should be 0")
end)

--=============================================================================
-- Hold Threshold Tests
--=============================================================================

test("Default hold threshold", function()
    local threshold = bestow.input.getDefaultHoldThreshold()
    assert_equal(0.5, threshold, "Default hold threshold should be 0.5 seconds")
end)

test("Set default hold threshold", function()
    bestow.input.setDefaultHoldThreshold(0.75)
    local threshold = bestow.input.getDefaultHoldThreshold()
    assert_equal(0.75, threshold, "Hold threshold should be 0.75 after setting")

    -- Reset to default
    bestow.input.setDefaultHoldThreshold(0.5)
end)

--=============================================================================
-- Print Results
--=============================================================================

print("")
print("===================================")
print("Test Results: " .. passed .. " passed, " .. failed .. " failed")
print("===================================")

-- Return results for test runner
return {
    passed = passed,
    failed = failed,
    total = passed + failed
}

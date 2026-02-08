// bestow-luabind/src/docs/action_binding_doc.cpp
// API documentation for bestow.action and bestow.phase

module bestow.luabind;

import std;

namespace bestow {

void registerActionDoc(DocRegistry& registry) {
    SystemDoc sys;
    sys.name = "action";
    sys.qualifiedName = "bestow.action";
    sys.description = "Event-driven input system using ActionBuilder for declarative input binding. Actions are registered with a fluent API that specifies phase, conditions, effects, and terminal mode.";

    // --- Enums ---

    sys.enums.push_back(EnumDoc{
        .name = "KeyCode",
        .qualifiedName = "KeyCode",
        .description = "Platform-agnostic keyboard key codes. Used with ActionBuilder conditions and direct key queries.",
        .values = {
            {"A", "Letter A"}, {"B", "Letter B"}, {"C", "Letter C"}, {"D", "Letter D"},
            {"E", "Letter E"}, {"F", "Letter F"}, {"G", "Letter G"}, {"H", "Letter H"},
            {"I", "Letter I"}, {"J", "Letter J"}, {"K", "Letter K"}, {"L", "Letter L"},
            {"M", "Letter M"}, {"N", "Letter N"}, {"O", "Letter O"}, {"P", "Letter P"},
            {"Q", "Letter Q"}, {"R", "Letter R"}, {"S", "Letter S"}, {"T", "Letter T"},
            {"U", "Letter U"}, {"V", "Letter V"}, {"W", "Letter W"}, {"X", "Letter X"},
            {"Y", "Letter Y"}, {"Z", "Letter Z"},
            {"Num0", "Number 0"}, {"Num1", "Number 1"}, {"Num2", "Number 2"},
            {"Num3", "Number 3"}, {"Num4", "Number 4"}, {"Num5", "Number 5"},
            {"Num6", "Number 6"}, {"Num7", "Number 7"}, {"Num8", "Number 8"}, {"Num9", "Number 9"},
            {"F1", "Function key F1"}, {"F2", "Function key F2"}, {"F3", "Function key F3"},
            {"F4", "Function key F4"}, {"F5", "Function key F5"}, {"F6", "Function key F6"},
            {"F7", "Function key F7"}, {"F8", "Function key F8"}, {"F9", "Function key F9"},
            {"F10", "Function key F10"}, {"F11", "Function key F11"}, {"F12", "Function key F12"},
            {"Space", "Spacebar"}, {"Enter", "Enter/Return"}, {"Escape", "Escape key"},
            {"Tab", "Tab key"}, {"Backspace", "Backspace key"}, {"Delete", "Delete key"},
            {"Insert", "Insert key"},
            {"Up", "Arrow Up"}, {"Down", "Arrow Down"}, {"Left", "Arrow Left"}, {"Right", "Arrow Right"},
            {"Home", "Home key"}, {"End", "End key"}, {"PageUp", "Page Up"}, {"PageDown", "Page Down"},
            {"LeftShift", "Left Shift"}, {"RightShift", "Right Shift"},
            {"LeftCtrl", "Left Control"}, {"RightCtrl", "Right Control"},
            {"LeftAlt", "Left Alt/Option"}, {"RightAlt", "Right Alt/Option"},
            {"LeftSuper", "Left Super/Command"}, {"RightSuper", "Right Super/Command"},
            {"Apostrophe", "Apostrophe (')"}, {"Comma", "Comma (,)"},
            {"Minus", "Minus (-)"}, {"Period", "Period (.)"},
            {"Slash", "Forward slash (/)"}, {"Semicolon", "Semicolon (;)"},
            {"Equal", "Equal sign (=)"}, {"LeftBracket", "Left bracket ([)"},
            {"RightBracket", "Right bracket (])"}, {"Backslash", "Backslash (\\)"},
            {"GraveAccent", "Grave accent (`)"},
            {"KP0", "Numpad 0"}, {"KP1", "Numpad 1"}, {"KP2", "Numpad 2"},
            {"KP3", "Numpad 3"}, {"KP4", "Numpad 4"}, {"KP5", "Numpad 5"},
            {"KP6", "Numpad 6"}, {"KP7", "Numpad 7"}, {"KP8", "Numpad 8"}, {"KP9", "Numpad 9"},
            {"KPDecimal", "Numpad decimal"}, {"KPDivide", "Numpad divide"},
            {"KPMultiply", "Numpad multiply"}, {"KPSubtract", "Numpad subtract"},
            {"KPAdd", "Numpad add"}, {"KPEnter", "Numpad enter"}, {"KPEqual", "Numpad equal"},
            {"CapsLock", "Caps Lock"}, {"ScrollLock", "Scroll Lock"},
            {"NumLock", "Num Lock"}, {"PrintScreen", "Print Screen"}, {"Pause", "Pause/Break"},
        }
    });

    sys.enums.push_back(EnumDoc{
        .name = "MouseButton",
        .qualifiedName = "MouseButton",
        .description = "Mouse button identifiers.",
        .values = {
            {"Left", "Left mouse button"},
            {"Right", "Right mouse button"},
            {"Middle", "Middle mouse button (scroll wheel click)"},
            {"Button4", "Extra mouse button 4"},
            {"Button5", "Extra mouse button 5"},
            {"Button6", "Extra mouse button 6"},
            {"Button7", "Extra mouse button 7"},
            {"Button8", "Extra mouse button 8"},
        }
    });

    sys.enums.push_back(EnumDoc{
        .name = "GamepadButton",
        .qualifiedName = "GamepadButton",
        .description = "Gamepad button identifiers (Xbox-style layout).",
        .values = {
            {"A", "A button (cross on PlayStation)"},
            {"B", "B button (circle on PlayStation)"},
            {"X", "X button (square on PlayStation)"},
            {"Y", "Y button (triangle on PlayStation)"},
            {"LeftBumper", "Left bumper (LB/L1)"},
            {"RightBumper", "Right bumper (RB/R1)"},
            {"Back", "Back/Select button"},
            {"Start", "Start/Options button"},
            {"Guide", "Guide/Home button"},
            {"LeftThumb", "Left stick click (L3)"},
            {"RightThumb", "Right stick click (R3)"},
            {"DPadUp", "D-pad up"},
            {"DPadDown", "D-pad down"},
            {"DPadLeft", "D-pad left"},
            {"DPadRight", "D-pad right"},
            {"LeftTrigger", "Left trigger as button (LT/L2)"},
            {"RightTrigger", "Right trigger as button (RT/R2)"},
        }
    });

    sys.enums.push_back(EnumDoc{
        .name = "GamepadAxis",
        .qualifiedName = "GamepadAxis",
        .description = "Gamepad analog axis identifiers.",
        .values = {
            {"LeftX", "Left stick horizontal axis"},
            {"LeftY", "Left stick vertical axis"},
            {"RightX", "Right stick horizontal axis"},
            {"RightY", "Right stick vertical axis"},
            {"LeftTrigger", "Left trigger analog axis (0.0 to 1.0)"},
            {"RightTrigger", "Right trigger analog axis (0.0 to 1.0)"},
        }
    });

    sys.enums.push_back(EnumDoc{
        .name = "InputState",
        .qualifiedName = "InputState",
        .description = "States an input can be in during its lifecycle.",
        .values = {
            {"NotPressed", "Input is not active"},
            {"JustPressed", "Input was pressed this frame"},
            {"Pressed", "Input is being held (after JustPressed)"},
            {"Held", "Input has been held past the hold threshold"},
            {"JustReleased", "Input was released this frame"},
        }
    });

    sys.enums.push_back(EnumDoc{
        .name = "InputSource",
        .qualifiedName = "InputSource",
        .description = "Source device of an input binding.",
        .values = {
            {"Keyboard", "Keyboard input"},
            {"Mouse", "Mouse input"},
            {"Gamepad", "Gamepad/controller input"},
        }
    });

    // --- Types ---

    sys.types.push_back(TypeDoc{
        .name = "InputBinding",
        .qualifiedName = "InputBinding",
        .description = "Describes a specific input source (key, mouse button, gamepad button, or axis). Created via factory methods or by passing KeyCode/MouseButton/GamepadButton/GamepadAxis directly to ActionBuilder conditions.",
        .fields = {
            {"source", "InputSource", "The device type of this binding"},
            {"deviceIndex", "number", "Device index (for multiple gamepads)"},
            {"requiredModifiers", "ModifierKey", "Modifier keys that must be held"},
            {"scale", "number", "Scale factor for axis values"},
            {"deadzone", "number", "Deadzone for axis inputs"},
        },
        .methods = {
            {.name = "isAxis", .qualifiedName = "InputBinding:isAxis",
             .description = "Check if this binding is an analog axis.",
             .returns = {{.type = "boolean", .description = "true if axis input"}}},
            {.name = "isButton", .qualifiedName = "InputBinding:isButton",
             .description = "Check if this binding is a digital button.",
             .returns = {{.type = "boolean", .description = "true if button input"}}},
            {.name = "key", .qualifiedName = "InputBinding.key",
             .description = "Create a keyboard key binding.",
             .params = {
                 {.name = "key", .type = "KeyCode", .description = "The key code"},
                 {.name = "modifiers", .type = "ModifierKey", .description = "Required modifier keys", .optional = true},
             },
             .returns = {{.type = "InputBinding", .description = "New key binding"}}},
            {.name = "mouseButton", .qualifiedName = "InputBinding.mouseButton",
             .description = "Create a mouse button binding.",
             .params = {
                 {.name = "button", .type = "MouseButton", .description = "The mouse button"},
             },
             .returns = {{.type = "InputBinding", .description = "New mouse button binding"}}},
            {.name = "gamepadButton", .qualifiedName = "InputBinding.gamepadButton",
             .description = "Create a gamepad button binding.",
             .params = {
                 {.name = "button", .type = "GamepadButton", .description = "The gamepad button"},
                 {.name = "index", .type = "number", .description = "Gamepad index", .optional = true, .defaultVal = "0"},
             },
             .returns = {{.type = "InputBinding", .description = "New gamepad button binding"}}},
            {.name = "gamepadAxis", .qualifiedName = "InputBinding.gamepadAxis",
             .description = "Create a gamepad axis binding.",
             .params = {
                 {.name = "axis", .type = "GamepadAxis", .description = "The gamepad axis"},
                 {.name = "index", .type = "number", .description = "Gamepad index", .optional = true, .defaultVal = "0"},
                 {.name = "scale", .type = "number", .description = "Scale factor", .optional = true, .defaultVal = "1.0"},
                 {.name = "deadzone", .type = "number", .description = "Deadzone threshold", .optional = true, .defaultVal = "0.15"},
             },
             .returns = {{.type = "InputBinding", .description = "New gamepad axis binding"}}},
        },
        .example = "-- Use directly in ActionBuilder:\nbestow.action.builder()\n  :duringPhase(\"gameplay\")\n  :whenPressed(KeyCode.Space)\n  :emitAction(\"Jump\")\n  :discretely()\n\n-- Or create explicit bindings:\nlocal binding = InputBinding.key(KeyCode.A, ModifierKey.Shift)",
    });

    sys.types.push_back(TypeDoc{
        .name = "ActionBuilder",
        .qualifiedName = "ActionBuilder",
        .description = "Fluent builder for registering event-driven input actions. Created via bestow.action.builder(). Chain methods to define phase, conditions, effects, and terminal mode.",
        .methods = {
            {.name = "duringPhase", .qualifiedName = "ActionBuilder:duringPhase",
             .description = "Set the phase this action is active during. Required.",
             .params = {{.name = "phase", .type = "string", .description = "Phase name (e.g., 'gameplay', 'menu')"}},
             .returns = {{.type = "ActionBuilder", .description = "self for chaining"}}},
            {.name = "whenPressed", .qualifiedName = "ActionBuilder:whenPressed",
             .description = "Add a condition that fires on the frame an input is first pressed.",
             .params = {{.name = "input", .type = "KeyCode|MouseButton|GamepadButton|InputBinding", .description = "The input to watch"}},
             .returns = {{.type = "ActionBuilder", .description = "self for chaining"}}},
            {.name = "whenReleased", .qualifiedName = "ActionBuilder:whenReleased",
             .description = "Add a condition that fires on the frame an input is released.",
             .params = {{.name = "input", .type = "KeyCode|MouseButton|GamepadButton|InputBinding", .description = "The input to watch"}},
             .returns = {{.type = "ActionBuilder", .description = "self for chaining"}}},
            {.name = "whenActive", .qualifiedName = "ActionBuilder:whenActive",
             .description = "Add a condition that is true while an input is pressed, held, or just pressed.",
             .params = {{.name = "input", .type = "KeyCode|MouseButton|GamepadButton|GamepadAxis|InputBinding", .description = "The input to watch"}},
             .returns = {{.type = "ActionBuilder", .description = "self for chaining"}}},
            {.name = "whenInactive", .qualifiedName = "ActionBuilder:whenInactive",
             .description = "Add a condition that is true while an input is not pressed or just released.",
             .params = {{.name = "input", .type = "KeyCode|MouseButton|GamepadButton|GamepadAxis|InputBinding", .description = "The input to watch"}},
             .returns = {{.type = "ActionBuilder", .description = "self for chaining"}}},
            {.name = "whenHeld", .qualifiedName = "ActionBuilder:whenHeld",
             .description = "Add a condition that fires when an input transitions to the Held state (past the hold threshold).",
             .params = {
                 {.name = "input", .type = "KeyCode|MouseButton|GamepadButton|InputBinding", .description = "The input to watch"},
                 {.name = "threshold", .type = "number", .description = "Hold threshold in seconds (uses default if omitted)", .optional = true},
             },
             .returns = {{.type = "ActionBuilder", .description = "self for chaining"}}},
            {.name = "withDeadzone", .qualifiedName = "ActionBuilder:withDeadzone",
             .description = "Set the deadzone for axis inputs (0.0 to 1.0). Only valid when conditions include axis inputs.",
             .params = {{.name = "deadzone", .type = "number", .description = "Deadzone threshold (0.0-1.0)"}},
             .returns = {{.type = "ActionBuilder", .description = "self for chaining"}}},
            {.name = "emitAction", .qualifiedName = "ActionBuilder:emitAction",
             .description = "Add an effect that emits a named action event when conditions are met.",
             .params = {{.name = "actionName", .type = "string", .description = "Name of the action event to emit"}},
             .returns = {{.type = "ActionBuilder", .description = "self for chaining"}}},
            {.name = "pushPhase", .qualifiedName = "ActionBuilder:pushPhase",
             .description = "Add an effect that pushes a phase onto the phase stack.",
             .params = {{.name = "phase", .type = "string", .description = "Phase name to push"}},
             .returns = {{.type = "ActionBuilder", .description = "self for chaining"}}},
            {.name = "popPhase", .qualifiedName = "ActionBuilder:popPhase",
             .description = "Add an effect that pops the current phase from the stack.",
             .returns = {{.type = "ActionBuilder", .description = "self for chaining"}}},
            {.name = "changePhase", .qualifiedName = "ActionBuilder:changePhase",
             .description = "Add an effect that replaces the entire phase stack with a new phase.",
             .params = {{.name = "phase", .type = "string", .description = "Phase to switch to"}},
             .returns = {{.type = "ActionBuilder", .description = "self for chaining"}}},
            {.name = "discretely", .qualifiedName = "ActionBuilder:discretely",
             .description = "Finalize and register the action as discrete (fires once when conditions transition to true). This is a terminal method.",
            },
            {.name = "continuously", .qualifiedName = "ActionBuilder:continuously",
             .description = "Finalize and register the action as continuous (fires every frame while conditions are true). This is a terminal method.",
            },
        },
        .example = "bestow.action.builder()\n  :duringPhase(\"gameplay\")\n  :whenPressed(bestow.input.keys.Space)\n  :emitAction(\"Jump\")\n  :discretely()\n\nbestow.action.builder()\n  :duringPhase(\"gameplay\")\n  :whenActive(bestow.input.axes.LeftX)\n  :withDeadzone(0.2)\n  :emitAction(\"MoveHorizontal\")\n  :continuously()",
    });

    // --- Methods ---

    sys.methods.push_back(MethodDoc{
        .name = "builder",
        .qualifiedName = "bestow.action.builder",
        .description = "Create a new ActionBuilder for registering an event-driven input action.",
        .returns = {{.type = "ActionBuilder", .description = "New builder instance for fluent configuration"}},
        .example = "local b = bestow.action.builder()\nb:duringPhase(\"gameplay\")\n :whenPressed(KeyCode.Space)\n :emitAction(\"Jump\")\n :discretely()",
    });

    // --- Phase management (bestow.phase) ---
    // Documented here since it's bound in the same file and tightly related

    sys.methods.push_back(MethodDoc{
        .name = "phase.current",
        .qualifiedName = "bestow.phase.current",
        .description = "Get the name of the current (top) phase on the phase stack.",
        .returns = {{.type = "string", .description = "Current phase name"}},
    });

    sys.methods.push_back(MethodDoc{
        .name = "phase.stack",
        .qualifiedName = "bestow.phase.stack",
        .description = "Get the full phase stack as a table (bottom to top).",
        .returns = {{.type = "string[]", .description = "Array of phase names"}},
    });

    sys.methods.push_back(MethodDoc{
        .name = "phase.push",
        .qualifiedName = "bestow.phase.push",
        .description = "Push a new phase onto the phase stack. The previous phase remains on the stack.",
        .params = {
            {.name = "phase", .type = "string", .description = "Phase name to push"},
        },
        .seeAlso = {"bestow.phase.pop", "bestow.phase.change"},
    });

    sys.methods.push_back(MethodDoc{
        .name = "phase.pop",
        .qualifiedName = "bestow.phase.pop",
        .description = "Pop the current phase from the stack. The previous phase becomes active.",
        .seeAlso = {"bestow.phase.push"},
    });

    sys.methods.push_back(MethodDoc{
        .name = "phase.change",
        .qualifiedName = "bestow.phase.change",
        .description = "Replace the entire phase stack with a single new phase.",
        .params = {
            {.name = "phase", .type = "string", .description = "Phase name to switch to"},
        },
        .seeAlso = {"bestow.phase.push", "bestow.phase.pop"},
    });

    sys.methods.push_back(MethodDoc{
        .name = "phase.isActive",
        .qualifiedName = "bestow.phase.isActive",
        .description = "Check if a phase is currently anywhere on the phase stack.",
        .params = {
            {.name = "phase", .type = "string", .description = "Phase name to check"},
        },
        .returns = {{.type = "boolean", .description = "true if the phase is on the stack"}},
    });

    sys.seeAlso = {"bestow.input", "bestow.phase"};

    registry.addSystem(std::move(sys));
}

} // namespace bestow

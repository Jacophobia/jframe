// bestow-luabind/src/docs/input_binding_doc.cpp
// API documentation for bestow.input (legacy polling API)

module bestow.luabind;

import std;

namespace bestow {

void registerInputDoc(DocRegistry& registry) {
    SystemDoc sys;
    sys.name = "input";
    sys.qualifiedName = "bestow.input";
    sys.description = "Input system providing mouse, keyboard, and controller state queries. Includes the legacy polling API (deprecated) and current platform-agnostic state queries. For event-driven input, see bestow.action.";

    // --- Enums ---

    sys.enums.push_back(EnumDoc{
        .name = "InputDeviceType",
        .qualifiedName = "InputDeviceType",
        .description = "Type of input device. Deprecated: use InputSource from bestow.action instead.",
        .values = {
            {"Keyboard", "Keyboard input device"},
            {"Mouse", "Mouse input device"},
            {"Controller", "Game controller input device"},
        }
    });

    sys.enums.push_back(EnumDoc{
        .name = "ModifierKey",
        .qualifiedName = "ModifierKey",
        .description = "Modifier key bitmask flags for key combinations.",
        .values = {
            {"None", "No modifier key"},
            {"Shift", "Shift key"},
            {"Ctrl", "Control key"},
            {"Alt", "Alt/Option key"},
            {"Super", "Super/Command/Windows key"},
            {"CapsLock", "Caps Lock active"},
            {"NumLock", "Num Lock active"},
        }
    });

    sys.enums.push_back(EnumDoc{
        .name = "CursorMode",
        .qualifiedName = "CursorMode",
        .description = "Cursor visibility and capture modes.",
        .values = {
            {"Normal", "Cursor is visible and moves freely"},
            {"Hidden", "Cursor is hidden but still tracks movement"},
            {"Disabled", "Cursor is hidden and locked to the window (for FPS-style mouse look)"},
        }
    });

    // --- Types ---

    sys.types.push_back(TypeDoc{
        .name = "ActionState",
        .qualifiedName = "ActionState",
        .description = "State of an input action (legacy polling).",
        .fields = {
            {"action", "string", "Name of the action"},
            {"active", "boolean", "Whether the action is currently active"},
            {"value", "number", "Analog value of the action (0.0 to 1.0)"},
            {"justPressed", "boolean", "True only on the frame the action was first pressed"},
            {"justReleased", "boolean", "True only on the frame the action was released"},
        },
    });

    sys.types.push_back(TypeDoc{
        .name = "InputMapping",
        .qualifiedName = "InputMapping",
        .description = "Maps an input binding to an action name (legacy).",
        .fields = {
            {"binding", "InputBinding", "The input binding (key, mouse button, etc.)"},
            {"action", "string", "Name of the action this binding triggers"},
        },
    });

    // --- Deprecated Action State Queries ---

    sys.methods.push_back(MethodDoc{
        .name = "isActionActive",
        .qualifiedName = "bestow.input.isActionActive",
        .description = "Check if a named action is currently active.",
        .params = {
            {.name = "action", .type = "string", .description = "Name of the action to query"},
        },
        .returns = {{.type = "boolean", .description = "true if the action is active"}},
        .deprecated = true,
        .seeAlso = {"bestow.action.builder"},
    });

    sys.methods.push_back(MethodDoc{
        .name = "wasActionJustPressed",
        .qualifiedName = "bestow.input.wasActionJustPressed",
        .description = "Check if a named action was just pressed this frame.",
        .params = {
            {.name = "action", .type = "string", .description = "Name of the action to query"},
        },
        .returns = {{.type = "boolean", .description = "true if just pressed this frame"}},
        .deprecated = true,
        .seeAlso = {"bestow.action.builder"},
    });

    sys.methods.push_back(MethodDoc{
        .name = "wasActionJustReleased",
        .qualifiedName = "bestow.input.wasActionJustReleased",
        .description = "Check if a named action was just released this frame.",
        .params = {
            {.name = "action", .type = "string", .description = "Name of the action to query"},
        },
        .returns = {{.type = "boolean", .description = "true if just released this frame"}},
        .deprecated = true,
        .seeAlso = {"bestow.action.builder"},
    });

    sys.methods.push_back(MethodDoc{
        .name = "getActionValue",
        .qualifiedName = "bestow.input.getActionValue",
        .description = "Get the analog value of a named action (0.0 to 1.0).",
        .params = {
            {.name = "action", .type = "string", .description = "Name of the action to query"},
        },
        .returns = {{.type = "number", .description = "Analog value (0.0 inactive, 1.0 fully active)"}},
        .deprecated = true,
        .seeAlso = {"bestow.action.builder"},
    });

    sys.methods.push_back(MethodDoc{
        .name = "getActionState",
        .qualifiedName = "bestow.input.getActionState",
        .description = "Get the full state struct of a named action.",
        .params = {
            {.name = "action", .type = "string", .description = "Name of the action to query"},
        },
        .returns = {{.type = "ActionState", .description = "Full action state with active, value, justPressed, justReleased"}},
        .deprecated = true,
        .seeAlso = {"bestow.action.builder"},
    });

    sys.methods.push_back(MethodDoc{
        .name = "getAllActionStates",
        .qualifiedName = "bestow.input.getAllActionStates",
        .description = "Get the states of all registered actions.",
        .returns = {{.type = "ActionState[]", .description = "Array of all action states"}},
        .deprecated = true,
    });

    // --- Mouse State ---

    sys.methods.push_back(MethodDoc{
        .name = "getMousePosition",
        .qualifiedName = "bestow.input.getMousePosition",
        .description = "Get the current mouse position in window coordinates.",
        .returns = {{.type = "Vec2", .description = "Mouse position (x, y) in pixels"}},
    });

    sys.methods.push_back(MethodDoc{
        .name = "getMouseDelta",
        .qualifiedName = "bestow.input.getMouseDelta",
        .description = "Get the mouse movement since the last frame.",
        .returns = {{.type = "Vec2", .description = "Mouse delta (dx, dy) in pixels"}},
    });

    sys.methods.push_back(MethodDoc{
        .name = "isMouseButtonDownLegacy",
        .qualifiedName = "bestow.input.isMouseButtonDownLegacy",
        .description = "Check if a mouse button is currently held down (legacy int-based). Use bestow.input.isMouseButtonDown(MouseButton) instead.",
        .params = {
            {.name = "button", .type = "number", .description = "Integer mouse button ID (0=Left, 1=Right, 2=Middle)"},
        },
        .returns = {{.type = "boolean", .description = "true if the button is held down"}},
        .deprecated = true,
        .seeAlso = {"bestow.input.isMouseButtonDown"},
    });

    sys.methods.push_back(MethodDoc{
        .name = "wasMouseButtonJustPressedLegacy",
        .qualifiedName = "bestow.input.wasMouseButtonJustPressedLegacy",
        .description = "Check if a mouse button was just pressed this frame (legacy int-based).",
        .params = {
            {.name = "button", .type = "number", .description = "Integer mouse button ID"},
        },
        .returns = {{.type = "boolean", .description = "true if just pressed this frame"}},
        .deprecated = true,
        .seeAlso = {"bestow.input.wasMouseButtonJustPressed"},
    });

    sys.methods.push_back(MethodDoc{
        .name = "wasMouseButtonJustReleasedLegacy",
        .qualifiedName = "bestow.input.wasMouseButtonJustReleasedLegacy",
        .description = "Check if a mouse button was just released this frame (legacy int-based).",
        .params = {
            {.name = "button", .type = "number", .description = "Integer mouse button ID"},
        },
        .returns = {{.type = "boolean", .description = "true if just released this frame"}},
        .deprecated = true,
        .seeAlso = {"bestow.input.wasMouseButtonJustReleased"},
    });

    sys.methods.push_back(MethodDoc{
        .name = "getScrollDelta",
        .qualifiedName = "bestow.input.getScrollDelta",
        .description = "Get the scroll wheel delta since the last frame.",
        .returns = {{.type = "Vec2", .description = "Scroll delta (horizontal, vertical)"}},
    });

    // --- Cursor Control ---

    sys.methods.push_back(MethodDoc{
        .name = "showMouseCursor",
        .qualifiedName = "bestow.input.showMouseCursor",
        .description = "Make the mouse cursor visible.",
        .seeAlso = {"bestow.input.hideMouseCursor", "bestow.input.setCursorMode"},
    });

    sys.methods.push_back(MethodDoc{
        .name = "hideMouseCursor",
        .qualifiedName = "bestow.input.hideMouseCursor",
        .description = "Hide the mouse cursor.",
        .seeAlso = {"bestow.input.showMouseCursor", "bestow.input.setCursorMode"},
    });

    sys.methods.push_back(MethodDoc{
        .name = "isMouseCursorVisible",
        .qualifiedName = "bestow.input.isMouseCursorVisible",
        .description = "Check if the mouse cursor is currently visible.",
        .returns = {{.type = "boolean", .description = "true if cursor is visible"}},
    });

    sys.methods.push_back(MethodDoc{
        .name = "setCursorMode",
        .qualifiedName = "bestow.input.setCursorMode",
        .description = "Set the cursor mode (Normal, Hidden, or Disabled for mouse-look).",
        .params = {
            {.name = "mode", .type = "CursorMode", .description = "Desired cursor mode"},
        },
        .seeAlso = {"bestow.input.getCursorMode"},
    });

    sys.methods.push_back(MethodDoc{
        .name = "getCursorMode",
        .qualifiedName = "bestow.input.getCursorMode",
        .description = "Get the current cursor mode.",
        .returns = {{.type = "CursorMode", .description = "Current cursor mode"}},
        .seeAlso = {"bestow.input.setCursorMode"},
    });

    // --- Modifier Keys ---

    sys.methods.push_back(MethodDoc{
        .name = "getModifierState",
        .qualifiedName = "bestow.input.getModifierState",
        .description = "Get the current modifier key bitmask state.",
        .returns = {{.type = "ModifierKey", .description = "Bitmask of currently active modifier keys"}},
    });

    sys.methods.push_back(MethodDoc{
        .name = "isModifierPressed",
        .qualifiedName = "bestow.input.isModifierPressed",
        .description = "Check if a specific modifier key is currently pressed.",
        .params = {
            {.name = "mod", .type = "ModifierKey", .description = "The modifier key to check"},
        },
        .returns = {{.type = "boolean", .description = "true if the modifier is pressed"}},
    });

    sys.methods.push_back(MethodDoc{
        .name = "isShiftPressed",
        .qualifiedName = "bestow.input.isShiftPressed",
        .description = "Check if either Shift key is currently pressed.",
        .returns = {{.type = "boolean", .description = "true if Shift is pressed"}},
    });

    sys.methods.push_back(MethodDoc{
        .name = "isCtrlPressed",
        .qualifiedName = "bestow.input.isCtrlPressed",
        .description = "Check if either Ctrl key is currently pressed.",
        .returns = {{.type = "boolean", .description = "true if Ctrl is pressed"}},
    });

    sys.methods.push_back(MethodDoc{
        .name = "isAltPressed",
        .qualifiedName = "bestow.input.isAltPressed",
        .description = "Check if either Alt key is currently pressed.",
        .returns = {{.type = "boolean", .description = "true if Alt is pressed"}},
    });

    sys.methods.push_back(MethodDoc{
        .name = "isSuperPressed",
        .qualifiedName = "bestow.input.isSuperPressed",
        .description = "Check if either Super/Command key is currently pressed.",
        .returns = {{.type = "boolean", .description = "true if Super is pressed"}},
    });

    // --- Legacy Direct Keyboard State ---

    sys.methods.push_back(MethodDoc{
        .name = "isKeyDownLegacy",
        .qualifiedName = "bestow.input.isKeyDownLegacy",
        .description = "Check if a key is held down (legacy int-based GLFW key codes). Use bestow.input.isKeyDown(KeyCode) instead.",
        .params = {
            {.name = "keyCode", .type = "number", .description = "Integer GLFW key code"},
        },
        .returns = {{.type = "boolean", .description = "true if the key is held down"}},
        .deprecated = true,
        .seeAlso = {"bestow.input.isKeyDown"},
    });

    sys.methods.push_back(MethodDoc{
        .name = "wasKeyJustPressedLegacy",
        .qualifiedName = "bestow.input.wasKeyJustPressedLegacy",
        .description = "Check if a key was just pressed this frame (legacy int-based).",
        .params = {
            {.name = "keyCode", .type = "number", .description = "Integer GLFW key code"},
        },
        .returns = {{.type = "boolean", .description = "true if just pressed this frame"}},
        .deprecated = true,
        .seeAlso = {"bestow.input.wasKeyJustPressed"},
    });

    sys.methods.push_back(MethodDoc{
        .name = "wasKeyJustReleasedLegacy",
        .qualifiedName = "bestow.input.wasKeyJustReleasedLegacy",
        .description = "Check if a key was just released this frame (legacy int-based).",
        .params = {
            {.name = "keyCode", .type = "number", .description = "Integer GLFW key code"},
        },
        .returns = {{.type = "boolean", .description = "true if just released this frame"}},
        .deprecated = true,
        .seeAlso = {"bestow.input.wasKeyJustReleased"},
    });

    // --- Legacy Mapping Management ---

    sys.methods.push_back(MethodDoc{
        .name = "registerMapping",
        .qualifiedName = "bestow.input.registerMapping",
        .description = "Register an input mapping (legacy). Use ActionBuilder instead.",
        .params = {
            {.name = "mapping", .type = "InputMapping", .description = "The input mapping to register"},
        },
        .deprecated = true,
        .seeAlso = {"bestow.action.builder"},
    });

    sys.methods.push_back(MethodDoc{
        .name = "removeMapping",
        .qualifiedName = "bestow.input.removeMapping",
        .description = "Remove an input mapping by its binding (legacy).",
        .params = {
            {.name = "binding", .type = "InputBinding", .description = "The binding to remove"},
        },
        .deprecated = true,
    });

    sys.methods.push_back(MethodDoc{
        .name = "clearMappings",
        .qualifiedName = "bestow.input.clearMappings",
        .description = "Remove all registered input mappings (legacy).",
        .deprecated = true,
    });

    sys.methods.push_back(MethodDoc{
        .name = "getMappings",
        .qualifiedName = "bestow.input.getMappings",
        .description = "Get all currently registered input mappings (legacy).",
        .returns = {{.type = "InputMapping[]", .description = "Array of all input mappings"}},
        .deprecated = true,
    });

    // --- Raw Input (for Rebinding UI) ---

    sys.methods.push_back(MethodDoc{
        .name = "getLastInput",
        .qualifiedName = "bestow.input.getLastInput",
        .description = "Get the last raw input event, or nil if none. Useful for building key-rebinding UIs.",
        .returns = {{.type = "InputBinding|nil", .description = "The last input binding, or nil"}},
        .seeAlso = {"bestow.input.startListeningForInput"},
    });

    sys.methods.push_back(MethodDoc{
        .name = "isListeningForInput",
        .qualifiedName = "bestow.input.isListeningForInput",
        .description = "Check if the system is currently listening for raw input (rebinding mode).",
        .returns = {{.type = "boolean", .description = "true if listening for input"}},
    });

    sys.methods.push_back(MethodDoc{
        .name = "startListeningForInput",
        .qualifiedName = "bestow.input.startListeningForInput",
        .description = "Begin listening for the next raw input. Used for key-rebinding UIs.",
        .seeAlso = {"bestow.input.stopListeningForInput", "bestow.input.getLastInput"},
    });

    sys.methods.push_back(MethodDoc{
        .name = "stopListeningForInput",
        .qualifiedName = "bestow.input.stopListeningForInput",
        .description = "Stop listening for raw input.",
        .seeAlso = {"bestow.input.startListeningForInput"},
    });

    // --- Text Input ---

    sys.methods.push_back(MethodDoc{
        .name = "enableTextInput",
        .qualifiedName = "bestow.input.enableTextInput",
        .description = "Enable text input mode. Characters typed will be buffered and accessible via getTextInput().",
        .seeAlso = {"bestow.input.disableTextInput", "bestow.input.getTextInput"},
    });

    sys.methods.push_back(MethodDoc{
        .name = "disableTextInput",
        .qualifiedName = "bestow.input.disableTextInput",
        .description = "Disable text input mode.",
        .seeAlso = {"bestow.input.enableTextInput"},
    });

    sys.methods.push_back(MethodDoc{
        .name = "isTextInputEnabled",
        .qualifiedName = "bestow.input.isTextInputEnabled",
        .description = "Check if text input mode is enabled.",
        .returns = {{.type = "boolean", .description = "true if text input is enabled"}},
    });

    sys.methods.push_back(MethodDoc{
        .name = "getTextInput",
        .qualifiedName = "bestow.input.getTextInput",
        .description = "Get the buffered text input string since the last clear.",
        .returns = {{.type = "string", .description = "Buffered text characters"}},
        .seeAlso = {"bestow.input.clearTextInput"},
    });

    sys.methods.push_back(MethodDoc{
        .name = "clearTextInput",
        .qualifiedName = "bestow.input.clearTextInput",
        .description = "Clear the text input buffer.",
        .seeAlso = {"bestow.input.getTextInput"},
    });

    // --- Controller ---

    sys.methods.push_back(MethodDoc{
        .name = "getConnectedControllerCount",
        .qualifiedName = "bestow.input.getConnectedControllerCount",
        .description = "Get the number of currently connected game controllers.",
        .returns = {{.type = "number", .description = "Number of connected controllers"}},
    });

    sys.methods.push_back(MethodDoc{
        .name = "isControllerConnected",
        .qualifiedName = "bestow.input.isControllerConnected",
        .description = "Check if a controller at the given index is connected.",
        .params = {
            {.name = "index", .type = "number", .description = "Controller index (0-based)"},
        },
        .returns = {{.type = "boolean", .description = "true if connected"}},
    });

    sys.methods.push_back(MethodDoc{
        .name = "getControllerName",
        .qualifiedName = "bestow.input.getControllerName",
        .description = "Get the name of the controller at the given index.",
        .params = {
            {.name = "index", .type = "number", .description = "Controller index (0-based)"},
        },
        .returns = {{.type = "string", .description = "Controller name string"}},
    });

    // --- Platform-agnostic State Queries (new API, defined in action_binding.cpp) ---

    sys.methods.push_back(MethodDoc{
        .name = "isKeyDown",
        .qualifiedName = "bestow.input.isKeyDown",
        .description = "Check if a key is currently held down using platform-agnostic KeyCode.",
        .params = {
            {.name = "key", .type = "KeyCode", .description = "The key to check"},
        },
        .returns = {{.type = "boolean", .description = "true if the key is held down"}},
    });

    sys.methods.push_back(MethodDoc{
        .name = "wasKeyJustPressed",
        .qualifiedName = "bestow.input.wasKeyJustPressed",
        .description = "Check if a key was just pressed this frame.",
        .params = {
            {.name = "key", .type = "KeyCode", .description = "The key to check"},
        },
        .returns = {{.type = "boolean", .description = "true if just pressed this frame"}},
    });

    sys.methods.push_back(MethodDoc{
        .name = "wasKeyJustReleased",
        .qualifiedName = "bestow.input.wasKeyJustReleased",
        .description = "Check if a key was just released this frame.",
        .params = {
            {.name = "key", .type = "KeyCode", .description = "The key to check"},
        },
        .returns = {{.type = "boolean", .description = "true if just released this frame"}},
    });

    sys.methods.push_back(MethodDoc{
        .name = "isMouseButtonDown",
        .qualifiedName = "bestow.input.isMouseButtonDown",
        .description = "Check if a mouse button is currently held down.",
        .params = {
            {.name = "button", .type = "MouseButton", .description = "The mouse button to check"},
        },
        .returns = {{.type = "boolean", .description = "true if the button is held down"}},
    });

    sys.methods.push_back(MethodDoc{
        .name = "wasMouseButtonJustPressed",
        .qualifiedName = "bestow.input.wasMouseButtonJustPressed",
        .description = "Check if a mouse button was just pressed this frame.",
        .params = {
            {.name = "button", .type = "MouseButton", .description = "The mouse button to check"},
        },
        .returns = {{.type = "boolean", .description = "true if just pressed this frame"}},
    });

    sys.methods.push_back(MethodDoc{
        .name = "wasMouseButtonJustReleased",
        .qualifiedName = "bestow.input.wasMouseButtonJustReleased",
        .description = "Check if a mouse button was just released this frame.",
        .params = {
            {.name = "button", .type = "MouseButton", .description = "The mouse button to check"},
        },
        .returns = {{.type = "boolean", .description = "true if just released this frame"}},
    });

    sys.methods.push_back(MethodDoc{
        .name = "isGamepadButtonDown",
        .qualifiedName = "bestow.input.isGamepadButtonDown",
        .description = "Check if a gamepad button is currently held down.",
        .params = {
            {.name = "button", .type = "GamepadButton", .description = "The gamepad button to check"},
            {.name = "gamepadIndex", .type = "number", .description = "Gamepad index (0-based)", .optional = true, .defaultVal = "0"},
        },
        .returns = {{.type = "boolean", .description = "true if the button is held down"}},
    });

    sys.methods.push_back(MethodDoc{
        .name = "wasGamepadButtonJustPressed",
        .qualifiedName = "bestow.input.wasGamepadButtonJustPressed",
        .description = "Check if a gamepad button was just pressed this frame.",
        .params = {
            {.name = "button", .type = "GamepadButton", .description = "The gamepad button to check"},
            {.name = "gamepadIndex", .type = "number", .description = "Gamepad index (0-based)", .optional = true, .defaultVal = "0"},
        },
        .returns = {{.type = "boolean", .description = "true if just pressed this frame"}},
    });

    sys.methods.push_back(MethodDoc{
        .name = "wasGamepadButtonJustReleased",
        .qualifiedName = "bestow.input.wasGamepadButtonJustReleased",
        .description = "Check if a gamepad button was just released this frame.",
        .params = {
            {.name = "button", .type = "GamepadButton", .description = "The gamepad button to check"},
            {.name = "gamepadIndex", .type = "number", .description = "Gamepad index (0-based)", .optional = true, .defaultVal = "0"},
        },
        .returns = {{.type = "boolean", .description = "true if just released this frame"}},
    });

    sys.methods.push_back(MethodDoc{
        .name = "getGamepadAxisValue",
        .qualifiedName = "bestow.input.getGamepadAxisValue",
        .description = "Get the current value of a gamepad axis (-1.0 to 1.0).",
        .params = {
            {.name = "axis", .type = "GamepadAxis", .description = "The gamepad axis to query"},
            {.name = "gamepadIndex", .type = "number", .description = "Gamepad index (0-based)", .optional = true, .defaultVal = "0"},
        },
        .returns = {{.type = "number", .description = "Axis value (-1.0 to 1.0)"}},
    });

    sys.methods.push_back(MethodDoc{
        .name = "getLeftStick",
        .qualifiedName = "bestow.input.getLeftStick",
        .description = "Get the left analog stick position as a 2D vector.",
        .params = {
            {.name = "gamepadIndex", .type = "number", .description = "Gamepad index (0-based)", .optional = true, .defaultVal = "0"},
        },
        .returns = {{.type = "Vec2", .description = "Left stick position (x, y), each -1.0 to 1.0"}},
    });

    sys.methods.push_back(MethodDoc{
        .name = "getRightStick",
        .qualifiedName = "bestow.input.getRightStick",
        .description = "Get the right analog stick position as a 2D vector.",
        .params = {
            {.name = "gamepadIndex", .type = "number", .description = "Gamepad index (0-based)", .optional = true, .defaultVal = "0"},
        },
        .returns = {{.type = "Vec2", .description = "Right stick position (x, y), each -1.0 to 1.0"}},
    });

    sys.methods.push_back(MethodDoc{
        .name = "getInputState",
        .qualifiedName = "bestow.input.getInputState",
        .description = "Get the current state of any input binding.",
        .params = {
            {.name = "binding", .type = "InputBinding", .description = "The input binding to query"},
        },
        .returns = {{.type = "InputState", .description = "Current input state (NotPressed, JustPressed, Pressed, Held, JustReleased)"}},
    });

    sys.methods.push_back(MethodDoc{
        .name = "getInputHoldDuration",
        .qualifiedName = "bestow.input.getInputHoldDuration",
        .description = "Get how long an input binding has been held down in seconds.",
        .params = {
            {.name = "binding", .type = "InputBinding", .description = "The input binding to query"},
        },
        .returns = {{.type = "number", .description = "Hold duration in seconds (0 if not held)"}},
    });

    sys.methods.push_back(MethodDoc{
        .name = "setDefaultHoldThreshold",
        .qualifiedName = "bestow.input.setDefaultHoldThreshold",
        .description = "Set the default hold threshold for whenHeld conditions (in seconds).",
        .params = {
            {.name = "seconds", .type = "number", .description = "Hold threshold in seconds"},
        },
        .seeAlso = {"bestow.input.getDefaultHoldThreshold"},
    });

    sys.methods.push_back(MethodDoc{
        .name = "getDefaultHoldThreshold",
        .qualifiedName = "bestow.input.getDefaultHoldThreshold",
        .description = "Get the current default hold threshold in seconds.",
        .returns = {{.type = "number", .description = "Hold threshold in seconds"}},
        .seeAlso = {"bestow.input.setDefaultHoldThreshold"},
    });

    sys.methods.push_back(MethodDoc{
        .name = "onAction",
        .qualifiedName = "bestow.input.onAction",
        .description = "Subscribe to a named action event. Currently a placeholder -- use bestow.events.subscribe() directly.",
        .params = {
            {.name = "actionName", .type = "string", .description = "Name of the action to listen for"},
            {.name = "callback", .type = "function", .description = "Callback invoked when the action fires"},
        },
        .returns = {{.type = "number", .description = "Subscription ID (placeholder)"}},
    });

    // --- Properties (subtables) ---

    sys.properties.push_back(PropertyDoc{
        .name = "keys",
        .type = "table",
        .description = "Platform-agnostic KeyCode lookup table. e.g., bestow.input.keys.Space, bestow.input.keys.A",
        .readOnly = true,
    });

    sys.properties.push_back(PropertyDoc{
        .name = "buttons",
        .type = "table",
        .description = "Gamepad button lookup table. e.g., bestow.input.buttons.A, bestow.input.buttons.LeftBumper",
        .readOnly = true,
    });

    sys.properties.push_back(PropertyDoc{
        .name = "axes",
        .type = "table",
        .description = "Gamepad axis lookup table. e.g., bestow.input.axes.LeftX, bestow.input.axes.RightTrigger",
        .readOnly = true,
    });

    sys.properties.push_back(PropertyDoc{
        .name = "mouse",
        .type = "table",
        .description = "Mouse button lookup table. e.g., bestow.input.mouse.Left, bestow.input.mouse.Right",
        .readOnly = true,
    });

    sys.seeAlso = {"bestow.action"};

    registry.addSystem(std::move(sys));
}

} // namespace bestow

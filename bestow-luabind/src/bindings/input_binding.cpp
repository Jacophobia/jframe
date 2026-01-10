// bestow-luabind/src/bindings/input_binding.cpp
// Input system Lua bindings
//
// NOTE: This file contains the legacy polling-based input API.
// The new event-driven input API (ActionBuilder, phase management) is in action_binding.cpp.
// The legacy API is maintained for backwards compatibility but is deprecated.

module;

#include <bestow/sol2_compat.hpp>
#include <spdlog/spdlog.h>

module bestow.luabind;

import std;

namespace bestow {

namespace {
    // Track if deprecation warning has been logged (to avoid spam)
    bool g_deprecationWarningLogged = false;

    void logDeprecationWarning(const char* functionName) {
        if (!g_deprecationWarningLogged) {
            spdlog::warn("[Input] Legacy polling API is deprecated. "
                        "Use event-driven input with ActionBuilder and bestow.events instead. "
                        "See docs/input-migration.md for migration guide.");
            g_deprecationWarningLogged = true;
        }
        spdlog::debug("[Input] Deprecated function called: {}", functionName);
    }
}

void bindInputSystem(sol::state& lua, IInputSystem& input) {
    //=========================================================================
    // Input-related types (Legacy - for backwards compatibility)
    //=========================================================================

    // InputDeviceType enum (deprecated - use InputSource from action_binding.cpp)
    lua.new_enum<InputDeviceType>("InputDeviceType",
        {
            {"Keyboard", InputDeviceType::Keyboard},
            {"Mouse", InputDeviceType::Mouse},
            {"Controller", InputDeviceType::Controller}
        }
    );

    // ModifierKey enum (bitmask) - still valid in new API
    lua.new_enum<ModifierKey>("ModifierKey",
        {
            {"None", ModifierKey::None},
            {"Shift", ModifierKey::Shift},
            {"Ctrl", ModifierKey::Ctrl},
            {"Alt", ModifierKey::Alt},
            {"Super", ModifierKey::Super},
            {"CapsLock", ModifierKey::CapsLock},
            {"NumLock", ModifierKey::NumLock}
        }
    );

    // CursorMode enum - for cursor visibility control
    lua.new_enum<CursorMode>("CursorMode",
        {
            {"Normal", CursorMode::Normal},
            {"Hidden", CursorMode::Hidden},
            {"Disabled", CursorMode::Disabled}
        }
    );

    // Note: InputBinding usertype is now defined in action_binding.cpp with the new structure

    // ActionState struct (legacy)
    lua.new_usertype<ActionState>("ActionState",
        sol::constructors<ActionState()>(),
        "action", &ActionState::action,
        "active", &ActionState::active,
        "value", &ActionState::value,
        "justPressed", &ActionState::justPressed,
        "justReleased", &ActionState::justReleased
    );

    // InputMapping struct (legacy)
    lua.new_usertype<InputMapping>("InputMapping",
        sol::constructors<InputMapping()>(),
        "binding", &InputMapping::binding,
        "action", &InputMapping::action
    );

    //=========================================================================
    // bestow.input table
    //=========================================================================

    sol::table bestow = lua["bestow"];
    sol::table inputTable = lua.create_table();

    //-------------------------------------------------------------------------
    // Action State Queries (DEPRECATED - use event subscriptions)
    //-------------------------------------------------------------------------

    inputTable["isActionActive"] = [&input](const std::string& action) {
        logDeprecationWarning("isActionActive");
        return input.isActionActive(action);
    };

    inputTable["wasActionJustPressed"] = [&input](const std::string& action) {
        logDeprecationWarning("wasActionJustPressed");
        return input.wasActionJustPressed(action);
    };

    inputTable["wasActionJustReleased"] = [&input](const std::string& action) {
        logDeprecationWarning("wasActionJustReleased");
        return input.wasActionJustReleased(action);
    };

    inputTable["getActionValue"] = [&input](const std::string& action) {
        logDeprecationWarning("getActionValue");
        return input.getActionValue(action);
    };

    inputTable["getActionState"] = [&input](const std::string& action) {
        logDeprecationWarning("getActionState");
        return input.getActionState(action);
    };

    inputTable["getAllActionStates"] = [&input]() {
        logDeprecationWarning("getAllActionStates");
        return input.getAllActionStates();
    };

    //-------------------------------------------------------------------------
    // Mouse State (still valid - use bestow.input.mouse for button constants)
    //-------------------------------------------------------------------------

    inputTable["getMousePosition"] = [&input]() {
        return input.getMousePosition();
    };

    inputTable["getMouseDelta"] = [&input]() {
        return input.getMouseDelta();
    };

    // Legacy int-based overloads for backwards compatibility
    // New code should use MouseButton enum from action_binding.cpp
    inputTable["isMouseButtonDownLegacy"] = [&input](int button) {
        logDeprecationWarning("isMouseButtonDown(int)");
        // Convert int to MouseButton
        if (button >= 0 && button < static_cast<int>(MouseButton::Count)) {
            return input.isMouseButtonDown(static_cast<MouseButton>(button));
        }
        return false;
    };

    inputTable["wasMouseButtonJustPressedLegacy"] = [&input](int button) {
        logDeprecationWarning("wasMouseButtonJustPressed(int)");
        if (button >= 0 && button < static_cast<int>(MouseButton::Count)) {
            return input.wasMouseButtonJustPressed(static_cast<MouseButton>(button));
        }
        return false;
    };

    inputTable["wasMouseButtonJustReleasedLegacy"] = [&input](int button) {
        logDeprecationWarning("wasMouseButtonJustReleased(int)");
        if (button >= 0 && button < static_cast<int>(MouseButton::Count)) {
            return input.wasMouseButtonJustReleased(static_cast<MouseButton>(button));
        }
        return false;
    };

    inputTable["getScrollDelta"] = [&input]() {
        return input.getScrollDelta();
    };

    //-------------------------------------------------------------------------
    // Cursor Control
    //-------------------------------------------------------------------------

    inputTable["showMouseCursor"] = [&input]() {
        input.showMouseCursor();
    };

    inputTable["hideMouseCursor"] = [&input]() {
        input.hideMouseCursor();
    };

    inputTable["isMouseCursorVisible"] = [&input]() {
        return input.isMouseCursorVisible();
    };

    inputTable["setCursorMode"] = [&input](CursorMode mode) {
        input.setCursorMode(mode);
    };

    inputTable["getCursorMode"] = [&input]() {
        return input.getCursorMode();
    };

    //-------------------------------------------------------------------------
    // Modifier Keys (still valid)
    //-------------------------------------------------------------------------

    inputTable["getModifierState"] = [&input]() {
        return input.getModifierState();
    };

    inputTable["isModifierPressed"] = [&input](ModifierKey mod) {
        return input.isModifierPressed(mod);
    };

    inputTable["isShiftPressed"] = [&input]() {
        return input.isShiftPressed();
    };

    inputTable["isCtrlPressed"] = [&input]() {
        return input.isCtrlPressed();
    };

    inputTable["isAltPressed"] = [&input]() {
        return input.isAltPressed();
    };

    inputTable["isSuperPressed"] = [&input]() {
        return input.isSuperPressed();
    };

    //-------------------------------------------------------------------------
    // Direct Keyboard State (Legacy int-based - use KeyCode from action_binding.cpp)
    //-------------------------------------------------------------------------

    // Legacy int-based overloads for backwards compatibility
    inputTable["isKeyDownLegacy"] = [&input](int keyCode) {
        logDeprecationWarning("isKeyDown(int)");
        // Convert GLFW key code to KeyCode
        // This is a simple cast since KeyCode values match GLFW values for common keys
        return input.isKeyDown(static_cast<KeyCode>(keyCode));
    };

    inputTable["wasKeyJustPressedLegacy"] = [&input](int keyCode) {
        logDeprecationWarning("wasKeyJustPressed(int)");
        return input.wasKeyJustPressed(static_cast<KeyCode>(keyCode));
    };

    inputTable["wasKeyJustReleasedLegacy"] = [&input](int keyCode) {
        logDeprecationWarning("wasKeyJustReleased(int)");
        return input.wasKeyJustReleased(static_cast<KeyCode>(keyCode));
    };

    //-------------------------------------------------------------------------
    // Mapping Management (DEPRECATED - use ActionBuilder)
    //-------------------------------------------------------------------------

    inputTable["registerMapping"] = [&input](const InputMapping& mapping) {
        logDeprecationWarning("registerMapping");
        input.registerMapping(mapping);
    };

    inputTable["removeMapping"] = [&input](const InputBinding& binding) {
        logDeprecationWarning("removeMapping");
        input.removeMapping(binding);
    };

    inputTable["clearMappings"] = [&input]() {
        logDeprecationWarning("clearMappings");
        input.clearMappings();
    };

    inputTable["getMappings"] = [&input]() {
        logDeprecationWarning("getMappings");
        return input.getMappings();
    };

    //-------------------------------------------------------------------------
    // Raw Input (for Rebinding UI) - still valid
    //-------------------------------------------------------------------------

    inputTable["getLastInput"] = [&input, &lua]() -> sol::object {
        auto lastInput = input.getLastInput();
        if (lastInput) {
            return sol::make_object(lua, *lastInput);
        }
        return sol::nil;
    };

    inputTable["isListeningForInput"] = [&input]() {
        return input.isListeningForInput();
    };

    inputTable["startListeningForInput"] = [&input]() {
        input.startListeningForInput();
    };

    inputTable["stopListeningForInput"] = [&input]() {
        input.stopListeningForInput();
    };

    //-------------------------------------------------------------------------
    // Text Input - still valid
    //-------------------------------------------------------------------------

    inputTable["enableTextInput"] = [&input]() {
        input.enableTextInput();
    };

    inputTable["disableTextInput"] = [&input]() {
        input.disableTextInput();
    };

    inputTable["isTextInputEnabled"] = [&input]() {
        return input.isTextInputEnabled();
    };

    inputTable["getTextInput"] = [&input]() {
        return input.getTextInput();
    };

    inputTable["clearTextInput"] = [&input]() {
        input.clearTextInput();
    };

    //-------------------------------------------------------------------------
    // Controller - still valid
    //-------------------------------------------------------------------------

    inputTable["getConnectedControllerCount"] = [&input]() {
        return input.getConnectedControllerCount();
    };

    inputTable["isControllerConnected"] = [&input](int index) {
        return input.isControllerConnected(index);
    };

    inputTable["getControllerName"] = [&input](int index) {
        return input.getControllerName(index);
    };

    //-------------------------------------------------------------------------
    // Lifecycle Management (internal use)
    //-------------------------------------------------------------------------

    inputTable["initialize"] = [&input](void* windowHandle) {
        input.initialize(windowHandle);
    };

    inputTable["update"] = [&input]() {
        input.update();
    };

    inputTable["shutdown"] = [&input]() {
        input.shutdown();
    };

    //-------------------------------------------------------------------------
    // Legacy key code constants (DEPRECATED - use bestow.input.keys instead)
    // These are GLFW key codes exposed as integers.
    // New code should use bestow.input.keys.* which provides KeyCode enums.
    //-------------------------------------------------------------------------

    sol::table keys = lua.create_table();

    // Note: These integer values match GLFW key codes for backwards compatibility
    // New code should use bestow.input.keys.Space, etc. from action_binding.cpp
    keys["SPACE"] = 32;
    keys["APOSTROPHE"] = 39;
    keys["COMMA"] = 44;
    keys["MINUS"] = 45;
    keys["PERIOD"] = 46;
    keys["SLASH"] = 47;
    keys["_0"] = 48;
    keys["_1"] = 49;
    keys["_2"] = 50;
    keys["_3"] = 51;
    keys["_4"] = 52;
    keys["_5"] = 53;
    keys["_6"] = 54;
    keys["_7"] = 55;
    keys["_8"] = 56;
    keys["_9"] = 57;
    keys["SEMICOLON"] = 59;
    keys["EQUAL"] = 61;
    keys["A"] = 65;
    keys["B"] = 66;
    keys["C"] = 67;
    keys["D"] = 68;
    keys["E"] = 69;
    keys["F"] = 70;
    keys["G"] = 71;
    keys["H"] = 72;
    keys["I"] = 73;
    keys["J"] = 74;
    keys["K"] = 75;
    keys["L"] = 76;
    keys["M"] = 77;
    keys["N"] = 78;
    keys["O"] = 79;
    keys["P"] = 80;
    keys["Q"] = 81;
    keys["R"] = 82;
    keys["S"] = 83;
    keys["T"] = 84;
    keys["U"] = 85;
    keys["V"] = 86;
    keys["W"] = 87;
    keys["X"] = 88;
    keys["Y"] = 89;
    keys["Z"] = 90;
    keys["LEFT_BRACKET"] = 91;
    keys["BACKSLASH"] = 92;
    keys["RIGHT_BRACKET"] = 93;
    keys["GRAVE_ACCENT"] = 96;
    keys["ESCAPE"] = 256;
    keys["ENTER"] = 257;
    keys["TAB"] = 258;
    keys["BACKSPACE"] = 259;
    keys["INSERT"] = 260;
    keys["DELETE"] = 261;
    keys["RIGHT"] = 262;
    keys["LEFT"] = 263;
    keys["DOWN"] = 264;
    keys["UP"] = 265;
    keys["PAGE_UP"] = 266;
    keys["PAGE_DOWN"] = 267;
    keys["HOME"] = 268;
    keys["END"] = 269;
    keys["CAPS_LOCK"] = 280;
    keys["SCROLL_LOCK"] = 281;
    keys["NUM_LOCK"] = 282;
    keys["PRINT_SCREEN"] = 283;
    keys["PAUSE"] = 284;
    keys["F1"] = 290;
    keys["F2"] = 291;
    keys["F3"] = 292;
    keys["F4"] = 293;
    keys["F5"] = 294;
    keys["F6"] = 295;
    keys["F7"] = 296;
    keys["F8"] = 297;
    keys["F9"] = 298;
    keys["F10"] = 299;
    keys["F11"] = 300;
    keys["F12"] = 301;
    keys["LEFT_SHIFT"] = 340;
    keys["LEFT_CONTROL"] = 341;
    keys["LEFT_ALT"] = 342;
    keys["LEFT_SUPER"] = 343;
    keys["RIGHT_SHIFT"] = 344;
    keys["RIGHT_CONTROL"] = 345;
    keys["RIGHT_ALT"] = 346;
    keys["RIGHT_SUPER"] = 347;

    // DEPRECATED: Use bestow.input.keys instead
    inputTable["Key"] = keys;

    // Legacy mouse button constants (integers)
    // DEPRECATED: Use bestow.input.mouse instead
    sol::table mouse = lua.create_table();
    mouse["LEFT"] = 0;
    mouse["RIGHT"] = 1;
    mouse["MIDDLE"] = 2;
    mouse["BUTTON_4"] = 3;
    mouse["BUTTON_5"] = 4;
    mouse["BUTTON_6"] = 5;
    mouse["BUTTON_7"] = 6;
    mouse["BUTTON_8"] = 7;

    inputTable["Mouse"] = mouse;

    bestow["input"] = inputTable;

    //=========================================================================
    // Global aliases for convenience (DEPRECATED)
    // These are kept for backwards compatibility but new code should use
    // bestow.input.keys.* and bestow.input.buttons.* instead.
    //=========================================================================

    // Create a Keys global with both UPPER_CASE (canonical) and PascalCase (convenience)
    sol::table globalKeys = lua.create_table();

    // Copy all key constants with canonical names
    for (auto& [key, value] : keys) {
        globalKeys[key] = value;
    }

    // Add PascalCase aliases for common keys (convenience for documentation examples)
    globalKeys["Space"] = 32;
    globalKeys["Escape"] = 256;
    globalKeys["Enter"] = 257;
    globalKeys["Tab"] = 258;
    globalKeys["Backspace"] = 259;
    globalKeys["Comma"] = 44;
    globalKeys["Period"] = 46;

    // Letter keys (lowercase as PascalCase since they're single letters)
    globalKeys["A"] = 65;
    globalKeys["B"] = 66;
    globalKeys["C"] = 67;
    globalKeys["D"] = 68;
    globalKeys["E"] = 69;
    globalKeys["F"] = 70;
    globalKeys["G"] = 71;
    globalKeys["H"] = 72;
    globalKeys["I"] = 73;
    globalKeys["J"] = 74;
    globalKeys["K"] = 75;
    globalKeys["L"] = 76;
    globalKeys["M"] = 77;
    globalKeys["N"] = 78;
    globalKeys["O"] = 79;
    globalKeys["P"] = 80;
    globalKeys["Q"] = 81;
    globalKeys["R"] = 82;
    globalKeys["S"] = 83;
    globalKeys["T"] = 84;
    globalKeys["U"] = 85;
    globalKeys["V"] = 86;
    globalKeys["W"] = 87;
    globalKeys["X"] = 88;
    globalKeys["Y"] = 89;
    globalKeys["Z"] = 90;

    // Arrow keys
    globalKeys["Up"] = 265;
    globalKeys["Down"] = 264;
    globalKeys["Left"] = 263;
    globalKeys["Right"] = 262;

    // Function keys
    globalKeys["F1"] = 290;
    globalKeys["F2"] = 291;
    globalKeys["F3"] = 292;
    globalKeys["F4"] = 293;
    globalKeys["F5"] = 294;
    globalKeys["F6"] = 295;
    globalKeys["F7"] = 296;
    globalKeys["F8"] = 297;
    globalKeys["F9"] = 298;
    globalKeys["F10"] = 299;
    globalKeys["F11"] = 300;
    globalKeys["F12"] = 301;

    // Modifier keys (PascalCase)
    globalKeys["LeftShift"] = 340;
    globalKeys["RightShift"] = 344;
    globalKeys["LeftCtrl"] = 341;
    globalKeys["RightCtrl"] = 345;
    globalKeys["LeftAlt"] = 342;
    globalKeys["RightAlt"] = 346;

    // DEPRECATED: Use bestow.input.keys instead
    inputTable["Keys"] = globalKeys;

    spdlog::debug("[LuaContractBinder] Bound legacy input API (use bestow.input.keys/buttons/axes for new code)");
}

}  // namespace bestow

// bestow-luabind/src/bindings/input_binding.cpp
// Input system Lua bindings

module;

#include <bestow/sol2_compat.hpp>
#include <spdlog/spdlog.h>

module bestow.luabind;

import std;

namespace bestow {

void bindInputSystem(sol::state& lua, IInputSystem& input) {
    //=========================================================================
    // Input-related types
    //=========================================================================

    // InputDeviceType enum
    lua.new_enum<InputDeviceType>("InputDeviceType",
        {
            {"Keyboard", InputDeviceType::Keyboard},
            {"Mouse", InputDeviceType::Mouse},
            {"Controller", InputDeviceType::Controller}
        }
    );

    // ModifierKey enum (bitmask)
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

    // InputBinding struct
    lua.new_usertype<InputBinding>("InputBinding",
        sol::constructors<InputBinding()>(),
        "deviceType", &InputBinding::deviceType,
        "deviceIndex", &InputBinding::deviceIndex,
        "keyCode", &InputBinding::keyCode,
        "requiredModifiers", &InputBinding::requiredModifiers,
        "scale", &InputBinding::scale,
        "deadzone", &InputBinding::deadzone
    );

    // ActionState struct
    lua.new_usertype<ActionState>("ActionState",
        sol::constructors<ActionState()>(),
        "action", &ActionState::action,
        "active", &ActionState::active,
        "value", &ActionState::value,
        "justPressed", &ActionState::justPressed,
        "justReleased", &ActionState::justReleased
    );

    // InputMapping struct
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
    // Action State Queries (most commonly used)
    //-------------------------------------------------------------------------

    inputTable["isActionActive"] = [&input](const std::string& action) {
        return input.isActionActive(action);
    };

    inputTable["wasActionJustPressed"] = [&input](const std::string& action) {
        return input.wasActionJustPressed(action);
    };

    inputTable["wasActionJustReleased"] = [&input](const std::string& action) {
        return input.wasActionJustReleased(action);
    };

    inputTable["getActionValue"] = [&input](const std::string& action) {
        return input.getActionValue(action);
    };

    inputTable["getActionState"] = [&input](const std::string& action) {
        return input.getActionState(action);
    };

    inputTable["getAllActionStates"] = [&input]() {
        return input.getAllActionStates();
    };

    //-------------------------------------------------------------------------
    // Mouse State
    //-------------------------------------------------------------------------

    inputTable["getMousePosition"] = [&input]() {
        return input.getMousePosition();
    };

    inputTable["getMouseDelta"] = [&input]() {
        return input.getMouseDelta();
    };

    inputTable["isMouseButtonDown"] = [&input](int button) {
        return input.isMouseButtonDown(button);
    };

    inputTable["wasMouseButtonJustPressed"] = [&input](int button) {
        return input.wasMouseButtonJustPressed(button);
    };

    inputTable["wasMouseButtonJustReleased"] = [&input](int button) {
        return input.wasMouseButtonJustReleased(button);
    };

    inputTable["getScrollDelta"] = [&input]() {
        return input.getScrollDelta();
    };

    //-------------------------------------------------------------------------
    // Modifier Keys
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
    // Direct Keyboard State
    //-------------------------------------------------------------------------

    inputTable["isKeyDown"] = [&input](int keyCode) {
        return input.isKeyDown(keyCode);
    };

    inputTable["wasKeyJustPressed"] = [&input](int keyCode) {
        return input.wasKeyJustPressed(keyCode);
    };

    inputTable["wasKeyJustReleased"] = [&input](int keyCode) {
        return input.wasKeyJustReleased(keyCode);
    };

    //-------------------------------------------------------------------------
    // Mapping Management
    //-------------------------------------------------------------------------

    inputTable["registerMapping"] = [&input](const InputMapping& mapping) {
        input.registerMapping(mapping);
    };

    inputTable["removeMapping"] = [&input](const InputBinding& binding) {
        input.removeMapping(binding);
    };

    inputTable["clearMappings"] = [&input]() {
        input.clearMappings();
    };

    inputTable["getMappings"] = [&input]() {
        return input.getMappings();
    };

    //-------------------------------------------------------------------------
    // Raw Input (for Rebinding UI)
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
    // Text Input
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
    // Controller
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
    // Lifecycle Management
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
    // Common key code constants (subset of GLFW key codes)
    // Users can also use raw integer key codes
    //-------------------------------------------------------------------------

    sol::table keys = lua.create_table();
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

    inputTable["Key"] = keys;

    // Mouse button constants
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
    // Global aliases for convenience (so users can write Keys.Escape instead
    // of bestow.input.Key.ESCAPE)
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

    inputTable["Keys"] = globalKeys;
}

}  // namespace bestow

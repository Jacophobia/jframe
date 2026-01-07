// bestow-luabind/src/bindings/action_binding.cpp
// Action builder and action system Lua bindings

module;

#include <bestow/sol2_compat.hpp>
#include <spdlog/spdlog.h>

module bestow.luabind;

import std;

namespace bestow {

//=============================================================================
// ActionBuilder - Fluent API for registering input actions
//=============================================================================

/// ActionBuilder provides a fluent interface for constructing ActionRegistrations.
/// Used from Lua like:
///   bestow.action.builder()
///     :duringPhase("game.melee")
///     :whenPressed(bestow.input.keys.Space)
///     :emitAction("Jump")
///     :discretely()
class ActionBuilder {
public:
    explicit ActionBuilder(IInputSystem* inputSystem)
        : inputSystem_(inputSystem) {}

    //=========================================================================
    // Phase Configuration
    //=========================================================================

    ActionBuilder& duringPhase(const std::string& phase) {
        registration_.phase = phase;
        return *this;
    }

    //=========================================================================
    // Condition Methods
    //=========================================================================

    /// Add a WhenPressed condition (fires on JustPressed frame)
    ActionBuilder& whenPressed(sol::object input) {
        ActionCondition condition;
        condition.type = ActionConditionType::WhenPressed;
        condition.input = parseInputBinding(input);
        registration_.conditions.push_back(condition);
        return *this;
    }

    /// Add a WhenReleased condition (fires on JustReleased frame)
    ActionBuilder& whenReleased(sol::object input) {
        ActionCondition condition;
        condition.type = ActionConditionType::WhenReleased;
        condition.input = parseInputBinding(input);
        registration_.conditions.push_back(condition);
        return *this;
    }

    /// Add a WhenActive condition (true while JustPressed, Pressed, or Held)
    ActionBuilder& whenActive(sol::object input) {
        ActionCondition condition;
        condition.type = ActionConditionType::WhenActive;
        condition.input = parseInputBinding(input);
        registration_.conditions.push_back(condition);
        return *this;
    }

    /// Add a WhenInactive condition (true while NotPressed or JustReleased)
    ActionBuilder& whenInactive(sol::object input) {
        ActionCondition condition;
        condition.type = ActionConditionType::WhenInactive;
        condition.input = parseInputBinding(input);
        registration_.conditions.push_back(condition);
        return *this;
    }

    /// Add a WhenHeld condition (fires on transition to Held state)
    /// Uses default hold threshold if not specified
    ActionBuilder& whenHeld(sol::object input, std::optional<float> threshold = std::nullopt) {
        ActionCondition condition;
        condition.type = ActionConditionType::WhenHeld;
        condition.input = parseInputBinding(input);
        condition.holdThreshold = threshold;
        registration_.conditions.push_back(condition);
        return *this;
    }

    //=========================================================================
    // Configuration Methods
    //=========================================================================

    /// Set deadzone for axis inputs (0.0-1.0)
    ActionBuilder& withDeadzone(float deadzone) {
        registration_.deadzone = std::clamp(deadzone, 0.0f, 1.0f);
        return *this;
    }

    //=========================================================================
    // Effect Methods
    //=========================================================================

    /// Emit an action event when conditions are met
    ActionBuilder& emitAction(const std::string& actionName) {
        ActionEffect effect;
        effect.type = ActionEffectType::EmitAction;
        effect.value = actionName;
        registration_.effects.push_back(effect);
        return *this;
    }

    /// Push a phase onto the phase stack
    ActionBuilder& pushPhase(const std::string& phase) {
        ActionEffect effect;
        effect.type = ActionEffectType::PushPhase;
        effect.value = phase;
        registration_.effects.push_back(effect);
        return *this;
    }

    /// Pop the current phase from the stack
    ActionBuilder& popPhase() {
        ActionEffect effect;
        effect.type = ActionEffectType::PopPhase;
        effect.value = "";
        registration_.effects.push_back(effect);
        return *this;
    }

    /// Replace the entire phase stack with a new phase
    ActionBuilder& changePhase(const std::string& phase) {
        ActionEffect effect;
        effect.type = ActionEffectType::ChangePhase;
        effect.value = phase;
        registration_.effects.push_back(effect);
        return *this;
    }

    //=========================================================================
    // Terminal Methods (finalize and register)
    //=========================================================================

    /// Register as discrete action (fires once when conditions transition to true)
    void discretely() {
        registration_.terminal = ActionTerminal::Discrete;
        finalizeAndRegister();
    }

    /// Register as continuous action (fires every frame while conditions are true)
    void continuously() {
        registration_.terminal = ActionTerminal::Continuous;
        finalizeAndRegister();
    }

private:
    /// Parse a Lua value into an InputBinding
    InputBinding parseInputBinding(sol::object input) {
        // Handle KeyCode enum
        if (input.is<KeyCode>()) {
            return InputBinding::key(input.as<KeyCode>());
        }

        // Handle MouseButton enum
        if (input.is<MouseButton>()) {
            return InputBinding::mouseButton(input.as<MouseButton>());
        }

        // Handle GamepadButton enum
        if (input.is<GamepadButton>()) {
            return InputBinding::gamepadButton(input.as<GamepadButton>());
        }

        // Handle GamepadAxis enum
        if (input.is<GamepadAxis>()) {
            return InputBinding::gamepadAxis(input.as<GamepadAxis>());
        }

        // Handle InputBinding directly (for custom bindings)
        if (input.is<InputBinding>()) {
            return input.as<InputBinding>();
        }

        // Handle table with binding properties
        if (input.get_type() == sol::type::table) {
            sol::table t = input;
            return parseInputBindingTable(t);
        }

        spdlog::warn("ActionBuilder: Unrecognized input type, using Unknown key");
        return InputBinding::key(KeyCode::Unknown);
    }

    /// Parse a Lua table into an InputBinding
    InputBinding parseInputBindingTable(sol::table t) {
        InputBinding binding;

        // Check for key - use sol::optional to avoid template issues
        sol::optional<KeyCode> keyOpt = t["key"];
        sol::optional<MouseButton> mouseOpt = t["mouseButton"];
        sol::optional<GamepadButton> gamepadBtnOpt = t["gamepadButton"];
        sol::optional<GamepadAxis> gamepadAxisOpt = t["gamepadAxis"];
        sol::optional<ModifierKey> modsOpt = t["modifiers"];

        if (keyOpt) {
            binding = InputBinding::key(*keyOpt);
        }
        else if (mouseOpt) {
            binding = InputBinding::mouseButton(*mouseOpt);
        }
        else if (gamepadBtnOpt) {
            int index = t.get_or("gamepadIndex", 0);
            binding = InputBinding::gamepadButton(*gamepadBtnOpt, index);
        }
        else if (gamepadAxisOpt) {
            int index = t.get_or("gamepadIndex", 0);
            float scale = t.get_or("scale", 1.0f);
            float deadzone = t.get_or("deadzone", 0.15f);
            binding = InputBinding::gamepadAxis(*gamepadAxisOpt, index, scale, deadzone);
        }

        // Override modifiers if specified
        if (modsOpt) {
            binding.requiredModifiers = *modsOpt;
        }

        return binding;
    }

    /// Validate and register the action
    void finalizeAndRegister() {
        // Validation
        registration_.valid = true;
        registration_.validationError.clear();

        // Check required fields
        if (registration_.phase.empty()) {
            registration_.valid = false;
            registration_.validationError = "No phase specified. Call duringPhase() before finalizing.";
        } else if (registration_.conditions.empty()) {
            registration_.valid = false;
            registration_.validationError = "No conditions specified. Add at least one whenPressed/whenReleased/etc.";
        } else if (registration_.effects.empty()) {
            registration_.valid = false;
            registration_.validationError = "No effects specified. Add at least one emitAction/pushPhase/etc.";
        }

        // Check for contradictory conditions
        if (registration_.valid) {
            validateConditions();
        }

        // Check for logical issues (warnings)
        if (registration_.valid) {
            validateLogic();
        }

        // Register if valid
        if (registration_.valid) {
            if (inputSystem_) {
                inputSystem_->registerAction(registration_);
                spdlog::debug("ActionBuilder: Registered action in phase '{}' with {} conditions, {} effects",
                             registration_.phase,
                             registration_.conditions.size(),
                             registration_.effects.size());
            } else {
                spdlog::error("ActionBuilder: Cannot register action - no InputSystem available");
            }
        } else {
            spdlog::error("ActionBuilder: Validation failed - {}", registration_.validationError);
        }
    }

    /// Check for contradictory conditions
    void validateConditions() {
        // Track what inputs have which condition types
        std::unordered_map<std::string, std::set<ActionConditionType>> inputConditions;

        for (const auto& cond : registration_.conditions) {
            std::string inputKey = serializeBinding(cond.input);
            inputConditions[inputKey].insert(cond.type);
        }

        // Check for contradictions
        for (const auto& [inputKey, types] : inputConditions) {
            bool hasActive = types.contains(ActionConditionType::WhenActive);
            bool hasInactive = types.contains(ActionConditionType::WhenInactive);

            if (hasActive && hasInactive) {
                registration_.valid = false;
                registration_.validationError = "Contradictory conditions: WhenActive and WhenInactive on same input";
                return;
            }
        }

        // Check for negative hold threshold
        for (const auto& cond : registration_.conditions) {
            if (cond.type == ActionConditionType::WhenHeld &&
                cond.holdThreshold.has_value() &&
                cond.holdThreshold.value() < 0.0f) {
                registration_.valid = false;
                registration_.validationError = "Negative hold threshold specified";
                return;
            }
        }

        // Check deadzone on non-axis input
        if (registration_.deadzone > 0.0f) {
            bool hasAxis = false;
            for (const auto& cond : registration_.conditions) {
                if (cond.input.isAxis()) {
                    hasAxis = true;
                    break;
                }
            }
            if (!hasAxis) {
                registration_.valid = false;
                registration_.validationError = "Deadzone specified but no axis inputs in conditions";
                return;
            }
        }
    }

    /// Check for logical issues (warnings only, doesn't prevent registration)
    void validateLogic() {
        bool hasPressed = false;
        bool hasReleased = false;
        bool hasHeld = false;

        for (const auto& cond : registration_.conditions) {
            switch (cond.type) {
                case ActionConditionType::WhenPressed: hasPressed = true; break;
                case ActionConditionType::WhenReleased: hasReleased = true; break;
                case ActionConditionType::WhenHeld: hasHeld = true; break;
                default: break;
            }
        }

        bool isContinuous = registration_.terminal == ActionTerminal::Continuous;

        // Single-frame conditions with continuous mode is usually a mistake
        if (isContinuous && (hasPressed || hasReleased || hasHeld)) {
            spdlog::warn("ActionBuilder: Using continuous mode with single-frame condition "
                        "(whenPressed/whenReleased/whenHeld). This may not behave as expected.");
        }

        // Phase effects with continuous mode will spam
        for (const auto& effect : registration_.effects) {
            if (isContinuous &&
                (effect.type == ActionEffectType::PushPhase ||
                 effect.type == ActionEffectType::PopPhase ||
                 effect.type == ActionEffectType::ChangePhase)) {
                spdlog::warn("ActionBuilder: Using continuous mode with phase effect. "
                            "This will spam phase changes every frame while conditions are true.");
            }
        }

        // Multiple whenPressed on different inputs rarely fires
        if (registration_.conditions.size() > 1) {
            int pressedCount = 0;
            for (const auto& cond : registration_.conditions) {
                if (cond.type == ActionConditionType::WhenPressed) {
                    pressedCount++;
                }
            }
            if (pressedCount > 1) {
                spdlog::warn("ActionBuilder: Multiple whenPressed conditions on different inputs "
                            "require all inputs to be pressed on the exact same frame.");
            }
        }

        // Very short hold threshold
        for (const auto& cond : registration_.conditions) {
            if (cond.type == ActionConditionType::WhenHeld &&
                cond.holdThreshold.has_value() &&
                cond.holdThreshold.value() < 0.1f) {
                spdlog::warn("ActionBuilder: Hold threshold less than 0.1s may be indistinguishable "
                            "from whenActive. Consider using whenActive instead.");
            }
        }
    }

    /// Serialize an InputBinding to a string key (for deduplication)
    std::string serializeBinding(const InputBinding& binding) const {
        std::string key = std::to_string(static_cast<int>(binding.source));
        key += ":";
        key += std::to_string(binding.deviceIndex);
        key += ":";

        std::visit([&key](auto&& arg) {
            using T = std::decay_t<decltype(arg)>;
            if constexpr (std::is_same_v<T, KeyCode>) {
                key += "key:" + std::to_string(static_cast<int>(arg));
            } else if constexpr (std::is_same_v<T, MouseButton>) {
                key += "mouse:" + std::to_string(static_cast<int>(arg));
            } else if constexpr (std::is_same_v<T, GamepadButton>) {
                key += "gpadbtn:" + std::to_string(static_cast<int>(arg));
            } else if constexpr (std::is_same_v<T, GamepadAxis>) {
                key += "gpadaxis:" + std::to_string(static_cast<int>(arg));
            }
        }, binding.input);

        return key;
    }

    IInputSystem* inputSystem_ = nullptr;
    ActionRegistration registration_;
};

//=============================================================================
// Lua Binding Function
//=============================================================================

void bindActionBuilder(sol::state& lua, IInputSystem& input) {
    //=========================================================================
    // Platform-agnostic input enums
    //=========================================================================

    // KeyCode enum - replaces raw GLFW key codes
    lua.new_enum<KeyCode>("KeyCode",
        {
            // Letters
            {"A", KeyCode::A}, {"B", KeyCode::B}, {"C", KeyCode::C}, {"D", KeyCode::D},
            {"E", KeyCode::E}, {"F", KeyCode::F}, {"G", KeyCode::G}, {"H", KeyCode::H},
            {"I", KeyCode::I}, {"J", KeyCode::J}, {"K", KeyCode::K}, {"L", KeyCode::L},
            {"M", KeyCode::M}, {"N", KeyCode::N}, {"O", KeyCode::O}, {"P", KeyCode::P},
            {"Q", KeyCode::Q}, {"R", KeyCode::R}, {"S", KeyCode::S}, {"T", KeyCode::T},
            {"U", KeyCode::U}, {"V", KeyCode::V}, {"W", KeyCode::W}, {"X", KeyCode::X},
            {"Y", KeyCode::Y}, {"Z", KeyCode::Z},

            // Numbers
            {"Num0", KeyCode::Num0}, {"Num1", KeyCode::Num1}, {"Num2", KeyCode::Num2},
            {"Num3", KeyCode::Num3}, {"Num4", KeyCode::Num4}, {"Num5", KeyCode::Num5},
            {"Num6", KeyCode::Num6}, {"Num7", KeyCode::Num7}, {"Num8", KeyCode::Num8},
            {"Num9", KeyCode::Num9},

            // Function keys
            {"F1", KeyCode::F1}, {"F2", KeyCode::F2}, {"F3", KeyCode::F3}, {"F4", KeyCode::F4},
            {"F5", KeyCode::F5}, {"F6", KeyCode::F6}, {"F7", KeyCode::F7}, {"F8", KeyCode::F8},
            {"F9", KeyCode::F9}, {"F10", KeyCode::F10}, {"F11", KeyCode::F11}, {"F12", KeyCode::F12},

            // Special keys
            {"Space", KeyCode::Space}, {"Enter", KeyCode::Enter}, {"Escape", KeyCode::Escape},
            {"Tab", KeyCode::Tab}, {"Backspace", KeyCode::Backspace}, {"Delete", KeyCode::Delete},
            {"Insert", KeyCode::Insert},
            {"Up", KeyCode::Up}, {"Down", KeyCode::Down}, {"Left", KeyCode::Left}, {"Right", KeyCode::Right},
            {"Home", KeyCode::Home}, {"End", KeyCode::End}, {"PageUp", KeyCode::PageUp}, {"PageDown", KeyCode::PageDown},

            // Modifiers
            {"LeftShift", KeyCode::LeftShift}, {"RightShift", KeyCode::RightShift},
            {"LeftCtrl", KeyCode::LeftCtrl}, {"RightCtrl", KeyCode::RightCtrl},
            {"LeftAlt", KeyCode::LeftAlt}, {"RightAlt", KeyCode::RightAlt},
            {"LeftSuper", KeyCode::LeftSuper}, {"RightSuper", KeyCode::RightSuper},

            // Punctuation
            {"Apostrophe", KeyCode::Apostrophe}, {"Comma", KeyCode::Comma},
            {"Minus", KeyCode::Minus}, {"Period", KeyCode::Period}, {"Slash", KeyCode::Slash},
            {"Semicolon", KeyCode::Semicolon}, {"Equal", KeyCode::Equal},
            {"LeftBracket", KeyCode::LeftBracket}, {"RightBracket", KeyCode::RightBracket},
            {"Backslash", KeyCode::Backslash}, {"GraveAccent", KeyCode::GraveAccent},

            // Numpad
            {"KP0", KeyCode::KP0}, {"KP1", KeyCode::KP1}, {"KP2", KeyCode::KP2}, {"KP3", KeyCode::KP3},
            {"KP4", KeyCode::KP4}, {"KP5", KeyCode::KP5}, {"KP6", KeyCode::KP6}, {"KP7", KeyCode::KP7},
            {"KP8", KeyCode::KP8}, {"KP9", KeyCode::KP9},
            {"KPDecimal", KeyCode::KPDecimal}, {"KPDivide", KeyCode::KPDivide},
            {"KPMultiply", KeyCode::KPMultiply}, {"KPSubtract", KeyCode::KPSubtract},
            {"KPAdd", KeyCode::KPAdd}, {"KPEnter", KeyCode::KPEnter}, {"KPEqual", KeyCode::KPEqual},

            // Other
            {"CapsLock", KeyCode::CapsLock}, {"ScrollLock", KeyCode::ScrollLock},
            {"NumLock", KeyCode::NumLock}, {"PrintScreen", KeyCode::PrintScreen}, {"Pause", KeyCode::Pause}
        }
    );

    // MouseButton enum
    lua.new_enum<MouseButton>("MouseButton",
        {
            {"Left", MouseButton::Left}, {"Right", MouseButton::Right}, {"Middle", MouseButton::Middle},
            {"Button4", MouseButton::Button4}, {"Button5", MouseButton::Button5},
            {"Button6", MouseButton::Button6}, {"Button7", MouseButton::Button7}, {"Button8", MouseButton::Button8}
        }
    );

    // GamepadButton enum
    lua.new_enum<GamepadButton>("GamepadButton",
        {
            {"A", GamepadButton::A}, {"B", GamepadButton::B},
            {"X", GamepadButton::X}, {"Y", GamepadButton::Y},
            {"LeftBumper", GamepadButton::LeftBumper}, {"RightBumper", GamepadButton::RightBumper},
            {"Back", GamepadButton::Back}, {"Start", GamepadButton::Start}, {"Guide", GamepadButton::Guide},
            {"LeftThumb", GamepadButton::LeftThumb}, {"RightThumb", GamepadButton::RightThumb},
            {"DPadUp", GamepadButton::DPadUp}, {"DPadDown", GamepadButton::DPadDown},
            {"DPadLeft", GamepadButton::DPadLeft}, {"DPadRight", GamepadButton::DPadRight},
            {"LeftTrigger", GamepadButton::LeftTrigger}, {"RightTrigger", GamepadButton::RightTrigger}
        }
    );

    // GamepadAxis enum
    lua.new_enum<GamepadAxis>("GamepadAxis",
        {
            {"LeftX", GamepadAxis::LeftX}, {"LeftY", GamepadAxis::LeftY},
            {"RightX", GamepadAxis::RightX}, {"RightY", GamepadAxis::RightY},
            {"LeftTrigger", GamepadAxis::LeftTrigger}, {"RightTrigger", GamepadAxis::RightTrigger}
        }
    );

    // InputState enum
    lua.new_enum<InputState>("InputState",
        {
            {"NotPressed", InputState::NotPressed},
            {"JustPressed", InputState::JustPressed},
            {"Pressed", InputState::Pressed},
            {"Held", InputState::Held},
            {"JustReleased", InputState::JustReleased}
        }
    );

    // InputSource enum
    lua.new_enum<InputSource>("InputSource",
        {
            {"Keyboard", InputSource::Keyboard},
            {"Mouse", InputSource::Mouse},
            {"Gamepad", InputSource::Gamepad}
        }
    );

    //=========================================================================
    // InputBinding struct (updated for new variant-based design)
    //=========================================================================

    lua.new_usertype<InputBinding>("InputBinding",
        sol::constructors<InputBinding()>(),
        "source", &InputBinding::source,
        "deviceIndex", &InputBinding::deviceIndex,
        "requiredModifiers", &InputBinding::requiredModifiers,
        "scale", &InputBinding::scale,
        "deadzone", &InputBinding::deadzone,
        "isAxis", &InputBinding::isAxis,
        "isButton", &InputBinding::isButton,
        // Factory methods
        "key", sol::overload(
            [](KeyCode k) { return InputBinding::key(k); },
            [](KeyCode k, ModifierKey mods) { return InputBinding::key(k, mods); }
        ),
        "mouseButton", [](MouseButton btn) { return InputBinding::mouseButton(btn); },
        "gamepadButton", sol::overload(
            [](GamepadButton btn) { return InputBinding::gamepadButton(btn); },
            [](GamepadButton btn, int index) { return InputBinding::gamepadButton(btn, index); }
        ),
        "gamepadAxis", sol::overload(
            [](GamepadAxis axis) { return InputBinding::gamepadAxis(axis); },
            [](GamepadAxis axis, int index) { return InputBinding::gamepadAxis(axis, index); },
            [](GamepadAxis axis, int index, float scale) { return InputBinding::gamepadAxis(axis, index, scale); },
            [](GamepadAxis axis, int index, float scale, float deadzone) {
                return InputBinding::gamepadAxis(axis, index, scale, deadzone);
            }
        )
    );

    //=========================================================================
    // ActionBuilder usertype
    //=========================================================================

    lua.new_usertype<ActionBuilder>("ActionBuilder",
        sol::no_constructor,
        "duringPhase", &ActionBuilder::duringPhase,
        "whenPressed", &ActionBuilder::whenPressed,
        "whenReleased", &ActionBuilder::whenReleased,
        "whenActive", &ActionBuilder::whenActive,
        "whenInactive", &ActionBuilder::whenInactive,
        "whenHeld", sol::overload(
            [](ActionBuilder& b, sol::object input) -> ActionBuilder& {
                return b.whenHeld(input, std::nullopt);
            },
            [](ActionBuilder& b, sol::object input, float threshold) -> ActionBuilder& {
                return b.whenHeld(input, threshold);
            }
        ),
        "withDeadzone", &ActionBuilder::withDeadzone,
        "emitAction", &ActionBuilder::emitAction,
        "pushPhase", &ActionBuilder::pushPhase,
        "popPhase", &ActionBuilder::popPhase,
        "changePhase", &ActionBuilder::changePhase,
        "discretely", &ActionBuilder::discretely,
        "continuously", &ActionBuilder::continuously
    );

    //=========================================================================
    // bestow.action table
    //=========================================================================

    sol::table bestow = lua["bestow"];
    sol::table actionTable = lua.create_table();

    // Factory for creating new builders
    actionTable["builder"] = [&input]() {
        return ActionBuilder(&input);
    };

    bestow["action"] = actionTable;

    //=========================================================================
    // bestow.input.keys, bestow.input.buttons, bestow.input.axes tables
    // (Platform-agnostic replacements for old bestow.input.Key table)
    //=========================================================================

    sol::table inputTable = bestow["input"];

    // keys table
    sol::table keysTable = lua.create_table();
    keysTable["A"] = KeyCode::A; keysTable["B"] = KeyCode::B; keysTable["C"] = KeyCode::C;
    keysTable["D"] = KeyCode::D; keysTable["E"] = KeyCode::E; keysTable["F"] = KeyCode::F;
    keysTable["G"] = KeyCode::G; keysTable["H"] = KeyCode::H; keysTable["I"] = KeyCode::I;
    keysTable["J"] = KeyCode::J; keysTable["K"] = KeyCode::K; keysTable["L"] = KeyCode::L;
    keysTable["M"] = KeyCode::M; keysTable["N"] = KeyCode::N; keysTable["O"] = KeyCode::O;
    keysTable["P"] = KeyCode::P; keysTable["Q"] = KeyCode::Q; keysTable["R"] = KeyCode::R;
    keysTable["S"] = KeyCode::S; keysTable["T"] = KeyCode::T; keysTable["U"] = KeyCode::U;
    keysTable["V"] = KeyCode::V; keysTable["W"] = KeyCode::W; keysTable["X"] = KeyCode::X;
    keysTable["Y"] = KeyCode::Y; keysTable["Z"] = KeyCode::Z;
    keysTable["Num0"] = KeyCode::Num0; keysTable["Num1"] = KeyCode::Num1;
    keysTable["Num2"] = KeyCode::Num2; keysTable["Num3"] = KeyCode::Num3;
    keysTable["Num4"] = KeyCode::Num4; keysTable["Num5"] = KeyCode::Num5;
    keysTable["Num6"] = KeyCode::Num6; keysTable["Num7"] = KeyCode::Num7;
    keysTable["Num8"] = KeyCode::Num8; keysTable["Num9"] = KeyCode::Num9;
    keysTable["F1"] = KeyCode::F1; keysTable["F2"] = KeyCode::F2;
    keysTable["F3"] = KeyCode::F3; keysTable["F4"] = KeyCode::F4;
    keysTable["F5"] = KeyCode::F5; keysTable["F6"] = KeyCode::F6;
    keysTable["F7"] = KeyCode::F7; keysTable["F8"] = KeyCode::F8;
    keysTable["F9"] = KeyCode::F9; keysTable["F10"] = KeyCode::F10;
    keysTable["F11"] = KeyCode::F11; keysTable["F12"] = KeyCode::F12;
    keysTable["Space"] = KeyCode::Space; keysTable["Enter"] = KeyCode::Enter;
    keysTable["Escape"] = KeyCode::Escape; keysTable["Tab"] = KeyCode::Tab;
    keysTable["Backspace"] = KeyCode::Backspace; keysTable["Delete"] = KeyCode::Delete;
    keysTable["Insert"] = KeyCode::Insert;
    keysTable["Up"] = KeyCode::Up; keysTable["Down"] = KeyCode::Down;
    keysTable["Left"] = KeyCode::Left; keysTable["Right"] = KeyCode::Right;
    keysTable["Home"] = KeyCode::Home; keysTable["End"] = KeyCode::End;
    keysTable["PageUp"] = KeyCode::PageUp; keysTable["PageDown"] = KeyCode::PageDown;
    keysTable["LeftShift"] = KeyCode::LeftShift; keysTable["RightShift"] = KeyCode::RightShift;
    keysTable["LeftCtrl"] = KeyCode::LeftCtrl; keysTable["RightCtrl"] = KeyCode::RightCtrl;
    keysTable["LeftAlt"] = KeyCode::LeftAlt; keysTable["RightAlt"] = KeyCode::RightAlt;
    keysTable["LeftSuper"] = KeyCode::LeftSuper; keysTable["RightSuper"] = KeyCode::RightSuper;
    keysTable["Apostrophe"] = KeyCode::Apostrophe; keysTable["Comma"] = KeyCode::Comma;
    keysTable["Minus"] = KeyCode::Minus; keysTable["Period"] = KeyCode::Period;
    keysTable["Slash"] = KeyCode::Slash; keysTable["Semicolon"] = KeyCode::Semicolon;
    keysTable["Equal"] = KeyCode::Equal; keysTable["LeftBracket"] = KeyCode::LeftBracket;
    keysTable["RightBracket"] = KeyCode::RightBracket; keysTable["Backslash"] = KeyCode::Backslash;
    keysTable["GraveAccent"] = KeyCode::GraveAccent;
    keysTable["CapsLock"] = KeyCode::CapsLock; keysTable["ScrollLock"] = KeyCode::ScrollLock;
    keysTable["NumLock"] = KeyCode::NumLock; keysTable["PrintScreen"] = KeyCode::PrintScreen;
    keysTable["Pause"] = KeyCode::Pause;
    inputTable["keys"] = keysTable;

    // buttons table (gamepad)
    sol::table buttonsTable = lua.create_table();
    buttonsTable["A"] = GamepadButton::A; buttonsTable["B"] = GamepadButton::B;
    buttonsTable["X"] = GamepadButton::X; buttonsTable["Y"] = GamepadButton::Y;
    buttonsTable["LeftBumper"] = GamepadButton::LeftBumper;
    buttonsTable["RightBumper"] = GamepadButton::RightBumper;
    buttonsTable["LB"] = GamepadButton::LeftBumper;  // Alias
    buttonsTable["RB"] = GamepadButton::RightBumper; // Alias
    buttonsTable["Back"] = GamepadButton::Back; buttonsTable["Start"] = GamepadButton::Start;
    buttonsTable["Guide"] = GamepadButton::Guide;
    buttonsTable["LeftThumb"] = GamepadButton::LeftThumb;
    buttonsTable["RightThumb"] = GamepadButton::RightThumb;
    buttonsTable["LS"] = GamepadButton::LeftThumb;   // Alias
    buttonsTable["RS"] = GamepadButton::RightThumb;  // Alias
    buttonsTable["DPadUp"] = GamepadButton::DPadUp;
    buttonsTable["DPadDown"] = GamepadButton::DPadDown;
    buttonsTable["DPadLeft"] = GamepadButton::DPadLeft;
    buttonsTable["DPadRight"] = GamepadButton::DPadRight;
    buttonsTable["LeftTrigger"] = GamepadButton::LeftTrigger;
    buttonsTable["RightTrigger"] = GamepadButton::RightTrigger;
    buttonsTable["LT"] = GamepadButton::LeftTrigger;  // Alias
    buttonsTable["RT"] = GamepadButton::RightTrigger; // Alias
    inputTable["buttons"] = buttonsTable;

    // axes table (gamepad analog)
    sol::table axesTable = lua.create_table();
    axesTable["LeftX"] = GamepadAxis::LeftX;
    axesTable["LeftY"] = GamepadAxis::LeftY;
    axesTable["RightX"] = GamepadAxis::RightX;
    axesTable["RightY"] = GamepadAxis::RightY;
    axesTable["LeftTrigger"] = GamepadAxis::LeftTrigger;
    axesTable["RightTrigger"] = GamepadAxis::RightTrigger;
    axesTable["LT"] = GamepadAxis::LeftTrigger;  // Alias
    axesTable["RT"] = GamepadAxis::RightTrigger; // Alias
    inputTable["axes"] = axesTable;

    // mouse table (for mouse buttons in spec naming)
    sol::table mouseTable = lua.create_table();
    mouseTable["Left"] = MouseButton::Left;
    mouseTable["Right"] = MouseButton::Right;
    mouseTable["Middle"] = MouseButton::Middle;
    mouseTable["Button4"] = MouseButton::Button4;
    mouseTable["Button5"] = MouseButton::Button5;
    inputTable["mouse"] = mouseTable;

    //=========================================================================
    // bestow.phase table (phase management)
    //=========================================================================

    sol::table phaseTable = lua.create_table();

    phaseTable["current"] = [&input]() {
        return input.getCurrentPhase();
    };

    phaseTable["stack"] = [&input]() {
        return input.getPhaseStack();
    };

    phaseTable["push"] = [&input](const std::string& phase) {
        input.pushPhase(phase);
    };

    phaseTable["pop"] = [&input]() {
        input.popPhase();
    };

    phaseTable["change"] = [&input](const std::string& phase) {
        input.changePhase(phase);
    };

    phaseTable["isActive"] = [&input](const std::string& phase) {
        return input.isPhaseActive(phase);
    };

    bestow["phase"] = phaseTable;

    //=========================================================================
    // Event-driven input helpers
    //=========================================================================

    // Helper for subscribing to specific actions
    // Usage: bestow.input.onAction("Jump", function(data) ... end)
    inputTable["onAction"] = [&lua](const std::string& /*actionName*/, sol::function /*callback*/) {
        // This will be connected to EventSystem in the full binding
        // For now, just log that it was called
        spdlog::warn("bestow.input.onAction: EventSystem integration pending - use bestow.events.subscribe() directly");
        return 0;  // Return subscription ID (placeholder)
    };

    //=========================================================================
    // Platform-agnostic input state queries (new API)
    //=========================================================================

    inputTable["isKeyDown"] = [&input](KeyCode key) {
        return input.isKeyDown(key);
    };

    inputTable["wasKeyJustPressed"] = [&input](KeyCode key) {
        return input.wasKeyJustPressed(key);
    };

    inputTable["wasKeyJustReleased"] = [&input](KeyCode key) {
        return input.wasKeyJustReleased(key);
    };

    inputTable["isMouseButtonDown"] = [&input](MouseButton button) {
        return input.isMouseButtonDown(button);
    };

    inputTable["wasMouseButtonJustPressed"] = [&input](MouseButton button) {
        return input.wasMouseButtonJustPressed(button);
    };

    inputTable["wasMouseButtonJustReleased"] = [&input](MouseButton button) {
        return input.wasMouseButtonJustReleased(button);
    };

    inputTable["isGamepadButtonDown"] = sol::overload(
        [&input](GamepadButton button) {
            return input.isGamepadButtonDown(button);
        },
        [&input](GamepadButton button, int gamepadIndex) {
            return input.isGamepadButtonDown(button, gamepadIndex);
        }
    );

    inputTable["wasGamepadButtonJustPressed"] = sol::overload(
        [&input](GamepadButton button) {
            return input.wasGamepadButtonJustPressed(button);
        },
        [&input](GamepadButton button, int gamepadIndex) {
            return input.wasGamepadButtonJustPressed(button, gamepadIndex);
        }
    );

    inputTable["wasGamepadButtonJustReleased"] = sol::overload(
        [&input](GamepadButton button) {
            return input.wasGamepadButtonJustReleased(button);
        },
        [&input](GamepadButton button, int gamepadIndex) {
            return input.wasGamepadButtonJustReleased(button, gamepadIndex);
        }
    );

    inputTable["getGamepadAxisValue"] = sol::overload(
        [&input](GamepadAxis axis) {
            return input.getGamepadAxisValue(axis);
        },
        [&input](GamepadAxis axis, int gamepadIndex) {
            return input.getGamepadAxisValue(axis, gamepadIndex);
        }
    );

    inputTable["getLeftStick"] = sol::overload(
        [&input]() { return input.getLeftStick(); },
        [&input](int gamepadIndex) { return input.getLeftStick(gamepadIndex); }
    );

    inputTable["getRightStick"] = sol::overload(
        [&input]() { return input.getRightStick(); },
        [&input](int gamepadIndex) { return input.getRightStick(gamepadIndex); }
    );

    inputTable["getInputState"] = [&input](const InputBinding& binding) {
        return input.getInputState(binding);
    };

    inputTable["getInputHoldDuration"] = [&input](const InputBinding& binding) {
        return input.getInputHoldDuration(binding);
    };

    inputTable["setDefaultHoldThreshold"] = [&input](float seconds) {
        input.setDefaultHoldThreshold(seconds);
    };

    inputTable["getDefaultHoldThreshold"] = [&input]() {
        return input.getDefaultHoldThreshold();
    };

    spdlog::debug("ActionBuilder and event-driven input API bound to Lua");
}

}  // namespace bestow

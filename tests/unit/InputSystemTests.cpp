// tests/unit/InputSystemTests.cpp
// Input system unit tests - includes both legacy and event-driven API tests

#include <memory>
#include <string>
#include <vector>

#include <gtest/gtest.h>
#include <kangaru/kangaru.hpp>

import bestow.input;
import bestow.input.impl;
import bestow.types;
import bestow.events;
import bestow.assets.impl;   // InputSystem depends on AssetSystem
import bestow.events.impl;   // AssetSystem depends on EventSystem

namespace bestow::tests {

//=============================================================================
// Test Fixture
//=============================================================================

class InputSystemTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create with event system for action event emission
        eventSystem_ = std::make_unique<EventSystem>();
        inputSystem_ = std::make_unique<InputSystem>(eventSystem_.get());
    }

    std::unique_ptr<IEventSystem> eventSystem_;
    std::unique_ptr<IInputSystem> inputSystem_;
};

//=============================================================================
// PhaseTree Utility Tests
//=============================================================================

TEST(PhaseTreeTest, IsDescendantOrSame_SamePhase) {
    EXPECT_TRUE(PhaseTree::isDescendantOrSame("game", "game"));
    EXPECT_TRUE(PhaseTree::isDescendantOrSame("menu", "menu"));
    EXPECT_TRUE(PhaseTree::isDescendantOrSame("game.melee", "game.melee"));
}

TEST(PhaseTreeTest, IsDescendantOrSame_DirectDescendant) {
    EXPECT_TRUE(PhaseTree::isDescendantOrSame("game.melee", "game"));
    EXPECT_TRUE(PhaseTree::isDescendantOrSame("menu.settings", "menu"));
}

TEST(PhaseTreeTest, IsDescendantOrSame_NestedDescendant) {
    EXPECT_TRUE(PhaseTree::isDescendantOrSame("game.melee.combo", "game"));
    EXPECT_TRUE(PhaseTree::isDescendantOrSame("game.melee.combo", "game.melee"));
    EXPECT_TRUE(PhaseTree::isDescendantOrSame("menu.settings.audio.volume", "menu"));
}

TEST(PhaseTreeTest, IsDescendantOrSame_NotDescendant) {
    EXPECT_FALSE(PhaseTree::isDescendantOrSame("game", "menu"));
    EXPECT_FALSE(PhaseTree::isDescendantOrSame("game.melee", "game.ranged"));
    EXPECT_FALSE(PhaseTree::isDescendantOrSame("menu", "game"));
}

TEST(PhaseTreeTest, IsDescendantOrSame_ParentIsNotDescendant) {
    EXPECT_FALSE(PhaseTree::isDescendantOrSame("game", "game.melee"));
    EXPECT_FALSE(PhaseTree::isDescendantOrSame("menu", "menu.settings"));
}

TEST(PhaseTreeTest, IsDescendantOrSame_PartialMatch) {
    // "gameplay" should NOT be a descendant of "game"
    EXPECT_FALSE(PhaseTree::isDescendantOrSame("gameplay", "game"));
    EXPECT_FALSE(PhaseTree::isDescendantOrSame("game_over", "game"));
}

TEST(PhaseTreeTest, GetParent_SingleLevel) {
    EXPECT_EQ(PhaseTree::getParent("game"), "");
    EXPECT_EQ(PhaseTree::getParent("menu"), "");
}

TEST(PhaseTreeTest, GetParent_TwoLevels) {
    EXPECT_EQ(PhaseTree::getParent("game.melee"), "game");
    EXPECT_EQ(PhaseTree::getParent("menu.settings"), "menu");
}

TEST(PhaseTreeTest, GetParent_ThreeLevels) {
    EXPECT_EQ(PhaseTree::getParent("game.melee.combo"), "game.melee");
    EXPECT_EQ(PhaseTree::getParent("menu.settings.audio"), "menu.settings");
}

TEST(PhaseTreeTest, GetParent_EmptyPhase) {
    EXPECT_EQ(PhaseTree::getParent(""), "");
}

TEST(PhaseTreeTest, GetAncestors_SingleLevel) {
    auto ancestors = PhaseTree::getAncestors("game");
    ASSERT_EQ(ancestors.size(), 1);
    EXPECT_EQ(ancestors[0], "game");
}

TEST(PhaseTreeTest, GetAncestors_TwoLevels) {
    auto ancestors = PhaseTree::getAncestors("game.melee");
    ASSERT_EQ(ancestors.size(), 2);
    EXPECT_EQ(ancestors[0], "game.melee");
    EXPECT_EQ(ancestors[1], "game");
}

TEST(PhaseTreeTest, GetAncestors_ThreeLevels) {
    auto ancestors = PhaseTree::getAncestors("game.melee.combo");
    ASSERT_EQ(ancestors.size(), 3);
    EXPECT_EQ(ancestors[0], "game.melee.combo");
    EXPECT_EQ(ancestors[1], "game.melee");
    EXPECT_EQ(ancestors[2], "game");
}

TEST(PhaseTreeTest, GetAncestors_EmptyPhase) {
    auto ancestors = PhaseTree::getAncestors("");
    EXPECT_TRUE(ancestors.empty());
}

TEST(PhaseTreeTest, IsValidPhase_ValidPhases) {
    EXPECT_TRUE(PhaseTree::isValidPhase("game"));
    EXPECT_TRUE(PhaseTree::isValidPhase("menu"));
    EXPECT_TRUE(PhaseTree::isValidPhase("game.melee"));
    EXPECT_TRUE(PhaseTree::isValidPhase("game.melee.combo"));
    EXPECT_TRUE(PhaseTree::isValidPhase("game_over"));
    EXPECT_TRUE(PhaseTree::isValidPhase("level_1"));
}

TEST(PhaseTreeTest, IsValidPhase_InvalidPhases) {
    EXPECT_FALSE(PhaseTree::isValidPhase(""));
    EXPECT_FALSE(PhaseTree::isValidPhase(".game"));
    EXPECT_FALSE(PhaseTree::isValidPhase("game."));
    EXPECT_FALSE(PhaseTree::isValidPhase("game..melee"));
    EXPECT_FALSE(PhaseTree::isValidPhase("game melee"));  // space
}

//=============================================================================
// InputBinding Factory Tests
//=============================================================================

TEST(InputBindingTest, KeyFactory) {
    auto binding = InputBinding::key(KeyCode::Space);
    EXPECT_EQ(binding.source, InputSource::Keyboard);
    EXPECT_EQ(binding.deviceIndex, 0);
    EXPECT_TRUE(std::holds_alternative<KeyCode>(binding.input));
    EXPECT_EQ(std::get<KeyCode>(binding.input), KeyCode::Space);
}

TEST(InputBindingTest, KeyFactoryWithModifiers) {
    auto binding = InputBinding::key(KeyCode::S, ModifierKey::Ctrl);
    EXPECT_EQ(binding.source, InputSource::Keyboard);
    EXPECT_EQ(std::get<KeyCode>(binding.input), KeyCode::S);
    EXPECT_EQ(binding.requiredModifiers, ModifierKey::Ctrl);
}

TEST(InputBindingTest, MouseButtonFactory) {
    auto binding = InputBinding::mouseButton(MouseButton::Left);
    EXPECT_EQ(binding.source, InputSource::Mouse);
    EXPECT_TRUE(std::holds_alternative<MouseButton>(binding.input));
    EXPECT_EQ(std::get<MouseButton>(binding.input), MouseButton::Left);
}

TEST(InputBindingTest, GamepadButtonFactory) {
    auto binding = InputBinding::gamepadButton(GamepadButton::A);
    EXPECT_EQ(binding.source, InputSource::Gamepad);
    EXPECT_EQ(binding.deviceIndex, 0);
    EXPECT_TRUE(std::holds_alternative<GamepadButton>(binding.input));
    EXPECT_EQ(std::get<GamepadButton>(binding.input), GamepadButton::A);
}

TEST(InputBindingTest, GamepadButtonFactoryWithIndex) {
    auto binding = InputBinding::gamepadButton(GamepadButton::X, 2);
    EXPECT_EQ(binding.source, InputSource::Gamepad);
    EXPECT_EQ(binding.deviceIndex, 2);
    EXPECT_EQ(std::get<GamepadButton>(binding.input), GamepadButton::X);
}

TEST(InputBindingTest, GamepadAxisFactory) {
    auto binding = InputBinding::gamepadAxis(GamepadAxis::LeftX);
    EXPECT_EQ(binding.source, InputSource::Gamepad);
    EXPECT_TRUE(std::holds_alternative<GamepadAxis>(binding.input));
    EXPECT_EQ(std::get<GamepadAxis>(binding.input), GamepadAxis::LeftX);
    EXPECT_EQ(binding.deadzone, 0.15f);  // default
}

TEST(InputBindingTest, GamepadAxisFactoryWithOptions) {
    auto binding = InputBinding::gamepadAxis(GamepadAxis::RightY, 1, -1.0f, 0.2f);
    EXPECT_EQ(binding.source, InputSource::Gamepad);
    EXPECT_EQ(binding.deviceIndex, 1);
    EXPECT_EQ(std::get<GamepadAxis>(binding.input), GamepadAxis::RightY);
    EXPECT_EQ(binding.scale, -1.0f);  // inverted
    EXPECT_EQ(binding.deadzone, 0.2f);
}

TEST(InputBindingTest, IsAxis) {
    auto keyBinding = InputBinding::key(KeyCode::A);
    auto axisBinding = InputBinding::gamepadAxis(GamepadAxis::LeftX);

    EXPECT_FALSE(keyBinding.isAxis());
    EXPECT_TRUE(axisBinding.isAxis());
}

TEST(InputBindingTest, IsButton) {
    auto keyBinding = InputBinding::key(KeyCode::A);
    auto buttonBinding = InputBinding::gamepadButton(GamepadButton::A);
    auto axisBinding = InputBinding::gamepadAxis(GamepadAxis::LeftX);

    EXPECT_TRUE(keyBinding.isButton());
    EXPECT_TRUE(buttonBinding.isButton());
    EXPECT_FALSE(axisBinding.isButton());
}

//=============================================================================
// Phase Management Tests
//=============================================================================

TEST_F(InputSystemTest, InitiallyNoPhaseSet) {
    EXPECT_FALSE(inputSystem_->hasPhaseBeenSet());
    EXPECT_TRUE(inputSystem_->getCurrentPhase().empty());
}

TEST_F(InputSystemTest, ChangePhase) {
    inputSystem_->changePhase("game");
    EXPECT_TRUE(inputSystem_->hasPhaseBeenSet());
    EXPECT_EQ(inputSystem_->getCurrentPhase(), "game");
}

TEST_F(InputSystemTest, ChangePhaseReplacesStack) {
    inputSystem_->changePhase("menu");
    inputSystem_->pushPhase("settings");
    inputSystem_->changePhase("game");

    auto stack = inputSystem_->getPhaseStack();
    ASSERT_EQ(stack.size(), 1);
    EXPECT_EQ(stack[0], "game");
}

TEST_F(InputSystemTest, PushPhase) {
    inputSystem_->changePhase("game");
    inputSystem_->pushPhase("game.melee");

    EXPECT_EQ(inputSystem_->getCurrentPhase(), "game.melee");

    auto stack = inputSystem_->getPhaseStack();
    ASSERT_EQ(stack.size(), 2);
    EXPECT_EQ(stack[0], "game");
    EXPECT_EQ(stack[1], "game.melee");
}

TEST_F(InputSystemTest, PopPhase) {
    inputSystem_->changePhase("game");
    inputSystem_->pushPhase("game.melee");
    inputSystem_->popPhase();

    EXPECT_EQ(inputSystem_->getCurrentPhase(), "game");

    auto stack = inputSystem_->getPhaseStack();
    ASSERT_EQ(stack.size(), 1);
}

TEST_F(InputSystemTest, PopPhaseOnEmptyStackDoesNothing) {
    inputSystem_->changePhase("game");
    inputSystem_->popPhase();

    // Should still have the base phase
    EXPECT_EQ(inputSystem_->getCurrentPhase(), "game");
}

TEST_F(InputSystemTest, IsPhaseActive_CurrentPhase) {
    inputSystem_->changePhase("game.melee");
    EXPECT_TRUE(inputSystem_->isPhaseActive("game.melee"));
}

TEST_F(InputSystemTest, IsPhaseActive_AncestorPhase) {
    inputSystem_->changePhase("game.melee.combo");
    EXPECT_TRUE(inputSystem_->isPhaseActive("game.melee"));
    EXPECT_TRUE(inputSystem_->isPhaseActive("game"));
}

TEST_F(InputSystemTest, IsPhaseActive_InactivePhase) {
    inputSystem_->changePhase("game");
    EXPECT_FALSE(inputSystem_->isPhaseActive("menu"));
    EXPECT_FALSE(inputSystem_->isPhaseActive("game.melee"));
}

TEST_F(InputSystemTest, IsPhaseActive_StackedPhases) {
    inputSystem_->changePhase("game");
    inputSystem_->pushPhase("game.pause");

    // Both phases in stack should be active
    EXPECT_TRUE(inputSystem_->isPhaseActive("game.pause"));
    EXPECT_TRUE(inputSystem_->isPhaseActive("game"));
}

//=============================================================================
// Action Registration Tests
//=============================================================================

TEST_F(InputSystemTest, RegisterAction) {
    ActionRegistration reg;
    reg.phase = "game";
    reg.conditions.push_back({
        .type = ActionConditionType::WhenPressed,
        .input = InputBinding::key(KeyCode::Space)
    });
    reg.effects.push_back({
        .type = ActionEffectType::EmitAction,
        .value = "Jump"
    });
    reg.terminal = ActionTerminal::Discrete;
    reg.valid = true;

    inputSystem_->registerAction(reg);

    auto actions = inputSystem_->getActions();
    ASSERT_EQ(actions.size(), 1);
    EXPECT_EQ(actions[0].phase, "game");
}

TEST_F(InputSystemTest, RegisterMultipleActions) {
    ActionRegistration jump;
    jump.phase = "game";
    jump.conditions.push_back({ActionConditionType::WhenPressed, InputBinding::key(KeyCode::Space)});
    jump.effects.push_back({ActionEffectType::EmitAction, "Jump"});
    jump.valid = true;

    ActionRegistration attack;
    attack.phase = "game";
    attack.conditions.push_back({ActionConditionType::WhenPressed, InputBinding::key(KeyCode::F)});
    attack.effects.push_back({ActionEffectType::EmitAction, "Attack"});
    attack.valid = true;

    inputSystem_->registerAction(jump);
    inputSystem_->registerAction(attack);

    auto actions = inputSystem_->getActions();
    EXPECT_EQ(actions.size(), 2);
}

TEST_F(InputSystemTest, UnregisterAction) {
    ActionRegistration reg;
    reg.phase = "game";
    reg.conditions.push_back({ActionConditionType::WhenPressed, InputBinding::key(KeyCode::Space)});
    reg.effects.push_back({ActionEffectType::EmitAction, "Jump"});
    reg.valid = true;

    inputSystem_->registerAction(reg);
    EXPECT_EQ(inputSystem_->getActions().size(), 1);

    inputSystem_->unregisterAction("Jump");
    EXPECT_TRUE(inputSystem_->getActions().empty());
}

TEST_F(InputSystemTest, UnregisterPhaseActions) {
    ActionRegistration gameJump;
    gameJump.phase = "game";
    gameJump.conditions.push_back({ActionConditionType::WhenPressed, InputBinding::key(KeyCode::Space)});
    gameJump.effects.push_back({ActionEffectType::EmitAction, "Jump"});
    gameJump.valid = true;

    ActionRegistration menuSelect;
    menuSelect.phase = "menu";
    menuSelect.conditions.push_back({ActionConditionType::WhenPressed, InputBinding::key(KeyCode::Enter)});
    menuSelect.effects.push_back({ActionEffectType::EmitAction, "Select"});
    menuSelect.valid = true;

    inputSystem_->registerAction(gameJump);
    inputSystem_->registerAction(menuSelect);
    EXPECT_EQ(inputSystem_->getActions().size(), 2);

    inputSystem_->unregisterPhaseActions("game");

    auto actions = inputSystem_->getActions();
    ASSERT_EQ(actions.size(), 1);
    EXPECT_EQ(actions[0].phase, "menu");
}

TEST_F(InputSystemTest, ClearActions) {
    ActionRegistration reg;
    reg.phase = "game";
    reg.conditions.push_back({ActionConditionType::WhenPressed, InputBinding::key(KeyCode::Space)});
    reg.effects.push_back({ActionEffectType::EmitAction, "Jump"});
    reg.valid = true;

    inputSystem_->registerAction(reg);
    inputSystem_->registerAction(reg);

    inputSystem_->clearActions();
    EXPECT_TRUE(inputSystem_->getActions().empty());
}

//=============================================================================
// Input State Tracking Tests
//=============================================================================

TEST_F(InputSystemTest, GetInputStateInitially) {
    auto binding = InputBinding::key(KeyCode::Space);
    auto state = inputSystem_->getInputState(binding);
    EXPECT_EQ(state, InputState::NotPressed);
}

TEST_F(InputSystemTest, GetInputHoldDurationInitially) {
    auto binding = InputBinding::key(KeyCode::Space);
    float duration = inputSystem_->getInputHoldDuration(binding);
    EXPECT_EQ(duration, 0.0f);
}

TEST_F(InputSystemTest, SetAndGetDefaultHoldThreshold) {
    inputSystem_->setDefaultHoldThreshold(0.75f);
    EXPECT_EQ(inputSystem_->getDefaultHoldThreshold(), 0.75f);
}

TEST_F(InputSystemTest, DefaultHoldThresholdValue) {
    // Default should be 0.5 seconds
    EXPECT_EQ(inputSystem_->getDefaultHoldThreshold(), 0.5f);
}

//=============================================================================
// Platform-Agnostic Keyboard Tests
//=============================================================================

TEST_F(InputSystemTest, IsKeyDownWithKeyCode) {
    // Without a window, all keys should be unpressed
    EXPECT_FALSE(inputSystem_->isKeyDown(KeyCode::Space));
    EXPECT_FALSE(inputSystem_->isKeyDown(KeyCode::A));
    EXPECT_FALSE(inputSystem_->isKeyDown(KeyCode::Escape));
    EXPECT_FALSE(inputSystem_->isKeyDown(KeyCode::Enter));
}

TEST_F(InputSystemTest, WasKeyJustPressedWithKeyCode) {
    EXPECT_FALSE(inputSystem_->wasKeyJustPressed(KeyCode::Space));
    EXPECT_FALSE(inputSystem_->wasKeyJustPressed(KeyCode::A));
}

TEST_F(InputSystemTest, WasKeyJustReleasedWithKeyCode) {
    EXPECT_FALSE(inputSystem_->wasKeyJustReleased(KeyCode::Space));
    EXPECT_FALSE(inputSystem_->wasKeyJustReleased(KeyCode::A));
}

//=============================================================================
// Platform-Agnostic Mouse Tests
//=============================================================================

TEST_F(InputSystemTest, IsMouseButtonDownWithEnum) {
    EXPECT_FALSE(inputSystem_->isMouseButtonDown(MouseButton::Left));
    EXPECT_FALSE(inputSystem_->isMouseButtonDown(MouseButton::Right));
    EXPECT_FALSE(inputSystem_->isMouseButtonDown(MouseButton::Middle));
}

TEST_F(InputSystemTest, WasMouseButtonJustPressedWithEnum) {
    EXPECT_FALSE(inputSystem_->wasMouseButtonJustPressed(MouseButton::Left));
    EXPECT_FALSE(inputSystem_->wasMouseButtonJustPressed(MouseButton::Right));
}

TEST_F(InputSystemTest, WasMouseButtonJustReleasedWithEnum) {
    EXPECT_FALSE(inputSystem_->wasMouseButtonJustReleased(MouseButton::Left));
    EXPECT_FALSE(inputSystem_->wasMouseButtonJustReleased(MouseButton::Right));
}

//=============================================================================
// Platform-Agnostic Gamepad Tests
//=============================================================================

TEST_F(InputSystemTest, IsGamepadButtonDownWithEnum) {
    EXPECT_FALSE(inputSystem_->isGamepadButtonDown(GamepadButton::A));
    EXPECT_FALSE(inputSystem_->isGamepadButtonDown(GamepadButton::B));
    EXPECT_FALSE(inputSystem_->isGamepadButtonDown(GamepadButton::X));
    EXPECT_FALSE(inputSystem_->isGamepadButtonDown(GamepadButton::Y));
}

TEST_F(InputSystemTest, IsGamepadButtonDownWithIndex) {
    EXPECT_FALSE(inputSystem_->isGamepadButtonDown(GamepadButton::A, 0));
    EXPECT_FALSE(inputSystem_->isGamepadButtonDown(GamepadButton::A, 1));
    EXPECT_FALSE(inputSystem_->isGamepadButtonDown(GamepadButton::A, 2));
    EXPECT_FALSE(inputSystem_->isGamepadButtonDown(GamepadButton::A, 3));
}

TEST_F(InputSystemTest, WasGamepadButtonJustPressedWithEnum) {
    EXPECT_FALSE(inputSystem_->wasGamepadButtonJustPressed(GamepadButton::A));
    EXPECT_FALSE(inputSystem_->wasGamepadButtonJustPressed(GamepadButton::Start));
}

TEST_F(InputSystemTest, WasGamepadButtonJustReleasedWithEnum) {
    EXPECT_FALSE(inputSystem_->wasGamepadButtonJustReleased(GamepadButton::A));
    EXPECT_FALSE(inputSystem_->wasGamepadButtonJustReleased(GamepadButton::Back));
}

TEST_F(InputSystemTest, GetGamepadAxisValue) {
    // Without controller, should return 0
    EXPECT_EQ(inputSystem_->getGamepadAxisValue(GamepadAxis::LeftX), 0.0f);
    EXPECT_EQ(inputSystem_->getGamepadAxisValue(GamepadAxis::LeftY), 0.0f);
    EXPECT_EQ(inputSystem_->getGamepadAxisValue(GamepadAxis::RightX), 0.0f);
    EXPECT_EQ(inputSystem_->getGamepadAxisValue(GamepadAxis::RightY), 0.0f);
    EXPECT_EQ(inputSystem_->getGamepadAxisValue(GamepadAxis::LeftTrigger), 0.0f);
    EXPECT_EQ(inputSystem_->getGamepadAxisValue(GamepadAxis::RightTrigger), 0.0f);
}

TEST_F(InputSystemTest, GetLeftStick) {
    Vec2 stick = inputSystem_->getLeftStick();
    EXPECT_EQ(stick.x, 0.0f);
    EXPECT_EQ(stick.y, 0.0f);
}

TEST_F(InputSystemTest, GetRightStick) {
    Vec2 stick = inputSystem_->getRightStick();
    EXPECT_EQ(stick.x, 0.0f);
    EXPECT_EQ(stick.y, 0.0f);
}

TEST_F(InputSystemTest, GetStickWithIndex) {
    Vec2 left0 = inputSystem_->getLeftStick(0);
    Vec2 left1 = inputSystem_->getLeftStick(1);
    Vec2 right0 = inputSystem_->getRightStick(0);
    Vec2 right1 = inputSystem_->getRightStick(1);

    EXPECT_EQ(left0.x, 0.0f);
    EXPECT_EQ(left1.x, 0.0f);
    EXPECT_EQ(right0.x, 0.0f);
    EXPECT_EQ(right1.x, 0.0f);
}

//=============================================================================
// ActionRegistration Validation Tests
//=============================================================================

TEST(ActionRegistrationTest, DefaultState) {
    ActionRegistration reg;
    EXPECT_TRUE(reg.phase.empty());
    EXPECT_TRUE(reg.conditions.empty());
    EXPECT_TRUE(reg.effects.empty());
    EXPECT_EQ(reg.terminal, ActionTerminal::Discrete);
    EXPECT_EQ(reg.deadzone, 0.0f);
    EXPECT_FALSE(reg.valid);
}

TEST(ActionRegistrationTest, ValidRegistration) {
    ActionRegistration reg;
    reg.phase = "game";
    reg.conditions.push_back({ActionConditionType::WhenPressed, InputBinding::key(KeyCode::Space)});
    reg.effects.push_back({ActionEffectType::EmitAction, "Jump"});
    reg.valid = true;

    EXPECT_TRUE(reg.valid);
    EXPECT_TRUE(reg.validationError.empty());
}

//=============================================================================
// ActionCondition Tests
//=============================================================================

TEST(ActionConditionTest, DefaultValues) {
    ActionCondition cond;
    EXPECT_EQ(cond.type, ActionConditionType::WhenPressed);
    EXPECT_FALSE(cond.holdThreshold.has_value());
}

TEST(ActionConditionTest, WithHoldThreshold) {
    ActionCondition cond;
    cond.type = ActionConditionType::WhenHeld;
    cond.input = InputBinding::key(KeyCode::Space);
    cond.holdThreshold = 1.0f;

    EXPECT_EQ(cond.type, ActionConditionType::WhenHeld);
    EXPECT_TRUE(cond.holdThreshold.has_value());
    EXPECT_EQ(cond.holdThreshold.value(), 1.0f);
}

//=============================================================================
// ActionEffect Tests
//=============================================================================

TEST(ActionEffectTest, EmitActionEffect) {
    ActionEffect effect;
    effect.type = ActionEffectType::EmitAction;
    effect.value = "Jump";

    EXPECT_EQ(effect.type, ActionEffectType::EmitAction);
    EXPECT_EQ(effect.value, "Jump");
}

TEST(ActionEffectTest, PushPhaseEffect) {
    ActionEffect effect;
    effect.type = ActionEffectType::PushPhase;
    effect.value = "game.melee";

    EXPECT_EQ(effect.type, ActionEffectType::PushPhase);
    EXPECT_EQ(effect.value, "game.melee");
}

TEST(ActionEffectTest, PopPhaseEffect) {
    ActionEffect effect;
    effect.type = ActionEffectType::PopPhase;
    effect.value = "";

    EXPECT_EQ(effect.type, ActionEffectType::PopPhase);
}

TEST(ActionEffectTest, ChangePhaseEffect) {
    ActionEffect effect;
    effect.type = ActionEffectType::ChangePhase;
    effect.value = "menu";

    EXPECT_EQ(effect.type, ActionEffectType::ChangePhase);
    EXPECT_EQ(effect.value, "menu");
}

//=============================================================================
// Config Loading Tests
//=============================================================================

TEST_F(InputSystemTest, LoadInputConfigNonexistent) {
    // Should return false for non-existent file
    bool result = inputSystem_->loadInputConfig("/nonexistent/path/inputs.lua");
    EXPECT_FALSE(result);
}

TEST_F(InputSystemTest, ReloadInputConfigWithoutInitialLoad) {
    // Should return false if no config was loaded
    bool result = inputSystem_->reloadInputConfig();
    EXPECT_FALSE(result);
}

//=============================================================================
// Mouse Position Tests
//=============================================================================

TEST_F(InputSystemTest, GetMousePositionInitially) {
    Vec2 pos = inputSystem_->getMousePosition();
    EXPECT_EQ(pos.x, 0.0f);
    EXPECT_EQ(pos.y, 0.0f);
}

TEST_F(InputSystemTest, GetMouseDeltaInitially) {
    Vec2 delta = inputSystem_->getMouseDelta();
    EXPECT_EQ(delta.x, 0.0f);
    EXPECT_EQ(delta.y, 0.0f);
}

TEST_F(InputSystemTest, GetScrollDeltaInitially) {
    Vec2 scroll = inputSystem_->getScrollDelta();
    EXPECT_EQ(scroll.x, 0.0f);
    EXPECT_EQ(scroll.y, 0.0f);
}

//=============================================================================
// Modifier Key Tests
//=============================================================================

TEST_F(InputSystemTest, GetModifierStateInitially) {
    ModifierKey mods = inputSystem_->getModifierState();
    EXPECT_EQ(mods, ModifierKey::None);
}

TEST_F(InputSystemTest, IsModifierPressedInitially) {
    EXPECT_FALSE(inputSystem_->isModifierPressed(ModifierKey::Shift));
    EXPECT_FALSE(inputSystem_->isModifierPressed(ModifierKey::Ctrl));
    EXPECT_FALSE(inputSystem_->isModifierPressed(ModifierKey::Alt));
    EXPECT_FALSE(inputSystem_->isModifierPressed(ModifierKey::Super));
}

TEST_F(InputSystemTest, ModifierConvenienceMethods) {
    EXPECT_FALSE(inputSystem_->isShiftPressed());
    EXPECT_FALSE(inputSystem_->isCtrlPressed());
    EXPECT_FALSE(inputSystem_->isAltPressed());
    EXPECT_FALSE(inputSystem_->isSuperPressed());
}

//=============================================================================
// Text Input Tests
//=============================================================================

TEST_F(InputSystemTest, TextInputInitiallyDisabled) {
    EXPECT_FALSE(inputSystem_->isTextInputEnabled());
}

TEST_F(InputSystemTest, EnableTextInput) {
    inputSystem_->enableTextInput();
    EXPECT_TRUE(inputSystem_->isTextInputEnabled());
}

TEST_F(InputSystemTest, DisableTextInput) {
    inputSystem_->enableTextInput();
    inputSystem_->disableTextInput();
    EXPECT_FALSE(inputSystem_->isTextInputEnabled());
}

TEST_F(InputSystemTest, GetTextInputWhenDisabled) {
    std::string text = inputSystem_->getTextInput();
    EXPECT_TRUE(text.empty());
}

TEST_F(InputSystemTest, ClearTextInput) {
    inputSystem_->enableTextInput();
    inputSystem_->clearTextInput();
    EXPECT_TRUE(inputSystem_->getTextInput().empty());
}

//=============================================================================
// Input Listening Tests
//=============================================================================

TEST_F(InputSystemTest, InitiallyNotListening) {
    EXPECT_FALSE(inputSystem_->isListeningForInput());
}

TEST_F(InputSystemTest, StartListeningForInput) {
    inputSystem_->startListeningForInput();
    EXPECT_TRUE(inputSystem_->isListeningForInput());
}

TEST_F(InputSystemTest, StopListeningForInput) {
    inputSystem_->startListeningForInput();
    inputSystem_->stopListeningForInput();
    EXPECT_FALSE(inputSystem_->isListeningForInput());
}

TEST_F(InputSystemTest, GetLastInputInitially) {
    auto lastInput = inputSystem_->getLastInput();
    EXPECT_FALSE(lastInput.has_value());
}

//=============================================================================
// Controller Tests
//=============================================================================

TEST_F(InputSystemTest, NoControllersInitially) {
    EXPECT_EQ(inputSystem_->getConnectedControllerCount(), 0);
}

TEST_F(InputSystemTest, ControllerNotConnected) {
    EXPECT_FALSE(inputSystem_->isControllerConnected(0));
    EXPECT_FALSE(inputSystem_->isControllerConnected(1));
    EXPECT_FALSE(inputSystem_->isControllerConnected(2));
    EXPECT_FALSE(inputSystem_->isControllerConnected(3));
}

TEST_F(InputSystemTest, GetControllerNameForDisconnected) {
    std::string name = inputSystem_->getControllerName(0);
    EXPECT_TRUE(name.empty());
}

//=============================================================================
// Update Lifecycle Tests
//=============================================================================

TEST_F(InputSystemTest, UpdateWithoutWindow) {
    // Should not crash
    inputSystem_->update();
    SUCCEED();
}

TEST_F(InputSystemTest, MultipleUpdates) {
    inputSystem_->update();
    inputSystem_->update();
    inputSystem_->update();
    SUCCEED();
}

//=============================================================================
// Kangaru DI Integration Tests
//=============================================================================

TEST(InputSystemKangaruTest, ServiceInjection) {
    kgr::container container;

    // Register dependencies
    container.service<EventSystemService>();
    container.service<AssetSystemService>();

    // Register InputSystem
    auto& service = container.service<InputSystemService>();

    EXPECT_FALSE(service.hasPhaseBeenSet());
}

TEST(InputSystemKangaruTest, SingletonBehavior) {
    kgr::container container;

    container.service<EventSystemService>();
    container.service<AssetSystemService>();

    auto& inputSystem1 = container.service<InputSystemService>();
    auto& inputSystem2 = container.service<InputSystemService>();

    EXPECT_EQ(&inputSystem1, &inputSystem2);
}

TEST(InputSystemKangaruTest, ServicePersistsState) {
    kgr::container container;

    container.service<EventSystemService>();
    container.service<AssetSystemService>();

    auto& inputSystem1 = container.service<InputSystemService>();
    inputSystem1.changePhase("test_phase");

    auto& inputSystem2 = container.service<InputSystemService>();
    EXPECT_EQ(inputSystem2.getCurrentPhase(), "test_phase");
}

//=============================================================================
// Legacy API Tests (for backwards compatibility)
//=============================================================================

TEST_F(InputSystemTest, LegacyGetMappingsEmpty) {
    auto mappings = inputSystem_->getMappings();
    EXPECT_TRUE(mappings.empty());
}

TEST_F(InputSystemTest, LegacyClearMappings) {
    inputSystem_->clearMappings();
    EXPECT_TRUE(inputSystem_->getMappings().empty());
}

TEST_F(InputSystemTest, LegacyGetActionStateForUnmapped) {
    ActionState state = inputSystem_->getActionState("nonexistent");
    EXPECT_EQ(state.action, "nonexistent");
    EXPECT_FALSE(state.active);
}

TEST_F(InputSystemTest, LegacyIsActionActiveForUnmapped) {
    EXPECT_FALSE(inputSystem_->isActionActive("nonexistent"));
}

TEST_F(InputSystemTest, LegacyGetActionValueForUnmapped) {
    EXPECT_EQ(inputSystem_->getActionValue("nonexistent"), 0.0f);
}

TEST_F(InputSystemTest, LegacyGetAllActionStatesEmpty) {
    auto states = inputSystem_->getAllActionStates();
    EXPECT_TRUE(states.empty());
}

//=============================================================================
// Cursor Control Tests
//=============================================================================

TEST_F(InputSystemTest, CursorModeDefaultsToNormal) {
    // Without initialization, cursor mode should default to Normal
    EXPECT_EQ(inputSystem_->getCursorMode(), CursorMode::Normal);
}

TEST_F(InputSystemTest, CursorIsVisibleByDefault) {
    EXPECT_TRUE(inputSystem_->isMouseCursorVisible());
}

TEST_F(InputSystemTest, ShowMouseCursorSetsNormalMode) {
    inputSystem_->setCursorMode(CursorMode::Hidden);
    inputSystem_->showMouseCursor();
    EXPECT_EQ(inputSystem_->getCursorMode(), CursorMode::Normal);
    EXPECT_TRUE(inputSystem_->isMouseCursorVisible());
}

TEST_F(InputSystemTest, HideMouseCursorSetsHiddenMode) {
    inputSystem_->hideMouseCursor();
    EXPECT_EQ(inputSystem_->getCursorMode(), CursorMode::Hidden);
    EXPECT_FALSE(inputSystem_->isMouseCursorVisible());
}

TEST_F(InputSystemTest, SetCursorModeNormal) {
    inputSystem_->setCursorMode(CursorMode::Normal);
    EXPECT_EQ(inputSystem_->getCursorMode(), CursorMode::Normal);
    EXPECT_TRUE(inputSystem_->isMouseCursorVisible());
}

TEST_F(InputSystemTest, SetCursorModeHidden) {
    inputSystem_->setCursorMode(CursorMode::Hidden);
    EXPECT_EQ(inputSystem_->getCursorMode(), CursorMode::Hidden);
    EXPECT_FALSE(inputSystem_->isMouseCursorVisible());
}

TEST_F(InputSystemTest, SetCursorModeDisabled) {
    inputSystem_->setCursorMode(CursorMode::Disabled);
    EXPECT_EQ(inputSystem_->getCursorMode(), CursorMode::Disabled);
    EXPECT_FALSE(inputSystem_->isMouseCursorVisible());
}

TEST_F(InputSystemTest, CursorModePersistsAcrossCalls) {
    inputSystem_->setCursorMode(CursorMode::Disabled);
    EXPECT_EQ(inputSystem_->getCursorMode(), CursorMode::Disabled);

    // Call again with same mode
    inputSystem_->setCursorMode(CursorMode::Disabled);
    EXPECT_EQ(inputSystem_->getCursorMode(), CursorMode::Disabled);
}

TEST_F(InputSystemTest, CursorModeTransitions) {
    // Normal -> Hidden -> Disabled -> Normal
    inputSystem_->setCursorMode(CursorMode::Normal);
    EXPECT_EQ(inputSystem_->getCursorMode(), CursorMode::Normal);

    inputSystem_->setCursorMode(CursorMode::Hidden);
    EXPECT_EQ(inputSystem_->getCursorMode(), CursorMode::Hidden);

    inputSystem_->setCursorMode(CursorMode::Disabled);
    EXPECT_EQ(inputSystem_->getCursorMode(), CursorMode::Disabled);

    inputSystem_->setCursorMode(CursorMode::Normal);
    EXPECT_EQ(inputSystem_->getCursorMode(), CursorMode::Normal);
}

TEST_F(InputSystemTest, ShowCursorAfterDisabled) {
    inputSystem_->setCursorMode(CursorMode::Disabled);
    EXPECT_FALSE(inputSystem_->isMouseCursorVisible());

    inputSystem_->showMouseCursor();
    EXPECT_TRUE(inputSystem_->isMouseCursorVisible());
    EXPECT_EQ(inputSystem_->getCursorMode(), CursorMode::Normal);
}

TEST_F(InputSystemTest, HideCursorAfterDisabled) {
    inputSystem_->setCursorMode(CursorMode::Disabled);
    inputSystem_->hideMouseCursor();
    EXPECT_FALSE(inputSystem_->isMouseCursorVisible());
    EXPECT_EQ(inputSystem_->getCursorMode(), CursorMode::Hidden);
}

TEST_F(InputSystemTest, CursorModeDoesNotAffectMousePosition) {
    // Mouse position should still be queryable regardless of cursor mode
    Vec2 pos1 = inputSystem_->getMousePosition();

    inputSystem_->setCursorMode(CursorMode::Hidden);
    Vec2 pos2 = inputSystem_->getMousePosition();

    inputSystem_->setCursorMode(CursorMode::Disabled);
    Vec2 pos3 = inputSystem_->getMousePosition();

    // All should return valid positions (in headless mode, likely 0,0)
    EXPECT_GE(pos1.x, 0.0f);
    EXPECT_GE(pos2.x, 0.0f);
    EXPECT_GE(pos3.x, 0.0f);
}

}  // namespace bestow::tests

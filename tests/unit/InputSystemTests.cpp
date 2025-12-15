// tests/unit/InputSystemTests.cpp
// Input system unit tests

#include <memory>
#include <string>
#include <vector>

#include <gtest/gtest.h>
#include <kangaru/kangaru.hpp>

import bestow.input;
import bestow.input.impl;
import bestow.types;
import bestow.assets.impl;   // InputSystem depends on AssetSystem
import bestow.events.impl;   // AssetSystem depends on EventSystem

namespace bestow::tests {

class InputSystemTest : public ::testing::Test {
protected:
    void SetUp() override {
        inputSystem_ = std::make_unique<InputSystem>();
    }

    std::unique_ptr<IInputSystem> inputSystem_;
};

TEST_F(InputSystemTest, InitiallyNoMappings) {
    auto mappings = inputSystem_->getMappings();
    EXPECT_TRUE(mappings.empty());
}

TEST_F(InputSystemTest, RegisterMapping) {
    InputMapping mapping{
        .binding = {
            .deviceType = InputDeviceType::Keyboard,
            .deviceIndex = 0,
            .keyCode = 32,  // Space
            .scale = 1.0f,
            .deadzone = 0.0f
        },
        .action = "jump"
    };

    inputSystem_->registerMapping(mapping);

    auto mappings = inputSystem_->getMappings();
    EXPECT_EQ(mappings.size(), 1);
    EXPECT_EQ(mappings[0].action, "jump");
}

TEST_F(InputSystemTest, RegisterMultipleMappings) {
    InputMapping jump{
        .binding = {.deviceType = InputDeviceType::Keyboard, .deviceIndex = 0, .keyCode = 32},
        .action = "jump"
    };
    InputMapping moveLeft{
        .binding = {.deviceType = InputDeviceType::Keyboard, .deviceIndex = 0, .keyCode = 65},  // A
        .action = "move_left"
    };
    InputMapping moveRight{
        .binding = {.deviceType = InputDeviceType::Keyboard, .deviceIndex = 0, .keyCode = 68},  // D
        .action = "move_right"
    };

    inputSystem_->registerMapping(jump);
    inputSystem_->registerMapping(moveLeft);
    inputSystem_->registerMapping(moveRight);

    auto mappings = inputSystem_->getMappings();
    EXPECT_EQ(mappings.size(), 3);
}

TEST_F(InputSystemTest, RemoveMapping) {
    InputMapping mapping{
        .binding = {.deviceType = InputDeviceType::Keyboard, .deviceIndex = 0, .keyCode = 32},
        .action = "jump"
    };

    inputSystem_->registerMapping(mapping);
    EXPECT_EQ(inputSystem_->getMappings().size(), 1);

    inputSystem_->removeMapping(mapping.binding);
    EXPECT_TRUE(inputSystem_->getMappings().empty());
}

TEST_F(InputSystemTest, ClearMappings) {
    InputMapping jump{
        .binding = {.deviceType = InputDeviceType::Keyboard, .deviceIndex = 0, .keyCode = 32},
        .action = "jump"
    };
    InputMapping move{
        .binding = {.deviceType = InputDeviceType::Keyboard, .deviceIndex = 0, .keyCode = 65},
        .action = "move"
    };

    inputSystem_->registerMapping(jump);
    inputSystem_->registerMapping(move);
    EXPECT_EQ(inputSystem_->getMappings().size(), 2);

    inputSystem_->clearMappings();
    EXPECT_TRUE(inputSystem_->getMappings().empty());
}

TEST_F(InputSystemTest, GetActionStateForUnmappedAction) {
    ActionState state = inputSystem_->getActionState("nonexistent");
    EXPECT_EQ(state.action, "nonexistent");
    EXPECT_FALSE(state.active);
    EXPECT_EQ(state.value, 0.0f);
}

TEST_F(InputSystemTest, IsActionActiveForUnmappedAction) {
    EXPECT_FALSE(inputSystem_->isActionActive("nonexistent"));
}

TEST_F(InputSystemTest, GetActionValueForUnmappedAction) {
    EXPECT_EQ(inputSystem_->getActionValue("nonexistent"), 0.0f);
}

TEST_F(InputSystemTest, InitiallyNotListeningForInput) {
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

TEST_F(InputSystemTest, NoLastInputInitially) {
    EXPECT_FALSE(inputSystem_->getLastInput().has_value());
}

TEST_F(InputSystemTest, NoControllersInitially) {
    // Without initializing SDL, no controllers should be connected
    EXPECT_EQ(inputSystem_->getConnectedControllerCount(), 0);
}

TEST_F(InputSystemTest, ControllerNotConnectedByDefault) {
    EXPECT_FALSE(inputSystem_->isControllerConnected(0));
    EXPECT_FALSE(inputSystem_->isControllerConnected(1));
    EXPECT_FALSE(inputSystem_->isControllerConnected(2));
    EXPECT_FALSE(inputSystem_->isControllerConnected(3));
}

TEST_F(InputSystemTest, ControllerNameForDisconnected) {
    std::string name = inputSystem_->getControllerName(0);
    EXPECT_TRUE(name.empty());
}

TEST_F(InputSystemTest, MultipleBindingsToSameAction) {
    // Can bind both keyboard and controller to same action
    InputMapping keyboardJump{
        .binding = {.deviceType = InputDeviceType::Keyboard, .deviceIndex = 0, .keyCode = 32},
        .action = "jump"
    };
    InputMapping controllerJump{
        .binding = {.deviceType = InputDeviceType::Controller, .deviceIndex = 0, .keyCode = 0},  // A button
        .action = "jump"
    };

    inputSystem_->registerMapping(keyboardJump);
    inputSystem_->registerMapping(controllerJump);

    auto mappings = inputSystem_->getMappings();
    EXPECT_EQ(mappings.size(), 2);
}

//======================================================================
// Action State Tests
//======================================================================

TEST_F(InputSystemTest, WasActionJustPressedForUnmappedAction) {
    EXPECT_FALSE(inputSystem_->wasActionJustPressed("nonexistent"));
}

TEST_F(InputSystemTest, WasActionJustReleasedForUnmappedAction) {
    EXPECT_FALSE(inputSystem_->wasActionJustReleased("nonexistent"));
}

TEST_F(InputSystemTest, GetAllActionStatesWhenEmpty) {
    auto states = inputSystem_->getAllActionStates();
    EXPECT_TRUE(states.empty());
}

TEST_F(InputSystemTest, GetAllActionStatesWithMappings) {
    InputMapping jump{
        .binding = {.deviceType = InputDeviceType::Keyboard, .deviceIndex = 0, .keyCode = 32},
        .action = "jump"
    };
    InputMapping moveLeft{
        .binding = {.deviceType = InputDeviceType::Keyboard, .deviceIndex = 0, .keyCode = 65},
        .action = "move_left"
    };

    inputSystem_->registerMapping(jump);
    inputSystem_->registerMapping(moveLeft);

    // Need to call update to populate action states
    inputSystem_->update();

    auto states = inputSystem_->getAllActionStates();
    EXPECT_EQ(states.size(), 2);

    // Verify we can find both actions
    bool foundJump = false;
    bool foundMoveLeft = false;
    for (const auto& state : states) {
        if (state.action == "jump") foundJump = true;
        if (state.action == "move_left") foundMoveLeft = true;
    }
    EXPECT_TRUE(foundJump);
    EXPECT_TRUE(foundMoveLeft);
}

TEST_F(InputSystemTest, ActionStateStructure) {
    InputMapping mapping{
        .binding = {.deviceType = InputDeviceType::Keyboard, .deviceIndex = 0, .keyCode = 32},
        .action = "test_action"
    };

    inputSystem_->registerMapping(mapping);
    inputSystem_->update();

    ActionState state = inputSystem_->getActionState("test_action");
    EXPECT_EQ(state.action, "test_action");
    // Without actual key press, should be inactive
    EXPECT_FALSE(state.active);
    EXPECT_EQ(state.value, 0.0f);
    EXPECT_FALSE(state.justPressed);
    EXPECT_FALSE(state.justReleased);
}

//======================================================================
// Mouse State Tests
//======================================================================

TEST_F(InputSystemTest, GetMousePositionInitially) {
    Vec2 pos = inputSystem_->getMousePosition();
    // Without a window, should return default (0, 0)
    EXPECT_EQ(pos.x, 0.0f);
    EXPECT_EQ(pos.y, 0.0f);
}

TEST_F(InputSystemTest, GetMouseDeltaInitially) {
    Vec2 delta = inputSystem_->getMouseDelta();
    // Without a window or movement, should be zero
    EXPECT_EQ(delta.x, 0.0f);
    EXPECT_EQ(delta.y, 0.0f);
}

TEST_F(InputSystemTest, IsMouseButtonDownForAllButtons) {
    // Test all 8 mouse buttons without a window
    for (int i = 0; i < 8; ++i) {
        EXPECT_FALSE(inputSystem_->isMouseButtonDown(i));
    }
}

TEST_F(InputSystemTest, IsMouseButtonDownOutOfRange) {
    // Test boundary conditions
    EXPECT_FALSE(inputSystem_->isMouseButtonDown(-1));
    EXPECT_FALSE(inputSystem_->isMouseButtonDown(8));
    EXPECT_FALSE(inputSystem_->isMouseButtonDown(100));
}

//======================================================================
// Controller Tests
//======================================================================

TEST_F(InputSystemTest, IsControllerConnectedOutOfRange) {
    // Test boundary conditions
    EXPECT_FALSE(inputSystem_->isControllerConnected(-1));
    EXPECT_FALSE(inputSystem_->isControllerConnected(4));
    EXPECT_FALSE(inputSystem_->isControllerConnected(100));
}

TEST_F(InputSystemTest, GetControllerNameForOutOfRange) {
    std::string name = inputSystem_->getControllerName(-1);
    EXPECT_TRUE(name.empty());

    name = inputSystem_->getControllerName(4);
    EXPECT_TRUE(name.empty());

    name = inputSystem_->getControllerName(100);
    EXPECT_TRUE(name.empty());
}

TEST_F(InputSystemTest, GetControllerNameForAllSlots) {
    // Test all 4 controller slots when disconnected
    for (int i = 0; i < 4; ++i) {
        std::string name = inputSystem_->getControllerName(i);
        EXPECT_TRUE(name.empty());
    }
}

//======================================================================
// Mapping Edge Cases
//======================================================================

TEST_F(InputSystemTest, RemoveNonexistentMapping) {
    InputBinding binding{
        .deviceType = InputDeviceType::Keyboard,
        .deviceIndex = 0,
        .keyCode = 32
    };

    // Should not crash when removing mapping that doesn't exist
    inputSystem_->removeMapping(binding);
    EXPECT_TRUE(inputSystem_->getMappings().empty());
}

TEST_F(InputSystemTest, RemoveSpecificMappingAmongMultiple) {
    InputMapping jump{
        .binding = {.deviceType = InputDeviceType::Keyboard, .deviceIndex = 0, .keyCode = 32},
        .action = "jump"
    };
    InputMapping moveLeft{
        .binding = {.deviceType = InputDeviceType::Keyboard, .deviceIndex = 0, .keyCode = 65},
        .action = "move_left"
    };
    InputMapping moveRight{
        .binding = {.deviceType = InputDeviceType::Keyboard, .deviceIndex = 0, .keyCode = 68},
        .action = "move_right"
    };

    inputSystem_->registerMapping(jump);
    inputSystem_->registerMapping(moveLeft);
    inputSystem_->registerMapping(moveRight);
    EXPECT_EQ(inputSystem_->getMappings().size(), 3);

    // Remove the middle one
    inputSystem_->removeMapping(moveLeft.binding);

    auto mappings = inputSystem_->getMappings();
    EXPECT_EQ(mappings.size(), 2);

    // Verify the correct ones remain
    bool hasJump = false;
    bool hasMoveRight = false;
    bool hasMoveLeft = false;
    for (const auto& mapping : mappings) {
        if (mapping.action == "jump") hasJump = true;
        if (mapping.action == "move_right") hasMoveRight = true;
        if (mapping.action == "move_left") hasMoveLeft = true;
    }
    EXPECT_TRUE(hasJump);
    EXPECT_TRUE(hasMoveRight);
    EXPECT_FALSE(hasMoveLeft);
}

TEST_F(InputSystemTest, RegisterDuplicateMapping) {
    InputMapping mapping{
        .binding = {.deviceType = InputDeviceType::Keyboard, .deviceIndex = 0, .keyCode = 32},
        .action = "jump"
    };

    inputSystem_->registerMapping(mapping);
    inputSystem_->registerMapping(mapping);  // Register same mapping twice

    auto mappings = inputSystem_->getMappings();
    // Should have both instances (system doesn't deduplicate)
    EXPECT_EQ(mappings.size(), 2);
}

TEST_F(InputSystemTest, ClearMappingsAlsoClearsActionStates) {
    InputMapping mapping{
        .binding = {.deviceType = InputDeviceType::Keyboard, .deviceIndex = 0, .keyCode = 32},
        .action = "jump"
    };

    inputSystem_->registerMapping(mapping);
    inputSystem_->update();  // Populate action states

    EXPECT_FALSE(inputSystem_->getAllActionStates().empty());

    inputSystem_->clearMappings();

    EXPECT_TRUE(inputSystem_->getMappings().empty());
    EXPECT_TRUE(inputSystem_->getAllActionStates().empty());
}

//======================================================================
// Input Binding with Different Device Types
//======================================================================

TEST_F(InputSystemTest, RegisterMouseButtonMapping) {
    InputMapping leftClick{
        .binding = {
            .deviceType = InputDeviceType::Mouse,
            .deviceIndex = 0,
            .keyCode = 0,  // Left button
            .scale = 1.0f,
            .deadzone = 0.0f
        },
        .action = "fire"
    };

    inputSystem_->registerMapping(leftClick);

    auto mappings = inputSystem_->getMappings();
    EXPECT_EQ(mappings.size(), 1);
    EXPECT_EQ(mappings[0].binding.deviceType, InputDeviceType::Mouse);
    EXPECT_EQ(mappings[0].binding.keyCode, 0);
}

TEST_F(InputSystemTest, RegisterControllerButtonMapping) {
    InputMapping aButton{
        .binding = {
            .deviceType = InputDeviceType::Controller,
            .deviceIndex = 0,
            .keyCode = 0,  // A button
            .scale = 1.0f,
            .deadzone = 0.0f
        },
        .action = "jump"
    };

    inputSystem_->registerMapping(aButton);

    auto mappings = inputSystem_->getMappings();
    EXPECT_EQ(mappings.size(), 1);
    EXPECT_EQ(mappings[0].binding.deviceType, InputDeviceType::Controller);
}

TEST_F(InputSystemTest, RegisterControllerAxisMapping) {
    InputMapping leftStick{
        .binding = {
            .deviceType = InputDeviceType::Controller,
            .deviceIndex = 0,
            .keyCode = 100,  // Arbitrary axis code
            .scale = 1.0f,
            .deadzone = 0.2f
        },
        .action = "move_horizontal"
    };

    inputSystem_->registerMapping(leftStick);

    auto mappings = inputSystem_->getMappings();
    EXPECT_EQ(mappings.size(), 1);
    EXPECT_EQ(mappings[0].binding.deadzone, 0.2f);
}

TEST_F(InputSystemTest, MappingWithNegativeScale) {
    InputMapping mapping{
        .binding = {
            .deviceType = InputDeviceType::Keyboard,
            .deviceIndex = 0,
            .keyCode = 65,  // A key
            .scale = -1.0f,  // Negative scale for inverted input
            .deadzone = 0.0f
        },
        .action = "move_left"
    };

    inputSystem_->registerMapping(mapping);

    auto mappings = inputSystem_->getMappings();
    EXPECT_EQ(mappings.size(), 1);
    EXPECT_EQ(mappings[0].binding.scale, -1.0f);
}

//======================================================================
// Input Listening Tests
//======================================================================

TEST_F(InputSystemTest, StartListeningClearsLastInput) {
    // Simulate that we had a previous input
    InputMapping mapping{
        .binding = {.deviceType = InputDeviceType::Keyboard, .deviceIndex = 0, .keyCode = 32},
        .action = "jump"
    };
    inputSystem_->registerMapping(mapping);

    inputSystem_->startListeningForInput();

    // After starting to listen, last input should be empty
    EXPECT_FALSE(inputSystem_->getLastInput().has_value());
    EXPECT_TRUE(inputSystem_->isListeningForInput());
}

TEST_F(InputSystemTest, MultipleStartListeningCalls) {
    inputSystem_->startListeningForInput();
    EXPECT_TRUE(inputSystem_->isListeningForInput());

    inputSystem_->startListeningForInput();  // Call again
    EXPECT_TRUE(inputSystem_->isListeningForInput());
}

TEST_F(InputSystemTest, MultipleStopListeningCalls) {
    inputSystem_->startListeningForInput();
    inputSystem_->stopListeningForInput();
    EXPECT_FALSE(inputSystem_->isListeningForInput());

    inputSystem_->stopListeningForInput();  // Call again
    EXPECT_FALSE(inputSystem_->isListeningForInput());
}

TEST_F(InputSystemTest, StopListeningWithoutStarting) {
    // Should be safe to stop listening when not listening
    EXPECT_FALSE(inputSystem_->isListeningForInput());
    inputSystem_->stopListeningForInput();
    EXPECT_FALSE(inputSystem_->isListeningForInput());
}

//======================================================================
// Update Lifecycle Tests
//======================================================================

TEST_F(InputSystemTest, UpdateWithoutWindow) {
    // Should not crash when update is called without initialization
    inputSystem_->update();
    // If we reach here, no crash occurred
    SUCCEED();
}

TEST_F(InputSystemTest, UpdateWithNoMappings) {
    inputSystem_->update();

    auto states = inputSystem_->getAllActionStates();
    EXPECT_TRUE(states.empty());
}

TEST_F(InputSystemTest, MultipleUpdatesWithMappings) {
    InputMapping mapping{
        .binding = {.deviceType = InputDeviceType::Keyboard, .deviceIndex = 0, .keyCode = 32},
        .action = "jump"
    };

    inputSystem_->registerMapping(mapping);

    // Multiple updates should not cause issues
    inputSystem_->update();
    inputSystem_->update();
    inputSystem_->update();

    auto states = inputSystem_->getAllActionStates();
    EXPECT_EQ(states.size(), 1);
}

//======================================================================
// Device Index Tests
//======================================================================

TEST_F(InputSystemTest, MappingsWithDifferentDeviceIndices) {
    InputMapping keyboard1{
        .binding = {.deviceType = InputDeviceType::Keyboard, .deviceIndex = 0, .keyCode = 32},
        .action = "jump"
    };
    InputMapping keyboard2{
        .binding = {.deviceType = InputDeviceType::Keyboard, .deviceIndex = 1, .keyCode = 32},
        .action = "jump"
    };

    inputSystem_->registerMapping(keyboard1);
    inputSystem_->registerMapping(keyboard2);

    auto mappings = inputSystem_->getMappings();
    EXPECT_EQ(mappings.size(), 2);
    EXPECT_NE(mappings[0].binding.deviceIndex, mappings[1].binding.deviceIndex);
}

TEST_F(InputSystemTest, RemoveMappingByDeviceIndex) {
    InputMapping controller0{
        .binding = {.deviceType = InputDeviceType::Controller, .deviceIndex = 0, .keyCode = 0},
        .action = "jump"
    };
    InputMapping controller1{
        .binding = {.deviceType = InputDeviceType::Controller, .deviceIndex = 1, .keyCode = 0},
        .action = "jump"
    };

    inputSystem_->registerMapping(controller0);
    inputSystem_->registerMapping(controller1);
    EXPECT_EQ(inputSystem_->getMappings().size(), 2);

    // Remove only controller 0's mapping
    inputSystem_->removeMapping(controller0.binding);

    auto mappings = inputSystem_->getMappings();
    EXPECT_EQ(mappings.size(), 1);
    EXPECT_EQ(mappings[0].binding.deviceIndex, 1);
}

//======================================================================
// Action Name Tests
//======================================================================

TEST_F(InputSystemTest, ActionWithEmptyString) {
    ActionState state = inputSystem_->getActionState("");
    EXPECT_EQ(state.action, "");
    EXPECT_FALSE(state.active);
}

TEST_F(InputSystemTest, ActionWithSpecialCharacters) {
    InputMapping mapping{
        .binding = {.deviceType = InputDeviceType::Keyboard, .deviceIndex = 0, .keyCode = 32},
        .action = "player_1/jump"
    };

    inputSystem_->registerMapping(mapping);
    inputSystem_->update();

    ActionState state = inputSystem_->getActionState("player_1/jump");
    EXPECT_EQ(state.action, "player_1/jump");
}

TEST_F(InputSystemTest, ActionWithLongName) {
    std::string longAction(1000, 'a');  // Very long action name
    InputMapping mapping{
        .binding = {.deviceType = InputDeviceType::Keyboard, .deviceIndex = 0, .keyCode = 32},
        .action = longAction
    };

    inputSystem_->registerMapping(mapping);
    inputSystem_->update();

    ActionState state = inputSystem_->getActionState(longAction);
    EXPECT_EQ(state.action, longAction);
}

//======================================================================
// Scroll Delta Tests
//======================================================================

TEST_F(InputSystemTest, GetScrollDeltaInitially) {
    Vec2 scroll = inputSystem_->getScrollDelta();
    EXPECT_EQ(scroll.x, 0.0f);
    EXPECT_EQ(scroll.y, 0.0f);
}

TEST_F(InputSystemTest, ScrollDeltaIsZeroWithoutInput) {
    inputSystem_->update();
    Vec2 scroll = inputSystem_->getScrollDelta();
    EXPECT_EQ(scroll.x, 0.0f);
    EXPECT_EQ(scroll.y, 0.0f);
}

//======================================================================
// Text Input Tests
//======================================================================

TEST_F(InputSystemTest, TextInputInitiallyDisabled) {
    EXPECT_FALSE(inputSystem_->isTextInputEnabled());
}

TEST_F(InputSystemTest, EnableTextInput) {
    inputSystem_->enableTextInput();
    EXPECT_TRUE(inputSystem_->isTextInputEnabled());
}

TEST_F(InputSystemTest, DisableTextInput) {
    inputSystem_->enableTextInput();
    EXPECT_TRUE(inputSystem_->isTextInputEnabled());

    inputSystem_->disableTextInput();
    EXPECT_FALSE(inputSystem_->isTextInputEnabled());
}

TEST_F(InputSystemTest, GetTextInputWhenDisabled) {
    EXPECT_FALSE(inputSystem_->isTextInputEnabled());
    std::string text = inputSystem_->getTextInput();
    EXPECT_TRUE(text.empty());
}

TEST_F(InputSystemTest, GetTextInputWhenEnabled) {
    inputSystem_->enableTextInput();
    std::string text = inputSystem_->getTextInput();
    // Should be empty if no input received
    EXPECT_TRUE(text.empty());
}

TEST_F(InputSystemTest, ClearTextInputWhenDisabled) {
    // Should be safe to clear when disabled
    inputSystem_->clearTextInput();
    EXPECT_TRUE(inputSystem_->getTextInput().empty());
}

TEST_F(InputSystemTest, ClearTextInputWhenEnabled) {
    inputSystem_->enableTextInput();
    inputSystem_->clearTextInput();
    EXPECT_TRUE(inputSystem_->getTextInput().empty());
}

TEST_F(InputSystemTest, EnableTextInputMultipleTimes) {
    inputSystem_->enableTextInput();
    EXPECT_TRUE(inputSystem_->isTextInputEnabled());

    inputSystem_->enableTextInput();  // Call again
    EXPECT_TRUE(inputSystem_->isTextInputEnabled());
}

TEST_F(InputSystemTest, DisableTextInputMultipleTimes) {
    inputSystem_->enableTextInput();
    inputSystem_->disableTextInput();
    EXPECT_FALSE(inputSystem_->isTextInputEnabled());

    inputSystem_->disableTextInput();  // Call again
    EXPECT_FALSE(inputSystem_->isTextInputEnabled());
}

TEST_F(InputSystemTest, DisableTextInputWithoutEnabling) {
    EXPECT_FALSE(inputSystem_->isTextInputEnabled());
    inputSystem_->disableTextInput();
    EXPECT_FALSE(inputSystem_->isTextInputEnabled());
}

//======================================================================
// Kangaru DI Integration Tests
//======================================================================

TEST(InputSystemKangaruTest, ServiceInjection) {
    kgr::container container;

    // Register dependencies first - InputSystem depends on AssetSystem which depends on EventSystem
    container.service<EventSystemService>();
    container.service<AssetSystemService>();

    // Register the InputSystem service
    auto& service = container.service<InputSystemService>();

    // Verify we got a valid instance
    EXPECT_TRUE(service.getMappings().empty());
}

TEST(InputSystemKangaruTest, SingletonBehavior) {
    kgr::container container;

    // Register dependencies first
    container.service<EventSystemService>();
    container.service<AssetSystemService>();

    // Get references to the same service multiple times
    auto& inputSystem1 = container.service<InputSystemService>();
    auto& inputSystem2 = container.service<InputSystemService>();

    // Should be the same instance (same memory address)
    EXPECT_EQ(&inputSystem1, &inputSystem2);
}

TEST(InputSystemKangaruTest, ServicePersistsState) {
    kgr::container container;

    // Register dependencies first
    container.service<EventSystemService>();
    container.service<AssetSystemService>();

    auto& inputSystem1 = container.service<InputSystemService>();

    // Register a mapping
    InputMapping mapping{
        .binding = {.deviceType = InputDeviceType::Keyboard, .deviceIndex = 0, .keyCode = 32},
        .action = "jump"
    };
    inputSystem1.registerMapping(mapping);

    // Get the service again
    auto& inputSystem2 = container.service<InputSystemService>();

    // State should be preserved (same instance)
    EXPECT_EQ(inputSystem2.getMappings().size(), 1);
    EXPECT_EQ(inputSystem2.getMappings()[0].action, "jump");
}

TEST(InputSystemKangaruTest, InterfacePointer) {
    kgr::container container;

    // Register dependencies first
    container.service<EventSystemService>();
    container.service<AssetSystemService>();

    // Get as interface pointer
    IInputSystem* inputSystemPtr = &container.service<InputSystemService>();

    EXPECT_NE(inputSystemPtr, nullptr);
    EXPECT_TRUE(inputSystemPtr->getMappings().empty());
}

TEST(InputSystemKangaruTest, ServiceUpdate) {
    kgr::container container;

    // Register dependencies first
    container.service<EventSystemService>();
    container.service<AssetSystemService>();

    auto& inputSystem = container.service<InputSystemService>();

    // Should not crash when calling update
    inputSystem.update();
    SUCCEED();
}

}  // namespace bestow::tests

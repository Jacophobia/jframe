// tests/unit/InputSystemTests.cpp
// Input system unit tests

#include <memory>
#include <string>
#include <vector>

#include <gtest/gtest.h>

import jframe.input;
import jframe.input.impl;
import jframe.types;

namespace jframe::tests {

class InputSystemTest : public ::testing::Test {
protected:
    void SetUp() override {
        inputSystem_ = createInputSystem();
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

}  // namespace jframe::tests

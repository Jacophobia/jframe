// tests/unit/SaveSystemTests.cpp
// Save system unit tests

#include <chrono>
#include <memory>
#include <string>
#include <vector>

#include <gtest/gtest.h>

import jframe.save;
import jframe.save.impl;
import jframe.types;

namespace jframe::tests {

class SaveSystemTest : public ::testing::Test {
protected:
    void SetUp() override {
        saveSystem_ = createSaveSystem();
    }

    std::unique_ptr<ISaveSystem> saveSystem_;
};

TEST_F(SaveSystemTest, ActiveProfile) {
    EXPECT_EQ(saveSystem_->getActiveProfile(), "default");

    saveSystem_->setActiveProfile("player1");
    EXPECT_EQ(saveSystem_->getActiveProfile(), "player1");
}

TEST_F(SaveSystemTest, AutoSaveConfig) {
    saveSystem_->enableAutoSave(std::chrono::seconds(300));
    // Auto-save is enabled
    saveSystem_->disableAutoSave();
}

TEST_F(SaveSystemTest, SaveNotExists) {
    EXPECT_FALSE(saveSystem_->saveExists(999));
}

}  // namespace jframe::tests

// tests/unit/SceneSystemTests.cpp
// Scene system unit tests

#include <memory>
#include <set>
#include <string>
#include <vector>

#include <gtest/gtest.h>

#include "../mocks/MockSceneSystem.hpp"

import bestow;
import bestow.types;

namespace bestow::tests {

//==========================================================================
// Test Fixture
//==========================================================================

class SceneSystemTest : public ::testing::Test {
protected:
    void SetUp() override {
        sceneSystem_ = std::make_unique<MockSceneSystem>();
    }

    AssetHandle makeAsset(UUID uuid = 1) {
        return AssetHandle{uuid, AssetType::Scene};
    }

    std::unique_ptr<ISceneSystem> sceneSystem_;
};

//==========================================================================
// Registration Tests
//==========================================================================

TEST_F(SceneSystemTest, RegisterSceneReturnsValidId) {
    auto result = sceneSystem_->registerScene("menu", makeAsset());
    ASSERT_TRUE(result.has_value());
    EXPECT_GT(result.value(), 0u);
}

TEST_F(SceneSystemTest, RegisterSceneSetsStateToReady) {
    sceneSystem_->registerScene("menu", makeAsset());
    EXPECT_EQ(sceneSystem_->getSceneState("menu"), SceneState::Ready);
}

TEST_F(SceneSystemTest, RegisterDuplicateSceneFails) {
    sceneSystem_->registerScene("menu", makeAsset());
    auto result = sceneSystem_->registerScene("menu", makeAsset(2));
    ASSERT_FALSE(result.has_value());
}

TEST_F(SceneSystemTest, UnregisterSceneSucceeds) {
    sceneSystem_->registerScene("menu", makeAsset());
    auto result = sceneSystem_->unregisterScene("menu");
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(sceneSystem_->getSceneState("menu"), SceneState::Unloaded);
}

TEST_F(SceneSystemTest, UnregisterNonExistentSceneFails) {
    auto result = sceneSystem_->unregisterScene("nonexistent");
    ASSERT_FALSE(result.has_value());
}

TEST_F(SceneSystemTest, UnregisterSceneOnStackFails) {
    sceneSystem_->registerScene("menu", makeAsset());
    sceneSystem_->pushScene("menu");
    auto result = sceneSystem_->unregisterScene("menu");
    ASSERT_FALSE(result.has_value());
}

TEST_F(SceneSystemTest, GetRegisteredScenesReturnsAll) {
    sceneSystem_->registerScene("menu", makeAsset(1));
    sceneSystem_->registerScene("gameplay", makeAsset(2));
    sceneSystem_->registerScene("pause", makeAsset(3));

    auto scenes = sceneSystem_->getRegisteredScenes();
    EXPECT_EQ(scenes.size(), 3u);
    std::set<std::string> names(scenes.begin(), scenes.end());
    EXPECT_TRUE(names.contains("menu"));
    EXPECT_TRUE(names.contains("gameplay"));
    EXPECT_TRUE(names.contains("pause"));
}

//==========================================================================
// Stack Operation Tests
//==========================================================================

TEST_F(SceneSystemTest, NoActiveSceneInitially) {
    EXPECT_FALSE(sceneSystem_->getActiveSceneName().has_value());
}

TEST_F(SceneSystemTest, PushSceneMakesItActive) {
    sceneSystem_->registerScene("menu", makeAsset());
    sceneSystem_->pushScene("menu");
    EXPECT_EQ(sceneSystem_->getActiveSceneName(), "menu");
    EXPECT_EQ(sceneSystem_->getSceneState("menu"), SceneState::Active);
}

TEST_F(SceneSystemTest, PushUnregisteredSceneFails) {
    auto result = sceneSystem_->pushScene("nonexistent");
    ASSERT_FALSE(result.has_value());
}

TEST_F(SceneSystemTest, PushSecondScenePausesPrevious) {
    sceneSystem_->registerScene("menu", makeAsset(1));
    sceneSystem_->registerScene("gameplay", makeAsset(2));

    sceneSystem_->pushScene("menu");
    sceneSystem_->pushScene("gameplay");

    EXPECT_EQ(sceneSystem_->getActiveSceneName(), "gameplay");
    EXPECT_EQ(sceneSystem_->getSceneState("menu"), SceneState::Paused);
    EXPECT_EQ(sceneSystem_->getSceneState("gameplay"), SceneState::Active);
}

TEST_F(SceneSystemTest, PopSceneResumesPrevious) {
    sceneSystem_->registerScene("menu", makeAsset(1));
    sceneSystem_->registerScene("gameplay", makeAsset(2));

    sceneSystem_->pushScene("menu");
    sceneSystem_->pushScene("gameplay");
    sceneSystem_->popScene();

    EXPECT_EQ(sceneSystem_->getActiveSceneName(), "menu");
    EXPECT_EQ(sceneSystem_->getSceneState("menu"), SceneState::Active);
    EXPECT_EQ(sceneSystem_->getSceneState("gameplay"), SceneState::Ready);
}

TEST_F(SceneSystemTest, PopEmptyStackFails) {
    auto result = sceneSystem_->popScene();
    ASSERT_FALSE(result.has_value());
}

TEST_F(SceneSystemTest, PopLastSceneLeavesNoActive) {
    sceneSystem_->registerScene("menu", makeAsset());
    sceneSystem_->pushScene("menu");
    sceneSystem_->popScene();

    EXPECT_FALSE(sceneSystem_->getActiveSceneName().has_value());
}

TEST_F(SceneSystemTest, ReplaceSceneSwapsTop) {
    sceneSystem_->registerScene("level1", makeAsset(1));
    sceneSystem_->registerScene("level2", makeAsset(2));

    sceneSystem_->pushScene("level1");
    sceneSystem_->replaceScene("level2");

    EXPECT_EQ(sceneSystem_->getActiveSceneName(), "level2");
    EXPECT_EQ(sceneSystem_->getSceneState("level1"), SceneState::Ready);
    EXPECT_EQ(sceneSystem_->getSceneState("level2"), SceneState::Active);
}

TEST_F(SceneSystemTest, ReplaceUnregisteredSceneFails) {
    sceneSystem_->registerScene("level1", makeAsset());
    sceneSystem_->pushScene("level1");
    auto result = sceneSystem_->replaceScene("nonexistent");
    ASSERT_FALSE(result.has_value());
}

TEST_F(SceneSystemTest, ReplacePreservesStackDepth) {
    sceneSystem_->registerScene("menu", makeAsset(1));
    sceneSystem_->registerScene("level1", makeAsset(2));
    sceneSystem_->registerScene("level2", makeAsset(3));

    sceneSystem_->pushScene("menu");
    sceneSystem_->pushScene("level1");
    EXPECT_EQ(sceneSystem_->getSceneStack().size(), 2u);

    sceneSystem_->replaceScene("level2");
    EXPECT_EQ(sceneSystem_->getSceneStack().size(), 2u);
}

TEST_F(SceneSystemTest, ClearStackRemovesAll) {
    sceneSystem_->registerScene("menu", makeAsset(1));
    sceneSystem_->registerScene("gameplay", makeAsset(2));
    sceneSystem_->registerScene("pause", makeAsset(3));

    sceneSystem_->pushScene("menu");
    sceneSystem_->pushScene("gameplay");
    sceneSystem_->pushScene("pause");

    sceneSystem_->clearStack();

    EXPECT_TRUE(sceneSystem_->getSceneStack().empty());
    EXPECT_FALSE(sceneSystem_->getActiveSceneName().has_value());
}

//==========================================================================
// Query Tests
//==========================================================================

TEST_F(SceneSystemTest, GetSceneStackReturnsCorrectOrder) {
    sceneSystem_->registerScene("menu", makeAsset(1));
    sceneSystem_->registerScene("gameplay", makeAsset(2));
    sceneSystem_->registerScene("pause", makeAsset(3));

    sceneSystem_->pushScene("menu");
    sceneSystem_->pushScene("gameplay");
    sceneSystem_->pushScene("pause");

    auto stack = sceneSystem_->getSceneStack();
    ASSERT_EQ(stack.size(), 3u);
    EXPECT_EQ(stack[0], "menu");
    EXPECT_EQ(stack[1], "gameplay");
    EXPECT_EQ(stack[2], "pause");
}

TEST_F(SceneSystemTest, GetSceneMetadataReturnsData) {
    sceneSystem_->registerScene("menu", makeAsset(42));
    auto meta = sceneSystem_->getSceneMetadata("menu");
    ASSERT_TRUE(meta.has_value());
    EXPECT_EQ(meta->name, "menu");
    EXPECT_EQ(meta->state, SceneState::Ready);
}

TEST_F(SceneSystemTest, GetSceneMetadataForNonExistent) {
    auto meta = sceneSystem_->getSceneMetadata("nonexistent");
    EXPECT_FALSE(meta.has_value());
}

TEST_F(SceneSystemTest, GetSceneStateForUnregistered) {
    EXPECT_EQ(sceneSystem_->getSceneState("nonexistent"), SceneState::Unloaded);
}

//==========================================================================
// Complex Workflow Tests
//==========================================================================

TEST_F(SceneSystemTest, CompleteGameFlow) {
    // Register scenes
    sceneSystem_->registerScene("splash", makeAsset(1));
    sceneSystem_->registerScene("menu", makeAsset(2));
    sceneSystem_->registerScene("gameplay", makeAsset(3));
    sceneSystem_->registerScene("pause", makeAsset(4));

    // Start with splash screen
    sceneSystem_->pushScene("splash");
    EXPECT_EQ(sceneSystem_->getActiveSceneName(), "splash");

    // Replace splash with menu
    sceneSystem_->replaceScene("menu");
    EXPECT_EQ(sceneSystem_->getActiveSceneName(), "menu");

    // Push gameplay on top of menu
    sceneSystem_->pushScene("gameplay");
    EXPECT_EQ(sceneSystem_->getActiveSceneName(), "gameplay");
    EXPECT_EQ(sceneSystem_->getSceneState("menu"), SceneState::Paused);

    // Push pause on top of gameplay
    sceneSystem_->pushScene("pause");
    EXPECT_EQ(sceneSystem_->getActiveSceneName(), "pause");
    EXPECT_EQ(sceneSystem_->getSceneState("gameplay"), SceneState::Paused);

    // Pop pause to resume gameplay
    sceneSystem_->popScene();
    EXPECT_EQ(sceneSystem_->getActiveSceneName(), "gameplay");
    EXPECT_EQ(sceneSystem_->getSceneState("pause"), SceneState::Ready);

    // Pop gameplay to return to menu
    sceneSystem_->popScene();
    EXPECT_EQ(sceneSystem_->getActiveSceneName(), "menu");
    EXPECT_EQ(sceneSystem_->getSceneState("gameplay"), SceneState::Ready);

    // Clear everything
    sceneSystem_->clearStack();
    EXPECT_FALSE(sceneSystem_->getActiveSceneName().has_value());
}

TEST_F(SceneSystemTest, MultipleReplacements) {
    sceneSystem_->registerScene("level1", makeAsset(1));
    sceneSystem_->registerScene("level2", makeAsset(2));
    sceneSystem_->registerScene("level3", makeAsset(3));

    sceneSystem_->pushScene("level1");
    sceneSystem_->replaceScene("level2");
    sceneSystem_->replaceScene("level3");

    EXPECT_EQ(sceneSystem_->getActiveSceneName(), "level3");
    EXPECT_EQ(sceneSystem_->getSceneStack().size(), 1u);
}

TEST_F(SceneSystemTest, PushSameSceneTwice) {
    sceneSystem_->registerScene("menu", makeAsset());

    sceneSystem_->pushScene("menu");
    sceneSystem_->pushScene("menu");

    auto stack = sceneSystem_->getSceneStack();
    EXPECT_EQ(stack.size(), 2u);
    EXPECT_EQ(stack[0], "menu");
    EXPECT_EQ(stack[1], "menu");
}

TEST_F(SceneSystemTest, UnregisterAfterPopSucceeds) {
    sceneSystem_->registerScene("temp", makeAsset());
    sceneSystem_->pushScene("temp");
    sceneSystem_->popScene();
    auto result = sceneSystem_->unregisterScene("temp");
    EXPECT_TRUE(result.has_value());
}

//==========================================================================
// Edge Cases
//==========================================================================

TEST_F(SceneSystemTest, EmptyStackAfterAllPops) {
    sceneSystem_->registerScene("a", makeAsset(1));
    sceneSystem_->registerScene("b", makeAsset(2));

    sceneSystem_->pushScene("a");
    sceneSystem_->pushScene("b");
    sceneSystem_->popScene();
    sceneSystem_->popScene();

    EXPECT_TRUE(sceneSystem_->getSceneStack().empty());
    auto popResult = sceneSystem_->popScene();
    EXPECT_FALSE(popResult.has_value());
}

TEST_F(SceneSystemTest, ReplaceOnEmptyStack) {
    sceneSystem_->registerScene("menu", makeAsset());
    auto result = sceneSystem_->replaceScene("menu");
    EXPECT_TRUE(result.has_value());
    EXPECT_EQ(sceneSystem_->getActiveSceneName(), "menu");
}

TEST_F(SceneSystemTest, ClearEmptyStackIsNoOp) {
    sceneSystem_->clearStack();  // Should not crash
    EXPECT_TRUE(sceneSystem_->getSceneStack().empty());
}

TEST_F(SceneSystemTest, SceneIdUniqueness) {
    auto r1 = sceneSystem_->registerScene("a", makeAsset(1));
    auto r2 = sceneSystem_->registerScene("b", makeAsset(2));
    auto r3 = sceneSystem_->registerScene("c", makeAsset(3));

    ASSERT_TRUE(r1.has_value());
    ASSERT_TRUE(r2.has_value());
    ASSERT_TRUE(r3.has_value());

    std::set<SceneId> ids{r1.value(), r2.value(), r3.value()};
    EXPECT_EQ(ids.size(), 3u);
}

}  // namespace bestow::tests

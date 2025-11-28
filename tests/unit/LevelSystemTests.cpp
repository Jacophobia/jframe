// tests/unit/LevelSystemTests.cpp
// Level system unit tests

#include <cstddef>
#include <memory>
#include <optional>
#include <string>
#include <system_error>
#include <vector>

#include <gtest/gtest.h>

import jframe.level;
import jframe.level.impl;
import jframe.types;
import jframe.assets;
import jframe.assets.impl;

namespace jframe::tests {

class LevelSystemTest : public ::testing::Test {
protected:
    void SetUp() override {
        levelSystem_ = createLevelSystem();
    }

    // Helper to create a mock asset handle
    AssetHandle createMockAsset(UUID uuid = 1) {
        return AssetHandle{uuid, AssetType::Level};
    }

    std::unique_ptr<ILevelSystem> levelSystem_;
};

//==============================================================================
// Level Loading Tests
//==============================================================================

TEST_F(LevelSystemTest, LoadLevelReturnsValidLevelId) {
    auto asset = createMockAsset(1);
    auto result = levelSystem_->loadLevel(asset);

    ASSERT_TRUE(result.has_value());
    EXPECT_NE(result.value(), 0);
}

TEST_F(LevelSystemTest, LoadLevelSetsStateToLoaded) {
    auto asset = createMockAsset(1);
    auto result = levelSystem_->loadLevel(asset);

    ASSERT_TRUE(result.has_value());
    LevelId levelId = result.value();

    LevelState state = levelSystem_->getLevelState(levelId);
    EXPECT_EQ(state, LevelState::Loaded);
}

TEST_F(LevelSystemTest, LoadLevelStoresAssetHandle) {
    auto asset = createMockAsset(42);
    auto result = levelSystem_->loadLevel(asset);

    ASSERT_TRUE(result.has_value());
    LevelId levelId = result.value();

    LevelMetadata metadata = levelSystem_->getLevelMetadata(levelId);
    EXPECT_EQ(metadata.assetHandle.uuid, 42);
    EXPECT_EQ(metadata.assetHandle.type, AssetType::Level);
}

TEST_F(LevelSystemTest, LoadMultipleLevels) {
    auto asset1 = createMockAsset(1);
    auto asset2 = createMockAsset(2);
    auto asset3 = createMockAsset(3);

    auto result1 = levelSystem_->loadLevel(asset1);
    auto result2 = levelSystem_->loadLevel(asset2);
    auto result3 = levelSystem_->loadLevel(asset3);

    ASSERT_TRUE(result1.has_value());
    ASSERT_TRUE(result2.has_value());
    ASSERT_TRUE(result3.has_value());

    // All level IDs should be unique
    EXPECT_NE(result1.value(), result2.value());
    EXPECT_NE(result2.value(), result3.value());
    EXPECT_NE(result1.value(), result3.value());
}

TEST_F(LevelSystemTest, LoadedLevelsAppearInGetLoadedLevels) {
    auto asset1 = createMockAsset(1);
    auto asset2 = createMockAsset(2);

    levelSystem_->loadLevel(asset1);
    levelSystem_->loadLevel(asset2);

    auto loadedLevels = levelSystem_->getLoadedLevels();
    EXPECT_EQ(loadedLevels.size(), 2);
}

//==============================================================================
// Level Unloading Tests
//==============================================================================

TEST_F(LevelSystemTest, UnloadLevelRemovesLevel) {
    auto asset = createMockAsset(1);
    auto result = levelSystem_->loadLevel(asset);
    ASSERT_TRUE(result.has_value());
    LevelId levelId = result.value();

    levelSystem_->unloadLevel(levelId);

    LevelState state = levelSystem_->getLevelState(levelId);
    EXPECT_EQ(state, LevelState::Unloaded);
}

TEST_F(LevelSystemTest, UnloadLevelRemovesFromLoadedList) {
    auto asset1 = createMockAsset(1);
    auto asset2 = createMockAsset(2);

    auto result1 = levelSystem_->loadLevel(asset1);
    auto result2 = levelSystem_->loadLevel(asset2);

    ASSERT_TRUE(result1.has_value());
    ASSERT_TRUE(result2.has_value());

    levelSystem_->unloadLevel(result1.value());

    auto loadedLevels = levelSystem_->getLoadedLevels();
    EXPECT_EQ(loadedLevels.size(), 1);
    EXPECT_EQ(loadedLevels[0].id, result2.value());
}

TEST_F(LevelSystemTest, UnloadActiveLevelClearsActiveLevel) {
    auto asset = createMockAsset(1);
    auto result = levelSystem_->loadLevel(asset);
    ASSERT_TRUE(result.has_value());
    LevelId levelId = result.value();

    levelSystem_->setActiveLevel(levelId);
    EXPECT_TRUE(levelSystem_->getActiveLevel().has_value());

    levelSystem_->unloadLevel(levelId);
    EXPECT_FALSE(levelSystem_->getActiveLevel().has_value());
}

TEST_F(LevelSystemTest, UnloadNonExistentLevelDoesNotCrash) {
    // Should not crash or throw
    levelSystem_->unloadLevel(999999);
}

//==============================================================================
// Active Level Tests
//==============================================================================

TEST_F(LevelSystemTest, NoActiveLevelInitially) {
    auto activeLevel = levelSystem_->getActiveLevel();
    EXPECT_FALSE(activeLevel.has_value());
}

TEST_F(LevelSystemTest, SetActiveLevelWorks) {
    auto asset = createMockAsset(1);
    auto result = levelSystem_->loadLevel(asset);
    ASSERT_TRUE(result.has_value());
    LevelId levelId = result.value();

    levelSystem_->setActiveLevel(levelId);

    auto activeLevel = levelSystem_->getActiveLevel();
    ASSERT_TRUE(activeLevel.has_value());
    EXPECT_EQ(activeLevel.value(), levelId);
}

TEST_F(LevelSystemTest, SetActiveLevelOnUnloadedLevelDoesNothing) {
    levelSystem_->setActiveLevel(999999);

    auto activeLevel = levelSystem_->getActiveLevel();
    EXPECT_FALSE(activeLevel.has_value());
}

TEST_F(LevelSystemTest, SetActiveLevelCanSwitchBetweenLevels) {
    auto asset1 = createMockAsset(1);
    auto asset2 = createMockAsset(2);

    auto result1 = levelSystem_->loadLevel(asset1);
    auto result2 = levelSystem_->loadLevel(asset2);

    ASSERT_TRUE(result1.has_value());
    ASSERT_TRUE(result2.has_value());

    levelSystem_->setActiveLevel(result1.value());
    EXPECT_EQ(levelSystem_->getActiveLevel().value(), result1.value());

    levelSystem_->setActiveLevel(result2.value());
    EXPECT_EQ(levelSystem_->getActiveLevel().value(), result2.value());
}

//==============================================================================
// Level Transition Tests
//==============================================================================

TEST_F(LevelSystemTest, TransitionWithoutUnload) {
    auto asset1 = createMockAsset(1);
    auto asset2 = createMockAsset(2);

    auto result1 = levelSystem_->loadLevel(asset1);
    auto result2 = levelSystem_->loadLevel(asset2);

    ASSERT_TRUE(result1.has_value());
    ASSERT_TRUE(result2.has_value());

    levelSystem_->setActiveLevel(result1.value());

    LevelTransition transition{
        .fromLevel = result1.value(),
        .toLevel = result2.value(),
        .spawnPoint = std::nullopt,
        .unloadPrevious = false
    };

    levelSystem_->transition(transition);

    // Transition happens on update
    levelSystem_->update(0.016f);

    EXPECT_EQ(levelSystem_->getActiveLevel().value(), result2.value());
    EXPECT_EQ(levelSystem_->getLevelState(result1.value()), LevelState::Loaded);
}

TEST_F(LevelSystemTest, TransitionWithUnload) {
    auto asset1 = createMockAsset(1);
    auto asset2 = createMockAsset(2);

    auto result1 = levelSystem_->loadLevel(asset1);
    auto result2 = levelSystem_->loadLevel(asset2);

    ASSERT_TRUE(result1.has_value());
    ASSERT_TRUE(result2.has_value());

    levelSystem_->setActiveLevel(result1.value());

    LevelTransition transition{
        .fromLevel = result1.value(),
        .toLevel = result2.value(),
        .spawnPoint = std::nullopt,
        .unloadPrevious = true
    };

    levelSystem_->transition(transition);

    // Transition happens on update
    levelSystem_->update(0.016f);

    EXPECT_EQ(levelSystem_->getActiveLevel().value(), result2.value());
    EXPECT_EQ(levelSystem_->getLevelState(result1.value()), LevelState::Unloaded);
}

TEST_F(LevelSystemTest, TransitionWithSpawnPoint) {
    auto asset1 = createMockAsset(1);
    auto asset2 = createMockAsset(2);

    auto result1 = levelSystem_->loadLevel(asset1);
    auto result2 = levelSystem_->loadLevel(asset2);

    ASSERT_TRUE(result1.has_value());
    ASSERT_TRUE(result2.has_value());

    LevelTransition transition{
        .fromLevel = result1.value(),
        .toLevel = result2.value(),
        .spawnPoint = "checkpoint1",
        .unloadPrevious = false
    };

    levelSystem_->transition(transition);
    levelSystem_->update(0.016f);

    EXPECT_EQ(levelSystem_->getActiveLevel().value(), result2.value());
}

TEST_F(LevelSystemTest, TransitionDoesNotHappenImmediately) {
    auto asset1 = createMockAsset(1);
    auto asset2 = createMockAsset(2);

    auto result1 = levelSystem_->loadLevel(asset1);
    auto result2 = levelSystem_->loadLevel(asset2);

    ASSERT_TRUE(result1.has_value());
    ASSERT_TRUE(result2.has_value());

    levelSystem_->setActiveLevel(result1.value());

    LevelTransition transition{
        .fromLevel = result1.value(),
        .toLevel = result2.value(),
        .spawnPoint = std::nullopt,
        .unloadPrevious = false
    };

    levelSystem_->transition(transition);

    // Before update, active level should still be the old one
    EXPECT_EQ(levelSystem_->getActiveLevel().value(), result1.value());
}

TEST_F(LevelSystemTest, MultipleUpdatesOnlyProcessOneTransition) {
    auto asset1 = createMockAsset(1);
    auto asset2 = createMockAsset(2);

    auto result1 = levelSystem_->loadLevel(asset1);
    auto result2 = levelSystem_->loadLevel(asset2);

    ASSERT_TRUE(result1.has_value());
    ASSERT_TRUE(result2.has_value());

    levelSystem_->setActiveLevel(result1.value());

    LevelTransition transition{
        .fromLevel = result1.value(),
        .toLevel = result2.value(),
        .spawnPoint = std::nullopt,
        .unloadPrevious = true
    };

    levelSystem_->transition(transition);
    levelSystem_->update(0.016f);
    levelSystem_->update(0.016f);
    levelSystem_->update(0.016f);

    // First level should still be unloaded (not unloaded multiple times)
    EXPECT_EQ(levelSystem_->getLevelState(result1.value()), LevelState::Unloaded);
    EXPECT_EQ(levelSystem_->getActiveLevel().value(), result2.value());
}

//==============================================================================
// Level Metadata Tests
//==============================================================================

TEST_F(LevelSystemTest, GetLevelMetadataReturnsCorrectId) {
    auto asset = createMockAsset(42);
    auto result = levelSystem_->loadLevel(asset);
    ASSERT_TRUE(result.has_value());
    LevelId levelId = result.value();

    LevelMetadata metadata = levelSystem_->getLevelMetadata(levelId);
    EXPECT_EQ(metadata.id, levelId);
}

TEST_F(LevelSystemTest, GetLevelMetadataForNonExistentLevel) {
    LevelMetadata metadata = levelSystem_->getLevelMetadata(999999);
    EXPECT_EQ(metadata.id, 0);  // Default-constructed metadata
}

TEST_F(LevelSystemTest, GetLevelStateForNonExistentLevel) {
    LevelState state = levelSystem_->getLevelState(999999);
    EXPECT_EQ(state, LevelState::Unloaded);
}

TEST_F(LevelSystemTest, GetLoadedLevelsReturnsEmptyInitially) {
    auto loadedLevels = levelSystem_->getLoadedLevels();
    EXPECT_TRUE(loadedLevels.empty());
}

TEST_F(LevelSystemTest, GetLoadedLevelsReturnsAllLoadedLevels) {
    auto asset1 = createMockAsset(1);
    auto asset2 = createMockAsset(2);
    auto asset3 = createMockAsset(3);

    levelSystem_->loadLevel(asset1);
    levelSystem_->loadLevel(asset2);
    levelSystem_->loadLevel(asset3);

    auto loadedLevels = levelSystem_->getLoadedLevels();
    EXPECT_EQ(loadedLevels.size(), 3);
}

//==============================================================================
// Spawn Point Tests
//==============================================================================

TEST_F(LevelSystemTest, GetSpawnPointForNonExistentLevel) {
    auto spawnPoint = levelSystem_->getSpawnPoint(999999, "default");
    EXPECT_FALSE(spawnPoint.has_value());
}

TEST_F(LevelSystemTest, GetNonExistentSpawnPoint) {
    auto asset = createMockAsset(1);
    auto result = levelSystem_->loadLevel(asset);
    ASSERT_TRUE(result.has_value());

    auto spawnPoint = levelSystem_->getSpawnPoint(result.value(), "nonexistent");
    EXPECT_FALSE(spawnPoint.has_value());
}

TEST_F(LevelSystemTest, GetSpawnPointNamesForNonExistentLevel) {
    auto names = levelSystem_->getSpawnPointNames(999999);
    EXPECT_TRUE(names.empty());
}

TEST_F(LevelSystemTest, GetSpawnPointNamesForLevelWithoutSpawnPoints) {
    auto asset = createMockAsset(1);
    auto result = levelSystem_->loadLevel(asset);
    ASSERT_TRUE(result.has_value());

    auto names = levelSystem_->getSpawnPointNames(result.value());
    EXPECT_TRUE(names.empty());
}

// Note: These tests would require actual Lua parsing to work fully
// For now, they test the interface and basic functionality
TEST_F(LevelSystemTest, GetSpawnPointReturnsEmptyForBasicLevel) {
    auto asset = createMockAsset(1);
    auto result = levelSystem_->loadLevel(asset);
    ASSERT_TRUE(result.has_value());

    auto spawnPoint = levelSystem_->getSpawnPoint(result.value(), "default");
    // Without Lua parsing implemented, this will be empty
    EXPECT_FALSE(spawnPoint.has_value());
}

//==============================================================================
// Entity Management Tests
//==============================================================================

TEST_F(LevelSystemTest, GetLevelEntitiesForNonExistentLevel) {
    auto entities = levelSystem_->getLevelEntities(999999);
    EXPECT_TRUE(entities.empty());
}

TEST_F(LevelSystemTest, GetLevelEntitiesReturnsEmptyForNewLevel) {
    auto asset = createMockAsset(1);
    auto result = levelSystem_->loadLevel(asset);
    ASSERT_TRUE(result.has_value());

    auto entities = levelSystem_->getLevelEntities(result.value());
    EXPECT_TRUE(entities.empty());
}

// Note: This test would require entity creation integration
TEST_F(LevelSystemTest, GetLevelEntitiesAfterUnload) {
    auto asset = createMockAsset(1);
    auto result = levelSystem_->loadLevel(asset);
    ASSERT_TRUE(result.has_value());
    LevelId levelId = result.value();

    levelSystem_->unloadLevel(levelId);

    auto entities = levelSystem_->getLevelEntities(levelId);
    EXPECT_TRUE(entities.empty());
}

//==============================================================================
// Level State Tests
//==============================================================================

TEST_F(LevelSystemTest, NewlyLoadedLevelHasLoadedState) {
    auto asset = createMockAsset(1);
    auto result = levelSystem_->loadLevel(asset);
    ASSERT_TRUE(result.has_value());

    LevelState state = levelSystem_->getLevelState(result.value());
    EXPECT_EQ(state, LevelState::Loaded);
}

TEST_F(LevelSystemTest, UnloadedLevelHasUnloadedState) {
    auto asset = createMockAsset(1);
    auto result = levelSystem_->loadLevel(asset);
    ASSERT_TRUE(result.has_value());
    LevelId levelId = result.value();

    levelSystem_->unloadLevel(levelId);

    LevelState state = levelSystem_->getLevelState(levelId);
    EXPECT_EQ(state, LevelState::Unloaded);
}

//==============================================================================
// Edge Cases and Error Handling
//==============================================================================

TEST_F(LevelSystemTest, LoadSameAssetMultipleTimes) {
    auto asset = createMockAsset(1);

    auto result1 = levelSystem_->loadLevel(asset);
    auto result2 = levelSystem_->loadLevel(asset);

    ASSERT_TRUE(result1.has_value());
    ASSERT_TRUE(result2.has_value());

    // Should create different level instances
    EXPECT_NE(result1.value(), result2.value());

    auto loadedLevels = levelSystem_->getLoadedLevels();
    EXPECT_EQ(loadedLevels.size(), 2);
}

TEST_F(LevelSystemTest, UnloadLevelTwice) {
    auto asset = createMockAsset(1);
    auto result = levelSystem_->loadLevel(asset);
    ASSERT_TRUE(result.has_value());
    LevelId levelId = result.value();

    levelSystem_->unloadLevel(levelId);
    levelSystem_->unloadLevel(levelId);  // Should not crash

    EXPECT_EQ(levelSystem_->getLevelState(levelId), LevelState::Unloaded);
}

TEST_F(LevelSystemTest, TransitionToNonExistentLevel) {
    auto asset1 = createMockAsset(1);
    auto result1 = levelSystem_->loadLevel(asset1);
    ASSERT_TRUE(result1.has_value());

    levelSystem_->setActiveLevel(result1.value());

    LevelTransition transition{
        .fromLevel = result1.value(),
        .toLevel = 999999,  // Non-existent
        .spawnPoint = std::nullopt,
        .unloadPrevious = false
    };

    levelSystem_->transition(transition);
    levelSystem_->update(0.016f);

    // Active level should change even if target doesn't exist (up to implementation)
    auto activeLevel = levelSystem_->getActiveLevel();
    EXPECT_TRUE(activeLevel.has_value());
}

TEST_F(LevelSystemTest, UpdateWithoutPendingTransition) {
    auto asset = createMockAsset(1);
    auto result = levelSystem_->loadLevel(asset);
    ASSERT_TRUE(result.has_value());

    levelSystem_->setActiveLevel(result.value());
    auto activeBefore = levelSystem_->getActiveLevel();

    levelSystem_->update(0.016f);

    auto activeAfter = levelSystem_->getActiveLevel();
    EXPECT_EQ(activeBefore, activeAfter);
}

TEST_F(LevelSystemTest, MultipleTransitionsOnlyLastOneApplies) {
    auto asset1 = createMockAsset(1);
    auto asset2 = createMockAsset(2);
    auto asset3 = createMockAsset(3);

    auto result1 = levelSystem_->loadLevel(asset1);
    auto result2 = levelSystem_->loadLevel(asset2);
    auto result3 = levelSystem_->loadLevel(asset3);

    ASSERT_TRUE(result1.has_value());
    ASSERT_TRUE(result2.has_value());
    ASSERT_TRUE(result3.has_value());

    levelSystem_->setActiveLevel(result1.value());

    // Queue multiple transitions (only last should apply)
    LevelTransition transition1{
        .fromLevel = result1.value(),
        .toLevel = result2.value(),
        .spawnPoint = std::nullopt,
        .unloadPrevious = false
    };

    LevelTransition transition2{
        .fromLevel = result1.value(),
        .toLevel = result3.value(),
        .spawnPoint = std::nullopt,
        .unloadPrevious = false
    };

    levelSystem_->transition(transition1);
    levelSystem_->transition(transition2);

    levelSystem_->update(0.016f);

    // Should transition to level 3 (last transition)
    EXPECT_EQ(levelSystem_->getActiveLevel().value(), result3.value());
}

//==============================================================================
// Integration Tests
//==============================================================================

TEST_F(LevelSystemTest, LoadMultipleLevelsSwitchBetweenThem) {
    auto asset1 = createMockAsset(1);
    auto asset2 = createMockAsset(2);
    auto asset3 = createMockAsset(3);

    auto level1 = levelSystem_->loadLevel(asset1);
    auto level2 = levelSystem_->loadLevel(asset2);
    auto level3 = levelSystem_->loadLevel(asset3);

    ASSERT_TRUE(level1.has_value());
    ASSERT_TRUE(level2.has_value());
    ASSERT_TRUE(level3.has_value());

    levelSystem_->setActiveLevel(level1.value());
    EXPECT_EQ(levelSystem_->getActiveLevel().value(), level1.value());

    levelSystem_->setActiveLevel(level2.value());
    EXPECT_EQ(levelSystem_->getActiveLevel().value(), level2.value());

    levelSystem_->setActiveLevel(level3.value());
    EXPECT_EQ(levelSystem_->getActiveLevel().value(), level3.value());

    levelSystem_->setActiveLevel(level1.value());
    EXPECT_EQ(levelSystem_->getActiveLevel().value(), level1.value());
}

TEST_F(LevelSystemTest, CompleteWorkflow) {
    // Load a level
    auto asset = createMockAsset(1);
    auto result = levelSystem_->loadLevel(asset);
    ASSERT_TRUE(result.has_value());
    LevelId levelId = result.value();

    // Verify it's loaded
    EXPECT_EQ(levelSystem_->getLevelState(levelId), LevelState::Loaded);

    // Set it active
    levelSystem_->setActiveLevel(levelId);
    EXPECT_EQ(levelSystem_->getActiveLevel().value(), levelId);

    // Load another level
    auto asset2 = createMockAsset(2);
    auto result2 = levelSystem_->loadLevel(asset2);
    ASSERT_TRUE(result2.has_value());
    LevelId levelId2 = result2.value();

    // Transition to new level with unload
    LevelTransition transition{
        .fromLevel = levelId,
        .toLevel = levelId2,
        .spawnPoint = std::nullopt,
        .unloadPrevious = true
    };
    levelSystem_->transition(transition);
    levelSystem_->update(0.016f);

    // Verify transition
    EXPECT_EQ(levelSystem_->getActiveLevel().value(), levelId2);
    EXPECT_EQ(levelSystem_->getLevelState(levelId), LevelState::Unloaded);
    EXPECT_EQ(levelSystem_->getLevelState(levelId2), LevelState::Loaded);

    // Verify only one level is loaded
    auto loadedLevels = levelSystem_->getLoadedLevels();
    EXPECT_EQ(loadedLevels.size(), 1);
}

//==============================================================================
// Lua Integration Tests
//==============================================================================

class LevelSystemLuaTest : public ::testing::Test {
protected:
    void SetUp() override {
        assetSystem_ = createAssetSystem();

        // Create LevelSystem and initialize with AssetSystem
        auto levelSystemImpl = std::make_unique<LevelSystem>();
        levelSystemImpl->initialize(assetSystem_.get());
        levelSystem_ = std::move(levelSystemImpl);
    }

    std::unique_ptr<IAssetSystem> assetSystem_;
    std::unique_ptr<ILevelSystem> levelSystem_;
};

TEST_F(LevelSystemLuaTest, LoadLevelWithSpawnPoints) {
    // Register and load the test level asset
    AssetHandle handle = assetSystem_->registerAsset(
        AssetType::Level,
        "../../../tests/testdata/test_level_with_spawns.lua"
    );
    assetSystem_->loadAsset(handle);
    ASSERT_TRUE(assetSystem_->isLoaded(handle));

    // Load the level through the level system
    auto result = levelSystem_->loadLevel(handle);
    ASSERT_TRUE(result.has_value());
    LevelId levelId = result.value();

    // Verify metadata was parsed
    LevelMetadata metadata = levelSystem_->getLevelMetadata(levelId);
    EXPECT_EQ(metadata.levelName, "Test Level With Spawns");
    EXPECT_EQ(metadata.width, 2000.0f);
    EXPECT_EQ(metadata.height, 1200.0f);

    // Verify spawn points were parsed
    auto spawnNames = levelSystem_->getSpawnPointNames(levelId);
    EXPECT_EQ(spawnNames.size(), 5);

    // Check default spawn point
    auto defaultSpawn = levelSystem_->getSpawnPoint(levelId, "default");
    ASSERT_TRUE(defaultSpawn.has_value());
    EXPECT_FLOAT_EQ(defaultSpawn->x, 100.0f);
    EXPECT_FLOAT_EQ(defaultSpawn->y, 500.0f);
    EXPECT_FLOAT_EQ(defaultSpawn->rotation, 0.0f);

    // Check checkpoint1
    auto checkpoint1 = levelSystem_->getSpawnPoint(levelId, "checkpoint1");
    ASSERT_TRUE(checkpoint1.has_value());
    EXPECT_FLOAT_EQ(checkpoint1->x, 500.0f);
    EXPECT_FLOAT_EQ(checkpoint1->y, 400.0f);

    // Check boss_room (with rotation)
    auto bossRoom = levelSystem_->getSpawnPoint(levelId, "boss_room");
    ASSERT_TRUE(bossRoom.has_value());
    EXPECT_FLOAT_EQ(bossRoom->x, 1800.0f);
    EXPECT_FLOAT_EQ(bossRoom->y, 600.0f);
    EXPECT_FLOAT_EQ(bossRoom->rotation, 180.0f);
}

TEST_F(LevelSystemLuaTest, LoadBasicLevel) {
    // Register and load the basic test level
    AssetHandle handle = assetSystem_->registerAsset(
        AssetType::Level,
        "../../../tests/testdata/test_level.lua"
    );
    assetSystem_->loadAsset(handle);
    ASSERT_TRUE(assetSystem_->isLoaded(handle));

    // Load the level
    auto result = levelSystem_->loadLevel(handle);
    ASSERT_TRUE(result.has_value());
    LevelId levelId = result.value();

    // Verify metadata
    LevelMetadata metadata = levelSystem_->getLevelMetadata(levelId);
    EXPECT_EQ(metadata.levelName, "Test Level 1");
    EXPECT_EQ(metadata.width, 1920.0f);
    EXPECT_EQ(metadata.height, 1080.0f);

    // Basic level has no spawn points
    auto spawnNames = levelSystem_->getSpawnPointNames(levelId);
    EXPECT_TRUE(spawnNames.empty());
}

//==============================================================================
// Entity Definition Tests
//==============================================================================

TEST_F(LevelSystemTest, GetEntityDefsForNonExistentLevel) {
    auto entityDefs = levelSystem_->getEntityDefs(999999);
    EXPECT_TRUE(entityDefs.empty());
}

TEST_F(LevelSystemTest, GetEntityDefsReturnsEmptyForNewLevel) {
    auto asset = createMockAsset(1);
    auto result = levelSystem_->loadLevel(asset);
    ASSERT_TRUE(result.has_value());

    auto entityDefs = levelSystem_->getEntityDefs(result.value());
    EXPECT_TRUE(entityDefs.empty());
}

TEST_F(LevelSystemLuaTest, LoadLevelWithEntityDefinitions) {
    // Register and load the test level asset with entities
    AssetHandle handle = assetSystem_->registerAsset(
        AssetType::Level,
        "../../../tests/testdata/test_level_with_entities.lua"
    );
    assetSystem_->loadAsset(handle);
    ASSERT_TRUE(assetSystem_->isLoaded(handle));

    // Load the level through the level system
    auto result = levelSystem_->loadLevel(handle);
    ASSERT_TRUE(result.has_value());
    LevelId levelId = result.value();

    // Verify entity definitions were parsed
    auto entityDefs = levelSystem_->getEntityDefs(levelId);
    EXPECT_EQ(entityDefs.size(), 5);

    // Check platform entity
    const auto& platform = entityDefs[0];
    EXPECT_EQ(platform.type, "platform");
    EXPECT_FLOAT_EQ(platform.transform.x, 0.0f);
    EXPECT_FLOAT_EQ(platform.transform.y, 550.0f);
    // Lua stores all numbers as doubles
    EXPECT_NEAR(std::any_cast<double>(platform.properties.at("width")), 800.0, 0.01);
    EXPECT_NEAR(std::any_cast<double>(platform.properties.at("height")), 50.0, 0.01);

    // Check enemy entity
    const auto& enemy = entityDefs[1];
    EXPECT_EQ(enemy.type, "enemy");
    EXPECT_FLOAT_EQ(enemy.transform.x, 400.0f);
    EXPECT_FLOAT_EQ(enemy.transform.y, 500.0f);
    EXPECT_NEAR(std::any_cast<double>(enemy.properties.at("patrolRange")), 100.0, 0.01);
    EXPECT_NEAR(std::any_cast<double>(enemy.properties.at("speed")), 50.0, 0.01);
    EXPECT_EQ(std::any_cast<bool>(enemy.properties.at("hostile")), true);

    // Check collectible entity
    const auto& collectible = entityDefs[2];
    EXPECT_EQ(collectible.type, "collectible");
    EXPECT_FLOAT_EQ(collectible.transform.x, 200.0f);
    EXPECT_FLOAT_EQ(collectible.transform.y, 450.0f);
    EXPECT_NEAR(std::any_cast<double>(collectible.properties.at("value")), 10.0, 0.01);
    EXPECT_EQ(std::any_cast<std::string>(collectible.properties.at("collectType")), "coin");

    // Check rotating platform with transform properties
    const auto& rotatingPlatform = entityDefs[3];
    EXPECT_EQ(rotatingPlatform.type, "rotating_platform");
    EXPECT_FLOAT_EQ(rotatingPlatform.transform.x, 600.0f);
    EXPECT_FLOAT_EQ(rotatingPlatform.transform.y, 300.0f);
    EXPECT_FLOAT_EQ(rotatingPlatform.transform.rotation, 45.0f);
    EXPECT_FLOAT_EQ(rotatingPlatform.transform.scaleX, 2.0f);
    EXPECT_FLOAT_EQ(rotatingPlatform.transform.scaleY, 1.5f);

    // Check trigger entity with various property types
    const auto& trigger = entityDefs[4];
    EXPECT_EQ(trigger.type, "trigger");
    EXPECT_FLOAT_EQ(trigger.transform.x, 800.0f);
    EXPECT_FLOAT_EQ(trigger.transform.y, 400.0f);

    // Verify various property types
    EXPECT_NEAR(std::any_cast<double>(trigger.properties.at("radius")), 50.5, 0.01);
    EXPECT_EQ(std::any_cast<bool>(trigger.properties.at("active")), true);
    EXPECT_EQ(std::any_cast<std::string>(trigger.properties.at("message")), "You found a secret!");
    EXPECT_NEAR(std::any_cast<double>(trigger.properties.at("triggerCount")), 1.0, 0.01);
}

TEST_F(LevelSystemLuaTest, EntityDefinitionsFromSpawnLevel) {
    // Test the existing test_level_with_spawns.lua which also has an entity
    AssetHandle handle = assetSystem_->registerAsset(
        AssetType::Level,
        "../../../tests/testdata/test_level_with_spawns.lua"
    );
    assetSystem_->loadAsset(handle);
    ASSERT_TRUE(assetSystem_->isLoaded(handle));

    auto result = levelSystem_->loadLevel(handle);
    ASSERT_TRUE(result.has_value());
    LevelId levelId = result.value();

    // Should have 1 entity (the platform)
    auto entityDefs = levelSystem_->getEntityDefs(levelId);
    EXPECT_EQ(entityDefs.size(), 1);

    if (!entityDefs.empty()) {
        const auto& entity = entityDefs[0];
        EXPECT_EQ(entity.type, "platform");
        EXPECT_FLOAT_EQ(entity.transform.x, 0.0f);
        EXPECT_FLOAT_EQ(entity.transform.y, 1000.0f);
    }
}

TEST_F(LevelSystemTest, GetEntityDefsAfterUnload) {
    auto asset = createMockAsset(1);
    auto result = levelSystem_->loadLevel(asset);
    ASSERT_TRUE(result.has_value());
    LevelId levelId = result.value();

    levelSystem_->unloadLevel(levelId);

    auto entityDefs = levelSystem_->getEntityDefs(levelId);
    EXPECT_TRUE(entityDefs.empty());
}

}  // namespace jframe::tests

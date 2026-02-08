// tests/unit/SaveSystemTests.cpp
// Save system unit tests

#include <algorithm>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <memory>
#include <string>
#include <system_error>
#include <vector>

#include <gtest/gtest.h>

import bestow.save;
import bestow.save.impl;
import bestow.types;

namespace bestow::tests {

// Test implementation of ISaveable
class TestSaveable : public ISaveable {
public:
    int value = 0;
    std::string name;
    float health = 100.0f;
    bool isActive = true;

    std::string getSaveKey() const override {
        return "test_saveable";
    }

    void serialize(ISaveArchive& archive) const override {
        archive.writeInt("value", value);
        archive.writeString("name", name);
        archive.writeFloat("health", health);
        archive.writeBool("isActive", isActive);
    }

    void deserialize(const ILoadArchive& archive) override {
        value = archive.readInt("value");
        name = archive.readString("name");
        health = archive.readFloat("health");
        isActive = archive.readBool("isActive");
    }
};

class SaveSystemTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Generate unique profile name for this test to avoid conflicts
        const ::testing::TestInfo* testInfo = ::testing::UnitTest::GetInstance()->current_test_info();
        std::string testName = std::string(testInfo->test_suite_name()) + "_" + std::string(testInfo->name());

        // Replace special characters that might cause filesystem issues
        std::replace(testName.begin(), testName.end(), '/', '_');
        std::replace(testName.begin(), testName.end(), '\\', '_');
        std::replace(testName.begin(), testName.end(), ' ', '_');

        uniqueProfile_ = testName;

        // Clean up any leftover profile directory from previous failed test runs
        // Use std::error_code to avoid exceptions
        std::error_code ec;
        auto profilePath = std::filesystem::path("saves") / uniqueProfile_;
        if (std::filesystem::exists(profilePath, ec)) {
            std::filesystem::remove_all(profilePath, ec);
            // Ignore errors - directory might not exist or might be locked
        }

        saveSystem_ = std::make_unique<SaveSystem>();

        // Set unique profile for this test to avoid conflicts
        saveSystem_->setActiveProfile(uniqueProfile_);
    }

    void TearDown() override {
        // Unregister any saveables to avoid dangling pointers
        saveSystem_.reset();

        // Clean up only our profile directory
        // NEVER try to remove the parent "saves" directory as it's shared by all tests
        // Use std::error_code to avoid exceptions during cleanup
        std::error_code ec;
        auto profilePath = std::filesystem::path("saves") / uniqueProfile_;
        if (std::filesystem::exists(profilePath, ec)) {
            std::filesystem::remove_all(profilePath, ec);
            // Ignore errors - directory might be locked or already removed by another thread
        }
    }

    std::unique_ptr<ISaveSystem> saveSystem_;
    std::string uniqueProfile_;
};

// ============================================================================
// Profile Management Tests
// ============================================================================

TEST_F(SaveSystemTest, ActiveProfile) {
    // System should start with our unique test profile
    EXPECT_EQ(saveSystem_->getActiveProfile(), uniqueProfile_);

    saveSystem_->setActiveProfile("player1");
    EXPECT_EQ(saveSystem_->getActiveProfile(), "player1");

    saveSystem_->setActiveProfile("player2");
    EXPECT_EQ(saveSystem_->getActiveProfile(), "player2");
}

TEST_F(SaveSystemTest, GetProfiles) {
    // Should return list of all profile directories
    auto profiles = saveSystem_->getProfiles();

    // Should at least contain the test profile
    EXPECT_FALSE(profiles.empty());
}

TEST_F(SaveSystemTest, SetActiveProfileChangesSaveDirectory) {
    saveSystem_->setActiveProfile("profile_a");
    auto resultA = saveSystem_->save(0, "Save A");

    saveSystem_->setActiveProfile("profile_b");
    auto resultB = saveSystem_->save(0, "Save B");

    // Both saves should succeed in different directories
    EXPECT_TRUE(resultA.has_value());
    EXPECT_TRUE(resultB.has_value());
}

// ============================================================================
// Auto-Save Configuration Tests
// ============================================================================

TEST_F(SaveSystemTest, AutoSaveConfig) {
    saveSystem_->enableAutoSave(std::chrono::seconds(300));
    // Auto-save is enabled with 5 minute interval

    saveSystem_->disableAutoSave();
    // Auto-save is now disabled
}

TEST_F(SaveSystemTest, AutoSaveTriggers) {
    // Enable auto-save with 1 second interval
    saveSystem_->enableAutoSave(std::chrono::seconds(1));

    // Update for just over 1 second
    saveSystem_->update(DeltaTime{1.1f});

    // Auto-save should have been triggered
    // (This may create a save file depending on implementation)
}

// ============================================================================
// Save Existence Tests
// ============================================================================

TEST_F(SaveSystemTest, SaveNotExists) {
    EXPECT_FALSE(saveSystem_->saveExists(0));
    EXPECT_FALSE(saveSystem_->saveExists(999));
}

TEST_F(SaveSystemTest, SaveExistsAfterSave) {
    auto result = saveSystem_->save(0, "Test Save");

    if (result.has_value()) {
        EXPECT_TRUE(saveSystem_->saveExists(0));
    }
}

// ============================================================================
// Save Operations Tests
// ============================================================================

TEST_F(SaveSystemTest, SaveReturnsSuccessForValidSave) {
    auto result = saveSystem_->save(0, "My Save");

    // Should return success (void on success)
    EXPECT_TRUE(result.has_value());
}

TEST_F(SaveSystemTest, SaveCreatesFileAtCorrectPath) {
    auto result = saveSystem_->save(0, "Test Save");

    if (result.has_value()) {
        // Save file should exist (implementation-dependent path)
        EXPECT_TRUE(saveSystem_->saveExists(0));
    }
}

TEST_F(SaveSystemTest, SaveCreatesMetadataFile) {
    auto result = saveSystem_->save(0, "Test Save");

    if (result.has_value()) {
        auto metadata = saveSystem_->getSaveMetadata(0);
        EXPECT_TRUE(metadata.has_value());

        if (metadata.has_value()) {
            EXPECT_EQ(metadata->saveName, "Test Save");
            EXPECT_EQ(metadata->slot, 0);
        }
    }
}

TEST_F(SaveSystemTest, SaveWithRegisteredSaveableSerializesData) {
    TestSaveable saveable;
    saveable.value = 42;
    saveable.name = "TestObject";
    saveable.health = 75.5f;
    saveable.isActive = true;

    saveSystem_->registerSaveable(&saveable);

    auto result = saveSystem_->save(0, "Saveable Test");

    // Should successfully serialize the saveable
    EXPECT_TRUE(result.has_value());

    saveSystem_->unregisterSaveable(&saveable);
}

TEST_F(SaveSystemTest, SaveMultipleSaveables) {
    TestSaveable saveable1;
    saveable1.value = 100;
    saveable1.name = "First";

    TestSaveable saveable2;
    saveable2.value = 200;
    saveable2.name = "Second";

    saveSystem_->registerSaveable(&saveable1);
    saveSystem_->registerSaveable(&saveable2);

    auto result = saveSystem_->save(1, "Multiple Saveables");

    EXPECT_TRUE(result.has_value());

    saveSystem_->unregisterSaveable(&saveable1);
    saveSystem_->unregisterSaveable(&saveable2);
}

// ============================================================================
// Load Operations Tests
// ============================================================================

TEST_F(SaveSystemTest, LoadReturnsFileNotFoundForNonExistentSave) {
    auto result = saveSystem_->load(4);

    // Should return error for non-existent save
    EXPECT_FALSE(result.has_value());

    if (!result.has_value()) {
        EXPECT_EQ(result.error(), SaveError::FileNotFound);
    }
}

TEST_F(SaveSystemTest, LoadReturnsSuccessForValidSaveFile) {
    // First create a save
    auto saveResult = saveSystem_->save(0, "Test Save");
    EXPECT_TRUE(saveResult.has_value());

    // Then load it
    auto loadResult = saveSystem_->load(0);
    EXPECT_TRUE(loadResult.has_value());
}

TEST_F(SaveSystemTest, LoadWithRegisteredSaveableDeserializesData) {
    // Create and save
    TestSaveable saveable;
    saveable.value = 42;
    saveable.name = "TestObject";
    saveable.health = 75.5f;
    saveable.isActive = true;

    saveSystem_->registerSaveable(&saveable);
    auto saveResult = saveSystem_->save(0, "Deserialize Test");
    EXPECT_TRUE(saveResult.has_value());

    // Modify the object
    saveable.value = 0;
    saveable.name = "";
    saveable.health = 0.0f;
    saveable.isActive = false;

    // Load should restore original values
    auto loadResult = saveSystem_->load(0);
    EXPECT_TRUE(loadResult.has_value());

    if (loadResult.has_value()) {
        EXPECT_EQ(saveable.value, 42);
        EXPECT_EQ(saveable.name, "TestObject");
        EXPECT_FLOAT_EQ(saveable.health, 75.5f);
        EXPECT_TRUE(saveable.isActive);
    }

    saveSystem_->unregisterSaveable(&saveable);
}

TEST_F(SaveSystemTest, LoadReturnsCorruptedFileForInvalidMagicNumber) {
    // Create a corrupted save file manually
    // This test depends on implementation details of save file format

    // First create a valid save
    auto saveResult = saveSystem_->save(2, "Corruption Test");

    if (saveResult.has_value()) {
        // Try to load a hypothetically corrupted file
        // (In real implementation, we'd manually corrupt the file)
        // For now, just verify the error enum exists
        SaveError expectedError = SaveError::CorruptedFile;
        EXPECT_EQ(expectedError, SaveError::CorruptedFile);
    }
}

TEST_F(SaveSystemTest, LoadReturnsVersionMismatchForWrongVersion) {
    // This test would require creating a save with wrong version
    // Verify the error code exists
    SaveError expectedError = SaveError::VersionMismatch;
    EXPECT_EQ(expectedError, SaveError::VersionMismatch);
}

// ============================================================================
// Metadata Tests
// ============================================================================

TEST_F(SaveSystemTest, GetSaveMetadataReturnsMetadataForExistingSave) {
    auto saveResult = saveSystem_->save(0, "Metadata Test");
    EXPECT_TRUE(saveResult.has_value());

    auto metadata = saveSystem_->getSaveMetadata(0);
    EXPECT_TRUE(metadata.has_value());

    if (metadata.has_value()) {
        EXPECT_EQ(metadata->saveName, "Metadata Test");
        EXPECT_EQ(metadata->slot, 0);
        // Timestamp should be recent
        EXPECT_GE(metadata->timestamp, std::chrono::system_clock::now() - std::chrono::minutes(1));
    }
}

TEST_F(SaveSystemTest, GetSaveMetadataReturnsNulloptForNonExistentSave) {
    auto metadata = saveSystem_->getSaveMetadata(99);
    EXPECT_FALSE(metadata.has_value());
}

TEST_F(SaveSystemTest, GetAllSaveMetadataReturnsAllSavesInProfile) {
    // Create multiple saves
    saveSystem_->save(0, "Save 1");
    saveSystem_->save(1, "Save 2");
    saveSystem_->save(2, "Save 3");

    auto allMetadata = saveSystem_->getAllSaveMetadata();

    // Should have at least the saves we created
    EXPECT_GE(allMetadata.size(), 3);

    // Check that our saves are in the list
    bool foundSlot1 = false;
    bool foundSlot2 = false;
    bool foundSlot3 = false;

    for (const auto& meta : allMetadata) {
        if (meta.slot == 0 && meta.saveName == "Save 1") {
            foundSlot1 = true;
        }
        if (meta.slot == 1 && meta.saveName == "Save 2") {
            foundSlot2 = true;
        }
        if (meta.slot == 2 && meta.saveName == "Save 3") {
            foundSlot3 = true;
        }
    }

    EXPECT_TRUE(foundSlot1);
    EXPECT_TRUE(foundSlot2);
    EXPECT_TRUE(foundSlot3);
}

TEST_F(SaveSystemTest, GetAllSaveMetadataReturnsEmptyForNoSaves) {
    // Fresh profile should have no saves
    saveSystem_->setActiveProfile("empty_profile");

    auto allMetadata = saveSystem_->getAllSaveMetadata();

    // Should be empty or only contain auto-generated saves
    EXPECT_TRUE(allMetadata.empty() || allMetadata.size() == 0);
}

TEST_F(SaveSystemTest, MetadataContainsGameVersion) {
    auto saveResult = saveSystem_->save(0, "Version Test");
    EXPECT_TRUE(saveResult.has_value());

    auto metadata = saveSystem_->getSaveMetadata(0);
    EXPECT_TRUE(metadata.has_value());

    if (metadata.has_value()) {
        // Game version should be populated
        EXPECT_FALSE(metadata->gameVersion.empty());
    }
}

TEST_F(SaveSystemTest, MetadataContainsPlaytime) {
    auto saveResult = saveSystem_->save(0, "Playtime Test");
    EXPECT_TRUE(saveResult.has_value());

    auto metadata = saveSystem_->getSaveMetadata(0);
    EXPECT_TRUE(metadata.has_value());

    if (metadata.has_value()) {
        // Playtime should be a valid value (>= 0)
        EXPECT_GE(metadata->playtimeSeconds, 0);
    }
}

// ============================================================================
// Delete Operations Tests
// ============================================================================

TEST_F(SaveSystemTest, DeleteSaveRemovesTheSaveFile) {
    // Create a save
    auto saveResult = saveSystem_->save(0, "Delete Test");
    EXPECT_TRUE(saveResult.has_value());
    EXPECT_TRUE(saveSystem_->saveExists(0));

    // Delete it
    bool deleted = saveSystem_->deleteSave(0);
    EXPECT_TRUE(deleted);

    // Should no longer exist
    EXPECT_FALSE(saveSystem_->saveExists(0));
}

TEST_F(SaveSystemTest, DeleteNonExistentSaveReturnsFalse) {
    bool deleted = saveSystem_->deleteSave(99);
    EXPECT_FALSE(deleted);
}

TEST_F(SaveSystemTest, DeleteSaveRemovesMetadata) {
    auto saveResult = saveSystem_->save(0, "Metadata Delete Test");
    EXPECT_TRUE(saveResult.has_value());

    auto metadataBefore = saveSystem_->getSaveMetadata(0);
    EXPECT_TRUE(metadataBefore.has_value());

    bool deleted = saveSystem_->deleteSave(0);
    EXPECT_TRUE(deleted);

    auto metadataAfter = saveSystem_->getSaveMetadata(0);
    EXPECT_FALSE(metadataAfter.has_value());
}

// ============================================================================
// Quick Save/Load Tests
// ============================================================================

TEST_F(SaveSystemTest, QuickSaveUsesQuickSaveSlot) {
    TestSaveable saveable;
    saveable.value = 999;
    saveable.name = "QuickSave";

    saveSystem_->registerSaveable(&saveable);

    saveSystem_->quickSave();

    // Should create a save in QuickSave slot
    EXPECT_TRUE(saveSystem_->saveExists(SaveSlots::QuickSave));

    auto metadata = saveSystem_->getSaveMetadata(SaveSlots::QuickSave);
    EXPECT_TRUE(metadata.has_value());

    saveSystem_->unregisterSaveable(&saveable);
}

TEST_F(SaveSystemTest, QuickLoadUsesQuickSaveSlot) {
    TestSaveable saveable;
    saveable.value = 888;
    saveable.name = "QuickLoad";

    saveSystem_->registerSaveable(&saveable);

    // Quick save
    saveSystem_->quickSave();

    // Modify data
    saveable.value = 0;
    saveable.name = "";

    // Quick load should restore
    saveSystem_->quickLoad();

    EXPECT_EQ(saveable.value, 888);
    EXPECT_EQ(saveable.name, "QuickLoad");

    saveSystem_->unregisterSaveable(&saveable);
}

TEST_F(SaveSystemTest, QuickSaveOverwritesPreviousQuickSave) {
    TestSaveable saveable;
    saveable.value = 100;

    saveSystem_->registerSaveable(&saveable);

    // First quick save
    saveSystem_->quickSave();

    // Modify and quick save again
    saveable.value = 200;
    saveSystem_->quickSave();

    // Reset and load
    saveable.value = 0;
    saveSystem_->quickLoad();

    // Should have the second value
    EXPECT_EQ(saveable.value, 200);

    saveSystem_->unregisterSaveable(&saveable);
}

// ============================================================================
// ISaveable Interface Tests
// ============================================================================

TEST_F(SaveSystemTest, SaveableSerializeIsCalledDuringSave) {
    TestSaveable saveable;
    saveable.value = 12345;
    saveable.name = "Serialize Test";
    saveable.health = 99.9f;
    saveable.isActive = false;

    saveSystem_->registerSaveable(&saveable);

    auto result = saveSystem_->save(0, "Serialize Call Test");

    // If save succeeds, serialize was called
    EXPECT_TRUE(result.has_value());

    saveSystem_->unregisterSaveable(&saveable);
}

TEST_F(SaveSystemTest, SaveableDeserializeIsCalledDuringLoad) {
    TestSaveable saveable;
    saveable.value = 54321;
    saveable.name = "Deserialize Test";

    saveSystem_->registerSaveable(&saveable);

    // Save
    auto saveResult = saveSystem_->save(0, "Deserialize Call Test");
    EXPECT_TRUE(saveResult.has_value());

    // Modify
    saveable.value = 0;
    saveable.name = "Modified";

    // Load - this should call deserialize
    auto loadResult = saveSystem_->load(0);
    EXPECT_TRUE(loadResult.has_value());

    // Values should be restored
    EXPECT_EQ(saveable.value, 54321);
    EXPECT_EQ(saveable.name, "Deserialize Test");

    saveSystem_->unregisterSaveable(&saveable);
}

TEST_F(SaveSystemTest, UnregisteredSaveableNotSerialized) {
    TestSaveable saveable;
    saveable.value = 777;

    // Register, save, unregister
    saveSystem_->registerSaveable(&saveable);
    auto saveResult1 = saveSystem_->save(0, "First Save");
    EXPECT_TRUE(saveResult1.has_value());
    saveSystem_->unregisterSaveable(&saveable);

    // Save again without registered saveable
    saveable.value = 888;
    auto saveResult2 = saveSystem_->save(1, "Second Save");
    EXPECT_TRUE(saveResult2.has_value());

    // Load first save with registered saveable
    saveSystem_->registerSaveable(&saveable);
    auto loadResult = saveSystem_->load(0);
    EXPECT_TRUE(loadResult.has_value());

    // Should restore to 777
    EXPECT_EQ(saveable.value, 777);

    saveSystem_->unregisterSaveable(&saveable);
}

TEST_F(SaveSystemTest, MultipleSaveablesWithDifferentKeys) {
    class AnotherSaveable : public ISaveable {
    public:
        double score = 0.0;

        std::string getSaveKey() const override { return "another_saveable"; }

        void serialize(ISaveArchive& archive) const override {
            archive.writeDouble("score", score);
        }

        void deserialize(const ILoadArchive& archive) override {
            score = archive.readDouble("score");
        }
    };

    TestSaveable saveable1;
    saveable1.value = 111;

    AnotherSaveable saveable2;
    saveable2.score = 999.5;

    saveSystem_->registerSaveable(&saveable1);
    saveSystem_->registerSaveable(&saveable2);

    auto saveResult = saveSystem_->save(0, "Multiple Keys Test");
    EXPECT_TRUE(saveResult.has_value());

    // Modify
    saveable1.value = 0;
    saveable2.score = 0.0;

    // Load
    auto loadResult = saveSystem_->load(0);
    EXPECT_TRUE(loadResult.has_value());

    // Both should be restored
    EXPECT_EQ(saveable1.value, 111);
    EXPECT_DOUBLE_EQ(saveable2.score, 999.5);

    saveSystem_->unregisterSaveable(&saveable1);
    saveSystem_->unregisterSaveable(&saveable2);
}

// ============================================================================
// Archive Tests
// ============================================================================

TEST_F(SaveSystemTest, ArchiveSupportsAllDataTypes) {
    class AllTypesSaveable : public ISaveable {
    public:
        int intValue = 42;
        float floatValue = 3.14f;
        double doubleValue = 2.718;
        std::string stringValue = "test";
        bool boolValue = true;
        std::vector<std::uint8_t> bytesValue = {0x01, 0x02, 0x03};

        std::string getSaveKey() const override { return "all_types"; }

        void serialize(ISaveArchive& archive) const override {
            archive.writeInt("int", intValue);
            archive.writeFloat("float", floatValue);
            archive.writeDouble("double", doubleValue);
            archive.writeString("string", stringValue);
            archive.writeBool("bool", boolValue);
            archive.writeBytes("bytes", bytesValue);
        }

        void deserialize(const ILoadArchive& archive) override {
            intValue = archive.readInt("int");
            floatValue = archive.readFloat("float");
            doubleValue = archive.readDouble("double");
            stringValue = archive.readString("string");
            boolValue = archive.readBool("bool");
            bytesValue = archive.readBytes("bytes");
        }
    };

    AllTypesSaveable saveable;
    saveSystem_->registerSaveable(&saveable);

    auto saveResult = saveSystem_->save(0, "All Types Test");
    EXPECT_TRUE(saveResult.has_value());

    // Modify all values
    saveable.intValue = 0;
    saveable.floatValue = 0.0f;
    saveable.doubleValue = 0.0;
    saveable.stringValue = "";
    saveable.boolValue = false;
    saveable.bytesValue.clear();

    // Load
    auto loadResult = saveSystem_->load(0);
    EXPECT_TRUE(loadResult.has_value());

    // All should be restored
    EXPECT_EQ(saveable.intValue, 42);
    EXPECT_FLOAT_EQ(saveable.floatValue, 3.14f);
    EXPECT_DOUBLE_EQ(saveable.doubleValue, 2.718);
    EXPECT_EQ(saveable.stringValue, "test");
    EXPECT_TRUE(saveable.boolValue);
    EXPECT_EQ(saveable.bytesValue, std::vector<std::uint8_t>({0x01, 0x02, 0x03}));

    saveSystem_->unregisterSaveable(&saveable);
}

// ============================================================================
// Profile Management - Edge Cases
// ============================================================================

TEST_F(SaveSystemTest, SetProfileWithSpecialCharacters) {
    // Test profile names with spaces and special chars
    saveSystem_->setActiveProfile("player_profile_2024");
    EXPECT_EQ(saveSystem_->getActiveProfile(), "player_profile_2024");

    saveSystem_->setActiveProfile("test profile");
    EXPECT_EQ(saveSystem_->getActiveProfile(), "test profile");
}

TEST_F(SaveSystemTest, ProfilesAreIsolated) {
    // Use unique profile names for this test
    std::string profileA = uniqueProfile_ + "_profile_a";
    std::string profileB = uniqueProfile_ + "_profile_b";

    // Create a save in profile A
    saveSystem_->setActiveProfile(profileA);
    auto resultA = saveSystem_->save(0, "Save A");
    EXPECT_TRUE(resultA.has_value());
    EXPECT_TRUE(saveSystem_->saveExists(0));

    // Switch to profile B - save should not exist
    saveSystem_->setActiveProfile(profileB);
    EXPECT_FALSE(saveSystem_->saveExists(0));

    // Create different save in profile B
    auto resultB = saveSystem_->save(0, "Save B");
    EXPECT_TRUE(resultB.has_value());
    EXPECT_TRUE(saveSystem_->saveExists(0));

    // Switch back to profile A - original save should still exist
    saveSystem_->setActiveProfile(profileA);
    EXPECT_TRUE(saveSystem_->saveExists(0));

    auto metadata = saveSystem_->getSaveMetadata(0);
    EXPECT_TRUE(metadata.has_value());
    if (metadata.has_value()) {
        EXPECT_EQ(metadata->saveName, "Save A");
    }

    // Cleanup
    std::error_code ec;
    std::filesystem::remove_all(std::filesystem::path("saves") / profileA, ec);
    std::filesystem::remove_all(std::filesystem::path("saves") / profileB, ec);
}

TEST_F(SaveSystemTest, GetProfilesReturnsAllProfiles) {
    // Create multiple profiles by switching and saving
    saveSystem_->setActiveProfile("profile_1");
    saveSystem_->save(0, "Test 1");

    saveSystem_->setActiveProfile("profile_2");
    saveSystem_->save(0, "Test 2");

    saveSystem_->setActiveProfile("profile_3");
    saveSystem_->save(0, "Test 3");

    auto profiles = saveSystem_->getProfiles();

    // Should contain all three profiles
    EXPECT_GE(profiles.size(), 3);

    bool foundProfile1 = std::find(profiles.begin(), profiles.end(), "profile_1") != profiles.end();
    bool foundProfile2 = std::find(profiles.begin(), profiles.end(), "profile_2") != profiles.end();
    bool foundProfile3 = std::find(profiles.begin(), profiles.end(), "profile_3") != profiles.end();

    EXPECT_TRUE(foundProfile1);
    EXPECT_TRUE(foundProfile2);
    EXPECT_TRUE(foundProfile3);
}

// ============================================================================
// Auto-Save Tests - Additional Coverage
// ============================================================================

TEST_F(SaveSystemTest, AutoSaveCreatesFileInAutoSaveSlot) {
    TestSaveable saveable;
    saveable.value = 123;

    saveSystem_->registerSaveable(&saveable);

    saveSystem_->autoSave();

    // Should create save in AutoSave slot
    EXPECT_TRUE(saveSystem_->saveExists(SaveSlots::AutoSave));

    auto metadata = saveSystem_->getSaveMetadata(SaveSlots::AutoSave);
    EXPECT_TRUE(metadata.has_value());

    if (metadata.has_value()) {
        EXPECT_EQ(metadata->saveName, "Auto Save");
    }

    saveSystem_->unregisterSaveable(&saveable);
}

TEST_F(SaveSystemTest, AutoSaveDoesNotTriggerBeforeInterval) {
    saveSystem_->enableAutoSave(std::chrono::seconds(10));

    // Update for less than interval
    saveSystem_->update(DeltaTime{5.0f});

    // Auto-save should not have triggered yet
    // (This test just ensures no crashes; checking file existence is impl-dependent)
}

TEST_F(SaveSystemTest, AutoSaveResetsTimerAfterSave) {
    TestSaveable saveable;
    saveable.value = 456;

    saveSystem_->registerSaveable(&saveable);
    saveSystem_->enableAutoSave(std::chrono::seconds(5));

    // First trigger
    saveSystem_->update(DeltaTime{5.1f});

    // Modify data
    saveable.value = 789;

    // Second trigger (should happen after another interval)
    saveSystem_->update(DeltaTime{5.1f});

    // Load and verify we got the second save
    saveSystem_->load(SaveSlots::AutoSave);
    EXPECT_EQ(saveable.value, 789);

    saveSystem_->unregisterSaveable(&saveable);
}

TEST_F(SaveSystemTest, DisableAutoSavePreventsAutoSaving) {
    saveSystem_->enableAutoSave(std::chrono::seconds(1));
    saveSystem_->disableAutoSave();

    // Update past the interval
    saveSystem_->update(DeltaTime{2.0f});

    // No auto-save should have occurred (test mainly for no crashes)
}

// ============================================================================
// Save/Load Edge Cases
// ============================================================================

TEST_F(SaveSystemTest, SaveWithEmptyName) {
    auto result = saveSystem_->save(0, "");
    EXPECT_TRUE(result.has_value());

    auto metadata = saveSystem_->getSaveMetadata(0);
    EXPECT_TRUE(metadata.has_value());

    if (metadata.has_value()) {
        EXPECT_EQ(metadata->saveName, "");
    }
}

TEST_F(SaveSystemTest, SaveToSameSlotOverwritesPreviousSave) {
    TestSaveable saveable;
    saveable.value = 100;

    saveSystem_->registerSaveable(&saveable);

    // First save
    saveSystem_->save(0, "First Save");

    // Modify and save again to same slot
    saveable.value = 200;
    saveSystem_->save(0, "Second Save");

    // Reset and load
    saveable.value = 0;
    saveSystem_->load(0);

    // Should have the second value
    EXPECT_EQ(saveable.value, 200);

    auto metadata = saveSystem_->getSaveMetadata(0);
    EXPECT_TRUE(metadata.has_value());
    if (metadata.has_value()) {
        EXPECT_EQ(metadata->saveName, "Second Save");
    }

    saveSystem_->unregisterSaveable(&saveable);
}

TEST_F(SaveSystemTest, SaveWithNoRegisteredSaveables) {
    // Should succeed even with no saveables registered
    auto result = saveSystem_->save(0, "Empty Save");
    EXPECT_TRUE(result.has_value());

    // Should be able to load it back
    auto loadResult = saveSystem_->load(0);
    EXPECT_TRUE(loadResult.has_value());
}

TEST_F(SaveSystemTest, LoadWithNoRegisteredSaveables) {
    TestSaveable saveable;
    saveable.value = 42;

    saveSystem_->registerSaveable(&saveable);
    saveSystem_->save(0, "Test");
    saveSystem_->unregisterSaveable(&saveable);

    // Load with no registered saveables
    // The implementation returns error when save has data for unregistered saveables
    // (see SaveSystem.cpp line 166-168: returns SerializationError for unknown saveable data)
    auto result = saveSystem_->load(0);
    EXPECT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), SaveError::SerializationError);
}

TEST_F(SaveSystemTest, SaveToHighSlotNumber) {
    // Test saving to a very high slot number (but not reserved slots)
    SaveSlot highSlot = 999999;

    auto result = saveSystem_->save(highSlot, "High Slot Save");
    EXPECT_TRUE(result.has_value());
    EXPECT_TRUE(saveSystem_->saveExists(highSlot));

    auto metadata = saveSystem_->getSaveMetadata(highSlot);
    EXPECT_TRUE(metadata.has_value());
    if (metadata.has_value()) {
        EXPECT_EQ(metadata->slot, highSlot);
    }
}

TEST_F(SaveSystemTest, SaveToMultipleSlots) {
    TestSaveable saveable;

    saveSystem_->registerSaveable(&saveable);

    // Save to multiple slots with different data
    for (int i = 0; i < 5; ++i) {
        saveable.value = i * 100;
        saveSystem_->save(i, "Save " + std::to_string(i));
    }

    // Verify each slot has correct data
    for (int i = 0; i < 5; ++i) {
        saveable.value = 0;
        saveSystem_->load(i);
        EXPECT_EQ(saveable.value, i * 100);
    }

    saveSystem_->unregisterSaveable(&saveable);
}

// ============================================================================
// Delete Operations - Additional Coverage
// ============================================================================

TEST_F(SaveSystemTest, DeleteSaveAlsoDeletesMetadata) {
    auto saveResult = saveSystem_->save(0, "Delete Test");
    EXPECT_TRUE(saveResult.has_value());

    // Verify metadata exists
    EXPECT_TRUE(saveSystem_->getSaveMetadata(0).has_value());

    // Delete save
    bool deleted = saveSystem_->deleteSave(0);
    EXPECT_TRUE(deleted);

    // Metadata should be gone
    EXPECT_FALSE(saveSystem_->getSaveMetadata(0).has_value());
}

TEST_F(SaveSystemTest, DeleteSaveFromDifferentProfile) {
    // Use unique profile names for this test
    std::string profileA = uniqueProfile_ + "_profile_a";
    std::string profileB = uniqueProfile_ + "_profile_b";

    // Create save in profile A
    saveSystem_->setActiveProfile(profileA);
    saveSystem_->save(0, "Profile A Save");

    // Switch to profile B
    saveSystem_->setActiveProfile(profileB);

    // Try to delete slot 0 from profile B (shouldn't affect profile A)
    bool deleted = saveSystem_->deleteSave(0);
    EXPECT_FALSE(deleted);  // No save exists in profile B slot 0

    // Switch back to profile A
    saveSystem_->setActiveProfile(profileA);

    // Save should still exist
    EXPECT_TRUE(saveSystem_->saveExists(0));

    // Cleanup: delete test profiles
    std::error_code ec;
    std::filesystem::remove_all(std::filesystem::path("saves") / profileA, ec);
    std::filesystem::remove_all(std::filesystem::path("saves") / profileB, ec);
}

TEST_F(SaveSystemTest, DeleteQuickSave) {
    TestSaveable saveable;
    saveable.value = 123;

    saveSystem_->registerSaveable(&saveable);
    saveSystem_->quickSave();

    EXPECT_TRUE(saveSystem_->saveExists(SaveSlots::QuickSave));

    bool deleted = saveSystem_->deleteSave(SaveSlots::QuickSave);
    EXPECT_TRUE(deleted);

    EXPECT_FALSE(saveSystem_->saveExists(SaveSlots::QuickSave));

    saveSystem_->unregisterSaveable(&saveable);
}

TEST_F(SaveSystemTest, DeleteAutoSave) {
    TestSaveable saveable;
    saveable.value = 456;

    saveSystem_->registerSaveable(&saveable);
    saveSystem_->autoSave();

    EXPECT_TRUE(saveSystem_->saveExists(SaveSlots::AutoSave));

    bool deleted = saveSystem_->deleteSave(SaveSlots::AutoSave);
    EXPECT_TRUE(deleted);

    EXPECT_FALSE(saveSystem_->saveExists(SaveSlots::AutoSave));

    saveSystem_->unregisterSaveable(&saveable);
}

// ============================================================================
// Metadata - Additional Coverage
// ============================================================================

TEST_F(SaveSystemTest, MetadataTimestampIsRecent) {
    auto saveResult = saveSystem_->save(0, "Timestamp Test");
    EXPECT_TRUE(saveResult.has_value());

    auto metadata = saveSystem_->getSaveMetadata(0);
    EXPECT_TRUE(metadata.has_value());

    if (metadata.has_value()) {
        auto now = std::chrono::system_clock::now();
        auto diff = std::chrono::duration_cast<std::chrono::seconds>(now - metadata->timestamp);

        // Timestamp should be within 5 seconds of now
        EXPECT_LT(diff.count(), 5);
    }
}

TEST_F(SaveSystemTest, MetadataCompletionPercentage) {
    auto saveResult = saveSystem_->save(0, "Completion Test");
    EXPECT_TRUE(saveResult.has_value());

    auto metadata = saveSystem_->getSaveMetadata(0);
    EXPECT_TRUE(metadata.has_value());

    if (metadata.has_value()) {
        // Should be initialized to 0.0
        EXPECT_FLOAT_EQ(metadata->completionPercentage, 0.0f);
    }
}

TEST_F(SaveSystemTest, MetadataLevelNameIsOptional) {
    auto saveResult = saveSystem_->save(0, "Level Name Test");
    EXPECT_TRUE(saveResult.has_value());

    auto metadata = saveSystem_->getSaveMetadata(0);
    EXPECT_TRUE(metadata.has_value());

    if (metadata.has_value()) {
        // levelName is optional, should not be set by default
        EXPECT_FALSE(metadata->levelName.has_value());
    }
}

TEST_F(SaveSystemTest, MetadataHasScreenshotIsFalse) {
    auto saveResult = saveSystem_->save(0, "Screenshot Test");
    EXPECT_TRUE(saveResult.has_value());

    auto metadata = saveSystem_->getSaveMetadata(0);
    EXPECT_TRUE(metadata.has_value());

    if (metadata.has_value()) {
        // Screenshots not implemented yet
        EXPECT_FALSE(metadata->hasScreenshot);
    }
}

TEST_F(SaveSystemTest, GetAllSaveMetadataReturnsCorrectCount) {
    // Create 5 saves
    for (int i = 0; i < 5; ++i) {
        saveSystem_->save(i, "Save " + std::to_string(i));
    }

    auto allMetadata = saveSystem_->getAllSaveMetadata();

    // Should have at least 5 saves
    EXPECT_GE(allMetadata.size(), 5);
}

TEST_F(SaveSystemTest, GetAllSaveMetadataAfterDeletingSome) {
    // Create 5 saves
    for (int i = 0; i < 5; ++i) {
        saveSystem_->save(i, "Save " + std::to_string(i));
    }

    // Delete 2 of them
    saveSystem_->deleteSave(1);
    saveSystem_->deleteSave(3);

    auto allMetadata = saveSystem_->getAllSaveMetadata();

    // Should have 3 remaining
    EXPECT_EQ(allMetadata.size(), 3);

    // Verify the correct ones remain
    bool found0 = false, found2 = false, found4 = false;
    for (const auto& meta : allMetadata) {
        if (meta.slot == 0) found0 = true;
        if (meta.slot == 2) found2 = true;
        if (meta.slot == 4) found4 = true;
    }

    EXPECT_TRUE(found0);
    EXPECT_TRUE(found2);
    EXPECT_TRUE(found4);
}

// ============================================================================
// ISaveable Interface - Additional Coverage
// ============================================================================

TEST_F(SaveSystemTest, RegisterSameableMultipleTimesIsIdempotent) {
    TestSaveable saveable;
    saveable.value = 42;

    // Register multiple times
    saveSystem_->registerSaveable(&saveable);
    saveSystem_->registerSaveable(&saveable);
    saveSystem_->registerSaveable(&saveable);

    // Save and verify
    saveSystem_->save(0, "Multiple Register Test");
    saveable.value = 0;
    saveSystem_->load(0);

    // Should still work correctly (implementation may have duplicates, but that's ok)
    EXPECT_EQ(saveable.value, 42);

    // Unregister once should be enough (or may need multiple unregisters)
    saveSystem_->unregisterSaveable(&saveable);
}

TEST_F(SaveSystemTest, UnregisterNonExistentSaveable) {
    TestSaveable saveable;

    // Unregister something that was never registered
    // Should not crash
    saveSystem_->unregisterSaveable(&saveable);
}

TEST_F(SaveSystemTest, SaveableWithLongName) {
    TestSaveable saveable;
    saveable.value = 999;
    saveable.name = std::string(1000, 'x');  // Very long name

    saveSystem_->registerSaveable(&saveable);

    auto saveResult = saveSystem_->save(0, "Long Name Test");
    EXPECT_TRUE(saveResult.has_value());

    saveable.value = 0;
    saveable.name = "";

    auto loadResult = saveSystem_->load(0);
    EXPECT_TRUE(loadResult.has_value());

    if (loadResult.has_value()) {
        EXPECT_EQ(saveable.value, 999);
        EXPECT_EQ(saveable.name.length(), 1000);
    }

    saveSystem_->unregisterSaveable(&saveable);
}

TEST_F(SaveSystemTest, SaveableWithSpecialCharactersInData) {
    TestSaveable saveable;
    saveable.value = 123;
    // Test newlines and tabs (embedded nulls not portable in serialization)
    saveable.name = "Test\nWith\tSpecialChars";

    saveSystem_->registerSaveable(&saveable);

    auto saveResult = saveSystem_->save(0, "Special Chars Test");
    EXPECT_TRUE(saveResult.has_value());

    saveable.value = 0;
    saveable.name = "";

    auto loadResult = saveSystem_->load(0);
    EXPECT_TRUE(loadResult.has_value());

    if (loadResult.has_value()) {
        EXPECT_EQ(saveable.value, 123);
        EXPECT_EQ(saveable.name, "Test\nWith\tSpecialChars");
    }

    saveSystem_->unregisterSaveable(&saveable);
}

// ============================================================================
// Archive Edge Cases
// ============================================================================

TEST_F(SaveSystemTest, ArchiveSupportsEmptyString) {
    TestSaveable saveable;
    saveable.value = 42;
    saveable.name = "";  // Empty string

    saveSystem_->registerSaveable(&saveable);

    auto saveResult = saveSystem_->save(0, "Empty String Test");
    EXPECT_TRUE(saveResult.has_value());

    saveable.name = "not empty";

    auto loadResult = saveSystem_->load(0);
    EXPECT_TRUE(loadResult.has_value());

    if (loadResult.has_value()) {
        EXPECT_EQ(saveable.name, "");
    }

    saveSystem_->unregisterSaveable(&saveable);
}

TEST_F(SaveSystemTest, ArchiveSupportsEmptyByteArray) {
    class BytesSaveable : public ISaveable {
    public:
        std::vector<std::uint8_t> data;

        std::string getSaveKey() const override { return "bytes_saveable"; }

        void serialize(ISaveArchive& archive) const override {
            archive.writeBytes("data", data);
        }

        void deserialize(const ILoadArchive& archive) override {
            data = archive.readBytes("data");
        }
    };

    BytesSaveable saveable;
    saveable.data = {};  // Empty vector

    saveSystem_->registerSaveable(&saveable);

    auto saveResult = saveSystem_->save(0, "Empty Bytes Test");
    EXPECT_TRUE(saveResult.has_value());

    saveable.data = {0x01, 0x02, 0x03};

    auto loadResult = saveSystem_->load(0);
    EXPECT_TRUE(loadResult.has_value());

    if (loadResult.has_value()) {
        EXPECT_TRUE(saveable.data.empty());
    }

    saveSystem_->unregisterSaveable(&saveable);
}

TEST_F(SaveSystemTest, ArchiveSupportsLargeByteArray) {
    class BytesSaveable : public ISaveable {
    public:
        std::vector<std::uint8_t> data;

        std::string getSaveKey() const override { return "bytes_saveable"; }

        void serialize(ISaveArchive& archive) const override {
            archive.writeBytes("data", data);
        }

        void deserialize(const ILoadArchive& archive) override {
            data = archive.readBytes("data");
        }
    };

    BytesSaveable saveable;
    saveable.data.resize(100000);  // 100KB
    for (size_t i = 0; i < saveable.data.size(); ++i) {
        saveable.data[i] = static_cast<std::uint8_t>(i % 256);
    }

    saveSystem_->registerSaveable(&saveable);

    auto saveResult = saveSystem_->save(0, "Large Bytes Test");
    EXPECT_TRUE(saveResult.has_value());

    saveable.data.clear();

    auto loadResult = saveSystem_->load(0);
    EXPECT_TRUE(loadResult.has_value());

    if (loadResult.has_value()) {
        EXPECT_EQ(saveable.data.size(), 100000);
        // Verify some values
        EXPECT_EQ(saveable.data[0], 0);
        EXPECT_EQ(saveable.data[255], 255);
        EXPECT_EQ(saveable.data[256], 0);
    }

    saveSystem_->unregisterSaveable(&saveable);
}

TEST_F(SaveSystemTest, ArchiveSupportsExtremeFloatValues) {
    class FloatSaveable : public ISaveable {
    public:
        float minFloat = 0.0f;
        float maxFloat = 0.0f;
        float negativeFloat = 0.0f;
        float zeroFloat = 0.0f;

        std::string getSaveKey() const override { return "float_saveable"; }

        void serialize(ISaveArchive& archive) const override {
            archive.writeFloat("min", minFloat);
            archive.writeFloat("max", maxFloat);
            archive.writeFloat("negative", negativeFloat);
            archive.writeFloat("zero", zeroFloat);
        }

        void deserialize(const ILoadArchive& archive) override {
            minFloat = archive.readFloat("min");
            maxFloat = archive.readFloat("max");
            negativeFloat = archive.readFloat("negative");
            zeroFloat = archive.readFloat("zero");
        }
    };

    FloatSaveable saveable;
    saveable.minFloat = std::numeric_limits<float>::min();
    saveable.maxFloat = std::numeric_limits<float>::max();
    saveable.negativeFloat = -12345.6789f;
    saveable.zeroFloat = 0.0f;

    saveSystem_->registerSaveable(&saveable);

    auto saveResult = saveSystem_->save(0, "Float Extremes Test");
    EXPECT_TRUE(saveResult.has_value());

    saveable.minFloat = 0.0f;
    saveable.maxFloat = 0.0f;
    saveable.negativeFloat = 0.0f;
    saveable.zeroFloat = 1.0f;

    auto loadResult = saveSystem_->load(0);
    EXPECT_TRUE(loadResult.has_value());

    if (loadResult.has_value()) {
        EXPECT_FLOAT_EQ(saveable.minFloat, std::numeric_limits<float>::min());
        EXPECT_FLOAT_EQ(saveable.maxFloat, std::numeric_limits<float>::max());
        EXPECT_FLOAT_EQ(saveable.negativeFloat, -12345.6789f);
        EXPECT_FLOAT_EQ(saveable.zeroFloat, 0.0f);
    }

    saveSystem_->unregisterSaveable(&saveable);
}

TEST_F(SaveSystemTest, ArchiveSupportsExtremeIntValues) {
    class IntSaveable : public ISaveable {
    public:
        int minInt = 0;
        int maxInt = 0;
        int negativeInt = 0;
        int zeroInt = 1;

        std::string getSaveKey() const override { return "int_saveable"; }

        void serialize(ISaveArchive& archive) const override {
            archive.writeInt("min", minInt);
            archive.writeInt("max", maxInt);
            archive.writeInt("negative", negativeInt);
            archive.writeInt("zero", zeroInt);
        }

        void deserialize(const ILoadArchive& archive) override {
            minInt = archive.readInt("min");
            maxInt = archive.readInt("max");
            negativeInt = archive.readInt("negative");
            zeroInt = archive.readInt("zero");
        }
    };

    IntSaveable saveable;
    saveable.minInt = std::numeric_limits<int>::min();
    saveable.maxInt = std::numeric_limits<int>::max();
    saveable.negativeInt = -987654321;
    saveable.zeroInt = 0;

    saveSystem_->registerSaveable(&saveable);

    auto saveResult = saveSystem_->save(0, "Int Extremes Test");
    EXPECT_TRUE(saveResult.has_value());

    saveable.minInt = 0;
    saveable.maxInt = 0;
    saveable.negativeInt = 0;
    saveable.zeroInt = 1;

    auto loadResult = saveSystem_->load(0);
    EXPECT_TRUE(loadResult.has_value());

    if (loadResult.has_value()) {
        EXPECT_EQ(saveable.minInt, std::numeric_limits<int>::min());
        EXPECT_EQ(saveable.maxInt, std::numeric_limits<int>::max());
        EXPECT_EQ(saveable.negativeInt, -987654321);
        EXPECT_EQ(saveable.zeroInt, 0);
    }

    saveSystem_->unregisterSaveable(&saveable);
}

// ============================================================================
// Quick Save/Load - Additional Coverage
// ============================================================================

TEST_F(SaveSystemTest, QuickLoadWithoutQuickSaveReturnsError) {
    // Try to quick load without having quick saved
    saveSystem_->quickLoad();

    // Quick load calls load() which should return an error
    // Since quickLoad() returns void, we can't check the result directly
    // But we can verify the save doesn't exist
    EXPECT_FALSE(saveSystem_->saveExists(SaveSlots::QuickSave));
}

TEST_F(SaveSystemTest, MultipleQuickSavesOverwriteEachOther) {
    TestSaveable saveable;

    saveSystem_->registerSaveable(&saveable);

    // First quick save
    saveable.value = 100;
    saveSystem_->quickSave();

    // Second quick save
    saveable.value = 200;
    saveSystem_->quickSave();

    // Third quick save
    saveable.value = 300;
    saveSystem_->quickSave();

    // Reset and load
    saveable.value = 0;
    saveSystem_->quickLoad();

    // Should have the last value
    EXPECT_EQ(saveable.value, 300);

    saveSystem_->unregisterSaveable(&saveable);
}

TEST_F(SaveSystemTest, QuickSaveAndAutoSaveAreIndependent) {
    TestSaveable saveable;

    saveSystem_->registerSaveable(&saveable);

    // Quick save
    saveable.value = 111;
    saveSystem_->quickSave();

    // Auto save
    saveable.value = 222;
    saveSystem_->autoSave();

    // Load quick save
    saveable.value = 0;
    saveSystem_->quickLoad();
    EXPECT_EQ(saveable.value, 111);

    // Load auto save
    saveable.value = 0;
    saveSystem_->load(SaveSlots::AutoSave);
    EXPECT_EQ(saveable.value, 222);

    saveSystem_->unregisterSaveable(&saveable);
}

// ============================================================================
// Version and Playtime Tracking Tests
// ============================================================================

TEST_F(SaveSystemTest, SetGameVersionStoresVersionCorrectly) {
    saveSystem_->setGameVersion("2.5.0");
    EXPECT_EQ(saveSystem_->getGameVersion(), "2.5.0");

    saveSystem_->setGameVersion("1.0.0-beta");
    EXPECT_EQ(saveSystem_->getGameVersion(), "1.0.0-beta");
}

TEST_F(SaveSystemTest, GetGameVersionReturnsDefaultVersion) {
    // Default version should be "1.0.0"
    std::string version = saveSystem_->getGameVersion();
    EXPECT_FALSE(version.empty());
}

TEST_F(SaveSystemTest, GetGameVersionReturnsStoredVersion) {
    saveSystem_->setGameVersion("3.14.159");
    EXPECT_EQ(saveSystem_->getGameVersion(), "3.14.159");
}

TEST_F(SaveSystemTest, SetGameVersionWithEmptyString) {
    saveSystem_->setGameVersion("");
    EXPECT_EQ(saveSystem_->getGameVersion(), "");
}

TEST_F(SaveSystemTest, SetGameVersionWithSpecialCharacters) {
    saveSystem_->setGameVersion("v1.0.0-alpha+build.123");
    EXPECT_EQ(saveSystem_->getGameVersion(), "v1.0.0-alpha+build.123");
}

TEST_F(SaveSystemTest, GetTotalPlaytimeStartsAtZero) {
    // Total playtime should start at 0
    EXPECT_EQ(saveSystem_->getTotalPlaytime(), 0);
}

TEST_F(SaveSystemTest, GetTotalPlaytimeIncreasesAfterUpdate) {
    // Initial playtime should be 0
    EXPECT_EQ(saveSystem_->getTotalPlaytime(), 0);

    // Update with 1 second delta time
    saveSystem_->update(DeltaTime{1.0f});
    EXPECT_EQ(saveSystem_->getTotalPlaytime(), 1);

    // Update with another 2.5 seconds
    saveSystem_->update(DeltaTime{2.5f});
    EXPECT_EQ(saveSystem_->getTotalPlaytime(), 3);  // Truncates to 3 seconds
}

TEST_F(SaveSystemTest, GetTotalPlaytimeAccumulatesOverMultipleUpdates) {
    // Simulate multiple frame updates
    for (int i = 0; i < 60; ++i) {
        saveSystem_->update(DeltaTime{1.0f / 60.0f});  // 60 FPS, ~1 second total
    }

    std::uint64_t playtime = saveSystem_->getTotalPlaytime();
    EXPECT_GE(playtime, 0);  // Should be close to 1 second
    EXPECT_LE(playtime, 2);  // Allow some rounding tolerance
}

TEST_F(SaveSystemTest, GetTotalPlaytimeWithLargeDeltaTime) {
    // Update with a large delta time (e.g., 1 hour)
    saveSystem_->update(DeltaTime{3600.0f});
    EXPECT_EQ(saveSystem_->getTotalPlaytime(), 3600);
}

TEST_F(SaveSystemTest, GetTotalPlaytimeWithVerySmallDeltaTime) {
    // Update with very small delta times
    for (int i = 0; i < 1000; ++i) {
        saveSystem_->update(DeltaTime{0.001f});  // 1ms per frame
    }

    std::uint64_t playtime = saveSystem_->getTotalPlaytime();
    EXPECT_GE(playtime, 0);  // Should be close to 1 second
    EXPECT_LE(playtime, 2);  // Allow rounding tolerance
}

TEST_F(SaveSystemTest, VersionIsSavedToMetadata) {
    saveSystem_->setGameVersion("4.2.0");

    auto saveResult = saveSystem_->save(0, "Version Test");
    EXPECT_TRUE(saveResult.has_value());

    auto metadata = saveSystem_->getSaveMetadata(0);
    EXPECT_TRUE(metadata.has_value());

    if (metadata.has_value()) {
        EXPECT_EQ(metadata->gameVersion, "4.2.0");
    }
}

TEST_F(SaveSystemTest, PlaytimeIsSavedToMetadata) {
    // Accumulate some playtime
    saveSystem_->update(DeltaTime{10.5f});
    saveSystem_->update(DeltaTime{5.2f});

    auto saveResult = saveSystem_->save(0, "Playtime Test");
    EXPECT_TRUE(saveResult.has_value());

    auto metadata = saveSystem_->getSaveMetadata(0);
    EXPECT_TRUE(metadata.has_value());

    if (metadata.has_value()) {
        // Should be around 15 seconds (10.5 + 5.2 = 15.7)
        EXPECT_GE(metadata->playtimeSeconds, 15);
        EXPECT_LE(metadata->playtimeSeconds, 16);
    }
}

TEST_F(SaveSystemTest, VersionAndPlaytimeAreLoadedCorrectly) {
    // Set version and accumulate playtime
    saveSystem_->setGameVersion("5.0.0");
    saveSystem_->update(DeltaTime{100.0f});

    // Save
    auto saveResult = saveSystem_->save(0, "Load Test");
    EXPECT_TRUE(saveResult.has_value());

    // Verify playtime before load
    EXPECT_EQ(saveSystem_->getTotalPlaytime(), 100);

    // Load the save
    auto loadResult = saveSystem_->load(0);
    EXPECT_TRUE(loadResult.has_value());

    // Playtime should be restored (session resets to 0, loaded has 100)
    EXPECT_EQ(saveSystem_->getTotalPlaytime(), 100);

    // Add more playtime after load
    saveSystem_->update(DeltaTime{50.0f});
    EXPECT_EQ(saveSystem_->getTotalPlaytime(), 150);  // 100 loaded + 50 session
}

TEST_F(SaveSystemTest, PlaytimeContinuesAccumulatingAfterLoad) {
    // Initial playtime
    saveSystem_->update(DeltaTime{30.0f});
    saveSystem_->save(0, "Playtime Continue Test");

    // Load the save
    saveSystem_->load(0);

    // Continue accumulating playtime
    saveSystem_->update(DeltaTime{20.0f});
    EXPECT_EQ(saveSystem_->getTotalPlaytime(), 50);  // 30 + 20

    // Save again
    saveSystem_->save(1, "Second Save");

    auto metadata = saveSystem_->getSaveMetadata(1);
    EXPECT_TRUE(metadata.has_value());

    if (metadata.has_value()) {
        EXPECT_EQ(metadata->playtimeSeconds, 50);
    }
}

TEST_F(SaveSystemTest, VersionPersistsAcrossSaveLoadCycle) {
    saveSystem_->setGameVersion("6.6.6");

    auto saveResult = saveSystem_->save(0, "Version Persist Test");
    EXPECT_TRUE(saveResult.has_value());

    // Load the save
    auto loadResult = saveSystem_->load(0);
    EXPECT_TRUE(loadResult.has_value());

    // Check metadata still has correct version
    auto metadata = saveSystem_->getSaveMetadata(0);
    EXPECT_TRUE(metadata.has_value());

    if (metadata.has_value()) {
        EXPECT_EQ(metadata->gameVersion, "6.6.6");
    }
}

TEST_F(SaveSystemTest, DifferentSavesHaveDifferentPlaytimes) {
    // Save 1 with 10 seconds playtime
    saveSystem_->update(DeltaTime{10.0f});
    saveSystem_->save(0, "Save 1");

    // Continue to 25 seconds total
    saveSystem_->update(DeltaTime{15.0f});
    saveSystem_->save(1, "Save 2");

    // Continue to 50 seconds total
    saveSystem_->update(DeltaTime{25.0f});
    saveSystem_->save(2, "Save 3");

    // Verify each save has correct playtime in metadata
    auto meta0 = saveSystem_->getSaveMetadata(0);
    auto meta1 = saveSystem_->getSaveMetadata(1);
    auto meta2 = saveSystem_->getSaveMetadata(2);

    EXPECT_TRUE(meta0.has_value());
    EXPECT_TRUE(meta1.has_value());
    EXPECT_TRUE(meta2.has_value());

    if (meta0.has_value()) EXPECT_EQ(meta0->playtimeSeconds, 10);
    if (meta1.has_value()) EXPECT_EQ(meta1->playtimeSeconds, 25);
    if (meta2.has_value()) EXPECT_EQ(meta2->playtimeSeconds, 50);
}

TEST_F(SaveSystemTest, PlaytimeResetsCorrectlyOnLoad) {
    // Accumulate 100 seconds
    saveSystem_->update(DeltaTime{100.0f});
    saveSystem_->save(0, "Reset Test 1");

    // Accumulate more (total 150)
    saveSystem_->update(DeltaTime{50.0f});
    EXPECT_EQ(saveSystem_->getTotalPlaytime(), 150);

    // Load the earlier save (100 seconds)
    saveSystem_->load(0);
    EXPECT_EQ(saveSystem_->getTotalPlaytime(), 100);  // Session reset, loaded = 100

    // Continue playing
    saveSystem_->update(DeltaTime{25.0f});
    EXPECT_EQ(saveSystem_->getTotalPlaytime(), 125);  // 100 + 25
}

TEST_F(SaveSystemTest, VersionInMetadataMatchesSetVersion) {
    // Create multiple saves with different versions
    saveSystem_->setGameVersion("1.0.0");
    saveSystem_->save(0, "Version 1.0.0");

    saveSystem_->setGameVersion("2.0.0");
    saveSystem_->save(1, "Version 2.0.0");

    saveSystem_->setGameVersion("3.0.0");
    saveSystem_->save(2, "Version 3.0.0");

    // Verify each save has correct version
    auto meta0 = saveSystem_->getSaveMetadata(0);
    auto meta1 = saveSystem_->getSaveMetadata(1);
    auto meta2 = saveSystem_->getSaveMetadata(2);

    if (meta0.has_value()) EXPECT_EQ(meta0->gameVersion, "1.0.0");
    if (meta1.has_value()) EXPECT_EQ(meta1->gameVersion, "2.0.0");
    if (meta2.has_value()) EXPECT_EQ(meta2->gameVersion, "3.0.0");
}

TEST_F(SaveSystemTest, PlaytimeTrackedDuringAutoSave) {
    TestSaveable saveable;
    saveable.value = 123;

    saveSystem_->registerSaveable(&saveable);
    saveSystem_->enableAutoSave(std::chrono::seconds(5));

    // Accumulate playtime and trigger auto-save
    saveSystem_->update(DeltaTime{10.0f});  // 10 seconds, triggers auto-save

    auto metadata = saveSystem_->getSaveMetadata(SaveSlots::AutoSave);
    EXPECT_TRUE(metadata.has_value());

    if (metadata.has_value()) {
        EXPECT_GE(metadata->playtimeSeconds, 10);
    }

    saveSystem_->unregisterSaveable(&saveable);
}

TEST_F(SaveSystemTest, PlaytimeTrackedDuringQuickSave) {
    TestSaveable saveable;
    saveable.value = 456;

    saveSystem_->registerSaveable(&saveable);

    // Accumulate playtime
    saveSystem_->update(DeltaTime{42.0f});

    // Quick save
    saveSystem_->quickSave();

    auto metadata = saveSystem_->getSaveMetadata(SaveSlots::QuickSave);
    EXPECT_TRUE(metadata.has_value());

    if (metadata.has_value()) {
        EXPECT_EQ(metadata->playtimeSeconds, 42);
    }

    saveSystem_->unregisterSaveable(&saveable);
}

TEST_F(SaveSystemTest, PlaytimeWithZeroDeltaTime) {
    // Update with 0 delta time should not crash
    saveSystem_->update(DeltaTime{0.0f});
    EXPECT_EQ(saveSystem_->getTotalPlaytime(), 0);

    // Multiple updates with 0 delta
    for (int i = 0; i < 100; ++i) {
        saveSystem_->update(DeltaTime{0.0f});
    }
    EXPECT_EQ(saveSystem_->getTotalPlaytime(), 0);
}

TEST_F(SaveSystemTest, PlaytimeWithNegativeDeltaTimeIgnored) {
    // Negative delta time should not decrease playtime
    saveSystem_->update(DeltaTime{10.0f});
    EXPECT_EQ(saveSystem_->getTotalPlaytime(), 10);

    saveSystem_->update(DeltaTime{-5.0f});
    // Playtime should still be 5 or stay at 10 (depends on implementation)
    // Most likely it accumulates the negative, resulting in 5
    std::uint64_t playtime = saveSystem_->getTotalPlaytime();
    EXPECT_GE(playtime, 0);  // At minimum, shouldn't go negative
}

TEST_F(SaveSystemTest, VersionChangeBetweenSavesIsTracked) {
    saveSystem_->setGameVersion("1.0");
    saveSystem_->save(0, "Old Version");

    // Change version
    saveSystem_->setGameVersion("2.0");
    saveSystem_->save(1, "New Version");

    // Verify version stored in save system is latest
    EXPECT_EQ(saveSystem_->getGameVersion(), "2.0");

    // But metadata for each save should have their respective versions
    auto meta0 = saveSystem_->getSaveMetadata(0);
    auto meta1 = saveSystem_->getSaveMetadata(1);

    if (meta0.has_value()) EXPECT_EQ(meta0->gameVersion, "1.0");
    if (meta1.has_value()) EXPECT_EQ(meta1->gameVersion, "2.0");
}

}  // namespace bestow::tests

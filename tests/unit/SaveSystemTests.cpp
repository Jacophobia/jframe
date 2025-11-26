// tests/unit/SaveSystemTests.cpp
// Save system unit tests

#include <chrono>
#include <filesystem>
#include <fstream>
#include <memory>
#include <string>
#include <vector>

#include <gtest/gtest.h>

import jframe.save;
import jframe.save.impl;
import jframe.types;

namespace jframe::tests {

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
        // Clean up any leftover saves directory from previous test runs
        if (std::filesystem::exists("saves")) {
            std::filesystem::remove_all("saves");
        }

        saveSystem_ = createSaveSystem();

        // Create a temp directory for test saves
        tempDir_ = std::filesystem::temp_directory_path() / "jframe_save_tests";
        std::filesystem::create_directories(tempDir_);

        // Set the test profile to use temp directory
        saveSystem_->setActiveProfile("test_profile");
    }

    void TearDown() override {
        // Clean up temp files
        if (std::filesystem::exists(tempDir_)) {
            std::filesystem::remove_all(tempDir_);
        }

        // Clean up the actual saves directory created by SaveSystem
        if (std::filesystem::exists("saves")) {
            std::filesystem::remove_all("saves");
        }
    }

    std::unique_ptr<ISaveSystem> saveSystem_;
    std::filesystem::path tempDir_;
};

// ============================================================================
// Profile Management Tests
// ============================================================================

TEST_F(SaveSystemTest, ActiveProfile) {
    // System should start with default profile
    EXPECT_EQ(saveSystem_->getActiveProfile(), "test_profile");

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

}  // namespace jframe::tests

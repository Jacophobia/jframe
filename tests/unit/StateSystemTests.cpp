// tests/unit/StateSystemTests.cpp
// State system unit tests — SQLite-backed key-value persistence

#include <gtest/gtest.h>

import std;
import bestow.state;
import bestow.state.impl;
import bestow.types;

namespace bestow::tests {

class StateSystemTest : public ::testing::Test {
protected:
    void SetUp() override {
        const ::testing::TestInfo* testInfo =
            ::testing::UnitTest::GetInstance()->current_test_info();
        std::string testName = std::string(testInfo->test_suite_name()) + "_" +
                               std::string(testInfo->name());
        std::replace(testName.begin(), testName.end(), '/', '_');
        std::replace(testName.begin(), testName.end(), '\\', '_');
        std::replace(testName.begin(), testName.end(), ' ', '_');

        uniqueProfile_ = testName;

        std::error_code ec;
        auto profilePath = std::filesystem::path("saves") / uniqueProfile_;
        if (std::filesystem::exists(profilePath, ec)) {
            std::filesystem::remove_all(profilePath, ec);
        }

        state_ = std::make_unique<StateSystem>();
        state_->setActiveProfile(uniqueProfile_);
    }

    void TearDown() override {
        state_.reset();

        std::error_code ec;
        auto profilePath = std::filesystem::path("saves") / uniqueProfile_;
        if (std::filesystem::exists(profilePath, ec)) {
            std::filesystem::remove_all(profilePath, ec);
        }
    }

    /// Helper: commit synchronously by calling commit + pumping update until
    /// the callback fires. Returns true if the commit succeeded.
    bool commitSync(StateSlot slot, const std::string& name = "") {
        bool done = false;
        bool success = false;
        state_->commit(slot, name, [&](bool ok, StateError) {
            success = ok;
            done = true;
        });
        // Pump update to process the worker completion
        for (int i = 0; i < 200 && !done; ++i) {
            state_->update(0.01f);
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        return success;
    }

    /// Helper: restore synchronously
    bool restoreSync(StateSlot slot) {
        bool done = false;
        bool success = false;
        state_->restore(slot, [&](bool ok, StateError) {
            success = ok;
            done = true;
        });
        for (int i = 0; i < 200 && !done; ++i) {
            state_->update(0.01f);
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        return success;
    }

    std::unique_ptr<StateSystem> state_;
    std::string uniqueProfile_;
};

// ============================================================================
// Key-Value Data Operations
// ============================================================================

TEST_F(StateSystemTest, SetAndGetNumber) {
    state_->setNumber("score", 42.0);
    EXPECT_DOUBLE_EQ(state_->getNumber("score", 0.0), 42.0);
}

TEST_F(StateSystemTest, SetAndGetString) {
    state_->setString("name", "Hero");
    EXPECT_EQ(state_->getString("name", ""), "Hero");
}

TEST_F(StateSystemTest, SetAndGetBool) {
    state_->setBool("alive", true);
    EXPECT_TRUE(state_->getBool("alive", false));

    state_->setBool("dead", false);
    EXPECT_FALSE(state_->getBool("dead", true));
}

TEST_F(StateSystemTest, SetAndGetJson) {
    state_->setJsonData("inventory", R"(["sword","shield"])");
    EXPECT_EQ(state_->getJsonData("inventory"), R"(["sword","shield"])");
}

TEST_F(StateSystemTest, GetReturnsDefaultWhenKeyMissing) {
    EXPECT_DOUBLE_EQ(state_->getNumber("missing", 99.0), 99.0);
    EXPECT_EQ(state_->getString("missing", "default"), "default");
    EXPECT_TRUE(state_->getBool("missing", true));
    EXPECT_EQ(state_->getJsonData("missing"), "{}");
}

TEST_F(StateSystemTest, GetReturnsDefaultWhenTypeMismatched) {
    state_->setNumber("score", 42.0);
    // Ask for string on a number key — should return default
    EXPECT_EQ(state_->getString("score", "fallback"), "fallback");
    EXPECT_TRUE(state_->getBool("score", true));
}

TEST_F(StateSystemTest, HasData) {
    EXPECT_FALSE(state_->hasData("key"));
    state_->setNumber("key", 1.0);
    EXPECT_TRUE(state_->hasData("key"));
}

TEST_F(StateSystemTest, RemoveData) {
    state_->setNumber("key", 1.0);
    EXPECT_TRUE(state_->hasData("key"));
    state_->removeData("key");
    EXPECT_FALSE(state_->hasData("key"));
}

TEST_F(StateSystemTest, ClearData) {
    state_->setNumber("a", 1.0);
    state_->setString("b", "hello");
    state_->setBool("c", true);
    state_->clearData();
    EXPECT_FALSE(state_->hasData("a"));
    EXPECT_FALSE(state_->hasData("b"));
    EXPECT_FALSE(state_->hasData("c"));
}

TEST_F(StateSystemTest, OverwriteValueChangesType) {
    state_->setNumber("key", 42.0);
    EXPECT_DOUBLE_EQ(state_->getNumber("key", 0.0), 42.0);

    state_->setString("key", "now a string");
    EXPECT_DOUBLE_EQ(state_->getNumber("key", -1.0), -1.0); // type changed
    EXPECT_EQ(state_->getString("key", ""), "now a string");
}

// ============================================================================
// Commit/Restore (Async)
// ============================================================================

TEST_F(StateSystemTest, CommitAndRestoreRoundTrip) {
    state_->setNumber("health", 75.0);
    state_->setString("name", "TestHero");
    state_->setBool("alive", true);

    bool committed = commitSync(0, "Test Save");
    EXPECT_TRUE(committed);

    // Clear cache
    state_->clearData();
    EXPECT_FALSE(state_->hasData("health"));

    // Restore
    bool restored = restoreSync(0);
    EXPECT_TRUE(restored);

    EXPECT_DOUBLE_EQ(state_->getNumber("health", 0.0), 75.0);
    EXPECT_EQ(state_->getString("name", ""), "TestHero");
    EXPECT_TRUE(state_->getBool("alive", false));
}

TEST_F(StateSystemTest, CommitOverwritesPreviousSlot) {
    state_->setNumber("value", 100.0);
    EXPECT_TRUE(commitSync(0, "First"));

    state_->setNumber("value", 200.0);
    EXPECT_TRUE(commitSync(0, "Second"));

    state_->clearData();
    EXPECT_TRUE(restoreSync(0));

    EXPECT_DOUBLE_EQ(state_->getNumber("value", 0.0), 200.0);
}

TEST_F(StateSystemTest, RestoreNonExistentSlotFails) {
    bool restored = restoreSync(999);
    EXPECT_FALSE(restored);
}

TEST_F(StateSystemTest, CommitMultipleSlots) {
    state_->setNumber("slot0_val", 10.0);
    EXPECT_TRUE(commitSync(0, "Slot 0"));

    state_->setNumber("slot0_val", 999.0); // change cache
    state_->setNumber("slot1_val", 20.0);
    EXPECT_TRUE(commitSync(1, "Slot 1"));

    // Restore slot 0
    EXPECT_TRUE(restoreSync(0));
    EXPECT_DOUBLE_EQ(state_->getNumber("slot0_val", 0.0), 10.0);
    EXPECT_FALSE(state_->hasData("slot1_val")); // slot 0 didn't have this

    // Restore slot 1
    EXPECT_TRUE(restoreSync(1));
    EXPECT_DOUBLE_EQ(state_->getNumber("slot0_val", 0.0), 999.0);
    EXPECT_DOUBLE_EQ(state_->getNumber("slot1_val", 0.0), 20.0);
}

TEST_F(StateSystemTest, CommitWithJsonData) {
    state_->setJsonData("inventory", R"({"items":["sword","shield"],"gold":100})");
    EXPECT_TRUE(commitSync(0));

    state_->clearData();
    EXPECT_TRUE(restoreSync(0));

    std::string json = state_->getJsonData("inventory");
    EXPECT_NE(json, "{}");
    // Verify it contains expected data
    EXPECT_NE(json.find("sword"), std::string::npos);
    EXPECT_NE(json.find("shield"), std::string::npos);
}

// ============================================================================
// Quick Commit/Restore
// ============================================================================

TEST_F(StateSystemTest, QuickCommitAndRestore) {
    state_->setNumber("quick_val", 42.0);

    bool done = false;
    bool success = false;
    state_->quickCommit([&](bool ok, StateError) {
        success = ok;
        done = true;
    });
    for (int i = 0; i < 200 && !done; ++i) {
        state_->update(0.01f);
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    EXPECT_TRUE(success);

    state_->clearData();

    done = false;
    success = false;
    state_->quickRestore([&](bool ok, StateError) {
        success = ok;
        done = true;
    });
    for (int i = 0; i < 200 && !done; ++i) {
        state_->update(0.01f);
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    EXPECT_TRUE(success);
    EXPECT_DOUBLE_EQ(state_->getNumber("quick_val", 0.0), 42.0);
}

// ============================================================================
// Delete Slot
// ============================================================================

TEST_F(StateSystemTest, DeleteSlotRemovesDataAndMetadata) {
    state_->setNumber("val", 1.0);
    EXPECT_TRUE(commitSync(0, "To Delete"));

    EXPECT_TRUE(state_->slotExists(0));
    EXPECT_TRUE(state_->deleteSlot(0));
    EXPECT_FALSE(state_->slotExists(0));
    EXPECT_FALSE(state_->getSlotMetadata(0).has_value());
}

TEST_F(StateSystemTest, DeleteNonExistentSlotReturnsFalse) {
    EXPECT_FALSE(state_->deleteSlot(999));
}

// ============================================================================
// Metadata Queries
// ============================================================================

TEST_F(StateSystemTest, SlotMetadataContainsName) {
    state_->setNumber("x", 1.0);
    EXPECT_TRUE(commitSync(0, "My Save"));

    auto meta = state_->getSlotMetadata(0);
    ASSERT_TRUE(meta.has_value());
    EXPECT_EQ(meta->name, "My Save");
    EXPECT_EQ(meta->slot, 0u);
}

TEST_F(StateSystemTest, SlotMetadataContainsTimestamp) {
    state_->setNumber("x", 1.0);
    EXPECT_TRUE(commitSync(0, "Timestamp Test"));

    auto meta = state_->getSlotMetadata(0);
    ASSERT_TRUE(meta.has_value());

    auto now = std::chrono::system_clock::now();
    auto diff = std::chrono::duration_cast<std::chrono::seconds>(now - meta->timestamp);
    EXPECT_LT(diff.count(), 10); // within 10 seconds
}

TEST_F(StateSystemTest, SlotMetadataContainsGameVersion) {
    state_->setGameVersion("2.5.0");
    state_->setNumber("x", 1.0);
    EXPECT_TRUE(commitSync(0));

    auto meta = state_->getSlotMetadata(0);
    ASSERT_TRUE(meta.has_value());
    EXPECT_EQ(meta->gameVersion, "2.5.0");
}

TEST_F(StateSystemTest, SlotMetadataContainsPlaytime) {
    state_->update(DeltaTime{10.0f});
    state_->setNumber("x", 1.0);
    EXPECT_TRUE(commitSync(0));

    auto meta = state_->getSlotMetadata(0);
    ASSERT_TRUE(meta.has_value());
    // Playtime includes both loaded + session
    EXPECT_GE(meta->playtimeSeconds, 10u);
}

TEST_F(StateSystemTest, SlotMetadataContainsCompletionAndLevel) {
    state_->setCompletionPercentage(45.5f);
    state_->setCurrentLevel("dungeon_3");
    state_->setNumber("x", 1.0);
    EXPECT_TRUE(commitSync(0));

    auto meta = state_->getSlotMetadata(0);
    ASSERT_TRUE(meta.has_value());
    EXPECT_FLOAT_EQ(meta->completionPercentage, 45.5f);
    ASSERT_TRUE(meta->levelName.has_value());
    EXPECT_EQ(*meta->levelName, "dungeon_3");
}

TEST_F(StateSystemTest, GetAllSlotMetadata) {
    state_->setNumber("x", 1.0);
    EXPECT_TRUE(commitSync(0, "Save A"));
    state_->setNumber("x", 2.0);
    EXPECT_TRUE(commitSync(1, "Save B"));

    auto all = state_->getAllSlotMetadata();
    EXPECT_EQ(all.size(), 2u);
}

TEST_F(StateSystemTest, GetSlotMetadataReturnsNulloptForEmpty) {
    EXPECT_FALSE(state_->getSlotMetadata(99).has_value());
}

TEST_F(StateSystemTest, SlotExistsReturnsFalseForEmpty) {
    EXPECT_FALSE(state_->slotExists(99));
}

// ============================================================================
// Profile Management
// ============================================================================

TEST_F(StateSystemTest, ActiveProfileDefault) {
    auto fresh = std::make_unique<StateSystem>();
    EXPECT_EQ(fresh->getActiveProfile(), "default");
}

TEST_F(StateSystemTest, SetAndGetProfile) {
    state_->setActiveProfile("player2");
    EXPECT_EQ(state_->getActiveProfile(), "player2");
}

TEST_F(StateSystemTest, ProfilesAreIsolated) {
    std::string profileA = uniqueProfile_ + "_a";
    std::string profileB = uniqueProfile_ + "_b";

    state_->setActiveProfile(profileA);
    state_->setNumber("val", 111.0);
    EXPECT_TRUE(commitSync(0, "Profile A"));

    state_->setActiveProfile(profileB);
    EXPECT_FALSE(state_->slotExists(0)); // not visible in profile B

    state_->setNumber("val", 222.0);
    EXPECT_TRUE(commitSync(0, "Profile B"));

    // Switch back to A
    state_->setActiveProfile(profileA);
    EXPECT_TRUE(restoreSync(0));
    EXPECT_DOUBLE_EQ(state_->getNumber("val", 0.0), 111.0);

    // Cleanup
    std::error_code ec;
    std::filesystem::remove_all(std::filesystem::path("saves") / profileA, ec);
    std::filesystem::remove_all(std::filesystem::path("saves") / profileB, ec);
}

TEST_F(StateSystemTest, GetProfilesListsDirectories) {
    // Creating profiles by switching and committing
    std::string pA = uniqueProfile_ + "_list_a";
    std::string pB = uniqueProfile_ + "_list_b";

    state_->setActiveProfile(pA);
    state_->setNumber("x", 1.0);
    EXPECT_TRUE(commitSync(0));

    state_->setActiveProfile(pB);
    state_->setNumber("x", 2.0);
    EXPECT_TRUE(commitSync(0));

    auto profiles = state_->getProfiles();
    bool foundA = std::find(profiles.begin(), profiles.end(), pA) != profiles.end();
    bool foundB = std::find(profiles.begin(), profiles.end(), pB) != profiles.end();
    EXPECT_TRUE(foundA);
    EXPECT_TRUE(foundB);

    std::error_code ec;
    std::filesystem::remove_all(std::filesystem::path("saves") / pA, ec);
    std::filesystem::remove_all(std::filesystem::path("saves") / pB, ec);
}

// ============================================================================
// Game Version & Playtime Tracking
// ============================================================================

TEST_F(StateSystemTest, GameVersionDefault) {
    EXPECT_EQ(state_->getGameVersion(), "1.0.0");
}

TEST_F(StateSystemTest, SetGameVersion) {
    state_->setGameVersion("3.14.0");
    EXPECT_EQ(state_->getGameVersion(), "3.14.0");
}

TEST_F(StateSystemTest, PlaytimeStartsAtZero) {
    EXPECT_EQ(state_->getTotalPlaytime(), 0u);
    EXPECT_EQ(state_->getSessionPlaytime(), 0u);
}

TEST_F(StateSystemTest, PlaytimeIncreasesWithUpdate) {
    state_->update(DeltaTime{10.0f});
    EXPECT_EQ(state_->getTotalPlaytime(), 10u);
    EXPECT_EQ(state_->getSessionPlaytime(), 10u);
}

TEST_F(StateSystemTest, PlaytimeAccumulatesAcrossUpdates) {
    state_->update(DeltaTime{5.0f});
    state_->update(DeltaTime{3.0f});
    state_->update(DeltaTime{2.0f});
    EXPECT_EQ(state_->getTotalPlaytime(), 10u);
}

TEST_F(StateSystemTest, ResetSessionPlaytime) {
    state_->update(DeltaTime{100.0f});
    EXPECT_EQ(state_->getSessionPlaytime(), 100u);

    state_->resetSessionPlaytime();
    EXPECT_EQ(state_->getSessionPlaytime(), 0u);
    EXPECT_EQ(state_->getTotalPlaytime(), 0u);
}

TEST_F(StateSystemTest, PlaytimeRestoredFromSave) {
    state_->update(DeltaTime{50.0f});
    state_->setNumber("x", 1.0);
    EXPECT_TRUE(commitSync(0));

    // Clear playtime
    state_->resetSessionPlaytime();
    EXPECT_EQ(state_->getTotalPlaytime(), 0u);

    // Restore should bring back loaded playtime
    EXPECT_TRUE(restoreSync(0));
    // After restore: loaded=50, session=0 → total=50
    EXPECT_EQ(state_->getTotalPlaytime(), 50u);

    // Continue playing
    state_->update(DeltaTime{25.0f});
    EXPECT_EQ(state_->getTotalPlaytime(), 75u);
}

TEST_F(StateSystemTest, CompletionPercentageClamped) {
    state_->setCompletionPercentage(150.0f);
    state_->setNumber("x", 1.0);
    EXPECT_TRUE(commitSync(0));

    auto meta = state_->getSlotMetadata(0);
    ASSERT_TRUE(meta.has_value());
    EXPECT_FLOAT_EQ(meta->completionPercentage, 100.0f);
}

// ============================================================================
// Auto-Commit
// ============================================================================

TEST_F(StateSystemTest, AutoCommitTriggersAfterInterval) {
    state_->setNumber("auto_val", 123.0);
    state_->enableAutoCommit(std::chrono::seconds(5));

    // Update past the interval
    state_->update(DeltaTime{6.0f});

    // Give worker thread time to complete
    for (int i = 0; i < 100; ++i) {
        state_->update(0.01f);
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    // Auto-commit slot should exist
    EXPECT_TRUE(state_->slotExists(StateSlots::AutoCommit));
}

TEST_F(StateSystemTest, AutoCommitDoesNotTriggerBeforeInterval) {
    state_->setNumber("val", 1.0);
    state_->enableAutoCommit(std::chrono::seconds(10));

    state_->update(DeltaTime{5.0f}); // less than interval

    // Give a moment
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    state_->update(0.01f);

    EXPECT_FALSE(state_->slotExists(StateSlots::AutoCommit));
}

TEST_F(StateSystemTest, DisableAutoCommitStopsTriggers) {
    state_->setNumber("val", 1.0);
    state_->enableAutoCommit(std::chrono::seconds(1));
    state_->disableAutoCommit();

    state_->update(DeltaTime{5.0f});
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    state_->update(0.01f);

    EXPECT_FALSE(state_->slotExists(StateSlots::AutoCommit));
}

// ============================================================================
// IStateful Registration
// ============================================================================

TEST_F(StateSystemTest, StatefulOnCommitAndOnRestore) {
    class TestStateful : public IStateful {
    public:
        int value = 0;
        std::string getStateKey() const override { return "test_stateful"; }
        void onCommit(IStateSystem& s) const override {
            s.setNumber("_sys.test_stateful.value", static_cast<double>(value));
        }
        void onRestore(IStateSystem& s) override {
            value = static_cast<int>(s.getNumber("_sys.test_stateful.value", 0.0));
        }
    };

    TestStateful stateful;
    stateful.value = 42;

    state_->registerStateful(&stateful);

    // Commit should call onCommit
    EXPECT_TRUE(commitSync(0, "Stateful Test"));

    // Change value and restore
    stateful.value = 0;
    EXPECT_TRUE(restoreSync(0));
    EXPECT_EQ(stateful.value, 42);

    state_->unregisterStateful(&stateful);
}

// ============================================================================
// Migration
// ============================================================================

TEST_F(StateSystemTest, MigrationRunsOnRestore) {
    // Commit with format version 1
    state_->setFormatVersion(1);
    state_->setNumber("player.hp", 75.0);
    EXPECT_TRUE(commitSync(0, "V1 Save"));

    // Now upgrade to version 2 and register migration
    state_->setFormatVersion(2);
    bool migrationRan = false;
    state_->registerMigration(1, 2, [&]() {
        migrationRan = true;
        double hp = state_->getNumber("player.hp", 100.0);
        state_->setNumber("player.health", hp);
        state_->removeData("player.hp");
    });

    // Restore the v1 save
    EXPECT_TRUE(restoreSync(0));
    EXPECT_TRUE(migrationRan);
    EXPECT_DOUBLE_EQ(state_->getNumber("player.health", 0.0), 75.0);
    EXPECT_FALSE(state_->hasData("player.hp"));
}

// ============================================================================
// JSON Export/Import
// ============================================================================

TEST_F(StateSystemTest, JsonExportImportRoundTrip) {
    state_->setNumber("health", 50.0);
    state_->setString("name", "ExportTest");
    state_->setBool("flag", true);
    EXPECT_TRUE(commitSync(0, "Export Save"));

    // Export
    auto exportDir = std::filesystem::path("saves") / uniqueProfile_;
    std::filesystem::create_directories(exportDir);
    auto exportPath = exportDir / "export_test.json";

    auto exportResult = state_->exportToJson(0, exportPath.string());
    EXPECT_TRUE(exportResult.has_value());

    // Import into a different slot
    auto importResult = state_->importFromJson(5, exportPath.string());
    EXPECT_TRUE(importResult.has_value());

    // Restore from the imported slot
    EXPECT_TRUE(restoreSync(5));
    EXPECT_DOUBLE_EQ(state_->getNumber("health", 0.0), 50.0);
    EXPECT_EQ(state_->getString("name", ""), "ExportTest");
    EXPECT_TRUE(state_->getBool("flag", false));

    // Cleanup
    std::error_code ec;
    std::filesystem::remove(exportPath, ec);
}

// ============================================================================
// Schema Evolution (Forward Compatibility)
// ============================================================================

TEST_F(StateSystemTest, NewKeyGetsDefaultAfterRestore) {
    // Commit with only "health"
    state_->setNumber("health", 100.0);
    EXPECT_TRUE(commitSync(0));

    // Restore and query a key that wasn't in the save
    EXPECT_TRUE(restoreSync(0));
    EXPECT_DOUBLE_EQ(state_->getNumber("mana", 50.0), 50.0); // default returned
}

// ============================================================================
// Edge Cases
// ============================================================================

TEST_F(StateSystemTest, EmptyStringKey) {
    state_->setNumber("", 42.0);
    EXPECT_DOUBLE_EQ(state_->getNumber("", 0.0), 42.0);
    EXPECT_TRUE(state_->hasData(""));
}

TEST_F(StateSystemTest, VeryLongKey) {
    std::string longKey(1000, 'k');
    state_->setString(longKey, "value");
    EXPECT_EQ(state_->getString(longKey, ""), "value");
}

TEST_F(StateSystemTest, SpecialCharactersInValues) {
    state_->setString("special", "Hello\nWorld\t!");
    EXPECT_EQ(state_->getString("special", ""), "Hello\nWorld\t!");

    EXPECT_TRUE(commitSync(0));
    state_->clearData();
    EXPECT_TRUE(restoreSync(0));
    EXPECT_EQ(state_->getString("special", ""), "Hello\nWorld\t!");
}

TEST_F(StateSystemTest, CommitEmptyCache) {
    // Should succeed even with nothing to save
    EXPECT_TRUE(commitSync(0, "Empty"));
    EXPECT_TRUE(state_->slotExists(0));
}

TEST_F(StateSystemTest, UpdateWithZeroDeltaTime) {
    state_->update(DeltaTime{0.0f});
    EXPECT_EQ(state_->getTotalPlaytime(), 0u);
}

TEST_F(StateSystemTest, DifferentSlotsHaveDifferentPlaytimes) {
    state_->update(DeltaTime{10.0f});
    state_->setNumber("x", 1.0);
    EXPECT_TRUE(commitSync(0, "10s"));

    state_->update(DeltaTime{20.0f}); // total 30s
    EXPECT_TRUE(commitSync(1, "30s"));

    auto meta0 = state_->getSlotMetadata(0);
    auto meta1 = state_->getSlotMetadata(1);

    ASSERT_TRUE(meta0.has_value());
    ASSERT_TRUE(meta1.has_value());
    EXPECT_EQ(meta0->playtimeSeconds, 10u);
    EXPECT_EQ(meta1->playtimeSeconds, 30u);
}

TEST_F(StateSystemTest, DeleteSlotAfterCommit) {
    state_->setNumber("val", 1.0);
    EXPECT_TRUE(commitSync(0, "Delete Me"));
    EXPECT_TRUE(state_->slotExists(0));

    EXPECT_TRUE(state_->deleteSlot(0));
    EXPECT_FALSE(state_->slotExists(0));

    auto all = state_->getAllSlotMetadata();
    EXPECT_TRUE(all.empty());
}

TEST_F(StateSystemTest, SaveToHighSlotNumber) {
    state_->setNumber("val", 1.0);
    EXPECT_TRUE(commitSync(999999, "High Slot"));
    EXPECT_TRUE(state_->slotExists(999999));

    auto meta = state_->getSlotMetadata(999999);
    ASSERT_TRUE(meta.has_value());
    EXPECT_EQ(meta->slot, 999999u);
}

}  // namespace bestow::tests

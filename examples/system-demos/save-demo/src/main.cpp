// save-demo/src/main.cpp
// Comprehensive demonstration of the Bestow Save System API

#include <kangaru/kangaru.hpp>

import std;
import bestow;
import bestow.core;

using namespace bestow;

//==============================================================================
// Example Saveable: Player Data
//==============================================================================

class PlayerData : public ISaveable {
public:
    std::string name = "Hero";
    int level = 1;
    int experience = 0;
    float positionX = 0.0f;
    float positionY = 0.0f;
    int health = 100;
    int maxHealth = 100;

    std::string getSaveKey() const override {
        return "player_data";
    }

    void serialize(ISaveArchive& archive) const override {
        archive.writeString("name", name);
        archive.writeInt("level", level);
        archive.writeInt("experience", experience);
        archive.writeFloat("positionX", positionX);
        archive.writeFloat("positionY", positionY);
        archive.writeInt("health", health);
        archive.writeInt("maxHealth", maxHealth);
    }

    void deserialize(const ILoadArchive& archive) override {
        name = archive.readString("name");
        level = archive.readInt("level");
        experience = archive.readInt("experience");
        positionX = archive.readFloat("positionX");
        positionY = archive.readFloat("positionY");
        health = archive.readInt("health");
        maxHealth = archive.readInt("maxHealth");
    }

    void print() const {
        std::println("  Name: {}", name);
        std::println("  Level: {}", level);
        std::println("  Experience: {}", experience);
        std::println("  Position: ({:.2f}, {:.2f})", positionX, positionY);
        std::println("  Health: {}/{}", health, maxHealth);
    }
};

//==============================================================================
// Example Saveable: Game Progress
//==============================================================================

class GameProgress : public ISaveable {
public:
    std::vector<int> completedLevels;
    int collectiblesFound = 0;
    int totalCollectibles = 100;
    std::vector<std::string> unlockedAchievements;
    double totalPlaytimeSeconds = 0.0;

    std::string getSaveKey() const override {
        return "game_progress";
    }

    void serialize(ISaveArchive& archive) const override {
        // Serialize vector as bytes
        std::vector<std::uint8_t> levelData;
        levelData.reserve(completedLevels.size() * sizeof(int));
        for (int level : completedLevels) {
            const auto* bytes = reinterpret_cast<const std::uint8_t*>(&level);
            levelData.insert(levelData.end(), bytes, bytes + sizeof(int));
        }
        archive.writeBytes("completedLevels", levelData);

        archive.writeInt("collectiblesFound", collectiblesFound);
        archive.writeInt("totalCollectibles", totalCollectibles);

        // Serialize achievements as a concatenated string
        std::string achievementsStr;
        for (std::size_t i = 0; i < unlockedAchievements.size(); ++i) {
            if (i > 0) achievementsStr += "|";
            achievementsStr += unlockedAchievements[i];
        }
        archive.writeString("unlockedAchievements", achievementsStr);

        archive.writeDouble("totalPlaytimeSeconds", totalPlaytimeSeconds);
    }

    void deserialize(const ILoadArchive& archive) override {
        // Deserialize vector from bytes
        auto levelData = archive.readBytes("completedLevels");
        completedLevels.clear();
        for (std::size_t i = 0; i < levelData.size(); i += sizeof(int)) {
            if (i + sizeof(int) <= levelData.size()) {
                int level;
                std::memcpy(&level, &levelData[i], sizeof(int));
                completedLevels.push_back(level);
            }
        }

        collectiblesFound = archive.readInt("collectiblesFound");
        totalCollectibles = archive.readInt("totalCollectibles");

        // Deserialize achievements from concatenated string
        std::string achievementsStr = archive.readString("unlockedAchievements");
        unlockedAchievements.clear();
        if (!achievementsStr.empty()) {
            std::size_t start = 0;
            std::size_t end = achievementsStr.find('|');
            while (end != std::string::npos) {
                unlockedAchievements.push_back(achievementsStr.substr(start, end - start));
                start = end + 1;
                end = achievementsStr.find('|', start);
            }
            unlockedAchievements.push_back(achievementsStr.substr(start));
        }

        totalPlaytimeSeconds = archive.readDouble("totalPlaytimeSeconds");
    }

    void print() const {
        std::println("  Completed Levels: {}", completedLevels.size());
        for (int level : completedLevels) {
            std::print("    Level {}, ", level);
        }
        if (!completedLevels.empty()) std::println("");

        std::println("  Collectibles: {}/{} ({:.1f}%)",
            collectiblesFound, totalCollectibles,
            (collectiblesFound * 100.0f) / totalCollectibles);

        std::println("  Achievements: {}", unlockedAchievements.size());
        for (const auto& achievement : unlockedAchievements) {
            std::println("    - {}", achievement);
        }

        std::println("  Total Playtime: {:.1f} hours", totalPlaytimeSeconds / 3600.0);
    }
};

//==============================================================================
// Example Saveable: Game Settings
//==============================================================================

class GameSettings : public ISaveable {
public:
    float masterVolume = 0.8f;
    float musicVolume = 0.7f;
    float sfxVolume = 0.9f;
    int difficulty = 1; // 0=Easy, 1=Normal, 2=Hard
    bool fullscreen = false;
    std::string controlScheme = "default";

    std::string getSaveKey() const override {
        return "game_settings";
    }

    void serialize(ISaveArchive& archive) const override {
        archive.writeFloat("masterVolume", masterVolume);
        archive.writeFloat("musicVolume", musicVolume);
        archive.writeFloat("sfxVolume", sfxVolume);
        archive.writeInt("difficulty", difficulty);
        archive.writeBool("fullscreen", fullscreen);
        archive.writeString("controlScheme", controlScheme);
    }

    void deserialize(const ILoadArchive& archive) override {
        masterVolume = archive.readFloat("masterVolume");
        musicVolume = archive.readFloat("musicVolume");
        sfxVolume = archive.readFloat("sfxVolume");
        difficulty = archive.readInt("difficulty");
        fullscreen = archive.readBool("fullscreen");
        controlScheme = archive.readString("controlScheme");
    }

    void print() const {
        std::println("  Master Volume: {:.0f}%", masterVolume * 100);
        std::println("  Music Volume: {:.0f}%", musicVolume * 100);
        std::println("  SFX Volume: {:.0f}%", sfxVolume * 100);

        const char* difficultyStr[] = {"Easy", "Normal", "Hard"};
        std::println("  Difficulty: {}", difficultyStr[difficulty]);
        std::println("  Fullscreen: {}", fullscreen ? "Yes" : "No");
        std::println("  Control Scheme: {}", controlScheme);
    }
};

//==============================================================================
// Utility Functions
//==============================================================================

void printSeparator(const std::string& title) {
    std::println("\n{:=^80}", "");
    std::println("{:^80}", title);
    std::println("{:=^80}\n", "");
}

void printError(const std::string& operation, SaveError error) {
    std::println("ERROR: {} failed with error: {}", operation,
        [error]() -> std::string {
            switch (error) {
                case SaveError::Success: return "Success";
                case SaveError::FileNotFound: return "FileNotFound";
                case SaveError::CorruptedFile: return "CorruptedFile";
                case SaveError::InvalidChecksum: return "InvalidChecksum";
                case SaveError::VersionMismatch: return "VersionMismatch";
                case SaveError::MigrationFailed: return "MigrationFailed";
                case SaveError::IOError: return "IOError";
                case SaveError::SerializationError: return "SerializationError";
                default: return "Unknown";
            }
        }());
}

void printMetadata(const SaveMetadata& meta) {
    auto time = std::chrono::system_clock::to_time_t(meta.timestamp);
    std::println("  Slot: {}", meta.slot);
    std::println("  Name: {}", meta.saveName);
    std::println("  Timestamp: {}", std::ctime(&time));
    std::println("  Version: {}", meta.gameVersion);
    std::println("  Playtime: {} seconds", meta.playtimeSeconds);
    std::println("  Completion: {:.1f}%", meta.completionPercentage);
    if (meta.levelName) {
        std::println("  Level: {}", *meta.levelName);
    }
    std::println("  Screenshot: {}", meta.hasScreenshot ? "Yes" : "No");
}

//==============================================================================
// Main Demo
//==============================================================================

int main() {
    std::println("Bestow Save System Demo");
    std::println("Comprehensive demonstration of ISaveSystem API\n");

    // Create save system using Engine
    core::Engine engine;
    auto& sys = engine.systems();
    auto* saveSystem = sys.save;

    // Create saveable objects
    PlayerData player;
    GameProgress progress;
    GameSettings settings;

    //==========================================================================
    // DEMO 1: Basic Save/Load Operations
    //==========================================================================
    printSeparator("DEMO 1: Basic Save/Load Operations");

    std::println("Setting up initial game state...");
    player.name = "Alice";
    player.level = 5;
    player.experience = 1250;
    player.positionX = 100.5f;
    player.positionY = 200.75f;
    player.health = 85;
    player.maxHealth = 100;

    progress.completedLevels = {1, 2, 3, 4};
    progress.collectiblesFound = 45;
    progress.totalCollectibles = 100;
    progress.unlockedAchievements = {"First Steps", "Explorer", "Collector"};
    progress.totalPlaytimeSeconds = 7200.0; // 2 hours

    settings.masterVolume = 0.85f;
    settings.musicVolume = 0.6f;
    settings.sfxVolume = 1.0f;
    settings.difficulty = 2; // Hard
    settings.fullscreen = true;
    settings.controlScheme = "advanced";

    std::println("\nInitial State:");
    std::println("Player Data:");
    player.print();
    std::println("\nGame Progress:");
    progress.print();
    std::println("\nGame Settings:");
    settings.print();

    // Register saveables
    std::println("\n>> Registering saveable objects...");
    saveSystem->registerSaveable(&player);
    saveSystem->registerSaveable(&progress);
    saveSystem->registerSaveable(&settings);
    std::println("   Registered 3 saveable objects");

    // Save to slot 0
    std::println("\n>> Saving to slot 0...");
    auto saveResult = saveSystem->save(0, "My First Save");
    if (saveResult.has_value()) {
        std::println("   Save successful!");
    } else {
        printError("Save", saveResult.error());
    }

    // Modify the data
    std::println("\n>> Modifying game state...");
    player.name = "Modified Player";
    player.level = 99;
    player.experience = 99999;
    progress.collectiblesFound = 0;
    settings.masterVolume = 0.1f;

    std::println("\nModified State:");
    std::println("Player Data:");
    player.print();

    // Load from slot 0
    std::println("\n>> Loading from slot 0...");
    auto loadResult = saveSystem->load(0);
    if (loadResult.has_value()) {
        std::println("   Load successful!");
    } else {
        printError("Load", loadResult.error());
    }

    std::println("\nRestored State:");
    std::println("Player Data:");
    player.print();
    std::println("\nGame Progress:");
    progress.print();
    std::println("\nGame Settings:");
    settings.print();

    //==========================================================================
    // DEMO 2: Multiple Save Slots
    //==========================================================================
    printSeparator("DEMO 2: Multiple Save Slots");

    std::println("Creating saves in different slots...\n");

    // Save 1: Beginner
    player.name = "Beginner Bob";
    player.level = 1;
    player.experience = 0;
    progress.completedLevels = {};
    progress.collectiblesFound = 5;

    std::println(">> Saving to slot 1: Beginner Save");
    saveSystem->save(1, "Beginner Save");
    std::println("   Player: {}, Level {}", player.name, player.level);

    // Save 2: Mid-game
    player.name = "Midgame Mike";
    player.level = 15;
    player.experience = 5000;
    progress.completedLevels = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    progress.collectiblesFound = 55;

    std::println("\n>> Saving to slot 2: Mid-game Save");
    saveSystem->save(2, "Mid-game Save");
    std::println("   Player: {}, Level {}", player.name, player.level);

    // Save 3: End-game
    player.name = "Endgame Emma";
    player.level = 50;
    player.experience = 100000;
    progress.completedLevels = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15};
    progress.collectiblesFound = 98;
    progress.unlockedAchievements = {"First Steps", "Explorer", "Collector",
                                     "Master", "Legend", "Completionist"};

    std::println("\n>> Saving to slot 3: End-game Save");
    saveSystem->save(3, "End-game Save");
    std::println("   Player: {}, Level {}", player.name, player.level);

    // Test loading from different slots
    std::println("\n>> Testing load from slot 1...");
    saveSystem->load(1);
    std::println("   Loaded: {}, Level {}", player.name, player.level);

    std::println("\n>> Testing load from slot 3...");
    saveSystem->load(3);
    std::println("   Loaded: {}, Level {}", player.name, player.level);

    //==========================================================================
    // DEMO 3: Quick Save/Load
    //==========================================================================
    printSeparator("DEMO 3: Quick Save/Load");

    player.name = "Quick Save Test";
    player.level = 25;
    player.positionX = 500.0f;
    player.positionY = 300.0f;

    std::println("Current state:");
    std::println("  Player: {}, Level {}", player.name, player.level);
    std::println("  Position: ({:.2f}, {:.2f})", player.positionX, player.positionY);

    std::println("\n>> Performing quick save...");
    saveSystem->quickSave();
    std::println("   Quick save created (slot: {})", SaveSlots::QuickSave);

    // Modify state
    player.name = "Modified After Quick Save";
    player.level = 1;
    player.positionX = 0.0f;
    player.positionY = 0.0f;

    std::println("\n>> State modified:");
    std::println("  Player: {}, Level {}", player.name, player.level);
    std::println("  Position: ({:.2f}, {:.2f})", player.positionX, player.positionY);

    std::println("\n>> Performing quick load...");
    saveSystem->quickLoad();
    std::println("   State restored from quick save");

    std::println("\nRestored state:");
    std::println("  Player: {}, Level {}", player.name, player.level);
    std::println("  Position: ({:.2f}, {:.2f})", player.positionX, player.positionY);

    //==========================================================================
    // DEMO 4: Auto-Save System
    //==========================================================================
    printSeparator("DEMO 4: Auto-Save System");

    std::println("Testing auto-save functionality...\n");

    player.name = "Auto Save Test";
    player.level = 10;

    std::println(">> Enabling auto-save (interval: 5 seconds)");
    saveSystem->enableAutoSave(std::chrono::seconds(5));
    std::println("   Auto-save enabled");

    std::println("\n>> Simulating game updates...");
    for (int i = 0; i < 8; ++i) {
        // Simulate 1-second updates
        saveSystem->update(1.0f);
        player.experience += 100;

        if (i == 0 || i == 4 || i == 7) {
            std::println("   Update {}: Experience = {}", i + 1, player.experience);
        }
    }

    std::println("\n>> Checking if auto-save was triggered...");
    if (saveSystem->saveExists(SaveSlots::AutoSave)) {
        std::println("   Auto-save file exists!");

        // Load auto-save to verify
        player.experience = 0; // Reset
        saveSystem->load(SaveSlots::AutoSave);
        std::println("   Loaded auto-save: Experience = {}", player.experience);
    }

    std::println("\n>> Disabling auto-save...");
    saveSystem->disableAutoSave();
    std::println("   Auto-save disabled");

    std::println("\n>> Manual auto-save trigger...");
    player.experience = 12345;
    saveSystem->autoSave();
    std::println("   Auto-save manually triggered");

    //==========================================================================
    // DEMO 5: Save Metadata Queries
    //==========================================================================
    printSeparator("DEMO 5: Save Metadata Queries");

    std::println("Querying save file metadata...\n");

    std::println(">> Checking if saves exist:");
    for (SaveSlot slot : {0, 1, 2, 3, 4, 99}) {
        bool exists = saveSystem->saveExists(slot);
        std::println("   Slot {}: {}", slot, exists ? "EXISTS" : "NOT FOUND");
    }

    std::println("\n>> Getting metadata for slot 0:");
    auto meta0 = saveSystem->getSaveMetadata(0);
    if (meta0) {
        printMetadata(*meta0);
    } else {
        std::println("   No metadata found for slot 0");
    }

    std::println("\n>> Getting metadata for slot 2:");
    auto meta2 = saveSystem->getSaveMetadata(2);
    if (meta2) {
        printMetadata(*meta2);
    } else {
        std::println("   No metadata found for slot 2");
    }

    std::println("\n>> Getting all save metadata:");
    auto allMeta = saveSystem->getAllSaveMetadata();
    std::println("   Found {} save files", allMeta.size());
    for (const auto& meta : allMeta) {
        std::println("\n   Slot {}:", meta.slot);
        std::println("     Name: {}", meta.saveName);
        auto time = std::chrono::system_clock::to_time_t(meta.timestamp);
        std::string timeStr = std::ctime(&time);
        timeStr.pop_back(); // Remove newline
        std::println("     Date: {}", timeStr);
    }

    //==========================================================================
    // DEMO 6: Profile Management
    //==========================================================================
    printSeparator("DEMO 6: Profile Management");

    std::println("Testing profile management...\n");

    std::println(">> Current active profile:");
    std::println("   {}", saveSystem->getActiveProfile());

    std::println("\n>> Listing all profiles:");
    auto profiles = saveSystem->getProfiles();
    for (const auto& profile : profiles) {
        std::println("   - {}", profile);
    }

    // Switch to a new profile
    std::println("\n>> Creating and switching to 'player2' profile...");
    saveSystem->setActiveProfile("player2");
    std::println("   Active profile: {}", saveSystem->getActiveProfile());

    // Save in new profile
    player.name = "Player 2 Character";
    player.level = 3;
    std::println("\n>> Saving to slot 0 in player2 profile...");
    saveSystem->save(0, "Player 2 Save");
    std::println("   Saved: {}, Level {}", player.name, player.level);

    // Switch back to default profile
    std::println("\n>> Switching back to 'default' profile...");
    saveSystem->setActiveProfile("default");
    std::println("   Active profile: {}", saveSystem->getActiveProfile());

    // Load from default profile (should be different)
    std::println("\n>> Loading slot 0 from default profile...");
    saveSystem->load(0);
    std::println("   Loaded: {}, Level {}", player.name, player.level);

    // List profiles again
    std::println("\n>> Listing all profiles after creation:");
    profiles = saveSystem->getProfiles();
    for (const auto& profile : profiles) {
        std::println("   - {}", profile);
    }

    //==========================================================================
    // DEMO 7: Error Handling
    //==========================================================================
    printSeparator("DEMO 7: Error Handling");

    std::println("Testing error conditions...\n");

    std::println(">> Attempting to load non-existent save (slot 999):");
    auto loadError = saveSystem->load(999);
    if (!loadError.has_value()) {
        printError("Load", loadError.error());
    } else {
        std::println("   Unexpected success!");
    }

    std::println("\n>> Attempting to get metadata for non-existent save:");
    auto metaError = saveSystem->getSaveMetadata(999);
    if (!metaError) {
        std::println("   Correctly returned std::nullopt");
    } else {
        std::println("   Unexpected: metadata found!");
    }

    //==========================================================================
    // DEMO 8: Save Deletion
    //==========================================================================
    printSeparator("DEMO 8: Save Deletion");

    std::println("Testing save deletion...\n");

    std::println(">> Saves before deletion:");
    for (SaveSlot slot : {0, 1, 2, 3}) {
        std::println("   Slot {}: {}", slot,
            saveSystem->saveExists(slot) ? "EXISTS" : "NOT FOUND");
    }

    std::println("\n>> Deleting save in slot 1...");
    bool deleted = saveSystem->deleteSave(1);
    std::println("   Deletion {}", deleted ? "SUCCESSFUL" : "FAILED");

    std::println("\n>> Saves after deletion:");
    for (SaveSlot slot : {0, 1, 2, 3}) {
        std::println("   Slot {}: {}", slot,
            saveSystem->saveExists(slot) ? "EXISTS" : "NOT FOUND");
    }

    std::println("\n>> Attempting to delete non-existent save (slot 999):");
    deleted = saveSystem->deleteSave(999);
    std::println("   Result: {}", deleted ? "SUCCESSFUL" : "FAILED (as expected)");

    //==========================================================================
    // DEMO 9: Saveable Registration/Unregistration
    //==========================================================================
    printSeparator("DEMO 9: Saveable Registration/Unregistration");

    std::println("Testing saveable registration management...\n");

    // Create a temporary saveable
    PlayerData tempPlayer;
    tempPlayer.name = "Temporary Player";
    tempPlayer.level = 777;

    std::println(">> Saving with 3 registered saveables (to slot 10)...");
    saveSystem->save(10, "Three Saveables");

    std::println("\n>> Unregistering player saveable...");
    saveSystem->unregisterSaveable(&player);
    std::println("   Player unregistered");

    std::println("\n>> Registering temporary player...");
    saveSystem->registerSaveable(&tempPlayer);
    std::println("   Temporary player registered");

    std::println("\n>> Saving with modified registrations (to slot 11)...");
    saveSystem->save(11, "Modified Saveables");

    std::println("\n>> Re-registering original player...");
    saveSystem->registerSaveable(&player);
    saveSystem->unregisterSaveable(&tempPlayer);
    std::println("   Registration restored to original state");

    //==========================================================================
    // DEMO 10: Archive Operations - All Data Types
    //==========================================================================
    printSeparator("DEMO 10: Archive Operations - All Data Types");

    std::println("Demonstrating all archive read/write operations...\n");

    // Create a comprehensive test saveable
    class ArchiveTestData : public ISaveable {
    public:
        int intValue = 42;
        float floatValue = 3.14159f;
        double doubleValue = 2.71828;
        std::string stringValue = "Hello, Bestow!";
        bool boolValue = true;
        std::vector<std::uint8_t> bytesValue = {0x4A, 0x46, 0x52, 0x41, 0x4D, 0x45}; // "BESTOW"

        std::string getSaveKey() const override {
            return "archive_test";
        }

        void serialize(ISaveArchive& archive) const override {
            archive.writeInt("intValue", intValue);
            archive.writeFloat("floatValue", floatValue);
            archive.writeDouble("doubleValue", doubleValue);
            archive.writeString("stringValue", stringValue);
            archive.writeBool("boolValue", boolValue);
            archive.writeBytes("bytesValue", bytesValue);
        }

        void deserialize(const ILoadArchive& archive) override {
            intValue = archive.readInt("intValue");
            floatValue = archive.readFloat("floatValue");
            doubleValue = archive.readDouble("doubleValue");
            stringValue = archive.readString("stringValue");
            boolValue = archive.readBool("boolValue");
            bytesValue = archive.readBytes("bytesValue");
        }

        void print() const {
            std::println("  int: {}", intValue);
            std::println("  float: {}", floatValue);
            std::println("  double: {}", doubleValue);
            std::println("  string: {}", stringValue);
            std::println("  bool: {}", boolValue);
            std::print("  bytes: ");
            for (auto byte : bytesValue) {
                std::print("{:02X} ", byte);
            }
            std::println("");
        }
    };

    ArchiveTestData archiveTest;
    saveSystem->registerSaveable(&archiveTest);

    std::println("Original values:");
    archiveTest.print();

    std::println("\n>> Saving archive test data...");
    saveSystem->save(20, "Archive Test");

    std::println("\n>> Modifying values...");
    archiveTest.intValue = -999;
    archiveTest.floatValue = 0.0f;
    archiveTest.doubleValue = 0.0;
    archiveTest.stringValue = "Modified";
    archiveTest.boolValue = false;
    archiveTest.bytesValue.clear();

    std::println("\nModified values:");
    archiveTest.print();

    std::println("\n>> Loading archive test data...");
    saveSystem->load(20);

    std::println("\nRestored values:");
    archiveTest.print();

    saveSystem->unregisterSaveable(&archiveTest);

    //==========================================================================
    // Summary
    //==========================================================================
    printSeparator("DEMO COMPLETE");

    std::println("All Save System API features have been demonstrated:");
    std::println("  [X] ISaveSystem::update() - Auto-save timer updates");
    std::println("  [X] ISaveSystem::registerSaveable() - Register objects");
    std::println("  [X] ISaveSystem::unregisterSaveable() - Unregister objects");
    std::println("  [X] ISaveSystem::save() - Save to slot with name");
    std::println("  [X] ISaveSystem::load() - Load from slot");
    std::println("  [X] ISaveSystem::deleteSave() - Delete save file");
    std::println("  [X] ISaveSystem::quickSave() - Quick save shortcut");
    std::println("  [X] ISaveSystem::quickLoad() - Quick load shortcut");
    std::println("  [X] ISaveSystem::autoSave() - Manual auto-save trigger");
    std::println("  [X] ISaveSystem::enableAutoSave() - Enable with interval");
    std::println("  [X] ISaveSystem::disableAutoSave() - Disable auto-save");
    std::println("  [X] ISaveSystem::getAllSaveMetadata() - List all saves");
    std::println("  [X] ISaveSystem::getSaveMetadata() - Get single save info");
    std::println("  [X] ISaveSystem::saveExists() - Check save existence");
    std::println("  [X] ISaveSystem::setActiveProfile() - Switch profiles");
    std::println("  [X] ISaveSystem::getActiveProfile() - Get current profile");
    std::println("  [X] ISaveSystem::getProfiles() - List all profiles");
    std::println("");
    std::println("  [X] ISaveable::getSaveKey() - Unique identifier");
    std::println("  [X] ISaveable::serialize() - Write to archive");
    std::println("  [X] ISaveable::deserialize() - Read from archive");
    std::println("");
    std::println("  [X] ISaveArchive::writeInt() - Write integer");
    std::println("  [X] ISaveArchive::writeFloat() - Write float");
    std::println("  [X] ISaveArchive::writeDouble() - Write double");
    std::println("  [X] ISaveArchive::writeString() - Write string");
    std::println("  [X] ISaveArchive::writeBool() - Write boolean");
    std::println("  [X] ISaveArchive::writeBytes() - Write byte array");
    std::println("");
    std::println("  [X] ILoadArchive::readInt() - Read integer");
    std::println("  [X] ILoadArchive::readFloat() - Read float");
    std::println("  [X] ILoadArchive::readDouble() - Read double");
    std::println("  [X] ILoadArchive::readString() - Read string");
    std::println("  [X] ILoadArchive::readBool() - Read boolean");
    std::println("  [X] ILoadArchive::readBytes() - Read byte array");
    std::println("");
    std::println("  [X] SaveMetadata - All fields demonstrated");
    std::println("  [X] SaveSlots::QuickSave - Special slot constant");
    std::println("  [X] SaveSlots::AutoSave - Special slot constant");
    std::println("  [X] SaveError enum - Error handling");
    std::println("");
    std::println("Check the 'saves/' directory for generated save files!");

    return 0;
}

# Bestow Save System Guide

**Version:** 1.0
**Module:** `bestow.save`
**Interface:** `ISaveSystem`
**Implementation:** `SaveSystem`

## Table of Contents

1. [Overview](#overview)
2. [Core Concepts](#core-concepts)
3. [Quick Start](#quick-start)
4. [API Reference](#api-reference)
5. [Serialization Guide](#serialization-guide)
6. [Profile Management](#profile-management)
7. [Versioning and Migration](#versioning-and-migration)
8. [Best Practices](#best-practices)
9. [Advanced Usage](#advanced-usage)
10. [Error Handling](#error-handling)
11. [File Format Details](#file-format-details)

---

## Overview

The Bestow Save System provides a robust, binary-based save/load mechanism for game state persistence. It uses **cereal** for binary serialization and **JSON** for metadata, offering fast, compact saves with rich metadata support.

### Key Features

- **Binary serialization** with cereal for compact, fast saves
- **JSON metadata** files for easy inspection (save name, timestamp, playtime, etc.)
- **Profile system** for multiple save profiles (multi-user support)
- **Save slots** for multiple saves per profile
- **Auto-save** with configurable intervals
- **Quick save/load** for instant save points
- **Version management** with magic number validation
- **ISaveable interface** for component-based serialization

### Architecture Note

The Save System is an **intentional exception** to the AssetSystem rule. While most file I/O goes through AssetSystem, SaveSystem needs direct file access because:

1. **Write operations** - AssetSystem is read-only; saves require writing user data
2. **User data vs game assets** - Save files are user-generated, not bundled assets
3. **No hot reload** - Users don't modify save files while playing
4. **Different lifecycle** - Saves are created/deleted during gameplay, not loaded at startup

---

## Core Concepts

### Save Slots

Save slots are identified by `SaveSlot` (a `std::uint32_t`). You can use any slot number from 0 to `UINT32_MAX - 2`. Two special slots are reserved:

```cpp
// Regular save slots
SaveSlot slot0 = 0;
SaveSlot slot1 = 1;
SaveSlot mySlot = 42;

// Reserved slots (defined in SaveSlots namespace)
SaveSlots::QuickSave  // UINT32_MAX - 1 (used by quickSave())
SaveSlots::AutoSave   // UINT32_MAX     (used by autoSave())
```

### Profiles

Profiles enable multiple users to have separate save files. Each profile has its own directory under `saves/`:

```
saves/
  ├── default/          # Default profile
  │   ├── save_0.sav
  │   ├── save_0.meta
  │   └── save_1.sav
  ├── player1/          # Custom profile
  │   └── save_0.sav
  └── player2/
      └── save_0.sav
```

Switching profiles changes which save directory is active:

```cpp
saveSystem->setActiveProfile("player1");
saveSystem->save(0, "Player 1's Save");  // Saves to saves/player1/save_0.sav

saveSystem->setActiveProfile("player2");
saveSystem->save(0, "Player 2's Save");  // Saves to saves/player2/save_0.sav
```

### Save Metadata

Each save has two files:

1. **`.sav` file** - Binary data (cereal format)
2. **`.meta` file** - JSON metadata

Metadata includes:

```cpp
struct SaveMetadata {
    SaveSlot slot;                           // Slot number
    std::string saveName;                    // User-friendly name
    std::chrono::system_clock::time_point timestamp;  // When saved
    std::string gameVersion;                 // Game version (currently "0.1.0")
    std::uint64_t playtimeSeconds;          // Total playtime (not yet tracked)
    float completionPercentage;              // Progress 0-100% (not yet tracked)
    std::optional<std::string> levelName;   // Current level (optional)
    bool hasScreenshot;                      // Screenshot support (not yet implemented)
};
```

Example metadata JSON:

```json
{
  "slot": 0,
  "saveName": "Forest Temple - 65%",
  "timestamp": 1735776000,
  "gameVersion": "0.1.0",
  "playtimeSeconds": 3600,
  "completionPercentage": 65.0
}
```

### ISaveable Interface

Game components implement `ISaveable` to participate in save/load:

```cpp
class ISaveable {
public:
    virtual ~ISaveable() = default;

    // Unique identifier for this saveable type
    virtual std::string getSaveKey() const = 0;

    // Write data to save file
    virtual void serialize(ISaveArchive& archive) const = 0;

    // Read data from save file
    virtual void deserialize(const ILoadArchive& archive) = 0;
};
```

---

## Quick Start

### 1. Basic Save and Load

```cpp
import bestow.save;

// Get the save system (injected via DI)
ISaveSystem* saveSystem = /* from dependency injection */;

// Save to slot 0
auto result = saveSystem->save(0, "My First Save");
if (result.has_value()) {
    std::println("Save successful!");
} else {
    std::println("Save failed: {}", static_cast<int>(result.error()));
}

// Load from slot 0
auto loadResult = saveSystem->load(0);
if (loadResult.has_value()) {
    std::println("Load successful!");
} else {
    std::println("Load failed: {}", static_cast<int>(loadResult.error()));
}
```

### 2. Making Your Component Saveable

```cpp
class PlayerStats : public ISaveable {
public:
    int health = 100;
    int maxHealth = 100;
    int coins = 0;
    std::string currentLevel;

    std::string getSaveKey() const override {
        return "player_stats";
    }

    void serialize(ISaveArchive& archive) const override {
        archive.writeInt("health", health);
        archive.writeInt("maxHealth", maxHealth);
        archive.writeInt("coins", coins);
        archive.writeString("currentLevel", currentLevel);
    }

    void deserialize(const ILoadArchive& archive) override {
        health = archive.readInt("health");
        maxHealth = archive.readInt("maxHealth");
        coins = archive.readInt("coins");
        currentLevel = archive.readString("currentLevel");
    }
};

// Register with save system
PlayerStats playerStats;
saveSystem->registerSaveable(&playerStats);

// Save will now include player stats
saveSystem->save(0, "Checkpoint");

// Clean up when done
saveSystem->unregisterSaveable(&playerStats);
```

### 3. Quick Save/Load

```cpp
// Quick save to reserved slot (F5 key)
saveSystem->quickSave();

// Quick load from reserved slot (F9 key)
saveSystem->quickLoad();
```

### 4. Auto-Save

```cpp
// Enable auto-save every 5 minutes
saveSystem->enableAutoSave(std::chrono::minutes(5));

// In your game loop
void gameUpdate(DeltaTime dt) {
    saveSystem->update(dt);  // Handles auto-save timer

    // ... rest of game logic
}

// Disable auto-save
saveSystem->disableAutoSave();
```

---

## API Reference

### Lifecycle

#### `update(DeltaTime dt)`

Updates the save system. Must be called every frame to handle auto-save.

```cpp
void update(DeltaTime dt) override;
```

**Usage:**
```cpp
void gameLoop() {
    while (running) {
        float dt = calculateDeltaTime();
        saveSystem->update(dt);
    }
}
```

---

### Saveable Registration

#### `registerSaveable(ISaveable* saveable)`

Registers a component to be included in saves.

```cpp
void registerSaveable(ISaveable* saveable) override;
```

**Parameters:**
- `saveable` - Pointer to ISaveable object (must remain valid until unregistered)

**Usage:**
```cpp
PlayerStats stats;
saveSystem->registerSaveable(&stats);
```

**Important:**
- The saveable object must outlive its registration
- Multiple registrations of the same pointer are allowed (implementation keeps duplicates)
- Each saveable must have a unique `getSaveKey()` value

#### `unregisterSaveable(ISaveable* saveable)`

Removes a component from the save list.

```cpp
void unregisterSaveable(ISaveable* saveable) override;
```

**Usage:**
```cpp
saveSystem->unregisterSaveable(&stats);
```

---

### Save/Load Operations

#### `save(SaveSlot slot, const std::string& saveName)`

Saves all registered saveables to the specified slot.

```cpp
Result<void, SaveError> save(SaveSlot slot, const std::string& saveName) override;
```

**Parameters:**
- `slot` - Save slot number (0 to UINT32_MAX - 2)
- `saveName` - User-friendly name for the save

**Returns:**
- `Result<void, SaveError>` - Success (void) or error code

**Errors:**
- `SaveError::IOError` - Failed to create/write save file
- `SaveError::SerializationError` - Exception during serialization

**Usage:**
```cpp
auto result = saveSystem->save(0, "Level 1 Checkpoint");
if (result.has_value()) {
    std::println("Game saved!");
} else {
    switch (result.error()) {
        case SaveError::IOError:
            std::println("Failed to write save file");
            break;
        case SaveError::SerializationError:
            std::println("Failed to serialize game state");
            break;
    }
}
```

#### `load(SaveSlot slot)`

Loads save data from the specified slot into all registered saveables.

```cpp
Result<void, SaveError> load(SaveSlot slot) override;
```

**Parameters:**
- `slot` - Save slot number

**Returns:**
- `Result<void, SaveError>` - Success (void) or error code

**Errors:**
- `SaveError::FileNotFound` - Save file doesn't exist
- `SaveError::IOError` - Failed to read save file
- `SaveError::CorruptedFile` - Invalid magic number
- `SaveError::VersionMismatch` - Save file version mismatch
- `SaveError::SerializationError` - Save has data for unregistered saveables, or deserialization exception

**Usage:**
```cpp
auto result = saveSystem->load(0);
if (!result.has_value()) {
    switch (result.error()) {
        case SaveError::FileNotFound:
            std::println("No save found in slot 0");
            break;
        case SaveError::CorruptedFile:
            std::println("Save file is corrupted!");
            break;
        case SaveError::VersionMismatch:
            std::println("Save was created with a different game version");
            break;
    }
}
```

**Important:**
- All saveables in the save file must have corresponding registered ISaveable objects
- If the save contains data for an unregistered saveable, load returns `SerializationError`
- Saveables are matched by their `getSaveKey()` string

#### `deleteSave(SaveSlot slot)`

Deletes the save file and metadata for the specified slot.

```cpp
bool deleteSave(SaveSlot slot) override;
```

**Returns:**
- `true` if save was deleted
- `false` if save doesn't exist

**Usage:**
```cpp
if (saveSystem->deleteSave(0)) {
    std::println("Save deleted");
} else {
    std::println("Save not found");
}
```

---

### Quick Save/Load

#### `quickSave()`

Saves to the QuickSave reserved slot.

```cpp
void quickSave() override;
```

**Equivalent to:**
```cpp
save(SaveSlots::QuickSave, "Quick Save");
```

#### `quickLoad()`

Loads from the QuickSave reserved slot.

```cpp
void quickLoad() override;
```

**Equivalent to:**
```cpp
load(SaveSlots::QuickSave);
```

---

### Auto-Save

#### `autoSave()`

Manually triggers an auto-save to the AutoSave reserved slot.

```cpp
void autoSave() override;
```

**Equivalent to:**
```cpp
save(SaveSlots::AutoSave, "Auto Save");
```

#### `enableAutoSave(std::chrono::seconds interval)`

Enables automatic saving at the specified interval.

```cpp
void enableAutoSave(std::chrono::seconds interval) override;
```

**Usage:**
```cpp
// Auto-save every 5 minutes
saveSystem->enableAutoSave(std::chrono::minutes(5));

// Auto-save every 30 seconds (for testing)
saveSystem->enableAutoSave(std::chrono::seconds(30));
```

**Important:**
- You must call `update(dt)` every frame for auto-save to work
- Timer starts immediately when enabled
- Auto-save overwrites the previous auto-save

#### `disableAutoSave()`

Disables automatic saving.

```cpp
void disableAutoSave() override;
```

---

### Metadata Queries

#### `saveExists(SaveSlot slot)`

Checks if a save file exists in the specified slot.

```cpp
bool saveExists(SaveSlot slot) const override;
```

**Usage:**
```cpp
if (saveSystem->saveExists(0)) {
    // Show "Continue" option
} else {
    // Show "New Game" option
}
```

#### `getSaveMetadata(SaveSlot slot)`

Retrieves metadata for a specific save slot.

```cpp
std::optional<SaveMetadata> getSaveMetadata(SaveSlot slot) const override;
```

**Returns:**
- `SaveMetadata` if save exists
- `std::nullopt` if save doesn't exist or metadata can't be read

**Usage:**
```cpp
auto meta = saveSystem->getSaveMetadata(0);
if (meta) {
    std::println("Save: {}", meta->saveName);
    std::println("Created: {}", formatTimestamp(meta->timestamp));
    std::println("Playtime: {} hours", meta->playtimeSeconds / 3600);
    std::println("Progress: {}%", meta->completionPercentage);
}
```

#### `getAllSaveMetadata()`

Retrieves metadata for all saves in the current profile.

```cpp
std::vector<SaveMetadata> getAllSaveMetadata() const override;
```

**Returns:**
- Vector of metadata for all existing saves in the active profile

**Usage:**
```cpp
auto allSaves = saveSystem->getAllSaveMetadata();

// Show save selection screen
for (const auto& meta : allSaves) {
    std::println("Slot {}: {} ({}%)",
        meta.slot, meta.saveName, meta.completionPercentage);
}

// Sort by most recent
std::sort(allSaves.begin(), allSaves.end(),
    [](const auto& a, const auto& b) {
        return a.timestamp > b.timestamp;
    });
```

---

### Profile Management

#### `setActiveProfile(const std::string& profileId)`

Switches to a different save profile.

```cpp
void setActiveProfile(const std::string& profileId) override;
```

**Effects:**
- Creates the profile directory if it doesn't exist
- All subsequent save/load operations use this profile

**Usage:**
```cpp
// Main menu - profile selection
saveSystem->setActiveProfile("player1");

// Save to player1's directory
saveSystem->save(0, "Player 1's Save");

// Switch profiles
saveSystem->setActiveProfile("player2");
saveSystem->save(0, "Player 2's Save");
```

#### `getActiveProfile()`

Returns the currently active profile ID.

```cpp
std::string getActiveProfile() const override;
```

**Usage:**
```cpp
std::string current = saveSystem->getActiveProfile();
std::println("Current profile: {}", current);
```

#### `getProfiles()`

Returns a list of all existing profile IDs.

```cpp
std::vector<std::string> getProfiles() const override;
```

**Returns:**
- Vector of profile directory names in `saves/`

**Usage:**
```cpp
// Profile selection screen
auto profiles = saveSystem->getProfiles();
for (const auto& profile : profiles) {
    std::println("Profile: {}", profile);
}
```

---

## Serialization Guide

### Archive Interface

The Save System provides two archive types:

```cpp
class ISaveArchive {
    void writeInt(const std::string& key, int value);
    void writeFloat(const std::string& key, float value);
    void writeDouble(const std::string& key, double value);
    void writeString(const std::string& key, const std::string& value);
    void writeBool(const std::string& key, bool value);
    void writeBytes(const std::string& key, const std::vector<std::uint8_t>& value);
};

class ILoadArchive {
    int readInt(const std::string& key) const;
    float readFloat(const std::string& key) const;
    double readDouble(const std::string& key) const;
    std::string readString(const std::string& key) const;
    bool readBool(const std::string& key) const;
    std::vector<std::uint8_t> readBytes(const std::string& key) const;
};
```

### Implementing ISaveable

#### Basic Example

```cpp
class PlayerInventory : public ISaveable {
public:
    int coins = 0;
    int keys = 0;
    std::vector<std::string> items;
    bool hasMap = false;

    std::string getSaveKey() const override {
        return "player_inventory";
    }

    void serialize(ISaveArchive& archive) const override {
        archive.writeInt("coins", coins);
        archive.writeInt("keys", keys);
        archive.writeBool("hasMap", hasMap);

        // Serialize vector as bytes
        // Convert vector<string> to binary format manually
        // (See "Complex Types" section below)
    }

    void deserialize(const ILoadArchive& archive) override {
        coins = archive.readInt("coins");
        keys = archive.readInt("keys");
        hasMap = archive.readBool("hasMap");
    }
};
```

#### Complex Types

For types not directly supported by the archive interface, use `writeBytes()` / `readBytes()`:

```cpp
class LevelState : public ISaveable {
public:
    glm::vec3 playerPosition;
    std::vector<int> completedQuests;

    std::string getSaveKey() const override {
        return "level_state";
    }

    void serialize(ISaveArchive& archive) const override {
        // Serialize glm::vec3 as 3 floats
        archive.writeFloat("posX", playerPosition.x);
        archive.writeFloat("posY", playerPosition.y);
        archive.writeFloat("posZ", playerPosition.z);

        // Serialize vector<int> as bytes
        std::vector<std::uint8_t> bytes;
        bytes.resize(completedQuests.size() * sizeof(int));
        std::memcpy(bytes.data(), completedQuests.data(), bytes.size());
        archive.writeBytes("completedQuests", bytes);
    }

    void deserialize(const ILoadArchive& archive) override {
        playerPosition.x = archive.readFloat("posX");
        playerPosition.y = archive.readFloat("posY");
        playerPosition.z = archive.readFloat("posZ");

        auto bytes = archive.readBytes("completedQuests");
        completedQuests.resize(bytes.size() / sizeof(int));
        std::memcpy(completedQuests.data(), bytes.data(), bytes.size());
    }
};
```

#### Alternative: Direct Cereal Access

For advanced users who want full cereal features (versioning, complex containers), you can access the underlying cereal archive:

**Note:** This requires including cereal headers and is platform-specific.

```cpp
// NOT RECOMMENDED - breaks the abstraction
// Only use if you need advanced cereal features

#include <cereal/archives/binary.hpp>
#include <cereal/types/vector.hpp>
#include <cereal/types/map.hpp>

class AdvancedSaveable : public ISaveable {
public:
    std::map<std::string, int> questProgress;

    void serialize(ISaveArchive& archive) const override {
        // Get the underlying cereal archive (internal implementation detail)
        auto& cerealArchive = *static_cast<cereal::BinaryOutputArchive*>(
            static_cast<CerealSaveArchive&>(archive).getArchivePtr());

        cerealArchive(questProgress);
    }

    // Similar for deserialize...
};
```

**Warning:** This approach:
- Breaks encapsulation
- Requires platform-specific casts
- May break with future implementations
- Only use if the simple archive interface is insufficient

---

## Profile Management

### Use Cases

1. **Local Multiplayer** - Each player has their own profile
2. **Multiple Playthroughs** - Keep separate saves for different runs
3. **Testing** - Isolate test saves from real saves

### Example: Profile Selection Screen

```cpp
class ProfileManager {
    ISaveSystem* saveSystem_;

    void showProfileSelector() {
        auto profiles = saveSystem_->getProfiles();

        // Show UI for each profile
        for (const auto& profile : profiles) {
            if (ui->button(profile)) {
                selectProfile(profile);
            }
        }

        // New profile button
        if (ui->button("New Profile")) {
            createNewProfile();
        }
    }

    void selectProfile(const std::string& profileId) {
        saveSystem_->setActiveProfile(profileId);

        // Load last save for this profile
        auto saves = saveSystem_->getAllSaveMetadata();
        if (!saves.empty()) {
            // Load most recent
            auto mostRecent = std::max_element(saves.begin(), saves.end(),
                [](const auto& a, const auto& b) {
                    return a.timestamp < b.timestamp;
                });
            saveSystem_->load(mostRecent->slot);
        }
    }

    void createNewProfile() {
        std::string newProfileId = promptForName();
        saveSystem_->setActiveProfile(newProfileId);
        // Start new game
    }
};
```

---

## Versioning and Migration

### Save File Format

Each save file has a header:

```cpp
struct SaveHeader {
    std::uint32_t magicNumber;  // 0x42465356 ("BFSV" = Bestow Save)
    std::uint32_t version;      // Currently 1
};
```

### Version Checking

The system validates save files on load:

```cpp
auto result = saveSystem->load(0);
if (!result.has_value() && result.error() == SaveError::VersionMismatch) {
    std::println("This save was created with a different game version");
    // Offer to migrate or start new game
}
```

### Migration (Future Feature)

**Note:** Migration is not yet implemented. The system reserves `SaveError::MigrationFailed` for future use.

Planned migration system:

```cpp
// FUTURE API - not yet implemented
saveSystem->registerMigration(1, 2, [](SaveData& data) {
    // Migrate save data from version 1 to version 2
    data.addField("newFeature", defaultValue);
    return true;  // success
});
```

For now, version mismatches result in load failure. To handle this:

1. **Manual migration** - Load raw file, convert, re-save
2. **Graceful degradation** - Detect version, load what you can
3. **Fresh start** - Prompt user to start a new game

---

## Best Practices

### 1. What to Save vs Derive

**Save:**
- Player state (health, inventory, position)
- Quest progress
- World changes (defeated enemies, opened chests)
- Player choices and flags

**Don't Save (derive at runtime):**
- Texture handles (reload via AssetSystem)
- Entity IDs (regenerate from blueprints)
- UI state (rebuild from game state)
- Transient effects (particles, sounds)

### 2. Save Frequently, But Not Too Frequently

```cpp
// Good: Save at checkpoints
void onCheckpointReached() {
    saveSystem->save(currentSlot, "Checkpoint");
}

// Good: Auto-save on level transitions
void onLevelComplete() {
    saveSystem->autoSave();
}

// Bad: Save every frame
void update(DeltaTime dt) {
    saveSystem->save(0, "Frame Save");  // DON'T DO THIS
}
```

**Recommendation:**
- Manual saves at player-initiated checkpoints
- Auto-save every 5-10 minutes
- Quick save for player convenience (F5/F9)

### 3. Graceful Error Handling

```cpp
void saveGame(SaveSlot slot) {
    auto result = saveSystem->save(slot, "My Save");

    if (result.has_value()) {
        showNotification("Game Saved!");
    } else {
        switch (result.error()) {
            case SaveError::IOError:
                showError("Failed to save. Check disk space.");
                break;
            case SaveError::SerializationError:
                showError("Failed to save game state. Please report this bug.");
                break;
            default:
                showError("Unknown save error.");
                break;
        }
    }
}
```

### 4. Provide Fallbacks

```cpp
void loadGame(SaveSlot slot) {
    auto result = saveSystem->load(slot);

    if (!result.has_value()) {
        if (result.error() == SaveError::FileNotFound) {
            // No save found, start new game
            startNewGame();
        } else {
            // Corrupted or incompatible save
            if (confirmDialog("Save file is corrupted. Start new game?")) {
                startNewGame();
            }
        }
    }
}
```

### 5. Use Unique Save Keys

```cpp
// Good: Unique, descriptive keys
class PlayerStats : public ISaveable {
    std::string getSaveKey() const override { return "player_stats"; }
};

class EnemyManager : public ISaveable {
    std::string getSaveKey() const override { return "enemy_manager"; }
};

// Bad: Generic or duplicate keys
class PlayerStats : public ISaveable {
    std::string getSaveKey() const override { return "data"; }  // Too generic
};

class PlayerInventory : public ISaveable {
    std::string getSaveKey() const override { return "player_stats"; }  // DUPLICATE!
};
```

### 6. Validate Loaded Data

```cpp
void deserialize(const ILoadArchive& archive) override {
    health = archive.readInt("health");
    maxHealth = archive.readInt("maxHealth");

    // Validate and clamp
    if (health < 0) health = 0;
    if (health > maxHealth) health = maxHealth;
    if (maxHealth <= 0) maxHealth = 100;  // Sensible default
}
```

### 7. Test Your Save System Early

```cpp
// Unit test example
void testSaveLoad() {
    PlayerStats stats;
    stats.health = 75;
    stats.coins = 100;

    saveSystem->registerSaveable(&stats);
    saveSystem->save(999, "Test Save");  // Use high slot for tests

    // Modify state
    stats.health = 0;
    stats.coins = 0;

    // Load should restore
    saveSystem->load(999);
    assert(stats.health == 75);
    assert(stats.coins == 100);

    // Clean up
    saveSystem->deleteSave(999);
    saveSystem->unregisterSaveable(&stats);
}
```

---

## Advanced Usage

### Example: Save Slot Management System

```cpp
class SaveSlotManager {
public:
    SaveSlotManager(ISaveSystem* saveSystem) : saveSystem_(saveSystem) {}

    // Find the first empty slot
    std::optional<SaveSlot> findEmptySlot() {
        for (SaveSlot slot = 0; slot < maxSlots_; ++slot) {
            if (!saveSystem_->saveExists(slot)) {
                return slot;
            }
        }
        return std::nullopt;  // All slots full
    }

    // Get all saves sorted by recency
    std::vector<SaveMetadata> getSavesSortedByDate() {
        auto saves = saveSystem_->getAllSaveMetadata();
        std::sort(saves.begin(), saves.end(),
            [](const auto& a, const auto& b) {
                return a.timestamp > b.timestamp;
            });
        return saves;
    }

    // Get oldest save (for auto-overwrite)
    std::optional<SaveSlot> getOldestSave() {
        auto saves = saveSystem_->getAllSaveMetadata();
        if (saves.empty()) return std::nullopt;

        auto oldest = std::min_element(saves.begin(), saves.end(),
            [](const auto& a, const auto& b) {
                return a.timestamp < b.timestamp;
            });
        return oldest->slot;
    }

    // Save to first available slot, or overwrite oldest
    SaveSlot smartSave(const std::string& name) {
        auto slot = findEmptySlot();
        if (!slot) {
            slot = getOldestSave();
            if (!slot) slot = 0;  // Fallback
        }

        saveSystem_->save(*slot, name);
        return *slot;
    }

private:
    ISaveSystem* saveSystem_;
    static constexpr SaveSlot maxSlots_ = 10;
};
```

### Example: Save Screenshot System (Future)

```cpp
class SaveWithScreenshot {
public:
    void saveWithScreenshot(SaveSlot slot, const std::string& name) {
        // Take screenshot
        auto screenshot = captureScreenshot();

        // Save game state
        auto result = saveSystem_->save(slot, name);
        if (!result.has_value()) return;

        // Save screenshot as PNG
        auto metaPath = saveSystem_->getMetadataPath(slot);
        auto screenshotPath = metaPath.parent_path() /
            ("save_" + std::to_string(slot) + ".png");
        saveScreenshot(screenshot, screenshotPath);

        // Update metadata to indicate screenshot exists
        // (Requires extending SaveMetadata)
    }

private:
    ISaveSystem* saveSystem_;

    Image captureScreenshot() {
        // Capture framebuffer
        // ... implementation ...
    }

    void saveScreenshot(const Image& img, const std::filesystem::path& path) {
        // Save as PNG using stb_image_write or similar
        // ... implementation ...
    }
};
```

### Example: Cloud Save Integration

```cpp
class CloudSaveManager {
public:
    void uploadSave(SaveSlot slot) {
        if (!saveSystem_->saveExists(slot)) return;

        auto savePath = getSavePath(slot);
        auto metaPath = getMetadataPath(slot);

        // Read save file
        std::ifstream saveFile(savePath, std::ios::binary);
        std::vector<std::uint8_t> saveData(
            (std::istreambuf_iterator<char>(saveFile)),
            std::istreambuf_iterator<char>());

        // Upload to cloud (Steam, Epic, etc.)
        cloudService_->uploadFile("save_" + std::to_string(slot) + ".sav", saveData);
    }

    void downloadSave(SaveSlot slot) {
        auto saveData = cloudService_->downloadFile("save_" + std::to_string(slot) + ".sav");

        auto savePath = getSavePath(slot);
        std::ofstream saveFile(savePath, std::ios::binary);
        saveFile.write(reinterpret_cast<const char*>(saveData.data()), saveData.size());
        saveFile.close();
    }

    void syncSaves() {
        // Compare local and cloud timestamps
        // Download newer saves, upload newer local saves
    }

private:
    ISaveSystem* saveSystem_;
    CloudService* cloudService_;
};
```

---

## Error Handling

### Error Codes

```cpp
enum class SaveError {
    Success,              // No error (not used in Result)
    FileNotFound,         // Save file doesn't exist
    CorruptedFile,        // Invalid magic number
    InvalidChecksum,      // (Reserved for future use)
    VersionMismatch,      // Save version != current version
    MigrationFailed,      // (Reserved for future use)
    IOError,              // File I/O failed (permissions, disk full)
    SerializationError    // Exception during save/load, or unknown saveable
};
```

### Error Handling Patterns

#### Pattern 1: Early Return

```cpp
void loadGameOrStartNew(SaveSlot slot) {
    auto result = saveSystem->load(slot);
    if (!result.has_value()) {
        startNewGame();
        return;
    }

    continueGame();
}
```

#### Pattern 2: Match on Error

```cpp
void handleLoadError(SaveError error) {
    switch (error) {
        case SaveError::FileNotFound:
            std::println("No save found");
            break;
        case SaveError::CorruptedFile:
            std::println("Save is corrupted. Try another slot.");
            break;
        case SaveError::VersionMismatch:
            std::println("Save is from a different game version");
            break;
        case SaveError::IOError:
            std::println("Failed to read save file");
            break;
        case SaveError::SerializationError:
            std::println("Failed to load save data");
            break;
        default:
            std::println("Unknown error");
            break;
    }
}
```

#### Pattern 3: Retry Logic

```cpp
bool saveWithRetry(SaveSlot slot, const std::string& name, int maxRetries = 3) {
    for (int attempt = 0; attempt < maxRetries; ++attempt) {
        auto result = saveSystem->save(slot, name);
        if (result.has_value()) {
            return true;
        }

        if (result.error() == SaveError::IOError) {
            // Wait and retry for I/O errors
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        } else {
            // Don't retry for other errors
            break;
        }
    }

    return false;
}
```

---

## File Format Details

### Save File Structure

```
[Header: 8 bytes]
  - Magic Number: 4 bytes (0x42465356 = "BFSV")
  - Version: 4 bytes (currently 1)

[Cereal Binary Archive]
  - Saveable Count: uint32_t
  - For each saveable:
    - Key: string (cereal serialized)
    - Data: serialized via ISaveArchive calls
```

### Metadata File Structure

JSON format:

```json
{
  "slot": 0,
  "saveName": "Forest Temple",
  "timestamp": 1735776000,
  "gameVersion": "0.1.0",
  "playtimeSeconds": 3600,
  "completionPercentage": 65.0,
  "levelName": "forest_temple",     // optional
  "hasScreenshot": false
}
```

### File Locations

```
saves/
  └── {profile}/
      ├── save_{slot}.sav   # Binary save data
      └── save_{slot}.meta  # JSON metadata
```

Examples:
- `saves/default/save_0.sav`
- `saves/default/save_0.meta`
- `saves/player1/save_0.sav`
- `saves/default/save_4294967294.sav` (QuickSave slot)
- `saves/default/save_4294967295.sav` (AutoSave slot)

### Compression (Future)

**Note:** Compression with zstd is planned but not yet implemented.

Future implementation will compress the cereal archive section:

```cpp
// FUTURE API
saveSystem->enableCompression(true);
saveSystem->setCompressionLevel(3);  // zstd level 1-22
```

Compressed saves will have a modified header:

```
[Header: 12 bytes]
  - Magic Number: 4 bytes (0x42465356)
  - Version: 4 bytes
  - Flags: 4 bytes (bit 0 = compressed)

[Compressed Data]
  - Decompressed Size: 4 bytes
  - zstd Compressed Archive: variable length
```

---

## Complete Example: Platformer Save System

```cpp
import bestow.save;

// ============================================================================
// Saveable Components
// ============================================================================

class PlayerState : public ISaveable {
public:
    int health = 100;
    int maxHealth = 100;
    int coins = 0;
    int lives = 3;
    glm::vec2 position = {0, 0};
    std::string currentLevel = "level1";

    std::string getSaveKey() const override {
        return "player_state";
    }

    void serialize(ISaveArchive& archive) const override {
        archive.writeInt("health", health);
        archive.writeInt("maxHealth", maxHealth);
        archive.writeInt("coins", coins);
        archive.writeInt("lives", lives);
        archive.writeFloat("posX", position.x);
        archive.writeFloat("posY", position.y);
        archive.writeString("currentLevel", currentLevel);
    }

    void deserialize(const ILoadArchive& archive) override {
        health = archive.readInt("health");
        maxHealth = archive.readInt("maxHealth");
        coins = archive.readInt("coins");
        lives = archive.readInt("lives");
        position.x = archive.readFloat("posX");
        position.y = archive.readFloat("posY");
        currentLevel = archive.readString("currentLevel");

        // Validate
        if (health < 0) health = 0;
        if (health > maxHealth) health = maxHealth;
        if (lives < 0) lives = 0;
    }
};

class GameProgress : public ISaveable {
public:
    std::set<std::string> completedLevels;
    std::set<std::string> unlockedAbilities;
    int highScore = 0;
    float totalPlaytime = 0.0f;

    std::string getSaveKey() const override {
        return "game_progress";
    }

    void serialize(ISaveArchive& archive) const override {
        // Serialize sets as byte arrays
        std::vector<std::uint8_t> levelsData;
        for (const auto& level : completedLevels) {
            levelsData.insert(levelsData.end(), level.begin(), level.end());
            levelsData.push_back('\0');  // Null terminator
        }
        archive.writeBytes("completedLevels", levelsData);

        std::vector<std::uint8_t> abilitiesData;
        for (const auto& ability : unlockedAbilities) {
            abilitiesData.insert(abilitiesData.end(), ability.begin(), ability.end());
            abilitiesData.push_back('\0');
        }
        archive.writeBytes("unlockedAbilities", abilitiesData);

        archive.writeInt("highScore", highScore);
        archive.writeFloat("totalPlaytime", totalPlaytime);
    }

    void deserialize(const ILoadArchive& archive) override {
        auto levelsData = archive.readBytes("completedLevels");
        completedLevels.clear();
        std::string current;
        for (auto byte : levelsData) {
            if (byte == '\0') {
                if (!current.empty()) {
                    completedLevels.insert(current);
                    current.clear();
                }
            } else {
                current.push_back(byte);
            }
        }

        auto abilitiesData = archive.readBytes("unlockedAbilities");
        unlockedAbilities.clear();
        current.clear();
        for (auto byte : abilitiesData) {
            if (byte == '\0') {
                if (!current.empty()) {
                    unlockedAbilities.insert(current);
                    current.clear();
                }
            } else {
                current.push_back(byte);
            }
        }

        highScore = archive.readInt("highScore");
        totalPlaytime = archive.readFloat("totalPlaytime");
    }
};

// ============================================================================
// Game Class
// ============================================================================

class PlatformerGame {
public:
    PlatformerGame(ISaveSystem* saveSystem)
        : saveSystem_(saveSystem) {

        // Register saveables
        saveSystem_->registerSaveable(&playerState_);
        saveSystem_->registerSaveable(&gameProgress_);

        // Enable auto-save every 5 minutes
        saveSystem_->enableAutoSave(std::chrono::minutes(5));
    }

    ~PlatformerGame() {
        saveSystem_->unregisterSaveable(&playerState_);
        saveSystem_->unregisterSaveable(&gameProgress_);
    }

    void update(float dt) {
        // Update save system (for auto-save)
        saveSystem_->update(dt);

        // Update game logic
        // ...
    }

    // Called when player presses F5
    void onQuickSave() {
        saveSystem_->quickSave();
        showNotification("Quick Saved!");
    }

    // Called when player presses F9
    void onQuickLoad() {
        saveSystem_->quickLoad();
        showNotification("Quick Loaded!");
    }

    // Called when player completes a level
    void onLevelComplete(const std::string& levelName) {
        gameProgress_.completedLevels.insert(levelName);

        // Auto-save on level completion
        saveSystem_->autoSave();
    }

    // Called when player dies
    void onPlayerDeath() {
        playerState_.lives--;

        if (playerState_.lives <= 0) {
            showGameOver();
        } else {
            // Load from last checkpoint (auto-save)
            auto result = saveSystem_->load(SaveSlots::AutoSave);
            if (!result.has_value()) {
                // No auto-save, restart level
                restartLevel();
            }
        }
    }

    // Called from main menu
    void loadGame(SaveSlot slot) {
        auto result = saveSystem_->load(slot);

        if (result.has_value()) {
            // Load successful, continue from saved position
            loadLevel(playerState_.currentLevel);
            spawnPlayerAt(playerState_.position);
        } else {
            handleLoadError(result.error());
        }
    }

    // Called from pause menu
    void saveGame(SaveSlot slot) {
        // Update current position and level
        playerState_.position = player_->getPosition();
        playerState_.currentLevel = currentLevel_->getName();

        // Calculate completion percentage
        float completion = (gameProgress_.completedLevels.size() / float(totalLevels_)) * 100.0f;

        // Generate save name
        std::string saveName = std::format("{} - {}%",
            playerState_.currentLevel, static_cast<int>(completion));

        auto result = saveSystem_->save(slot, saveName);

        if (result.has_value()) {
            showNotification("Game Saved!");
        } else {
            showError("Failed to save game");
        }
    }

private:
    ISaveSystem* saveSystem_;

    // Saveable state
    PlayerState playerState_;
    GameProgress gameProgress_;

    // Runtime state (not saved)
    Player* player_ = nullptr;
    Level* currentLevel_ = nullptr;
    int totalLevels_ = 10;

    void handleLoadError(SaveError error) {
        switch (error) {
            case SaveError::FileNotFound:
                showError("No save found in this slot");
                break;
            case SaveError::CorruptedFile:
                showError("Save file is corrupted");
                break;
            case SaveError::VersionMismatch:
                showError("Save is from a different game version");
                break;
            default:
                showError("Failed to load save");
                break;
        }
    }

    void showNotification(const std::string& msg) { /* ... */ }
    void showError(const std::string& msg) { /* ... */ }
    void showGameOver() { /* ... */ }
    void restartLevel() { /* ... */ }
    void loadLevel(const std::string& name) { /* ... */ }
    void spawnPlayerAt(glm::vec2 pos) { /* ... */ }
};
```

---

## Summary

The Bestow Save System provides:

- Fast binary serialization with cereal
- Rich JSON metadata for save management
- Profile support for multi-user scenarios
- Auto-save and quick save convenience features
- Robust error handling with `std::expected`
- Simple ISaveable interface for components

**Key Takeaways:**

1. Implement `ISaveable` for components that need persistence
2. Register saveables before saving, unregister when done
3. Use `save()` and `load()` for manual saves
4. Use `quickSave()` / `quickLoad()` for player convenience
5. Enable auto-save for safety
6. Handle errors gracefully with fallbacks
7. Test your save system early and often

For questions or issues, refer to:
- Interface: `/bestow-contract/src/bestow.save.cppm`
- Implementation: `/bestow-save/src/SaveSystem.cpp`
- Tests: `/tests/unit/SaveSystemTests.cpp`

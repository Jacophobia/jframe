# Bestow Save System Guide

**Version:** 2.0
**Module:** `bestow.save`
**Interface:** `ISaveSystem`

## Table of Contents

1. [Overview](#overview)
2. [Core Concepts](#core-concepts)
3. [Quick Start](#quick-start)
4. [API Reference](#api-reference)
5. [Serialization Guide](#serialization-guide)
6. [Profile Management](#profile-management)
7. [Best Practices](#best-practices)
8. [Error Handling](#error-handling)
9. [Complete Examples](#complete-examples)

---

## Overview

The Bestow Save System provides a robust save/load mechanism for game state persistence. It uses abstract archive interfaces (`ISaveArchive` and `ILoadArchive`) for serialization, allowing game components to save their state without depending on specific serialization libraries.

### Key Features

- **Abstract archive interfaces** - No direct cereal dependency in your game code
- **ISaveable interface** - Simple contract for serializable components
- **Profile system** - Multiple save profiles for different users
- **Save slots** - Numbered save slots (0 to `UINT32_MAX - 2`)
- **Quick save/load** - Instant save points (reserved slot `UINT32_MAX - 1`)
- **Auto-save** - Automatic saves at configurable intervals (reserved slot `UINT32_MAX`)
- **Rich metadata** - Save name, timestamp, playtime, completion percentage
- **Result-based error handling** - `std::expected<void, SaveError>` for robust error handling

### Architecture Note: Intentional Exception to AssetSystem Rule

**The Save System is an INTENTIONAL EXCEPTION to the AssetSystem gateway rule.**

While the engine architecture requires all file I/O to go through AssetSystem, SaveSystem is exempt because:

1. **Write operations required** - AssetSystem is read-only by design; saves need write access
2. **User data vs game assets** - Save files are user-generated data, not bundled game assets
3. **No hot reload needed** - Users don't modify save files while the game runs
4. **Different lifecycle** - Saves are created/deleted during gameplay, not preloaded at startup

SaveSystem directly uses `std::ofstream`, `std::ifstream`, and `std::filesystem` for file operations. This is by design and architecturally correct.

---

## Core Concepts

### ISaveable Interface

The `ISaveable` interface defines how game components participate in save/load operations:

```cpp
class ISaveable {
public:
    virtual ~ISaveable() = default;

    // Unique identifier for this saveable type
    virtual std::string getSaveKey() const = 0;

    // Write data to save archive
    virtual void serialize(ISaveArchive& archive) const = 0;

    // Read data from load archive
    virtual void deserialize(const ILoadArchive& archive) = 0;
};
```

**Key Points:**

- **getSaveKey()** - Returns a unique string identifying this saveable (e.g., `"player_stats"`)
- **serialize()** - Called during save to write data to the archive
- **deserialize()** - Called during load to read data from the archive
- Each saveable must have a unique save key within your game

### Archive Interfaces

The save system uses abstract archive interfaces to decouple your code from specific serialization libraries:

```cpp
class ISaveArchive {
public:
    virtual void writeInt(const std::string& key, int value) = 0;
    virtual void writeFloat(const std::string& key, float value) = 0;
    virtual void writeDouble(const std::string& key, double value) = 0;
    virtual void writeString(const std::string& key, const std::string& value) = 0;
    virtual void writeBool(const std::string& key, bool value) = 0;
    virtual void writeBytes(const std::string& key, const std::vector<std::uint8_t>& value) = 0;
};

class ILoadArchive {
public:
    virtual int readInt(const std::string& key) const = 0;
    virtual float readFloat(const std::string& key) const = 0;
    virtual double readDouble(const std::string& key) const = 0;
    virtual std::string readString(const std::string& key) const = 0;
    virtual bool readBool(const std::string& key) const = 0;
    virtual std::vector<std::uint8_t> readBytes(const std::string& key) const = 0;
};
```

These interfaces provide a simple, key-value API for serialization. Complex types can be serialized using `writeBytes()` and `readBytes()`.

### Save Slots

Save slots are identified by `SaveSlot` (a `std::uint32_t` alias):

```cpp
using SaveSlot = std::uint32_t;

namespace SaveSlots {
    inline constexpr SaveSlot QuickSave = UINT32_MAX - 1;  // 4294967294
    inline constexpr SaveSlot AutoSave = UINT32_MAX;       // 4294967295
}
```

**Available Slots:**

- **Regular slots:** 0 to `UINT32_MAX - 2` (use 0-9 for player-accessible saves)
- **QuickSave:** `SaveSlots::QuickSave` (used by `quickSave()` and `quickLoad()`)
- **AutoSave:** `SaveSlots::AutoSave` (used by `autoSave()`)

**Example:**

```cpp
// Regular save slots
saveSystem->save(0, "Checkpoint 1");
saveSystem->save(1, "Before Boss Fight");

// Quick save (F5/F9 pattern)
saveSystem->quickSave();  // Uses SaveSlots::QuickSave

// Auto-save (periodic background saves)
saveSystem->enableAutoSave(std::chrono::minutes(5));  // Uses SaveSlots::AutoSave
```

### Save Metadata

Each save slot has associated metadata:

```cpp
struct SaveMetadata {
    SaveSlot slot;                                      // Slot number
    std::string saveName;                               // User-friendly name
    std::chrono::system_clock::time_point timestamp;    // When saved
    std::string gameVersion;                            // Game version string
    std::uint64_t playtimeSeconds = 0;                  // Total playtime
    float completionPercentage = 0.0f;                  // Progress (0-100)
    std::optional<std::string> levelName;               // Current level (optional)
    bool hasScreenshot = false;                         // Screenshot exists
};
```

Metadata allows you to display save information without loading the entire save file.

### Profiles

Profiles enable multiple users to have separate save directories:

```
saves/
  ├── default/          # Default profile
  │   ├── save_0.sav
  │   └── save_0.meta
  ├── player1/
  │   └── save_0.sav
  └── player2/
      └── save_0.sav
```

Switch profiles with `setActiveProfile()`:

```cpp
saveSystem->setActiveProfile("player1");
saveSystem->save(0, "Player 1 Save");  // Saves to saves/player1/save_0.sav
```

---

## Quick Start

### 1. Implement ISaveable

```cpp
import bestow.save;

class PlayerStats : public ISaveable {
public:
    int health = 100;
    int maxHealth = 100;
    int coins = 0;

    std::string getSaveKey() const override {
        return "player_stats";  // Must be unique
    }

    void serialize(ISaveArchive& archive) const override {
        archive.writeInt("health", health);
        archive.writeInt("maxHealth", maxHealth);
        archive.writeInt("coins", coins);
    }

    void deserialize(const ILoadArchive& archive) override {
        health = archive.readInt("health");
        maxHealth = archive.readInt("maxHealth");
        coins = archive.readInt("coins");
    }
};
```

### 2. Register Your Saveable

```cpp
PlayerStats playerStats;

// Register before saving/loading
saveSystem->registerSaveable(&playerStats);

// When done (e.g., in destructor)
saveSystem->unregisterSaveable(&playerStats);
```

### 3. Save and Load

```cpp
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
    // playerStats now has loaded values
} else {
    std::println("Load failed: {}", static_cast<int>(loadResult.error()));
}
```

### 4. Quick Save/Load

```cpp
// Quick save (F5 pattern)
saveSystem->quickSave();

// Quick load (F9 pattern)
saveSystem->quickLoad();
```

### 5. Auto-Save

```cpp
// Enable auto-save every 5 minutes
saveSystem->enableAutoSave(std::chrono::minutes(5));

// In your game loop
void gameUpdate(DeltaTime dt) {
    saveSystem->update(dt);  // Handles auto-save timer
}

// Disable when not needed
saveSystem->disableAutoSave();
```

---

## API Reference

### Lifecycle

#### `update(DeltaTime dt)`

Updates the save system (handles auto-save timer). Must be called every frame if using auto-save.

**Parameters:**
- `dt` - Delta time in seconds

**Usage:**
```cpp
void gameLoop() {
    while (running) {
        float dt = calculateDeltaTime();
        saveSystem->update(dt);
        // ... rest of game logic
    }
}
```

---

### Saveable Registration

#### `registerSaveable(ISaveable* saveable)`

Registers a component to be included in saves.

**Parameters:**
- `saveable` - Pointer to ISaveable object (must remain valid until unregistered)

**Important:**
- The saveable object must outlive its registration
- Each saveable must have a unique `getSaveKey()` value
- Multiple registrations of the same pointer are allowed (implementation may keep duplicates)

**Usage:**
```cpp
PlayerStats stats;
saveSystem->registerSaveable(&stats);
```

#### `unregisterSaveable(ISaveable* saveable)`

Removes a component from the save list.

**Parameters:**
- `saveable` - Pointer previously passed to `registerSaveable()`

**Usage:**
```cpp
saveSystem->unregisterSaveable(&stats);
```

---

### Save/Load Operations

#### `save(SaveSlot slot, const std::string& saveName)`

Saves all registered saveables to the specified slot.

```cpp
Result<void, SaveError> save(SaveSlot slot, const std::string& saveName);
```

**Parameters:**
- `slot` - Save slot number (0 to `UINT32_MAX - 2`, or use `SaveSlots::QuickSave`/`AutoSave`)
- `saveName` - User-friendly name for the save (displayed in UI)

**Returns:**
- `Result<void, SaveError>` - Success (empty value) or error code

**Errors:**
- `SaveError::IOError` - Failed to create/write save file
- `SaveError::SerializationError` - Exception during serialization

**Usage:**
```cpp
auto result = saveSystem->save(0, "Level 1 Checkpoint");
if (result.has_value()) {
    std::println("Game saved!");
} else {
    handleSaveError(result.error());
}
```

#### `load(SaveSlot slot)`

Loads save data from the specified slot into all registered saveables.

```cpp
Result<void, SaveError> load(SaveSlot slot);
```

**Parameters:**
- `slot` - Save slot number

**Returns:**
- `Result<void, SaveError>` - Success (empty value) or error code

**Errors:**
- `SaveError::FileNotFound` - Save file doesn't exist
- `SaveError::IOError` - Failed to read save file
- `SaveError::CorruptedFile` - Invalid magic number or corrupted data
- `SaveError::VersionMismatch` - Save file version mismatch
- `SaveError::SerializationError` - Save contains data for unregistered saveables, or deserialization exception

**Important:**
- All saveables in the save file must have corresponding registered ISaveable objects
- If the save contains data for an unregistered saveable, load returns `SerializationError`
- Saveables are matched by their `getSaveKey()` string

**Usage:**
```cpp
auto result = saveSystem->load(0);
if (!result.has_value()) {
    switch (result.error()) {
        case SaveError::FileNotFound:
            std::println("No save found");
            break;
        case SaveError::CorruptedFile:
            std::println("Save file is corrupted!");
            break;
        case SaveError::VersionMismatch:
            std::println("Save is from a different game version");
            break;
        default:
            std::println("Failed to load save");
            break;
    }
}
```

#### `deleteSave(SaveSlot slot)`

Deletes the save file and metadata for the specified slot.

```cpp
bool deleteSave(SaveSlot slot);
```

**Returns:**
- `true` if save was deleted
- `false` if save doesn't exist

**Usage:**
```cpp
if (saveSystem->deleteSave(0)) {
    std::println("Save deleted");
} else {
    std::println("No save in that slot");
}
```

---

### Quick Save/Load

#### `quickSave()`

Saves to the QuickSave reserved slot (`SaveSlots::QuickSave`).

```cpp
void quickSave();
```

**Equivalent to:**
```cpp
save(SaveSlots::QuickSave, "Quick Save");
```

#### `quickLoad()`

Loads from the QuickSave reserved slot.

```cpp
void quickLoad();
```

**Equivalent to:**
```cpp
load(SaveSlots::QuickSave);
```

**Usage:**
```cpp
// Bind to F5/F9 keys
if (input->isKeyJustPressed(Key::F5)) {
    saveSystem->quickSave();
    showNotification("Quick Saved!");
}

if (input->isKeyJustPressed(Key::F9)) {
    saveSystem->quickLoad();
    showNotification("Quick Loaded!");
}
```

---

### Auto-Save

#### `autoSave()`

Manually triggers an auto-save to the AutoSave reserved slot (`SaveSlots::AutoSave`).

```cpp
void autoSave();
```

**Equivalent to:**
```cpp
save(SaveSlots::AutoSave, "Auto Save");
```

**Usage:**
```cpp
// Auto-save on level completion
void onLevelComplete() {
    saveSystem->autoSave();
}
```

#### `enableAutoSave(std::chrono::seconds interval)`

Enables automatic saving at the specified interval.

```cpp
void enableAutoSave(std::chrono::seconds interval);
```

**Parameters:**
- `interval` - Time between auto-saves (e.g., `std::chrono::minutes(5)`)

**Important:**
- You must call `update(dt)` every frame for auto-save to work
- Timer starts immediately when enabled
- Auto-save overwrites the previous auto-save (always uses `SaveSlots::AutoSave`)

**Usage:**
```cpp
// Auto-save every 5 minutes
saveSystem->enableAutoSave(std::chrono::minutes(5));

// Auto-save every 30 seconds (for testing)
saveSystem->enableAutoSave(std::chrono::seconds(30));
```

#### `disableAutoSave()`

Disables automatic saving.

```cpp
void disableAutoSave();
```

**Usage:**
```cpp
// Disable during cutscenes or boss fights
saveSystem->disableAutoSave();
```

---

### Metadata Queries

#### `saveExists(SaveSlot slot)`

Checks if a save file exists in the specified slot.

```cpp
bool saveExists(SaveSlot slot) const;
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
std::optional<SaveMetadata> getSaveMetadata(SaveSlot slot) const;
```

**Returns:**
- `SaveMetadata` if save exists
- `std::nullopt` if save doesn't exist or metadata can't be read

**Usage:**
```cpp
auto meta = saveSystem->getSaveMetadata(0);
if (meta) {
    std::println("Save: {}", meta->saveName);
    std::println("Version: {}", meta->gameVersion);
    std::println("Playtime: {} hours", meta->playtimeSeconds / 3600);
    std::println("Progress: {}%", meta->completionPercentage);
    if (meta->levelName) {
        std::println("Level: {}", *meta->levelName);
    }
}
```

#### `getAllSaveMetadata()`

Retrieves metadata for all saves in the current profile.

```cpp
std::vector<SaveMetadata> getAllSaveMetadata() const;
```

**Returns:**
- Vector of metadata for all existing saves in the active profile

**Usage:**
```cpp
// Display save selection screen
auto allSaves = saveSystem->getAllSaveMetadata();

for (const auto& meta : allSaves) {
    std::println("Slot {}: {} ({}%)",
        meta.slot, meta.saveName, meta.completionPercentage);
}

// Sort by most recent
std::ranges::sort(allSaves, [](const auto& a, const auto& b) {
    return a.timestamp > b.timestamp;
});
```

---

### Profile Management

#### `setActiveProfile(const std::string& profileId)`

Switches to a different save profile.

```cpp
void setActiveProfile(const std::string& profileId);
```

**Parameters:**
- `profileId` - Profile identifier (becomes directory name under `saves/`)

**Effects:**
- Creates the profile directory if it doesn't exist
- All subsequent save/load operations use this profile

**Usage:**
```cpp
// Main menu - profile selection
saveSystem->setActiveProfile("player1");
saveSystem->save(0, "Player 1's Save");

// Switch profiles
saveSystem->setActiveProfile("player2");
saveSystem->save(0, "Player 2's Save");
```

#### `getActiveProfile()`

Returns the currently active profile ID.

```cpp
std::string getActiveProfile() const;
```

**Usage:**
```cpp
std::string current = saveSystem->getActiveProfile();
std::println("Current profile: {}", current);
```

#### `getProfiles()`

Returns a list of all existing profile IDs.

```cpp
std::vector<std::string> getProfiles() const;
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

// Create new profile button
if (ui->button("New Profile")) {
    std::string name = promptForName();
    saveSystem->setActiveProfile(name);  // Creates directory
}
```

---

## Serialization Guide

### Basic Types

The archive interfaces support these types directly:

```cpp
void serialize(ISaveArchive& archive) const override {
    archive.writeInt("health", health);
    archive.writeFloat("speed", speed);
    archive.writeDouble("preciseValue", preciseValue);
    archive.writeString("name", name);
    archive.writeBool("isAlive", isAlive);
}

void deserialize(const ILoadArchive& archive) override {
    health = archive.readInt("health");
    speed = archive.readFloat("speed");
    preciseValue = archive.readDouble("preciseValue");
    name = archive.readString("name");
    isAlive = archive.readBool("isAlive");
}
```

### Complex Types

For types not directly supported, use `writeBytes()` and `readBytes()`:

```cpp
class LevelState : public ISaveable {
    glm::vec2 playerPosition;
    std::vector<int> completedQuests;

    void serialize(ISaveArchive& archive) const override {
        // Serialize glm::vec2 as 2 floats
        archive.writeFloat("posX", playerPosition.x);
        archive.writeFloat("posY", playerPosition.y);

        // Serialize vector<int> as bytes
        std::vector<std::uint8_t> bytes(completedQuests.size() * sizeof(int));
        std::memcpy(bytes.data(), completedQuests.data(), bytes.size());
        archive.writeBytes("completedQuests", bytes);
    }

    void deserialize(const ILoadArchive& archive) override {
        playerPosition.x = archive.readFloat("posX");
        playerPosition.y = archive.readFloat("posY");

        auto bytes = archive.readBytes("completedQuests");
        completedQuests.resize(bytes.size() / sizeof(int));
        std::memcpy(completedQuests.data(), bytes.data(), bytes.size());
    }
};
```

### Strings and Collections

```cpp
class Inventory : public ISaveable {
    std::vector<std::string> items;
    std::map<std::string, int> quantities;

    void serialize(ISaveArchive& archive) const override {
        // Serialize vector of strings as concatenated bytes
        std::vector<std::uint8_t> itemsData;
        for (const auto& item : items) {
            itemsData.insert(itemsData.end(), item.begin(), item.end());
            itemsData.push_back('\0');  // Null terminator
        }
        archive.writeBytes("items", itemsData);

        // Serialize map manually
        std::vector<std::uint8_t> mapData;
        for (const auto& [key, value] : quantities) {
            // Write key
            mapData.insert(mapData.end(), key.begin(), key.end());
            mapData.push_back('\0');
            // Write value
            mapData.insert(mapData.end(),
                reinterpret_cast<const std::uint8_t*>(&value),
                reinterpret_cast<const std::uint8_t*>(&value) + sizeof(int));
        }
        archive.writeBytes("quantities", mapData);
    }

    void deserialize(const ILoadArchive& archive) override {
        // Deserialize vector of strings
        auto itemsData = archive.readBytes("items");
        items.clear();
        std::string current;
        for (auto byte : itemsData) {
            if (byte == '\0') {
                if (!current.empty()) {
                    items.push_back(current);
                    current.clear();
                }
            } else {
                current.push_back(static_cast<char>(byte));
            }
        }

        // Deserialize map
        auto mapData = archive.readBytes("quantities");
        quantities.clear();
        std::string key;
        size_t i = 0;
        while (i < mapData.size()) {
            // Read key
            while (i < mapData.size() && mapData[i] != '\0') {
                key.push_back(static_cast<char>(mapData[i++]));
            }
            i++;  // Skip null terminator
            // Read value
            int value;
            std::memcpy(&value, &mapData[i], sizeof(int));
            i += sizeof(int);
            quantities[key] = value;
            key.clear();
        }
    }
};
```

### Validation on Load

Always validate loaded data:

```cpp
void deserialize(const ILoadArchive& archive) override {
    health = archive.readInt("health");
    maxHealth = archive.readInt("maxHealth");

    // Validate and clamp
    if (health < 0) health = 0;
    if (maxHealth <= 0) maxHealth = 100;  // Sensible default
    if (health > maxHealth) health = maxHealth;
}
```

---

## Profile Management

### Use Cases

1. **Local Multiplayer** - Each player has their own profile
2. **Multiple Playthroughs** - Separate saves for different runs
3. **Testing** - Isolate test saves from real saves

### Example: Profile Selection

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

        // Load most recent save for this profile
        auto saves = saveSystem_->getAllSaveMetadata();
        if (!saves.empty()) {
            auto mostRecent = std::ranges::max_element(saves,
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

## Best Practices

### 1. What to Save vs Derive

**Save:**
- Player state (health, inventory, position)
- Quest progress and flags
- World changes (defeated enemies, opened chests)
- Player choices

**Don't Save (derive at runtime):**
- Asset handles (reload via AssetSystem)
- Entity IDs (regenerate from blueprints)
- UI state (rebuild from game state)
- Transient effects (particles, sounds)

### 2. Use Unique Save Keys

```cpp
// ✅ Good: Unique, descriptive keys
class PlayerStats : public ISaveable {
    std::string getSaveKey() const override { return "player_stats"; }
};

class EnemyManager : public ISaveable {
    std::string getSaveKey() const override { return "enemy_manager"; }
};

// ❌ Bad: Generic or duplicate keys
class PlayerStats : public ISaveable {
    std::string getSaveKey() const override { return "data"; }  // Too generic
};

class PlayerInventory : public ISaveable {
    std::string getSaveKey() const override { return "player_stats"; }  // DUPLICATE!
};
```

### 3. Handle Errors Gracefully

```cpp
void saveGame(SaveSlot slot, const std::string& name) {
    auto result = saveSystem->save(slot, name);

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

### 4. Register/Unregister Properly

```cpp
class Game {
    PlayerStats playerStats_;
    Inventory inventory_;
    ISaveSystem* saveSystem_;

    Game(ISaveSystem* saveSystem) : saveSystem_(saveSystem) {
        // Register in constructor
        saveSystem_->registerSaveable(&playerStats_);
        saveSystem_->registerSaveable(&inventory_);
    }

    ~Game() {
        // Unregister in destructor
        saveSystem_->unregisterSaveable(&inventory_);
        saveSystem_->unregisterSaveable(&playerStats_);
    }
};
```

### 5. Test Your Save System Early

```cpp
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

## Error Handling

### Error Codes

```cpp
enum class SaveError {
    Success,              // No error (not used in Result)
    FileNotFound,         // Save file doesn't exist
    CorruptedFile,        // Invalid magic number or corrupted data
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

## Complete Examples

### Example 1: Platformer Save System

```cpp
import bestow.save;

// Saveable components
class PlayerState : public ISaveable {
public:
    int health = 100;
    int maxHealth = 100;
    int coins = 0;
    int lives = 3;
    float posX = 0, posY = 0;
    std::string currentLevel = "level1";

    std::string getSaveKey() const override {
        return "player_state";
    }

    void serialize(ISaveArchive& archive) const override {
        archive.writeInt("health", health);
        archive.writeInt("maxHealth", maxHealth);
        archive.writeInt("coins", coins);
        archive.writeInt("lives", lives);
        archive.writeFloat("posX", posX);
        archive.writeFloat("posY", posY);
        archive.writeString("currentLevel", currentLevel);
    }

    void deserialize(const ILoadArchive& archive) override {
        health = archive.readInt("health");
        maxHealth = archive.readInt("maxHealth");
        coins = archive.readInt("coins");
        lives = archive.readInt("lives");
        posX = archive.readFloat("posX");
        posY = archive.readFloat("posY");
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

    std::string getSaveKey() const override {
        return "game_progress";
    }

    void serialize(ISaveArchive& archive) const override {
        // Serialize sets as byte arrays
        std::vector<std::uint8_t> levelsData;
        for (const auto& level : completedLevels) {
            levelsData.insert(levelsData.end(), level.begin(), level.end());
            levelsData.push_back('\0');
        }
        archive.writeBytes("completedLevels", levelsData);

        std::vector<std::uint8_t> abilitiesData;
        for (const auto& ability : unlockedAbilities) {
            abilitiesData.insert(abilitiesData.end(), ability.begin(), ability.end());
            abilitiesData.push_back('\0');
        }
        archive.writeBytes("unlockedAbilities", abilitiesData);

        archive.writeInt("highScore", highScore);
    }

    void deserialize(const ILoadArchive& archive) override {
        // Deserialize sets from bytes
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
                current.push_back(static_cast<char>(byte));
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
                current.push_back(static_cast<char>(byte));
            }
        }

        highScore = archive.readInt("highScore");
    }
};

// Game class
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

        // Game logic...
    }

    void onQuickSave() {
        saveSystem_->quickSave();
        showNotification("Quick Saved!");
    }

    void onQuickLoad() {
        saveSystem_->quickLoad();
        showNotification("Quick Loaded!");
    }

    void onLevelComplete(const std::string& levelName) {
        gameProgress_.completedLevels.insert(levelName);
        saveSystem_->autoSave();  // Auto-save on level completion
    }

    void saveGame(SaveSlot slot) {
        // Update current position
        playerState_.posX = player_->getPosition().x;
        playerState_.posY = player_->getPosition().y;
        playerState_.currentLevel = currentLevel_->getName();

        // Calculate completion
        float completion = (gameProgress_.completedLevels.size() /
                           float(totalLevels_)) * 100.0f;

        std::string saveName = std::format("{} - {}%",
            playerState_.currentLevel, static_cast<int>(completion));

        auto result = saveSystem_->save(slot, saveName);

        if (result.has_value()) {
            showNotification("Game Saved!");
        } else {
            showError("Failed to save game");
        }
    }

    void loadGame(SaveSlot slot) {
        auto result = saveSystem_->load(slot);

        if (result.has_value()) {
            loadLevel(playerState_.currentLevel);
            spawnPlayerAt(playerState_.posX, playerState_.posY);
        } else {
            handleLoadError(result.error());
        }
    }

private:
    ISaveSystem* saveSystem_;
    PlayerState playerState_;
    GameProgress gameProgress_;

    // Runtime state (not saved)
    Player* player_ = nullptr;
    Level* currentLevel_ = nullptr;
    int totalLevels_ = 10;

    void handleLoadError(SaveError error) { /* ... */ }
    void showNotification(const std::string& msg) { /* ... */ }
    void showError(const std::string& msg) { /* ... */ }
    void loadLevel(const std::string& name) { /* ... */ }
    void spawnPlayerAt(float x, float y) { /* ... */ }
};
```

### Example 2: Save Slot Manager

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
        std::ranges::sort(saves, [](const auto& a, const auto& b) {
            return a.timestamp > b.timestamp;
        });
        return saves;
    }

    // Get oldest save (for auto-overwrite)
    std::optional<SaveSlot> getOldestSave() {
        auto saves = saveSystem_->getAllSaveMetadata();
        if (saves.empty()) return std::nullopt;

        auto oldest = std::ranges::min_element(saves,
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

---

## Summary

The Bestow Save System provides:

- **Simple ISaveable interface** for game components
- **Abstract archive interfaces** to decouple from serialization libraries
- **Profile support** for multi-user scenarios
- **Auto-save and quick save** convenience features
- **Robust error handling** with `std::expected`
- **Rich metadata** for save management UI

**Key Takeaways:**

1. Implement `ISaveable` for components that need persistence
2. Register saveables before saving, unregister when done
3. Use `save()` and `load()` with `Result<void, SaveError>` return type
4. Use `quickSave()` / `quickLoad()` for player convenience (F5/F9)
5. Enable auto-save for periodic background saves
6. Query metadata for save UI without loading full save files
7. Handle errors gracefully with fallbacks
8. SaveSystem is an intentional exception to the AssetSystem rule

**For more information, refer to:**
- Interface: `/bestow-contract/src/bestow.save.cppm`
- Implementation: `/bestow-save/src/SaveSystem.cpp` (if available)
- Tests: `/tests/unit/SaveSystemTests.cpp` (if available)

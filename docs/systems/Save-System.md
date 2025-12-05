# Bestow Save System

## Table of Contents

- [Overview](#overview)
- [Architecture](#architecture)
- [Save Profiles](#save-profiles)
- [Saving Game Data](#saving-game-data)
- [Loading Game Data](#loading-game-data)
- [User Settings](#user-settings)
- [Binary Serialization](#binary-serialization)
- [Save File Locations](#save-file-locations)
- [Quick Save / Load](#quick-save--load)
- [Auto-Save](#auto-save)
- [Save Metadata](#save-metadata)
- [Code Examples](#code-examples)
- [Best Practices](#best-practices)
- [Error Handling](#error-handling)

## Overview

The Bestow Save System provides a robust, type-safe mechanism for persisting and restoring game state. It supports multiple save slots, user profiles, metadata tracking, and automatic serialization using cereal for binary efficiency.

### Key Features

- **Multiple Save Slots**: Support for unlimited numbered save slots plus special QuickSave and AutoSave slots
- **Profile Management**: Multiple user profiles with isolated save data
- **Binary Serialization**: Fast, compact save files using cereal
- **JSON Metadata**: Human-readable save metadata with timestamps, playtime, completion tracking
- **ISaveable Interface**: Clean abstraction for serializable game components
- **Quick Save/Load**: Instant save and restore with dedicated slot
- **Auto-Save**: Configurable automatic saving at intervals
- **Error Handling**: C++23 `std::expected` for error reporting without exceptions

### Design Philosophy

The Save System follows the **ISaveable pattern**: game components implement the `ISaveable` interface and register themselves with the system. When a save is triggered, the system automatically serializes all registered components to a binary file with metadata.

## Architecture

### Core Components

```
ISaveSystem                Main interface for save/load operations
ISaveable                  Interface for components that can be saved
ISaveArchive              Write interface for serialization
ILoadArchive              Read interface for deserialization
CerealSaveArchive         Binary output implementation using cereal
CerealLoadArchive         Binary input implementation using cereal
SaveMetadata              JSON metadata about a save file
```

### Module Structure

```cpp
import bestow.save;        // Interface only
import bestow.save.impl;   // Implementation + factory
```

### File Format

Each save consists of two files:

1. **Binary Save File** (`.sav`): Contains game state
   - Magic number: `0x4A465356` ("JFSV" = Bestow Save)
   - Version number: `1`
   - Saveable count
   - For each saveable:
     - Key (string)
     - Serialized data (cereal binary)

2. **Metadata File** (`.meta`): Contains save information (JSON)
   - Slot number
   - Save name
   - Timestamp
   - Game version
   - Playtime
   - Completion percentage
   - Optional level name
   - Screenshot flag

## Save Profiles

Profiles allow multiple users to have separate save data on the same system. Each profile gets its own directory under `saves/`.

### Setting the Active Profile

```cpp
import bestow.save;
import bestow.save.impl;

auto saveSystem = createSaveSystem();

// Use the default profile
saveSystem->setActiveProfile("default");

// Switch to a specific player's profile
saveSystem->setActiveProfile("player1");
saveSystem->setActiveProfile("alice");
```

### Getting Available Profiles

```cpp
// List all existing profiles
auto profiles = saveSystem->getProfiles();
for (const auto& profile : profiles) {
    std::cout << "Profile: " << profile << "\n";
}

// Get current profile
std::string currentProfile = saveSystem->getActiveProfile();
```

### Profile Isolation

When you switch profiles, all save operations (save, load, delete, metadata queries) operate only on that profile's directory:

```cpp
saveSystem->setActiveProfile("alice");
saveSystem->save(0, "Alice's First Save");  // saves/alice/save_0.sav

saveSystem->setActiveProfile("bob");
saveSystem->save(0, "Bob's First Save");    // saves/bob/save_0.sav

// Both save slot 0 files coexist in separate directories
```

## Saving Game Data

### Step 1: Implement ISaveable

Any component that needs to persist data implements the `ISaveable` interface:

```cpp
import bestow.save;

class PlayerState : public ISaveable {
public:
    // Player data
    int health = 100;
    int maxHealth = 100;
    float positionX = 0.0f;
    float positionY = 0.0f;
    int coins = 0;
    std::string currentLevel;
    bool hasDoubleJump = false;

    // ISaveable interface
    std::string getSaveKey() const override {
        return "player_state";
    }

    void serialize(ISaveArchive& archive) const override {
        archive.writeInt("health", health);
        archive.writeInt("maxHealth", maxHealth);
        archive.writeFloat("positionX", positionX);
        archive.writeFloat("positionY", positionY);
        archive.writeInt("coins", coins);
        archive.writeString("currentLevel", currentLevel);
        archive.writeBool("hasDoubleJump", hasDoubleJump);
    }

    void deserialize(const ILoadArchive& archive) override {
        health = archive.readInt("health");
        maxHealth = archive.readInt("maxHealth");
        positionX = archive.readFloat("positionX");
        positionY = archive.readFloat("positionY");
        coins = archive.readInt("coins");
        currentLevel = archive.readString("currentLevel");
        hasDoubleJump = archive.readBool("hasDoubleJump");
    }
};
```

### Step 2: Register with Save System

```cpp
import bestow.save.impl;

auto saveSystem = createSaveSystem();
PlayerState playerState;

// Register the saveable (it will be included in all saves)
saveSystem->registerSaveable(&playerState);
```

### Step 3: Trigger Save

```cpp
// Save to slot 0 with a descriptive name
auto result = saveSystem->save(0, "Forest Checkpoint");

if (result.has_value()) {
    std::cout << "Game saved successfully!\n";
} else {
    // Handle error
    SaveError error = result.error();
    std::cout << "Save failed with error code: " << static_cast<int>(error) << "\n";
}
```

### Multiple Saveables

You can register multiple saveable components. Each gets its own key:

```cpp
class InventoryState : public ISaveable {
public:
    std::vector<std::string> items;
    int selectedSlot = 0;

    std::string getSaveKey() const override { return "inventory"; }

    void serialize(ISaveArchive& archive) const override {
        // Serialize item count, then each item
        archive.writeInt("itemCount", static_cast<int>(items.size()));
        for (size_t i = 0; i < items.size(); ++i) {
            archive.writeString("item_" + std::to_string(i), items[i]);
        }
        archive.writeInt("selectedSlot", selectedSlot);
    }

    void deserialize(const ILoadArchive& archive) override {
        int itemCount = archive.readInt("itemCount");
        items.clear();
        for (int i = 0; i < itemCount; ++i) {
            items.push_back(archive.readString("item_" + std::to_string(i)));
        }
        selectedSlot = archive.readInt("selectedSlot");
    }
};

// Register both
PlayerState playerState;
InventoryState inventory;
saveSystem->registerSaveable(&playerState);
saveSystem->registerSaveable(&inventory);

// Both will be saved together
saveSystem->save(1, "Dungeon Entrance");
```

### Unregistering Saveables

When a saveable is destroyed or should no longer be saved:

```cpp
saveSystem->unregisterSaveable(&playerState);
```

## Loading Game Data

### Basic Load

```cpp
auto result = saveSystem->load(0);

if (result.has_value()) {
    std::cout << "Game loaded successfully!\n";
    // All registered saveables have been updated
} else {
    SaveError error = result.error();
    switch (error) {
        case SaveError::FileNotFound:
            std::cout << "No save file in this slot\n";
            break;
        case SaveError::CorruptedFile:
            std::cout << "Save file is corrupted\n";
            break;
        case SaveError::VersionMismatch:
            std::cout << "Save file from incompatible version\n";
            break;
        default:
            std::cout << "Load failed\n";
            break;
    }
}
```

### Load Process

When you call `load()`:

1. Save file is opened and header validated (magic number + version)
2. Number of saved components is read
3. For each saved component:
   - Read the save key
   - Find the registered saveable with matching key
   - Call `deserialize()` on that saveable
4. All registered saveables are updated with loaded data

### Checking if Save Exists

```cpp
if (saveSystem->saveExists(0)) {
    // Safe to load
    saveSystem->load(0);
} else {
    std::cout << "No save in slot 0\n";
}
```

## User Settings

While the Save System primarily handles binary game state, it also writes JSON metadata files that can be used for user-facing settings.

### Settings Pattern (User-Facing)

For user settings that should be human-editable, use a separate JSON-based approach:

```cpp
#include <nlohmann/json.hpp>
#include <fstream>

class GameSettings {
public:
    float masterVolume = 1.0f;
    float musicVolume = 0.8f;
    float sfxVolume = 1.0f;
    bool fullscreen = false;
    int resolutionWidth = 1920;
    int resolutionHeight = 1080;

    void saveToJson(const std::filesystem::path& path) {
        nlohmann::json j;
        j["masterVolume"] = masterVolume;
        j["musicVolume"] = musicVolume;
        j["sfxVolume"] = sfxVolume;
        j["fullscreen"] = fullscreen;
        j["resolution"]["width"] = resolutionWidth;
        j["resolution"]["height"] = resolutionHeight;

        std::ofstream file(path);
        file << j.dump(2);  // Pretty-print with 2-space indent
    }

    void loadFromJson(const std::filesystem::path& path) {
        if (!std::filesystem::exists(path)) return;

        std::ifstream file(path);
        nlohmann::json j;
        file >> j;

        masterVolume = j.value("masterVolume", 1.0f);
        musicVolume = j.value("musicVolume", 0.8f);
        sfxVolume = j.value("sfxVolume", 1.0f);
        fullscreen = j.value("fullscreen", false);
        resolutionWidth = j["resolution"].value("width", 1920);
        resolutionHeight = j["resolution"].value("height", 1080);
    }
};

// Usage
GameSettings settings;
settings.saveToJson("settings.json");
settings.loadFromJson("settings.json");
```

### Metadata as Read-Only Settings

You can also query save metadata for UI display:

```cpp
auto metadata = saveSystem->getSaveMetadata(0);
if (metadata) {
    std::cout << "Save Name: " << metadata->saveName << "\n";
    std::cout << "Playtime: " << metadata->playtimeSeconds << " seconds\n";
    std::cout << "Completion: " << metadata->completionPercentage << "%\n";
}
```

## Binary Serialization

The Save System uses **cereal** for efficient binary serialization. The `ISaveArchive` and `ILoadArchive` interfaces abstract this away.

### Supported Data Types

```cpp
// Integer types
archive.writeInt("myInt", 42);
int value = archive.readInt("myInt");

// Floating-point types
archive.writeFloat("myFloat", 3.14f);
archive.writeDouble("myDouble", 2.718);

// Strings
archive.writeString("myString", "Hello, World!");

// Booleans
archive.writeBool("isActive", true);

// Raw bytes (for custom serialization)
std::vector<std::uint8_t> bytes = {0x01, 0x02, 0x03};
archive.writeBytes("rawData", bytes);
```

### Serializing Complex Types

For custom types, break them down into primitives:

```cpp
struct Weapon {
    std::string name;
    int damage;
    float fireRate;
    bool isAutomatic;
};

class WeaponInventory : public ISaveable {
public:
    std::vector<Weapon> weapons;

    std::string getSaveKey() const override { return "weapons"; }

    void serialize(ISaveArchive& archive) const override {
        archive.writeInt("weaponCount", static_cast<int>(weapons.size()));

        for (size_t i = 0; i < weapons.size(); ++i) {
            std::string prefix = "weapon_" + std::to_string(i) + "_";
            archive.writeString(prefix + "name", weapons[i].name);
            archive.writeInt(prefix + "damage", weapons[i].damage);
            archive.writeFloat(prefix + "fireRate", weapons[i].fireRate);
            archive.writeBool(prefix + "isAutomatic", weapons[i].isAutomatic);
        }
    }

    void deserialize(const ILoadArchive& archive) override {
        int count = archive.readInt("weaponCount");
        weapons.clear();
        weapons.reserve(count);

        for (int i = 0; i < count; ++i) {
            std::string prefix = "weapon_" + std::to_string(i) + "_";
            Weapon w;
            w.name = archive.readString(prefix + "name");
            w.damage = archive.readInt(prefix + "damage");
            w.fireRate = archive.readFloat(prefix + "fireRate");
            w.isAutomatic = archive.readBool(prefix + "isAutomatic");
            weapons.push_back(w);
        }
    }
};
```

### Binary vs JSON

| Use Case | Format | Reason |
|----------|--------|--------|
| Game state (player, inventory, world) | Binary (cereal) | Fast, compact, versioned |
| Save metadata (name, timestamp, playtime) | JSON | Human-readable, editable |
| User settings (volume, resolution) | JSON | User-editable, simple |
| Level data | Lua | Supports comments, functions, inheritance |

## Save File Locations

### Directory Structure

```
saves/
├── default/
│   ├── save_0.sav
│   ├── save_0.meta
│   ├── save_1.sav
│   ├── save_1.meta
│   ├── save_4294967294.sav  (QuickSave slot)
│   ├── save_4294967294.meta
│   ├── save_4294967295.sav  (AutoSave slot)
│   └── save_4294967295.meta
├── player1/
│   ├── save_0.sav
│   └── save_0.meta
└── alice/
    ├── save_0.sav
    ├── save_0.meta
    ├── save_1.sav
    └── save_1.meta
```

### Path Construction

The system builds paths as:

```cpp
// General pattern
saves/{profile}/save_{slot}.sav
saves/{profile}/save_{slot}.meta

// Examples
saves/default/save_0.sav          // Slot 0 in default profile
saves/alice/save_5.sav            // Slot 5 in alice profile
saves/default/save_4294967294.sav // QuickSave (UINT32_MAX - 1)
saves/default/save_4294967295.sav // AutoSave (UINT32_MAX)
```

### Platform-Specific Locations (Future)

In production, you may want to use platform-specific save directories:

```cpp
// Windows: C:\Users\{Username}\AppData\Local\YourGame\saves\
// macOS: ~/Library/Application Support/YourGame/saves/
// Linux: ~/.local/share/YourGame/saves/
```

Currently, the system uses a relative `saves/` directory in the working directory.

## Quick Save / Load

Quick Save/Load provides instant save and restore using a dedicated slot.

### Quick Save

```cpp
// Saves to SaveSlots::QuickSave slot (UINT32_MAX - 1)
saveSystem->quickSave();
```

This is equivalent to:

```cpp
saveSystem->save(SaveSlots::QuickSave, "Quick Save");
```

### Quick Load

```cpp
// Loads from SaveSlots::QuickSave slot
saveSystem->quickLoad();
```

This is equivalent to:

```cpp
saveSystem->load(SaveSlots::QuickSave);
```

### Input Binding Example

```cpp
import bestow.input;
import bestow.save.impl;

void handleInput(IInputSystem* input, ISaveSystem* saveSystem) {
    if (input->isActionJustPressed("QuickSave")) {
        saveSystem->quickSave();
        std::cout << "Quick saved!\n";
    }

    if (input->isActionJustPressed("QuickLoad")) {
        saveSystem->quickLoad();
        std::cout << "Quick loaded!\n";
    }
}
```

## Auto-Save

Auto-save automatically triggers saves at regular intervals.

### Enable Auto-Save

```cpp
// Auto-save every 5 minutes
saveSystem->enableAutoSave(std::chrono::seconds(300));
```

### Disable Auto-Save

```cpp
saveSystem->disableAutoSave();
```

### Update Loop Integration

Auto-save requires calling `update()` each frame:

```cpp
void gameLoop(ISaveSystem* saveSystem, DeltaTime dt) {
    // Update auto-save timer
    saveSystem->update(dt);

    // ... rest of game logic
}
```

### Auto-Save Slot

Auto-saves use the special `SaveSlots::AutoSave` slot (UINT32_MAX):

```cpp
// Check if an auto-save exists
if (saveSystem->saveExists(SaveSlots::AutoSave)) {
    auto metadata = saveSystem->getSaveMetadata(SaveSlots::AutoSave);
    std::cout << "Last auto-save: " << metadata->saveName << "\n";
}

// Manually load the auto-save
saveSystem->load(SaveSlots::AutoSave);
```

### Manual Auto-Save Trigger

You can also manually trigger an auto-save (e.g., at checkpoints):

```cpp
void onCheckpointReached() {
    saveSystem->autoSave();
}
```

## Save Metadata

Metadata provides information about save files without loading the full game state.

### Metadata Fields

```cpp
struct SaveMetadata {
    SaveSlot slot;                             // Save slot number
    std::string saveName;                      // User-provided name
    std::chrono::system_clock::time_point timestamp; // When saved
    std::string gameVersion;                   // Game version
    std::uint64_t playtimeSeconds;             // Total playtime
    float completionPercentage;                // 0.0 - 100.0
    std::optional<std::string> levelName;      // Current level
    bool hasScreenshot;                        // Screenshot available
};
```

### Querying Metadata

```cpp
// Get metadata for a specific slot
auto metadata = saveSystem->getSaveMetadata(0);
if (metadata) {
    std::cout << "Slot 0:\n";
    std::cout << "  Name: " << metadata->saveName << "\n";
    std::cout << "  Game Version: " << metadata->gameVersion << "\n";
    std::cout << "  Playtime: " << metadata->playtimeSeconds / 3600 << " hours\n";
    std::cout << "  Completion: " << metadata->completionPercentage << "%\n";

    if (metadata->levelName) {
        std::cout << "  Level: " << *metadata->levelName << "\n";
    }
}
```

### Get All Saves

```cpp
// Get metadata for all saves in the current profile
auto allSaves = saveSystem->getAllSaveMetadata();

for (const auto& meta : allSaves) {
    std::cout << "Slot " << meta.slot << ": " << meta.saveName << "\n";
}
```

### Save UI Example

```cpp
void renderSaveMenu(ISaveSystem* saveSystem) {
    auto allSaves = saveSystem->getAllSaveMetadata();

    // Sort by slot number
    std::sort(allSaves.begin(), allSaves.end(),
        [](const SaveMetadata& a, const SaveMetadata& b) {
            return a.slot < b.slot;
        });

    for (const auto& meta : allSaves) {
        // Skip special slots (QuickSave, AutoSave)
        if (meta.slot >= SaveSlots::QuickSave) continue;

        // Format timestamp
        auto time = std::chrono::system_clock::to_time_t(meta.timestamp);
        std::string timeStr = std::ctime(&time);

        std::cout << "[Slot " << meta.slot << "] "
                  << meta.saveName << " - "
                  << timeStr;
    }
}
```

## Code Examples

### Complete Game Save Example

```cpp
import bestow.save;
import bestow.save.impl;
import bestow.types;

// Game state components
class PlayerProgress : public ISaveable {
public:
    int currentLevel = 1;
    int experience = 0;
    int gold = 0;

    std::string getSaveKey() const override { return "progress"; }

    void serialize(ISaveArchive& archive) const override {
        archive.writeInt("currentLevel", currentLevel);
        archive.writeInt("experience", experience);
        archive.writeInt("gold", gold);
    }

    void deserialize(const ILoadArchive& archive) override {
        currentLevel = archive.readInt("currentLevel");
        experience = archive.readInt("experience");
        gold = archive.readInt("gold");
    }
};

class PlayerStats : public ISaveable {
public:
    int strength = 10;
    int agility = 10;
    int intelligence = 10;

    std::string getSaveKey() const override { return "stats"; }

    void serialize(ISaveArchive& archive) const override {
        archive.writeInt("strength", strength);
        archive.writeInt("agility", agility);
        archive.writeInt("intelligence", intelligence);
    }

    void deserialize(const ILoadArchive& archive) override {
        strength = archive.readInt("strength");
        agility = archive.readInt("agility");
        intelligence = archive.readInt("intelligence");
    }
};

// Main game class
class Game {
public:
    void initialize() {
        saveSystem_ = createSaveSystem();
        saveSystem_->setActiveProfile("default");

        // Register saveables
        saveSystem_->registerSaveable(&playerProgress_);
        saveSystem_->registerSaveable(&playerStats_);

        // Enable auto-save every 5 minutes
        saveSystem_->enableAutoSave(std::chrono::seconds(300));
    }

    void update(DeltaTime dt) {
        // Update auto-save timer
        saveSystem_->update(dt);

        // Game logic...
    }

    void saveGame(SaveSlot slot, const std::string& name) {
        auto result = saveSystem_->save(slot, name);

        if (result.has_value()) {
            std::cout << "Game saved to slot " << slot << "\n";
        } else {
            std::cout << "Failed to save game\n";
        }
    }

    void loadGame(SaveSlot slot) {
        auto result = saveSystem_->load(slot);

        if (result.has_value()) {
            std::cout << "Game loaded from slot " << slot << "\n";
            std::cout << "Player Level: " << playerProgress_.currentLevel << "\n";
            std::cout << "Strength: " << playerStats_.strength << "\n";
        } else {
            std::cout << "Failed to load game\n";
        }
    }

    void shutdown() {
        saveSystem_->unregisterSaveable(&playerProgress_);
        saveSystem_->unregisterSaveable(&playerStats_);
    }

private:
    std::unique_ptr<ISaveSystem> saveSystem_;
    PlayerProgress playerProgress_;
    PlayerStats playerStats_;
};
```

### Save Menu Example

```cpp
void showSaveMenu(ISaveSystem* saveSystem) {
    std::cout << "=== Save Game ===\n";
    std::cout << "Enter slot number (0-9): ";

    int slot;
    std::cin >> slot;

    std::cout << "Enter save name: ";
    std::string name;
    std::cin.ignore();
    std::getline(std::cin, name);

    auto result = saveSystem->save(slot, name);
    if (result.has_value()) {
        std::cout << "Game saved!\n";
    } else {
        std::cout << "Save failed!\n";
    }
}

void showLoadMenu(ISaveSystem* saveSystem) {
    std::cout << "=== Load Game ===\n";

    auto allSaves = saveSystem->getAllSaveMetadata();
    if (allSaves.empty()) {
        std::cout << "No saves found.\n";
        return;
    }

    for (const auto& meta : allSaves) {
        if (meta.slot >= SaveSlots::QuickSave) continue;

        std::cout << "[" << meta.slot << "] "
                  << meta.saveName << " - "
                  << meta.gameVersion << " - "
                  << meta.playtimeSeconds / 60 << " minutes\n";
    }

    std::cout << "Enter slot number: ";
    int slot;
    std::cin >> slot;

    auto result = saveSystem->load(slot);
    if (result.has_value()) {
        std::cout << "Game loaded!\n";
    } else {
        std::cout << "Load failed!\n";
    }
}
```

## Best Practices

### 1. Use Unique Save Keys

Each saveable must have a unique key:

```cpp
// Good
class PlayerState : public ISaveable {
    std::string getSaveKey() const override { return "player_state"; }
};

class InventoryState : public ISaveable {
    std::string getSaveKey() const override { return "inventory"; }
};

// Bad - duplicate keys will cause issues
class PlayerState : public ISaveable {
    std::string getSaveKey() const override { return "state"; }
};

class InventoryState : public ISaveable {
    std::string getSaveKey() const override { return "state"; }  // Conflict!
};
```

### 2. Register Early, Unregister on Destruction

```cpp
class GameState : public ISaveable {
public:
    GameState(ISaveSystem* saveSystem) : saveSystem_(saveSystem) {
        saveSystem_->registerSaveable(this);
    }

    ~GameState() {
        saveSystem_->unregisterSaveable(this);
    }

private:
    ISaveSystem* saveSystem_;
};
```

### 3. Handle Load Errors Gracefully

```cpp
void loadOrStartNew(ISaveSystem* saveSystem, SaveSlot slot) {
    auto result = saveSystem->load(slot);

    if (!result.has_value()) {
        switch (result.error()) {
            case SaveError::FileNotFound:
                std::cout << "No save found, starting new game\n";
                startNewGame();
                break;

            case SaveError::VersionMismatch:
                std::cout << "Save file from old version, starting new game\n";
                startNewGame();
                break;

            case SaveError::CorruptedFile:
                std::cout << "Save file corrupted, starting new game\n";
                startNewGame();
                break;

            default:
                std::cout << "Load failed, starting new game\n";
                startNewGame();
                break;
        }
    }
}
```

### 4. Use Versioning for Schema Changes

When you modify what data is saved, handle backward compatibility:

```cpp
class PlayerState : public ISaveable {
public:
    int health = 100;
    int maxHealth = 100;
    int mana = 50;  // New field added in v2

    void serialize(ISaveArchive& archive) const override {
        archive.writeInt("version", 2);  // Include version
        archive.writeInt("health", health);
        archive.writeInt("maxHealth", maxHealth);
        archive.writeInt("mana", mana);
    }

    void deserialize(const ILoadArchive& archive) override {
        int version = archive.readInt("version");

        health = archive.readInt("health");
        maxHealth = archive.readInt("maxHealth");

        if (version >= 2) {
            mana = archive.readInt("mana");
        } else {
            mana = 50;  // Default for old saves
        }
    }
};
```

### 5. Validate Save Data After Load

```cpp
void deserialize(const ILoadArchive& archive) override {
    health = archive.readInt("health");
    maxHealth = archive.readInt("maxHealth");

    // Validate loaded data
    if (health < 0) health = 0;
    if (health > maxHealth) health = maxHealth;
    if (maxHealth <= 0) maxHealth = 100;
}
```

### 6. Don't Save Transient State

Avoid saving temporary data:

```cpp
class PlayerState : public ISaveable {
public:
    int health = 100;              // Save
    float invulnerabilityTimer = 0.0f;  // DON'T save (transient)
    bool isJumping = false;        // DON'T save (transient)

    void serialize(ISaveArchive& archive) const override {
        archive.writeInt("health", health);
        // Don't save invulnerabilityTimer or isJumping
    }
};
```

### 7. Profile-Specific vs Global Settings

- **Profile-specific**: Save slots, player progress, campaign state
- **Global**: Graphics settings, audio settings, keybindings

```cpp
// Profile-specific (use Save System)
saveSystem->setActiveProfile("alice");
saveSystem->save(0, "Alice's Progress");

// Global (use separate JSON file)
GameSettings settings;
settings.saveToJson("settings.json");
```

## Error Handling

The Save System uses `std::expected` for error handling:

### SaveError Enum

```cpp
enum class SaveError {
    Success,              // No error (not returned, use .has_value())
    FileNotFound,         // Load: file doesn't exist
    CorruptedFile,        // Load: invalid magic number
    InvalidChecksum,      // Reserved for future use
    VersionMismatch,      // Load: incompatible version
    MigrationFailed,      // Reserved for future use
    IOError,              // File I/O failed
    SerializationError    // Cereal exception or deserialization error
};
```

### Error Handling Pattern

```cpp
import bestow.types;  // For Result<T, E>

auto result = saveSystem->save(0, "My Save");

if (result.has_value()) {
    // Success
    std::cout << "Save succeeded\n";
} else {
    // Error
    SaveError error = result.error();

    switch (error) {
        case SaveError::IOError:
            std::cout << "Disk write failed\n";
            break;
        case SaveError::SerializationError:
            std::cout << "Failed to serialize data\n";
            break;
        default:
            std::cout << "Unknown error\n";
            break;
    }
}
```

### Common Error Scenarios

| Error | Cause | Solution |
|-------|-------|----------|
| `FileNotFound` | Loading non-existent save | Check with `saveExists()` first |
| `CorruptedFile` | Save file has wrong magic number | Delete and create new save |
| `VersionMismatch` | Save from different game version | Migration or reset |
| `IOError` | Disk full, permissions issue | Check disk space, permissions |
| `SerializationError` | Saveable threw exception | Fix `serialize()` / `deserialize()` |

### Defensive Loading

```cpp
Result<void, SaveError> safeLoad(ISaveSystem* saveSystem, SaveSlot slot) {
    // Check existence first
    if (!saveSystem->saveExists(slot)) {
        std::cout << "Slot " << slot << " is empty\n";
        return std::unexpected(SaveError::FileNotFound);
    }

    // Try to load
    auto result = saveSystem->load(slot);

    if (!result.has_value()) {
        std::cout << "Load failed, keeping current state\n";
        return result;  // Propagate error
    }

    std::cout << "Load successful\n";
    return result;
}
```

---

## Summary

The Bestow Save System provides:

- **Binary serialization** with cereal for fast, compact saves
- **JSON metadata** for user-facing information
- **Multiple profiles** for multi-user support
- **Quick Save/Load** for instant save points
- **Auto-Save** for automatic periodic saves
- **Type-safe error handling** with `std::expected`
- **ISaveable interface** for clean component-based serialization

For more details, see:
- Interface: `/Users/jaaaacob/Documents/GameDev/bestow/bestow-contract/src/bestow.save.cppm`
- Implementation: `/Users/jaaaacob/Documents/GameDev/bestow/bestow-save/src/`
- Tests: `/Users/jaaaacob/Documents/GameDev/bestow/tests/unit/SaveSystemTests.cpp`

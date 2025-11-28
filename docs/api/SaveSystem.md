# SaveSystem API

The `SaveSystem` provides binary save/load with versioning, compression, and integrity checks using cereal and zstd.

## Overview

```cpp
auto& save = sys.save;

// Save game state
GameSaveData saveData{
    .playerPosition = physics->getPosition(player),
    .health = entities->get<Health>(player).current,
    .score = currentScore,
    .levelId = *levels->getActiveLevel()
};

auto result = save->saveGame(SaveSlots::AutoSave, saveData, "saves");
if (!result) {
    logError("Save failed");
}

// Load game state
auto loadResult = save->loadGame<GameSaveData>(SaveSlots::AutoSave, "saves");
if (loadResult) {
    auto& data = *loadResult;
    // Restore game state
    physics->setPosition(player, data.playerPosition);
}
```

## Save Operations

### saveGame(SaveSlot slot, const T& data, const std::filesystem::path& directory)

```cpp
template<typename T>
Result<void, SaveError> saveGame(
    SaveSlot slot,
    const T& data,
    const std::filesystem::path& directory
);
```

Saves game data to a binary file.

**Parameters:**
- `slot`: Save slot number (or `SaveSlots::AutoSave`, `SaveSlots::QuickSave`)
- `data`: Data to save (must be serializable with cereal)
- `directory`: Directory to save to (e.g., "saves")

**Returns:** `Result<void, SaveError>`

**Example:**

```cpp
struct PlayerSaveData {
    Vec2 position;
    int health;
    int maxHealth;
    std::vector<std::string> inventory;

    template<typename Archive>
    void serialize(Archive& ar) {
        ar(position.x, position.y, health, maxHealth, inventory);
    }
};

PlayerSaveData data{
    .position = physics->getPosition(player),
    .health = 80,
    .maxHealth = 100,
    .inventory = {"sword", "potion", "key"}
};

auto result = save->saveGame(1, data, "saves");
if (!result) {
    SaveError err = result.error();
    logError("Save failed: " + std::to_string(static_cast<int>(err)));
}
```

---

### loadGame(SaveSlot slot, const std::filesystem::path& directory)

```cpp
template<typename T>
Result<T, SaveError> loadGame(
    SaveSlot slot,
    const std::filesystem::path& directory
);
```

Loads game data from a binary file.

**Returns:** `Result<T, SaveError>`

**Example:**

```cpp
auto result = save->loadGame<PlayerSaveData>(1, "saves");
if (result) {
    PlayerSaveData& data = *result;

    // Restore state
    physics->setPosition(player, data.position);
    entities->get<Health>(player).current = data.health;
    // ... restore inventory
} else {
    SaveError err = result.error();
    logError("Load failed");
}
```

---

## Save Slots

Predefined save slot constants:

```cpp
namespace SaveSlots {
    inline constexpr SaveSlot QuickSave = UINT32_MAX - 1;
    inline constexpr SaveSlot AutoSave = UINT32_MAX;
}
```

You can use any `uint32_t` value (0 to UINT32_MAX-2) for numbered slots.

**Example:**

```cpp
// Numbered slots (manual saves)
save->saveGame(0, data, "saves");  // Slot 1
save->saveGame(1, data, "saves");  // Slot 2
save->saveGame(2, data, "saves");  // Slot 3

// Auto-save
save->saveGame(SaveSlots::AutoSave, data, "saves");

// Quick-save (F5)
save->saveGame(SaveSlots::QuickSave, data, "saves");
```

---

## Save Errors

```cpp
enum class SaveError {
    Success,
    FileNotFound,
    CorruptedFile,
    InvalidChecksum,
    VersionMismatch,
    MigrationFailed,
    IOError,
    SerializationError
};
```

**Error Handling:**

```cpp
auto result = save->loadGame<SaveData>(slot, "saves");
if (!result) {
    switch (result.error()) {
        case SaveError::FileNotFound:
            logInfo("No save file found");
            break;
        case SaveError::CorruptedFile:
            logError("Save file is corrupted");
            break;
        case SaveError::VersionMismatch:
            logError("Save file version incompatible");
            break;
        default:
            logError("Load error");
            break;
    }
}
```

---

## Serializable Data Structures

### Basic Types

cereal supports these types out of the box:
- Primitives: `int`, `float`, `bool`, `std::string`
- Containers: `std::vector`, `std::map`, `std::unordered_map`, `std::set`
- Smart pointers: `std::unique_ptr`, `std::shared_ptr`

### Custom Structures

Add a `serialize()` method:

```cpp
struct PlayerStats {
    int level;
    int experience;
    int gold;
    std::vector<std::string> skills;

    template<typename Archive>
    void serialize(Archive& ar) {
        ar(level, experience, gold, skills);
    }
};
```

---

### Nested Structures

```cpp
struct Inventory {
    std::vector<std::string> items;
    int maxSlots;

    template<typename Archive>
    void serialize(Archive& ar) {
        ar(items, maxSlots);
    }
};

struct PlayerData {
    std::string name;
    PlayerStats stats;
    Inventory inventory;

    template<typename Archive>
    void serialize(Archive& ar) {
        ar(name, stats, inventory);
    }
};
```

---

### Versioning

Include a version number for migration:

```cpp
struct SaveData {
    int saveVersion = 2;  // Increment on breaking changes

    // V1 fields
    Vec2 playerPosition;
    int health;

    // V2 fields (added later)
    std::optional<int> mana;  // Use optional for new fields

    template<typename Archive>
    void serialize(Archive& ar, const uint32_t version) {
        ar(playerPosition, health);

        if (version >= 2) {
            ar(mana);
        }
    }
};

// Cereal macro for versioning
CEREAL_CLASS_VERSION(SaveData, 2);
```

---

## Common Patterns

### Complete Game Save

```cpp
struct GameSaveData {
    // Version
    int version = 1;

    // Player state
    Vec2 playerPosition;
    int health;
    int maxHealth;
    int stamina;
    std::vector<std::string> inventory;

    // World state
    LevelId currentLevel;
    std::vector<LevelId> unlockedLevels;
    std::unordered_map<std::string, bool> flags;  // Quest flags, etc.

    // Progress
    int score;
    float playTime;
    std::vector<std::string> achievements;

    template<typename Archive>
    void serialize(Archive& ar) {
        ar(version,
           playerPosition, health, maxHealth, stamina, inventory,
           currentLevel, unlockedLevels, flags,
           score, playTime, achievements);
    }
};

void saveGame(SaveSlot slot) {
    GameSaveData data{
        .playerPosition = physics->getPosition(player),
        .health = entities->get<Health>(player).current,
        .maxHealth = entities->get<Health>(player).max,
        .stamina = entities->get<Stamina>(player).current,
        .inventory = getPlayerInventory(),
        .currentLevel = *levels->getActiveLevel(),
        .unlockedLevels = getUnlockedLevels(),
        .flags = gameState.flags,
        .score = gameState.score,
        .playTime = gameState.playTime,
        .achievements = gameState.achievements
    };

    auto result = save->saveGame(slot, data, "saves");
    if (result) {
        events->publish(Events::GameSaved, {});
        showNotification("Game Saved");
    } else {
        showError("Failed to save game");
    }
}

void loadGame(SaveSlot slot) {
    auto result = save->loadGame<GameSaveData>(slot, "saves");
    if (!result) {
        showError("Failed to load game");
        return;
    }

    GameSaveData& data = *result;

    // Restore player state
    physics->setPosition(player, data.playerPosition);
    entities->get<Health>(player) = {data.health, data.maxHealth};
    entities->get<Stamina>(player).current = data.stamina;
    setPlayerInventory(data.inventory);

    // Restore world state
    levels->transition({
        .fromLevel = *levels->getActiveLevel(),
        .toLevel = data.currentLevel,
        .unloadPrevious = true
    });
    gameState.unlockedLevels = data.unlockedLevels;
    gameState.flags = data.flags;

    // Restore progress
    gameState.score = data.score;
    gameState.playTime = data.playTime;
    gameState.achievements = data.achievements;

    events->publish(Events::GameLoaded, {});
}
```

---

### Auto-Save System

```cpp
class AutoSaveSystem {
    float timeSinceLastSave_ = 0.0f;
    const float autoSaveInterval_ = 60.0f;  // Every 60 seconds

public:
    void update(DeltaTime dt) {
        timeSinceLastSave_ += dt;

        if (timeSinceLastSave_ >= autoSaveInterval_) {
            autoSave();
            timeSinceLastSave_ = 0.0f;
        }
    }

    void autoSave() {
        GameSaveData data = captureGameState();
        auto result = save->saveGame(SaveSlots::AutoSave, data, "saves");

        if (result) {
            showNotification("Auto-saved", 2.0f);
        }
    }

    void onCheckpoint() {
        // Save immediately on checkpoint
        autoSave();
    }
};
```

---

### Save Slot Management

```cpp
struct SaveSlotInfo {
    SaveSlot slot;
    std::string timestamp;
    std::string levelName;
    int playerLevel;
    float playTime;
    bool exists;
};

std::vector<SaveSlotInfo> getSaveSlots() {
    std::vector<SaveSlotInfo> slots;

    for (SaveSlot slot = 0; slot < 10; ++slot) {
        auto result = save->loadGame<GameSaveData>(slot, "saves");

        if (result) {
            GameSaveData& data = *result;
            slots.push_back({
                .slot = slot,
                .timestamp = getFileTimestamp(slot),
                .levelName = getLevelName(data.currentLevel),
                .playerLevel = data.level,
                .playTime = data.playTime,
                .exists = true
            });
        } else {
            slots.push_back({
                .slot = slot,
                .exists = false
            });
        }
    }

    return slots;
}

void renderSaveMenu() {
    auto slots = getSaveSlots();

    for (const auto& info : slots) {
        if (info.exists) {
            drawText(std::format("Slot {}: {} - Level {} - {}h",
                info.slot,
                info.levelName,
                info.playerLevel,
                info.playTime / 3600.0f
            ));
        } else {
            drawText(std::format("Slot {}: Empty", info.slot));
        }
    }
}
```

---

### Migration Between Versions

```cpp
struct SaveDataV2 {
    int version = 2;
    Vec2 playerPosition;
    int health;
    int mana;  // New in V2

    template<typename Archive>
    void serialize(Archive& ar, const uint32_t version) {
        ar(playerPosition, health);

        if (version >= 2) {
            ar(mana);
        } else {
            // Migrate from V1: set default mana
            mana = 100;
        }
    }
};

CEREAL_CLASS_VERSION(SaveDataV2, 2);
```

---

### Settings Persistence

```cpp
struct GameSettings {
    // Video
    int resolutionWidth = 1280;
    int resolutionHeight = 720;
    bool fullscreen = false;
    bool vsync = true;

    // Audio
    float masterVolume = 1.0f;
    float musicVolume = 0.7f;
    float sfxVolume = 0.8f;

    // Controls
    std::unordered_map<std::string, int> keyBindings;

    template<typename Archive>
    void serialize(Archive& ar) {
        ar(resolutionWidth, resolutionHeight, fullscreen, vsync,
           masterVolume, musicVolume, sfxVolume,
           keyBindings);
    }
};

void saveSettings(const GameSettings& settings) {
    save->saveGame(0, settings, "config");
}

std::optional<GameSettings> loadSettings() {
    auto result = save->loadGame<GameSettings>(0, "config");
    if (result) {
        return *result;
    }
    return std::nullopt;
}

void applySettings(const GameSettings& settings) {
    graphics->setWindowSize({settings.resolutionWidth, settings.resolutionHeight});
    graphics->setFullscreen(settings.fullscreen);
    graphics->setVSync(settings.vsync);

    audio->setMasterVolume(settings.masterVolume);
    audio->setGroupVolume("Music", settings.musicVolume);
    audio->setGroupVolume("SFX", settings.sfxVolume);

    // Apply key bindings...
}
```

---

### Cloud Save Integration

```cpp
class CloudSaveSync {
public:
    void uploadSave(SaveSlot slot) {
        auto result = save->loadGame<GameSaveData>(slot, "saves");
        if (!result) return;

        // Serialize to JSON or binary for cloud
        std::string cloudData = serializeForCloud(*result);

        // Upload to cloud service
        cloudService->upload("save_" + std::to_string(slot), cloudData);
    }

    void downloadSave(SaveSlot slot) {
        auto cloudData = cloudService->download("save_" + std::to_string(slot));
        if (!cloudData) return;

        // Deserialize from cloud
        GameSaveData data = deserializeFromCloud(*cloudData);

        // Save locally
        save->saveGame(slot, data, "saves");
    }

    void syncAllSaves() {
        // Compare local and cloud timestamps
        // Upload newer saves, download missing saves
    }
};
```

---

## File Format

Save files are stored as:
- Binary format (cereal)
- Compressed (zstd)
- Checksummed (integrity validation)

**Filename format:** `save_<slot>.dat`

**Location:** `<directory>/save_<slot>.dat`

**Example paths:**
- `saves/save_0.dat` (Slot 1)
- `saves/save_1.dat` (Slot 2)
- `saves/save_4294967294.dat` (QuickSave)
- `saves/save_4294967295.dat` (AutoSave)

---

## Performance Tips

1. **Save asynchronously** - Don't block game loop during save
2. **Incremental saves** - Save only changed data for frequent auto-saves
3. **Compress large saves** - cereal + zstd handles this automatically
4. **Limit save frequency** - Don't save every frame
5. **Validate before save** - Check data integrity before writing

## See Also

- [LevelSystem](LevelSystem.md) - Saving current level
- [EntitySystem](EntitySystem.md) - Saving entity state
- [EventSystem](EventSystem.md) - Save/load events

# State System

> **Visibility:** Public (Lua + C++ API)
> **Tier:** 2
> **Dependencies:** Types, Events
> **Lua Paths:** `bestow.state` (high-level), `bestow.state.core` (low-level)

## Purpose

The State System provides key-value data persistence for save/load functionality, backed by SQLite with async commit and restore operations. Game state is stored in an in-memory cache with write-back semantics, supporting number, string, boolean, and JSON data types. Multiple save slots, player profiles, auto-save, playtime tracking, format migration, and JSON export/import for dev tools are all supported. The high-level API offers simple set/get/save/load operations, while the low-level API adds JSON data, slot metadata queries, profile management, playtime tracking, migration registration, and JSON export/import.

## High-Level API: `IStateSystem`

The simplified API for common save/load tasks. Set and get typed values by key, save to numbered slots, and quick-save/quick-load. No lifecycle methods -- the engine manages those internally.

### Key-Value Store

| Method | Returns | Description |
|--------|---------|-------------|
| `set(std::string_view key, double value)` | `void` | Store a numeric value under the given key |
| `set(std::string_view key, std::string_view value)` | `void` | Store a string value under the given key |
| `set(std::string_view key, bool value)` | `void` | Store a boolean value under the given key |
| `getNumber(std::string_view key, double def = 0)` | `double` | Retrieve a numeric value by key, returning the default if not found |
| `getString(std::string_view key, std::string_view def = "")` | `std::string` | Retrieve a string value by key, returning the default if not found |
| `getBool(std::string_view key, bool def = false)` | `bool` | Retrieve a boolean value by key, returning the default if not found |
| `has(std::string_view key)` | `bool` | Check whether a key exists in the state store |
| `remove(std::string_view key)` | `void` | Remove a key and its value from the state store |

### Save/Load

| Method | Returns | Description |
|--------|---------|-------------|
| `save(int slot, std::string_view name)` | `void` | Commit the current in-memory state to the given save slot with a display name |
| `load(int slot)` | `void` | Restore state from the given save slot into memory |
| `quickSave()` | `void` | Save to the reserved quick-save slot |
| `quickLoad()` | `void` | Load from the reserved quick-save slot |
| `slotExists(int slot)` | `bool` | Check whether the given save slot contains data |

## Low-Level API: `IStateCore`

Full control API. Exposes lifecycle, JSON data operations, async commit/restore with callbacks, slot deletion, auto-commit, slot metadata queries, profile management, playtime tracking, format migration, and JSON export/import for development tools.

### Lifecycle

| Method | Returns | Description |
|--------|---------|-------------|
| `update(DeltaTime dt)` | `void` | Process pending async operations and auto-commit timers |

### Key-Value Storage

| Method | Returns | Description |
|--------|---------|-------------|
| `setNumber(std::string_view key, double value)` | `void` | Store a numeric value |
| `getNumber(std::string_view key, double def = 0)` | `double` | Retrieve a numeric value with a default |
| `setString(std::string_view key, std::string_view value)` | `void` | Store a string value |
| `getString(std::string_view key, std::string_view def = "")` | `std::string` | Retrieve a string value with a default |
| `setBool(std::string_view key, bool value)` | `void` | Store a boolean value |
| `getBool(std::string_view key, bool def = false)` | `bool` | Retrieve a boolean value with a default |
| `setJson(std::string_view key, std::string_view json)` | `void` | Store a JSON-encoded string for complex nested data |
| `getJson(std::string_view key)` | `std::string` | Retrieve a JSON-encoded string by key |
| `hasData(std::string_view key)` | `bool` | Check whether a key exists |
| `removeData(std::string_view key)` | `void` | Remove a key-value pair |
| `clearData()` | `void` | Remove all key-value data from the in-memory store |

### Persistence

| Method | Returns | Description |
|--------|---------|-------------|
| `commit(int slot, std::string_view name, SaveCallback cb = {})` | `void` | Asynchronously commit the in-memory state to a save slot with an optional completion callback |
| `restore(int slot, SaveCallback cb = {})` | `void` | Asynchronously restore state from a save slot with an optional completion callback |
| `deleteSlot(int slot)` | `Result<void>` | Delete the data in a save slot |
| `quickCommit(SaveCallback cb = {})` | `void` | Commit to the quick-save slot |
| `quickRestore(SaveCallback cb = {})` | `void` | Restore from the quick-save slot |

### Auto-Commit

| Method | Returns | Description |
|--------|---------|-------------|
| `enableAutoCommit(float intervalSeconds)` | `void` | Enable automatic periodic commits at the given interval |
| `disableAutoCommit()` | `void` | Disable automatic periodic commits |

### Slot Queries

| Method | Returns | Description |
|--------|---------|-------------|
| `getAllSlotMetadata()` | `std::vector<StateMetadata>` | Return metadata for all occupied save slots |
| `getSlotMetadata(int slot)` | `std::optional<StateMetadata>` | Return metadata for a specific save slot, or nullopt if empty |
| `slotExists(int slot)` | `bool` | Check whether a save slot contains data |

### Profiles

| Method | Returns | Description |
|--------|---------|-------------|
| `setActiveProfile(std::string_view id)` | `void` | Switch to a different player profile; save data is scoped per profile |
| `getActiveProfile()` | `std::string` | Return the currently active profile identifier |
| `getProfiles()` | `std::vector<std::string>` | Return all available profile identifiers |

### Playtime Tracking

| Method | Returns | Description |
|--------|---------|-------------|
| `getSessionPlaytime()` | `std::uint64_t` | Return playtime for the current session in seconds |
| `getTotalPlaytime()` | `std::uint64_t` | Return total accumulated playtime across all sessions in seconds |

### Migration

| Method | Returns | Description |
|--------|---------|-------------|
| `setFormatVersion(int version)` | `void` | Set the current save format version; used to detect when migration is needed |
| `registerMigration(int fromVersion, int toVersion, std::function<void()> fn)` | `void` | Register a migration function that transforms data from one format version to another |

### JSON Export/Import (Dev Tools)

| Method | Returns | Description |
|--------|---------|-------------|
| `exportToJson(int slot, std::string_view path)` | `Result<void>` | Export a save slot to a human-readable JSON file at the given path |
| `importFromJson(int slot, std::string_view path)` | `Result<void>` | Import save data from a JSON file into the specified slot |

## Types

### StateMetadata

| Field | Type | Default | Description |
|-------|------|---------|-------------|
| `slot` | `StateSlot` | -- | The save slot identifier |
| `name` | `std::string` | `""` | Display name given when saving |
| `timestamp` | `std::chrono::system_clock::time_point` | -- | When the save was created |
| `gameVersion` | `std::string` | `""` | Game version string at the time of save |
| `playtimeSeconds` | `std::uint64_t` | `0` | Total playtime in seconds at the time of save |
| `completionPercentage` | `float` | `0.0f` | Game completion percentage (0.0 to 1.0) |
| `levelName` | `std::optional<std::string>` | `nullopt` | Name of the level/scene when the save was made |
| `formatVersion` | `int` | `1` | Save format version for migration detection |

### StateSlot

An integer identifying a save slot. Slot 0 is typically reserved for the quick-save feature. Positive integers (1, 2, 3, ...) are user-facing save slots.

### SaveCallback

```cpp
using SaveCallback = std::function<void(bool success, SystemError error)>;
```

A callback invoked when an asynchronous commit or restore operation completes. Receives a boolean success flag and a `SystemError` with details on failure.

## Lua Examples

```lua
-- High-level: Simple key-value state
bestow.state.set("player.health", 100)
bestow.state.set("player.name", "Hero")
bestow.state.set("tutorial.completed", true)

local health = bestow.state.getNumber("player.health", 0)
local name = bestow.state.getString("player.name", "Unknown")
local done = bestow.state.getBool("tutorial.completed", false)

if bestow.state.has("player.gold") then
    bestow.state.remove("player.gold")
end

-- Save and load
bestow.state.save(1, "Chapter 3 - Boss Fight")
bestow.state.load(1)

-- Quick save/load
bestow.state.quickSave()
bestow.state.quickLoad()

-- Check slots
if bestow.state.slotExists(1) then
    print("Slot 1 has data")
end

-- Low-level: JSON data for complex structures
bestow.state.core.setJson("inventory", '{"items":["sword","shield"],"gold":500}')
local inv = bestow.state.core.getJson("inventory")

-- Slot metadata
local slots = bestow.state.core.getAllSlotMetadata()
for _, meta in ipairs(slots) do
    print(meta.name .. " - " .. meta.playtimeSeconds .. "s")
end

-- Profiles
bestow.state.core.setActiveProfile("player2")
local profiles = bestow.state.core.getProfiles()

-- Auto-commit every 60 seconds
bestow.state.core.enableAutoCommit(60)

-- Migration
bestow.state.core.setFormatVersion(2)
bestow.state.core.registerMigration(1, 2, function()
    -- Rename old key to new key
    local old = bestow.state.core.getNumber("hp", 100)
    bestow.state.core.setNumber("player.health", old)
    bestow.state.core.removeData("hp")
end)
```

## C++ Examples

```cpp
// High-level: Simple state management
state->set("player.health", 100.0);
state->set("player.name", "Hero");
state->set("tutorial.completed", true);

double health = state->getNumber("player.health", 0.0);
std::string name = state->getString("player.name", "Unknown");
bool done = state->getBool("tutorial.completed", false);

state->save(1, "Chapter 3 - Boss Fight");
state->load(1);
state->quickSave();
state->quickLoad();

// Low-level: Async commit with callback
stateCore->commit(1, "Autosave",
    [](bool success, SystemError error) {
        if (!success) {
            spdlog::error("Save failed: {}", error.message);
        }
    });

// JSON data for complex nested structures
stateCore->setJson("inventory",
    R"({"items":["sword","shield"],"gold":500})");
auto json = stateCore->getJson("inventory");

// Slot metadata for save/load screen
auto slots = stateCore->getAllSlotMetadata();
for (const auto& meta : slots) {
    spdlog::info("Slot {}: {} ({} seconds played)",
        meta.slot, meta.name, meta.playtimeSeconds);
}

// Profile management
stateCore->setActiveProfile("player2");
auto profiles = stateCore->getProfiles();

// Auto-commit every 60 seconds
stateCore->enableAutoCommit(60.0f);

// Migration
stateCore->setFormatVersion(2);
stateCore->registerMigration(1, 2, [&]() {
    double old = stateCore->getNumber("hp", 100.0);
    stateCore->setNumber("player.health", old);
    stateCore->removeData("hp");
});

// JSON export for debugging
auto result = stateCore->exportToJson(1, "debug_save.json");
if (!result) spdlog::error("{}", result.error().message);
```

// bestow-luabind/src/docs/state_binding_doc.cpp
// API documentation for bestow.state

module bestow.luabind;

import std;

namespace bestow {

void registerStateDoc(DocRegistry& registry) {
    SystemDoc sys;
    sys.name = "state";
    sys.qualifiedName = "bestow.state";
    sys.description = "Key-value data persistence system with async SQLite commits. "
        "Data is stored in an in-memory cache (instant read/write) and persisted to "
        "disk via bestow.state.commit(). Supports multiple save slots, profiles, "
        "auto-commit, quick save/load, and schema migration.";

    //=========================================================================
    // Data Operations
    //=========================================================================

    sys.methods.push_back(MethodDoc{
        .name = "set",
        .qualifiedName = "bestow.state.set",
        .description = "Set a value in the in-memory cache. No disk I/O occurs. "
            "Supports number, string, boolean, table, and nil (which removes the key).",
        .params = {
            {.name = "key", .type = "string", .description = "Data key (e.g., \"player.health\")"},
            {.name = "value", .type = "number|string|boolean|table|nil", .description = "Value to store. nil removes the key."},
        },
        .example = "bestow.state.set(\"player.health\", 100)\n"
            "bestow.state.set(\"player.name\", \"Hero\")\n"
            "bestow.state.set(\"flags.bossDefeated\", true)\n"
            "bestow.state.set(\"inventory\", {\"sword\", \"shield\"})\n"
            "bestow.state.set(\"old_key\", nil)  -- removes key",
        .seeAlso = {"bestow.state.get"},
    });

    sys.methods.push_back(MethodDoc{
        .name = "get",
        .qualifiedName = "bestow.state.get",
        .description = "Get a value from the in-memory cache. Returns the default "
            "if the key doesn't exist. Automatically returns the correct Lua type.",
        .params = {
            {.name = "key", .type = "string", .description = "Data key to look up"},
            {.name = "default", .type = "any", .description = "Default value if key not found", .optional = true},
        },
        .returns = {{.type = "any", .description = "The stored value or the default"}},
        .example = "local health = bestow.state.get(\"player.health\", 100)\n"
            "local name = bestow.state.get(\"player.name\", \"Unknown\")\n"
            "local items = bestow.state.get(\"inventory\", {})",
        .seeAlso = {"bestow.state.set", "bestow.state.has"},
    });

    sys.methods.push_back(MethodDoc{
        .name = "has",
        .qualifiedName = "bestow.state.has",
        .description = "Check if a key exists in the in-memory cache.",
        .params = {
            {.name = "key", .type = "string", .description = "Data key to check"},
        },
        .returns = {{.type = "boolean", .description = "true if the key exists"}},
    });

    sys.methods.push_back(MethodDoc{
        .name = "remove",
        .qualifiedName = "bestow.state.remove",
        .description = "Remove a key from the in-memory cache.",
        .params = {
            {.name = "key", .type = "string", .description = "Data key to remove"},
        },
        .seeAlso = {"bestow.state.clear"},
    });

    sys.methods.push_back(MethodDoc{
        .name = "clear",
        .qualifiedName = "bestow.state.clear",
        .description = "Remove all data from the in-memory cache.",
        .seeAlso = {"bestow.state.remove"},
    });

    //=========================================================================
    // Commit/Restore
    //=========================================================================

    sys.methods.push_back(MethodDoc{
        .name = "commit",
        .qualifiedName = "bestow.state.commit",
        .description = "Persist the in-memory cache to a save slot. The write happens "
            "on a background thread. The optional callback fires on the next update() "
            "after completion.",
        .params = {
            {.name = "slot", .type = "number", .description = "Save slot index (0-based)"},
            {.name = "name", .type = "string", .description = "User-facing name for the save", .optional = true},
            {.name = "callback", .type = "function", .description = "Called with (success, err) on completion", .optional = true},
        },
        .example = "bestow.state.commit(0, \"Chapter 1 Complete\", function(success, err)\n"
            "    if success then\n"
            "        bestow.info(\"State committed!\")\n"
            "    else\n"
            "        bestow.error(\"Commit failed:\", err)\n"
            "    end\n"
            "end)",
        .seeAlso = {"bestow.state.restore"},
    });

    sys.methods.push_back(MethodDoc{
        .name = "restore",
        .qualifiedName = "bestow.state.restore",
        .description = "Load data from a save slot into the in-memory cache, replacing "
            "all current data. The read happens on a background thread.",
        .params = {
            {.name = "slot", .type = "number", .description = "Save slot index to restore from"},
            {.name = "callback", .type = "function", .description = "Called with (success, err) on completion", .optional = true},
        },
        .example = "bestow.state.restore(0, function(success, err)\n"
            "    if success then\n"
            "        local health = bestow.state.get(\"player.health\", 100)\n"
            "    end\n"
            "end)",
        .seeAlso = {"bestow.state.commit"},
    });

    sys.methods.push_back(MethodDoc{
        .name = "deleteSlot",
        .qualifiedName = "bestow.state.deleteSlot",
        .description = "Delete a save slot and all its data from the database.",
        .params = {
            {.name = "slot", .type = "number", .description = "Save slot index to delete"},
        },
        .returns = {{.type = "boolean", .description = "true if the slot was deleted"}},
    });

    //=========================================================================
    // Quick Commit/Restore
    //=========================================================================

    sys.methods.push_back(MethodDoc{
        .name = "quickCommit",
        .qualifiedName = "bestow.state.quickCommit",
        .description = "Quick-save to the reserved quick-commit slot.",
        .params = {
            {.name = "callback", .type = "function", .description = "Called with (success, err) on completion", .optional = true},
        },
        .seeAlso = {"bestow.state.quickRestore"},
    });

    sys.methods.push_back(MethodDoc{
        .name = "quickRestore",
        .qualifiedName = "bestow.state.quickRestore",
        .description = "Quick-load from the reserved quick-commit slot.",
        .params = {
            {.name = "callback", .type = "function", .description = "Called with (success, err) on completion", .optional = true},
        },
        .seeAlso = {"bestow.state.quickCommit"},
    });

    //=========================================================================
    // Auto-Commit
    //=========================================================================

    sys.methods.push_back(MethodDoc{
        .name = "enableAutoCommit",
        .qualifiedName = "bestow.state.enableAutoCommit",
        .description = "Enable periodic auto-commit at a fixed interval.",
        .params = {
            {.name = "seconds", .type = "number", .description = "Interval in seconds between auto-commits"},
        },
        .example = "bestow.state.enableAutoCommit(300)  -- every 5 minutes",
        .seeAlso = {"bestow.state.disableAutoCommit"},
    });

    sys.methods.push_back(MethodDoc{
        .name = "disableAutoCommit",
        .qualifiedName = "bestow.state.disableAutoCommit",
        .description = "Disable periodic auto-commit.",
        .seeAlso = {"bestow.state.enableAutoCommit"},
    });

    //=========================================================================
    // Metadata Queries
    //=========================================================================

    sys.methods.push_back(MethodDoc{
        .name = "getAllSlots",
        .qualifiedName = "bestow.state.getAllSlots",
        .description = "Get metadata for all save slots. Useful for building a save/load screen.",
        .returns = {{.type = "table[]", .description = "Array of {slot, name, playtime, version, completion, level, timestamp}"}},
        .example = "local slots = bestow.state.getAllSlots()\nfor _, meta in ipairs(slots) do\n    bestow.info(meta.name, meta.playtime .. \"s\")\nend",
    });

    sys.methods.push_back(MethodDoc{
        .name = "getSlotMetadata",
        .qualifiedName = "bestow.state.getSlotMetadata",
        .description = "Get metadata for a specific save slot.",
        .params = {
            {.name = "slot", .type = "number", .description = "Save slot index"},
        },
        .returns = {{.type = "table|nil", .description = "Metadata table or nil if slot is empty"}},
    });

    sys.methods.push_back(MethodDoc{
        .name = "slotExists",
        .qualifiedName = "bestow.state.slotExists",
        .description = "Check if a save slot has data.",
        .params = {
            {.name = "slot", .type = "number", .description = "Save slot index"},
        },
        .returns = {{.type = "boolean", .description = "true if the slot has saved data"}},
    });

    //=========================================================================
    // Profile Management
    //=========================================================================

    sys.methods.push_back(MethodDoc{
        .name = "setProfile",
        .qualifiedName = "bestow.state.setProfile",
        .description = "Set the active save profile. Each profile has its own database.",
        .params = {
            {.name = "profileId", .type = "string", .description = "Profile identifier"},
        },
        .seeAlso = {"bestow.state.getProfile", "bestow.state.getProfiles"},
    });

    sys.methods.push_back(MethodDoc{
        .name = "getProfile",
        .qualifiedName = "bestow.state.getProfile",
        .description = "Get the active profile identifier.",
        .returns = {{.type = "string", .description = "Active profile identifier"}},
    });

    sys.methods.push_back(MethodDoc{
        .name = "getProfiles",
        .qualifiedName = "bestow.state.getProfiles",
        .description = "Get all available profile identifiers.",
        .returns = {{.type = "string[]", .description = "Array of profile identifiers"}},
    });

    //=========================================================================
    // Game Tracking
    //=========================================================================

    sys.methods.push_back(MethodDoc{
        .name = "setGameVersion",
        .qualifiedName = "bestow.state.setGameVersion",
        .description = "Set the game version string stored in save metadata.",
        .params = {
            {.name = "version", .type = "string", .description = "Version string (e.g., \"1.2.0\")"},
        },
    });

    sys.methods.push_back(MethodDoc{
        .name = "setLevel",
        .qualifiedName = "bestow.state.setLevel",
        .description = "Set the current level name stored in save metadata.",
        .params = {
            {.name = "level", .type = "string", .description = "Level or scene name"},
        },
    });

    sys.methods.push_back(MethodDoc{
        .name = "setCompletion",
        .qualifiedName = "bestow.state.setCompletion",
        .description = "Set the game completion percentage stored in save metadata.",
        .params = {
            {.name = "percentage", .type = "number", .description = "Completion percentage (0.0 to 100.0)"},
        },
    });

    //=========================================================================
    // Migration
    //=========================================================================

    sys.methods.push_back(MethodDoc{
        .name = "setFormatVersion",
        .qualifiedName = "bestow.state.setFormatVersion",
        .description = "Set the current format version for new commits. When restoring "
            "older saves, registered migrations will run automatically.",
        .params = {
            {.name = "version", .type = "number", .description = "Format version integer"},
        },
        .seeAlso = {"bestow.state.registerMigration"},
    });

    sys.methods.push_back(MethodDoc{
        .name = "registerMigration",
        .qualifiedName = "bestow.state.registerMigration",
        .description = "Register a migration function that runs when restoring data "
            "from an older format version.",
        .params = {
            {.name = "fromVersion", .type = "number", .description = "Source format version"},
            {.name = "toVersion", .type = "number", .description = "Target format version"},
            {.name = "fn", .type = "function", .description = "Migration function (no arguments)"},
        },
        .example = "bestow.state.setFormatVersion(2)\n"
            "bestow.state.registerMigration(1, 2, function()\n"
            "    local old = bestow.state.get(\"player.hp\", 100)\n"
            "    bestow.state.set(\"player.health\", old)\n"
            "    bestow.state.remove(\"player.hp\")\n"
            "end)",
        .seeAlso = {"bestow.state.setFormatVersion"},
    });

    registry.addSystem(std::move(sys));
}

} // namespace bestow

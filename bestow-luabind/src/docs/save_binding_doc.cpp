// bestow-luabind/src/docs/save_binding_doc.cpp
// API documentation for bestow.save

module bestow.luabind;

import std;

namespace bestow {

void registerSaveDoc(DocRegistry& registry) {
    SystemDoc sys;
    sys.name = "save";
    sys.qualifiedName = "bestow.save";
    sys.description = "Save and load system for persisting game progress. Supports multiple save slots, quick save/load, auto-save, profile management, and playtime tracking with binary serialization.";

    //=========================================================================
    // Types
    //=========================================================================

    sys.types.push_back(TypeDoc{
        .name = "SaveMetadata",
        .qualifiedName = "SaveMetadata",
        .description = "Metadata describing a save file, including slot, timestamp, playtime, and progress information.",
        .fields = {
            {.name = "slot", .type = "number", .description = "Save slot index"},
            {.name = "saveName", .type = "string", .description = "User-facing name for the save"},
            {.name = "timestamp", .type = "number", .description = "Unix timestamp of when the save was created"},
            {.name = "gameVersion", .type = "string", .description = "Game version string at time of save"},
            {.name = "playtimeSeconds", .type = "number", .description = "Total playtime in seconds at time of save"},
            {.name = "completionPercentage", .type = "number", .description = "Game completion percentage (0.0 to 100.0)"},
            {.name = "levelName", .type = "string|nil", .description = "Name of the level/scene at time of save (optional)"},
            {.name = "hasScreenshot", .type = "boolean", .description = "Whether a screenshot is associated with this save"},
        },
    });

    //=========================================================================
    // Saveable Registration
    //=========================================================================

    sys.methods.push_back(MethodDoc{
        .name = "registerSaveable",
        .qualifiedName = "bestow.save.registerSaveable",
        .description = "Register a saveable object. The object must implement getSaveKey(), serialize(), and deserialize() methods. Registered objects are included when save() is called.",
        .params = {
            {.name = "saveable", .type = "table", .description = "A table with getSaveKey(), serialize(archive), and deserialize(archive) methods"},
        },
        .example = "local playerData = {\n    health = 100,\n    score = 0,\n    getSaveKey = function(self) return \"player\" end,\n    serialize = function(self, archive)\n        archive:writeInt(\"health\", self.health)\n        archive:writeInt(\"score\", self.score)\n    end,\n    deserialize = function(self, archive)\n        self.health = archive:readInt(\"health\")\n        self.score = archive:readInt(\"score\")\n    end,\n}\nbestow.save.registerSaveable(playerData)",
        .seeAlso = {"bestow.save.unregisterSaveable"},
    });

    sys.methods.push_back(MethodDoc{
        .name = "unregisterSaveable",
        .qualifiedName = "bestow.save.unregisterSaveable",
        .description = "Unregister a previously registered saveable object. It will no longer be included in save operations.",
        .params = {
            {.name = "saveable", .type = "table", .description = "The saveable object to unregister"},
        },
        .seeAlso = {"bestow.save.registerSaveable"},
    });

    //=========================================================================
    // Save/Load Operations
    //=========================================================================

    sys.methods.push_back(MethodDoc{
        .name = "save",
        .qualifiedName = "bestow.save.save",
        .description = "Save all registered saveable objects to a specific slot with a name.",
        .params = {
            {.name = "slot", .type = "number", .description = "Save slot index"},
            {.name = "saveName", .type = "string", .description = "User-facing name for the save (e.g., \"Chapter 3 - Boss Fight\")"},
        },
        .returns = {{.type = "boolean", .description = "true if the save succeeded"}},
        .example = "local ok = bestow.save.save(1, \"Before the boss\")\nif not ok then\n    print(\"Save failed!\")\nend",
        .seeAlso = {"bestow.save.load"},
    });

    sys.methods.push_back(MethodDoc{
        .name = "load",
        .qualifiedName = "bestow.save.load",
        .description = "Load game data from a specific save slot. Calls deserialize() on all registered saveable objects.",
        .params = {
            {.name = "slot", .type = "number", .description = "Save slot index to load from"},
        },
        .returns = {{.type = "boolean", .description = "true if the load succeeded"}},
        .seeAlso = {"bestow.save.save"},
    });

    sys.methods.push_back(MethodDoc{
        .name = "deleteSave",
        .qualifiedName = "bestow.save.deleteSave",
        .description = "Delete a save file from a specific slot.",
        .params = {
            {.name = "slot", .type = "number", .description = "Save slot index to delete"},
        },
        .returns = {{.type = "boolean", .description = "true if the save was deleted"}},
    });

    //=========================================================================
    // Quick Save/Load
    //=========================================================================

    sys.methods.push_back(MethodDoc{
        .name = "quickSave",
        .qualifiedName = "bestow.save.quickSave",
        .description = "Perform a quick save to a dedicated quick-save slot.",
        .example = "bestow.save.quickSave()",
        .seeAlso = {"bestow.save.quickLoad"},
    });

    sys.methods.push_back(MethodDoc{
        .name = "quickLoad",
        .qualifiedName = "bestow.save.quickLoad",
        .description = "Load from the dedicated quick-save slot.",
        .seeAlso = {"bestow.save.quickSave"},
    });

    //=========================================================================
    // Auto-Save
    //=========================================================================

    sys.methods.push_back(MethodDoc{
        .name = "autoSave",
        .qualifiedName = "bestow.save.autoSave",
        .description = "Trigger an auto-save immediately to the auto-save slot.",
    });

    sys.methods.push_back(MethodDoc{
        .name = "enableAutoSave",
        .qualifiedName = "bestow.save.enableAutoSave",
        .description = "Enable periodic auto-saving at a fixed interval.",
        .params = {
            {.name = "intervalSeconds", .type = "number", .description = "Time in seconds between auto-saves"},
        },
        .example = "-- Auto-save every 5 minutes\nbestow.save.enableAutoSave(300)",
        .seeAlso = {"bestow.save.disableAutoSave", "bestow.save.autoSave"},
    });

    sys.methods.push_back(MethodDoc{
        .name = "disableAutoSave",
        .qualifiedName = "bestow.save.disableAutoSave",
        .description = "Disable periodic auto-saving.",
        .seeAlso = {"bestow.save.enableAutoSave"},
    });

    //=========================================================================
    // Metadata Queries
    //=========================================================================

    sys.methods.push_back(MethodDoc{
        .name = "getAllSaveMetadata",
        .qualifiedName = "bestow.save.getAllSaveMetadata",
        .description = "Get metadata for all existing save files. Useful for building a save/load screen.",
        .returns = {{.type = "SaveMetadata[]", .description = "Array of metadata for all existing saves"}},
        .example = "local saves = bestow.save.getAllSaveMetadata()\nfor _, meta in ipairs(saves) do\n    print(meta.saveName, meta.playtimeSeconds .. \"s\")\nend",
    });

    sys.methods.push_back(MethodDoc{
        .name = "getSaveMetadata",
        .qualifiedName = "bestow.save.getSaveMetadata",
        .description = "Get metadata for a specific save slot.",
        .params = {
            {.name = "slot", .type = "number", .description = "Save slot index"},
        },
        .returns = {{.type = "SaveMetadata|nil", .description = "Metadata for the save, or nil if the slot is empty"}},
    });

    sys.methods.push_back(MethodDoc{
        .name = "saveExists",
        .qualifiedName = "bestow.save.saveExists",
        .description = "Check if a save file exists in a specific slot.",
        .params = {
            {.name = "slot", .type = "number", .description = "Save slot index to check"},
        },
        .returns = {{.type = "boolean", .description = "true if a save exists in the slot"}},
    });

    //=========================================================================
    // Profile Management
    //=========================================================================

    sys.methods.push_back(MethodDoc{
        .name = "setActiveProfile",
        .qualifiedName = "bestow.save.setActiveProfile",
        .description = "Set the active save profile. Each profile has its own set of save slots.",
        .params = {
            {.name = "profileId", .type = "string", .description = "Profile identifier"},
        },
        .seeAlso = {"bestow.save.getActiveProfile", "bestow.save.getProfiles"},
    });

    sys.methods.push_back(MethodDoc{
        .name = "getActiveProfile",
        .qualifiedName = "bestow.save.getActiveProfile",
        .description = "Get the currently active save profile identifier.",
        .returns = {{.type = "string", .description = "Active profile identifier"}},
    });

    sys.methods.push_back(MethodDoc{
        .name = "getProfiles",
        .qualifiedName = "bestow.save.getProfiles",
        .description = "Get all available profile identifiers.",
        .returns = {{.type = "string[]", .description = "Array of profile identifiers"}},
    });

    //=========================================================================
    // Game Version & Playtime Tracking
    //=========================================================================

    sys.methods.push_back(MethodDoc{
        .name = "setGameVersion",
        .qualifiedName = "bestow.save.setGameVersion",
        .description = "Set the game version string to include in save metadata. Useful for save migration and compatibility checks.",
        .params = {
            {.name = "version", .type = "string", .description = "Game version string (e.g., \"1.2.0\")"},
        },
        .seeAlso = {"bestow.save.getGameVersion"},
    });

    sys.methods.push_back(MethodDoc{
        .name = "getGameVersion",
        .qualifiedName = "bestow.save.getGameVersion",
        .description = "Get the current game version string.",
        .returns = {{.type = "string", .description = "Current game version string"}},
    });

    sys.methods.push_back(MethodDoc{
        .name = "getSessionPlaytime",
        .qualifiedName = "bestow.save.getSessionPlaytime",
        .description = "Get the current session playtime in seconds (time since the game was started or last reset).",
        .returns = {{.type = "number", .description = "Session playtime in seconds"}},
    });

    sys.methods.push_back(MethodDoc{
        .name = "getTotalPlaytime",
        .qualifiedName = "bestow.save.getTotalPlaytime",
        .description = "Get the total playtime in seconds (loaded from save + current session).",
        .returns = {{.type = "number", .description = "Total playtime in seconds"}},
    });

    sys.methods.push_back(MethodDoc{
        .name = "resetSessionPlaytime",
        .qualifiedName = "bestow.save.resetSessionPlaytime",
        .description = "Reset the session playtime counter. Call when starting a new game.",
    });

    sys.methods.push_back(MethodDoc{
        .name = "setCompletionPercentage",
        .qualifiedName = "bestow.save.setCompletionPercentage",
        .description = "Set the game completion percentage for save metadata.",
        .params = {
            {.name = "percentage", .type = "number", .description = "Completion percentage from 0.0 to 100.0"},
        },
    });

    sys.methods.push_back(MethodDoc{
        .name = "setCurrentLevel",
        .qualifiedName = "bestow.save.setCurrentLevel",
        .description = "Set the current level name for save metadata. This is stored when save() is called.",
        .params = {
            {.name = "levelName", .type = "string", .description = "Name of the current level or scene"},
        },
    });

    registry.addSystem(std::move(sys));
}

} // namespace bestow

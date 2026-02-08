-- Save system configuration (stub — save system not yet implemented)
-- This file establishes the config pattern so it's ready when save lands.

return {
    savePath        = "saves/",
    settingsFile    = "settings.json",      -- Human-readable (user prefs, control remaps)
    maxSlots        = 10,
    autoSave        = { enabled = true, intervalSeconds = 300 },
    compression     = true,                 -- zstd compression for binary saves
}

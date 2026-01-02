-- games/lua-demo/app.config.lua
-- Configuration for the ScriptManager
--
-- This file is loaded automatically when the game initializes.

return {
    -- Folders to ignore when discovering Lua scripts
    -- (relative to game root)
    ignore = {
        "assets",
        "build",
        ".git",
        "node_modules",
        "vendor"
    }
}

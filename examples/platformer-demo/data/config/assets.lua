-- config/assets.lua
-- Asset path definitions

return {
    textures = {
        player = "data/textures/player_spritesheet.png",
        coin = "data/textures/coinGold.png",
        enemy = "data/textures/enemyWalking_1.png",
        platform = "data/textures/block.png"
    },
    sounds = {
        jump = "data/audio/sfx/phaseJump1.ogg",
        coin = "data/audio/sfx/pepSound1.ogg",
        hurt = "data/audio/sfx/impactBell_heavy_000.ogg"
    },
    fonts = {
        main = "data/fonts/PressStart2P-Regular.ttf"
    },
    levels = {
        level1 = "data/levels/level1.lua"
    }
}

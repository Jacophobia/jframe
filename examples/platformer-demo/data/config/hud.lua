-- config/hud.lua
-- HUD configuration

return {
    font = {
        name = "main",    -- References assets.fonts.main
        size = 16         -- Font size in pixels
    },
    healthBar = {
        position = { x = 10, y = 10 },
        size = { width = 200, height = 20 },
        colors = {
            background = { r = 50, g = 50, b = 50, a = 255 },
            fill = { r = 0, g = 255, b = 0, a = 255 }
        }
    },
    score = {
        position = { x = 620, y = 10 },
        fontSize = 16,
        color = { r = 255, g = 215, b = 0, a = 255 },  -- Gold
        prefix = "SCORE: "
    },
    gameOver = {
        text = "GAME OVER",
        fontSize = 32,
        color = { r = 200, g = 0, b = 0, a = 255 }
    }
}

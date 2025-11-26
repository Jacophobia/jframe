-- data/blueprints/player.lua
-- Player entity blueprint

local C = require("blueprints.helpers")

return {
    id = "player",
    displayName = "Player",
    tags = { "player", "controllable" },

    components = {
        transform = {},  -- Position set at spawn time

        sprite = {
            textureAsset = "textures/player.png",
            sourceW = 32,
            sourceH = 48,
            layer = 10,
            anchor = { x = 0.5, y = 0.0 }
        },

        physics = C.dynamicBody({
            width = 24,
            height = 44,
            categoryBits = C.Layers.PLAYER,
            maskBits = C.Layers.TERRAIN + C.Layers.ENEMY + C.Layers.ITEM + C.Layers.TRIGGER
        }),

        health = {
            maxHealth = 100,
            invincibilityTime = 1.0
        },

        playerController = {
            moveSpeed = 200,
            jumpForce = 450,
            airControl = 0.3
        }
    }
}

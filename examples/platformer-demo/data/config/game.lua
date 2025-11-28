-- data/config/game.lua
-- Platformer Demo - Main Configuration Aggregator
-- Loads and combines all domain-specific config files
-- Uses the safe 'include' function provided by ConfigSystem

local assets = include("data/config/assets.lua")
local sprites = include("data/config/sprites.lua")
local animations = include("data/config/animations.lua")
local physics = include("data/config/physics.lua")
local audio = include("data/config/audio.lua")
local hud = include("data/config/hud.lua")
local input = include("data/config/input.lua")
local camera = include("data/config/camera.lua")
local player = include("data/config/player.lua")
local enemies = include("data/config/enemies.lua")
local items = include("data/config/items.lua")

return {
    assets = assets,
    sprites = sprites,
    animations = animations,
    physics = physics,
    audio = audio,
    hud = hud,
    input = input,
    camera = camera,
    player = player,
    enemy = enemies,
    coin = items.coin
}

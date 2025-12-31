-- games/snake3d/modules/effects.lua
-- Visual effects: food pop, screen shake, particles
--
-- Matches C++ effects from snake.game.cppm

local Constants = bestow.include("modules/constants")
local Grid = bestow.include("modules/grid")
local Log = bestow.include("modules/log")

local Effects = {}

local log = Log.category("Effects")

-- Initialize effects state
function Effects.init(game)
    game.foodPopEffect = {
        active = false,
        pos = {x = 0, y = 0, z = 0},
        time = 0,
        maxTime = Constants.FOOD_POP_DURATION
    }

    game.screenShakeOffset = {x = 0, y = 0, z = 0}
    game.screenShakeTime = 0
    game.screenShakeIntensity = 0

    log.debug("Effects system initialized")
end

-- Trigger food pop effect at position
function Effects.triggerFoodPop(game, pos)
    local worldPos = Grid.toWorld(game.gridSize, pos)

    game.foodPopEffect = {
        active = true,
        pos = {x = worldPos.x, y = worldPos.y, z = worldPos.z},
        time = 0,
        maxTime = Constants.FOOD_POP_DURATION
    }

    log.trace("Food pop triggered at (%.2f, %.2f, %.2f)",
        worldPos.x, worldPos.y, worldPos.z)
end

-- Trigger screen shake
function Effects.triggerScreenShake(game, intensity, duration)
    game.screenShakeIntensity = intensity
    game.screenShakeTime = duration

    log.trace("Screen shake triggered: intensity=%.2f, duration=%.2f",
        intensity, duration)
end

-- Update all effects
function Effects.update(game, dt)
    -- Update food pop effect
    if game.foodPopEffect.active then
        game.foodPopEffect.time = game.foodPopEffect.time + dt

        if game.foodPopEffect.time >= game.foodPopEffect.maxTime then
            game.foodPopEffect.active = false
            log.trace("Food pop effect ended")
        end
    end
end

-- Get food pop animation progress (0-1)
function Effects.getFoodPopProgress(game)
    if not game.foodPopEffect.active then
        return 1
    end
    return game.foodPopEffect.time / game.foodPopEffect.maxTime
end

-- Get food pop scale (1 -> 0 as it fades)
function Effects.getFoodPopScale(game)
    if not game.foodPopEffect.active then
        return 0
    end
    return 1 - (game.foodPopEffect.time / game.foodPopEffect.maxTime)
end

return Effects

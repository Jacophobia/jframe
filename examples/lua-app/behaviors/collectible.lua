-- behaviors/collectible.lua
-- Behavior for collectible items like coins

return {
    name = "collectible",

    init = function(entity, state)
        state.bobSpeed = 2.0
        state.bobHeight = 10.0
        state.spinSpeed = 180.0
        state.startY = entity:getPosition().y
        state.time = 0
        state.collected = false
    end,

    update = function(entity, state, dt)
        if state.collected then return end

        state.time = state.time + dt

        -- Bobbing animation
        local pos = entity:getPosition()
        local newY = state.startY + math.sin(state.time * state.bobSpeed) * state.bobHeight
        entity:setPosition(pos.x, newY)

        -- Spinning animation
        local rotation = entity:getRotation()
        entity:setRotation(rotation + state.spinSpeed * dt * (math.pi / 180))
    end,

    -- Called when player collects this item (from physics callback)
    onCollect = function(entity, state, collector)
        if state.collected then return end
        state.collected = true

        -- Play collection sound
        audio.playSound("sounds/coin.wav")

        -- Emit score event
        events.emit("scoreChanged", 10)

        -- Destroy after a short delay for visual feedback
        timer.after(0.1, function()
            entity:destroy()
        end)
    end
}

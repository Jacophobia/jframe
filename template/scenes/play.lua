-- Gameplay scene
-- Empty scaffold — add your gameplay logic here.
-- Escape / Start pushes the pause scene.

return {
    phase = "gameplay",

    enter = function(params)
        -- Subscribe to pause action
        bestow.events.subscribe("action:Pause", function()
            bestow.scene.push("pause")
        end)

        -- Set up your gameplay here:
        -- - Create entities
        -- - Set up camera
        -- - Load level data
    end,

    update = function(dt)
        -- Update your gameplay systems here:
        -- app.systems.movement.update(dt)
        -- app.systems.camera.update(dt)
        return true
    end,

    render = function()
        bestow.graphics3d.beginFrame()
        -- Entities with MeshRenderer are drawn automatically.
        -- Add any manual draw calls here.
        bestow.graphics3d.endFrame()
    end,

    exit = function()
        -- Clean up when leaving gameplay (popped or paused)
    end,
}

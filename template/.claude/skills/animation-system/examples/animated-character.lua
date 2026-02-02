-- Example: Animated Character Setup
-- Shows loading, playing, blending animations, and state machine pattern

-- systems/player_animation.lua
return {
    init = function()
        local self = app.systems.player_animation
        local state = app.main.state

        -- Quick setup: loadCharacter does model + skeleton + animator in one call
        local result = bestow.animation.loadCharacter(state.player, "models/character.gltf")
        state.skeleton = result.skeleton
        state.animator = result.animator

        -- Get available clips
        local clips = bestow.assets.getAnimationNames("models/character.gltf")
        -- e.g., {"idle", "walk", "run", "jump", "attack"}

        -- Play idle animation
        bestow.animation.play(state.animator, "idle", 0.2)

        -- Subscribe to animation events (footsteps, etc.)
        bestow.animation.subscribeToEvents(state.animator, app.systems.player_animation, "onAnimEvent")

        -- Subscribe to animation complete (for one-shots)
        bestow.animation.subscribeToComplete(state.animator, app.systems.player_animation, "onAnimComplete")
    end,

    update = function(dt)
        local self = app.systems.player_animation
        local state = app.main.state
        if not state.animator then return end

        -- Simple state machine: pick animation based on movement
        local groundInfo = bestow.physics3d.getCharacterGroundInfo(state.player)
        local grounded = groundInfo and groundInfo.grounded

        local moveX, moveZ = 0, 0
        if bestow.input.isActionActive("MoveLeft") then moveX = moveX - 1 end
        if bestow.input.isActionActive("MoveRight") then moveX = moveX + 1 end
        if bestow.input.isActionActive("MoveForward") then moveZ = moveZ - 1 end
        if bestow.input.isActionActive("MoveBack") then moveZ = moveZ + 1 end
        local moving = (moveX ~= 0 or moveZ ~= 0)
        local sprinting = bestow.input.isActionActive("Sprint")

        if not grounded then
            bestow.animation.play(state.animator, "jump", 0.15)
        elseif moving and sprinting then
            bestow.animation.play(state.animator, "run", 0.2)
        elseif moving then
            bestow.animation.play(state.animator, "walk", 0.2)
        else
            bestow.animation.play(state.animator, "idle", 0.3)
        end
    end,

    onAnimEvent = function(event)
        -- event.name is the animation event name (e.g., "footstep_left")
        if event.name == "footstep_left" or event.name == "footstep_right" then
            local pos = bestow.entity.getField(app.main.state.player, "Transform3D", "position")
            bestow.audio.playPositional({
                asset = app.main.state.sounds.footstep,
                position = pos,
                volume = 0.5,
                minDistance = 1.0,
                maxDistance = 15.0
            })
        end
    end,

    onAnimComplete = function(event)
        -- Called when a non-looping animation finishes
        if event.clipName == "attack" then
            bestow.animation.play(app.main.state.animator, "idle", 0.2)
        end
    end
}

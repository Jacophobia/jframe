-- games/lua-demo/main.lua
-- Entry point for the Lua Demo Game
--
-- This demonstrates the Lua-driven approach where games are defined
-- entirely in Lua and run via: bestow run main.lua

-- Use the namespace pattern for game configuration
return {
    title = "Lua Demo Game",
    width = 1280,
    height = 720,
    vsync = true,

    -- Initialize the game
    init = function()
        print("[LuaDemo] Initializing game...")

        -- Access game systems through app.* namespace
        local player = app.entities.player
        local camera = app.systems.camera

        -- Create player entity using the entity system
        -- bestow.entity is the C++ IEntitySystem bound to Lua
        local playerEntity = bestow.entity.create()
        bestow.entity.addComponent(playerEntity, "Transform3D", {
            position = Vec3.new(0, 0, 0),
            rotation = Quat.identity(),
            scale = Vec3.new(1, 1, 1)
        })

        -- Store reference in game state
        app.main.state = {
            player = playerEntity,
            score = 0,
            running = true
        }

        -- Initialize systems
        if camera and camera.init then
            camera.init()
        end

        print("[LuaDemo] Game initialized!")
    end,

    -- Update game logic each frame
    update = function(dt)
        local state = app.main.state
        if not state or not state.running then
            return false  -- Stop the game loop
        end

        -- Get input state
        local movement = Vec3.new(0, 0, 0)

        -- Dvorak-friendly controls: ,AOE instead of WASD
        if bestow.input.isKeyDown(Keys.Comma) or bestow.input.isKeyDown(Keys.W) then
            movement.z = movement.z - 1  -- Forward
        end
        if bestow.input.isKeyDown(Keys.O) or bestow.input.isKeyDown(Keys.S) then
            movement.z = movement.z + 1  -- Backward
        end
        if bestow.input.isKeyDown(Keys.A) then
            movement.x = movement.x - 1  -- Left
        end
        if bestow.input.isKeyDown(Keys.E) or bestow.input.isKeyDown(Keys.D) then
            movement.x = movement.x + 1  -- Right
        end

        -- Apply movement to player
        if movement:length() > 0 then
            movement = movement:normalize() * 5.0 * dt

            local transform = bestow.entity.getComponent(state.player, "Transform3D")
            if transform then
                local newPos = transform.position + movement
                bestow.entity.setField(state.player, "Transform3D", "position", newPos)
            end
        end

        -- Check for quit
        if bestow.input.isActionActive("quit") or bestow.input.wasKeyJustPressed(Keys.Escape) then
            state.running = false
        end

        -- Update game systems
        local movement_system = app.systems.movement
        if movement_system and movement_system.update then
            movement_system.update(dt)
        end

        return state.running
    end,

    -- Render the game
    render = function()
        -- Clear and begin frame
        bestow.graphics3d.beginFrame()

        -- Set up camera using Camera3D struct
        local cameraSys = app.systems.camera
        if cameraSys and cameraSys.getCamera then
            bestow.graphics3d.setCamera(cameraSys.getCamera())
        end

        -- Render all visible entities
        bestow.entity.each(function(entity)
            if bestow.entity.hasComponent(entity, "MeshRenderer") then
                local transform = bestow.entity.getComponent(entity, "Transform3D")
                local renderer = bestow.entity.getComponent(entity, "MeshRenderer")
                if transform and renderer then
                    -- drawMesh takes Transform3D directly, not separate pos/rot/scale
                    bestow.graphics3d.drawMesh(
                        renderer.mesh,
                        renderer.material,
                        transform
                    )
                end
            end
        end)

        -- End frame
        bestow.graphics3d.endFrame()
    end,

    -- Main game loop
    run = function()
        local main = app.main

        -- Initialize
        main.init()

        -- Game loop
        while true do
            local dt = bestow.core.deltaTime()

            -- Update - returns false to exit
            if not main.update(dt) then
                break
            end

            -- Render
            main.render()
        end

        print("[LuaDemo] Game finished!")
    end
}

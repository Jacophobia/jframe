--[[
    Character Animation Demo

    A Lua-based game demonstrating user-controlled character movement with
    smooth animation state transitions.

    State Machine:
      idle <-> walking <-> running (based on movement speed)
      any grounded state -> jumping (on space press)
      jumping -> falling -> landing -> landing-recovery -> idle/walking

    Controls (Dvorak layout):
      ,AOE - Movement (forward/left/back/right)
      Shift - Run (hold)
      Space - Jump
      Escape - Quit

    Structure:
      main.lua     - App entry point (this file)
      inputs.lua   - Input action definitions
      config.lua   - Animation and movement settings
      player.lua   - Player state and animation logic
      camera.lua   - Follow camera

    State Pattern:
      - self.foo = saved to disk
      - self.transient.foo = not saved (runtime only)
]]

return {
    -- Metadata (read by engine)
    title = "Character Animation Demo - Lua",
    version = "1.0.0",

    -- Called once when game starts
    init = function(self, scope)
        -- Use app.main as self (GameRunner doesn't pass self correctly)
        local this = app.main

        bestow.info("Character Animation Demo initializing...")

        bestow.info("DEBUG: Registering input actions...")
        -- Register input actions (the inputs.lua file returns a function)
        app.inputs()

        bestow.info("DEBUG: Input actions registered")

        -- Initialize transient state table
        this.transient = this.transient or {}
        this.transient.running = true

        -- Load configuration
        local config = app.config

        bestow.info("DEBUG: Initializing graphics...")

        -- Initialize graphics
        bestow.graphics3d.initialize({
            windowWidth = config.window.width,
            windowHeight = config.window.height,
            windowTitle = this.title,
            vsync = true,
            fullscreen = false
        })
        bestow.graphics3d.setClearColor(Color.new(100, 140, 180, 255))

        -- Initialize input
        local windowHandle = bestow.graphics3d.getNativeWindowHandle()
        if windowHandle then
            bestow.input.initialize(windowHandle)
        end

        -- Setup lighting for outdoor scene
        bestow.graphics3d.setDirectionalLight({
            direction = Vec3.new(0.5, -1.0, 0.3),
            color = Vec3.new(1.0, 0.95, 0.9),
            intensity = 1.2
        })

        -- Create checker pattern ground
        local tileSize = 2.0  -- Size of each checker tile
        local gridSize = 25   -- Number of tiles in each direction (50x50 = 100 units total)
        this.transient.groundMesh = bestow.graphics3d.createPlaneMesh(tileSize, tileSize, 1, 1)

        -- Create two materials for checker pattern (light gray and dark gray)
        local lightMat = PBRMaterial.new()
        lightMat.baseColorFactor = Vec4.new(0.7, 0.7, 0.7, 1.0)
        lightMat.roughnessFactor = 0.9
        lightMat.metallicFactor = 0.0
        this.transient.lightMaterial = bestow.graphics3d.createMaterial(lightMat)

        local darkMat = PBRMaterial.new()
        darkMat.baseColorFactor = Vec4.new(0.3, 0.3, 0.3, 1.0)
        darkMat.roughnessFactor = 0.9
        darkMat.metallicFactor = 0.0
        this.transient.darkMaterial = bestow.graphics3d.createMaterial(darkMat)

        -- Store grid params for rendering
        this.transient.groundTileSize = tileSize
        this.transient.groundGridSize = gridSize

        -- Create ground plane physics body for collision
        this.transient.groundEntity = bestow.entity.create()
        local groundDef = PhysicsBodyDef3D.new()
        groundDef.type = BodyType3D.Static
        groundDef.shapeType = ShapeType3D.Box
        groundDef.shapeHalfExtents = Vec3.new(50.0, 0.5, 50.0)
        groundDef.transform = Transform3D.identity()
        groundDef.transform.position = Vec3.new(0, -0.5, 0)
        bestow.physics3d.createBody(this.transient.groundEntity, groundDef)
        bestow.info("Ground physics body created")

        -- Create player
        this.transient.player = app.player.create(this, scope)

        -- Create camera
        this.transient.camera = app.camera.create(this, scope)

        -- Subscribe to Jump action (hot-reload-safe: table + method name)
        this.transient.jumpSub = bestow.events.subscribe("Jump", {}, this, "onJump")

        -- Subscribe to Quit action
        this.transient.quitSub = bestow.events.subscribe("Quit", {}, this, "onQuit")

        -- Set initial phase for input system
        bestow.phase.change("game.playing")

        bestow.info("Initialization complete - use ,AOE to move, Shift to run, Space to jump")
    end,

    -- Called every frame (GameRunner passes dt as first argument)
    update = function(dt)
        local this = app.main

        -- Check if we should quit (window close button or Escape key)
        if bestow.graphics3d.shouldClose() then
            return false  -- Signal to stop game loop
        end

        if not this.transient or not this.transient.running then
            return false  -- Signal to stop game loop
        end

        local player = app.player
        local camera = app.camera

        -- Update input system
        bestow.input.update()

        -- Update physics simulation
        bestow.physics3d.update(dt)

        -- Update player (handles input and animation state machine)
        player.update(this, dt, nil)

        -- Update camera to follow player
        camera.update(this, dt, nil)

        -- Render
        bestow.graphics3d.beginFrame()

        -- Draw checker pattern ground
        local tileSize = this.transient.groundTileSize
        local gridSize = this.transient.groundGridSize
        local halfGrid = gridSize / 2
        for x = -halfGrid, halfGrid - 1 do
            for z = -halfGrid, halfGrid - 1 do
                local tileTransform = Transform3D.identity()
                tileTransform.position = Vec3.new(
                    (x + 0.5) * tileSize,
                    0,
                    (z + 0.5) * tileSize
                )
                -- Alternate colors based on x+z parity
                local mat = ((x + z) % 2 == 0) and this.transient.lightMaterial or this.transient.darkMaterial
                bestow.graphics3d.drawMesh(this.transient.groundMesh, mat, tileTransform)
            end
        end

        -- Draw player
        player.render(this, nil)

        bestow.graphics3d.endFrame()
    end,

    -- Called when app is destroyed
    destroy = function(self, scope)
        local this = app.main
        bestow.info("Shutting down...")

        -- Clean up all subscriptions and timers owned by this app
        bestow.events.unsubscribeAll(this)
        bestow.timer.cancelFor(this)

        -- Cleanup player
        app.player.destroy(this, scope)

        -- Cleanup ground physics entity
        if this.transient and this.transient.groundEntity then
            bestow.entity.destroy(this.transient.groundEntity)
        end

        -- Cleanup graphics
        bestow.graphics3d.shutdown()

        bestow.info("Shutdown complete")
    end,

    -- Event handlers (hot-reload-safe: looked up by name)
    onJump = function(self, event, scope)
        local this = app.main
        local p = this.transient and this.transient.player
        if p and p.isGrounded then
            app.player.jump(this, scope)
        end
    end,

    onQuit = function(self, event, scope)
        local this = app.main
        bestow.info("Quit requested")
        if this.transient then
            this.transient.running = false
        end
    end,

    -- Animation event handler for footsteps
    onFootstep = function(self, event)
        -- event contains: name, animation, time, data
        -- data.foot will be "left" or "right"
        local foot = event.data and event.data.foot or "unknown"
        bestow.debug("Footstep:", foot, "during", event.animation)

        -- Could play footstep sounds here:
        -- if foot == "left" then
        --     bestow.audio.play(":assets:/sounds/footstep_left.wav")
        -- else
        --     bestow.audio.play(":assets:/sounds/footstep_right.wav")
        -- end
    end,
}

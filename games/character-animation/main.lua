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
    title = "Character Animation Demo",
    version = "1.0.0",

    -- Called once when game starts
    init = function(self, scope)
        bestow.info("Character Animation Demo initializing...")

        -- Register input actions (the inputs.lua file returns a function)
        app.inputs()

        -- Transient app state
        self.transient.running = true

        -- Load configuration
        local config = app.config

        -- Initialize graphics
        bestow.graphics3d.initialize({
            windowWidth = config.window.width,
            windowHeight = config.window.height,
            windowTitle = self.title,
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

        -- Create ground plane
        self.transient.groundMesh = bestow.graphics3d.createPlaneMesh(100.0, 100.0, 1, 1)

        -- Create player
        self.transient.player = app.player.create(self, scope)

        -- Create camera
        self.transient.camera = app.camera.create(self, scope)

        -- Subscribe to Jump action (hot-reload-safe: table + method name)
        self.transient.jumpSub = bestow.events.subscribe("Jump", {}, self, "onJump")

        -- Subscribe to Quit action
        self.transient.quitSub = bestow.events.subscribe("Quit", {}, self, "onQuit")

        -- Set initial phase for input system
        bestow.phase.change("game.playing")

        bestow.info("Initialization complete - use ,AOE to move, Shift to run, Space to jump")
    end,

    -- Called every frame
    update = function(self, dt, scope)
        if not self.transient.running then
            return
        end

        local player = app.player
        local camera = app.camera

        -- Update input system
        bestow.input.update()

        -- Update player (handles input and animation state machine)
        player.update(self, dt, scope)

        -- Update camera to follow player
        camera.update(self, dt, scope)

        -- Render
        bestow.graphics3d.beginFrame()

        -- Draw ground with grid texture
        local groundTransform = Mat4.translation(0, 0, 0)
        bestow.graphics3d.drawMesh(self.transient.groundMesh, groundTransform, {
            color = Color.new(80, 120, 80, 255)
        })

        -- Draw player
        player.render(self, scope)

        bestow.graphics3d.endFrame()
    end,

    -- Called when app is destroyed
    destroy = function(self, scope)
        bestow.info("Shutting down...")

        -- Clean up all subscriptions and timers owned by this app
        bestow.events.unsubscribeAll(self)
        bestow.timer.cancelFor(self)

        -- Cleanup player
        app.player.destroy(self, scope)

        -- Cleanup graphics
        bestow.graphics3d.shutdown()

        bestow.info("Shutdown complete")
    end,

    -- Event handlers (hot-reload-safe: looked up by name)
    onJump = function(self, event, scope)
        local p = self.transient.player
        if p and p.isGrounded then
            app.player.jump(self, scope)
        end
    end,

    onQuit = function(self, event, scope)
        self.transient.running = false
        bestow.quit()
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

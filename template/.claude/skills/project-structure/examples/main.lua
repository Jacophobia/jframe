-- Example: Complete main.lua Template
-- Shows full game entry point with init, update, render, and system orchestration

return {
    title = "My Bestow Game",
    width = 1280,
    height = 720,

    init = function()
        -- Initialize persistent game state
        app.main.state = {
            player = nil,
            score = 0,
            lives = 3,
            running = true,
            currentLevel = nil
        }

        -- Set up input with Action Builder (Dvorak + QWERTY)
        local k = KeyCode
        for _, key in ipairs({k.Comma, k.W}) do
            bestow.action.builder():duringPhase("gameplay"):whenActive(key):emitAction("MoveForward"):continuously()
        end
        for _, key in ipairs({k.O, k.S}) do
            bestow.action.builder():duringPhase("gameplay"):whenActive(key):emitAction("MoveBack"):continuously()
        end
        bestow.action.builder():duringPhase("gameplay"):whenActive(k.A):emitAction("MoveLeft"):continuously()
        for _, key in ipairs({k.E, k.D}) do
            bestow.action.builder():duringPhase("gameplay"):whenActive(key):emitAction("MoveRight"):continuously()
        end
        bestow.action.builder():duringPhase("gameplay"):whenPressed(k.Space):emitAction("Jump"):discretely()
        bestow.action.builder():duringPhase("gameplay"):whenPressed(k.Escape):emitAction("Pause"):discretely()

        -- Set up camera
        bestow.graphics3d.setCamera({
            position = Vec3.new(0, 10, -20),
            rotation = Quat.lookAt(Vec3.new(0, -0.4, 1):normalize(), Vec3.up()),
            fov = 45.0,
            near = 0.1,
            far = 1000.0
        })

        -- Set up lighting
        bestow.graphics3d.setAmbientLight(Color.new(0.2, 0.2, 0.3, 1.0), 0.3)
        bestow.graphics3d.setDirectionalLight({
            direction = Vec3.new(-0.5, -1, -0.5):normalize(),
            color = Color.new(1.0, 0.95, 0.8, 1.0),
            intensity = 1.0
        })

        -- Initialize UI
        bestow.ui.initialize()

        -- Load first level
        bestow.input.pushPhase("gameplay")
        app.levels.level1.load()
    end,

    update = function(dt)
        local state = app.main.state

        -- Update timers (required every frame)
        bestow.timer.update(dt)

        -- Update game systems
        app.systems.player.update(dt)
        app.systems.enemy_ai.update(dt)
        app.systems.collectibles.update(dt)
        app.systems.camera.update(dt)

        -- Update UI
        bestow.ui.processInput()

        return state.running  -- Return false to quit
    end,

    render = function()
        bestow.graphics3d.beginFrame()
        -- Entities with MeshRenderer are drawn automatically
        bestow.ui.render()
        bestow.graphics3d.endFrame()
    end,

    shutdown = function()
        bestow.ui.shutdown()
    end
}

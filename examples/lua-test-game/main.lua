-- examples/lua-test-game/main.lua
-- Simple test game to validate the Lua runtime prototype
--
-- Run with: bestow examples/lua-test-game/main.lua

-- Game configuration and state
game = {
    title = "Lua Test Game",
    window = {
        width = 1280,
        height = 720
    },

    -- Game state
    frameCount = 0,
    entities = {},
    score = 0,
}

-- Called once at startup
function game:init()
    print("=== Lua Test Game Initializing ===")
    print("This demonstrates the Bestow Lua API")
    print("")

    -- When full engine is integrated, we'd create entities like this:
    -- self.player = game:createEntity()
    --     :at(400, 300)
    --     :withTag("player")
    --     :addComponent("Velocity", {x = 0, y = 0})

    print("Game initialized!")
    print("Window size: " .. self.window.width .. "x" .. self.window.height)
end

-- Called every frame
function game:update(dt)
    self.frameCount = self.frameCount + 1

    -- Log every 60 frames (roughly every second at 60 FPS)
    if self.frameCount % 60 == 0 then
        print("Frame: " .. self.frameCount .. " | dt: " .. string.format("%.4f", dt))
    end

    -- Simulate some game logic
    if self.frameCount == 1 then
        print("First frame - spawning test entity...")
        -- self.testEntity = game:createEntity():at(100, 100):withTag("test")
    end

    if self.frameCount == 3 then
        print("Frame 3 - updating score...")
        self.score = self.score + 100
        print("Score: " .. self.score)
    end
end

-- Called when game shuts down
function game:shutdown()
    print("")
    print("=== Lua Test Game Shutting Down ===")
    print("Total frames: " .. self.frameCount)
    print("Final score: " .. self.score)
    print("Goodbye!")
end

print("main.lua loaded successfully!")

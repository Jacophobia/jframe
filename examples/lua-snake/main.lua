-- examples/lua-snake/main.lua
-- Snake game written entirely in Lua for Bestow
--
-- This demonstrates what a complete Lua-first game would look like.
-- Currently a design document - will work when full engine integration is complete.
--
-- Run with: bestow examples/lua-snake/main.lua

local Vec2 = Vec2 or function(x, y) return {x = x, y = y} end

-- Game configuration
game = {
    title = "Snake - Lua Edition",
    window = {
        width = 800,
        height = 600
    },
    renderer = "vulkan",

    -- Game constants
    gridSize = 20,
    moveInterval = 0.12,

    -- Game state
    snake = {},
    direction = nil,
    nextDirection = nil,
    moveTimer = 0,
    score = 0,
    food = nil,
    gameOver = false,
}

-- Initialize the game
function game:init()
    print("=== Snake - Lua Edition ===")
    print("Controls: ,AOE (Dvorak) or Arrow Keys")
    print("")

    -- Initialize direction
    self.direction = Vec2(1, 0)      -- Moving right
    self.nextDirection = Vec2(1, 0)

    -- Bind input actions (Dvorak-friendly!)
    -- When engine is integrated:
    -- self.input:bindAction("up", {"comma", "up", "gamepad_dpad_up"})
    -- self.input:bindAction("down", {"o", "down", "gamepad_dpad_down"})
    -- self.input:bindAction("left", {"a", "left", "gamepad_dpad_left"})
    -- self.input:bindAction("right", {"e", "right", "gamepad_dpad_right"})

    -- Create initial snake
    self:createSnake()

    -- Spawn first food
    self:spawnFood()

    print("Snake initialized with " .. #self.snake .. " segments")
end

function game:createSnake()
    local startX = 400
    local startY = 300

    -- Create head
    -- self.snake[1] = game:createEntity()
    --     :at(startX, startY)
    --     :withSprite("textures/snake_head.png")
    --     :withCollider("box", {width = self.gridSize, height = self.gridSize})
    --     :withTag("snake_head")

    -- For prototype, just track positions
    self.snake[1] = {x = startX, y = startY, isHead = true}

    -- Create initial body segments
    for i = 2, 3 do
        local segment = {
            x = startX - (i - 1) * self.gridSize,
            y = startY,
            isHead = false
        }
        self.snake[i] = segment
    end
end

function game:spawnFood()
    local maxX = math.floor(self.window.width / self.gridSize) - 2
    local maxY = math.floor(self.window.height / self.gridSize) - 2

    local foodX = math.random(2, maxX) * self.gridSize
    local foodY = math.random(2, maxY) * self.gridSize

    -- When engine is integrated:
    -- if self.food then
    --     self.food:destroy()
    -- end
    -- self.food = game:createEntity()
    --     :at(foodX, foodY)
    --     :withSprite("textures/food.png")
    --     :withCollider("box", {width = self.gridSize, height = self.gridSize})
    --     :withTag("food")

    self.food = {x = foodX, y = foodY}
    print("Food spawned at " .. foodX .. ", " .. foodY)
end

function game:addSegment()
    local tail = self.snake[#self.snake]

    -- When engine is integrated:
    -- local segment = game:createEntity()
    --     :at(tail.x, tail.y)
    --     :withSprite("textures/snake_body.png")
    --     :withTag("snake_body")

    local segment = {x = tail.x, y = tail.y, isHead = false}
    table.insert(self.snake, segment)
    print("Snake grew! Length: " .. #self.snake)
end

-- Called every frame
function game:update(dt)
    if self.gameOver then
        return
    end

    -- Handle input (buffer direction changes)
    -- When engine is integrated:
    -- if self.input:justPressed("up") and self.direction.y == 0 then
    --     self.nextDirection = Vec2(0, -1)
    -- elseif self.input:justPressed("down") and self.direction.y == 0 then
    --     self.nextDirection = Vec2(0, 1)
    -- elseif self.input:justPressed("left") and self.direction.x == 0 then
    --     self.nextDirection = Vec2(-1, 0)
    -- elseif self.input:justPressed("right") and self.direction.x == 0 then
    --     self.nextDirection = Vec2(1, 0)
    -- end

    -- Movement timer
    self.moveTimer = self.moveTimer + dt
    if self.moveTimer >= self.moveInterval then
        self.moveTimer = 0
        self.direction = self.nextDirection
        self:moveSnake()
    end
end

function game:moveSnake()
    local head = self.snake[1]
    local newX = head.x + self.direction.x * self.gridSize
    local newY = head.y + self.direction.y * self.gridSize

    -- Move body segments (from tail to head)
    for i = #self.snake, 2, -1 do
        self.snake[i].x = self.snake[i - 1].x
        self.snake[i].y = self.snake[i - 1].y
    end

    -- Move head
    head.x = newX
    head.y = newY

    -- Wrap around screen
    if head.x < 0 then head.x = self.window.width - self.gridSize end
    if head.x >= self.window.width then head.x = 0 end
    if head.y < 0 then head.y = self.window.height - self.gridSize end
    if head.y >= self.window.height then head.y = 0 end

    -- Check food collision
    if self.food and head.x == self.food.x and head.y == self.food.y then
        self:eatFood()
    end

    -- Check self collision
    for i = 2, #self.snake do
        if head.x == self.snake[i].x and head.y == self.snake[i].y then
            self:die()
            return
        end
    end
end

function game:eatFood()
    self.score = self.score + 10
    print("Yum! Score: " .. self.score)

    -- When engine is integrated:
    -- self.audio:play("sounds/eat.wav")

    self:addSegment()
    self:spawnFood()

    -- Speed up slightly
    self.moveInterval = math.max(0.05, self.moveInterval - 0.005)
end

function game:die()
    self.gameOver = true
    print("")
    print("=== GAME OVER ===")
    print("Final Score: " .. self.score)
    print("Snake Length: " .. #self.snake)

    -- When engine is integrated:
    -- self.audio:play("sounds/game_over.wav")
end

-- Collision callback (when engine is integrated)
-- game:on("collision", function(a, b)
--     if a:hasTag("snake_head") and b:hasTag("food") then
--         game:eatFood()
--     end
-- end)

function game:shutdown()
    print("")
    print("Thanks for playing Snake - Lua Edition!")
    print("Final Score: " .. self.score)
end

print("Snake game loaded!")
print("This is a design preview - full functionality requires engine integration.")

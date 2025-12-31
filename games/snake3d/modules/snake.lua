-- games/snake3d/modules/snake.lua
-- Snake movement, collision detection, and growth logic
--
-- Matches C++ snake logic from snake.game.cppm

local Constants = bestow.include("modules/constants")
local GamePhase = bestow.include("modules/phases")
local Direction = bestow.include("modules/direction")
local Grid = bestow.include("modules/grid")
local Audio = bestow.include("modules/audio")
local Log = bestow.include("modules/log")

local Snake = {}

local log = Log.category("Snake")

-- Initialize snake state
function Snake.init(game)
    log.info("Initializing snake")

    game.snake = {}
    game.direction = Direction.Right
    game.nextDirection = Direction.Right
    game.inputDirection = Direction.Right
    game.hasBufferedInput = false
    game.moveTimer = 0
    game.moveInterval = Constants.INITIAL_MOVE_INTERVAL
    game.detachedSegments = {}

    -- Create initial snake (3 segments in center)
    local centerX = math.floor(game.gridSize / 2)
    local centerZ = math.floor(game.gridSize / 2)

    -- Head
    table.insert(game.snake, {
        pos = {x = centerX, z = centerZ},
        color = Constants.COLORS.SNAKE_HEAD
    })

    -- Body segment 1
    table.insert(game.snake, {
        pos = {x = centerX - 1, z = centerZ},
        color = Constants.COLORS.SNAKE_BODY
    })

    -- Body segment 2 (tail)
    table.insert(game.snake, {
        pos = {x = centerX - 2, z = centerZ},
        color = Constants.COLORS.SNAKE_TAIL
    })

    log.debug("Snake initialized with %d segments at (%d, %d)",
        #game.snake, centerX, centerZ)
end

-- Move the snake one step
-- Matches C++ moveSnake() at lines 1869-1936
function Snake.move(game)
    if game.gameOver then
        log.trace("Skipping move - game over")
        return
    end

    -- Apply next direction
    game.direction = game.nextDirection
    local offset = Direction.toOffset(game.direction)

    -- Calculate new head position
    local head = game.snake[1]
    local newHead = {
        x = head.pos.x + offset.x,
        z = head.pos.z + offset.z
    }

    -- Wrap around grid edges (toroidal)
    newHead = Grid.wrap(game.gridSize, newHead)

    log.trace("Moving snake: (%d,%d) -> (%d,%d) dir=%s",
        head.pos.x, head.pos.z, newHead.x, newHead.z,
        Direction.getName(game.direction))

    -- Check ring formation (head crossing body at index >= 2)
    local ringResult = Snake.detectRing(game, newHead)
    if ringResult.formed then
        log.info("Ring formed at segment %d!", ringResult.startIndex)
        Snake.executeRingAttack(game, ringResult)
    end

    -- Check obstacle collision
    if Snake.isObstacleAt(game, newHead) then
        log.info("Snake hit obstacle at (%d, %d)", newHead.x, newHead.z)
        Snake.explode(game)
        game:transitionTo(GamePhase.GameOver)
        return
    end

    -- Check enemy collision with head
    if Snake.isEnemyAt(game, newHead) then
        log.info("Snake hit enemy at (%d, %d)", newHead.x, newHead.z)
        Snake.explode(game)
        game:transitionTo(GamePhase.GameOver)
        return
    end

    -- Check food collision
    local ateFood = Grid.posEquals(newHead, game.foodPos)

    -- Move the snake
    -- Insert new head
    table.insert(game.snake, 1, {
        pos = Grid.copyPos(newHead),
        color = Constants.COLORS.SNAKE_HEAD
    })

    -- Update colors (head becomes body)
    game.snake[2].color = Constants.COLORS.SNAKE_BODY

    if ateFood then
        -- Keep tail (snake grows)
        log.info("Snake ate food! Score: %d -> %d", game.score, game.score + 1)
        Snake.onFoodEaten(game)
    else
        -- Remove tail (snake moves)
        table.remove(game.snake)
    end

    -- Gradually darken body segments
    Snake.updateSegmentColors(game)
end

-- Update segment colors for gradient effect
function Snake.updateSegmentColors(game)
    local count = #game.snake
    for i = 2, count do
        local t = (i - 1) / math.max(1, count - 1)
        game.snake[i].color = {
            0.2 + 0.1 * t,
            0.9 - 0.1 * t,
            0.3 + 0.1 * t,
            1.0
        }
    end
end

-- Handle food eaten event
function Snake.onFoodEaten(game)
    game.score = game.score + 1
    game.foodCollected = game.foodCollected + 1

    -- Audio and visual feedback
    Audio.playSFX("eat")
    game:triggerFoodPop(game.foodPos)
    game:triggerScreenShake(Constants.SHAKE_INTENSITY_EAT, Constants.SHAKE_DURATION_EAT)

    -- Speed up
    game.moveInterval = math.max(
        Constants.MIN_MOVE_INTERVAL,
        game.moveInterval - Constants.SPEED_INCREASE
    )

    log.debug("Food eaten: score=%d, collected=%d/%d, interval=%.4f",
        game.score, game.foodCollected, game.foodRequired, game.moveInterval)

    -- Check level complete
    if game.foodCollected >= game.foodRequired then
        log.info("Level complete! Collected %d food", game.foodCollected)
        game:completeLevel()
        return
    end

    -- Start grid expansion
    game:startExpansion()

    -- Spawn new food
    Snake.spawnFood(game)
end

-- Detect if head forms a ring by crossing body
-- Matches C++ detectRing() at lines 1233-1247
function Snake.detectRing(game, headPos)
    for i = 3, #game.snake do  -- Skip head (1) and neck (2)
        if Grid.posEquals(headPos, game.snake[i].pos) then
            log.debug("Ring detected: head crossing body at segment %d", i)
            return {
                formed = true,
                startIndex = i
            }
        end
    end
    return {formed = false}
end

-- Execute ring attack on enclosed enemies
-- Matches C++ executeRingAttack() at lines 1332-1361
function Snake.executeRingAttack(game, ring)
    log.info("Executing ring attack from segment %d", ring.startIndex)

    -- Calculate enclosed cells (simplified flood fill)
    local enclosedCells = Snake.calculateEnclosedCells(game, ring.startIndex)

    log.debug("Ring encloses %d cells", #enclosedCells)

    -- Damage enemies in enclosed cells
    local enemiesHit = 0
    for _, cell in ipairs(enclosedCells) do
        for _, enemy in ipairs(game.enemies) do
            if Grid.posEquals(cell, enemy.pos) then
                Snake.damageEnemy(game, enemy, 1)
                enemiesHit = enemiesHit + 1
            end
        end
    end

    if enemiesHit > 0 then
        log.info("Ring attack hit %d enemies", enemiesHit)
    end

    -- Remove dead enemies
    Snake.removeDeadEnemies(game)
end

-- Calculate cells enclosed by snake ring (simplified BFS flood fill)
function Snake.calculateEnclosedCells(game, ringEndIndex)
    local enclosed = {}
    local gridSize = game.gridSize

    -- Create a grid marking snake body positions
    local snakeGrid = {}
    for i = 1, ringEndIndex do
        local pos = game.snake[i].pos
        local key = pos.x .. "," .. pos.z
        snakeGrid[key] = true
    end

    -- Flood fill from all edges to find exterior cells
    local exterior = {}
    local queue = {}

    -- Add all edge cells that aren't snake to queue
    for x = 0, gridSize - 1 do
        for z = 0, gridSize - 1 do
            if x == 0 or x == gridSize - 1 or z == 0 or z == gridSize - 1 then
                local key = x .. "," .. z
                if not snakeGrid[key] then
                    table.insert(queue, {x = x, z = z})
                    exterior[key] = true
                end
            end
        end
    end

    -- BFS to find all exterior cells
    while #queue > 0 do
        local cell = table.remove(queue, 1)
        local neighbors = {
            {x = cell.x - 1, z = cell.z},
            {x = cell.x + 1, z = cell.z},
            {x = cell.x, z = cell.z - 1},
            {x = cell.x, z = cell.z + 1}
        }

        for _, n in ipairs(neighbors) do
            if n.x >= 0 and n.x < gridSize and n.z >= 0 and n.z < gridSize then
                local key = n.x .. "," .. n.z
                if not exterior[key] and not snakeGrid[key] then
                    exterior[key] = true
                    table.insert(queue, n)
                end
            end
        end
    end

    -- Interior = cells that are not exterior and not snake
    for x = 0, gridSize - 1 do
        for z = 0, gridSize - 1 do
            local key = x .. "," .. z
            if not exterior[key] and not snakeGrid[key] then
                table.insert(enclosed, {x = x, z = z})
            end
        end
    end

    return enclosed
end

-- Damage an enemy
function Snake.damageEnemy(game, enemy, damage)
    enemy.currentHealth = enemy.currentHealth - damage
    enemy.isDamaged = true
    enemy.damageFlashTimer = Constants.DAMAGE_FLASH_DURATION
    enemy.showHealthBar = true
    enemy.healthBarTimer = Constants.HEALTH_BAR_DURATION

    Audio.playSFX("enemy_hit")
    log.debug("Enemy damaged: health=%d/%d", enemy.currentHealth, enemy.maxHealth)
end

-- Remove dead enemies from the game
function Snake.removeDeadEnemies(game)
    local removed = 0
    for i = #game.enemies, 1, -1 do
        if game.enemies[i].currentHealth <= 0 then
            table.remove(game.enemies, i)
            removed = removed + 1
        end
    end
    if removed > 0 then
        log.debug("Removed %d dead enemies", removed)
    end
end

-- Trigger chain break when enemy hits snake body
-- Matches C++ triggerChainBreak() at lines 1378-1404
function Snake.triggerChainBreak(game, breakIndex)
    log.info("Chain break triggered at segment %d", breakIndex)

    Audio.playSFX("chain_break")

    -- Create detached segments for all segments after break point
    for i = breakIndex + 1, #game.snake do
        local seg = game.snake[i]
        local worldPos = Grid.toWorld(game.gridSize, seg.pos)

        local detached = {
            pos = Grid.copyPos(seg.pos),
            worldPos = {x = worldPos.x, y = worldPos.y, z = worldPos.z},
            color = seg.color,
            timer = Constants.DETACH_ANIMATION_TIME,
            velocity = {
                x = (math.random() - 0.5) * 6,
                y = math.random() * 4 + 2,
                z = (math.random() - 0.5) * 6
            },
            scale = 0.85,
            willShatter = math.random() < Constants.SHATTER_CHANCE,
            isParticle = false,
            bounceCount = 0,
            angularVel = (math.random() - 0.5) * 30,
            isResting = false
        }

        table.insert(game.detachedSegments, detached)
        log.trace("Created detached segment at (%d, %d)", seg.pos.x, seg.pos.z)
    end

    -- Truncate snake
    local newLength = breakIndex
    while #game.snake > newLength do
        table.remove(game.snake)
    end

    log.debug("Snake truncated to %d segments", #game.snake)
end

-- Explode snake on death
-- Matches C++ explodeSnake() at lines 1406-1485
function Snake.explode(game)
    log.info("Snake exploding! %d segments", #game.snake)

    Audio.playSFX("death")
    game:triggerScreenShake(Constants.SHAKE_INTENSITY_DEATH, Constants.SHAKE_DURATION_DEATH)

    -- Calculate snake center for explosion direction
    local centerX, centerZ = 0, 0
    for _, seg in ipairs(game.snake) do
        centerX = centerX + seg.pos.x
        centerZ = centerZ + seg.pos.z
    end
    centerX = centerX / #game.snake
    centerZ = centerZ / #game.snake

    -- Convert each segment into explosion particles
    for _, seg in ipairs(game.snake) do
        local worldPos = Grid.toWorld(game.gridSize, seg.pos)

        -- Direction away from center
        local dx = seg.pos.x - centerX
        local dz = seg.pos.z - centerZ
        local dist = math.sqrt(dx * dx + dz * dz) + 0.1

        -- Create 8 small particles per segment
        for p = 1, 8 do
            local angle = (p / 8) * math.pi * 2 + math.random() * 0.5
            local speed = 4 + math.random() * 2
            local upSpeed = 3 + math.random() * 4

            local particle = {
                pos = Grid.copyPos(seg.pos),
                worldPos = {
                    x = worldPos.x + (math.random() - 0.5) * 0.3,
                    y = worldPos.y + (math.random() - 0.5) * 0.3,
                    z = worldPos.z + (math.random() - 0.5) * 0.3
                },
                color = {seg.color[1], seg.color[2], seg.color[3], seg.color[4]},
                timer = Constants.PARTICLE_LIFE_MIN + math.random() * (Constants.PARTICLE_LIFE_MAX - Constants.PARTICLE_LIFE_MIN),
                velocity = {
                    x = (dx / dist) * speed + math.cos(angle) * 2 + (math.random() - 0.5) * 3,
                    y = upSpeed,
                    z = (dz / dist) * speed + math.sin(angle) * 2 + (math.random() - 0.5) * 3
                },
                scale = 0.4 + math.random() * 0.15,
                willShatter = math.random() < Constants.SHATTER_CHANCE,
                isParticle = true,
                bounceCount = 0,
                angularVel = (math.random() - 0.5) * 30,
                isResting = false
            }

            table.insert(game.detachedSegments, particle)
        end
    end

    game.gameOver = true
    log.debug("Created %d explosion particles", #game.detachedSegments)
end

-- Check if position has obstacle
function Snake.isObstacleAt(game, pos)
    for _, obstacle in ipairs(game.obstacles) do
        if Grid.posEquals(pos, obstacle) then
            return true
        end
    end
    return false
end

-- Check if position has enemy
function Snake.isEnemyAt(game, pos)
    for _, enemy in ipairs(game.enemies) do
        if Grid.posEquals(pos, enemy.pos) then
            return true
        end
    end
    return false
end

-- Check if position has snake segment (excluding head)
function Snake.isSnakeBodyAt(game, pos)
    for i = 2, #game.snake do
        if Grid.posEquals(pos, game.snake[i].pos) then
            return true
        end
    end
    return false
end

-- Check if position is valid for food spawning
function Snake.isValidFoodPosition(game, pos)
    -- Not on edge
    if pos.x <= 0 or pos.x >= game.gridSize - 1 or
       pos.z <= 0 or pos.z >= game.gridSize - 1 then
        return false
    end

    -- Not on snake
    for _, seg in ipairs(game.snake) do
        if Grid.posEquals(pos, seg.pos) then
            return false
        end
    end

    -- Not on obstacle
    if Snake.isObstacleAt(game, pos) then
        return false
    end

    -- Not on enemy
    if Snake.isEnemyAt(game, pos) then
        return false
    end

    return true
end

-- Check if food is reachable via BFS
function Snake.isFoodReachable(game, targetPos)
    if #game.snake == 0 then return true end

    local head = game.snake[1].pos
    local gridSize = game.gridSize

    -- Create blocked grid (snake body + obstacles)
    local blocked = {}
    for i = 2, #game.snake do
        local pos = game.snake[i].pos
        blocked[pos.x .. "," .. pos.z] = true
    end
    for _, obstacle in ipairs(game.obstacles) do
        blocked[obstacle.x .. "," .. obstacle.z] = true
    end

    -- BFS from head to target
    local visited = {}
    local queue = {{x = head.x, z = head.z}}
    visited[head.x .. "," .. head.z] = true

    while #queue > 0 do
        local cell = table.remove(queue, 1)

        if cell.x == targetPos.x and cell.z == targetPos.z then
            return true
        end

        local neighbors = {
            {x = cell.x - 1, z = cell.z},
            {x = cell.x + 1, z = cell.z},
            {x = cell.x, z = cell.z - 1},
            {x = cell.x, z = cell.z + 1}
        }

        for _, n in ipairs(neighbors) do
            -- Wrap around grid
            local wrapped = Grid.wrap(gridSize, n)
            local key = wrapped.x .. "," .. wrapped.z

            if not visited[key] and not blocked[key] then
                visited[key] = true
                table.insert(queue, wrapped)
            end
        end
    end

    return false
end

-- Spawn food at a valid position
-- Matches C++ spawnFood() at lines 1938-2024
function Snake.spawnFood(game)
    log.debug("Spawning food...")

    local minDistance = Constants.MIN_FOOD_DISTANCE
    local oldFoodPos = game.foodPos or {x = -100, z = -100}
    local gridSize = game.gridSize

    -- Try random positions with distance requirement
    for attempt = 1, 100 do
        local pos = {
            x = math.random(1, gridSize - 2),
            z = math.random(1, gridSize - 2)
        }

        local dist = Grid.distanceSquared(pos, oldFoodPos)

        if dist >= minDistance * minDistance and
           Snake.isValidFoodPosition(game, pos) and
           Snake.isFoodReachable(game, pos) then

            game.foodPos = pos
            game.foodColor = Snake.generateRandomColor()
            log.debug("Food spawned at (%d, %d)", pos.x, pos.z)
            return
        end
    end

    -- Try without distance requirement
    for attempt = 1, 100 do
        local pos = {
            x = math.random(1, gridSize - 2),
            z = math.random(1, gridSize - 2)
        }

        if Snake.isValidFoodPosition(game, pos) and
           Snake.isFoodReachable(game, pos) then

            game.foodPos = pos
            game.foodColor = Snake.generateRandomColor()
            log.debug("Food spawned at (%d, %d) (relaxed distance)", pos.x, pos.z)
            return
        end
    end

    -- Last resort: any valid position
    for x = 1, gridSize - 2 do
        for z = 1, gridSize - 2 do
            local pos = {x = x, z = z}
            if Snake.isValidFoodPosition(game, pos) then
                game.foodPos = pos
                game.foodColor = Snake.generateRandomColor()
                log.warn("Food spawned at (%d, %d) (last resort)", pos.x, pos.z)
                return
            end
        end
    end

    log.error("Failed to spawn food!")
end

-- Generate a random vibrant color for food
-- Matches C++ generateRandomColor() at lines 2098-2116
function Snake.generateRandomColor()
    -- Generate HSV with high saturation, then convert to RGB
    local h = math.random()  -- 0-1 hue
    local s = 0.8 + math.random() * 0.2  -- 80-100% saturation
    local v = 0.9 + math.random() * 0.1  -- 90-100% value

    -- HSV to RGB conversion
    local i = math.floor(h * 6)
    local f = h * 6 - i
    local p = v * (1 - s)
    local q = v * (1 - f * s)
    local t = v * (1 - (1 - f) * s)

    i = i % 6
    local r, g, b
    if i == 0 then r, g, b = v, t, p
    elseif i == 1 then r, g, b = q, v, p
    elseif i == 2 then r, g, b = p, v, t
    elseif i == 3 then r, g, b = p, q, v
    elseif i == 4 then r, g, b = t, p, v
    else r, g, b = v, p, q
    end

    return {r, g, b, 1.0}
end

-- Update detached segments physics
-- Matches C++ updateDetachedSegments() at lines 1487+
function Snake.updateDetachedSegments(game, dt)
    local gravity = Constants.PARTICLE_GRAVITY

    for i = #game.detachedSegments, 1, -1 do
        local seg = game.detachedSegments[i]

        seg.timer = seg.timer - dt

        if not seg.isResting then
            -- Apply gravity
            seg.velocity.y = seg.velocity.y + gravity * dt

            -- Update position
            seg.worldPos.x = seg.worldPos.x + seg.velocity.x * dt
            seg.worldPos.y = seg.worldPos.y + seg.velocity.y * dt
            seg.worldPos.z = seg.worldPos.z + seg.velocity.z * dt

            -- Ground collision
            if seg.worldPos.y < 0 then
                seg.worldPos.y = 0
                seg.velocity.y = -seg.velocity.y * 0.5  -- Bounce with friction

                -- Reduce horizontal velocity on bounce
                seg.velocity.x = seg.velocity.x * 0.7
                seg.velocity.z = seg.velocity.z * 0.7

                seg.bounceCount = seg.bounceCount + 1

                -- Stop after several bounces
                if seg.bounceCount >= 3 or math.abs(seg.velocity.y) < 0.5 then
                    seg.isResting = true
                    seg.velocity = {x = 0, y = 0, z = 0}
                end
            end

            -- Angular velocity (for rotation effect)
            seg.angularVel = seg.angularVel * 0.98  -- Dampen
        end

        -- Remove expired segments
        if seg.timer <= 0 then
            table.remove(game.detachedSegments, i)
        end
    end
end

return Snake

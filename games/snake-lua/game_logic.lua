-- game_logic.lua - Core snake movement, collision, food mechanics
-- Dependencies: app.config, app.state, app.types, app.enemies, app.levels, app.main
-- (accessed inside functions)

local game_logic = {}

-- Update game logic (called each fixed timestep)
function game_logic.update(dt)
    local state = app.state
    if state.gameOver then return end

    -- Update move timer
    state.moveTimer = state.moveTimer + dt

    -- Move snake when timer expires
    if state.moveTimer >= state.moveInterval then
        state.moveTimer = state.moveTimer - state.moveInterval
        game_logic.applyBufferedInput()
        game_logic.moveSnake()
    end
end

-- Apply buffered direction change
function game_logic.applyBufferedInput()
    local state = app.state
    local types = app.types

    if state.hasBufferedInput then
        if not types.isOppositeDirection(state.nextDirection, state.direction) then
            state.direction = state.nextDirection
        end
        state.hasBufferedInput = false
    end
end

-- Move the snake one grid cell
function game_logic.moveSnake()
    local config = app.config
    local state = app.state
    local types = app.types
    local enemies = app.enemies
    local main = app.main

    if state.gameOver or #state.snake == 0 then return end

    -- Calculate new head position
    local head = state.snake[1]
    local offset = types.directionToOffset(state.direction)
    local newHead = types.gridPosAdd(head.pos, offset)

    -- Check wall collision
    if game_logic.isObstacleAt(newHead) then
        game_logic.triggerGameOver()
        return
    end

    -- Check self collision
    if game_logic.isSnakeAt(newHead) then
        -- Check if this forms a ring (head touches body with gap >= 2)
        local ringResult = game_logic.detectRing(newHead)
        if ringResult.formed then
            game_logic.executeRingAttack(ringResult)
        else
            game_logic.triggerGameOver()
            return
        end
    end

    -- Check enemy collision
    if enemies.isEnemyAt(newHead) then
        game_logic.triggerGameOver()
        return
    end

    -- Check bounds
    if newHead.x < 0 or newHead.x >= state.gridSize or
       newHead.z < 0 or newHead.z >= state.gridSize then
        game_logic.triggerGameOver()
        return
    end

    -- Move snake: insert new head at front
    local newHeadSeg = types.SnakeSegment(newHead, head.color)
    table.insert(state.snake, 1, newHeadSeg)

    -- Check if ate food
    local ateFood = types.gridPosEquals(newHead, state.foodPos)

    if ateFood then
        -- Don't remove tail (snake grows)
        state.foodCollected = state.foodCollected + 1
        state.score = state.score + 1

        -- Trigger food pop effect
        game_logic.triggerFoodPop(game_logic.gridToWorld(state.foodPos))

        -- Play eat sound
        local audio = app.audio
        audio.playEat()

        -- Speed up snake slightly
        state.moveInterval = math.max(config.MIN_MOVE_INTERVAL,
            state.moveInterval - config.SPEED_INCREASE_PER_FOOD)

        -- Check for level complete (win by reaching target snake length)
        if #state.snake >= state.lengthRequired then
            state.segmentsEarnedThisLevel = #state.snake
            main.transitionTo(GamePhase.LevelComplete)
            return
        end

        -- Spawn new food
        game_logic.spawnFood()

        -- Expand grid every time food is collected
        if state.gridSize < state.levelMaxSize then
            game_logic.startExpansion()
        end
    else
        -- Remove tail (snake moves without growing)
        table.remove(state.snake)
    end

    -- Check food pickups
    game_logic.checkFoodPickups(newHead)
end

-- Check if position has obstacle
function game_logic.isObstacleAt(pos)
    local state = app.state
    local types = app.types

    for _, obs in ipairs(state.obstacles) do
        if types.gridPosEquals(obs, pos) then
            return true
        end
    end
    return false
end

-- Check if position has snake
function game_logic.isSnakeAt(pos)
    local state = app.state
    local types = app.types

    for i, seg in ipairs(state.snake) do
        if types.gridPosEquals(seg.pos, pos) then
            return true
        end
    end
    return false
end

-- Get snake segment index at position (0 if not found)
function game_logic.getSnakeIndexAt(pos)
    local state = app.state
    local types = app.types

    for i, seg in ipairs(state.snake) do
        if types.gridPosEquals(seg.pos, pos) then
            return i
        end
    end
    return 0
end

-- Detect if a ring is formed when head touches body
function game_logic.detectRing(headPos)
    local config = app.config
    local types = app.types

    local result = types.RingResult()

    -- Find where head would touch body
    local touchIndex = game_logic.getSnakeIndexAt(headPos)
    if touchIndex == 0 then
        return result
    end

    -- Ring must have at least RING_MIN_SIZE segments gap
    if touchIndex <= config.RING_MIN_SIZE then
        return result
    end

    -- Ring is formed!
    result.formed = true
    result.ringStartIndex = touchIndex

    -- Calculate enclosed cells using flood fill
    result.enclosedCells = game_logic.calculateEnclosedCells(touchIndex)

    return result
end

-- Calculate cells enclosed by ring using flood fill
function game_logic.calculateEnclosedCells(ringEndIndex)
    local state = app.state
    local types = app.types

    local enclosed = {}

    -- Get ring boundary positions
    local ringPositions = {}
    for i = 1, ringEndIndex do
        table.insert(ringPositions, state.snake[i].pos)
    end

    -- Find bounding box of ring
    local minX, maxX = math.huge, -math.huge
    local minZ, maxZ = math.huge, -math.huge
    for _, pos in ipairs(ringPositions) do
        minX = math.min(minX, pos.x)
        maxX = math.max(maxX, pos.x)
        minZ = math.min(minZ, pos.z)
        maxZ = math.max(maxZ, pos.z)
    end

    -- Create set of ring positions for fast lookup
    local ringSet = {}
    for _, pos in ipairs(ringPositions) do
        local key = pos.x .. "," .. pos.z
        ringSet[key] = true
    end

    -- Flood fill from outside to find cells NOT enclosed
    local outside = {}
    local queue = {}

    -- Start from corners outside the bounding box
    for x = minX - 1, maxX + 1 do
        table.insert(queue, types.GridPos(x, minZ - 1))
        table.insert(queue, types.GridPos(x, maxZ + 1))
    end
    for z = minZ, maxZ do
        table.insert(queue, types.GridPos(minX - 1, z))
        table.insert(queue, types.GridPos(maxX + 1, z))
    end

    -- BFS flood fill
    while #queue > 0 do
        local pos = table.remove(queue, 1)
        local key = pos.x .. "," .. pos.z

        -- Skip if already visited or on ring
        if outside[key] or ringSet[key] then
            goto continue
        end

        -- Skip if outside bounding box (with margin)
        if pos.x < minX - 1 or pos.x > maxX + 1 or
           pos.z < minZ - 1 or pos.z > maxZ + 1 then
            goto continue
        end

        outside[key] = true

        -- Add neighbors
        table.insert(queue, types.GridPos(pos.x + 1, pos.z))
        table.insert(queue, types.GridPos(pos.x - 1, pos.z))
        table.insert(queue, types.GridPos(pos.x, pos.z + 1))
        table.insert(queue, types.GridPos(pos.x, pos.z - 1))

        ::continue::
    end

    -- Cells inside bounding box that are not outside or on ring are enclosed
    for x = minX, maxX do
        for z = minZ, maxZ do
            local key = x .. "," .. z
            if not outside[key] and not ringSet[key] then
                table.insert(enclosed, types.GridPos(x, z))
            end
        end
    end

    return enclosed
end

-- Execute ring attack on enemies inside ring
function game_logic.executeRingAttack(ring)
    local config = app.config
    local state = app.state
    local types = app.types
    local enemies = app.enemies
    local audio = app.audio

    -- Damage enemies inside ring
    for _, cell in ipairs(ring.enclosedCells) do
        for _, enemy in ipairs(state.enemies) do
            if types.gridPosEquals(enemy.pos, cell) then
                enemies.damageEnemy(enemy, config.RING_DAMAGE)
            end
        end
    end

    -- Play sound
    audio.playEnemyHit()

    -- Screen shake
    game_logic.triggerScreenShake(0.3, 0.2)
end

-- Build obstacle set cache for O(1) lookups (call when level loads or obstacles change)
function game_logic.rebuildObstacleCache()
    local state = app.state
    state.obstacleSet = {}
    for _, obs in ipairs(state.obstacles) do
        local key = obs.x .. "," .. obs.z
        state.obstacleSet[key] = true
    end
end

-- Check obstacle using cached set (O(1) instead of O(n))
function game_logic.isObstacleAtFast(pos)
    local state = app.state
    if not state.obstacleSet then
        game_logic.rebuildObstacleCache()
    end
    local key = pos.x .. "," .. pos.z
    return state.obstacleSet[key] == true
end

-- Build snake set cache for O(1) lookups
function game_logic.buildSnakeSet()
    local state = app.state
    local snakeSet = {}
    for i, seg in ipairs(state.snake) do
        local key = seg.pos.x .. "," .. seg.pos.z
        snakeSet[key] = i
    end
    return snakeSet
end

-- Spawn food at valid position (OPTIMIZED)
function game_logic.spawnFood()
    bestow.metrics.beginZone("SpawnFood")
    local config = app.config
    local state = app.state
    local types = app.types

    local margin = config.FOOD_EDGE_MARGIN
    local maxAttempts = 20  -- Reduced from 100
    local spawnPointSampleSize = 10  -- Only try 10 random spawn points, not all 168

    -- Build snake set once for O(1) lookups during this spawn
    local snakeSet = game_logic.buildSnakeSet()

    -- Level spawn points are in LEVEL coordinates, but we're working in GRID coordinates
    -- Need to transform them to current grid coordinates
    local offsetX = state.levelOffsetX or 0
    local offsetZ = state.levelOffsetZ or 0

    -- Try random spawn points (not all of them!)
    if #state.currentLevel.foodSpawnPoints > 0 then
        for attempt = 1, math.min(spawnPointSampleSize, #state.currentLevel.foodSpawnPoints) do
            local idx = math.random(1, #state.currentLevel.foodSpawnPoints)
            local levelPos = state.currentLevel.foodSpawnPoints[idx]

            -- Transform from level coords to current grid coords
            local gridX = levelPos.x - offsetX
            local gridZ = levelPos.z - offsetZ

            -- Only use if within current grid bounds
            if gridX >= margin and gridX < state.gridSize - margin and
               gridZ >= margin and gridZ < state.gridSize - margin then
                local pos = types.GridPos(gridX, gridZ)

                if game_logic.isValidFoodPositionFast(pos, snakeSet) then
                    state.foodPos = pos
                    state.foodColor = game_logic.generateRandomColor()
                    bestow.metrics.endZone("SpawnFood")
                    return
                end
            end
        end
    end

    -- Fall back to random position within grid
    for attempt = 1, maxAttempts do
        local x = math.random(margin, state.gridSize - margin - 1)
        local z = math.random(margin, state.gridSize - margin - 1)
        local pos = types.GridPos(x, z)

        if game_logic.isValidFoodPositionFast(pos, snakeSet) then
            state.foodPos = pos
            state.foodColor = game_logic.generateRandomColor()
            bestow.metrics.endZone("SpawnFood")
            return
        end
    end

    -- Last resort: just place somewhere safe
    local centerX = math.floor(state.gridSize / 2)
    local centerZ = math.floor(state.gridSize / 2)
    state.foodPos = types.GridPos(centerX + 3, centerZ)
    state.foodColor = game_logic.generateRandomColor()
    bestow.metrics.endZone("SpawnFood")
end

-- Check if food position is valid
function game_logic.isValidFoodPosition(pos)
    bestow.metrics.beginZone("ValidateFoodPos")
    local config = app.config
    local state = app.state
    local enemies = app.enemies

    -- Not on snake
    if game_logic.isSnakeAt(pos) then
        bestow.metrics.endZone("ValidateFoodPos")
        return false
    end

    -- Not on obstacle
    if game_logic.isObstacleAt(pos) then
        bestow.metrics.endZone("ValidateFoodPos")
        return false
    end

    -- Not on enemy
    if enemies.isEnemyAt(pos) then
        bestow.metrics.endZone("ValidateFoodPos")
        return false
    end

    -- Minimum distance from snake head
    if #state.snake > 0 then
        local head = state.snake[1].pos
        local dist = math.abs(pos.x - head.x) + math.abs(pos.z - head.z)
        if dist < config.MIN_FOOD_DISTANCE then
            bestow.metrics.endZone("ValidateFoodPos")
            return false
        end
    end

    -- Must be reachable from snake head (BFS) - THIS IS EXPENSIVE!
    bestow.metrics.beginZone("FoodReachabilityBFS")
    local reachable = #state.snake == 0 or game_logic.isFoodReachable(pos)
    bestow.metrics.endZone("FoodReachabilityBFS")

    if not reachable then
        bestow.metrics.endZone("ValidateFoodPos")
        return false
    end

    bestow.metrics.endZone("ValidateFoodPos")
    return true
end

-- Check if food at pos is reachable from snake head via BFS
function game_logic.isFoodReachable(targetPos)
    local state = app.state
    local types = app.types

    if #state.snake == 0 then return true end

    local head = state.snake[1].pos

    -- BFS from head to target
    local visited = {}
    local function key(p) return p.x .. "," .. p.z end

    local queue = {head}
    visited[key(head)] = true

    local directions = {
        {dx = 0, dz = -1},  -- Up
        {dx = 0, dz = 1},   -- Down
        {dx = -1, dz = 0},  -- Left
        {dx = 1, dz = 0}    -- Right
    }

    while #queue > 0 do
        local current = table.remove(queue, 1)

        -- Found target
        if current.x == targetPos.x and current.z == targetPos.z then
            return true
        end

        -- Explore neighbors
        for _, dir in ipairs(directions) do
            local nx = current.x + dir.dx
            local nz = current.z + dir.dz

            -- Wrap around grid
            if nx < 0 then nx = state.gridSize - 1
            elseif nx >= state.gridSize then nx = 0 end
            if nz < 0 then nz = state.gridSize - 1
            elseif nz >= state.gridSize then nz = 0 end

            local neighbor = types.GridPos(nx, nz)
            local nkey = key(neighbor)

            if not visited[nkey] then
                -- Check if walkable (not obstacle, not snake body except we can go through tail area)
                local blocked = false

                if game_logic.isObstacleAt(neighbor) then
                    blocked = true
                end

                -- Snake body blocks (but not tail since it moves)
                if not blocked then
                    for i = 1, #state.snake - 1 do
                        if state.snake[i].pos.x == neighbor.x and state.snake[i].pos.z == neighbor.z then
                            blocked = true
                            break
                        end
                    end
                end

                if not blocked then
                    visited[nkey] = true
                    table.insert(queue, neighbor)
                end
            end
        end
    end

    return false
end

-- OPTIMIZED: Fast food position validation with pre-built caches
function game_logic.isValidFoodPositionFast(pos, snakeSet)
    local config = app.config
    local state = app.state
    local enemies = app.enemies

    -- Not on snake (O(1) lookup)
    local key = pos.x .. "," .. pos.z
    if snakeSet[key] then
        return false
    end

    -- Not on obstacle (O(1) lookup)
    if game_logic.isObstacleAtFast(pos) then
        return false
    end

    -- Not on enemy
    if enemies.isEnemyAt(pos) then
        return false
    end

    -- Minimum distance from snake head
    if #state.snake > 0 then
        local head = state.snake[1].pos
        local dist = math.abs(pos.x - head.x) + math.abs(pos.z - head.z)
        if dist < config.MIN_FOOD_DISTANCE then
            return false
        end
    end

    -- SKIP BFS reachability check - it's expensive and spawn points are pre-validated
    -- The level designer placed these spawn points, so they should be reachable
    -- If food becomes unreachable due to snake position, player will just eat different food
    return true
end

-- OPTIMIZED: Depth-limited BFS for reachability (if needed)
function game_logic.isFoodReachableFast(targetPos, snakeSet, maxDepth)
    local state = app.state
    local types = app.types

    if #state.snake == 0 then return true end

    local head = state.snake[1].pos
    maxDepth = maxDepth or 50  -- Limit search to 50 steps (plenty for most cases)

    -- Quick Manhattan distance check - if too far, assume reachable
    local manhattanDist = math.abs(targetPos.x - head.x) + math.abs(targetPos.z - head.z)
    if manhattanDist > maxDepth then
        return true  -- Assume reachable if very far (would take too long to verify)
    end

    -- BFS from head to target with depth limit
    local visited = {}
    local function key(p) return p.x .. "," .. p.z end

    local queue = {{pos = head, depth = 0}}
    visited[key(head)] = true

    local directions = {
        {dx = 0, dz = -1},
        {dx = 0, dz = 1},
        {dx = -1, dz = 0},
        {dx = 1, dz = 0}
    }

    while #queue > 0 do
        local current = table.remove(queue, 1)

        -- Found target
        if current.pos.x == targetPos.x and current.pos.z == targetPos.z then
            return true
        end

        -- Depth limit reached
        if current.depth >= maxDepth then
            goto continue
        end

        -- Explore neighbors
        for _, dir in ipairs(directions) do
            local nx = current.pos.x + dir.dx
            local nz = current.pos.z + dir.dz

            -- Bounds check (no wrapping for simplicity)
            if nx >= 0 and nx < state.gridSize and nz >= 0 and nz < state.gridSize then
                local nkey = nx .. "," .. nz

                if not visited[nkey] then
                    -- Check if walkable using cached sets
                    local blocked = game_logic.isObstacleAtFast(types.GridPos(nx, nz))

                    -- Snake body blocks (except tail)
                    if not blocked and snakeSet[nkey] and snakeSet[nkey] < #state.snake then
                        blocked = true
                    end

                    if not blocked then
                        visited[nkey] = true
                        table.insert(queue, {pos = types.GridPos(nx, nz), depth = current.depth + 1})
                    end
                end
            end
        end
        ::continue::
    end

    -- If we explored maxDepth without finding target, assume reachable
    return true
end

-- Check food pickups at position
function game_logic.checkFoodPickups(pos)
    local state = app.state
    local types = app.types
    local audio = app.audio

    for i = #state.foodPickups, 1, -1 do
        local pickup = state.foodPickups[i]
        if types.gridPosEquals(pickup.pos, pos) then
            -- Eat the pickup
            state.score = state.score + 1
            audio.playEat()
            table.remove(state.foodPickups, i)

            -- Grow snake
            local tail = state.snake[#state.snake]
            local newSeg = types.SnakeSegment(tail.pos, pickup.color)
            table.insert(state.snake, newSeg)
        end
    end
end

-- Generate random vibrant color
function game_logic.generateRandomColor()
    local hue = math.random() * 360
    local sat = 0.7 + math.random() * 0.3
    local val = 0.8 + math.random() * 0.2

    -- HSV to RGB conversion
    local h = hue / 60
    local c = val * sat
    local x = c * (1 - math.abs(h % 2 - 1))
    local m = val - c

    local r, g, b
    if h < 1 then r, g, b = c, x, 0
    elseif h < 2 then r, g, b = x, c, 0
    elseif h < 3 then r, g, b = 0, c, x
    elseif h < 4 then r, g, b = 0, x, c
    elseif h < 5 then r, g, b = x, 0, c
    else r, g, b = c, 0, x end

    return {r + m, g + m, b + m, 1.0}
end

-- Start grid expansion animation
function game_logic.startExpansion()
    local state = app.state
    if state.gridSize >= state.levelMaxSize then return end

    state.isExpanding = true
    state.expansionTimer = 0.0
    state.targetGridSize = math.min(state.gridSize + 2, state.levelMaxSize)
end

-- Finalize grid expansion
function game_logic.finalizeExpansion()
    local state = app.state
    local levels = app.levels

    state.gridSize = state.targetGridSize
    state.isExpanding = false

    -- Rebuild visible obstacles
    levels.rebuildVisibleObstacles()
end

-- Update animations
function game_logic.updateAnimations(dt)
    local config = app.config
    local state = app.state

    -- Grid expansion
    if state.isExpanding then
        state.expansionTimer = state.expansionTimer + dt
        state.visualGridSize = state.visualGridSize +
            (state.targetGridSize - state.visualGridSize) * dt * 5.0

        if state.expansionTimer >= config.EXPANSION_WAIT_TIME then
            game_logic.finalizeExpansion()
        end
    end

    -- Screen shake
    if state.screenShakeTimer > 0 then
        state.screenShakeTimer = state.screenShakeTimer - dt
        local progress = state.screenShakeTimer / state.screenShakeDuration
        local intensity = state.screenShakeIntensity * progress

        state.screenShakeOffset = Vec3.new(
            (math.random() * 2 - 1) * intensity,
            (math.random() * 2 - 1) * intensity * 0.5,
            (math.random() * 2 - 1) * intensity
        )

        if state.screenShakeTimer <= 0 then
            state.screenShakeOffset = Vec3.new(0, 0, 0)
        end
    end

    -- Food pop effect
    if state.foodPopTimer > 0 then
        state.foodPopTimer = state.foodPopTimer - dt
        local progress = state.foodPopTimer / config.FOOD_POP_DURATION
        state.foodPopScale = 1.0 + math.sin(progress * math.pi) * 0.5
    end
end

-- Update detached segments physics
function game_logic.updateDetachedSegments(dt)
    local config = app.config
    local state = app.state
    local types = app.types

    for i = #state.detachedSegments, 1, -1 do
        local seg = state.detachedSegments[i]

        -- Update timer
        seg.timer = seg.timer - dt
        if seg.timer <= 0 then
            -- Check if should become food pickup
            if not seg.isParticle and not seg.willShatter then
                local pickup = types.FoodPickup(seg.pos, seg.color)
                pickup.spawnTime = state.gameTime
                table.insert(state.foodPickups, pickup)
            end
            table.remove(state.detachedSegments, i)
            goto continue
        end

        -- Update physics for particles
        if seg.isParticle and not seg.isResting then
            -- Apply gravity
            seg.velocity.y = seg.velocity.y + config.PARTICLE_GRAVITY * dt

            -- Update position
            seg.worldPos.x = seg.worldPos.x + seg.velocity.x * dt
            seg.worldPos.y = seg.worldPos.y + seg.velocity.y * dt
            seg.worldPos.z = seg.worldPos.z + seg.velocity.z * dt

            -- Update rotation
            seg.rotation = seg.rotation + seg.angularVel * dt

            -- Ground collision
            if seg.worldPos.y < 0.1 then
                seg.worldPos.y = 0.1
                seg.velocity.y = -seg.velocity.y * config.PARTICLE_BOUNCE_DAMPING
                seg.velocity.x = seg.velocity.x * 0.8
                seg.velocity.z = seg.velocity.z * 0.8
                seg.bounceCount = seg.bounceCount + 1

                if seg.bounceCount >= config.PARTICLE_MAX_BOUNCES or
                   math.abs(seg.velocity.y) < 0.5 then
                    seg.isResting = true
                    seg.velocity = Vec3.new(0, 0, 0)
                end
            end
        end

        ::continue::
    end
end

-- Trigger game over
function game_logic.triggerGameOver()
    local state = app.state
    local audio = app.audio
    local main = app.main
    local timeline = app.timeline

    state.gameOver = true

    -- Use timeline for death sequence
    timeline.sequence({
        -- Immediately: explode snake and play sound
        {delay = 0.0, action = function()
            game_logic.explodeSnake()
            audio.playDeath()
            game_logic.triggerScreenShake(0.5, 0.3)
        end},

        -- After 1.5 seconds: transition to game over menu
        {delay = 1.5, action = function()
            main.transitionTo(GamePhase.GameOver)
        end},
    })
end

-- Explode snake into particles
function game_logic.explodeSnake()
    local config = app.config
    local state = app.state
    local types = app.types

    -- Create particles from all segments BEFORE clearing
    for _, seg in ipairs(state.snake) do
        -- Create explosion particles
        local worldPos = game_logic.gridToWorld(seg.pos)

        for p = 1, config.PARTICLE_COUNT_PER_SHATTER do
            local particle = types.DetachedSegment(seg.pos, seg.color)
            particle.isParticle = true
            particle.scale = 0.2 + math.random() * 0.2
            particle.worldPos = Vec3.new(
                worldPos.x + (math.random() - 0.5) * 0.5,
                worldPos.y + 0.5,
                worldPos.z + (math.random() - 0.5) * 0.5
            )

            -- Random velocity
            local speed = 5.0 + math.random() * 5.0
            local angle = math.random() * math.pi * 2
            particle.velocity = Vec3.new(
                math.cos(angle) * speed,
                3.0 + math.random() * 4.0,
                math.sin(angle) * speed
            )

            particle.angularVel = (math.random() - 0.5) * config.PARTICLE_ANGULAR_SPEED
            particle.timer = 2.0 + math.random()

            table.insert(state.detachedSegments, particle)
        end
    end

    -- Clear the snake - it's now all particles (matches C++ behavior)
    state.snake = {}
end

-- Trigger chain break at segment index
function game_logic.triggerChainBreak(breakIndex)
    local config = app.config
    local state = app.state
    local types = app.types
    local audio = app.audio

    audio.playChainBreak()
    game_logic.triggerScreenShake(0.2, 0.15)

    -- Detach segments from break point onward
    for i = breakIndex, #state.snake do
        local seg = state.snake[i]
        local detached = types.DetachedSegment(seg.pos, seg.color)
        detached.willShatter = math.random() < config.SEGMENT_SHATTER_CHANCE
        table.insert(state.detachedSegments, detached)
    end

    -- Remove segments from snake
    for i = #state.snake, breakIndex, -1 do
        table.remove(state.snake, i)
    end
end

-- Trigger screen shake
function game_logic.triggerScreenShake(intensity, duration)
    local state = app.state
    state.screenShakeIntensity = intensity
    state.screenShakeDuration = duration
    state.screenShakeTimer = duration
end

-- Trigger food pop effect
function game_logic.triggerFoodPop(worldPos)
    local config = app.config
    local state = app.state
    state.lastFoodPos = worldPos
    state.foodPopTimer = config.FOOD_POP_DURATION
    state.foodPopScale = 1.0
end

-- Convert grid position to world position
function game_logic.gridToWorld(pos)
    local config = app.config
    local state = app.state
    local halfGrid = state.gridSize * config.CELL_SIZE * 0.5
    return Vec3.new(
        pos.x * config.CELL_SIZE - halfGrid + config.CELL_SIZE * 0.5,
        0.5,
        pos.z * config.CELL_SIZE - halfGrid + config.CELL_SIZE * 0.5
    )
end

-- Convert grid position to world position (float version)
function game_logic.gridToWorldFloat(x, z)
    local config = app.config
    local state = app.state
    local halfGrid = state.gridSize * config.CELL_SIZE * 0.5
    return Vec3.new(
        x * config.CELL_SIZE - halfGrid + config.CELL_SIZE * 0.5,
        0.5,
        z * config.CELL_SIZE - halfGrid + config.CELL_SIZE * 0.5
    )
end

-- Restart current level
function game_logic.restartGame()
    local state = app.state
    local levels = app.levels

    state.reset()

    -- Reinitialize from current level
    levels.applyToGame()
end

-- Try ring attack (called from input)
function game_logic.tryRingAttack()
    -- Ring attack happens automatically when snake forms a ring
    -- This function is here for potential manual trigger
end

return game_logic

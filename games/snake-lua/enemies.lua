-- enemies.lua - Enemy AI and management
-- Matches C++ enemy functions exactly

local config = require("config")
local state = require("state")
local types = require("types")

local enemies = {}

-- Spawn enemies from the level's enemy zones
function enemies.spawnFromZones()
    state.enemies = {}

    for _, zone in ipairs(state.currentLevel.enemyZones) do
        -- Transform zone bounds from level coords to current grid coords
        local gridZone = types.AABB2Di(
            zone.bounds.minX - state.levelOffsetX,
            zone.bounds.minZ - state.levelOffsetZ,
            zone.bounds.maxX - state.levelOffsetX,
            zone.bounds.maxZ - state.levelOffsetZ
        )

        -- Skip zones that are entirely outside current grid
        if gridZone.maxX >= 0 and gridZone.minX < state.gridSize and
           gridZone.maxZ >= 0 and gridZone.minZ < state.gridSize then

            -- Clamp zone to current grid bounds
            gridZone.minX = math.max(0, gridZone.minX)
            gridZone.minZ = math.max(0, gridZone.minZ)
            gridZone.maxX = math.min(state.gridSize - 1, gridZone.maxX)
            gridZone.maxZ = math.min(state.gridSize - 1, gridZone.maxZ)

            for i = 1, zone.enemyCount do
                local enemy = types.Enemy()
                enemy.patrolZone = gridZone
                enemy.moveInterval = zone.moveInterval
                enemy.maxHealth = zone.health
                enemy.currentHealth = zone.health

                -- Spawn at random position within the visible part of the zone
                enemy.pos = types.GridPos(
                    math.random(gridZone.minX, gridZone.maxX),
                    math.random(gridZone.minZ, gridZone.maxZ)
                )

                -- Initialize visual position to match grid position
                enemy.visualX = enemy.pos.x
                enemy.visualZ = enemy.pos.z
                enemy.visualInitialized = true

                -- Random starting direction
                local dirs = {Direction.Up, Direction.Down, Direction.Left, Direction.Right}
                enemy.currentDir = dirs[math.random(1, 4)]

                table.insert(state.enemies, enemy)
            end
        end
    end
end

-- Update all enemies
function enemies.update(dt)
    if state.gameOver then return end

    for _, enemy in ipairs(state.enemies) do
        -- Initialize visual position if needed
        if not enemy.visualInitialized then
            enemy.visualX = enemy.pos.x
            enemy.visualZ = enemy.pos.z
            enemy.visualInitialized = true
        end

        -- Smoothly interpolate visual position toward grid position
        local targetX = enemy.pos.x
        local targetZ = enemy.pos.z
        enemy.visualX = enemy.visualX + (targetX - enemy.visualX) * config.ENEMY_LERP_SPEED * dt
        enemy.visualZ = enemy.visualZ + (targetZ - enemy.visualZ) * config.ENEMY_LERP_SPEED * dt

        -- Update damage flash timer
        if enemy.isDamaged then
            enemy.damageFlashTimer = enemy.damageFlashTimer - dt
            if enemy.damageFlashTimer <= 0 then
                enemy.isDamaged = false
            end
        end

        -- Update health bar visibility timer
        if enemy.showHealthBar then
            enemy.healthBarTimer = enemy.healthBarTimer - dt
            if enemy.healthBarTimer <= 0 then
                enemy.showHealthBar = false
            end
        end

        -- Update movement timer
        enemy.moveTimer = enemy.moveTimer + dt
        if enemy.moveTimer >= enemy.moveInterval then
            enemy.moveTimer = 0
            enemies.moveEnemy(enemy)
        end
    end

    -- Check enemy-snake collision
    enemies.checkSnakeCollision()
end

-- Move a single enemy using patrol AI
function enemies.moveEnemy(enemy)
    local game_logic = require("game_logic")

    -- Collect valid moves within patrol zone
    local validMoves = {}
    local allDirs = {Direction.Up, Direction.Down, Direction.Left, Direction.Right}

    for _, dir in ipairs(allDirs) do
        local offset = types.directionToOffset(dir)
        local newPos = types.gridPosAdd(enemy.pos, offset)

        -- Check if within patrol zone
        if types.aabb2diContains(enemy.patrolZone, newPos) then
            -- Check if not blocked by wall or other enemy
            if not game_logic.isObstacleAt(newPos) and not enemies.isEnemyAt(newPos, enemy) then
                table.insert(validMoves, dir)
            end
        end
    end

    if #validMoves == 0 then return end

    -- 85% chance to continue in current direction if valid (less random)
    local preferCurrent = math.random() < config.ENEMY_CONTINUE_CHANCE

    local chosenDir = enemy.currentDir
    local currentIsValid = false

    for _, dir in ipairs(validMoves) do
        if dir == enemy.currentDir then
            currentIsValid = true
            break
        end
    end

    if preferCurrent and currentIsValid then
        chosenDir = enemy.currentDir
    else
        -- Pick random valid direction
        chosenDir = validMoves[math.random(1, #validMoves)]
    end

    -- Apply movement
    local offset = types.directionToOffset(chosenDir)
    enemy.pos = types.gridPosAdd(enemy.pos, offset)
    enemy.currentDir = chosenDir
end

-- Check if there's an enemy at a position (optionally excluding one)
function enemies.isEnemyAt(pos, exclude)
    for _, enemy in ipairs(state.enemies) do
        if enemy ~= exclude and types.gridPosEquals(enemy.pos, pos) then
            return true
        end
    end
    return false
end

-- Check collisions between enemies and snake
function enemies.checkSnakeCollision()
    if #state.snake == 0 then return end

    local game_logic = require("game_logic")

    -- Check if snake head hit any enemy
    local headPos = state.snake[1].pos
    for _, enemy in ipairs(state.enemies) do
        if types.gridPosEquals(enemy.pos, headPos) then
            -- Snake head hit enemy = game over with explosion!
            game_logic.explodeSnake()
            local game = require("main")
            game.transitionTo(GamePhase.GameOver)
            return
        end
    end

    -- Check if any enemy hit the snake body - trigger chain break
    for _, enemy in ipairs(state.enemies) do
        for i = 2, #state.snake do
            if types.gridPosEquals(enemy.pos, state.snake[i].pos) then
                -- Chain break at collision point!
                game_logic.triggerChainBreak(i - 1)  -- Convert to 0-indexed for C++ compatibility
                return  -- Only one break per frame
            end
        end
    end
end

-- Damage an enemy
function enemies.damageEnemy(enemy, damage)
    enemy.currentHealth = enemy.currentHealth - damage
    enemy.isDamaged = true
    enemy.damageFlashTimer = config.ENEMY_DAMAGE_FLASH_DURATION
    enemy.showHealthBar = true
    enemy.healthBarTimer = config.ENEMY_HEALTH_BAR_DURATION

    -- Play hit sound
    local audio = require("audio")
    audio.playSound(state.soundEnemyHit)
end

-- Remove dead enemies (called after ring attack or damage)
function enemies.removeDeadEnemies()
    local i = 1
    while i <= #state.enemies do
        if state.enemies[i].currentHealth <= 0 then
            table.remove(state.enemies, i)
        else
            i = i + 1
        end
    end
end

-- Get enemy at a specific position (for ring attack damage)
function enemies.getEnemyAt(pos)
    for _, enemy in ipairs(state.enemies) do
        if types.gridPosEquals(enemy.pos, pos) then
            return enemy
        end
    end
    return nil
end

return enemies

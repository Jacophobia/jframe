-- games/snake3d/modules/enemies.lua
-- Enemy AI, spawning, and damage handling
--
-- Matches C++ enemy logic from snake.game.cppm

local Constants = bestow.include("modules/constants")
local Direction = bestow.include("modules/direction")
local Grid = bestow.include("modules/grid")
local Log = bestow.include("modules/log")

local Enemies = {}

local log = Log.category("Enemies")

-- Initialize enemies array
function Enemies.init(game)
    game.enemies = {}
    log.debug("Enemies system initialized")
end

-- Spawn enemies from level zones
function Enemies.spawnFromZones(game)
    log.debug("Spawning enemies from zones...")

    local level = game.currentLevel
    if not level or not level.enemyZones then
        log.debug("No enemy zones defined")
        return
    end

    for _, zone in ipairs(level.enemyZones) do
        Enemies.spawnInZone(game, zone)
    end

    log.info("Spawned %d enemies total", #game.enemies)
end

-- Spawn enemies in a single zone
function Enemies.spawnInZone(game, zone)
    local count = zone.count or 1
    local health = zone.health or 1
    local moveInterval = zone.moveInterval or Constants.ENEMY_MOVE_INTERVAL

    log.trace("Spawning %d enemies in zone, health=%d, interval=%.2f",
        count, health, moveInterval)

    for i = 1, count do
        -- Random position within zone bounds
        local minX = zone.bounds.min.x
        local maxX = zone.bounds.max.x
        local minZ = zone.bounds.min.z
        local maxZ = zone.bounds.max.z

        -- Clamp to current grid size
        minX = math.max(0, math.min(minX, game.gridSize - 1))
        maxX = math.max(0, math.min(maxX, game.gridSize - 1))
        minZ = math.max(0, math.min(minZ, game.gridSize - 1))
        maxZ = math.max(0, math.min(maxZ, game.gridSize - 1))

        local pos = {
            x = math.random(minX, maxX),
            z = math.random(minZ, maxZ)
        }

        local enemy = {
            pos = pos,
            patrolZone = {
                min = {x = minX, z = minZ},
                max = {x = maxX, z = maxZ}
            },
            currentDir = math.random(0, 3),  -- Random starting direction
            moveTimer = 0,
            moveInterval = moveInterval,
            maxHealth = health,
            currentHealth = health,
            isBoss = zone.isBoss or false,
            showHealthBar = false,
            healthBarTimer = 0,
            visualX = pos.x,
            visualZ = pos.z,
            visualInitialized = true,
            isDamaged = false,
            damageFlashTimer = 0
        }

        table.insert(game.enemies, enemy)
        log.trace("Spawned enemy at (%d, %d)", pos.x, pos.z)
    end
end

-- Update all enemies
function Enemies.update(game, dt)
    for _, enemy in ipairs(game.enemies) do
        -- Update movement timer
        enemy.moveTimer = enemy.moveTimer + dt

        if enemy.moveTimer >= enemy.moveInterval then
            enemy.moveTimer = 0
            Enemies.moveEnemy(game, enemy)
        end

        -- Update visual interpolation
        Enemies.updateVisual(enemy, dt)

        -- Update damage flash
        if enemy.isDamaged then
            enemy.damageFlashTimer = enemy.damageFlashTimer - dt
            if enemy.damageFlashTimer <= 0 then
                enemy.isDamaged = false
            end
        end

        -- Update health bar visibility
        if enemy.showHealthBar then
            enemy.healthBarTimer = enemy.healthBarTimer - dt
            if enemy.healthBarTimer <= 0 then
                enemy.showHealthBar = false
            end
        end
    end

    -- Check enemy-snake body collision (chain break)
    Enemies.checkSnakeCollision(game)
end

-- Move a single enemy
-- Matches C++ moveEnemy() at lines 1147-1192
function Enemies.moveEnemy(game, enemy)
    -- Collect valid move directions
    local validDirs = {}

    for dir = 0, 3 do
        local offset = Direction.toOffset(dir)
        local newPos = {
            x = enemy.pos.x + offset.x,
            z = enemy.pos.z + offset.z
        }

        -- Check if within patrol zone
        if newPos.x >= enemy.patrolZone.min.x and
           newPos.x <= enemy.patrolZone.max.x and
           newPos.z >= enemy.patrolZone.min.z and
           newPos.z <= enemy.patrolZone.max.z then

            -- Check not blocked by obstacle
            local isBlocked = false
            for _, obstacle in ipairs(game.obstacles) do
                if Grid.posEquals(newPos, obstacle) then
                    isBlocked = true
                    break
                end
            end

            -- Check not occupied by another enemy
            if not isBlocked then
                for _, other in ipairs(game.enemies) do
                    if other ~= enemy and Grid.posEquals(newPos, other.pos) then
                        isBlocked = true
                        break
                    end
                end
            end

            if not isBlocked then
                table.insert(validDirs, dir)
            end
        end
    end

    if #validDirs == 0 then
        log.trace("Enemy at (%d, %d) has no valid moves", enemy.pos.x, enemy.pos.z)
        return
    end

    -- 85% chance to prefer current direction
    local preferCurrent = math.random() < 0.85

    local chosenDir
    if preferCurrent then
        -- Check if current direction is valid
        local currentValid = false
        for _, dir in ipairs(validDirs) do
            if dir == enemy.currentDir then
                currentValid = true
                break
            end
        end

        if currentValid then
            chosenDir = enemy.currentDir
        else
            chosenDir = validDirs[math.random(#validDirs)]
        end
    else
        chosenDir = validDirs[math.random(#validDirs)]
    end

    -- Apply movement
    local offset = Direction.toOffset(chosenDir)
    enemy.pos.x = enemy.pos.x + offset.x
    enemy.pos.z = enemy.pos.z + offset.z
    enemy.currentDir = chosenDir

    log.trace("Enemy moved to (%d, %d) dir=%s",
        enemy.pos.x, enemy.pos.z, Direction.getName(chosenDir))
end

-- Update enemy visual position (smooth interpolation)
function Enemies.updateVisual(enemy, dt)
    local lerpSpeed = Constants.ENEMY_LERP_SPEED * dt

    enemy.visualX = enemy.visualX + (enemy.pos.x - enemy.visualX) * lerpSpeed
    enemy.visualZ = enemy.visualZ + (enemy.pos.z - enemy.visualZ) * lerpSpeed
end

-- Check for enemy-snake body collision (triggers chain break)
function Enemies.checkSnakeCollision(game)
    local Snake = bestow.include("modules/snake")

    for _, enemy in ipairs(game.enemies) do
        -- Check head collision (game over)
        if #game.snake > 0 and Grid.posEquals(enemy.pos, game.snake[1].pos) then
            log.info("Enemy hit snake head!")
            -- This is handled in Snake.move()
            return
        end

        -- Check body collision (chain break)
        for i = 2, #game.snake do
            if Grid.posEquals(enemy.pos, game.snake[i].pos) then
                log.info("Enemy hit snake body at segment %d", i)
                Snake.triggerChainBreak(game, i)
                return  -- Only one chain break per frame
            end
        end
    end
end

-- Get enemy world position for rendering (with bob animation)
function Enemies.getWorldPos(game, enemy)
    local worldPos = Grid.toWorldFloat(game.gridSize, enemy.visualX, enemy.visualZ)

    -- Add bobbing animation
    local bob = 0.05 * math.sin(game.gameTime * 3.0 + enemy.visualX * 0.5)
    worldPos.y = worldPos.y + bob

    return worldPos
end

return Enemies

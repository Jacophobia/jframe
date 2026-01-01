-- types.lua - Data structures matching C++ structs
-- No dependencies - pure utility functions

local types = {}

-- GridPos: 2D grid coordinates
function types.GridPos(x, z)
    return {
        x = x or 0,
        z = z or 0
    }
end

function types.gridPosEquals(a, b)
    return a.x == b.x and a.z == b.z
end

function types.gridPosAdd(a, b)
    return types.GridPos(a.x + b.x, a.z + b.z)
end

function types.gridPosSub(a, b)
    return types.GridPos(a.x - b.x, a.z - b.z)
end

-- SnakeSegment: A segment of the snake body
function types.SnakeSegment(pos, color)
    return {
        pos = pos or types.GridPos(0, 0),
        color = color or {0.2, 0.8, 0.3, 1.0}  -- RGBA floats
    }
end

-- DetachedSegment: Segment that has been separated from snake
function types.DetachedSegment(pos, color)
    return {
        pos = pos or types.GridPos(0, 0),
        color = color or {0.2, 0.8, 0.3, 1.0},
        timer = 3.0,            -- Countdown to fade
        willShatter = false,    -- 1/6 chance to create particles
        velocity = Vec3.new(0, 0, 0),
        worldPos = Vec3.new(0, 0, 0),
        scale = 1.0,
        isParticle = false,     -- True for explosion particles
        bounceCount = 0,
        angularVel = 0.0,
        rotation = 0.0,
        isResting = false
    }
end

-- FoodPickup: Food dropped by detached segments
function types.FoodPickup(pos, color)
    return {
        pos = pos or types.GridPos(0, 0),
        color = color or {1.0, 0.8, 0.2, 1.0},
        spawnTime = 0.0  -- For fade-in animation
    }
end

-- AABB2Di: 2D integer axis-aligned bounding box
function types.AABB2Di(minX, minZ, maxX, maxZ)
    return {
        minX = minX or 0,
        minZ = minZ or 0,
        maxX = maxX or 0,
        maxZ = maxZ or 0
    }
end

function types.aabb2diContains(aabb, pos)
    return pos.x >= aabb.minX and pos.x <= aabb.maxX and
           pos.z >= aabb.minZ and pos.z <= aabb.maxZ
end

-- Enemy: Roaming patrol enemy
function types.Enemy(pos, patrolZone)
    return {
        pos = pos or types.GridPos(0, 0),
        patrolZone = patrolZone or types.AABB2Di(0, 0, 10, 10),
        currentDir = Direction.Right,
        moveTimer = 0.0,
        moveInterval = 0.5,

        maxHealth = 1,
        currentHealth = 1,
        showHealthBar = false,
        healthBarTimer = 0.0,

        isBoss = false,
        bossType = "",

        isDamaged = false,
        damageFlashTimer = 0.0,

        visualX = 0.0,
        visualZ = 0.0,
        visualInitialized = false
    }
end

-- EnemyZone: Level definition for enemy spawn area
function types.EnemyZone(bounds, enemyCount, moveInterval, health)
    return {
        bounds = bounds or types.AABB2Di(0, 0, 5, 5),
        enemyCount = enemyCount or 1,
        moveInterval = moveInterval or 0.5,
        health = health or 1
    }
end

-- LevelMap: Parsed level data
function types.LevelMap()
    return {
        name = "",
        width = 15,
        height = 15,

        walls = {},           -- Array of GridPos
        foodSpawnPoints = {}, -- Array of GridPos
        enemyZones = {},      -- Array of EnemyZone
        playerStart = types.GridPos(5, 5),

        foodRequired = 5,
        isBossLevel = false
    }
end

-- WorldMapNode: Node on the world map
function types.WorldMapNode(gridPos, levelIndex)
    return {
        gridPos = gridPos or types.GridPos(0, 0),
        levelIndex = levelIndex or 0,
        displayName = "",
        isUnlocked = false,
        isCompleted = false,
        isBoss = false
    }
end

-- WorldConfig: World definition with levels
function types.WorldConfig()
    return {
        name = "Default World",
        theme = "forest",

        levelFiles = {},        -- Array of level file paths
        unlockRequirements = {}, -- Food needed to unlock each level
        nodes = {},             -- Array of WorldMapNode
        paths = {},             -- Array of {fromIndex, toIndex} pairs

        bossName = "",
        bossType = "",
        bossHealth = 10
    }
end

-- WorldProgress: Save data for a single world
function types.WorldProgress()
    return {
        levelsCompleted = {},   -- Array of booleans
        foodCollected = {},     -- Food earned per level
        bossDefeated = false,
        totalFoodEarned = 0
    }
end

-- GameSaveData: All save data
function types.GameSaveData()
    return {
        worldProgress = {},     -- Array of WorldProgress
        currentWorld = 0,
        unlockedWorlds = {true} -- First world always unlocked
    }
end

-- RingResult: Result of ring detection
function types.RingResult()
    return {
        formed = false,
        enclosedCells = {},     -- Array of GridPos
        ringStartIndex = 0
    }
end

-- Direction helpers
function types.directionToOffset(dir)
    if dir == Direction.Up then
        return types.GridPos(0, -1)
    elseif dir == Direction.Down then
        return types.GridPos(0, 1)
    elseif dir == Direction.Left then
        return types.GridPos(-1, 0)
    elseif dir == Direction.Right then
        return types.GridPos(1, 0)
    end
    return types.GridPos(0, 0)
end

function types.oppositeDirection(dir)
    if dir == Direction.Up then return Direction.Down
    elseif dir == Direction.Down then return Direction.Up
    elseif dir == Direction.Left then return Direction.Right
    elseif dir == Direction.Right then return Direction.Left
    end
    return dir
end

function types.isOppositeDirection(dir1, dir2)
    return types.oppositeDirection(dir1) == dir2
end

return types

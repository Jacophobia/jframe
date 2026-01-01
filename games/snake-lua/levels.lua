-- levels.lua - Level and world loading
-- Matches C++ level/world functions exactly

local config = require("config")
local state = require("state")
local types = require("types")

local levels = {}

-- Load a level from a Lua file path
function levels.loadLevel(levelPath)
    -- Use bestow config system to parse Lua
    local result = bestow.config.parseLuaFile(levelPath)
    if not result then
        print("Failed to load level: " .. levelPath)
        return false
    end

    return levels.parseLevelTable(result)
end

-- Parse a level table into currentLevel
function levels.parseLevelTable(table)
    -- Clear current level
    state.currentLevel = types.LevelMap()

    -- Basic properties
    state.currentLevel.name = table.name or "Unnamed Level"
    state.currentLevel.width = table.width or 15
    state.currentLevel.height = table.height or 15
    state.currentLevel.foodRequired = table.foodRequired or 5
    state.currentLevel.isBossLevel = table.isBossLevel or false

    -- Player start position
    if table.playerStart then
        state.currentLevel.playerStart = types.GridPos(
            table.playerStart.x or state.currentLevel.width / 2,
            table.playerStart.z or state.currentLevel.height / 2
        )
    end

    -- Walls
    if table.walls then
        for _, wallDef in ipairs(table.walls) do
            table.insert(state.currentLevel.walls, types.GridPos(
                wallDef.x or 0,
                wallDef.z or 0
            ))
        end
    end

    -- Food spawn points
    if table.foodSpawnPoints then
        for _, posDef in ipairs(table.foodSpawnPoints) do
            table.insert(state.currentLevel.foodSpawnPoints, types.GridPos(
                posDef.x or 0,
                posDef.z or 0
            ))
        end
    end

    -- Enemy zones
    if table.enemies then
        for _, enemyDef in ipairs(table.enemies) do
            local zone = enemyDef.zone
            local ez = types.EnemyZone(
                types.AABB2Di(
                    zone.minX or 0,
                    zone.minZ or 0,
                    zone.maxX or 0,
                    zone.maxZ or 0
                ),
                enemyDef.count or 1,
                enemyDef.moveInterval or config.ENEMY_MOVE_INTERVAL,
                enemyDef.health or 1
            )
            table.insert(state.currentLevel.enemyZones, ez)
        end
    end

    -- Update grid size from level
    state.gridSize = state.currentLevel.width
    state.foodRequired = state.currentLevel.foodRequired

    print("Loaded level: " .. state.currentLevel.name ..
          " (" .. state.currentLevel.width .. "x" .. state.currentLevel.height .. ")" ..
          " with " .. #state.currentLevel.walls .. " walls, " ..
          #state.currentLevel.foodSpawnPoints .. " spawn points, " ..
          #state.currentLevel.enemyZones .. " enemy zones")

    return true
end

-- Load a world from a Lua file path
function levels.loadWorld(worldPath)
    -- Use bestow config system to parse Lua
    local result = bestow.config.parseLuaFile(worldPath)
    if not result then
        print("Failed to load world: " .. worldPath)
        return false
    end

    return levels.parseWorldTable(result)
end

-- Parse a world table into currentWorld
function levels.parseWorldTable(table)
    -- Clear current world
    state.currentWorld = types.WorldConfig()

    -- Basic properties
    state.currentWorld.name = table.name or "Unnamed World"
    state.currentWorld.theme = table.theme or "default"

    -- Level files
    if table.levels then
        for _, levelFile in ipairs(table.levels) do
            table.insert(state.currentWorld.levelFiles, levelFile)
        end
    end

    -- Unlock requirements
    if table.unlockRequirements then
        for idx, requirement in pairs(table.unlockRequirements) do
            -- Ensure array is large enough
            while #state.currentWorld.unlockRequirements < idx do
                table.insert(state.currentWorld.unlockRequirements, 0)
            end
            state.currentWorld.unlockRequirements[idx] = requirement
        end
    end

    -- Map nodes
    if table.nodes then
        for _, nodeDef in ipairs(table.nodes) do
            local node = types.WorldMapNode(
                types.GridPos(nodeDef.x or 0, nodeDef.z or 0),
                (nodeDef.levelIndex or 1) - 1  -- Convert to 0-indexed
            )
            node.displayName = nodeDef.name or "Level"
            node.isBoss = nodeDef.isBoss or false
            table.insert(state.currentWorld.nodes, node)
        end
    end

    -- Paths (connections between nodes)
    if table.paths then
        for _, pathDef in ipairs(table.paths) do
            local a = pathDef[1] - 1  -- Convert to 0-indexed
            local b = pathDef[2] - 1
            table.insert(state.currentWorld.paths, {a, b})
        end
    end

    -- Boss config
    if table.boss then
        state.currentWorld.bossName = table.boss.name or "Boss"
        state.currentWorld.bossType = table.boss.type or "default"
        state.currentWorld.bossHealth = table.boss.health or 10
    end

    print("Loaded world: " .. state.currentWorld.name ..
          " with " .. #state.currentWorld.levelFiles .. " levels, " ..
          #state.currentWorld.nodes .. " nodes, " ..
          #state.currentWorld.paths .. " paths")

    return true
end

-- Apply the loaded level to the game state
function levels.applyToGame()
    local game_logic = require("game_logic")
    local enemies = require("enemies")

    -- Store the level's max size, but start with initial small grid
    state.levelMaxSize = state.currentLevel.width
    state.gridSize = config.INITIAL_GRID_SIZE
    state.targetGridSize = config.INITIAL_GRID_SIZE

    -- Calculate offset: how far into the level our current view is
    -- We start centered in the level
    levels.updateOffset()

    -- Clear all transient game state
    state.snake = {}
    state.detachedSegments = {}
    state.foodPickups = {}

    -- Reset snake at center of current grid
    local centerX = math.floor(state.gridSize / 2)
    local centerZ = math.floor(state.gridSize / 2)

    local headColor = {
        config.SNAKE_HEAD_COLOR[1] / 255,
        config.SNAKE_HEAD_COLOR[2] / 255,
        config.SNAKE_HEAD_COLOR[3] / 255,
        1.0
    }
    table.insert(state.snake, types.SnakeSegment(types.GridPos(centerX, centerZ), headColor))

    local bodyColor = {
        config.SNAKE_BODY_COLOR[1] / 255,
        config.SNAKE_BODY_COLOR[2] / 255,
        config.SNAKE_BODY_COLOR[3] / 255,
        1.0
    }
    table.insert(state.snake, types.SnakeSegment(types.GridPos(centerX - 1, centerZ), bodyColor))
    table.insert(state.snake, types.SnakeSegment(types.GridPos(centerX - 2, centerZ), bodyColor))

    -- Build visible obstacles from level walls
    levels.rebuildVisibleObstacles()

    -- Reset game state
    state.direction = Direction.Right
    state.nextDirection = Direction.Right
    state.foodCollected = 0
    state.score = 0
    state.gameOver = false
    state.moveInterval = config.INITIAL_MOVE_INTERVAL

    -- Spawn enemies from zones (only those in current view)
    enemies.spawnFromZones()

    -- Spawn initial food
    game_logic.spawnFood()
end

-- Update level offset based on current grid size
function levels.updateOffset()
    -- Offset from current grid origin to level origin
    -- As grid expands, offset decreases (we see more of the level)
    state.levelOffsetX = math.floor((state.levelMaxSize - state.gridSize) / 2)
    state.levelOffsetZ = math.floor((state.levelMaxSize - state.gridSize) / 2)
end

-- Rebuild visible obstacles from level walls
function levels.rebuildVisibleObstacles()
    state.obstacles = {}

    -- Convert level walls to current grid coordinates
    -- Only include walls that are within current grid bounds
    for _, wall in ipairs(state.currentLevel.walls) do
        -- Transform from level coords to current grid coords
        local gridX = wall.x - state.levelOffsetX
        local gridZ = wall.z - state.levelOffsetZ

        -- Only include if within current playable area
        if gridX >= 0 and gridX < state.gridSize and gridZ >= 0 and gridZ < state.gridSize then
            table.insert(state.obstacles, types.GridPos(gridX, gridZ))
        end
    end
end

-- Update world progress after level completion
function levels.updateWorldProgress()
    -- Ensure we have progress for this world
    while #state.saveData.worldProgress <= state.currentWorldIndex do
        local wp = types.WorldProgress()
        -- Assume 4 levels per world
        for i = 1, 4 do
            table.insert(wp.levelsCompleted, false)
            table.insert(wp.foodCollected, 0)
        end
        table.insert(state.saveData.worldProgress, wp)
    end

    local worldProg = state.saveData.worldProgress[state.currentWorldIndex + 1]

    -- Mark level as completed
    if state.currentLevelIndex < #worldProg.levelsCompleted then
        worldProg.levelsCompleted[state.currentLevelIndex + 1] = true
        worldProg.foodCollected[state.currentLevelIndex + 1] = state.foodCollected
    end

    -- Update total food earned in world
    worldProg.totalFoodEarned = 0
    for _, food in ipairs(worldProg.foodCollected) do
        worldProg.totalFoodEarned = worldProg.totalFoodEarned + food
    end
end

-- Proceed to next level
function levels.proceedToNextLevel()
    state.currentLevelIndex = state.currentLevelIndex + 1

    -- Check if we've completed all levels in the world
    if state.currentLevelIndex >= #state.currentWorld.levelFiles then
        -- World complete! Go to world map
        local game = require("main")
        game.transitionTo(GamePhase.WorldMap)
        return
    end

    -- Load next level
    local levelPath = "data/worlds/world1/" .. state.currentWorld.levelFiles[state.currentLevelIndex + 1]
    if levels.loadLevel(levelPath) then
        levels.applyToGame()
        local game = require("main")
        game.transitionTo(GamePhase.Playing)
    end
end

-- Return to world map
function levels.returnToWorldMap()
    local game = require("main")
    game.transitionTo(GamePhase.WorldMap)
end

-- Get total food collected in current world
function levels.getTotalFoodInCurrentWorld()
    if state.currentWorldIndex < #state.saveData.worldProgress then
        return state.saveData.worldProgress[state.currentWorldIndex + 1].totalFoodEarned
    end
    return 0
end

-- Check if a level is unlocked
function levels.isLevelUnlocked(levelIndex)
    if levelIndex == 0 then return true end  -- First level always unlocked

    if levelIndex >= #state.currentWorld.unlockRequirements then
        return false
    end

    local required = state.currentWorld.unlockRequirements[levelIndex + 1]
    return levels.getTotalFoodInCurrentWorld() >= required
end

-- Update world map node statuses
function levels.updateNodeStatuses()
    for i, node in ipairs(state.currentWorld.nodes) do
        node.isUnlocked = levels.isLevelUnlocked(node.levelIndex)

        -- Check if level is completed
        if state.currentWorldIndex < #state.saveData.worldProgress then
            local worldProg = state.saveData.worldProgress[state.currentWorldIndex + 1]
            if node.levelIndex < #worldProg.levelsCompleted then
                node.isCompleted = worldProg.levelsCompleted[node.levelIndex + 1]
            end
        end
    end
end

-- Update world map animation
function levels.updateWorldMap(dt)
    -- Animate cursor bob
    state.worldMapCursorBob = state.worldMapCursorBob + dt * 4.0

    -- Update camera to look at selected node
    if #state.currentWorld.nodes > 0 and state.selectedNodeIndex < #state.currentWorld.nodes then
        local node = state.currentWorld.nodes[state.selectedNodeIndex + 1]
        local targetX = node.gridPos.x
        local targetZ = node.gridPos.z

        -- Smooth camera follow
        state.worldMapCameraTarget.x = state.worldMapCameraTarget.x +
            (targetX - state.worldMapCameraTarget.x) * dt * 3.0
        state.worldMapCameraTarget.z = state.worldMapCameraTarget.z +
            (targetZ - state.worldMapCameraTarget.z) * dt * 3.0
    end
end

return levels

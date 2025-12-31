-- games/snake3d/modules/rendering.lua
-- All rendering functions
--
-- Matches C++ rendering from snake.game.cppm lines 2712-3434

local Constants = bestow.include("modules/constants")
local GamePhase = bestow.include("modules/phases")
local Grid = bestow.include("modules/grid")
local Log = bestow.include("modules/log")
local PixelText = bestow.include("modules/pixeltext")

local Rendering = {}

local log = Log.category("Rendering")

-- Cached mesh and material handles
local cubeMesh = nil
local materialCache = {}

-- Initialize rendering system
function Rendering.init()
    log.info("Initializing rendering system")

    -- Create cube mesh (0.85 * CELL_SIZE)
    local cubeSize = Constants.CELL_SIZE * 0.85
    cubeMesh = bestow.graphics.createCubeMesh(cubeSize)
    if cubeMesh and cubeMesh ~= 0 then
        log.debug("Created cube mesh: %d (size=%.2f)", cubeMesh, cubeSize)
    else
        log.error("Failed to create cube mesh!")
        cubeMesh = nil
    end

    -- Setup directional light (matches C++ snake.game.cppm)
    -- Parameters: dx, dy, dz, r, g, b, intensity, castShadows
    if bestow.graphics.setDirectionalLight then
        bestow.graphics.setDirectionalLight(
            Constants.LIGHT_DIRECTION[1],
            Constants.LIGHT_DIRECTION[2],
            Constants.LIGHT_DIRECTION[3],
            Constants.LIGHT_COLOR[1],
            Constants.LIGHT_COLOR[2],
            Constants.LIGHT_COLOR[3],
            Constants.LIGHT_INTENSITY,
            true  -- castShadows
        )
        log.debug("Set directional light: dir=(%.1f, %.1f, %.1f), intensity=%.2f",
            Constants.LIGHT_DIRECTION[1], Constants.LIGHT_DIRECTION[2],
            Constants.LIGHT_DIRECTION[3], Constants.LIGHT_INTENSITY)
    end

    -- Setup ambient light
    if bestow.graphics.setAmbientLight then
        bestow.graphics.setAmbientLight(
            Constants.AMBIENT_COLOR[1],
            Constants.AMBIENT_COLOR[2],
            Constants.AMBIENT_COLOR[3]
        )
        log.debug("Set ambient light: color=(%.2f, %.2f, %.2f)",
            Constants.AMBIENT_COLOR[1], Constants.AMBIENT_COLOR[2], Constants.AMBIENT_COLOR[3])
    end

    log.info("Rendering system initialized")
end

-- Rebuild ground for current grid size (no-op, ground is drawn with lines)
function Rendering.rebuildGroundMesh(gridSize)
    -- Ground is drawn using grid lines, not a mesh
    -- This function exists for compatibility but does nothing
    log.debug("Ground grid size set to %d (drawn with lines)", gridSize)
end

-- Create or get cached material
function Rendering.getMaterial(color, roughness, metallic, emissive)
    roughness = roughness or 0.5
    metallic = metallic or 0.0

    -- Create cache key from color and material properties
    local r = math.floor((color[1] or 1) * 1000)
    local g = math.floor((color[2] or 1) * 1000)
    local b = math.floor((color[3] or 1) * 1000)
    local a = math.floor((color[4] or 1) * 1000)
    local rough = math.floor(roughness * 100)
    local metal = math.floor(metallic * 100)
    local key = string.format("%d_%d_%d_%d_%d_%d", r, g, b, a, rough, metal)

    if materialCache[key] then
        return materialCache[key]
    end

    local params = {
        color = color,
        roughness = roughness,
        metallic = metallic
    }
    if emissive then
        params.emissive = emissive
    end

    local handle = bestow.graphics.createMaterial(params)
    materialCache[key] = handle
    return handle
end

-- Draw a cube at position with material
local function drawCube(x, y, z, scale, color, roughness, metallic, emissive)
    if not cubeMesh or cubeMesh == 0 then return end

    local mat = Rendering.getMaterial(color, roughness, metallic, emissive)
    if mat and mat ~= 0 then
        bestow.graphics.drawMesh(cubeMesh, mat, x, y, z, scale, true, true)
    end
end

-- Main draw function - dispatches based on phase
function Rendering.draw(game)
    log.trace("Drawing frame, phase=%s", GamePhase.getName(game.currentPhase))

    if game.currentPhase == GamePhase.MainMenu then
        Rendering.drawMainMenu(game)
        return
    end

    if game.currentPhase == GamePhase.WorldMap then
        Rendering.drawWorldMap(game)
        return
    end

    if game.currentPhase == GamePhase.Paused then
        -- Draw frozen game underneath
        Rendering.drawGround(game)
        Rendering.drawObstacles(game)
        Rendering.drawEnemies(game)
        Rendering.drawSnake(game)
        Rendering.drawDetachedSegments(game)
        Rendering.drawFoodPickups(game)
        Rendering.drawFood(game)
        Rendering.drawGridBorder(game)
        -- Draw pause menu overlay
        Rendering.drawPauseMenu(game)
        return
    end

    -- Playing, LevelComplete, GameOver phases
    Rendering.drawGround(game)
    Rendering.drawObstacles(game)
    Rendering.drawEnemies(game)
    Rendering.drawSnake(game)
    Rendering.drawDetachedSegments(game)
    Rendering.drawFoodPickups(game)
    Rendering.drawFood(game)
    Rendering.drawFoodPopEffect(game)
    Rendering.drawGridBorder(game)
    Rendering.drawHUD(game)
end

-- Draw ground plane (grid lines)
function Rendering.drawGround(game)
    local halfGrid = game.gridSize / 2.0 * Constants.CELL_SIZE
    local step = Constants.CELL_SIZE
    local y = 0

    -- Draw grid lines on the ground
    local color = Constants.COLORS.GROUND
    for i = -game.gridSize / 2, game.gridSize / 2 do
        local x = i * step
        bestow.graphics.drawLine(x, y, -halfGrid, x, y, halfGrid, color)
        bestow.graphics.drawLine(-halfGrid, y, x, halfGrid, y, x, color)
    end

    log.trace("Drew ground grid (%d lines)", (game.gridSize + 1) * 2)
end

-- Draw snake segments
function Rendering.drawSnake(game)
    if #game.snake == 0 then return end

    log.trace("Drawing %d snake segments", #game.snake)

    for i, segment in ipairs(game.snake) do
        local worldPos = Grid.toWorld(game.gridSize, segment.pos)

        -- Head slightly bigger
        local scale = (i == 1) and 1.0 or 0.9

        -- Get color from segment or default
        local color = segment.color or Constants.COLORS.SNAKE_HEAD

        -- Emissive (20% of base color)
        local emissive = {
            color[1] * 0.2,
            color[2] * 0.2,
            color[3] * 0.2
        }

        drawCube(
            worldPos.x, worldPos.y, worldPos.z,
            scale,
            color,
            Constants.PBR.SNAKE.roughness,
            Constants.PBR.SNAKE.metallic,
            emissive
        )
    end
end

-- Draw obstacles/walls
function Rendering.drawObstacles(game)
    if #game.obstacles == 0 then return end

    log.trace("Drawing %d obstacles", #game.obstacles)

    for _, obstacle in ipairs(game.obstacles) do
        local worldPos = Grid.toWorld(game.gridSize, obstacle)

        -- Flat rectangles (0.4x height)
        drawCube(
            worldPos.x, worldPos.y * 0.4, worldPos.z,
            0.9,
            Constants.COLORS.OBSTACLE,
            Constants.PBR.OBSTACLE.roughness,
            Constants.PBR.OBSTACLE.metallic
        )
    end
end

-- Draw enemies
function Rendering.drawEnemies(game)
    if #game.enemies == 0 then return end

    log.trace("Drawing %d enemies", #game.enemies)

    local Enemies = bestow.include("modules/enemies")

    for _, enemy in ipairs(game.enemies) do
        local worldPos = Enemies.getWorldPos(game, enemy)

        -- Determine color (flash white when damaged)
        local color
        if enemy.isDamaged then
            local flash = math.sin(enemy.damageFlashTimer * Constants.DAMAGE_FLASH_FREQUENCY) > 0
            if flash then
                color = Constants.COLORS.ENEMY_FLASH
            else
                color = {1.0, 0.5, 0.5, 1.0}  -- Pink
            end
        else
            color = Constants.COLORS.ENEMY
        end

        -- Enemies are 0.8x size
        drawCube(
            worldPos.x, worldPos.y, worldPos.z,
            0.8,
            color,
            Constants.PBR.ENEMY.roughness,
            Constants.PBR.ENEMY.metallic
        )

        -- Draw health bar if visible and multi-health
        if enemy.showHealthBar and enemy.maxHealth > 1 then
            Rendering.drawEnemyHealthBar(game, enemy, worldPos)
        end
    end
end

-- Draw enemy health bar
function Rendering.drawEnemyHealthBar(game, enemy, worldPos)
    local healthPercent = enemy.currentHealth / enemy.maxHealth
    local barWidth = 0.8
    local y = worldPos.y + 0.7

    -- Background (dark)
    bestow.graphics.drawLine(
        worldPos.x - barWidth/2, y, worldPos.z,
        worldPos.x + barWidth/2, y, worldPos.z,
        {0.16, 0.16, 0.16, 0.8}
    )

    -- Health bar (green to red gradient based on health)
    local r = 1 - healthPercent
    local g = healthPercent
    bestow.graphics.drawLine(
        worldPos.x - barWidth/2, y + 0.02, worldPos.z,
        worldPos.x - barWidth/2 + barWidth * healthPercent, y + 0.02, worldPos.z,
        {r, g, 0.2, 1.0}
    )
end

-- Draw detached segments (explosions, chain breaks)
function Rendering.drawDetachedSegments(game)
    if #game.detachedSegments == 0 then return end

    log.trace("Drawing %d detached segments", #game.detachedSegments)

    for _, seg in ipairs(game.detachedSegments) do
        local worldPos = seg.worldPos
        local scale = seg.scale

        -- Fade and ember effect near end of life
        local lifePercent = seg.timer / Constants.PARTICLE_LIFE_MAX
        local color = seg.color

        if lifePercent < 0.3 then
            -- Ember glow effect
            local emberBlend = 1 - (lifePercent / 0.3)
            color = {
                seg.color[1] * (1 - emberBlend) + Constants.COLORS.EMBER_GLOW[1] * emberBlend,
                seg.color[2] * (1 - emberBlend) + Constants.COLORS.EMBER_GLOW[2] * emberBlend,
                seg.color[3] * (1 - emberBlend) + Constants.COLORS.EMBER_GLOW[3] * emberBlend,
                1.0
            }
            scale = seg.scale * lifePercent / 0.3  -- Shrink
        end

        drawCube(
            worldPos.x, worldPos.y, worldPos.z,
            scale,
            color,
            Constants.PBR.SNAKE.roughness,
            Constants.PBR.SNAKE.metallic
        )
    end
end

-- Draw food pickups (bonus items)
function Rendering.drawFoodPickups(game)
    if not game.foodPickups or #game.foodPickups == 0 then return end

    log.trace("Drawing %d food pickups", #game.foodPickups)

    for _, pickup in ipairs(game.foodPickups) do
        local worldPos = Grid.toWorld(game.gridSize, pickup.pos)

        -- Pop-in animation
        local age = game.gameTime - pickup.spawnTime
        local spawnScale = math.min(1.0, age * 5.0)

        -- Gentle bounce
        local bounce = 0.1 * math.abs(math.sin(age * 5.0))

        local emissive = {
            pickup.color[1] * Constants.PBR.FOOD_PICKUP.emissive,
            pickup.color[2] * Constants.PBR.FOOD_PICKUP.emissive,
            pickup.color[3] * Constants.PBR.FOOD_PICKUP.emissive
        }

        local scale = spawnScale * 0.7

        drawCube(
            worldPos.x, worldPos.y + bounce, worldPos.z,
            scale,
            pickup.color,
            Constants.PBR.FOOD_PICKUP.roughness,
            Constants.PBR.FOOD_PICKUP.metallic,
            emissive
        )
    end
end

-- Draw main food
function Rendering.drawFood(game)
    -- Don't draw in level complete phase
    if game.currentPhase == GamePhase.LevelComplete then
        return
    end

    if not game.foodPos then return end

    local worldPos = Grid.toWorld(game.gridSize, game.foodPos)

    -- Pulsing effect
    local pulse = 0.85 + 0.15 * math.sin(game.gameTime * Constants.FOOD_PULSE_SPEED)

    -- Bounce effect
    local bounce = 0.15 * math.abs(math.sin(game.gameTime * Constants.FOOD_BOUNCE_SPEED))

    local color = game.foodColor or {1.0, 0.85, 0.2, 1.0}

    local emissive = {
        color[1] * Constants.PBR.FOOD.emissive,
        color[2] * Constants.PBR.FOOD.emissive,
        color[3] * Constants.PBR.FOOD.emissive
    }

    drawCube(
        worldPos.x, worldPos.y + bounce, worldPos.z,
        pulse,
        color,
        Constants.PBR.FOOD.roughness,
        Constants.PBR.FOOD.metallic,
        emissive
    )

    log.trace("Drew food at (%.2f, %.2f, %.2f) pulse=%.2f bounce=%.2f",
        worldPos.x, worldPos.y + bounce, worldPos.z, pulse, bounce)
end

-- Draw food pop burst effect
function Rendering.drawFoodPopEffect(game)
    if not game.foodPopEffect.active then return end

    local progress = game.foodPopEffect.time / game.foodPopEffect.maxTime
    local radius = 0.3 + progress * 1.5
    local alpha = 1 - progress

    local popColor = {
        Constants.COLORS.FOOD_POP[1] * alpha,
        Constants.COLORS.FOOD_POP[2] * alpha,
        Constants.COLORS.FOOD_POP[3] * alpha,
        alpha
    }

    local pos = game.foodPopEffect.pos
    local numRays = 8

    for i = 0, numRays - 1 do
        local angle = (i / numRays) * math.pi * 2
        local dx = math.cos(angle) * radius
        local dz = math.sin(angle) * radius

        local innerRadius = radius * 0.3
        local innerDx = math.cos(angle) * innerRadius
        local innerDz = math.sin(angle) * innerRadius

        bestow.graphics.drawLine(
            pos.x + innerDx, pos.y + 0.5, pos.z + innerDz,
            pos.x + dx, pos.y + 0.5, pos.z + dz,
            popColor
        )
    end

    log.trace("Drew food pop effect (progress=%.2f, radius=%.2f)", progress, radius)
end

-- Draw grid border
function Rendering.drawGridBorder(game)
    local halfGrid = Grid.getHalfSize(game.gridSize)

    local borderColor = Constants.COLORS.BORDER
    local y = 0.02

    -- Four edges of current grid
    bestow.graphics.drawLine(-halfGrid, y, -halfGrid, halfGrid, y, -halfGrid, borderColor)
    bestow.graphics.drawLine(halfGrid, y, -halfGrid, halfGrid, y, halfGrid, borderColor)
    bestow.graphics.drawLine(halfGrid, y, halfGrid, -halfGrid, y, halfGrid, borderColor)
    bestow.graphics.drawLine(-halfGrid, y, halfGrid, -halfGrid, y, -halfGrid, borderColor)

    -- Expansion border (glowing)
    if game.isExpanding and game.pendingGridSize then
        local newHalfGrid = Grid.getHalfSize(game.pendingGridSize)
        local pulse = math.sin(game.expansionTimer * 15) * 0.5 + 0.5
        local brightness = 0.7 + pulse * 0.3

        local glowColor = {brightness, brightness, brightness * 0.6, 1.0}
        local gy = 0.03

        bestow.graphics.drawLine(-newHalfGrid, gy, -newHalfGrid, newHalfGrid, gy, -newHalfGrid, glowColor)
        bestow.graphics.drawLine(newHalfGrid, gy, -newHalfGrid, newHalfGrid, gy, newHalfGrid, glowColor)
        bestow.graphics.drawLine(newHalfGrid, gy, newHalfGrid, -newHalfGrid, gy, newHalfGrid, glowColor)
        bestow.graphics.drawLine(-newHalfGrid, gy, newHalfGrid, -newHalfGrid, gy, -newHalfGrid, glowColor)
    end
end

-- Draw HUD elements
-- Matches C++ HUD at lines 3171-3188
function Rendering.drawHUD(game)
    local halfGrid = Grid.getHalfSize(game.gridSize)

    -- Food progress bar
    local foodBarWidth = game.gridSize * Constants.CELL_SIZE * 0.6
    local foodProgress = game.foodCollected / game.foodRequired
    local hudY = 0.1
    local hudZ = -halfGrid - 0.5

    -- Background
    bestow.graphics.drawLine(
        -foodBarWidth * 0.5, hudY, hudZ,
        foodBarWidth * 0.5, hudY, hudZ,
        {0.2, 0.2, 0.2, 1.0}
    )

    -- Foreground (gold)
    bestow.graphics.drawLine(
        -foodBarWidth * 0.5, hudY + 0.02, hudZ,
        -foodBarWidth * 0.5 + foodBarWidth * foodProgress, hudY + 0.02, hudZ,
        {1.0, 0.8, 0.2, 1.0}
    )

    -- ==================== HUD TEXT LABELS (Pixel Font) ====================
    if game.currentPhase == GamePhase.Playing and not game.gameOver then
        -- Food counter text (above the food bar) - e.g., "3/5"
        local foodText = tostring(game.foodCollected) .. "/" .. tostring(game.foodRequired)
        PixelText.drawShadow(foodText, 0.0, hudY + 0.6, hudZ - 0.3, 0.04,
                             {1.0, 0.78, 0.2, 1.0})

        -- Snake length counter
        local segmentBarX = halfGrid + 0.5
        local segmentBarHeight = 2.0
        local segmentText = tostring(#game.snake)
        PixelText.drawShadow(segmentText, segmentBarX - 0.3, hudY + segmentBarHeight + 0.3, -halfGrid, 0.03,
                             {0.4, 1.0, 0.4, 1.0})

        -- Level name (if available)
        if game.currentLevel and game.currentLevel.name and game.currentLevel.name ~= "" then
            PixelText.drawShadow(game.currentLevel.name, 0.0, 3.0, -halfGrid - 0.5, 0.05,
                                 {0.78, 0.78, 1.0, 1.0})
        end
    end

    -- Level complete overlay
    if game.currentPhase == GamePhase.LevelComplete and #game.snake > 0 then
        Rendering.drawLevelCompleteOverlay(game)
    end

    -- Game over overlay
    if game.gameOver then
        Rendering.drawGameOverOverlay(game)
    end
end

-- Draw level complete victory overlay
function Rendering.drawLevelCompleteOverlay(game)
    local halfGrid = Grid.getHalfSize(game.gridSize)

    -- Pulsing golden border
    local pulse = math.sin(game.gameTime * 4) * 0.5 + 0.5
    local borderColor = {1.0, 0.78 + pulse * 0.22, 0.2 + pulse * 0.2, 1.0}

    local y = 0.5

    bestow.graphics.drawLine(-halfGrid, y, -halfGrid, halfGrid, y, -halfGrid, borderColor)
    bestow.graphics.drawLine(halfGrid, y, -halfGrid, halfGrid, y, halfGrid, borderColor)
    bestow.graphics.drawLine(halfGrid, y, halfGrid, -halfGrid, y, halfGrid, borderColor)
    bestow.graphics.drawLine(-halfGrid, y, halfGrid, -halfGrid, y, -halfGrid, borderColor)

    -- Green checkmark above snake head
    if #game.snake > 0 then
        local headPos = Grid.toWorld(game.gridSize, game.snake[1].pos)
        local checkY = headPos.y + 2

        -- Draw checkmark using lines
        local checkColor = {0.2, 1.0, 0.2, 1.0}
        bestow.graphics.drawLine(
            headPos.x - 0.3, checkY, headPos.z,
            headPos.x, checkY - 0.3, headPos.z,
            checkColor
        )
        bestow.graphics.drawLine(
            headPos.x, checkY - 0.3, headPos.z,
            headPos.x + 0.5, checkY + 0.3, headPos.z,
            checkColor
        )
    end

    log.trace("Drew level complete overlay")
end

-- Draw game over overlay
-- Matches C++ game over at lines 3153-3168
function Rendering.drawGameOverOverlay(game)
    local halfGrid = Grid.getHalfSize(game.gridSize)

    -- Flashing red X
    local flash = math.sin(game.gameTime * 4.0) * 0.5 + 0.5
    local brightness = 0.4 + flash * 0.6
    local xColor = {brightness, 0.12, 0.12, 1.0}
    local y = 0.5

    local xSize = halfGrid * 0.7

    -- Diagonal lines forming X
    bestow.graphics.drawLine(-xSize, y, -xSize, xSize, y, xSize, xColor)
    bestow.graphics.drawLine(xSize, y, -xSize, -xSize, y, xSize, xColor)

    -- Game Over text using pixel font
    PixelText.drawShadow("GAME OVER", 0.0, 2.5, 0.0, 0.12,
                         {1.0, 0.4, 0.4, 1.0})  -- Red

    PixelText.drawShadow("PRESS R TO RESTART", 0.0, 1.5, 0.0, 0.04,
                         {0.78, 0.78, 0.78, 1.0})  -- Gray
end

-- Draw decorative coiled serpent (matches C++ exactly)
local function drawMenuSnake(game)
    if not cubeMesh or cubeMesh == 0 then return end

    local snakeAnim = game.gameTime * 0.8
    local coilSegments = 20

    for i = 0, coilSegments - 1 do
        local t = i / coilSegments
        local angle = t * 6.28 * 2.5 + snakeAnim  -- 2.5 coils
        local radius = 2.0 + t * 1.5  -- Spiral outward
        local x = math.cos(angle) * radius
        local z = math.sin(angle) * radius
        local y = 0.4 + math.sin(snakeAnim * 2.0 + t * 6.28) * 0.1

        local green = 0.9 - t * 0.4
        local color = {0.15, green, 0.2, 1.0}
        local emissive = {0.0, green * 0.1, 0.0}

        local segmentScale = 0.6 - t * 0.2  -- Smaller toward tail

        drawCube(x, y, z, segmentScale, color, 0.4, 0.1, emissive)
    end
end

-- Draw menu option cubes (matches C++ exactly)
local function drawMenuOptions(game)
    if not cubeMesh or cubeMesh == 0 then return end

    local menuZ = 5.0
    local menuSpacing = 2.0

    for i = 0, Constants.MAIN_MENU_COUNT - 1 do
        local z = menuZ + i * menuSpacing
        local selected = (i == game.mainMenuSelection)

        -- 4 cubes per option
        local numCubes = 4
        local cubeSpacing = 1.2
        local startX = -(numCubes - 1) * cubeSpacing * 0.5

        for c = 0, numCubes - 1 do
            local x = startX + c * cubeSpacing
            local y = 0.5

            -- Animate selected row
            if selected then
                local bounce = math.sin(game.gameTime * 4.0 + c * 0.5) * 0.2
                y = y + bounce + 0.3
            end

            local color, emissive
            if selected then
                -- Bright golden color for selected
                local pulse = math.sin(game.gameTime * 3.0) * 0.2 + 0.8
                color = {1.0, 0.8 * pulse, 0.2, 1.0}
                emissive = {0.3, 0.2, 0.0}
            else
                -- Dim gray for unselected
                color = {0.4, 0.4, 0.5, 1.0}
                emissive = nil
            end

            local scale = selected and 0.9 or 0.7
            drawCube(x, y, z, scale, color, 0.5, 0.2, emissive)
        end

        -- Draw selection arrows on sides
        if selected then
            local arrowX = startX - 1.5
            local arrowPulse = math.sin(game.gameTime * 5.0) * 0.3

            local arrowColor = {1.0, 1.0, 0.3, 1.0}
            local arrowEmissive = {0.5, 0.5, 0.0}

            -- Left arrow (3 cubes)
            for a = 0, 2 do
                local ax = arrowX - a * 0.4 + arrowPulse
                drawCube(ax, 0.5, z, 0.4, arrowColor, 0.3, 0.2, arrowEmissive)
            end

            -- Right arrow
            arrowX = startX + (numCubes - 1) * cubeSpacing + 1.5
            for a = 0, 2 do
                local ax = arrowX + a * 0.4 - arrowPulse
                drawCube(ax, 0.5, z, 0.4, arrowColor, 0.3, 0.2, arrowEmissive)
            end
        end
    end
end

-- Draw title cubes (matches C++ exactly)
local function drawTitleCubes(game)
    if not cubeMesh or cubeMesh == 0 then return end

    local titleZ = -2.0
    local titleCubes = 7
    local titleSpacing = 0.8
    local titleStartX = -(titleCubes - 1) * titleSpacing * 0.5

    for t = 0, titleCubes - 1 do
        local x = titleStartX + t * titleSpacing
        local y = 0.5 + math.sin(game.gameTime * 2.0 + t * 0.4) * 0.15

        local hue = t / titleCubes
        local color = {0.2 + hue * 0.3, 0.8, 0.3 + (1.0 - hue) * 0.3, 1.0}
        local emissive = {0.1, 0.2, 0.1}

        drawCube(x, y, titleZ, 0.6, color, 0.4, 0.1, emissive)
    end
end

-- Draw control hint cubes (matches C++ exactly)
local function drawHintCubes(game)
    if not cubeMesh or cubeMesh == 0 then return end

    local hintX = 6.0
    local hintZ = 3.0
    local hintColor = {0.6, 0.6, 0.7, 1.0}

    -- Vertical arrow (up/down navigation) - 3 cubes
    drawCube(hintX, 0.3, hintZ - 0.5, 0.3, hintColor, 0.5, 0.1)
    drawCube(hintX, 0.3, hintZ, 0.3, hintColor, 0.5, 0.1)
    drawCube(hintX, 0.3, hintZ + 0.5, 0.3, hintColor, 0.5, 0.1)
end

-- Draw dark ground grid for menu (using lines since we don't have a plane mesh)
local function drawMenuGround(game)
    -- Draw ground as a grid of dark green lines
    local groundColor = {0.05, 0.08, 0.05, 1.0}
    local size = 12
    local y = -0.1

    for i = -size, size do
        local pos = i * 1.0
        bestow.graphics.drawLine(pos, y, -size, pos, y, size, groundColor)
        bestow.graphics.drawLine(-size, y, pos, size, y, pos, groundColor)
    end
end

-- Draw main menu (matches C++ exactly)
function Rendering.drawMainMenu(game)
    -- Draw ground plane
    drawMenuGround(game)

    -- Draw decorative coiled serpent
    drawMenuSnake(game)

    -- Draw menu option cubes
    drawMenuOptions(game)

    -- Draw title cubes
    drawTitleCubes(game)

    -- Draw control hint cubes
    drawHintCubes(game)

    -- ==================== TEXT LABELS (Pixel Font) ====================
    -- Title text - "SERPENT" (matches C++ line 3368)
    local titleZ = -3.0
    local titlePixelSize = 0.15
    PixelText.drawShadow("SERPENT", 0.0, 2.0, titleZ, titlePixelSize,
                         {0.4, 1.0, 0.6, 1.0})  -- Light green

    -- Menu option labels (matches C++ lines 3372-3384)
    local menuLabels = {"PLAY", "QUIT"}
    local menuZ = 0.0
    local menuSpacing = 2.0

    for i = 0, Constants.MAIN_MENU_COUNT - 1 do
        local z = menuZ + i * menuSpacing
        local selected = (i == game.mainMenuSelection)

        local textColor
        if selected then
            textColor = {1.0, 0.86, 0.4, 1.0}   -- Gold for selected
        else
            textColor = {0.7, 0.7, 0.78, 1.0}   -- Gray for unselected
        end

        local textY = selected and 1.5 or 1.2
        local pixelSize = selected and 0.08 or 0.05

        PixelText.drawShadow(menuLabels[i + 1], 0.0, textY, z, pixelSize, textColor)
    end

    -- Controls hint text (matches C++ line 3388)
    local hintX = 4.0
    local hintZ = 2.0
    PixelText.draw(",O SELECT", hintX, 0.8, hintZ + 1.5, 0.03,
                   {0.6, 0.6, 0.67, 1.0}, false)  -- Not centered
end

-- Draw world map
function Rendering.drawWorldMap(game)
    local y = 0.1

    -- Draw level nodes in a path
    local pathColor = {0.4, 0.6, 0.4, 0.6}

    -- Draw connecting path
    bestow.graphics.drawLine(-4, y, 0, 0, y, 0, pathColor)
    bestow.graphics.drawLine(0, y, 0, 4, y, 0, pathColor)

    -- Draw level cubes
    local positions = {{-4, 0, 0}, {0, 0, 0}, {4, 0, 0}}

    for i, pos in ipairs(positions) do
        local bounce = 0.1 * math.sin(game.gameTime * 3.0 + i)
        local scale = (i == 1) and 0.8 or 0.6  -- First level is bigger (selected)
        local color = (i == 1) and {0.3, 0.9, 0.4, 1.0} or {0.2, 0.5, 0.3, 1.0}
        local emissive = {color[1] * 0.3, color[2] * 0.3, color[3] * 0.3}

        drawCube(pos[1], 0.5 + bounce, pos[3], scale, color, 0.4, 0.1, emissive)
    end

    -- Draw ground grid
    local gridColor = {0.1, 0.25, 0.15, 0.4}
    for i = -5, 5 do
        local pos = i * 1.5
        bestow.graphics.drawLine(pos, 0, -3, pos, 0, 3, gridColor)
        bestow.graphics.drawLine(-7.5, 0, pos * 0.4, 7.5, 0, pos * 0.4, gridColor)
    end

    log.trace("Drew world map")
end

-- Draw pause menu overlay
-- Matches C++ drawPauseMenu at lines 3392-3431
function Rendering.drawPauseMenu(game)
    local halfGrid = Grid.getHalfSize(game.gridSize)

    -- Dark overlay lines
    local overlayColor = {0.08, 0.08, 0.12, 0.8}
    for z = -halfGrid, halfGrid, 0.5 do
        bestow.graphics.drawLine(-halfGrid, 0.1, z, halfGrid, 0.1, z, overlayColor)
    end

    -- Menu in world space, centered on camera target
    local menuY = 2.0
    local menuSpacing = 1.0
    local cx = game.cameraTarget.x
    local cz = game.cameraTarget.z

    local options = {"RESUME", "RESTART", "QUIT"}

    for i = 0, Constants.PAUSE_MENU_COUNT - 1 do
        local y = menuY - i * menuSpacing
        local selected = (i == game.pauseMenuSelection)

        -- Selection indicator lines
        if selected then
            local pulse = math.sin(game.gameTime * 4.0) * 0.1 + 0.9
            local selectColor = {1.0, 0.78, 0.2, 1.0}
            bestow.graphics.drawLine(cx - 2.5 * pulse, y, cz, cx - 1.5, y, cz, selectColor)
            bestow.graphics.drawLine(cx + 1.5, y, cz, cx + 2.5 * pulse, y, cz, selectColor)
        end

        -- Draw text label using pixel font
        local textColor
        if selected then
            textColor = {1.0, 0.86, 0.4, 1.0}   -- Gold
        else
            textColor = {0.6, 0.6, 0.63, 1.0}   -- Gray
        end

        local pixelSize = selected and 0.06 or 0.04
        PixelText.drawShadow(options[i + 1], cx, y, cz, pixelSize, textColor)
    end

    -- PAUSED title
    PixelText.drawShadow("PAUSED", cx, menuY + 1.5, cz, 0.1,
                         {1.0, 1.0, 0.4, 1.0})  -- Yellow
end

-- Convert grid position to world for external use
function Rendering.gridToWorld(game, pos)
    return Grid.toWorld(game.gridSize, pos)
end

return Rendering

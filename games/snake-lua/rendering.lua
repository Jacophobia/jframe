-- rendering.lua - All drawing functions
-- Dependencies: app.config, app.state, app.types (accessed inside functions)

local rendering = {}

-- Convert grid position to world position
function rendering.gridToWorld(pos)
    local config = app.config
    local state = app.state
    local halfGrid = state.gridSize * config.CELL_SIZE * 0.5
    return Vec3.new(
        pos.x * config.CELL_SIZE - halfGrid + config.CELL_SIZE * 0.5,
        config.CELL_SIZE * 0.5,
        pos.z * config.CELL_SIZE - halfGrid + config.CELL_SIZE * 0.5
    )
end

-- Convert float grid position to world position (for smooth enemy movement)
function rendering.gridToWorldFloat(x, z)
    local config = app.config
    local state = app.state
    local halfGrid = state.gridSize * config.CELL_SIZE * 0.5
    return Vec3.new(
        x * config.CELL_SIZE - halfGrid + config.CELL_SIZE * 0.5,
        config.CELL_SIZE * 0.5,
        z * config.CELL_SIZE - halfGrid + config.CELL_SIZE * 0.5
    )
end

-- Draw the ground plane
function rendering.drawGround()
    local state = app.state
    if not state.groundMesh or not state.groundMaterial then return end

    -- Ground matches current grid size exactly (instant transition)
    local scale = state.gridSize / (state.gridSize + 10)
    local groundTransform = Transform3D.new()
    groundTransform.scale = Vec3.new(scale, 1.0, scale)

    bestow.graphics3d.drawMesh(state.groundMesh, state.groundMaterial, groundTransform, false, true)
end

-- Draw the snake
function rendering.drawSnake()
    local state = app.state
    if not state.cubeMesh then return end

    for i, segment in ipairs(state.snake) do
        local worldPos = rendering.gridToWorld(segment.pos)

        -- Slight size variation - head is bigger
        local scale = (i == 1) and 1.0 or 0.9

        local transform = Transform3D.new()
        transform.position = worldPos
        transform.scale = Vec3.new(scale, scale, scale)

        -- Create material with segment's color
        local mat = PBRMaterial.new()
        mat.baseColorFactor = Color.new(
            segment.color[1],
            segment.color[2],
            segment.color[3],
            segment.color[4]
        )
        mat.roughnessFactor = 0.4
        mat.metallicFactor = 0.1

        local matHandle = bestow.graphics3d.createMaterial(mat)
        if matHandle then
            bestow.graphics3d.drawMesh(state.cubeMesh, matHandle, transform, true, true)
        end
    end
end

-- Draw obstacles (walls)
function rendering.drawObstacles()
    local state = app.state
    if not state.cubeMesh then return end

    -- Dark gray/brown obstacle color
    local obstacleMat = PBRMaterial.new()
    obstacleMat.baseColorFactor = Color.new(0.3, 0.25, 0.2, 1.0)
    obstacleMat.roughnessFactor = 0.8
    obstacleMat.metallicFactor = 0.1

    local matHandle = bestow.graphics3d.createMaterial(obstacleMat)
    if not matHandle then return end

    for _, obstacle in ipairs(state.obstacles) do
        local worldPos = rendering.gridToWorld(obstacle)

        -- Obstacles are flat and wide rectangles
        local transform = Transform3D.new()
        transform.position = Vec3.new(worldPos.x, worldPos.y * 0.5, worldPos.z)
        transform.scale = Vec3.new(0.9, 0.4, 0.9)

        bestow.graphics3d.drawMesh(state.cubeMesh, matHandle, transform, true, true)
    end
end

-- Draw enemies
function rendering.drawEnemies()
    local config = app.config
    local state = app.state
    if not state.cubeMesh then return end

    for _, enemy in ipairs(state.enemies) do
        -- Use interpolated visual position for smooth movement
        local worldPos = rendering.gridToWorldFloat(enemy.visualX, enemy.visualZ)

        -- Enemy animation - slight bobbing
        local bob = 0.05 * math.sin(state.gameTime * 3.0 + enemy.visualX * 0.5)

        local transform = Transform3D.new()
        transform.position = Vec3.new(worldPos.x, worldPos.y + bob, worldPos.z)
        transform.scale = Vec3.new(0.8, 0.8, 0.8)

        -- Enemy color: red/purple, flashing white when damaged
        local mat = PBRMaterial.new()
        if enemy.isDamaged then
            -- Flash white when damaged
            local flash = math.sin(enemy.damageFlashTimer * 30.0) > 0 and 1.0 or 0.0
            mat.baseColorFactor = Color.new(1.0, flash, flash, 1.0)
        else
            -- Normal color: red-purple
            mat.baseColorFactor = Color.new(0.8, 0.2, 0.3, 1.0)
        end
        mat.roughnessFactor = 0.5
        mat.metallicFactor = 0.2

        local matHandle = bestow.graphics3d.createMaterial(mat)
        if matHandle then
            bestow.graphics3d.drawMesh(state.cubeMesh, matHandle, transform, true, true)
        end

        -- Draw health bar if visible
        if enemy.showHealthBar and enemy.maxHealth > 1 then
            rendering.drawEnemyHealthBar(enemy, worldPos)
        end
    end
end

-- Draw enemy health bar
function rendering.drawEnemyHealthBar(enemy, worldPos)
    local config = app.config
    -- Health bar above enemy
    local barWidth = config.CELL_SIZE * 0.8
    local barHeight = worldPos.y + config.CELL_SIZE * 0.8
    local healthPercent = enemy.currentHealth / enemy.maxHealth

    -- Background (dark red)
    local bgColor = Color.new(80, 20, 20, 255)
    -- Foreground (green to yellow to red based on health)
    local r = math.floor((1.0 - healthPercent) * 255)
    local g = math.floor(healthPercent * 255)
    local fgColor = Color.new(r, g, 0, 255)

    -- Draw background bar
    local halfBar = barWidth * 0.5
    bestow.graphics3d.debugDrawLine(
        Vec3.new(worldPos.x - halfBar, barHeight, worldPos.z),
        Vec3.new(worldPos.x + halfBar, barHeight, worldPos.z),
        bgColor, 0.0, false
    )

    -- Draw foreground (health remaining)
    local healthWidth = barWidth * healthPercent
    bestow.graphics3d.debugDrawLine(
        Vec3.new(worldPos.x - halfBar, barHeight + 0.02, worldPos.z),
        Vec3.new(worldPos.x - halfBar + healthWidth, barHeight + 0.02, worldPos.z),
        fgColor, 0.0, false
    )
end

-- Draw detached segments (explosion animation)
function rendering.drawDetachedSegments()
    local config = app.config
    local state = app.state
    if not state.cubeMesh then return end

    for _, segment in ipairs(state.detachedSegments) do
        local worldPos
        local scale
        local spin

        if segment.isParticle then
            -- Particles use their own worldPos directly (updated by physics)
            worldPos = segment.worldPos

            -- Only shrink in the last second of lifetime
            local fadeStart = 1.0
            if segment.timer < fadeStart then
                scale = segment.scale * (segment.timer / fadeStart)
            else
                scale = segment.scale
            end

            -- Use angular velocity for proper tumbling
            local timeAlive = 3.0 - segment.timer
            spin = segment.angularVel * timeAlive
        else
            -- Regular detached segments use grid position with velocity offset
            worldPos = rendering.gridToWorld(segment.pos)

            local progress = 1.0 - (segment.timer / config.DETACH_ANIMATION_TIME)
            worldPos = Vec3.new(
                worldPos.x + segment.velocity.x * progress * 0.3,
                worldPos.y + segment.velocity.y * progress * 0.3,
                worldPos.z + segment.velocity.z * progress * 0.3
            )

            scale = (segment.timer / config.DETACH_ANIMATION_TIME) * 0.85
            spin = progress * 20.0
        end

        local transform = Transform3D.new()
        transform.position = worldPos
        -- Apply rotation and scale (simplified - full version would use rotation matrix)
        transform.scale = Vec3.new(scale, scale, scale)

        -- Color: particles keep their original color, regular segments flash
        local mat = PBRMaterial.new()
        local colorR, colorG, colorB = 0.5, 0.5, 0.5  -- Default color for emissive calculation
        if segment.isParticle then
            -- Particles keep segment color, fade to ember glow near end
            local fadeStart = 1.0
            local colorAlpha = segment.timer < fadeStart and (segment.timer / fadeStart) or 1.0
            local emberBlend = 1.0 - colorAlpha
            colorR = segment.color[1] * colorAlpha + 0.8 * emberBlend
            colorG = segment.color[2] * colorAlpha + 0.2 * emberBlend
            colorB = segment.color[3] * colorAlpha + 0.1 * emberBlend
            mat.baseColorFactor = Color.new(colorR, colorG, colorB, 1.0)
            mat.emissiveFactor = Vec3.new(0.2 * emberBlend, 0.05 * emberBlend, 0.0)
        elseif segment.willShatter then
            local progress = 1.0 - (segment.timer / config.DETACH_ANIMATION_TIME)
            local flash = math.sin(progress * 30.0) > 0 and 1.0 or 0.3
            colorR, colorG, colorB = flash, 0.1, 0.1
            mat.baseColorFactor = Color.new(colorR, colorG, colorB, 1.0)
        else
            colorR = segment.color[1]
            colorG = segment.color[2]
            colorB = segment.color[3]
            mat.baseColorFactor = Color.new(colorR, colorG, colorB, 1.0)
        end
        mat.roughnessFactor = 0.4
        mat.metallicFactor = 0.1
        if not segment.isParticle then
            mat.emissiveFactor = Vec3.new(colorR * 0.2, colorG * 0.2, colorB * 0.2)
        end

        local matHandle = bestow.graphics3d.createMaterial(mat)
        if matHandle then
            bestow.graphics3d.drawMesh(state.cubeMesh, matHandle, transform, true, true)
        end
    end
end

-- Draw food pickups (spawned from detached segments)
function rendering.drawFoodPickups()
    local state = app.state
    if not state.cubeMesh then return end

    for _, pickup in ipairs(state.foodPickups) do
        local worldPos = rendering.gridToWorld(pickup.pos)

        -- Spawn animation: pop in and gentle bounce
        local age = state.gameTime - pickup.spawnTime
        local spawnScale = math.min(1.0, age * 5.0)  -- Pop in over 0.2s
        local bounce = 0.1 * math.abs(math.sin(age * 5.0))

        local transform = Transform3D.new()
        transform.position = Vec3.new(worldPos.x, worldPos.y + bounce, worldPos.z)
        transform.scale = Vec3.new(spawnScale * 0.7, spawnScale * 0.7, spawnScale * 0.7)

        -- Slightly different from regular food - more sparkly
        local mat = PBRMaterial.new()
        mat.baseColorFactor = Color.new(pickup.color[1], pickup.color[2], pickup.color[3], pickup.color[4])
        mat.roughnessFactor = 0.2
        mat.metallicFactor = 0.4
        mat.emissiveFactor = Vec3.new(
            pickup.color[1] * 0.4,
            pickup.color[2] * 0.4,
            pickup.color[3] * 0.4
        )

        local matHandle = bestow.graphics3d.createMaterial(mat)
        if matHandle then
            bestow.graphics3d.drawMesh(state.cubeMesh, matHandle, transform, true, true)
        end
    end
end

-- Draw main food
function rendering.drawFood()
    local config = app.config
    local state = app.state
    if not state.cubeMesh then return end
    -- Don't draw food when level is complete (it was just collected)
    if state.currentPhase == GamePhase.LevelComplete then return end

    local worldPos = rendering.gridToWorld(state.foodPos)

    -- Pulsing effect
    local pulse = 0.85 + 0.15 * math.sin(state.gameTime * 6.0)
    local bounce = 0.15 * math.abs(math.sin(state.gameTime * 4.0))

    -- Spin around Y axis
    local spinAngle = state.gameTime * config.FOOD_SPIN_SPEED

    local transform = Transform3D.new()
    transform.position = Vec3.new(worldPos.x, worldPos.y + bounce, worldPos.z)
    -- Apply rotation around Y axis (simplified)
    transform.rotation = Quat.fromAxisAngle(Vec3.new(0, 1, 0), spinAngle)
    transform.scale = Vec3.new(pulse, pulse, pulse)

    -- Create material with food's color
    local mat = PBRMaterial.new()
    mat.baseColorFactor = Color.new(
        state.foodColor[1],
        state.foodColor[2],
        state.foodColor[3],
        state.foodColor[4]
    )
    mat.roughnessFactor = 0.2
    mat.metallicFactor = 0.3
    -- Make food slightly emissive/bright
    mat.emissiveFactor = Vec3.new(
        state.foodColor[1] * 0.3,
        state.foodColor[2] * 0.3,
        state.foodColor[3] * 0.3
    )

    local matHandle = bestow.graphics3d.createMaterial(mat)
    if matHandle then
        bestow.graphics3d.drawMesh(state.cubeMesh, matHandle, transform, true, true)
    end
end

-- Draw food collection pop effect
function rendering.drawFoodPopEffect()
    local state = app.state
    if state.foodPopTimer <= 0 then return end

    -- Draw expanding burst lines from where food was collected
    local progress = 1.0 - state.foodPopScale  -- 0 to 1 as effect progresses
    local radius = 0.3 + progress * 1.5  -- Expanding radius
    local alpha = state.foodPopScale  -- Fade out

    local popColor = Color.new(
        math.floor(255 * alpha),
        math.floor(200 * alpha),
        math.floor(50 * alpha),
        255
    )

    -- Draw 8 burst lines radiating outward
    local NUM_RAYS = 8
    for i = 0, NUM_RAYS - 1 do
        local angle = i * (2.0 * math.pi / NUM_RAYS)
        local dx = math.cos(angle) * radius
        local dz = math.sin(angle) * radius

        local innerRadius = radius * 0.3
        local innerDx = math.cos(angle) * innerRadius
        local innerDz = math.sin(angle) * innerRadius

        bestow.graphics3d.debugDrawLine(
            Vec3.new(state.lastFoodPos.x + innerDx, state.lastFoodPos.y + 0.5, state.lastFoodPos.z + innerDz),
            Vec3.new(state.lastFoodPos.x + dx, state.lastFoodPos.y + 0.5, state.lastFoodPos.z + dz),
            popColor, 0.0, false
        )
    end
end

-- Draw grid border
function rendering.drawGridBorder()
    local config = app.config
    local state = app.state
    local halfGrid = state.gridSize * config.CELL_SIZE * 0.5
    local borderColor = Color.new(80, 60, 40, 255)
    local y = 0.05

    -- Draw the four sides
    bestow.graphics3d.debugDrawLine(
        Vec3.new(-halfGrid, y, -halfGrid),
        Vec3.new(halfGrid, y, -halfGrid),
        borderColor, 0.0, false
    )
    bestow.graphics3d.debugDrawLine(
        Vec3.new(halfGrid, y, -halfGrid),
        Vec3.new(halfGrid, y, halfGrid),
        borderColor, 0.0, false
    )
    bestow.graphics3d.debugDrawLine(
        Vec3.new(halfGrid, y, halfGrid),
        Vec3.new(-halfGrid, y, halfGrid),
        borderColor, 0.0, false
    )
    bestow.graphics3d.debugDrawLine(
        Vec3.new(-halfGrid, y, halfGrid),
        Vec3.new(-halfGrid, y, -halfGrid),
        borderColor, 0.0, false
    )
end

-- Draw game HUD (food progress bar, snake length indicator)
function rendering.drawHUD()
    local config = app.config
    local state = app.state
    local halfGrid = state.gridSize * config.CELL_SIZE * 0.5
    local hudY = 0.05  -- Just above ground
    local hudZ = -halfGrid - 0.5  -- Behind the play area

    -- Food progress bar (how many food collected vs required)
    local foodBarWidth = state.gridSize * config.CELL_SIZE * 0.6
    local foodProgress = state.foodCollected / state.foodRequired
    foodProgress = math.min(1.0, foodProgress)

    -- Background (dark)
    local bgColor = Color.new(40, 40, 40, 255)
    bestow.graphics3d.debugDrawLine(
        Vec3.new(-foodBarWidth * 0.5, hudY, hudZ),
        Vec3.new(foodBarWidth * 0.5, hudY, hudZ),
        bgColor, 0.0, false
    )

    -- Foreground (gold/yellow for food)
    local foodColor = Color.new(255, 200, 50, 255)
    if foodProgress > 0 then
        bestow.graphics3d.debugDrawLine(
            Vec3.new(-foodBarWidth * 0.5, hudY + 0.02, hudZ),
            Vec3.new(-foodBarWidth * 0.5 + foodBarWidth * foodProgress, hudY + 0.02, hudZ),
            foodColor, 0.0, false
        )
    end

    -- Segment count indicator (snake length) - vertical bar on left side
    local segmentBarHeight = 3.0
    local segmentBarX = -halfGrid - 0.5
    local segmentProgress = math.min(1.0, #state.snake / 20.0)  -- Cap at 20 for display

    -- Background
    bestow.graphics3d.debugDrawLine(
        Vec3.new(segmentBarX, hudY, -halfGrid),
        Vec3.new(segmentBarX, hudY + segmentBarHeight, -halfGrid),
        bgColor, 0.0, false
    )

    -- Foreground (green for snake)
    local snakeColor = Color.new(100, 255, 100, 255)
    if segmentProgress > 0 then
        bestow.graphics3d.debugDrawLine(
            Vec3.new(segmentBarX - 0.02, hudY, -halfGrid),
            Vec3.new(segmentBarX - 0.02, hudY + segmentBarHeight * segmentProgress, -halfGrid),
            snakeColor, 0.0, false
        )
    end

    -- Draw segment count as small markers
    for i = 1, math.min(#state.snake, 20) do
        local markerY = hudY + ((i - 1) / 20.0) * segmentBarHeight
        local markerColor = (i == 1) and Color.new(255, 255, 100, 255) or snakeColor
        bestow.graphics3d.debugDrawLine(
            Vec3.new(segmentBarX - 0.1, markerY, -halfGrid),
            Vec3.new(segmentBarX + 0.05, markerY, -halfGrid),
            markerColor, 0.0, false
        )
    end
end

-- Draw victory effect for level complete
function rendering.drawLevelCompleteEffect()
    local config = app.config
    local state = app.state
    if state.currentPhase ~= GamePhase.LevelComplete or #state.snake == 0 then return end

    local halfGrid = state.gridSize * config.CELL_SIZE * 0.5

    -- Celebratory golden border - pulsing
    local flash = math.sin(state.gameTime * 4.0) * 0.3 + 0.7
    local brightness = math.floor(200 * flash + 55)
    local victoryColor = Color.new(brightness, math.floor(brightness * 0.85), 50, 255)

    -- Draw multiple concentric borders for emphasis
    for offset = 0, 0.2, 0.1 do
        local vBorder = halfGrid + 0.1 + offset
        local y = 0.15 + offset
        bestow.graphics3d.debugDrawLine(Vec3.new(-vBorder, y, -vBorder), Vec3.new(vBorder, y, -vBorder), victoryColor, 0.0, false)
        bestow.graphics3d.debugDrawLine(Vec3.new(vBorder, y, -vBorder), Vec3.new(vBorder, y, vBorder), victoryColor, 0.0, false)
        bestow.graphics3d.debugDrawLine(Vec3.new(vBorder, y, vBorder), Vec3.new(-vBorder, y, vBorder), victoryColor, 0.0, false)
        bestow.graphics3d.debugDrawLine(Vec3.new(-vBorder, y, vBorder), Vec3.new(-vBorder, y, -vBorder), victoryColor, 0.0, false)
    end
end

-- Render the full game scene
function rendering.renderGame()
    rendering.drawGround()
    rendering.drawGridBorder()
    rendering.drawObstacles()
    rendering.drawEnemies()
    rendering.drawSnake()
    rendering.drawDetachedSegments()
    rendering.drawFoodPickups()
    rendering.drawFood()
    rendering.drawFoodPopEffect()
    rendering.drawHUD()
    rendering.drawLevelCompleteEffect()
end

return rendering

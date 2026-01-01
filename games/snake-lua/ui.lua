-- ui.lua - Menus and HUD
-- Dependencies: app.config, app.state, app.pixeltext (accessed inside functions)

local ui = {}

-- Draw the main menu
function ui.drawMainMenu()
    local config = app.config
    local state = app.state
    local pixeltext = app.pixeltext

    -- Set up camera for menu view - more top-down angle
    local cam = Camera3D.new()
    cam.fovY = 45.0
    cam.transform.position = Vec3.new(0.0, 15.0, 10.0)
    local lookDir = Vec3.new(0.0, -0.8, -0.4):normalize()
    cam.transform.rotation = Quat.lookAt(lookDir, Vec3.new(0.0, 1.0, 0.0))
    bestow.graphics3d.setCamera(cam)

    -- Draw simple dark ground plane
    if state.groundMesh then
        local groundMat = PBRMaterial.new()
        groundMat.baseColorFactor = Color.new(0.05, 0.08, 0.05, 1.0)  -- Very dark green
        local matHandle = bestow.graphics3d.createMaterial(groundMat)
        if matHandle then
            local transform = Transform3D.new()
            transform.position = Vec3.new(0.0, -0.5, 0.0)
            transform.scale = Vec3.new(25.0, 1.0, 25.0)
            bestow.graphics3d.drawMesh(state.groundMesh, matHandle, transform, true, true)
        end
    end

    -- Draw a coiled serpent in the center as decoration
    local snakeAnim = state.menuAnimTime * 0.8
    local coilSegments = 20
    for i = 0, coilSegments - 1 do
        local t = i / coilSegments
        local angle = t * 6.28 * 2.5 + snakeAnim  -- 2.5 coils
        local radius = 2.0 + t * 1.5  -- Spiral outward
        local x = math.cos(angle) * radius
        local z = math.sin(angle) * radius
        local y = 0.4 + math.sin(snakeAnim * 2.0 + t * 6.28) * 0.1

        local green = 0.9 - t * 0.4
        local mat = PBRMaterial.new()
        mat.baseColorFactor = Color.new(0.15, green, 0.2, 1.0)
        mat.emissiveFactor = Vec3.new(0.0, green * 0.1, 0.0)

        local segmentScale = 0.6 - t * 0.2  -- Smaller toward tail
        local transform = Transform3D.new()
        transform.position = Vec3.new(x, y, z)
        transform.scale = Vec3.new(segmentScale, segmentScale, segmentScale)

        local matHandle = bestow.graphics3d.createMaterial(mat)
        if matHandle and state.cubeMesh then
            bestow.graphics3d.drawMesh(state.cubeMesh, matHandle, transform, true, true)
        end
    end

    -- Menu options - simple cubes with selection indicator
    local menuZ = 5.0
    local menuSpacing = 2.0

    for i = 0, config.MAIN_MENU_COUNT - 1 do
        local z = menuZ + i * menuSpacing
        local selected = (i == state.mainMenuSelection)

        -- Number of cubes for each option (PLAY = 4 cubes, QUIT = 4 cubes)
        local numCubes = 4
        local cubeSpacing = 1.2
        local startX = -(numCubes - 1) * cubeSpacing * 0.5

        for c = 0, numCubes - 1 do
            local x = startX + c * cubeSpacing
            local y = 0.5

            -- Animate selected row
            if selected then
                local bounce = math.sin(state.menuAnimTime * 4.0 + c * 0.5) * 0.2
                y = y + bounce + 0.3
            end

            local mat = PBRMaterial.new()
            if selected then
                -- Bright golden color for selected
                local pulse = math.sin(state.menuAnimTime * 3.0) * 0.2 + 0.8
                mat.baseColorFactor = Color.new(1.0, 0.8 * pulse, 0.2, 1.0)
                mat.emissiveFactor = Vec3.new(0.3, 0.2, 0.0)
            else
                -- Dim gray for unselected
                mat.baseColorFactor = Color.new(0.4, 0.4, 0.5, 1.0)
            end

            local transform = Transform3D.new()
            transform.position = Vec3.new(x, y, z)
            local scale = selected and 0.9 or 0.7
            transform.scale = Vec3.new(scale, scale, scale)

            local matHandle = bestow.graphics3d.createMaterial(mat)
            if matHandle and state.cubeMesh then
                bestow.graphics3d.drawMesh(state.cubeMesh, matHandle, transform, true, true)
            end
        end

        -- Draw selection arrows on sides
        if selected then
            local arrowX = startX - 1.5
            local pulse = math.sin(state.menuAnimTime * 5.0) * 0.3

            local arrowMat = PBRMaterial.new()
            arrowMat.baseColorFactor = Color.new(1.0, 1.0, 0.3, 1.0)
            arrowMat.emissiveFactor = Vec3.new(0.5, 0.5, 0.0)

            -- Left arrow (triangle of cubes pointing right)
            for a = 0, 2 do
                local ax = arrowX - a * 0.4 + pulse
                local transform = Transform3D.new()
                transform.position = Vec3.new(ax, 0.5, z)
                transform.scale = Vec3.new(0.4, 0.4, 0.4)
                local matHandle = bestow.graphics3d.createMaterial(arrowMat)
                if matHandle and state.cubeMesh then
                    bestow.graphics3d.drawMesh(state.cubeMesh, matHandle, transform, true, true)
                end
            end

            -- Right arrow
            arrowX = startX + (numCubes - 1) * cubeSpacing + 1.5
            for a = 0, 2 do
                local ax = arrowX + a * 0.4 - pulse
                local transform = Transform3D.new()
                transform.position = Vec3.new(ax, 0.5, z)
                transform.scale = Vec3.new(0.4, 0.4, 0.4)
                local matHandle = bestow.graphics3d.createMaterial(arrowMat)
                if matHandle and state.cubeMesh then
                    bestow.graphics3d.drawMesh(state.cubeMesh, matHandle, transform, true, true)
                end
            end
        end
    end

    -- Draw title as a row of cubes at the front
    local titleZ = -2.0
    local titleCubes = 7
    local titleSpacing = 0.8
    local titleStartX = -(titleCubes - 1) * titleSpacing * 0.5

    for t = 0, titleCubes - 1 do
        local x = titleStartX + t * titleSpacing
        local y = 0.5 + math.sin(state.menuAnimTime * 2.0 + t * 0.4) * 0.15

        local mat = PBRMaterial.new()
        local hue = t / titleCubes
        mat.baseColorFactor = Color.new(0.2 + hue * 0.3, 0.8, 0.3 + (1.0 - hue) * 0.3, 1.0)
        mat.emissiveFactor = Vec3.new(0.1, 0.2, 0.1)

        local transform = Transform3D.new()
        transform.position = Vec3.new(x, y, titleZ)
        transform.scale = Vec3.new(0.6, 0.6, 0.6)

        local matHandle = bestow.graphics3d.createMaterial(mat)
        if matHandle and state.cubeMesh then
            bestow.graphics3d.drawMesh(state.cubeMesh, matHandle, transform, true, true)
        end
    end

    -- Instructions hint - small cubes indicating controls
    local hintX = 6.0
    local hintZ = 3.0
    local hintMat = PBRMaterial.new()
    hintMat.baseColorFactor = Color.new(0.6, 0.6, 0.7, 1.0)

    local function drawHintCube(x, y, z)
        local transform = Transform3D.new()
        transform.position = Vec3.new(x, y, z)
        transform.scale = Vec3.new(0.3, 0.3, 0.3)
        local matHandle = bestow.graphics3d.createMaterial(hintMat)
        if matHandle and state.cubeMesh then
            bestow.graphics3d.drawMesh(state.cubeMesh, matHandle, transform, true, true)
        end
    end

    -- Vertical arrow (up/down navigation)
    drawHintCube(hintX, 0.3, hintZ - 0.5)
    drawHintCube(hintX, 0.3, hintZ)
    drawHintCube(hintX, 0.3, hintZ + 0.5)

    -- ==================== TEXT LABELS (Pixel Font) ====================
    -- Title text - "SERPENT" (billboarded to face camera)
    pixeltext.drawBillboardText("SERPENT", 0.0, 2.0, titleZ, 0.15, Color.new(100, 255, 150, 255))

    -- Menu option labels (billboarded) - high contrast colors
    local menuLabels = {"PLAY", "QUIT"}
    for i = 0, config.MAIN_MENU_COUNT - 1 do
        local z = menuZ + i * menuSpacing
        local selected = (i == state.mainMenuSelection)

        -- White text for selected, light gray for unselected - high contrast
        local textColor = selected
            and Color.new(255, 255, 255, 255)  -- Pure white for selected
            or Color.new(120, 120, 140, 255)   -- Darker gray for unselected

        local textY = selected and 2.2 or 1.8  -- Position text higher above cubes
        local pixelSize = selected and 0.08 or 0.05

        pixeltext.drawBillboardText(menuLabels[i + 1], 0.0, textY, z, pixelSize, textColor)
    end

    -- Controls hint text (billboarded)
    pixeltext.drawBillboardText(",O SELECT", hintX, 0.8, hintZ + 1.5, 0.03, Color.new(150, 150, 170, 255))
end

-- Draw the pause menu
function ui.drawPauseMenu()
    local config = app.config
    local state = app.state
    local pixeltext = app.pixeltext

    local halfGrid = state.gridSize * config.CELL_SIZE * 0.5

    -- Darken overlay - draw large dark rectangle using multiple lines
    local overlayColor = Color.new(20, 20, 30, 200)
    local z = -halfGrid
    while z <= halfGrid do
        bestow.graphics3d.debugDrawLine(
            Vec3.new(-halfGrid, 0.1, z),
            Vec3.new(halfGrid, 0.1, z),
            overlayColor, 0.0, false
        )
        z = z + 0.5
    end

    -- Draw menu in world space, centered on camera target
    local menuY = 2.0
    local menuSpacing = 1.0
    local cx = state.cameraTarget.x
    local cz = state.cameraTarget.z

    local options = {"RESUME", "RESTART", "QUIT"}

    for i = 0, config.PAUSE_MENU_COUNT - 1 do
        local y = menuY - i * menuSpacing
        local selected = (i == state.pauseMenuSelection)

        -- Selection indicator
        if selected then
            local pulse = math.sin(state.gameTime * 4.0) * 0.1 + 0.9
            local selectColor = Color.new(255, 200, 50, 255)
            bestow.graphics3d.debugDrawLine(
                Vec3.new(cx - 2.5 * pulse, y, cz),
                Vec3.new(cx - 1.5, y, cz),
                selectColor, 0.0, false
            )
            bestow.graphics3d.debugDrawLine(
                Vec3.new(cx + 1.5, y, cz),
                Vec3.new(cx + 2.5 * pulse, y, cz),
                selectColor, 0.0, false
            )
        end

        -- Draw text label using pixel font
        local textColor = selected
            and Color.new(255, 220, 100, 255)
            or Color.new(150, 150, 160, 255)

        local pixelSize = selected and 0.06 or 0.04
        pixeltext.drawPixelTextShadow(options[i + 1], cx, y, cz, pixelSize, textColor)
    end

    -- PAUSED title
    pixeltext.drawPixelTextShadow("PAUSED", cx, menuY + 1.5, cz, 0.1, Color.new(255, 255, 100, 255))
end

-- Draw game over overlay
function ui.drawGameOverMenu()
    local config = app.config
    local state = app.state
    local pixeltext = app.pixeltext

    local halfGrid = state.gridSize * config.CELL_SIZE * 0.5

    -- Darken overlay
    local overlayColor = Color.new(20, 10, 10, 180)
    local z = -halfGrid
    while z <= halfGrid do
        bestow.graphics3d.debugDrawLine(
            Vec3.new(-halfGrid, 0.1, z),
            Vec3.new(halfGrid, 0.1, z),
            overlayColor, 0.0, false
        )
        z = z + 0.3
    end

    -- Game Over title
    pixeltext.drawPixelTextShadow("GAME OVER", 0.0, 2.8, 0.0, 0.12, Color.new(255, 100, 100, 255))

    -- Menu options
    local options = {"RETRY", "QUIT"}
    for i = 0, 1 do
        local selected = (i == state.gameOverMenuSelection)
        local textColor = selected
            and Color.new(255, 220, 100, 255)  -- Gold for selected
            or Color.new(150, 150, 160, 255)   -- Gray for unselected
        local pixelSize = selected and 0.07 or 0.05
        local y = 1.8 - i * 0.6
        pixeltext.drawPixelTextShadow(options[i + 1], 0.0, y, 0.0, pixelSize, textColor)
    end

    -- Controls hint
    pixeltext.drawPixelText(",O SELECT", 0.0, 0.5, 0.0, 0.025, Color.new(120, 120, 140, 255), true)
end

-- Draw HUD text labels
function ui.drawHUDText()
    local config = app.config
    local state = app.state
    local pixeltext = app.pixeltext

    if state.currentPhase ~= GamePhase.Playing or state.gameOver then return end

    local halfGrid = state.gridSize * config.CELL_SIZE * 0.5
    local hudY = 0.05
    local hudZ = -halfGrid - 0.5
    local segmentBarX = -halfGrid - 0.5
    local segmentBarHeight = 3.0

    -- Food counter text (above the food bar) - e.g., "3/5"
    local foodText = tostring(state.foodCollected) .. "/" .. tostring(state.foodRequired)
    pixeltext.drawPixelTextShadow(foodText, 0.0, hudY + 0.6, hudZ - 0.3, 0.04, Color.new(255, 200, 50, 255))

    -- Snake length counter (next to segment bar)
    local segmentText = tostring(#state.snake)
    pixeltext.drawPixelTextShadow(segmentText, segmentBarX - 0.3, hudY + segmentBarHeight + 0.3, -halfGrid, 0.03, Color.new(100, 255, 100, 255))

    -- Level name (top of screen)
    if state.currentLevel.name and state.currentLevel.name ~= "" then
        pixeltext.drawPixelTextShadow(state.currentLevel.name, 0.0, 3.0, -halfGrid - 0.5, 0.05, Color.new(200, 200, 255, 255))
    end
end

-- Draw level complete celebration
function ui.drawLevelCompleteUI()
    local config = app.config
    local state = app.state
    local pixeltext = app.pixeltext

    if state.currentPhase ~= GamePhase.LevelComplete or #state.snake == 0 then return end

    -- Get snake head position
    local headPos = state.snake[1].pos
    local headX = headPos.x * config.CELL_SIZE - state.gridSize * config.CELL_SIZE * 0.5 + config.CELL_SIZE * 0.5
    local headZ = headPos.z * config.CELL_SIZE - state.gridSize * config.CELL_SIZE * 0.5 + config.CELL_SIZE * 0.5

    -- "LEVEL COMPLETE" text above head
    pixeltext.drawPixelTextShadow("LEVEL COMPLETE!", headX, 2.5, headZ, 0.08, Color.new(255, 220, 50, 255))

    -- Animated checkmark
    local checkSize = 0.8
    local checkY = 1.8
    local flash = math.sin(state.gameTime * 6.0) * 0.2 + 0.8
    local checkColor = Color.new(
        math.floor(50 * flash + 100),
        math.floor(255 * flash),
        math.floor(50 * flash + 50),
        255
    )

    -- Draw checkmark with multiple lines for thickness
    for thickness = -0.02, 0.02, 0.02 do
        -- Left part of check
        bestow.graphics3d.debugDrawLine(
            Vec3.new(headX - checkSize * 0.5 + thickness, checkY + thickness, headZ),
            Vec3.new(headX, checkY - checkSize * 0.3 + thickness, headZ),
            checkColor, 0.0, false
        )
        -- Right part of check
        bestow.graphics3d.debugDrawLine(
            Vec3.new(headX, checkY - checkSize * 0.3 + thickness, headZ),
            Vec3.new(headX + checkSize * 0.7 + thickness, checkY + checkSize * 0.5 + thickness, headZ),
            checkColor, 0.0, false
        )
    end

    -- Draw floating cubes around the snake head as celebration
    local starRadius = 1.5
    local numStars = 8
    for i = 0, numStars - 1 do
        local angle = (i / numStars) * math.pi * 2.0 + state.gameTime * 2.0
        local starX = headX + math.cos(angle) * starRadius
        local starZ = headZ + math.sin(angle) * starRadius
        local starY = 0.5 + math.sin(state.gameTime * 3.0 + i) * 0.3

        local transform = Transform3D.new()
        transform.position = Vec3.new(starX, starY, starZ)
        transform.scale = Vec3.new(0.15, 0.15, 0.15)

        local starMat = PBRMaterial.new()
        starMat.baseColorFactor = Color.new(1.0, 0.84, 0.0, 1.0)  -- Gold
        starMat.roughnessFactor = 0.3
        starMat.metallicFactor = 0.8
        starMat.emissiveFactor = Vec3.new(0.5, 0.4, 0.0)

        local matHandle = bestow.graphics3d.createMaterial(starMat)
        if matHandle and state.cubeMesh then
            bestow.graphics3d.drawMesh(state.cubeMesh, matHandle, transform, true, true)
        end
    end

    -- Controls hint
    pixeltext.drawPixelText("ENTER NEXT  ESC MENU", headX, 0.8, headZ, 0.025, Color.new(180, 180, 200, 255), true)
end

return ui

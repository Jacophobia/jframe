-- worldmap.lua - World map rendering
-- Matches C++ world map code exactly

local config = require("config")
local state = require("state")
local levels = require("levels")
local pixeltext = require("pixeltext")

local worldmap = {}

-- Draw the world map
function worldmap.draw()
    -- Set up isometric camera for world map
    local cam = Camera3D.new()
    local angleRad = math.rad(45.0)
    local cameraPos = Vec3.new(
        state.worldMapCameraTarget.x + config.WORLD_MAP_CAMERA_DISTANCE * math.sin(angleRad),
        config.WORLD_MAP_CAMERA_DISTANCE * 0.8,  -- Height
        state.worldMapCameraTarget.z + config.WORLD_MAP_CAMERA_DISTANCE * math.cos(angleRad)
    )
    cam.transform.position = cameraPos
    cam.fovY = 45.0
    cam.aspectRatio = 16.0 / 9.0
    cam.nearPlane = 0.1
    cam.farPlane = 100.0

    -- Calculate rotation to look at target
    local lookDir = Vec3.new(
        state.worldMapCameraTarget.x - cameraPos.x,
        state.worldMapCameraTarget.y - cameraPos.y,
        state.worldMapCameraTarget.z - cameraPos.z
    ):normalize()
    local up = Vec3.new(0.0, 1.0, 0.0)
    cam.transform.rotation = Quat.lookAt(lookDir, up)

    bestow.graphics3d.setCamera(cam)

    -- Draw ground plane (larger for world map)
    local groundMat = PBRMaterial.new()
    groundMat.baseColorFactor = Color.new(0.2, 0.5, 0.25, 1.0)  -- Forest green
    groundMat.roughnessFactor = 0.9
    local groundMatHandle = bestow.graphics3d.createMaterial(groundMat)

    if groundMatHandle and state.cubeMesh then
        local groundTransform = Transform3D.new()
        groundTransform.position = Vec3.new(state.worldMapCameraTarget.x, -0.1, state.worldMapCameraTarget.z)
        groundTransform.scale = Vec3.new(50.0, 0.1, 50.0)
        bestow.graphics3d.drawMesh(state.cubeMesh, groundMatHandle, groundTransform, true, true)
    end

    -- Draw paths between nodes
    local pathColor = Color.new(139, 90, 43, 255)  -- Brown for paths
    for _, path in ipairs(state.currentWorld.paths) do
        local a, b = path[1], path[2]
        if a < #state.currentWorld.nodes and b < #state.currentWorld.nodes then
            local nodeA = state.currentWorld.nodes[a + 1]
            local nodeB = state.currentWorld.nodes[b + 1]

            local posA = Vec3.new(nodeA.gridPos.x, 0.05, nodeA.gridPos.z)
            local posB = Vec3.new(nodeB.gridPos.x, 0.05, nodeB.gridPos.z)

            -- Draw thicker path by drawing multiple lines
            local offset = -0.1
            while offset <= 0.1 do
                bestow.graphics3d.debugDrawLine(
                    Vec3.new(posA.x + offset, posA.y, posA.z),
                    Vec3.new(posB.x + offset, posB.y, posB.z),
                    pathColor, 0.0, false
                )
                offset = offset + 0.05
            end
        end
    end

    -- Draw nodes
    for i, node in ipairs(state.currentWorld.nodes) do
        local x = node.gridPos.x
        local z = node.gridPos.z
        local isSelected = (i - 1 == state.selectedNodeIndex)
        local unlocked = levels.isLevelUnlocked(node.levelIndex)

        -- Node base (platform)
        local nodeTransform = Transform3D.new()
        nodeTransform.position = Vec3.new(x, 0.1, z)
        nodeTransform.scale = Vec3.new(0.8, 0.2, 0.8)

        local nodeMat = PBRMaterial.new()
        if node.isBoss then
            nodeMat.baseColorFactor = Color.new(0.6, 0.1, 0.1, 1.0)  -- Red for boss
        elseif not unlocked then
            nodeMat.baseColorFactor = Color.new(0.3, 0.3, 0.3, 1.0)  -- Gray for locked
        elseif node.isCompleted then
            nodeMat.baseColorFactor = Color.new(0.2, 0.6, 0.2, 1.0)  -- Green for completed
        else
            nodeMat.baseColorFactor = Color.new(0.5, 0.4, 0.2, 1.0)  -- Brown for available
        end
        nodeMat.roughnessFactor = 0.6

        local nodeMatHandle = bestow.graphics3d.createMaterial(nodeMat)
        if nodeMatHandle and state.cubeMesh then
            bestow.graphics3d.drawMesh(state.cubeMesh, nodeMatHandle, nodeTransform, true, true)
        end

        -- Draw level indicator on top
        if node.levelIndex >= 0 then
            local indicatorTransform = Transform3D.new()
            local bobY = isSelected and (0.5 + math.sin(state.worldMapCursorBob) * 0.15) or 0.4
            indicatorTransform.position = Vec3.new(x, bobY, z)
            indicatorTransform.scale = Vec3.new(0.3, 0.3, 0.3)

            local indicatorMat = PBRMaterial.new()
            if node.isBoss then
                indicatorMat.baseColorFactor = Color.new(1.0, 0.3, 0.3, 1.0)
                indicatorMat.emissiveFactor = Vec3.new(0.5, 0.1, 0.1)
            elseif not unlocked then
                indicatorMat.baseColorFactor = Color.new(0.5, 0.5, 0.5, 1.0)
            else
                indicatorMat.baseColorFactor = Color.new(1.0, 0.85, 0.0, 1.0)  -- Gold
                indicatorMat.emissiveFactor = Vec3.new(0.3, 0.25, 0.0)
            end

            if isSelected then
                indicatorMat.emissiveFactor = Vec3.new(0.5, 0.5, 0.5)  -- Glow when selected
            end

            local indicatorMatHandle = bestow.graphics3d.createMaterial(indicatorMat)
            if indicatorMatHandle and state.cubeMesh then
                bestow.graphics3d.drawMesh(state.cubeMesh, indicatorMatHandle, indicatorTransform, true, true)
            end
        end

        -- Selection ring for selected node - smooth circle with glow effect
        if isSelected then
            local ringRadius = 0.6
            local pulse = math.sin(state.worldMapCursorBob * 2.0) * 0.1
            ringRadius = ringRadius + pulse
            local ringColor = Color.new(255, 255, 100, 255)
            local glowColor = Color.new(255, 255, 200, 128)
            local segments = 48  -- Smooth circle

            for s = 0, segments - 1 do
                local angle1 = (s / segments) * math.pi * 2.0
                local angle2 = ((s + 1) / segments) * math.pi * 2.0

                -- Inner ring
                bestow.graphics3d.debugDrawLine(
                    Vec3.new(x + math.cos(angle1) * ringRadius, 0.08, z + math.sin(angle1) * ringRadius),
                    Vec3.new(x + math.cos(angle2) * ringRadius, 0.08, z + math.sin(angle2) * ringRadius),
                    ringColor, 0.0, false
                )

                -- Outer glow ring
                local outerRadius = ringRadius + 0.1
                bestow.graphics3d.debugDrawLine(
                    Vec3.new(x + math.cos(angle1) * outerRadius, 0.06, z + math.sin(angle1) * outerRadius),
                    Vec3.new(x + math.cos(angle2) * outerRadius, 0.06, z + math.sin(angle2) * outerRadius),
                    glowColor, 0.0, false
                )
            end
        end
    end

    -- Draw HUD info for selected node
    if #state.currentWorld.nodes > 0 and state.selectedNodeIndex < #state.currentWorld.nodes then
        local node = state.currentWorld.nodes[state.selectedNodeIndex + 1]
        local unlocked = levels.isLevelUnlocked(node.levelIndex)

        -- Draw unlock requirement indicator if locked
        if not unlocked and node.levelIndex < #state.currentWorld.unlockRequirements then
            local required = state.currentWorld.unlockRequirements[node.levelIndex + 1]
            local current = levels.getTotalFoodInCurrentWorld()

            -- Draw a progress bar above the camera
            local barWidth = 3.0
            local progress = current / required
            progress = math.min(1.0, progress)

            local barX = state.worldMapCameraTarget.x
            local barY = 3.0
            local barZ = state.worldMapCameraTarget.z - 2.0

            -- Background
            local bgColor = Color.new(60, 60, 60, 255)
            bestow.graphics3d.debugDrawLine(
                Vec3.new(barX - barWidth / 2, barY, barZ),
                Vec3.new(barX + barWidth / 2, barY, barZ),
                bgColor, 0.0, false
            )

            -- Progress
            local progressColor = Color.new(100, 200, 100, 255)
            bestow.graphics3d.debugDrawLine(
                Vec3.new(barX - barWidth / 2, barY, barZ),
                Vec3.new(barX - barWidth / 2 + barWidth * progress, barY, barZ),
                progressColor, 0.0, false
            )
        end
    end

    -- World title text
    pixeltext.drawPixelTextShadow(
        state.currentWorld.name,
        state.worldMapCameraTarget.x, 4.0, state.worldMapCameraTarget.z - 3.0,
        0.08, Color.new(200, 220, 255, 255)
    )
end

-- Initialize world map (update node states from save data)
function worldmap.initialize()
    if state.currentWorldIndex < #state.saveData.worldProgress then
        local progress = state.saveData.worldProgress[state.currentWorldIndex + 1]
        for i, node in ipairs(state.currentWorld.nodes) do
            node.isUnlocked = levels.isLevelUnlocked(node.levelIndex)
            if node.levelIndex >= 0 and node.levelIndex < #progress.levelsCompleted then
                node.isCompleted = progress.levelsCompleted[node.levelIndex + 1]
            end
        end
    end

    -- Start camera centered on first unlocked node
    state.selectedNodeIndex = 0
    for i, node in ipairs(state.currentWorld.nodes) do
        if node.isUnlocked and not node.isCompleted then
            state.selectedNodeIndex = i - 1
            break
        end
    end

    -- Set camera target to selected node
    if #state.currentWorld.nodes > 0 then
        local node = state.currentWorld.nodes[state.selectedNodeIndex + 1]
        state.worldMapCameraTarget = Vec3.new(node.gridPos.x, 0, node.gridPos.z)
    end
end

return worldmap

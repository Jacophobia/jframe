-- World 1 Level 1: Forest Gate
-- A large open level with sparse obstacles that reveal as you expand

return {
    name = "Forest Gate",
    width = 200,
    height = 200,
    foodRequired = 50,

    -- Player starts in the center
    playerStart = { x = 100, z = 100 },

    -- Sparse wall clusters spread throughout the level
    -- These will be revealed gradually as the grid expands
    walls = (function()
        local walls = {}

        -- Create scattered obstacle clusters
        local clusters = {
            -- Inner ring (visible early)
            {cx = 100, cz = 100, radius = 15, count = 4},
            {cx = 100, cz = 100, radius = 25, count = 6},

            -- Middle ring
            {cx = 100, cz = 100, radius = 40, count = 8},
            {cx = 100, cz = 100, radius = 55, count = 10},
            {cx = 100, cz = 100, radius = 70, count = 12},

            -- Outer ring
            {cx = 100, cz = 100, radius = 85, count = 14},
            {cx = 100, cz = 100, radius = 95, count = 16},
        }

        -- Generate walls in circular patterns
        for _, cluster in ipairs(clusters) do
            for i = 1, cluster.count do
                local angle = (i / cluster.count) * math.pi * 2
                local x = math.floor(cluster.cx + math.cos(angle) * cluster.radius)
                local z = math.floor(cluster.cz + math.sin(angle) * cluster.radius)

                -- Add a small 2x2 or 3x3 cluster at each point
                table.insert(walls, {x = x, z = z})
                table.insert(walls, {x = x + 1, z = z})
                table.insert(walls, {x = x, z = z + 1})
            end
        end

        -- Add some random scattered single walls for variety
        local scattered = {
            {x = 90, z = 95}, {x = 110, z = 105}, {x = 95, z = 110}, {x = 105, z = 90},
            {x = 85, z = 85}, {x = 115, z = 85}, {x = 85, z = 115}, {x = 115, z = 115},
            {x = 70, z = 100}, {x = 130, z = 100}, {x = 100, z = 70}, {x = 100, z = 130},
            {x = 60, z = 60}, {x = 140, z = 60}, {x = 60, z = 140}, {x = 140, z = 140},
            {x = 50, z = 100}, {x = 150, z = 100}, {x = 100, z = 50}, {x = 100, z = 150},
        }
        for _, w in ipairs(scattered) do
            table.insert(walls, w)
        end

        return walls
    end)(),

    -- Food spawn points spread throughout the level
    foodSpawnPoints = (function()
        local points = {}
        -- Generate spawn points in a grid pattern, avoiding the very center
        for x = 10, 190, 15 do
            for z = 10, 190, 15 do
                -- Skip points too close to center (snake starts there)
                local dx = x - 100
                local dz = z - 100
                if dx*dx + dz*dz > 100 then  -- More than ~10 units from center
                    table.insert(points, {x = x, z = z})
                end
            end
        end
        return points
    end)(),

    -- Enemy patrol zones - spread out across the level
    enemies = {
        -- Inner zone (appears early)
        {
            zone = { minX = 90, minZ = 90, maxX = 110, maxZ = 110 },
            count = 1,
            moveInterval = 0.6,
            health = 1
        },
        -- Mid zones (appear as you expand)
        {
            zone = { minX = 60, minZ = 60, maxX = 80, maxZ = 80 },
            count = 1,
            moveInterval = 0.5,
            health = 2
        },
        {
            zone = { minX = 120, minZ = 60, maxX = 140, maxZ = 80 },
            count = 1,
            moveInterval = 0.5,
            health = 2
        },
        {
            zone = { minX = 60, minZ = 120, maxX = 80, maxZ = 140 },
            count = 1,
            moveInterval = 0.5,
            health = 2
        },
        {
            zone = { minX = 120, minZ = 120, maxX = 140, maxZ = 140 },
            count = 1,
            moveInterval = 0.5,
            health = 2
        },
        -- Outer zones (appear late game)
        {
            zone = { minX = 20, minZ = 90, maxX = 50, maxZ = 110 },
            count = 2,
            moveInterval = 0.4,
            health = 3
        },
        {
            zone = { minX = 150, minZ = 90, maxX = 180, maxZ = 110 },
            count = 2,
            moveInterval = 0.4,
            health = 3
        },
    },

    -- This is not a boss level
    isBossLevel = false
}

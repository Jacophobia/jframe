-- games/snake3d/modules/grid.lua
-- Grid coordinate helpers and expansion logic
--
-- Handles conversion between grid coordinates and world coordinates,
-- grid wrapping, and expansion mechanics

local Constants = bestow.include("modules/constants")
local Log = bestow.include("modules/log")

local Grid = {}

local log = Log.category("Grid")

-- Convert grid position {x, z} to world position {x, y, z}
-- Matches C++ gridToWorld() at line 2693
-- Grid is centered at world origin
function Grid.toWorld(gridSize, pos)
    local halfGrid = gridSize * Constants.CELL_SIZE * 0.5
    return {
        x = pos.x * Constants.CELL_SIZE - halfGrid + Constants.CELL_SIZE * 0.5,
        y = Constants.CELL_SIZE * 0.5,
        z = pos.z * Constants.CELL_SIZE - halfGrid + Constants.CELL_SIZE * 0.5
    }
end

-- Convert float grid position to world position (for smooth interpolation)
-- Matches C++ gridToWorldFloat() at line 2703
function Grid.toWorldFloat(gridSize, x, z)
    local halfGrid = gridSize * Constants.CELL_SIZE * 0.5
    return {
        x = x * Constants.CELL_SIZE - halfGrid + Constants.CELL_SIZE * 0.5,
        y = Constants.CELL_SIZE * 0.5,
        z = z * Constants.CELL_SIZE - halfGrid + Constants.CELL_SIZE * 0.5
    }
end

-- Wrap position to stay within grid bounds (toroidal topology)
-- Matches C++ wrapping logic in moveSnake() lines 1876-1881
function Grid.wrap(gridSize, pos)
    local x = pos.x
    local z = pos.z

    if x < 0 then x = gridSize - 1 end
    if x >= gridSize then x = 0 end
    if z < 0 then z = gridSize - 1 end
    if z >= gridSize then z = 0 end

    return {x = x, z = z}
end

-- Check if a position is within grid bounds
function Grid.isInBounds(gridSize, pos)
    return pos.x >= 0 and pos.x < gridSize and
           pos.z >= 0 and pos.z < gridSize
end

-- Check if position is at grid edge (1-cell margin)
function Grid.isAtEdge(gridSize, pos)
    return pos.x == 0 or pos.x == gridSize - 1 or
           pos.z == 0 or pos.z == gridSize - 1
end

-- Calculate Manhattan distance between two positions
function Grid.manhattanDistance(pos1, pos2)
    return math.abs(pos1.x - pos2.x) + math.abs(pos1.z - pos2.z)
end

-- Calculate squared Euclidean distance (faster than sqrt)
function Grid.distanceSquared(pos1, pos2)
    local dx = pos1.x - pos2.x
    local dz = pos1.z - pos2.z
    return dx * dx + dz * dz
end

-- Check if two positions are equal
function Grid.posEquals(pos1, pos2)
    return pos1.x == pos2.x and pos1.z == pos2.z
end

-- Copy a position
function Grid.copyPos(pos)
    return {x = pos.x, z = pos.z}
end

-- Add offset to position
function Grid.addOffset(pos, offset)
    return {x = pos.x + offset.x, z = pos.z + offset.z}
end

-- Get grid half-size in world units (for border drawing)
function Grid.getHalfSize(gridSize)
    return gridSize * Constants.CELL_SIZE * 0.5
end

-- Calculate camera distance for a given grid size
-- Matches C++ camera scaling logic at lines 1724-1729
function Grid.getCameraDistance(gridSize)
    return Constants.BASE_CAMERA_DISTANCE +
           (gridSize - Constants.INITIAL_GRID_SIZE) * Constants.CAMERA_SCALE
end

function Grid.getCameraHeight(gridSize)
    return Constants.BASE_CAMERA_HEIGHT +
           (gridSize - Constants.INITIAL_GRID_SIZE) * Constants.CAMERA_SCALE * 0.8
end

-- Shift all positions in a list by a given offset (for grid expansion)
function Grid.shiftPositions(positions, shift)
    for _, pos in ipairs(positions) do
        pos.x = pos.x + shift
        pos.z = pos.z + shift
    end
end

return Grid

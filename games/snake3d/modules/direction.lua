-- games/snake3d/modules/direction.lua
-- Direction enum and helper functions matching C++ exactly
--
-- From games/game1/src/snake.game-types.cppm and snake.game.cppm

local Direction = {}

-- Direction enum values
Direction.Up = 0       -- -Z (forward in isometric view)
Direction.Down = 1     -- +Z (backward)
Direction.Left = 2     -- -X
Direction.Right = 3    -- +X

-- Direction names for debugging
Direction.names = {
    [Direction.Up] = "Up",
    [Direction.Down] = "Down",
    [Direction.Left] = "Left",
    [Direction.Right] = "Right",
}

-- Convert direction to grid offset {x, z}
-- Matches C++ directionToOffset() at line 111-119
function Direction.toOffset(dir)
    if dir == Direction.Up then
        return {x = 0, z = -1}
    elseif dir == Direction.Down then
        return {x = 0, z = 1}
    elseif dir == Direction.Left then
        return {x = -1, z = 0}
    elseif dir == Direction.Right then
        return {x = 1, z = 0}
    end
    return {x = 0, z = 0}
end

-- Get opposite direction (for anti-reversing check)
function Direction.opposite(dir)
    if dir == Direction.Up then return Direction.Down end
    if dir == Direction.Down then return Direction.Up end
    if dir == Direction.Left then return Direction.Right end
    if dir == Direction.Right then return Direction.Left end
    return dir
end

-- Check if two directions are opposite
function Direction.isOpposite(dir1, dir2)
    return Direction.opposite(dir1) == dir2
end

-- Get direction name for logging
function Direction.getName(dir)
    return Direction.names[dir] or "Unknown"
end

return Direction

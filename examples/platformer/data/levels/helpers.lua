-- data/levels/helpers.lua
-- Level utilities for procedural placement

local H = {}

-- Basic utilities
function H.range(start, stop)
    local t = {}
    for i = start, stop do t[#t + 1] = i end
    return t
end

function H.map(array, fn)
    local result = {}
    for i, v in ipairs(array) do
        result[i] = fn(v, i)
    end
    return result
end

function H.concat(...)
    local result = {}
    for _, arr in ipairs({...}) do
        if arr then
            for _, v in ipairs(arr) do
                result[#result + 1] = v
            end
        end
    end
    return result
end

-- Pattern generators
function H.grid(blueprint, startX, startY, cols, rows, spacingX, spacingY, idPrefix)
    local entities = {}
    for row = 0, rows - 1 do
        for col = 0, cols - 1 do
            entities[#entities + 1] = {
                id = idPrefix .. "_" .. row .. "_" .. col,
                blueprint = blueprint,
                x = startX + (col * spacingX),
                y = startY + (row * spacingY)
            }
        end
    end
    return entities
end

function H.arc(blueprint, cx, cy, radius, startDeg, endDeg, count, idPrefix)
    local entities = {}
    local step = (endDeg - startDeg) / (count - 1)
    for i = 0, count - 1 do
        local angle = math.rad(startDeg + (i * step))
        entities[#entities + 1] = {
            id = idPrefix .. "_" .. i,
            blueprint = blueprint,
            x = cx + math.cos(angle) * radius,
            y = cy + math.sin(angle) * radius
        }
    end
    return entities
end

function H.wave(blueprint, startX, baseY, count, spacing, amplitude, frequency, idPrefix)
    local entities = {}
    for i = 0, count - 1 do
        entities[#entities + 1] = {
            id = idPrefix .. "_" .. i,
            blueprint = blueprint,
            x = startX + (i * spacing),
            y = baseY + math.sin(i * frequency) * amplitude
        }
    end
    return entities
end

function H.line(blueprint, x1, y1, x2, y2, count, idPrefix)
    local entities = {}
    for i = 0, count - 1 do
        local t = count > 1 and (i / (count - 1)) or 0
        entities[#entities + 1] = {
            id = idPrefix .. "_" .. i,
            blueprint = blueprint,
            x = x1 + (x2 - x1) * t,
            y = y1 + (y2 - y1) * t
        }
    end
    return entities
end

-- Conditional helpers
function H.ifDebug(entities)
    if os.getenv("BESTOW_DEBUG") == "1" then
        return entities
    end
    return {}
end

return H

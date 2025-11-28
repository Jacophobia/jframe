-- blueprints/_base.lua
-- Blueprint inheritance system using P.extend()

local P = {}

-- Blueprint registry (cached blueprints)
local blueprints = {}

-- Deep copy helper
local function deepCopy(tbl)
    if type(tbl) ~= "table" then return tbl end
    local copy = {}
    for k, v in pairs(tbl) do
        copy[k] = deepCopy(v)
    end
    return copy
end

-- Deep merge helper (source overwrites target)
local function deepMerge(target, source)
    for k, v in pairs(source) do
        if type(v) == "table" and type(target[k]) == "table" then
            deepMerge(target[k], v)
        else
            target[k] = deepCopy(v)
        end
    end
    return target
end

-- Extend a parent blueprint with overrides
function P.extend(parentName, overrides)
    local parent = blueprints[parentName]
    if not parent then
        error("Blueprint not found: " .. parentName)
    end

    local result = deepCopy(parent)
    if overrides then
        deepMerge(result, overrides)
    end
    return result
end

-- Register a new blueprint
function P.register(name, blueprint)
    blueprints[name] = blueprint
end

-- Get a blueprint by name
function P.get(name)
    local bp = blueprints[name]
    if bp then
        return deepCopy(bp)
    end
    return nil
end

-- Check if a blueprint exists
function P.exists(name)
    return blueprints[name] ~= nil
end

-- Clear all blueprints (for hot reload)
function P.clear()
    blueprints = {}
end

return P

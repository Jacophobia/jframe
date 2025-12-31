-- games/snake3d/modules/log.lua
-- Debug logging module with compile-out capability
--
-- Usage:
--   local Log = bestow.include("modules/log")
--   Log.trace("Snake", "Moving to position: %d, %d", x, z)
--   Log.debug("Audio", "Playing sound: %s", soundName)
--
-- In release builds, trace/debug calls compile to no-ops

local Log = {}

-- Log levels
Log.TRACE = 0
Log.DEBUG = 1
Log.INFO = 2
Log.WARN = 3
Log.ERROR = 4

-- Current minimum level (set via BESTOW_LOG_LEVEL or default to DEBUG in debug builds)
-- In release builds, this should be set to INFO or higher
Log.minLevel = Log.DEBUG

-- Color codes for terminal output (ANSI)
local colors = {
    [Log.TRACE] = "\27[90m",   -- Gray
    [Log.DEBUG] = "\27[36m",   -- Cyan
    [Log.INFO] = "\27[32m",    -- Green
    [Log.WARN] = "\27[33m",    -- Yellow
    [Log.ERROR] = "\27[31m",   -- Red
}
local resetColor = "\27[0m"

local levelNames = {
    [Log.TRACE] = "TRACE",
    [Log.DEBUG] = "DEBUG",
    [Log.INFO] = "INFO",
    [Log.WARN] = "WARN",
    [Log.ERROR] = "ERROR",
}

-- Internal log function
local function logMessage(level, category, fmt, ...)
    if level < Log.minLevel then return end

    local color = colors[level] or ""
    local levelName = levelNames[level] or "???"
    local message = string.format(fmt, ...)

    -- Get time from bestow if available (os is sandboxed)
    local timestamp = "??:??:??"
    if bestow and bestow.time and bestow.time.getTime then
        -- Convert engine time to MM:SS.ms format
        local t = bestow.time.getTime()
        local mins = math.floor(t / 60)
        local secs = math.floor(t % 60)
        local ms = math.floor((t * 1000) % 1000)
        timestamp = string.format("%02d:%02d.%03d", mins, secs, ms)
    end

    -- Format: [MM:SS.ms] [Category] [LEVEL] message
    print(string.format("%s[%s] [%s] [%s]%s %s",
        color, timestamp, category, levelName, resetColor, message))
end

-- Public logging functions
function Log.trace(category, fmt, ...)
    logMessage(Log.TRACE, category, fmt, ...)
end

function Log.debug(category, fmt, ...)
    logMessage(Log.DEBUG, category, fmt, ...)
end

function Log.info(category, fmt, ...)
    logMessage(Log.INFO, category, fmt, ...)
end

function Log.warn(category, fmt, ...)
    logMessage(Log.WARN, category, fmt, ...)
end

function Log.error(category, fmt, ...)
    logMessage(Log.ERROR, category, fmt, ...)
end

-- Set minimum log level
function Log.setLevel(level)
    Log.minLevel = level
end

-- Create a category-specific logger
function Log.category(name)
    return {
        trace = function(fmt, ...) Log.trace(name, fmt, ...) end,
        debug = function(fmt, ...) Log.debug(name, fmt, ...) end,
        info = function(fmt, ...) Log.info(name, fmt, ...) end,
        warn = function(fmt, ...) Log.warn(name, fmt, ...) end,
        error = function(fmt, ...) Log.error(name, fmt, ...) end,
    }
end

return Log

-- games/snake3d/modules/pixeltext.lua
-- Pixel font rendering using debug lines
--
-- Matches C++ pixel font from snake.game.cppm lines 214-328

local Log = bestow.include("modules/log")

local PixelText = {}

local log = Log.category("PixelText")

-- Font dimensions (5x7 pixel font)
local FONT_WIDTH = 5
local FONT_HEIGHT = 7

-- 5x7 pixel font patterns - each char is 5 columns x 7 rows
-- Each byte represents one column (bit 0 = top row)
local fontPatterns = {
    A = {0x7E, 0x11, 0x11, 0x11, 0x7E},
    B = {0x7F, 0x49, 0x49, 0x49, 0x36},
    C = {0x3E, 0x41, 0x41, 0x41, 0x22},
    D = {0x7F, 0x41, 0x41, 0x41, 0x3E},
    E = {0x7F, 0x49, 0x49, 0x49, 0x41},
    F = {0x7F, 0x09, 0x09, 0x09, 0x01},
    G = {0x3E, 0x41, 0x49, 0x49, 0x7A},
    H = {0x7F, 0x08, 0x08, 0x08, 0x7F},
    I = {0x00, 0x41, 0x7F, 0x41, 0x00},
    J = {0x20, 0x40, 0x41, 0x3F, 0x01},
    K = {0x7F, 0x08, 0x14, 0x22, 0x41},
    L = {0x7F, 0x40, 0x40, 0x40, 0x40},
    M = {0x7F, 0x02, 0x0C, 0x02, 0x7F},
    N = {0x7F, 0x04, 0x08, 0x10, 0x7F},
    O = {0x3E, 0x41, 0x41, 0x41, 0x3E},
    P = {0x7F, 0x09, 0x09, 0x09, 0x06},
    Q = {0x3E, 0x41, 0x51, 0x21, 0x5E},
    R = {0x7F, 0x09, 0x19, 0x29, 0x46},
    S = {0x46, 0x49, 0x49, 0x49, 0x31},
    T = {0x01, 0x01, 0x7F, 0x01, 0x01},
    U = {0x3F, 0x40, 0x40, 0x40, 0x3F},
    V = {0x1F, 0x20, 0x40, 0x20, 0x1F},
    W = {0x3F, 0x40, 0x38, 0x40, 0x3F},
    X = {0x63, 0x14, 0x08, 0x14, 0x63},
    Y = {0x07, 0x08, 0x70, 0x08, 0x07},
    Z = {0x61, 0x51, 0x49, 0x45, 0x43},
    ["0"] = {0x3E, 0x51, 0x49, 0x45, 0x3E},
    ["1"] = {0x00, 0x42, 0x7F, 0x40, 0x00},
    ["2"] = {0x42, 0x61, 0x51, 0x49, 0x46},
    ["3"] = {0x21, 0x41, 0x45, 0x4B, 0x31},
    ["4"] = {0x18, 0x14, 0x12, 0x7F, 0x10},
    ["5"] = {0x27, 0x45, 0x45, 0x45, 0x39},
    ["6"] = {0x3C, 0x4A, 0x49, 0x49, 0x30},
    ["7"] = {0x01, 0x71, 0x09, 0x05, 0x03},
    ["8"] = {0x36, 0x49, 0x49, 0x49, 0x36},
    ["9"] = {0x06, 0x49, 0x49, 0x29, 0x1E},
    ["/"] = {0x20, 0x10, 0x08, 0x04, 0x02},
    [":"] = {0x00, 0x36, 0x36, 0x00, 0x00},
    [","] = {0x00, 0x00, 0x58, 0x38, 0x00},
    ["-"] = {0x08, 0x08, 0x08, 0x08, 0x08},
    ["."] = {0x00, 0x60, 0x60, 0x00, 0x00},
    ["!"] = {0x00, 0x00, 0x5F, 0x00, 0x00},
    ["?"] = {0x02, 0x01, 0x51, 0x09, 0x06},
    [">"] = {0x41, 0x22, 0x14, 0x08, 0x00},
    ["<"] = {0x00, 0x08, 0x14, 0x22, 0x41},
    [" "] = {0x00, 0x00, 0x00, 0x00, 0x00},
}

-- Get pattern for a character
local function getCharPattern(c)
    local upper = string.upper(c)
    return fontPatterns[upper] or fontPatterns[" "]
end

-- Check if bit is set in value
local function bitSet(value, bit)
    return math.floor(value / (2 ^ bit)) % 2 == 1
end

-- Draw a single character at 3D world position using debug lines
-- Matches C++ drawPixelChar at lines 277-301
local function drawPixelChar(c, x, y, z, pixelSize, color)
    local pattern = getCharPattern(c)

    for col = 0, FONT_WIDTH - 1 do
        local colBits = pattern[col + 1]
        for row = 0, FONT_HEIGHT - 1 do
            if bitSet(colBits, row) then
                -- Calculate pixel position
                local px = x + col * pixelSize
                local py = y + (FONT_HEIGHT - 1 - row) * pixelSize
                local halfPx = pixelSize * 0.45

                -- Draw filled pixel using horizontal lines
                local step = pixelSize * 0.2
                local dy = -halfPx
                while dy <= halfPx do
                    bestow.graphics.drawLine(
                        px - halfPx, py + dy, z,
                        px + halfPx, py + dy, z,
                        color
                    )
                    dy = dy + step
                end
            end
        end
    end
end

-- Draw text string at position
-- Matches C++ drawPixelText at lines 305-316
function PixelText.draw(text, centerX, y, z, pixelSize, color, centered)
    -- Default to centered
    if centered == nil then centered = true end

    local charWidth = FONT_WIDTH * pixelSize + pixelSize  -- Char width + spacing
    local totalWidth = #text * charWidth - pixelSize       -- No trailing space

    local startX = centerX
    if centered then
        startX = centerX - totalWidth * 0.5
    end

    for i = 1, #text do
        local c = text:sub(i, i)
        local x = startX + (i - 1) * charWidth
        drawPixelChar(c, x, y, z, pixelSize, color)
    end
end

-- Draw text with shadow for better visibility
-- Matches C++ drawPixelTextShadow at lines 319-328
function PixelText.drawShadow(text, centerX, y, z, pixelSize, color, centered)
    -- Default to centered
    if centered == nil then centered = true end

    -- Shadow (slightly offset and darker)
    local shadowColor = {0.08, 0.08, 0.08, 0.8}
    local shadowOffset = pixelSize * 0.5

    PixelText.draw(text, centerX + shadowOffset, y - shadowOffset, z + 0.01,
                   pixelSize, shadowColor, centered)

    -- Main text
    PixelText.draw(text, centerX, y, z, pixelSize, color, centered)
end

return PixelText

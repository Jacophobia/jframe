-- pixeltext.lua - Pixel font rendering
-- Matches C++ pixel font code exactly

local pixeltext = {}

-- Font dimensions
pixeltext.FONT_WIDTH = 5
pixeltext.FONT_HEIGHT = 7

-- 5x7 pixel font patterns
-- Each char is 5 columns x 7 rows packed into bytes
-- Each byte represents one column (bottom bit = top row)
local charPatterns = {
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
    [" "] = {0x00, 0x00, 0x00, 0x00, 0x00}
}

-- Get pixel pattern for a character
function pixeltext.getCharPattern(c)
    local upper = string.upper(c)
    return charPatterns[upper] or {0x00, 0x00, 0x00, 0x00, 0x00}
end

-- Check if a bit is set
local function bitSet(value, bit)
    return math.floor(value / (2 ^ bit)) % 2 == 1
end

-- Draw a single character at 3D world position using debug lines
function pixeltext.drawPixelChar(c, x, y, z, pixelSize, color)
    local pattern = pixeltext.getCharPattern(c)

    for col = 0, pixeltext.FONT_WIDTH - 1 do
        local colBits = pattern[col + 1]
        for row = 0, pixeltext.FONT_HEIGHT - 1 do
            if bitSet(colBits, row) then
                -- Draw this pixel as a small square (horizontal lines)
                local px = x + col * pixelSize
                local py = y + (pixeltext.FONT_HEIGHT - 1 - row) * pixelSize
                local halfPx = pixelSize * 0.45

                -- Draw filled pixel using horizontal lines
                local dy = -halfPx
                while dy <= halfPx do
                    bestow.graphics3d.debugDrawLine(
                        Vec3.new(px - halfPx, py + dy, z),
                        Vec3.new(px + halfPx, py + dy, z),
                        color, 0.0, false
                    )
                    dy = dy + pixelSize * 0.2
                end
            end
        end
    end
end

-- Draw text string centered at position
function pixeltext.drawPixelText(text, centerX, y, z, pixelSize, color, centered)
    if centered == nil then centered = true end

    local charWidth = pixeltext.FONT_WIDTH * pixelSize + pixelSize  -- Char width + spacing
    local totalWidth = #text * charWidth - pixelSize  -- No trailing space

    local startX = centered and (centerX - totalWidth * 0.5) or centerX

    for i = 1, #text do
        local c = string.sub(text, i, i)
        local x = startX + (i - 1) * charWidth
        pixeltext.drawPixelChar(string.upper(c), x, y, z, pixelSize, color)
    end
end

-- Draw text with shadow for better visibility
function pixeltext.drawPixelTextShadow(text, centerX, y, z, pixelSize, color, centered)
    if centered == nil then centered = true end

    -- Shadow (slightly offset and darker)
    local shadowColor = Color.new(20, 20, 20, 200)
    local shadowOffset = pixelSize * 0.5
    pixeltext.drawPixelText(text, centerX + shadowOffset, y - shadowOffset, z + 0.01, pixelSize, shadowColor, centered)
    -- Main text
    pixeltext.drawPixelText(text, centerX, y, z, pixelSize, color, centered)
end

-- Draw billboarded text that faces the camera (spherical - perpendicular to view)
function pixeltext.drawBillboardText(text, worldX, worldY, worldZ, pixelSize, color)
    -- Get camera info
    local cam = bestow.graphics3d.getCamera()
    local cameraPos = Vec3.new(cam.transform.position.x, cam.transform.position.y, cam.transform.position.z)
    local textPos = Vec3.new(worldX, worldY, worldZ)

    -- Spherical billboard: text plane is perpendicular to camera view direction
    local forward = (cameraPos - textPos):normalize()

    -- Use camera's actual up direction for consistent orientation
    local camRot = cam.transform.rotation
    local camUp = camRot:rotateVector(Vec3.new(0.0, 1.0, 0.0))

    -- Calculate billboard axes
    local right = camUp:cross(forward):normalize()
    local up = forward:cross(right):normalize()

    -- Calculate text dimensions
    local charWidth = pixeltext.FONT_WIDTH * pixelSize + pixelSize
    local totalWidth = #text * charWidth - pixelSize
    local startOffset = -totalWidth * 0.5

    -- Shadow first
    local shadowColor = Color.new(20, 20, 20, 200)
    local shadowOffset = pixelSize * 0.5
    local shadowPos = textPos + right * shadowOffset - up * shadowOffset + forward * 0.02

    for i = 1, #text do
        local c = string.upper(string.sub(text, i, i))
        local pattern = pixeltext.getCharPattern(c)
        local charOffset = startOffset + (i - 1) * charWidth

        for col = 0, pixeltext.FONT_WIDTH - 1 do
            local colBits = pattern[col + 1]
            for row = 0, pixeltext.FONT_HEIGHT - 1 do
                if bitSet(colBits, row) then
                    local px = charOffset + col * pixelSize
                    local py = (pixeltext.FONT_HEIGHT - 1 - row) * pixelSize
                    local halfPx = pixelSize * 0.45

                    -- Draw shadow pixel
                    local dy = -halfPx
                    while dy <= halfPx do
                        local p1 = shadowPos + right * (px - halfPx) + up * (py + dy)
                        local p2 = shadowPos + right * (px + halfPx) + up * (py + dy)
                        bestow.graphics3d.debugDrawLine(
                            Vec3.new(p1.x, p1.y, p1.z),
                            Vec3.new(p2.x, p2.y, p2.z),
                            shadowColor, 0.0, false
                        )
                        dy = dy + pixelSize * 0.25
                    end
                end
            end
        end
    end

    -- Main text
    for i = 1, #text do
        local c = string.upper(string.sub(text, i, i))
        local pattern = pixeltext.getCharPattern(c)
        local charOffset = startOffset + (i - 1) * charWidth

        for col = 0, pixeltext.FONT_WIDTH - 1 do
            local colBits = pattern[col + 1]
            for row = 0, pixeltext.FONT_HEIGHT - 1 do
                if bitSet(colBits, row) then
                    local px = charOffset + col * pixelSize
                    local py = (pixeltext.FONT_HEIGHT - 1 - row) * pixelSize
                    local halfPx = pixelSize * 0.45

                    -- Draw main text pixel
                    local dy = -halfPx
                    while dy <= halfPx do
                        local p1 = textPos + right * (px - halfPx) + up * (py + dy)
                        local p2 = textPos + right * (px + halfPx) + up * (py + dy)
                        bestow.graphics3d.debugDrawLine(
                            Vec3.new(p1.x, p1.y, p1.z),
                            Vec3.new(p2.x, p2.y, p2.z),
                            color, 0.0, false
                        )
                        dy = dy + pixelSize * 0.25
                    end
                end
            end
        end
    end
end

return pixeltext

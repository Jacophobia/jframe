-- Grid/Checkerboard Material
-- Useful for prototyping and level design

return {
    shader = {
        vertex = "shaders/basic.vert",
        fragment = "shaders/grid.frag"
    },

    uniforms = {
        -- Checkerboard colors
        uColor1 = {0.3, 0.3, 0.3},  -- Dark squares
        uColor2 = {0.5, 0.5, 0.5},  -- Light squares

        -- Grid settings
        uGridScale = 1.0,  -- Size of each square in world units
        uLineWidth = 0.02, -- Width of grid lines
        uLineColor = {0.2, 0.2, 0.2},

        -- Toggle grid lines
        uShowLines = true
    },

    blendMode = "opaque",
    cullMode = "back",
    depthWrite = true,
    depthTest = true,
    hotReload = true
}

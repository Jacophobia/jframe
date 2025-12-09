-- Stained Glass Material
-- Voronoi cells with colored light transmission

return {
    shader = {
        vertex = "shaders/toon.vert",
        fragment = "shaders/stainedglass.frag"
    },

    uniforms = {
        uBaseColor = {0.9, 0.85, 0.7, 1.0},  -- Warm tint for glass colors

        -- Voronoi settings (in shader constants)
        uCellScale = 8.0,
        uLeadWidth = 0.08,
    },

    blendMode = "opaque",
    cullMode = "back",
    depthWrite = true,
    depthTest = true,
    hotReload = true
}

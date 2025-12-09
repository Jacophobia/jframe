-- Halftone Material
-- Comic book style with Ben-Day dots pattern

return {
    shader = {
        vertex = ":library:/shaders/toon.vert",
        fragment = ":library:/shaders/halftone.frag"
    },

    uniforms = {
        uBaseColor = {1.0, 0.8, 0.6, 1.0},  -- Paper-tinted base

        -- Dot pattern settings (in shader constants)
        uDotScale = 80.0,
        uDotSmooth = 0.05,
    },

    blendMode = "opaque",
    cullMode = "back",
    depthWrite = true,
    depthTest = true,
    hotReload = true
}

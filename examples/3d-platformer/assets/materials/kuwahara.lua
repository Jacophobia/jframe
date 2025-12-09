-- Kuwahara Material
-- Oil painting effect with brush strokes

return {
    shader = {
        vertex = "shaders/toon.vert",
        fragment = "shaders/kuwahara.frag"
    },

    uniforms = {
        uBaseColor = {0.7, 0.5, 0.3, 1.0},  -- Warm earthy base color

        -- Brush settings (in shader constants)
        uBrushScale = 15.0,
        uColorVariation = 0.15,
    },

    blendMode = "opaque",
    cullMode = "back",
    depthWrite = true,
    depthTest = true,
    hotReload = true
}

-- Painterly/Brushstroke Material
return {
    shader = {
        vertex = "shaders/basic.vert",
        fragment = "shaders/painterly.frag"
    },
    uniforms = {
        uBaseColor = {0.7, 0.5, 0.3, 1.0},
        uBrushSize = 0.05,
        uBrushStrength = 0.3,
        uColorVariation = 0.15,
        uImpasto = 0.2
    },
    blendMode = "opaque",
    cullMode = "back",
    depthWrite = true,
    depthTest = true,
    hotReload = true
}

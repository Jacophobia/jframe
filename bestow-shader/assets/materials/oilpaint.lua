-- Oil Paint Material
return {
    shader = {
        vertex = "shaders/basic.vert",
        fragment = "shaders/oilpaint.frag"
    },
    uniforms = {
        uBaseColor = {0.6, 0.4, 0.3, 1.0},
        uBrushSize = 0.08,
        uThickness = 0.4,
        uColorVariation = 0.2,
        uGlossiness = 0.3
    },
    blendMode = "opaque",
    cullMode = "back",
    depthWrite = true,
    depthTest = true,
    hotReload = true
}

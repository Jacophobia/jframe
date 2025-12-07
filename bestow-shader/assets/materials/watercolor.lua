-- Watercolor Material
return {
    shader = {
        vertex = "shaders/basic.vert",
        fragment = "shaders/watercolor.frag"
    },
    uniforms = {
        uBaseColor = {0.4, 0.6, 0.8, 1.0},
        uBleedAmount = 0.3,
        uPaperTexture = 0.2,
        uEdgeDarkening = 0.4,
        uPigmentDensity = 0.6
    },
    blendMode = "opaque",
    cullMode = "back",
    depthWrite = true,
    depthTest = true,
    hotReload = true
}

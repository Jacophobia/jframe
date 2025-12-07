-- Cross-Hatching Material
return {
    shader = {
        vertex = "shaders/basic.vert",
        fragment = "shaders/hatching.frag"
    },
    uniforms = {
        uBaseColor = {1.0, 1.0, 1.0, 1.0},
        uHatchDensity = 20.0,
        uLineThickness = 0.3,
        uHatchLayers = 3,
        uPaperColor = {0.95, 0.92, 0.88},
        uInkColor = {0.1, 0.1, 0.1}
    },
    blendMode = "opaque",
    cullMode = "back",
    depthWrite = true,
    depthTest = true,
    hotReload = true
}

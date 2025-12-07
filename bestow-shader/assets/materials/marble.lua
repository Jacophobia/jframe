-- Marble Material
return {
    shader = {
        vertex = "shaders/basic.vert",
        fragment = "shaders/marble.frag"
    },
    uniforms = {
        uBaseColor = {1.0, 1.0, 1.0, 1.0},
        uVeinColor = {0.3, 0.3, 0.3},
        uBaseStoneColor = {0.95, 0.95, 0.98},
        uVeinFrequency = 3.0,
        uVeinComplexity = 0.5,
        uGlossiness = 0.7
    },
    blendMode = "opaque",
    cullMode = "back",
    depthWrite = true,
    depthTest = true,
    hotReload = true
}

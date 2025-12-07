-- Checker Debug Material
return {
    shader = {
        vertex = "shaders/basic.vert",
        fragment = "shaders/checker.frag"
    },
    uniforms = {
        uColor1 = {1.0, 1.0, 1.0},
        uColor2 = {0.0, 0.0, 0.0},
        uDensity = 8.0
    },
    blendMode = "opaque",
    cullMode = "back",
    depthWrite = true,
    depthTest = true,
    hotReload = true
}

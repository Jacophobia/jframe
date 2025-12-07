-- Posterize Material
return {
    shader = {
        vertex = "shaders/basic.vert",
        fragment = "shaders/posterize.frag"
    },
    uniforms = {
        uBaseColor = {0.7, 0.5, 0.8, 1.0},
        uLevels = 4,
        uEdgeStrength = 0.2,
        uEdgeColor = {0.0, 0.0, 0.0},
        uQuantizeHSV = false
    },
    blendMode = "opaque",
    cullMode = "back",
    depthWrite = true,
    depthTest = true,
    hotReload = true
}

-- Crystal Material
return {
    shader = {
        vertex = "shaders/basic.vert",
        fragment = "shaders/crystal.frag"
    },
    uniforms = {
        uBaseColor = {0.9, 0.7, 1.0, 0.4},
        uCrystalColor = {1.0, 0.9, 1.0},
        uRefractiveIndex = 2.4,
        uDispersion = 0.3,
        uFacets = 8.0,
        uTransparency = 0.4
    },
    blendMode = "blend",
    cullMode = "back",
    depthWrite = false,
    depthTest = true,
    hotReload = true
}

-- Ice Material
return {
    shader = {
        vertex = "shaders/basic.vert",
        fragment = "shaders/ice.frag"
    },
    uniforms = {
        uBaseColor = {0.9, 0.95, 1.0, 0.6},
        uFrostiness = 0.5,
        uRefractiveIndex = 1.31,
        uIceTint = {0.85, 0.9, 1.0},
        uCrystalSize = 0.1,
        uTransparency = 0.6
    },
    blendMode = "blend",
    cullMode = "back",
    depthWrite = false,
    depthTest = true,
    hotReload = true
}

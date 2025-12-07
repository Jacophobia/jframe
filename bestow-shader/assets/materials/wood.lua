-- Wood Material
return {
    shader = {
        vertex = "shaders/basic.vert",
        fragment = "shaders/wood.frag"
    },
    uniforms = {
        uBaseColor = {1.0, 1.0, 1.0, 1.0},
        uDarkWoodColor = {0.3, 0.2, 0.1},
        uLightWoodColor = {0.7, 0.5, 0.3},
        uGrainFrequency = 5.0,
        uGrainVariation = 0.3,
        uRoughness = 0.5
    },
    blendMode = "opaque",
    cullMode = "back",
    depthWrite = true,
    depthTest = true,
    hotReload = true
}

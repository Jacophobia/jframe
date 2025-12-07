-- Water Material (animated)
return {
    shader = {
        vertex = "shaders/basic.vert",
        fragment = "shaders/water.frag"
    },
    uniforms = {
        uBaseColor = {0.2, 0.5, 0.8, 0.7},
        uWaveSpeed = 1.0,
        uWaveFrequency = 2.0,
        uWaveAmplitude = 0.1,
        uShallowColor = {0.3, 0.7, 0.9},
        uDeepColor = {0.1, 0.2, 0.5},
        uFoamAmount = 0.3,
        uTransparency = 0.7
    },
    blendMode = "blend",
    cullMode = "back",
    depthWrite = false,
    depthTest = true,
    hotReload = true
}

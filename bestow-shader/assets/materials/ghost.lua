-- Ghost Material
return {
    shader = {
        vertex = "shaders/basic.vert",
        fragment = "shaders/ghost.frag"
    },
    uniforms = {
        uBaseColor = {0.9, 0.95, 1.0, 0.3},
        uGhostColor = {0.85, 0.9, 1.0},
        uWispiness = 0.5,
        uFloatSpeed = 1.0,
        uTransparency = 0.3,
        uGlowIntensity = 1.5
    },
    blendMode = "blend",
    cullMode = "back",
    depthWrite = false,
    depthTest = true,
    hotReload = true
}

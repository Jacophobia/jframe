-- Lava Material (animated, emissive)
return {
    shader = {
        vertex = "shaders/basic.vert",
        fragment = "shaders/lava.frag"
    },
    uniforms = {
        uBaseColor = {1.0, 0.3, 0.0, 1.0},
        uHotColor = {1.0, 0.9, 0.5},
        uCoolColor = {0.6, 0.1, 0.0},
        uFlowSpeed = 0.5,
        uFlowComplexity = 3.0,
        uEmissive = 2.0
    },
    blendMode = "opaque",
    cullMode = "back",
    depthWrite = true,
    depthTest = true,
    hotReload = true
}

-- Glass Material
return {
    shader = {
        vertex = "shaders/basic.vert",
        fragment = "shaders/glass.frag"
    },
    uniforms = {
        uBaseColor = {0.9, 0.95, 1.0, 0.3},
        uRefractiveIndex = 1.5,
        uThickness = 0.1,
        uRoughness = 0.05,
        uTintColor = {1.0, 1.0, 1.0},
        uOpacity = 0.3
    },
    blendMode = "blend",
    cullMode = "none",
    depthWrite = false,
    depthTest = true,
    hotReload = true
}

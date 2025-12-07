-- Leather Material
return {
    shader = {
        vertex = "shaders/basic.vert",
        fragment = "shaders/leather.frag"
    },
    uniforms = {
        uBaseColor = {0.4, 0.25, 0.15, 1.0},
        uRoughness = 0.6,
        uPoreSize = 0.02,
        uWrinkleAmount = 0.3,
        uGlossiness = 0.2
    },
    blendMode = "opaque",
    cullMode = "back",
    depthWrite = true,
    depthTest = true,
    hotReload = true
}

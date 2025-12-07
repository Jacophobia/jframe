-- Metallic Material
return {
    shader = {
        vertex = "shaders/basic.vert",
        fragment = "shaders/metallic.frag"
    },
    uniforms = {
        uBaseColor = {0.8, 0.8, 0.8, 1.0},
        uRoughness = 0.2,
        uAnisotropy = 0.0,
        uTintColor = {1.0, 1.0, 1.0},
        uClearcoat = 0.0
    },
    blendMode = "opaque",
    cullMode = "back",
    depthWrite = true,
    depthTest = true,
    hotReload = true
}

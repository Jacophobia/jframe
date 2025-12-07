-- Skin Material
return {
    shader = {
        vertex = "shaders/basic.vert",
        fragment = "shaders/skin.frag"
    },
    uniforms = {
        uBaseColor = {0.95, 0.8, 0.7, 1.0},
        uSubsurfaceColor = {0.9, 0.4, 0.3},
        uScatterWidth = 0.5,
        uOiliness = 0.3,
        uPoreSize = 0.01,
        uSpecularColor = {1.0, 1.0, 1.0}
    },
    blendMode = "opaque",
    cullMode = "back",
    depthWrite = true,
    depthTest = true,
    hotReload = true
}

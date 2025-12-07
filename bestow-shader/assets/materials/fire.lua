-- Fire Effect Material
return {
    shader = {
        vertex = "shaders/basic.vert",
        fragment = "shaders/fire.frag"
    },
    uniforms = {
        uFireColorHot = {1.0, 1.0, 0.8},
        uFireColorMid = {1.0, 0.5, 0.0},
        uFireColorCool = {0.8, 0.1, 0.0},
        uFlameSpeed = 2.0,
        uFlameHeight = 1.0,
        uTurbulence = 0.5
    },
    blendMode = "additive",
    cullMode = "none",
    depthWrite = false,
    depthTest = true,
    hotReload = true
}

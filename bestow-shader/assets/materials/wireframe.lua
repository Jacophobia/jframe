-- Wireframe Debug Material
return {
    shader = {
        vertex = "shaders/basic.vert",
        fragment = "shaders/wireframe.frag"
    },
    uniforms = {
        uBaseColor = {0.2, 0.2, 0.2, 1.0},
        uWireColor = {0.0, 1.0, 0.0},
        uWireThickness = 0.02,
        uShowSolid = false
    },
    blendMode = "blend",
    cullMode = "back",
    depthWrite = true,
    depthTest = true,
    hotReload = true
}

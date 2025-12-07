-- Dissolve Effect Material
return {
    shader = {
        vertex = "shaders/basic.vert",
        fragment = "shaders/dissolve.frag"
    },
    uniforms = {
        uBaseColor = {0.6, 0.6, 0.6, 1.0},
        uDissolveAmount = 0.0,  -- Animate from 0 to 1
        uEdgeColor = {1.0, 0.5, 0.0},
        uEdgeColor2 = {1.0, 1.0, 0.3},
        uEdgeWidth = 0.1,
        uNoiseScale = 5.0
    },
    blendMode = "opaque",
    cullMode = "back",
    depthWrite = true,
    depthTest = true,
    hotReload = true
}

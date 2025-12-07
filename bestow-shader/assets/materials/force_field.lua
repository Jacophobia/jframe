-- Force Field Material
return {
    shader = {
        vertex = "shaders/basic.vert",
        fragment = "shaders/force_field.frag"
    },
    uniforms = {
        uBaseColor = {0.2, 0.5, 1.0, 0.3},
        uFieldColor = {0.3, 0.6, 1.0},
        uHexSize = 0.3,
        uPulseSpeed = 1.5,
        uImpactX = 0.0,  -- Set when hit
        uImpactY = 0.0,  -- Set when hit
        uImpactStrength = 0.0  -- Animate on impact
    },
    blendMode = "blend",
    cullMode = "back",
    depthWrite = false,
    depthTest = true,
    hotReload = true
}

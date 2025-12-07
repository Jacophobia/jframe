-- Glow/Emissive Material
return {
    shader = {
        vertex = "shaders/basic.vert",
        fragment = "shaders/glow.frag"
    },
    uniforms = {
        uBaseColor = {0.2, 0.5, 1.0, 1.0},
        uGlowColor = {0.3, 0.7, 1.0},
        uGlowIntensity = 2.0,
        uPulseSpeed = 1.0,
        uPulseAmount = 0.3,
        uFresnelPower = 2.0
    },
    blendMode = "additive",
    cullMode = "back",
    depthWrite = false,
    depthTest = true,
    hotReload = true
}

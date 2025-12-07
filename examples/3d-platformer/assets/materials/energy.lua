-- Energy Material
-- Blue energy effect (collectibles, power-ups)

return {
    shader = {
        vertex = "shaders/basic.vert",
        fragment = "shaders/glow.frag"
    },

    uniforms = {
        -- Dark base
        uBaseColor = {0.0, 0.05, 0.1, 1.0},

        -- Blue energy glow
        uGlowColor = {0.2, 0.6, 1.0},

        -- Moderate glow
        uGlowIntensity = 1.8,

        -- Fast pulse for energy
        uPulseSpeed = 3.0,
        uPulseMin = 0.4,

        -- Edge glow
        uFresnelGlow = 0.8
    },

    blendMode = "opaque",
    cullMode = "back",
    depthWrite = true,
    depthTest = true,
    hotReload = true
}

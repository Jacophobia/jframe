-- Lava Material
-- Hot glowing lava with pulsing effect (uses glow shader)

return {
    shader = {
        vertex = "shaders/basic.vert",
        fragment = "shaders/glow.frag"
    },

    uniforms = {
        -- Dark base surface
        uBaseColor = {0.05, 0.02, 0.0, 1.0},

        -- Hot lava glow
        uGlowColor = {1.0, 0.2, 0.0},

        -- Strong glow intensity
        uGlowIntensity = 2.5,

        -- Slow pulse for lava
        uPulseSpeed = 0.5,
        uPulseMin = 0.6,

        -- Strong edge glow
        uFresnelGlow = 1.0
    },

    blendMode = "opaque",
    cullMode = "back",
    depthWrite = true,
    depthTest = true,
    hotReload = true
}

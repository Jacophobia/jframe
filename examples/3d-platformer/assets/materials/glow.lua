-- Pulsing Glow Material
-- Emissive material with animated glow effect

return {
    shader = {
        vertex = "shaders/basic.vert",
        fragment = "shaders/glow.frag"
    },

    uniforms = {
        -- Base surface color
        uBaseColor = {0.1, 0.1, 0.1, 1.0},

        -- Glow color (orange/red for lava, blue for energy, etc.)
        uGlowColor = {1.0, 0.5, 0.0},

        -- Glow strength
        uGlowIntensity = 1.5,

        -- Pulse animation
        uPulseSpeed = 2.0,  -- Pulses per second
        uPulseMin = 0.3,    -- Minimum brightness during pulse

        -- Edge glow (fresnel)
        uFresnelGlow = 0.5
    },

    blendMode = "opaque",
    cullMode = "back",
    depthWrite = true,
    depthTest = true,
    hotReload = true
}

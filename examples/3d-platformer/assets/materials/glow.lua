-- Pulsing Glow Material
-- Golden goal platform with animated glow effect

return {
    shader = {
        vertex = "basic.vert",
        fragment = "glow.frag"
    },

    uniforms = {
        -- Golden base surface
        uBaseColor = {0.9, 0.75, 0.2, 1.0},

        -- Warm golden glow
        uGlowColor = {1.0, 0.85, 0.3},

        -- Strong glow for goal visibility
        uGlowIntensity = 1.8,

        -- Gentle pulse animation
        uPulseSpeed = 1.5,  -- Pulses per second
        uPulseMin = 0.6,    -- Stays bright

        -- Strong edge glow (fresnel) for beacon effect
        uFresnelGlow = 0.8
    },

    blendMode = "opaque",
    cullMode = "back",
    depthWrite = true,
    depthTest = true,
    hotReload = true
}

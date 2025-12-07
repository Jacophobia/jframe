-- Hologram Material
-- Sci-fi holographic effect with scanlines and glitch

return {
    shader = {
        vertex = "shaders/basic.vert",
        fragment = "shaders/hologram.frag"
    },

    uniforms = {
        -- Main hologram color (cyan is classic)
        uHoloColor = {0.0, 0.8, 1.0},

        -- Animation speeds
        uScanlineSpeed = 2.0,
        uScanlineCount = 50.0,
        uFlickerSpeed = 10.0,

        -- Glitch intensity (0 = none, 1 = maximum)
        uGlitchIntensity = 0.1,

        -- Edge glow intensity
        uFresnelPower = 2.0
    },

    blendMode = "alphaBlend",
    cullMode = "none",  -- Visible from both sides
    depthWrite = false,
    depthTest = true,
    hotReload = true
}

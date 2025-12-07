-- Sci-Fi Hologram Material
-- Futuristic hologram with scanlines, flickering, and glitches

return {
    shader = {
        vertex = "shaders/util/basic.vert",
        fragment = "shaders/effects/hologram.frag"
    },

    uniforms = {
        -- Base material properties
        uBaseColor = {1.0, 1.0, 1.0, 0.5},

        -- Hologram parameters
        uHologramColor = {0.0, 1.0, 1.0},  -- Cyan hologram
        uScanlineSpeed = 2.0,
        uScanlineScale = 100.0,
        uGlitchIntensity = 0.1,
        uFlickerSpeed = 5.0,

        -- Texture
        uUseTexture = false
    },

    textures = {
        -- uTexture = "textures/hologram_pattern.png"
    },

    -- Hologram is transparent and additive
    blendMode = "alphaBlend",
    cullMode = "none",        -- Show both sides
    depthWrite = false,       -- Don't block objects behind
    depthTest = true,
    hotReload = true
}

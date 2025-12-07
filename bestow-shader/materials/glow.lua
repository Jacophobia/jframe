-- Emissive Glow Material
-- Bright glowing object with pulsing animation

return {
    shader = {
        vertex = "shaders/util/basic.vert",
        fragment = "shaders/effects/glow.frag"
    },

    uniforms = {
        -- Base material properties
        uBaseColor = {1.0, 1.0, 1.0, 1.0},

        -- Glow parameters
        uGlowColor = {0.3, 0.7, 1.0},    -- Light blue glow
        uGlowIntensity = 2.0,
        uPulseSpeed = 2.0,
        uPulseAmount = 0.3,

        -- Texture
        uUseTexture = false
    },

    textures = {
        -- uTexture = "textures/glow_pattern.png"
    },

    blendMode = "additive",   -- Additive blending for bright glow
    cullMode = "back",
    depthWrite = true,
    depthTest = true,
    hotReload = true
}

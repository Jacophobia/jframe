-- Molten Lava Material
-- Flowing lava with emissive glow and heat distortion

return {
    shader = {
        vertex = "shaders/util/basic.vert",
        fragment = "shaders/materials/lava.frag"
    },

    uniforms = {
        -- Lava colors
        uLavaColor1 = {1.0, 0.9, 0.3},   -- Hot yellow-white
        uLavaColor2 = {1.0, 0.3, 0.0},   -- Orange-red

        -- Animation
        uFlowSpeed = 0.5,
        uGlowIntensity = 3.0
    },

    textures = {
        -- Optional noise texture for more variety
        -- uNoiseTexture = "textures/noise.png"
    },

    -- Lava glows brightly
    blendMode = "opaque",
    cullMode = "back",
    depthWrite = true,
    depthTest = true,
    hotReload = true
}

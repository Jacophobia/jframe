-- Dissolve/Disintegration Material
-- Burning dissolve effect with orange edge glow

return {
    shader = {
        vertex = "shaders/util/basic.vert",
        fragment = "shaders/effects/dissolve.frag"
    },

    uniforms = {
        -- Base material properties
        uBaseColor = {1.0, 1.0, 1.0, 1.0},

        -- Dissolve parameters
        uDissolveAmount = 0.5,      -- 0 = solid, 1 = fully dissolved
        uEdgeWidth = 0.1,           -- Width of burning edge
        uEdgeColor = {1.0, 0.5, 0.0},  -- Orange edge glow
        uNoiseScale = 5.0,

        -- Texture
        uUseTexture = false
    },

    textures = {
        -- uTexture = "textures/your_texture.png"
        -- uDissolveNoise = "textures/noise.png"  -- Optional custom noise
    },

    blendMode = "alphaBlend",
    cullMode = "back",
    depthWrite = true,
    depthTest = true,
    hotReload = true
}

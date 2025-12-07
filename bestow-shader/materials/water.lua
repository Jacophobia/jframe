-- Animated Water Material
-- Realistic water with waves, reflections, and foam

return {
    shader = {
        vertex = "shaders/materials/water.vert",
        fragment = "shaders/materials/water.frag"
    },

    uniforms = {
        -- Wave animation (vertex shader)
        uWaveHeight = 0.2,
        uWaveFrequency = 1.0,

        -- Water colors
        uWaterColor = {0.0, 0.2, 0.4, 1.0},      -- Deep water
        uShallowColor = {0.0, 0.6, 0.8, 1.0},    -- Shallow water

        -- Lighting
        uLightDir = {-0.5, -1.0, -0.5},
        uLightColor = {1.0, 1.0, 1.0},

        -- Water properties
        uSpecularPower = 128.0,
        uFoamAmount = 0.3
    },

    textures = {
        -- Optional normal map for more detail
        -- uNormalMap = "textures/water_normal.png"
    },

    -- Water is semi-transparent
    blendMode = "alphaBlend",
    cullMode = "back",
    depthWrite = false,  -- Don't write depth for transparency sorting
    depthTest = true,
    hotReload = true
}

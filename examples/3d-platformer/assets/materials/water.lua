-- Water Material
-- Animated water surface with waves and reflections

return {
    shader = {
        vertex = "shaders/water.vert",
        fragment = "shaders/water.frag"
    },

    uniforms = {
        -- Wave animation (vertex shader)
        uWaveHeight = 0.3,
        uWaveSpeed = 1.0,
        uWaveFrequency = 2.0,

        -- Water colors
        uWaterColor = {0.1, 0.3, 0.5, 0.7},   -- Shallow water
        uDeepColor = {0.0, 0.1, 0.2, 0.9},    -- Deep water

        -- Surface properties
        uFresnelPower = 2.0,    -- Reflection intensity at edges
        uSpecularPower = 64.0,  -- Sun reflection sharpness
        uFoamThreshold = 0.8    -- Wave height for foam effect
    },

    blendMode = "alphaBlend",
    cullMode = "back",
    depthWrite = true,
    depthTest = true,
    hotReload = true
}

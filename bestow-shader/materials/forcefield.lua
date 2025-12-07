-- Sci-Fi Force Field Material
-- Energy shield with hexagonal pattern and intersection glow

return {
    shader = {
        vertex = "shaders/util/basic.vert",
        fragment = "shaders/effects/forcefield.frag"
    },

    uniforms = {
        -- Force field parameters
        uFieldColor = {0.0, 1.0, 1.0, 0.3},  -- Cyan shield
        uHexScale = 10.0,
        uPulseSpeed = 2.0,
        uIntersectionGlow = 2.0
    },

    textures = {
        -- No textures needed - procedural
    },

    -- Force field is transparent and additive at edges
    blendMode = "alphaBlend",
    cullMode = "none",        -- Show both sides of shield
    depthWrite = false,
    depthTest = true,
    hotReload = true
}

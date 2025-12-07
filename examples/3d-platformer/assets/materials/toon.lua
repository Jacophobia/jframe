-- Toon/Cel Shading Material
-- Cartoon-style rendering with discrete light bands

return {
    shader = {
        vertex = "shaders/basic.vert",
        fragment = "shaders/toon.frag"
    },

    uniforms = {
        -- Base color of the material
        uBaseColor = {0.8, 0.4, 0.2, 1.0},  -- Orange-ish

        -- Number of discrete light bands (more = smoother gradient)
        uBands = 3,

        -- Outline/rim effect
        uOutlineWidth = 0.03,
        uOutlineColor = {0.0, 0.0, 0.0},  -- Black outline

        -- Specular highlight threshold
        uSpecularSize = 0.9
    },

    blendMode = "opaque",
    cullMode = "back",
    depthWrite = true,
    depthTest = true,
    hotReload = true
}

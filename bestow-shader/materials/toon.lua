-- Toon/Cel-Shading Material
-- Classic cel-shaded look with discrete light bands and rim lighting

return {
    shader = {
        vertex = "shaders/cel/toon.vert",
        fragment = "shaders/cel/toon.frag"
    },

    uniforms = {
        -- Base material properties
        uBaseColor = {1.0, 1.0, 1.0, 1.0},

        -- Lighting
        uLightDir = {-0.5, -1.0, -0.5},
        uLightColor = {1.0, 1.0, 1.0},
        uAmbientColor = {0.3, 0.3, 0.4},

        -- Cel-shading parameters
        uBands = 3,              -- Number of discrete light bands
        uRimPower = 3.0,         -- Rim light sharpness
        uRimColor = {1.0, 1.0, 1.0},  -- Rim light color

        -- Texture
        uUseTexture = false
    },

    textures = {
        -- uTexture = "textures/your_texture.png"
    },

    -- Render state
    blendMode = "opaque",
    cullMode = "back",
    depthWrite = true,
    depthTest = true,

    -- Enable hot reload during development
    hotReload = true
}

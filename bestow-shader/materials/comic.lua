-- Comic Book Style Material
-- Halftone dots, bold colors, and ink outlines

return {
    shader = {
        vertex = "shaders/cel/comic.vert",
        fragment = "shaders/cel/comic.frag"
    },

    uniforms = {
        -- Base material properties
        uBaseColor = {1.0, 1.0, 1.0, 1.0},

        -- Lighting
        uLightDir = {-0.5, -1.0, -0.5},
        uLightColor = {1.0, 1.0, 1.0},
        uAmbientColor = {0.2, 0.2, 0.3},

        -- Comic book parameters
        uHalftoneScale = 50.0,        -- Size of halftone dots
        uOutlineThickness = 0.02,     -- Thickness of ink outlines
        uColorBoost = 1.3,            -- Saturation boost for bold colors

        -- Texture
        uUseTexture = false
    },

    textures = {
        -- uTexture = "textures/your_texture.png"
    },

    blendMode = "opaque",
    cullMode = "back",
    depthWrite = true,
    depthTest = true,
    hotReload = true
}

-- Metallic Material
-- Shiny chrome-like metallic surface

return {
    shader = {
        vertex = "basic.vert",
        fragment = "metallic.frag"
    },

    uniforms = {
        -- Chrome/silver base
        uBaseColor = {0.8, 0.8, 0.85, 1.0},

        -- Metallic properties
        uRoughness = 0.15,      -- Low roughness = shiny
        uMetallic = 1.0,        -- Fully metallic
        uReflectivity = 1.2     -- Strong reflections
    },

    blendMode = "opaque",
    cullMode = "back",
    depthWrite = true,
    depthTest = true,
    hotReload = true
}

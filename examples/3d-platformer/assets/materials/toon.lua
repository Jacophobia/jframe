-- Toon/Cel Shading Material
-- Cartoon-style rendering with discrete light bands

return {
    shader = {
        vertex = "toon.vert",
        fragment = "toon.frag"
    },

    uniforms = {
        -- Base color of the material (will be overridden per-object)
        uBaseColor = {0.8, 0.6, 0.4, 1.0},

        -- Number of discrete light bands (2-5 typical, more = smoother)
        uBands = 3,

        -- Outline/rim effect
        uOutlineWidth = 0.02,
        uOutlineColor = {0.1, 0.05, 0.0},  -- Dark brown outline

        -- Specular highlight threshold (higher = smaller highlight)
        uSpecularSize = 0.92,

        -- Shadow tint (cool colors make shadows feel deeper)
        uShadowTint = {0.4, 0.5, 0.7}  -- Cool blue-ish shadow
    },

    blendMode = "opaque",
    cullMode = "back",
    depthWrite = true,
    depthTest = true,
    hotReload = true
}

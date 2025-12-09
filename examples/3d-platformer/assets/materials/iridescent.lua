-- Iridescent Material
-- Soap bubble / oil slick / beetle shell effect

return {
    shader = {
        vertex = ":library:/shaders/toon.vert",
        fragment = ":library:/shaders/iridescent.frag"
    },

    uniforms = {
        uBaseColor = {0.3, 0.3, 0.4, 1.0},  -- Dark base for contrast

        -- Iridescence settings (in shader constants)
        uFilmThickness = 2.5,
        uIridescenceStrength = 0.8,
        uFresnelPower = 3.0,
    },

    blendMode = "opaque",
    cullMode = "back",
    depthWrite = true,
    depthTest = true,
    hotReload = true
}

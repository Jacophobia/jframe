-- Anime Material
return {
    shader = {
        vertex = "shaders/basic.vert",
        fragment = "shaders/anime.frag"
    },
    uniforms = {
        uBaseColor = {0.95, 0.85, 0.75, 1.0},
        uShadowColor = {0.7, 0.6, 0.8},
        uShadowSharpness = 0.02,
        uRimPower = 3.0,
        uRimIntensity = 0.8,
        uRimColor = {0.8, 0.9, 1.0},
        uSpecSharpness = 0.98,
        uSpecIntensity = 1.0
    },
    blendMode = "opaque",
    cullMode = "back",
    depthWrite = true,
    depthTest = true,
    hotReload = true
}

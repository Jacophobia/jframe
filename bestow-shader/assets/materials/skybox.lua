-- Skybox Material
return {
    shader = {
        vertex = "shaders/basic.vert",
        fragment = "shaders/skybox.frag"
    },
    uniforms = {
        uSkyColorTop = {0.3, 0.5, 0.9},
        uSkyColorHorizon = {0.6, 0.7, 0.9},
        uGroundColor = {0.4, 0.4, 0.4},
        uSunDirection = {0.5, 0.7, 0.3},
        uSunColor = {1.0, 0.95, 0.8},
        uSunSize = 0.05,
        uShowStars = true
    },
    blendMode = "opaque",
    cullMode = "none",
    depthWrite = false,
    depthTest = false,
    hotReload = true
}

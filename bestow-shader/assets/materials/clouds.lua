-- Clouds Material
return {
    shader = {
        vertex = "shaders/basic.vert",
        fragment = "shaders/clouds.frag"
    },
    uniforms = {
        uCloudColor = {1.0, 1.0, 1.0},
        uSkyColor = {0.5, 0.7, 0.9},
        uCloudSpeed = 0.1,
        uCloudDensity = 0.5,
        uCloudScale = 3.0,
        uSunDirection = {0.5, 0.7, 0.3}
    },
    blendMode = "blend",
    cullMode = "none",
    depthWrite = false,
    depthTest = true,
    hotReload = true
}

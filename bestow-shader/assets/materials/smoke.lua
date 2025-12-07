-- Smoke Effect Material
return {
    shader = {
        vertex = "shaders/basic.vert",
        fragment = "shaders/smoke.frag"
    },
    uniforms = {
        uSmokeColor = {0.5, 0.5, 0.5},
        uDensity = 0.5,
        uRiseSpeed = 0.5,
        uTurbulence = 0.6,
        uDissipation = 0.5
    },
    blendMode = "blend",
    cullMode = "none",
    depthWrite = false,
    depthTest = true,
    hotReload = true
}

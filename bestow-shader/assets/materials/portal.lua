-- Portal Effect Material
return {
    shader = {
        vertex = "shaders/basic.vert",
        fragment = "shaders/portal.frag"
    },
    uniforms = {
        uPortalColor1 = {0.6, 0.2, 1.0},
        uPortalColor2 = {0.2, 0.4, 1.0},
        uRotationSpeed = 1.0,
        uWarpAmount = 0.5,
        uPulseSpeed = 2.0,
        uSunDirection = {0.0, 1.0, 0.0}
    },
    blendMode = "blend",
    cullMode = "none",
    depthWrite = false,
    depthTest = true,
    hotReload = true
}

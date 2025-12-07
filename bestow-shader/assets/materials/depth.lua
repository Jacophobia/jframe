-- Depth Debug Material
return {
    shader = {
        vertex = "shaders/basic.vert",
        fragment = "shaders/depth.frag"
    },
    uniforms = {
        uNearPlane = 0.1,
        uFarPlane = 100.0,
        uLinearize = true
    },
    blendMode = "opaque",
    cullMode = "back",
    depthWrite = true,
    depthTest = true,
    hotReload = true
}

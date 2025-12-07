-- UV Debug Material
return {
    shader = {
        vertex = "shaders/basic.vert",
        fragment = "shaders/uv.frag"
    },
    uniforms = {
        uShowGrid = true,
        uGridDensity = 10.0
    },
    blendMode = "opaque",
    cullMode = "back",
    depthWrite = true,
    depthTest = true,
    hotReload = true
}

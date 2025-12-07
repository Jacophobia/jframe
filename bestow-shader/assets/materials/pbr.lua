-- PBR Material
return {
    shader = {
        vertex = "shaders/basic.vert",
        fragment = "shaders/pbr.frag"
    },
    uniforms = {
        uBaseColor = {0.7, 0.7, 0.7, 1.0},
        uMetallic = 0.0,
        uRoughness = 0.5,
        uAO = 1.0,
        uF0 = {0.04, 0.04, 0.04}
    },
    blendMode = "opaque",
    cullMode = "back",
    depthWrite = true,
    depthTest = true,
    hotReload = true
}

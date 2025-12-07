-- Fabric Material
return {
    shader = {
        vertex = "shaders/basic.vert",
        fragment = "shaders/fabric.frag"
    },
    uniforms = {
        uBaseColor = {0.6, 0.3, 0.2, 1.0},
        uRoughness = 0.8,
        uSheenColor = {1.0, 1.0, 1.0},
        uSheenAmount = 0.3,
        uFuzziness = 0.5,
        uSubsurfaceColor = {0.7, 0.4, 0.3}
    },
    blendMode = "opaque",
    cullMode = "back",
    depthWrite = true,
    depthTest = true,
    hotReload = true
}

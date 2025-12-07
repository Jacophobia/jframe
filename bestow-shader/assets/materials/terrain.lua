-- Terrain Material
return {
    shader = {
        vertex = "shaders/basic.vert",
        fragment = "shaders/terrain.frag"
    },
    uniforms = {
        uGrassColor = {0.3, 0.6, 0.2},
        uDirtColor = {0.5, 0.35, 0.2},
        uRockColor = {0.5, 0.5, 0.5},
        uSandColor = {0.8, 0.7, 0.5},
        uSlopeThreshold = 0.7,
        uHeightScale = 10.0
    },
    blendMode = "opaque",
    cullMode = "back",
    depthWrite = true,
    depthTest = true,
    hotReload = true
}

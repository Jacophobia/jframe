-- Electricity Effect Material
return {
    shader = {
        vertex = "shaders/basic.vert",
        fragment = "shaders/electricity.frag"
    },
    uniforms = {
        uArcColor = {0.5, 0.7, 1.0},
        uBoltSpeed = 10.0,
        uBranchiness = 0.5,
        uThickness = 0.05,
        uIntensity = 2.0
    },
    blendMode = "additive",
    cullMode = "none",
    depthWrite = false,
    depthTest = true,
    hotReload = true
}

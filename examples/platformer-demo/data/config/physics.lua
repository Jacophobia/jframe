-- config/physics.lua
-- Physics configuration

return {
    gravity = 980.0,

    player = {
        size = { width = 50.0, height = 80.0 },
        fixedRotation = true,
        density = 1.0,
        friction = 0.0,
        restitution = 0.0
    },
    coin = {
        size = { width = 30.0, height = 30.0 },
        density = 0.0,
        isSensor = true
    },
    enemy = {
        size = { width = 40.0, height = 40.0 },
        fixedRotation = true,
        density = 1.0,
        friction = 0.3
    },
    platform = {
        defaultSize = { width = 100.0, height = 30.0 },
        density = 0.0,
        friction = 0.5
    }
}

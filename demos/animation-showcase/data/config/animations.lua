-- config/animations.lua
-- Animation clip definitions

return {
    -- Animation clips loaded from FBX files
    clips = {
        idle     = ":library:/animations/idle.fbx",
        walk     = ":library:/animations/walk.fbx",
        jog      = ":library:/animations/jog.fbx",
        run      = ":library:/animations/run.fbx",
        jump     = ":library:/animations/jump.fbx",
        falling  = ":library:/animations/falling.fbx",
        landing  = ":library:/animations/landing.fbx",
        recovery = ":library:/animations/recovery.fbx"
    },

    -- Default playback settings
    defaults = {
        speed = 1.0,
        blendTime = 0.25
    }
}

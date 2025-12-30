-- config/animations.lua
-- Animation clip definitions
-- Hot-reloadable: modify paths and see changes in real-time!

return {
    -- Animation clips loaded from FBX files
    -- Key names must match state machine clip references
    clips = {
        idle             = ":library:/animations/idle.fbx",
        walk             = ":library:/animations/walk.fbx",
        jog              = ":library:/animations/jog.fbx",
        run              = ":library:/animations/run.fbx",
        jump             = ":library:/animations/jumping-up.fbx",
        falling          = ":library:/animations/falling.fbx",
        landing          = ":library:/animations/landing.fbx",
        ["landing-recovery"] = ":library:/animations/landing-recovery.fbx"
    },

    -- Default playback settings
    defaults = {
        speed = 1.0,
        blendTime = 0.25
    }
}

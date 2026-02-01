-- Example: Complete Audio Setup
-- Shows background music, UI sounds, positional audio, and listener

return {
    title = "Audio Example",
    width = 1280,
    height = 720,

    init = function()
        app.main.state = {}

        -- Preload sound effects
        app.main.state.sounds = {
            jump = bestow.assets.registerAsset(AssetType.Sound, "sounds/sfx/jump.wav"),
            hit = bestow.assets.registerAsset(AssetType.Sound, "sounds/sfx/hit.wav"),
            coin = bestow.assets.registerAsset(AssetType.Sound, "sounds/sfx/coin.wav"),
        }
        for _, handle in pairs(app.main.state.sounds) do
            bestow.assets.loadAsset(handle)
        end

        -- Load and play background music
        local musicHandle = bestow.assets.registerAsset(AssetType.Music, "sounds/music/theme.ogg")
        bestow.assets.loadAsset(musicHandle)
        bestow.audio.playOnChannel(bestow.audio.Channel.Music, {
            asset = musicHandle,
            volume = 0.7,
            looping = true,
            fadeInTime = 2.0
        })

        -- Set master volume
        bestow.audio.setMasterVolume(1.0)
    end,

    update = function(dt)
        local state = app.main.state

        -- Update audio listener to match camera
        local cam = bestow.graphics3d.getCamera()
        if cam then
            bestow.audio.setListener({
                position = cam.position,
                forward = cam.rotation:rotateVector(Vec3.forward()),
                up = cam.rotation:rotateVector(Vec3.up()),
                velocity = Vec3.zero()
            })
        end

        -- Play UI sound on key press
        if bestow.input.wasKeyJustPressed(KeyCode.Space) then
            bestow.audio.playOnChannel(bestow.audio.Channel.UI, {
                asset = state.sounds.jump,
                volume = 1.0
            })
        end

        -- Play positional sound (3D) at a world location
        if bestow.input.wasKeyJustPressed(KeyCode.Enter) then
            bestow.audio.playPositional({
                asset = state.sounds.coin,
                position = Vec3.new(5, 1, 0),
                volume = 1.0,
                minDistance = 1.0,
                maxDistance = 20.0
            })
        end

        return true
    end,

    render = function()
        bestow.graphics3d.beginFrame()
        bestow.graphics3d.endFrame()
    end
}

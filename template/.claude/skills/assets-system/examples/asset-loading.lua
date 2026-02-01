-- Example: Asset Loading Patterns
-- Shows registering, loading, async loading, and hot reload handling

-- systems/asset_loader.lua
return {
    handles = {},

    init = function()
        local self = app.systems.asset_loader
        local state = app.main.state

        -- Register assets (creates handles, doesn't load yet)
        self.handles.playerMesh = bestow.assets.registerAsset(AssetType.Mesh, "meshes/player.obj")
        self.handles.enemyMesh = bestow.assets.registerAsset(AssetType.Mesh, "meshes/enemy.obj")
        self.handles.jumpSound = bestow.assets.registerAsset(AssetType.Sound, "sounds/sfx/jump.wav")
        self.handles.hitSound = bestow.assets.registerAsset(AssetType.Sound, "sounds/sfx/hit.wav")
        self.handles.bgMusic = bestow.assets.registerAsset(AssetType.Music, "sounds/music/theme.ogg")

        -- Sync load (blocks until loaded - good for small assets)
        bestow.assets.loadAsset(self.handles.jumpSound)
        bestow.assets.loadAsset(self.handles.hitSound)

        -- Async load (non-blocking - good for large assets)
        state.assetsLoaded = 0
        state.totalAssets = 3

        bestow.assets.loadAssetAsync(self.handles.playerMesh, function(h, assetState)
            if assetState == "Loaded" then
                state.assetsLoaded = state.assetsLoaded + 1
            end
        end)
        bestow.assets.loadAssetAsync(self.handles.enemyMesh, function(h, assetState)
            if assetState == "Loaded" then
                state.assetsLoaded = state.assetsLoaded + 1
            end
        end)
        bestow.assets.loadAssetAsync(self.handles.bgMusic, function(h, assetState)
            if assetState == "Loaded" then
                state.assetsLoaded = state.assetsLoaded + 1
            end
        end)

        -- Enable hot reload for development
        bestow.assets.enableHotReload(true)

        -- Subscribe to asset reload events (table+method pattern)
        self.reloadSubId = bestow.events.subscribe("asset_reloaded", {},
            app.systems.asset_loader, "onAssetReloaded")
    end,

    isLoadingComplete = function()
        local state = app.main.state
        return state.assetsLoaded >= state.totalAssets
    end,

    getLoadProgress = function()
        local state = app.main.state
        return state.assetsLoaded / state.totalAssets
    end,

    onAssetReloaded = function(event)
        print("Asset reloaded: " .. event.path)
    end,

    -- List available library assets
    listAvailable = function()
        local meshes = bestow.assets.listLibraryAssets("meshes")
        for _, path in ipairs(meshes) do
            print("Available mesh: " .. path)
        end
    end,

    shutdown = function()
        local self = app.systems.asset_loader
        if self.reloadSubId then bestow.events.unsubscribe(self.reloadSubId) end
    end
}

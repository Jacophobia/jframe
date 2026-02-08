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
        -- Note: loadAssetAsync does NOT accept a callback in Lua.
        -- Poll with bestow.assets.isLoaded(handle) to check completion.
        bestow.assets.loadAssetAsync(self.handles.playerMesh)
        bestow.assets.loadAssetAsync(self.handles.enemyMesh)
        bestow.assets.loadAssetAsync(self.handles.bgMusic)

        -- Enable hot reload for development
        bestow.assets.enableHotReload(true)

        -- Subscribe to asset reload events (table+method pattern)
        self.reloadSubId = bestow.events.subscribe("asset_reloaded", {},
            app.systems.asset_loader, "onAssetReloaded")
    end,

    -- Poll async loading status (no callback available in Lua)
    isLoadingComplete = function()
        local self = app.systems.asset_loader
        return bestow.assets.isLoaded(self.handles.playerMesh)
            and bestow.assets.isLoaded(self.handles.enemyMesh)
            and bestow.assets.isLoaded(self.handles.bgMusic)
    end,

    getLoadProgress = function()
        local self = app.systems.asset_loader
        local loaded = 0
        local total = 3
        if bestow.assets.isLoaded(self.handles.playerMesh) then loaded = loaded + 1 end
        if bestow.assets.isLoaded(self.handles.enemyMesh) then loaded = loaded + 1 end
        if bestow.assets.isLoaded(self.handles.bgMusic) then loaded = loaded + 1 end
        return loaded / total
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

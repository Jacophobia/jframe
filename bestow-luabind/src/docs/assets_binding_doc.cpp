// bestow-luabind/src/docs/assets_binding_doc.cpp
// API documentation for bestow.assets

module bestow.luabind;

import std;

namespace bestow {

void registerAssetsDoc(DocRegistry& registry) {
    SystemDoc sys;
    sys.name = "assets";
    sys.qualifiedName = "bestow.assets";
    sys.description = "Asset loading, caching, and hot-reload management.";

    // --- Enums ---

    sys.enums.push_back(EnumDoc{
        .name = "Type",
        .qualifiedName = "bestow.assets.Type",
        .description = "Types of assets that can be loaded.",
        .values = {
            {"Texture", "Image file (PNG, JPG, etc.)"},
            {"Sound", "Sound effect (.wav, .ogg)"},
            {"Music", "Music stream (.ogg, .mp3)"},
            {"Font", "Font file (.ttf, .otf)"},
            {"Scene", "Scene definition (.lua)"},
            {"Data", "Generic data file"},
            {"Shader", "Shader source (.glsl, .vert, .frag)"},
            {"NavMesh", "Navigation mesh"},
            {"BehaviorTree", "AI behavior tree"},
            {"Mesh", "3D mesh data"},
            {"Model", "3D model with meshes and animations (.fbx, .gltf)"},
            {"Material", "Material definition (.lua)"},
            {"Cubemap", "Cubemap texture (skybox)"},
        }
    });

    sys.enums.push_back(EnumDoc{
        .name = "State",
        .qualifiedName = "bestow.assets.State",
        .description = "Current state of an asset in the loading pipeline.",
        .values = {
            {"Unloaded", "Not yet loaded into memory"},
            {"Loading", "Currently being loaded (async)"},
            {"Loaded", "Successfully loaded and ready to use"},
            {"Failed", "Loading failed (check logs for errors)"},
        }
    });

    // --- Types ---

    sys.types.push_back(TypeDoc{
        .name = "AssetHandle",
        .qualifiedName = "bestow.assets.AssetHandle",
        .description = "Opaque handle to a registered asset. Used to reference assets throughout the API.",
        .fields = {
            {"uuid", "number", "Unique identifier", true},
            {"type", "AssetType", "The type of this asset", true},
        },
        .methods = {
            {.name = "isValid", .qualifiedName = "bestow.assets.AssetHandle:isValid",
             .description = "Check if this handle points to a valid asset.",
             .returns = {{.type = "boolean", .description = "true if handle is valid"}}},
        },
        .example = "local handle = bestow.assets.registerAsset(bestow.assets.Type.Texture, \"textures/player.png\")\nprint(handle:isValid())  -- true",
    });

    sys.types.push_back(TypeDoc{
        .name = "LibraryAssetInfo",
        .qualifiedName = "bestow.assets.LibraryAssetInfo",
        .description = "Information about an asset discovered in the library directory.",
        .fields = {
            {"relativePath", "string", "Path relative to library root"},
            {"libraryPath", "string", "Full :library:/ prefixed path"},
            {"name", "string", "File name with extension"},
            {"stem", "string", "File name without extension"},
            {"extension", "string", "File extension"},
            {"category", "string", "Top-level category (e.g., 'shaders', 'textures')"},
            {"isDirectory", "boolean", "true if this entry is a directory"},
        },
    });

    // --- Methods ---

    sys.methods.push_back(MethodDoc{
        .name = "registerAsset",
        .qualifiedName = "bestow.assets.registerAsset",
        .description = "Register an asset for tracking and loading. Does not load the asset — call loadAsset() or loadAssetAsync() after.",
        .params = {
            {.name = "type", .type = "AssetType", .description = "The type of asset (e.g., bestow.assets.Type.Texture)"},
            {.name = "path", .type = "string", .description = "File path relative to game root, or :library:/ prefix"},
        },
        .returns = {{.type = "AssetHandle", .description = "Handle to reference this asset"}},
        .example = "local tex = bestow.assets.registerAsset(bestow.assets.Type.Texture, \"textures/player.png\")\nbestow.assets.loadAsset(tex)",
        .seeAlso = {"bestow.assets.loadAsset", "bestow.assets.loadAssetAsync"},
    });

    sys.methods.push_back(MethodDoc{
        .name = "unregisterAsset",
        .qualifiedName = "bestow.assets.unregisterAsset",
        .description = "Unregister an asset, removing it from tracking. Unloads if loaded.",
        .params = {
            {.name = "handle", .type = "AssetHandle", .description = "Handle of the asset to unregister"},
        },
    });

    sys.methods.push_back(MethodDoc{
        .name = "loadAsset",
        .qualifiedName = "bestow.assets.loadAsset",
        .description = "Synchronously load an asset into memory. Blocks until complete.",
        .params = {
            {.name = "handle", .type = "AssetHandle", .description = "Handle of the asset to load"},
        },
        .seeAlso = {"bestow.assets.loadAssetAsync", "bestow.assets.isLoaded"},
    });

    sys.methods.push_back(MethodDoc{
        .name = "loadAssetAsync",
        .qualifiedName = "bestow.assets.loadAssetAsync",
        .description = "Begin asynchronous loading of an asset. Returns immediately.",
        .params = {
            {.name = "handle", .type = "AssetHandle", .description = "Handle of the asset to load"},
        },
        .seeAlso = {"bestow.assets.loadAsset", "bestow.assets.getAssetState"},
    });

    sys.methods.push_back(MethodDoc{
        .name = "unloadAsset",
        .qualifiedName = "bestow.assets.unloadAsset",
        .description = "Unload an asset from memory. The handle remains valid for reloading.",
        .params = {
            {.name = "handle", .type = "AssetHandle", .description = "Handle of the asset to unload"},
        },
    });

    sys.methods.push_back(MethodDoc{
        .name = "getAssetState",
        .qualifiedName = "bestow.assets.getAssetState",
        .description = "Get the current loading state of an asset.",
        .params = {
            {.name = "handle", .type = "AssetHandle", .description = "Handle to query"},
        },
        .returns = {{.type = "AssetState", .description = "Current state (Unloaded, Loading, Loaded, Failed)"}},
    });

    sys.methods.push_back(MethodDoc{
        .name = "isLoaded",
        .qualifiedName = "bestow.assets.isLoaded",
        .description = "Check if an asset is fully loaded and ready to use.",
        .params = {
            {.name = "handle", .type = "AssetHandle", .description = "Handle to query"},
        },
        .returns = {{.type = "boolean", .description = "true if the asset is loaded"}},
    });

    sys.methods.push_back(MethodDoc{
        .name = "loadAll",
        .qualifiedName = "bestow.assets.loadAll",
        .description = "Synchronously load all registered assets.",
    });

    sys.methods.push_back(MethodDoc{
        .name = "unloadAll",
        .qualifiedName = "bestow.assets.unloadAll",
        .description = "Unload all loaded assets from memory.",
    });

    sys.methods.push_back(MethodDoc{
        .name = "getAssetsOfType",
        .qualifiedName = "bestow.assets.getAssetsOfType",
        .description = "Get all registered asset handles of a specific type.",
        .params = {
            {.name = "type", .type = "AssetType", .description = "The asset type to filter by"},
        },
        .returns = {{.type = "AssetHandle[]", .description = "Array of matching asset handles"}},
    });

    sys.methods.push_back(MethodDoc{
        .name = "enableHotReload",
        .qualifiedName = "bestow.assets.enableHotReload",
        .description = "Enable or disable file watching for hot reload.",
        .params = {
            {.name = "enable", .type = "boolean", .description = "true to enable, false to disable"},
        },
        .seeAlso = {"bestow.assets.checkForReloads", "bestow.assets.reloadAsset"},
    });

    sys.methods.push_back(MethodDoc{
        .name = "checkForReloads",
        .qualifiedName = "bestow.assets.checkForReloads",
        .description = "Process pending file-change events and reload modified assets. Call in your update loop.",
    });

    sys.methods.push_back(MethodDoc{
        .name = "reloadAsset",
        .qualifiedName = "bestow.assets.reloadAsset",
        .description = "Force-reload a specific asset from disk.",
        .params = {
            {.name = "handle", .type = "AssetHandle", .description = "Handle of the asset to reload"},
        },
    });

    sys.methods.push_back(MethodDoc{
        .name = "loadModel",
        .qualifiedName = "bestow.assets.loadModel",
        .description = "Load a 3D model file (FBX, glTF, etc.) and return its handle.",
        .params = {
            {.name = "path", .type = "string", .description = "Path to model file (e.g., ':library:/characters/hero.fbx')"},
        },
        .returns = {{.type = "AssetHandle", .description = "Handle to the loaded model"}},
        .example = "local model = bestow.assets.loadModel(\":library:/characters/hero.fbx\")",
        .seeAlso = {"bestow.assets.hasAnimations", "bestow.assets.hasSkeleton"},
    });

    sys.methods.push_back(MethodDoc{
        .name = "hasAnimations",
        .qualifiedName = "bestow.assets.hasAnimations",
        .description = "Check if a loaded model contains animation data.",
        .params = {
            {.name = "handle", .type = "AssetHandle", .description = "Handle to a loaded model"},
        },
        .returns = {{.type = "boolean", .description = "true if the model has animations"}},
    });

    sys.methods.push_back(MethodDoc{
        .name = "hasSkeleton",
        .qualifiedName = "bestow.assets.hasSkeleton",
        .description = "Check if a loaded model contains skeleton/bone data.",
        .params = {
            {.name = "handle", .type = "AssetHandle", .description = "Handle to a loaded model"},
        },
        .returns = {{.type = "boolean", .description = "true if the model has a skeleton"}},
    });

    sys.methods.push_back(MethodDoc{
        .name = "getAnimationCount",
        .qualifiedName = "bestow.assets.getAnimationCount",
        .description = "Get the number of animations in a loaded model.",
        .params = {
            {.name = "handle", .type = "AssetHandle", .description = "Handle to a loaded model"},
        },
        .returns = {{.type = "number", .description = "Number of animations (0 if none or invalid)"}},
    });

    sys.methods.push_back(MethodDoc{
        .name = "getAnimationNames",
        .qualifiedName = "bestow.assets.getAnimationNames",
        .description = "Get the names of all animations in a loaded model.",
        .params = {
            {.name = "handle", .type = "AssetHandle", .description = "Handle to a loaded model"},
        },
        .returns = {{.type = "string[]|nil", .description = "Array of animation names, or nil if invalid"}},
    });

    sys.methods.push_back(MethodDoc{
        .name = "listLibraryAssets",
        .qualifiedName = "bestow.assets.listLibraryAssets",
        .description = "List assets in a library subdirectory (non-recursive).",
        .params = {
            {.name = "relativeDir", .type = "string", .description = "Subdirectory relative to library root", .optional = true, .defaultVal = "\"\""},
        },
        .returns = {{.type = "LibraryAssetInfo[]", .description = "Array of asset info objects"}},
    });

    sys.methods.push_back(MethodDoc{
        .name = "listLibraryAssetsRecursive",
        .qualifiedName = "bestow.assets.listLibraryAssetsRecursive",
        .description = "List all assets in a library subdirectory recursively (files only).",
        .params = {
            {.name = "relativeDir", .type = "string", .description = "Subdirectory relative to library root", .optional = true, .defaultVal = "\"\""},
        },
        .returns = {{.type = "LibraryAssetInfo[]", .description = "Array of asset info objects"}},
    });

    sys.methods.push_back(MethodDoc{
        .name = "listLibraryCategories",
        .qualifiedName = "bestow.assets.listLibraryCategories",
        .description = "Get top-level categories (directories) in the library.",
        .returns = {{.type = "string[]", .description = "Array of category names (e.g., {'shaders', 'textures', 'fonts'})"}},
    });

    // --- Properties ---

    sys.properties.push_back(PropertyDoc{
        .name = "library",
        .type = "table",
        .description = "Nested table for library asset autocomplete. e.g., bestow.assets.library.shaders.debug3d_frag",
        .readOnly = true,
    });

    registry.addSystem(std::move(sys));
}

} // namespace bestow

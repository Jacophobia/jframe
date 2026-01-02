// bestow-luabind/src/bindings/assets_binding.cpp
// Asset system Lua bindings

module;

#include <bestow/sol2_compat.hpp>
#include <spdlog/spdlog.h>

module bestow.luabind;

import std;

namespace bestow {

void bindAssetSystem(sol::state& lua, IAssetSystem& assets) {
    //=========================================================================
    // Asset-related usertypes (these need to be at global scope for sol2)
    //=========================================================================

    // AssetHandle usertype
    lua.new_usertype<AssetHandle>("AssetHandle",
        sol::constructors<AssetHandle()>(),
        "uuid", &AssetHandle::uuid,
        "type", &AssetHandle::type,
        "isValid", &AssetHandle::isValid,
        sol::meta_function::equal_to, [](const AssetHandle& a, const AssetHandle& b) {
            return a == b;
        }
    );

    //=========================================================================
    // bestow.assets table
    //=========================================================================

    sol::table bestow = lua["bestow"];
    sol::table assetsTable = lua.create_table();

    //-------------------------------------------------------------------------
    // AssetType enum (bestow.assets.Type)
    //-------------------------------------------------------------------------

    sol::table assetTypeTable = lua.create_table();
    assetTypeTable["Texture"] = AssetType::Texture;
    assetTypeTable["Sound"] = AssetType::Sound;
    assetTypeTable["Music"] = AssetType::Music;
    assetTypeTable["Font"] = AssetType::Font;
    assetTypeTable["Level"] = AssetType::Level;
    assetTypeTable["Data"] = AssetType::Data;
    assetTypeTable["Shader"] = AssetType::Shader;
    assetTypeTable["NavMesh"] = AssetType::NavMesh;
    assetTypeTable["BehaviorTree"] = AssetType::BehaviorTree;
    assetTypeTable["Mesh"] = AssetType::Mesh;
    assetTypeTable["Model"] = AssetType::Model;
    assetTypeTable["Material"] = AssetType::Material;
    assetTypeTable["Cubemap"] = AssetType::Cubemap;
    assetsTable["Type"] = assetTypeTable;

    //-------------------------------------------------------------------------
    // AssetState enum (bestow.assets.State)
    //-------------------------------------------------------------------------

    sol::table assetStateTable = lua.create_table();
    assetStateTable["Unloaded"] = AssetState::Unloaded;
    assetStateTable["Loading"] = AssetState::Loading;
    assetStateTable["Loaded"] = AssetState::Loaded;
    assetStateTable["Failed"] = AssetState::Failed;
    assetsTable["State"] = assetStateTable;

    //-------------------------------------------------------------------------
    // Registration
    //-------------------------------------------------------------------------

    assetsTable["registerAsset"] = [&assets](AssetType type, const std::string& path) {
        return assets.registerAsset(type, std::filesystem::path(path));
    };

    assetsTable["unregisterAsset"] = [&assets](const AssetHandle& handle) {
        assets.unregisterAsset(handle);
    };

    //-------------------------------------------------------------------------
    // Loading
    //-------------------------------------------------------------------------

    assetsTable["loadAsset"] = [&assets](const AssetHandle& handle) {
        assets.loadAsset(handle);
    };

    assetsTable["loadAssetAsync"] = [&assets](const AssetHandle& handle) {
        assets.loadAssetAsync(handle, nullptr);
    };

    assetsTable["unloadAsset"] = [&assets](const AssetHandle& handle) {
        assets.unloadAsset(handle);
    };

    //-------------------------------------------------------------------------
    // State Queries
    //-------------------------------------------------------------------------

    assetsTable["getAssetState"] = [&assets](const AssetHandle& handle) {
        return assets.getAssetState(handle);
    };

    assetsTable["isLoaded"] = [&assets](const AssetHandle& handle) {
        return assets.isLoaded(handle);
    };

    //-------------------------------------------------------------------------
    // Bulk Operations
    //-------------------------------------------------------------------------

    assetsTable["loadAll"] = [&assets]() {
        assets.loadAll();
    };

    assetsTable["unloadAll"] = [&assets]() {
        assets.unloadAll();
    };

    assetsTable["getAssetsOfType"] = [&assets](AssetType type) {
        return assets.getAssetsOfType(type);
    };

    //-------------------------------------------------------------------------
    // Hot Reload
    //-------------------------------------------------------------------------

    assetsTable["enableHotReload"] = [&assets](bool enable) {
        assets.enableHotReload(enable);
    };

    assetsTable["checkForReloads"] = [&assets]() {
        assets.checkForReloads();
    };

    assetsTable["reloadAsset"] = [&assets](const AssetHandle& handle) {
        assets.reloadAsset(handle);
    };

    //-------------------------------------------------------------------------
    // Library Discovery (for IDE autocomplete and exploration)
    //-------------------------------------------------------------------------

    // LibraryAssetInfo usertype for discovery results
    lua.new_usertype<LibraryAssetInfo>("LibraryAssetInfo",
        "relativePath", &LibraryAssetInfo::relativePath,
        "libraryPath", &LibraryAssetInfo::libraryPath,
        "name", &LibraryAssetInfo::name,
        "stem", &LibraryAssetInfo::stem,
        "extension", &LibraryAssetInfo::extension,
        "category", &LibraryAssetInfo::category,
        "isDirectory", &LibraryAssetInfo::isDirectory
    );

    // List assets in a library subdirectory (non-recursive)
    // Returns array of LibraryAssetInfo
    assetsTable["listLibraryAssets"] = [&assets](sol::optional<std::string> relativeDir) {
        return assets.listLibraryAssets(relativeDir.value_or(""));
    };

    // List assets in a library subdirectory (recursive, files only)
    assetsTable["listLibraryAssetsRecursive"] = [&assets](sol::optional<std::string> relativeDir) {
        return assets.listLibraryAssetsRecursive(relativeDir.value_or(""));
    };

    // Get top-level categories (directories) in the library
    // Returns array of strings like {"shaders", "textures", "fonts"}
    assetsTable["listLibraryCategories"] = [&assets]() {
        return assets.listLibraryCategories();
    };

    // Build a nested library table for autocomplete
    // Creates: bestow.assets.library.shaders.debug3d_frag = ":library:/shaders/debug3d.frag"
    sol::table libraryTable = lua.create_table();
    auto categories = assets.listLibraryCategories();
    for (const auto& category : categories) {
        sol::table categoryTable = lua.create_table();
        auto categoryAssets = assets.listLibraryAssetsRecursive(category);
        for (const auto& asset : categoryAssets) {
            // Convert path to valid Lua identifier (replace . and / with _)
            std::string key = asset.relativePath;
            // Remove category prefix
            if (key.starts_with(category + "/")) {
                key = key.substr(category.size() + 1);
            }
            // Replace path separators and dots with underscores
            for (char& c : key) {
                if (c == '/' || c == '.' || c == '-') c = '_';
            }
            categoryTable[key] = asset.libraryPath;
        }
        libraryTable[category] = categoryTable;
    }
    assetsTable["library"] = libraryTable;

    bestow["assets"] = assetsTable;
}

}  // namespace bestow

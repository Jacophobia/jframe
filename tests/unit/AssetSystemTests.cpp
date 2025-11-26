// tests/unit/AssetSystemTests.cpp
// Asset system unit tests

#include <chrono>
#include <cstddef>
#include <filesystem>
#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <vector>

#include <gtest/gtest.h>

import jframe.assets;
import jframe.assets.impl;
import jframe.types;

namespace jframe::tests {

class AssetSystemTest : public ::testing::Test {
protected:
    void SetUp() override {
        assetSystem_ = createAssetSystem();
    }

    std::unique_ptr<IAssetSystem> assetSystem_;
};

//==========================================================================
// Asset Registration Tests
//==========================================================================

TEST_F(AssetSystemTest, RegisterAssetReturnsValidHandle) {
    AssetHandle handle = assetSystem_->registerAsset(AssetType::Texture, "textures/test.png");
    EXPECT_TRUE(handle.isValid());
    EXPECT_NE(handle.uuid, 0);
    EXPECT_EQ(handle.type, AssetType::Texture);
}

TEST_F(AssetSystemTest, RegisterAssetInitialStateIsUnloaded) {
    AssetHandle handle = assetSystem_->registerAsset(AssetType::Sound, "sounds/jump.wav");
    EXPECT_EQ(assetSystem_->getAssetState(handle), AssetState::Unloaded);
    EXPECT_FALSE(assetSystem_->isLoaded(handle));
}

TEST_F(AssetSystemTest, RegisterMultipleAssetsDifferentTypes) {
    AssetHandle texture = assetSystem_->registerAsset(AssetType::Texture, "textures/player.png");
    AssetHandle sound = assetSystem_->registerAsset(AssetType::Sound, "sounds/explosion.wav");
    AssetHandle font = assetSystem_->registerAsset(AssetType::Font, "fonts/arial.ttf");
    AssetHandle data = assetSystem_->registerAsset(AssetType::Data, "data/config.json");

    EXPECT_TRUE(texture.isValid());
    EXPECT_TRUE(sound.isValid());
    EXPECT_TRUE(font.isValid());
    EXPECT_TRUE(data.isValid());

    EXPECT_EQ(texture.type, AssetType::Texture);
    EXPECT_EQ(sound.type, AssetType::Sound);
    EXPECT_EQ(font.type, AssetType::Font);
    EXPECT_EQ(data.type, AssetType::Data);

    // All handles should be unique
    EXPECT_NE(texture.uuid, sound.uuid);
    EXPECT_NE(texture.uuid, font.uuid);
    EXPECT_NE(sound.uuid, font.uuid);
}

TEST_F(AssetSystemTest, RegisterSamePathMultipleTimes) {
    AssetHandle handle1 = assetSystem_->registerAsset(AssetType::Texture, "textures/shared.png");
    AssetHandle handle2 = assetSystem_->registerAsset(AssetType::Texture, "textures/shared.png");

    // Should create different handles even for same path
    EXPECT_NE(handle1.uuid, handle2.uuid);
}

TEST_F(AssetSystemTest, UnregisterAsset) {
    AssetHandle handle = assetSystem_->registerAsset(AssetType::Texture, "textures/temp.png");
    EXPECT_TRUE(handle.isValid());

    assetSystem_->unregisterAsset(handle);

    // After unregistering, state should be Unloaded (default for non-existent asset)
    EXPECT_EQ(assetSystem_->getAssetState(handle), AssetState::Unloaded);
}

TEST_F(AssetSystemTest, UnregisterUnregisteredAssetDoesNotCrash) {
    AssetHandle invalid = AssetHandle::invalid();
    EXPECT_NO_THROW(assetSystem_->unregisterAsset(invalid));
}

//==========================================================================
// Asset Metadata Tests
//==========================================================================

TEST_F(AssetSystemTest, GetAssetMetadataReturnsCorrectPath) {
    std::filesystem::path path = "textures/player.png";
    AssetHandle handle = assetSystem_->registerAsset(AssetType::Texture, path);

    AssetMetadata metadata = assetSystem_->getAssetMetadata(handle);

    EXPECT_EQ(metadata.handle.uuid, handle.uuid);
    EXPECT_EQ(metadata.sourcePath, path);
    EXPECT_EQ(metadata.state, AssetState::Unloaded);
}

TEST_F(AssetSystemTest, GetAssetMetadataForInvalidHandleReturnsDefault) {
    AssetHandle invalid = AssetHandle::invalid();
    AssetMetadata metadata = assetSystem_->getAssetMetadata(invalid);

    EXPECT_EQ(metadata.handle.uuid, 0);
    EXPECT_TRUE(metadata.sourcePath.empty());
}

TEST_F(AssetSystemTest, GetAssetMetadataAfterLoad) {
    AssetHandle handle = assetSystem_->registerAsset(AssetType::Texture, "textures/test.png");
    assetSystem_->loadAsset(handle);

    AssetMetadata metadata = assetSystem_->getAssetMetadata(handle);

    // Non-existent file should fail to load
    EXPECT_EQ(metadata.state, AssetState::Failed);
    EXPECT_TRUE(metadata.errorMessage.has_value());
}

//==========================================================================
// Asset State Tests
//==========================================================================

TEST_F(AssetSystemTest, GetAssetStateForUnregisteredAsset) {
    AssetHandle invalid{999, AssetType::Texture};
    EXPECT_EQ(assetSystem_->getAssetState(invalid), AssetState::Unloaded);
}

TEST_F(AssetSystemTest, IsLoadedReturnsFalseForUnloadedAsset) {
    AssetHandle handle = assetSystem_->registerAsset(AssetType::Sound, "sounds/bgm.wav");
    EXPECT_FALSE(assetSystem_->isLoaded(handle));
}

TEST_F(AssetSystemTest, IsLoadedReturnsTrueAfterLoad) {
    // Use a real test file that exists
    AssetHandle handle = assetSystem_->registerAsset(AssetType::Data, "../../../tests/testdata/test_config.json");
    assetSystem_->loadAsset(handle);
    EXPECT_TRUE(assetSystem_->isLoaded(handle));
}

TEST_F(AssetSystemTest, AssetStateTransitionsCorrectly) {
    // Use a real test file
    AssetHandle handle = assetSystem_->registerAsset(AssetType::Data, "../../../tests/testdata/test_config.json");

    // Initial state
    EXPECT_EQ(assetSystem_->getAssetState(handle), AssetState::Unloaded);

    // After loading
    assetSystem_->loadAsset(handle);
    AssetState state = assetSystem_->getAssetState(handle);
    EXPECT_TRUE(state == AssetState::Loaded || state == AssetState::Loading);

    // After unloading
    assetSystem_->unloadAsset(handle);
    EXPECT_EQ(assetSystem_->getAssetState(handle), AssetState::Unloaded);
}

//==========================================================================
// Asset Loading Tests (Synchronous)
//==========================================================================

TEST_F(AssetSystemTest, LoadRegisteredAsset) {
    // Use a real test file
    AssetHandle handle = assetSystem_->registerAsset(AssetType::Data, "../../../tests/testdata/test_plaintext.txt");

    assetSystem_->loadAsset(handle);

    EXPECT_TRUE(assetSystem_->isLoaded(handle));
}

TEST_F(AssetSystemTest, LoadUnregisteredAssetDoesNotCrash) {
    AssetHandle invalid{999, AssetType::Texture};
    EXPECT_NO_THROW(assetSystem_->loadAsset(invalid));
}

TEST_F(AssetSystemTest, LoadAssetMultipleTimes) {
    // Use a real test file
    AssetHandle handle = assetSystem_->registerAsset(AssetType::Sound, "../../../tests/testdata/test_sound.wav");

    assetSystem_->loadAsset(handle);
    EXPECT_TRUE(assetSystem_->isLoaded(handle));

    // Loading again should be safe
    assetSystem_->loadAsset(handle);
    EXPECT_TRUE(assetSystem_->isLoaded(handle));
}

TEST_F(AssetSystemTest, LoadMultipleAssets) {
    // Use real test files
    AssetHandle h1 = assetSystem_->registerAsset(AssetType::Data, "../../../tests/testdata/test_config.json");
    AssetHandle h2 = assetSystem_->registerAsset(AssetType::Sound, "../../../tests/testdata/test_sound.wav");
    AssetHandle h3 = assetSystem_->registerAsset(AssetType::Level, "../../../tests/testdata/test_level.lua");

    assetSystem_->loadAsset(h1);
    assetSystem_->loadAsset(h2);
    assetSystem_->loadAsset(h3);

    EXPECT_TRUE(assetSystem_->isLoaded(h1));
    EXPECT_TRUE(assetSystem_->isLoaded(h2));
    EXPECT_TRUE(assetSystem_->isLoaded(h3));
}

//==========================================================================
// Asset Loading Tests (Asynchronous)
//==========================================================================

TEST_F(AssetSystemTest, LoadAssetAsyncWithoutCallback) {
    // Use a real test file
    AssetHandle handle = assetSystem_->registerAsset(AssetType::Data, "../../../tests/testdata/test_plaintext.txt");

    assetSystem_->loadAssetAsync(handle, nullptr);

    // Poll update() until async load completes (with timeout)
    for (int i = 0; i < 100; ++i) {
        assetSystem_->update();
        if (assetSystem_->isLoaded(handle)) break;
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    EXPECT_TRUE(assetSystem_->isLoaded(handle));
}

TEST_F(AssetSystemTest, LoadAssetAsyncWithCallback) {
    // Use a real test file
    AssetHandle handle = assetSystem_->registerAsset(AssetType::Sound, "../../../tests/testdata/test_sound.wav");

    bool callbackInvoked = false;
    AssetHandle receivedHandle = AssetHandle::invalid();
    AssetState receivedState = AssetState::Unloaded;

    assetSystem_->loadAssetAsync(handle, [&](AssetHandle h, AssetState s) {
        callbackInvoked = true;
        receivedHandle = h;
        receivedState = s;
    });

    // Poll update() until callback is invoked (with timeout)
    for (int i = 0; i < 100; ++i) {
        assetSystem_->update();
        if (callbackInvoked) break;
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    EXPECT_TRUE(callbackInvoked);
    EXPECT_EQ(receivedHandle.uuid, handle.uuid);

    // If failed, print error message for debugging
    if (receivedState != AssetState::Loaded) {
        AssetMetadata metadata = assetSystem_->getAssetMetadata(handle);
        if (metadata.errorMessage) {
            std::cerr << "Asset load failed: " << *metadata.errorMessage << std::endl;
        }
    }

    EXPECT_EQ(receivedState, AssetState::Loaded);
}

TEST_F(AssetSystemTest, LoadAssetAsyncMultipleCallbacks) {
    // Use real test files
    AssetHandle h1 = assetSystem_->registerAsset(AssetType::Data, "../../../tests/testdata/test_config.json");
    AssetHandle h2 = assetSystem_->registerAsset(AssetType::Data, "../../../tests/testdata/test_plaintext.txt");

    int callbackCount = 0;

    assetSystem_->loadAssetAsync(h1, [&](AssetHandle, AssetState) { callbackCount++; });
    assetSystem_->loadAssetAsync(h2, [&](AssetHandle, AssetState) { callbackCount++; });

    // Poll update() until both callbacks are invoked (with timeout)
    for (int i = 0; i < 100; ++i) {
        assetSystem_->update();
        if (callbackCount == 2) break;
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    EXPECT_EQ(callbackCount, 2);
}

TEST_F(AssetSystemTest, LoadAssetAsyncStateProgression) {
    // Use a real test file
    AssetHandle handle = assetSystem_->registerAsset(AssetType::Data, "../../../tests/testdata/test_config.json");

    assetSystem_->loadAssetAsync(handle);

    // State should be Loading or Loaded
    AssetState state = assetSystem_->getAssetState(handle);
    EXPECT_TRUE(state == AssetState::Loading || state == AssetState::Loaded);

    // Poll update() until async load completes (with timeout)
    for (int i = 0; i < 100; ++i) {
        assetSystem_->update();
        if (assetSystem_->getAssetState(handle) == AssetState::Loaded) break;
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    // After update, should be Loaded
    EXPECT_EQ(assetSystem_->getAssetState(handle), AssetState::Loaded);
}

//==========================================================================
// Asset Unloading Tests
//==========================================================================

TEST_F(AssetSystemTest, UnloadLoadedAsset) {
    // Use a real test file
    AssetHandle handle = assetSystem_->registerAsset(AssetType::Data, "../../../tests/testdata/test_plaintext.txt");
    assetSystem_->loadAsset(handle);
    EXPECT_TRUE(assetSystem_->isLoaded(handle));

    assetSystem_->unloadAsset(handle);

    EXPECT_FALSE(assetSystem_->isLoaded(handle));
    EXPECT_EQ(assetSystem_->getAssetState(handle), AssetState::Unloaded);
}

TEST_F(AssetSystemTest, UnloadUnloadedAssetDoesNotCrash) {
    AssetHandle handle = assetSystem_->registerAsset(AssetType::Sound, "sounds/test.wav");
    EXPECT_NO_THROW(assetSystem_->unloadAsset(handle));
}

TEST_F(AssetSystemTest, UnloadUnregisteredAssetDoesNotCrash) {
    AssetHandle invalid{999, AssetType::Texture};
    EXPECT_NO_THROW(assetSystem_->unloadAsset(invalid));
}

TEST_F(AssetSystemTest, UnloadAndReload) {
    // Use a real test file
    AssetHandle handle = assetSystem_->registerAsset(AssetType::Data, "../../../tests/testdata/test_config.json");

    assetSystem_->loadAsset(handle);
    EXPECT_TRUE(assetSystem_->isLoaded(handle));

    assetSystem_->unloadAsset(handle);
    EXPECT_FALSE(assetSystem_->isLoaded(handle));

    assetSystem_->loadAsset(handle);
    EXPECT_TRUE(assetSystem_->isLoaded(handle));
}

//==========================================================================
// Bulk Operations Tests
//==========================================================================

TEST_F(AssetSystemTest, LoadAllAssets) {
    // Use real test files
    AssetHandle h1 = assetSystem_->registerAsset(AssetType::Data, "../../../tests/testdata/test_config.json");
    AssetHandle h2 = assetSystem_->registerAsset(AssetType::Sound, "../../../tests/testdata/test_sound.wav");
    AssetHandle h3 = assetSystem_->registerAsset(AssetType::Level, "../../../tests/testdata/test_level.lua");

    assetSystem_->loadAll();

    EXPECT_TRUE(assetSystem_->isLoaded(h1));
    EXPECT_TRUE(assetSystem_->isLoaded(h2));
    EXPECT_TRUE(assetSystem_->isLoaded(h3));
}

TEST_F(AssetSystemTest, LoadAllWithPartiallyLoadedAssets) {
    // Use real test files
    AssetHandle h1 = assetSystem_->registerAsset(AssetType::Data, "../../../tests/testdata/test_config.json");
    AssetHandle h2 = assetSystem_->registerAsset(AssetType::Sound, "../../../tests/testdata/test_sound.wav");

    assetSystem_->loadAsset(h1);
    EXPECT_TRUE(assetSystem_->isLoaded(h1));
    EXPECT_FALSE(assetSystem_->isLoaded(h2));

    assetSystem_->loadAll();

    EXPECT_TRUE(assetSystem_->isLoaded(h1));
    EXPECT_TRUE(assetSystem_->isLoaded(h2));
}

TEST_F(AssetSystemTest, UnloadAllAssets) {
    // Use real test files
    AssetHandle h1 = assetSystem_->registerAsset(AssetType::Data, "../../../tests/testdata/test_config.json");
    AssetHandle h2 = assetSystem_->registerAsset(AssetType::Sound, "../../../tests/testdata/test_sound.wav");
    AssetHandle h3 = assetSystem_->registerAsset(AssetType::Level, "../../../tests/testdata/test_level.lua");

    assetSystem_->loadAll();
    EXPECT_TRUE(assetSystem_->isLoaded(h1));
    EXPECT_TRUE(assetSystem_->isLoaded(h2));
    EXPECT_TRUE(assetSystem_->isLoaded(h3));

    assetSystem_->unloadAll();

    EXPECT_FALSE(assetSystem_->isLoaded(h1));
    EXPECT_FALSE(assetSystem_->isLoaded(h2));
    EXPECT_FALSE(assetSystem_->isLoaded(h3));
}

TEST_F(AssetSystemTest, UnloadAllOnEmptySystemDoesNotCrash) {
    EXPECT_NO_THROW(assetSystem_->unloadAll());
}

//==========================================================================
// Asset Query Tests
//==========================================================================

TEST_F(AssetSystemTest, GetAssetsOfTypeSingleType) {
    AssetHandle h1 = assetSystem_->registerAsset(AssetType::Texture, "textures/1.png");
    AssetHandle h2 = assetSystem_->registerAsset(AssetType::Texture, "textures/2.png");
    AssetHandle h3 = assetSystem_->registerAsset(AssetType::Sound, "sounds/1.wav");

    std::vector<AssetHandle> textures = assetSystem_->getAssetsOfType(AssetType::Texture);

    EXPECT_EQ(textures.size(), 2);
    EXPECT_TRUE(std::find_if(textures.begin(), textures.end(),
                             [&](const AssetHandle& h) { return h.uuid == h1.uuid; }) != textures.end());
    EXPECT_TRUE(std::find_if(textures.begin(), textures.end(),
                             [&](const AssetHandle& h) { return h.uuid == h2.uuid; }) != textures.end());
}

TEST_F(AssetSystemTest, GetAssetsOfTypeNoMatches) {
    assetSystem_->registerAsset(AssetType::Texture, "textures/1.png");
    assetSystem_->registerAsset(AssetType::Texture, "textures/2.png");

    std::vector<AssetHandle> sounds = assetSystem_->getAssetsOfType(AssetType::Sound);

    EXPECT_TRUE(sounds.empty());
}

TEST_F(AssetSystemTest, GetAssetsOfTypeEmptySystem) {
    std::vector<AssetHandle> assets = assetSystem_->getAssetsOfType(AssetType::Texture);
    EXPECT_TRUE(assets.empty());
}

TEST_F(AssetSystemTest, GetAssetsOfTypeAllTypes) {
    assetSystem_->registerAsset(AssetType::Texture, "textures/1.png");
    assetSystem_->registerAsset(AssetType::Sound, "sounds/1.wav");
    assetSystem_->registerAsset(AssetType::Music, "music/bgm.mp3");
    assetSystem_->registerAsset(AssetType::Font, "fonts/main.ttf");
    assetSystem_->registerAsset(AssetType::Data, "data/config.json");

    EXPECT_EQ(assetSystem_->getAssetsOfType(AssetType::Texture).size(), 1);
    EXPECT_EQ(assetSystem_->getAssetsOfType(AssetType::Sound).size(), 1);
    EXPECT_EQ(assetSystem_->getAssetsOfType(AssetType::Music).size(), 1);
    EXPECT_EQ(assetSystem_->getAssetsOfType(AssetType::Font).size(), 1);
    EXPECT_EQ(assetSystem_->getAssetsOfType(AssetType::Data).size(), 1);
}

//==========================================================================
// Raw Data Access Tests
//==========================================================================

TEST_F(AssetSystemTest, GetRawAssetUnloadedReturnsNull) {
    AssetHandle handle = assetSystem_->registerAsset(AssetType::Texture, "textures/test.png");

    void* rawData = assetSystem_->getRawAsset(handle);
    EXPECT_EQ(rawData, nullptr);
}

TEST_F(AssetSystemTest, GetRawAssetConstUnloadedReturnsNull) {
    AssetHandle handle = assetSystem_->registerAsset(AssetType::Sound, "sounds/test.wav");

    const IAssetSystem* constSystem = assetSystem_.get();
    const void* rawData = constSystem->getRawAsset(handle);
    EXPECT_EQ(rawData, nullptr);
}

TEST_F(AssetSystemTest, GetRawAssetInvalidHandleReturnsNull) {
    AssetHandle invalid{999, AssetType::Texture};

    void* rawData = assetSystem_->getRawAsset(invalid);
    EXPECT_EQ(rawData, nullptr);
}

TEST_F(AssetSystemTest, GetRawAssetAfterUnload) {
    // Use a real test file
    AssetHandle handle = assetSystem_->registerAsset(AssetType::Data, "../../../tests/testdata/test_plaintext.txt");
    assetSystem_->loadAsset(handle);

    assetSystem_->unloadAsset(handle);

    void* rawData = assetSystem_->getRawAsset(handle);
    EXPECT_EQ(rawData, nullptr);
}

//==========================================================================
// Hot Reload Tests
//==========================================================================

TEST_F(AssetSystemTest, EnableHotReload) {
    EXPECT_NO_THROW(assetSystem_->enableHotReload(true));
    EXPECT_NO_THROW(assetSystem_->enableHotReload(false));
}

TEST_F(AssetSystemTest, CheckForReloadsDoesNotCrash) {
    assetSystem_->enableHotReload(true);
    EXPECT_NO_THROW(assetSystem_->checkForReloads());
}

TEST_F(AssetSystemTest, CheckForReloadsWhenDisabled) {
    assetSystem_->enableHotReload(false);
    EXPECT_NO_THROW(assetSystem_->checkForReloads());
}

TEST_F(AssetSystemTest, ReloadAsset) {
    // Use a real test file
    AssetHandle handle = assetSystem_->registerAsset(AssetType::Data, "../../../tests/testdata/test_plaintext.txt");
    assetSystem_->loadAsset(handle);
    EXPECT_TRUE(assetSystem_->isLoaded(handle));

    assetSystem_->reloadAsset(handle);

    EXPECT_TRUE(assetSystem_->isLoaded(handle));
}

TEST_F(AssetSystemTest, ReloadUnloadedAsset) {
    // Use a real test file
    AssetHandle handle = assetSystem_->registerAsset(AssetType::Sound, "../../../tests/testdata/test_sound.wav");

    assetSystem_->reloadAsset(handle);

    EXPECT_TRUE(assetSystem_->isLoaded(handle));
}

TEST_F(AssetSystemTest, ReloadUnregisteredAssetDoesNotCrash) {
    AssetHandle invalid{999, AssetType::Texture};
    EXPECT_NO_THROW(assetSystem_->reloadAsset(invalid));
}

TEST_F(AssetSystemTest, CheckForReloadsDetectsModifiedFile) {
    // Create a temporary test file
    std::filesystem::path tempFilePath = std::filesystem::temp_directory_path() / "jframe_test_hotreload.txt";

    // Write initial content
    {
        std::ofstream file(tempFilePath);
        file << "Initial content";
    }

    // Register and load the asset
    AssetHandle handle = assetSystem_->registerAsset(AssetType::Data, tempFilePath);
    assetSystem_->loadAsset(handle);
    EXPECT_TRUE(assetSystem_->isLoaded(handle));

    // Get initial data
    void* rawData1 = assetSystem_->getRawAsset(handle);
    ASSERT_NE(rawData1, nullptr);
    auto* anyData1 = static_cast<std::any*>(rawData1);
    const DataAsset& dataAsset1 = std::any_cast<const DataAsset&>(*anyData1);
    std::string initialContent = dataAsset1.rawText;
    EXPECT_EQ(initialContent, "Initial content");

    // Wait a bit to ensure file system timestamp resolution
    std::this_thread::sleep_for(std::chrono::milliseconds(1100));

    // Modify the file
    {
        std::ofstream file(tempFilePath);
        file << "Modified content";
    }

    // Enable hot reload and check for changes
    assetSystem_->enableHotReload(true);
    assetSystem_->checkForReloads();

    // Verify the asset was reloaded with new content
    void* rawData2 = assetSystem_->getRawAsset(handle);
    ASSERT_NE(rawData2, nullptr);
    auto* anyData2 = static_cast<std::any*>(rawData2);
    const DataAsset& dataAsset2 = std::any_cast<const DataAsset&>(*anyData2);
    std::string modifiedContent = dataAsset2.rawText;
    EXPECT_EQ(modifiedContent, "Modified content");

    // Cleanup
    std::filesystem::remove(tempFilePath);
}

TEST_F(AssetSystemTest, CheckForReloadsIgnoresUnmodifiedFiles) {
    // Create a temporary test file
    std::filesystem::path tempFilePath = std::filesystem::temp_directory_path() / "jframe_test_unmodified.txt";

    {
        std::ofstream file(tempFilePath);
        file << "Unmodified content";
    }

    AssetHandle handle = assetSystem_->registerAsset(AssetType::Data, tempFilePath);
    assetSystem_->loadAsset(handle);
    EXPECT_TRUE(assetSystem_->isLoaded(handle));

    // Get initial data to verify it wasn't changed
    void* rawData1 = assetSystem_->getRawAsset(handle);
    ASSERT_NE(rawData1, nullptr);
    auto* anyData1 = static_cast<std::any*>(rawData1);
    const DataAsset& dataAsset1 = std::any_cast<const DataAsset&>(*anyData1);
    std::string initialContent = dataAsset1.rawText;

    // Enable hot reload and check (file hasn't changed)
    assetSystem_->enableHotReload(true);
    assetSystem_->checkForReloads();

    // Verify asset is still loaded with same content
    EXPECT_TRUE(assetSystem_->isLoaded(handle));
    void* rawData2 = assetSystem_->getRawAsset(handle);
    ASSERT_NE(rawData2, nullptr);
    auto* anyData2 = static_cast<std::any*>(rawData2);
    const DataAsset& dataAsset2 = std::any_cast<const DataAsset&>(*anyData2);
    EXPECT_EQ(dataAsset2.rawText, initialContent);

    // Cleanup
    std::filesystem::remove(tempFilePath);
}

TEST_F(AssetSystemTest, CheckForReloadsIgnoresNonExistentFiles) {
    // Create a temporary file, load it, then delete it
    std::filesystem::path tempFilePath = std::filesystem::temp_directory_path() / "jframe_test_deleted.txt";

    {
        std::ofstream file(tempFilePath);
        file << "Temporary content";
    }

    AssetHandle handle = assetSystem_->registerAsset(AssetType::Data, tempFilePath);
    assetSystem_->loadAsset(handle);
    EXPECT_TRUE(assetSystem_->isLoaded(handle));

    // Delete the file
    std::filesystem::remove(tempFilePath);

    // Enable hot reload and check - should not crash
    assetSystem_->enableHotReload(true);
    EXPECT_NO_THROW(assetSystem_->checkForReloads());

    // Asset should still be loaded (not reloaded since file is gone)
    EXPECT_TRUE(assetSystem_->isLoaded(handle));
}

TEST_F(AssetSystemTest, CheckForReloadsIgnoresUnloadedAssets) {
    AssetHandle handle = assetSystem_->registerAsset(AssetType::Data, "../../../tests/testdata/test_config.json");

    // Don't load the asset
    EXPECT_FALSE(assetSystem_->isLoaded(handle));

    // Enable hot reload and check
    assetSystem_->enableHotReload(true);
    EXPECT_NO_THROW(assetSystem_->checkForReloads());

    // Asset should still be unloaded
    EXPECT_FALSE(assetSystem_->isLoaded(handle));
}

TEST_F(AssetSystemTest, CheckForReloadsIgnoresAssetsBeingLoaded) {
    // Create a temporary test file
    std::filesystem::path tempFilePath = std::filesystem::temp_directory_path() / "jframe_test_loading.txt";
    {
        std::ofstream file(tempFilePath);
        file << "Loading test content";
    }

    AssetHandle handle = assetSystem_->registerAsset(AssetType::Data, tempFilePath);

    // Start async load but don't wait for it
    assetSystem_->loadAssetAsync(handle);

    // Enable hot reload and check - should not interfere with pending load
    assetSystem_->enableHotReload(true);
    EXPECT_NO_THROW(assetSystem_->checkForReloads());

    // Wait for async load to complete
    for (int i = 0; i < 100; ++i) {
        assetSystem_->update();
        if (assetSystem_->isLoaded(handle)) break;
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    EXPECT_TRUE(assetSystem_->isLoaded(handle));

    // Cleanup
    std::filesystem::remove(tempFilePath);
}

TEST_F(AssetSystemTest, CheckForReloadsWithMultipleAssets) {
    // Create multiple temporary test files
    std::filesystem::path tempFile1 = std::filesystem::temp_directory_path() / "jframe_test_multi1.txt";
    std::filesystem::path tempFile2 = std::filesystem::temp_directory_path() / "jframe_test_multi2.txt";
    std::filesystem::path tempFile3 = std::filesystem::temp_directory_path() / "jframe_test_multi3.txt";

    {
        std::ofstream file1(tempFile1);
        file1 << "File 1 initial";
        std::ofstream file2(tempFile2);
        file2 << "File 2 initial";
        std::ofstream file3(tempFile3);
        file3 << "File 3 initial";
    }

    // Register and load all assets
    AssetHandle handle1 = assetSystem_->registerAsset(AssetType::Data, tempFile1);
    AssetHandle handle2 = assetSystem_->registerAsset(AssetType::Data, tempFile2);
    AssetHandle handle3 = assetSystem_->registerAsset(AssetType::Data, tempFile3);

    assetSystem_->loadAsset(handle1);
    assetSystem_->loadAsset(handle2);
    assetSystem_->loadAsset(handle3);

    EXPECT_TRUE(assetSystem_->isLoaded(handle1));
    EXPECT_TRUE(assetSystem_->isLoaded(handle2));
    EXPECT_TRUE(assetSystem_->isLoaded(handle3));

    // Wait for file system timestamp resolution
    std::this_thread::sleep_for(std::chrono::milliseconds(1100));

    // Modify only file2
    {
        std::ofstream file2(tempFile2);
        file2 << "File 2 modified";
    }

    // Enable hot reload and check
    assetSystem_->enableHotReload(true);
    assetSystem_->checkForReloads();

    // Verify file2 was reloaded, others weren't
    void* rawData1 = assetSystem_->getRawAsset(handle1);
    auto* anyData1 = static_cast<std::any*>(rawData1);
    const DataAsset& dataAsset1 = std::any_cast<const DataAsset&>(*anyData1);
    EXPECT_EQ(dataAsset1.rawText, "File 1 initial");

    void* rawData2 = assetSystem_->getRawAsset(handle2);
    auto* anyData2 = static_cast<std::any*>(rawData2);
    const DataAsset& dataAsset2 = std::any_cast<const DataAsset&>(*anyData2);
    EXPECT_EQ(dataAsset2.rawText, "File 2 modified");

    void* rawData3 = assetSystem_->getRawAsset(handle3);
    auto* anyData3 = static_cast<std::any*>(rawData3);
    const DataAsset& dataAsset3 = std::any_cast<const DataAsset&>(*anyData3);
    EXPECT_EQ(dataAsset3.rawText, "File 3 initial");

    // Cleanup
    std::filesystem::remove(tempFile1);
    std::filesystem::remove(tempFile2);
    std::filesystem::remove(tempFile3);
}

//==========================================================================
// Update Loop Tests
//==========================================================================

TEST_F(AssetSystemTest, UpdateWithNoPendingLoads) {
    EXPECT_NO_THROW(assetSystem_->update());
}

TEST_F(AssetSystemTest, UpdateProcessesPendingLoads) {
    // Use a real test file
    AssetHandle handle = assetSystem_->registerAsset(AssetType::Data, "../../../tests/testdata/test_plaintext.txt");

    bool processed = false;
    assetSystem_->loadAssetAsync(handle, [&](AssetHandle, AssetState) {
        processed = true;
    });

    EXPECT_FALSE(processed);

    // Poll update() until callback is invoked (with timeout)
    for (int i = 0; i < 100; ++i) {
        assetSystem_->update();
        if (processed) break;
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    EXPECT_TRUE(processed);
}

TEST_F(AssetSystemTest, UpdateMultipleTimes) {
    // Use a real test file
    AssetHandle handle = assetSystem_->registerAsset(AssetType::Data, "../../../tests/testdata/test_config.json");
    assetSystem_->loadAssetAsync(handle);

    // Poll update() until async load completes (with timeout)
    for (int i = 0; i < 100; ++i) {
        assetSystem_->update();
        if (assetSystem_->isLoaded(handle)) break;
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    EXPECT_TRUE(assetSystem_->isLoaded(handle));
}

//==========================================================================
// Asset Type Tests
//==========================================================================

TEST_F(AssetSystemTest, RegisterAllAssetTypes) {
    AssetHandle texture = assetSystem_->registerAsset(AssetType::Texture, "textures/sprite.png");
    AssetHandle sound = assetSystem_->registerAsset(AssetType::Sound, "sounds/effect.wav");
    AssetHandle music = assetSystem_->registerAsset(AssetType::Music, "music/bgm.mp3");
    AssetHandle font = assetSystem_->registerAsset(AssetType::Font, "fonts/arial.ttf");
    AssetHandle level = assetSystem_->registerAsset(AssetType::Level, "levels/level1.lua");
    AssetHandle data = assetSystem_->registerAsset(AssetType::Data, "data/config.json");
    AssetHandle shader = assetSystem_->registerAsset(AssetType::Shader, "shaders/basic.glsl");
    AssetHandle navMesh = assetSystem_->registerAsset(AssetType::NavMesh, "navmesh/level1.nav");
    AssetHandle behaviorTree = assetSystem_->registerAsset(AssetType::BehaviorTree, "ai/enemy.bt");

    EXPECT_EQ(texture.type, AssetType::Texture);
    EXPECT_EQ(sound.type, AssetType::Sound);
    EXPECT_EQ(music.type, AssetType::Music);
    EXPECT_EQ(font.type, AssetType::Font);
    EXPECT_EQ(level.type, AssetType::Level);
    EXPECT_EQ(data.type, AssetType::Data);
    EXPECT_EQ(shader.type, AssetType::Shader);
    EXPECT_EQ(navMesh.type, AssetType::NavMesh);
    EXPECT_EQ(behaviorTree.type, AssetType::BehaviorTree);
}

TEST_F(AssetSystemTest, LoadAllAssetTypes) {
    // Use real test files where possible
    AssetHandle sound = assetSystem_->registerAsset(AssetType::Sound, "../../../tests/testdata/test_sound.wav");
    AssetHandle music = assetSystem_->registerAsset(AssetType::Music, "../../../tests/testdata/test_sound.wav");
    AssetHandle data = assetSystem_->registerAsset(AssetType::Data, "../../../tests/testdata/test_config.json");

    assetSystem_->loadAsset(sound);
    assetSystem_->loadAsset(music);
    assetSystem_->loadAsset(data);

    EXPECT_TRUE(assetSystem_->isLoaded(sound));
    EXPECT_TRUE(assetSystem_->isLoaded(music));
    EXPECT_TRUE(assetSystem_->isLoaded(data));

    // Test types that don't have files - they should fail
    AssetHandle texture = assetSystem_->registerAsset(AssetType::Texture, "textures/sprite.png");
    AssetHandle font = assetSystem_->registerAsset(AssetType::Font, "fonts/arial.ttf");

    assetSystem_->loadAsset(texture);
    assetSystem_->loadAsset(font);

    EXPECT_FALSE(assetSystem_->isLoaded(texture));
    EXPECT_FALSE(assetSystem_->isLoaded(font));
}

//==========================================================================
// Edge Cases and Error Handling
//==========================================================================

TEST_F(AssetSystemTest, InvalidHandleOperationsDoNotCrash) {
    AssetHandle invalid = AssetHandle::invalid();

    EXPECT_NO_THROW(assetSystem_->loadAsset(invalid));
    EXPECT_NO_THROW(assetSystem_->loadAssetAsync(invalid));
    EXPECT_NO_THROW(assetSystem_->unloadAsset(invalid));
    EXPECT_NO_THROW(assetSystem_->unregisterAsset(invalid));
    EXPECT_NO_THROW(assetSystem_->reloadAsset(invalid));
    EXPECT_EQ(assetSystem_->getAssetState(invalid), AssetState::Unloaded);
    EXPECT_EQ(assetSystem_->getRawAsset(invalid), nullptr);
}

TEST_F(AssetSystemTest, EmptyPathRegistration) {
    AssetHandle handle = assetSystem_->registerAsset(AssetType::Texture, "");
    EXPECT_TRUE(handle.isValid());

    AssetMetadata metadata = assetSystem_->getAssetMetadata(handle);
    EXPECT_TRUE(metadata.sourcePath.empty());
}

TEST_F(AssetSystemTest, LongPathRegistration) {
    std::filesystem::path longPath = "assets/textures/characters/enemies/boss/phase1/attack/sprite_sheet_001.png";
    AssetHandle handle = assetSystem_->registerAsset(AssetType::Texture, longPath);

    AssetMetadata metadata = assetSystem_->getAssetMetadata(handle);
    EXPECT_EQ(metadata.sourcePath, longPath);
}

TEST_F(AssetSystemTest, ManyAssetsRegistration) {
    std::vector<AssetHandle> handles;
    for (int i = 0; i < 1000; ++i) {
        std::string filename = "textures/sprite_" + std::to_string(i) + ".png";
        handles.push_back(assetSystem_->registerAsset(
            AssetType::Texture,
            filename
        ));
    }

    EXPECT_EQ(handles.size(), 1000);

    // Verify all handles are unique
    for (size_t i = 0; i < handles.size(); ++i) {
        for (size_t j = i + 1; j < handles.size(); ++j) {
            EXPECT_NE(handles[i].uuid, handles[j].uuid);
        }
    }
}

//==========================================================================
// Data and Level Asset Loading Tests
//==========================================================================

TEST_F(AssetSystemTest, LoadDataAssetJSON) {
    // Path relative to build/macos-debug/tests directory
    std::filesystem::path testDataPath = "../../../tests/testdata/test_config.json";
    AssetHandle handle = assetSystem_->registerAsset(AssetType::Data, testDataPath);

    assetSystem_->loadAsset(handle);

    EXPECT_TRUE(assetSystem_->isLoaded(handle));
    EXPECT_EQ(assetSystem_->getAssetState(handle), AssetState::Loaded);

    // Get the loaded data
    void* rawData = assetSystem_->getRawAsset(handle);
    ASSERT_NE(rawData, nullptr);

    // Cast to std::any and extract DataAsset
    auto* anyData = static_cast<std::any*>(rawData);
    ASSERT_TRUE(anyData->has_value());
    ASSERT_TRUE(anyData->type() == typeid(DataAsset));

    const DataAsset& dataAsset = std::any_cast<const DataAsset&>(*anyData);
    EXPECT_TRUE(dataAsset.isJson);
    EXPECT_FALSE(dataAsset.rawText.empty());

    // Verify JSON was parsed correctly
    EXPECT_TRUE(dataAsset.jsonData.contains("name"));
    EXPECT_EQ(dataAsset.jsonData["name"], "TestGame");
    EXPECT_TRUE(dataAsset.jsonData.contains("settings"));
    EXPECT_EQ(dataAsset.jsonData["settings"]["windowWidth"], 1920);
}

TEST_F(AssetSystemTest, LoadDataAssetNonJSON) {
    std::filesystem::path testDataPath = "../../../tests/testdata/test_plaintext.txt";
    AssetHandle handle = assetSystem_->registerAsset(AssetType::Data, testDataPath);

    assetSystem_->loadAsset(handle);

    EXPECT_TRUE(assetSystem_->isLoaded(handle));

    void* rawData = assetSystem_->getRawAsset(handle);
    ASSERT_NE(rawData, nullptr);

    auto* anyData = static_cast<std::any*>(rawData);
    ASSERT_TRUE(anyData->has_value());

    const DataAsset& dataAsset = std::any_cast<const DataAsset&>(*anyData);
    EXPECT_FALSE(dataAsset.isJson);  // Plain text, not JSON
    EXPECT_FALSE(dataAsset.rawText.empty());
    EXPECT_TRUE(dataAsset.rawText.find("plain text file") != std::string::npos);
}

TEST_F(AssetSystemTest, LoadLevelAsset) {
    std::filesystem::path testLevelPath = "../../../tests/testdata/test_level.lua";
    AssetHandle handle = assetSystem_->registerAsset(AssetType::Level, testLevelPath);

    assetSystem_->loadAsset(handle);

    EXPECT_TRUE(assetSystem_->isLoaded(handle));
    EXPECT_EQ(assetSystem_->getAssetState(handle), AssetState::Loaded);

    void* rawData = assetSystem_->getRawAsset(handle);
    ASSERT_NE(rawData, nullptr);

    auto* anyData = static_cast<std::any*>(rawData);
    ASSERT_TRUE(anyData->has_value());

    const DataAsset& levelAsset = std::any_cast<const DataAsset&>(*anyData);
    EXPECT_FALSE(levelAsset.isJson);  // Lua files are not JSON
    EXPECT_FALSE(levelAsset.rawText.empty());
    EXPECT_TRUE(levelAsset.rawText.find("Test Level 1") != std::string::npos);
    EXPECT_TRUE(levelAsset.rawText.find("return") != std::string::npos);
}

TEST_F(AssetSystemTest, LoadDataAssetFileNotFound) {
    std::filesystem::path nonexistentPath = "data/nonexistent.json";
    AssetHandle handle = assetSystem_->registerAsset(AssetType::Data, nonexistentPath);

    assetSystem_->loadAsset(handle);

    EXPECT_EQ(assetSystem_->getAssetState(handle), AssetState::Failed);
    AssetMetadata metadata = assetSystem_->getAssetMetadata(handle);
    EXPECT_TRUE(metadata.errorMessage.has_value());
    EXPECT_FALSE(metadata.errorMessage->empty());
}

TEST_F(AssetSystemTest, LoadLevelAssetFileNotFound) {
    std::filesystem::path nonexistentPath = "levels/nonexistent.lua";
    AssetHandle handle = assetSystem_->registerAsset(AssetType::Level, nonexistentPath);

    assetSystem_->loadAsset(handle);

    EXPECT_EQ(assetSystem_->getAssetState(handle), AssetState::Failed);
    AssetMetadata metadata = assetSystem_->getAssetMetadata(handle);
    EXPECT_TRUE(metadata.errorMessage.has_value());
}

TEST_F(AssetSystemTest, UnloadDataAsset) {
    std::filesystem::path testDataPath = "../../../tests/testdata/test_config.json";
    AssetHandle handle = assetSystem_->registerAsset(AssetType::Data, testDataPath);

    assetSystem_->loadAsset(handle);
    EXPECT_TRUE(assetSystem_->isLoaded(handle));

    assetSystem_->unloadAsset(handle);

    EXPECT_FALSE(assetSystem_->isLoaded(handle));
    EXPECT_EQ(assetSystem_->getAssetState(handle), AssetState::Unloaded);
    EXPECT_EQ(assetSystem_->getRawAsset(handle), nullptr);
}

TEST_F(AssetSystemTest, ReloadDataAsset) {
    std::filesystem::path testDataPath = "../../../tests/testdata/test_config.json";
    AssetHandle handle = assetSystem_->registerAsset(AssetType::Data, testDataPath);

    assetSystem_->loadAsset(handle);
    EXPECT_TRUE(assetSystem_->isLoaded(handle));

    assetSystem_->reloadAsset(handle);
    EXPECT_TRUE(assetSystem_->isLoaded(handle));

    // Verify data is still accessible
    void* rawData = assetSystem_->getRawAsset(handle);
    ASSERT_NE(rawData, nullptr);

    auto* anyData = static_cast<std::any*>(rawData);
    const DataAsset& dataAsset = std::any_cast<const DataAsset&>(*anyData);
    EXPECT_TRUE(dataAsset.isJson);
}

//==========================================================================
// Sound Loading Tests
//==========================================================================

TEST_F(AssetSystemTest, LoadSoundAssetFromFile) {
    AssetHandle handle = assetSystem_->registerAsset(AssetType::Sound, "../../../tests/testdata/test_sound.wav");

    assetSystem_->loadAsset(handle);

    EXPECT_TRUE(assetSystem_->isLoaded(handle));
    EXPECT_EQ(assetSystem_->getAssetState(handle), AssetState::Loaded);

    // Verify we can get raw data
    void* rawData = assetSystem_->getRawAsset(handle);
    ASSERT_NE(rawData, nullptr);

    // Cast to std::any and extract SoundData
    auto* anyData = static_cast<std::any*>(rawData);
    EXPECT_TRUE(anyData->has_value());

    const SoundData& soundData = std::any_cast<const SoundData&>(*anyData);
    EXPECT_GT(soundData.fileSize, 0);
    EXPECT_EQ(soundData.fileData.size(), soundData.fileSize);
    EXPECT_EQ(soundData.path, "../../../tests/testdata/test_sound.wav");
}

TEST_F(AssetSystemTest, LoadMusicAssetFromFile) {
    AssetHandle handle = assetSystem_->registerAsset(AssetType::Music, "../../../tests/testdata/test_sound.wav");

    assetSystem_->loadAsset(handle);

    EXPECT_TRUE(assetSystem_->isLoaded(handle));
    EXPECT_EQ(assetSystem_->getAssetState(handle), AssetState::Loaded);

    void* rawData = assetSystem_->getRawAsset(handle);
    ASSERT_NE(rawData, nullptr);

    auto* anyData = static_cast<std::any*>(rawData);
    const SoundData& soundData = std::any_cast<const SoundData&>(*anyData);
    EXPECT_GT(soundData.fileSize, 0);
}

TEST_F(AssetSystemTest, LoadSoundNonExistentFileFails) {
    AssetHandle handle = assetSystem_->registerAsset(AssetType::Sound, "nonexistent.wav");

    assetSystem_->loadAsset(handle);

    EXPECT_FALSE(assetSystem_->isLoaded(handle));
    EXPECT_EQ(assetSystem_->getAssetState(handle), AssetState::Failed);

    AssetMetadata metadata = assetSystem_->getAssetMetadata(handle);
    EXPECT_TRUE(metadata.errorMessage.has_value());
}

TEST_F(AssetSystemTest, LoadSoundAndUnload) {
    AssetHandle handle = assetSystem_->registerAsset(AssetType::Sound, "../../../tests/testdata/test_sound.wav");

    assetSystem_->loadAsset(handle);
    EXPECT_TRUE(assetSystem_->isLoaded(handle));

    assetSystem_->unloadAsset(handle);
    EXPECT_FALSE(assetSystem_->isLoaded(handle));

    void* rawData = assetSystem_->getRawAsset(handle);
    EXPECT_EQ(rawData, nullptr);
}

TEST_F(AssetSystemTest, LoadSoundDataIntegrity) {
    AssetHandle handle = assetSystem_->registerAsset(AssetType::Sound, "../../../tests/testdata/test_sound.wav");

    assetSystem_->loadAsset(handle);

    void* rawData = assetSystem_->getRawAsset(handle);
    ASSERT_NE(rawData, nullptr);

    auto* anyData = static_cast<std::any*>(rawData);
    const SoundData& soundData = std::any_cast<const SoundData&>(*anyData);

    // Verify WAV file header (RIFF)
    EXPECT_EQ(soundData.fileData[0], 'R');
    EXPECT_EQ(soundData.fileData[1], 'I');
    EXPECT_EQ(soundData.fileData[2], 'F');
    EXPECT_EQ(soundData.fileData[3], 'F');
}

//==========================================================================
// Font Loading Tests
//==========================================================================

TEST_F(AssetSystemTest, LoadFontAssetFromFile) {
    AssetHandle handle = assetSystem_->registerAsset(AssetType::Font, "../../../tests/testdata/test_font.ttf");

    assetSystem_->loadAsset(handle);

    EXPECT_TRUE(assetSystem_->isLoaded(handle));
    EXPECT_EQ(assetSystem_->getAssetState(handle), AssetState::Loaded);

    // Verify we can get raw data
    void* rawData = assetSystem_->getRawAsset(handle);
    ASSERT_NE(rawData, nullptr);

    // Cast to std::any and extract FontData
    auto* anyData = static_cast<std::any*>(rawData);
    EXPECT_TRUE(anyData->has_value());

    const FontData& fontData = std::any_cast<const FontData&>(*anyData);
    EXPECT_GT(fontData.fileSize, 0);
    EXPECT_EQ(fontData.fileData.size(), fontData.fileSize);
    EXPECT_EQ(fontData.path, "../../../tests/testdata/test_font.ttf");
}

TEST_F(AssetSystemTest, LoadFontDataIntegrity) {
    AssetHandle handle = assetSystem_->registerAsset(AssetType::Font, "../../../tests/testdata/test_font.ttf");

    assetSystem_->loadAsset(handle);

    void* rawData = assetSystem_->getRawAsset(handle);
    ASSERT_NE(rawData, nullptr);

    auto* anyData = static_cast<std::any*>(rawData);
    const FontData& fontData = std::any_cast<const FontData&>(*anyData);

    // Verify TTF file header (sfnt version 0x00010000)
    EXPECT_EQ(fontData.fileData[0], 0x00);
    EXPECT_EQ(fontData.fileData[1], 0x01);
    EXPECT_EQ(fontData.fileData[2], 0x00);
    EXPECT_EQ(fontData.fileData[3], 0x00);
}

TEST_F(AssetSystemTest, LoadFontNonExistentFileFails) {
    AssetHandle handle = assetSystem_->registerAsset(AssetType::Font, "nonexistent.ttf");

    assetSystem_->loadAsset(handle);

    EXPECT_FALSE(assetSystem_->isLoaded(handle));
    EXPECT_EQ(assetSystem_->getAssetState(handle), AssetState::Failed);

    AssetMetadata metadata = assetSystem_->getAssetMetadata(handle);
    EXPECT_TRUE(metadata.errorMessage.has_value());
}

TEST_F(AssetSystemTest, LoadFontAndUnload) {
    AssetHandle handle = assetSystem_->registerAsset(AssetType::Font, "../../../tests/testdata/test_font.ttf");

    assetSystem_->loadAsset(handle);
    EXPECT_TRUE(assetSystem_->isLoaded(handle));

    assetSystem_->unloadAsset(handle);
    EXPECT_FALSE(assetSystem_->isLoaded(handle));

    void* rawData = assetSystem_->getRawAsset(handle);
    EXPECT_EQ(rawData, nullptr);
}

//==========================================================================
// Shader Loading Tests
//==========================================================================

TEST_F(AssetSystemTest, LoadShaderAssetFromFile) {
    AssetHandle handle = assetSystem_->registerAsset(AssetType::Shader, "../../../tests/testdata/test_shader.glsl");

    assetSystem_->loadAsset(handle);

    EXPECT_TRUE(assetSystem_->isLoaded(handle));
    EXPECT_EQ(assetSystem_->getAssetState(handle), AssetState::Loaded);

    // Verify we can get raw data
    void* rawData = assetSystem_->getRawAsset(handle);
    ASSERT_NE(rawData, nullptr);

    // Cast to std::any and extract ShaderData
    auto* anyData = static_cast<std::any*>(rawData);
    EXPECT_TRUE(anyData->has_value());

    const ShaderData& shaderData = std::any_cast<const ShaderData&>(*anyData);
    EXPECT_FALSE(shaderData.source.empty());
    EXPECT_EQ(shaderData.path, "../../../tests/testdata/test_shader.glsl");
}

TEST_F(AssetSystemTest, LoadShaderSourceContent) {
    AssetHandle handle = assetSystem_->registerAsset(AssetType::Shader, "../../../tests/testdata/test_shader.glsl");

    assetSystem_->loadAsset(handle);

    void* rawData = assetSystem_->getRawAsset(handle);
    ASSERT_NE(rawData, nullptr);

    auto* anyData = static_cast<std::any*>(rawData);
    const ShaderData& shaderData = std::any_cast<const ShaderData&>(*anyData);

    // Verify shader contains expected content
    EXPECT_TRUE(shaderData.source.find("#version") != std::string::npos);
    EXPECT_TRUE(shaderData.source.find("void main()") != std::string::npos);
    EXPECT_TRUE(shaderData.source.find("gl_Position") != std::string::npos);
}

TEST_F(AssetSystemTest, LoadShaderNonExistentFileFails) {
    AssetHandle handle = assetSystem_->registerAsset(AssetType::Shader, "nonexistent.glsl");

    assetSystem_->loadAsset(handle);

    EXPECT_FALSE(assetSystem_->isLoaded(handle));
    EXPECT_EQ(assetSystem_->getAssetState(handle), AssetState::Failed);

    AssetMetadata metadata = assetSystem_->getAssetMetadata(handle);
    EXPECT_TRUE(metadata.errorMessage.has_value());
}

TEST_F(AssetSystemTest, LoadShaderAndUnload) {
    AssetHandle handle = assetSystem_->registerAsset(AssetType::Shader, "../../../tests/testdata/test_shader.glsl");

    assetSystem_->loadAsset(handle);
    EXPECT_TRUE(assetSystem_->isLoaded(handle));

    assetSystem_->unloadAsset(handle);
    EXPECT_FALSE(assetSystem_->isLoaded(handle));

    void* rawData = assetSystem_->getRawAsset(handle);
    EXPECT_EQ(rawData, nullptr);
}

TEST_F(AssetSystemTest, LoadShaderAndReload) {
    AssetHandle handle = assetSystem_->registerAsset(AssetType::Shader, "../../../tests/testdata/test_shader.glsl");

    assetSystem_->loadAsset(handle);
    EXPECT_TRUE(assetSystem_->isLoaded(handle));

    assetSystem_->reloadAsset(handle);
    EXPECT_TRUE(assetSystem_->isLoaded(handle));

    // Verify data is still accessible
    void* rawData = assetSystem_->getRawAsset(handle);
    ASSERT_NE(rawData, nullptr);

    auto* anyData = static_cast<std::any*>(rawData);
    const ShaderData& shaderData = std::any_cast<const ShaderData&>(*anyData);
    EXPECT_FALSE(shaderData.source.empty());
}

//==========================================================================
// NavMesh Loading Tests
//==========================================================================

TEST_F(AssetSystemTest, LoadNavMeshAsset) {
    AssetHandle handle = assetSystem_->registerAsset(AssetType::NavMesh, "../../../tests/testdata/test_navmesh.nav");

    assetSystem_->loadAsset(handle);

    EXPECT_TRUE(assetSystem_->isLoaded(handle));
    EXPECT_EQ(assetSystem_->getAssetState(handle), AssetState::Loaded);

    void* rawData = assetSystem_->getRawAsset(handle);
    ASSERT_NE(rawData, nullptr);

    auto* anyData = static_cast<std::any*>(rawData);
    ASSERT_TRUE(anyData->has_value());
    ASSERT_TRUE(anyData->type() == typeid(NavMeshData));

    const NavMeshData& navMeshData = std::any_cast<const NavMeshData&>(*anyData);
    EXPECT_GT(navMeshData.fileSize, 0);
    EXPECT_EQ(navMeshData.fileData.size(), navMeshData.fileSize);
    EXPECT_EQ(navMeshData.path, "../../../tests/testdata/test_navmesh.nav");
}

TEST_F(AssetSystemTest, LoadNavMeshNonExistentFileFails) {
    AssetHandle handle = assetSystem_->registerAsset(AssetType::NavMesh, "navmesh/nonexistent.nav");

    assetSystem_->loadAsset(handle);

    EXPECT_FALSE(assetSystem_->isLoaded(handle));
    EXPECT_EQ(assetSystem_->getAssetState(handle), AssetState::Failed);

    AssetMetadata metadata = assetSystem_->getAssetMetadata(handle);
    EXPECT_TRUE(metadata.errorMessage.has_value());
}

TEST_F(AssetSystemTest, LoadNavMeshAndUnload) {
    AssetHandle handle = assetSystem_->registerAsset(AssetType::NavMesh, "../../../tests/testdata/test_navmesh.nav");

    assetSystem_->loadAsset(handle);
    EXPECT_TRUE(assetSystem_->isLoaded(handle));

    assetSystem_->unloadAsset(handle);
    EXPECT_FALSE(assetSystem_->isLoaded(handle));

    void* rawData = assetSystem_->getRawAsset(handle);
    EXPECT_EQ(rawData, nullptr);
}

TEST_F(AssetSystemTest, LoadNavMeshDataIntegrity) {
    AssetHandle handle = assetSystem_->registerAsset(AssetType::NavMesh, "../../../tests/testdata/test_navmesh.nav");

    assetSystem_->loadAsset(handle);

    void* rawData = assetSystem_->getRawAsset(handle);
    ASSERT_NE(rawData, nullptr);

    auto* anyData = static_cast<std::any*>(rawData);
    const NavMeshData& navMeshData = std::any_cast<const NavMeshData&>(*anyData);

    // Verify we can read the header
    EXPECT_GE(navMeshData.fileData.size(), 20);

    // Check for expected header text in test file
    std::string header(navMeshData.fileData.begin(), navMeshData.fileData.begin() + 20);
    EXPECT_TRUE(header.find("NAVMESH_TEST_DATA") != std::string::npos);
}

//==========================================================================
// BehaviorTree Loading Tests
//==========================================================================

TEST_F(AssetSystemTest, LoadBehaviorTreeAssetJSON) {
    AssetHandle handle = assetSystem_->registerAsset(AssetType::BehaviorTree, "../../../tests/testdata/test_behaviortree.json");

    assetSystem_->loadAsset(handle);

    EXPECT_TRUE(assetSystem_->isLoaded(handle));
    EXPECT_EQ(assetSystem_->getAssetState(handle), AssetState::Loaded);

    void* rawData = assetSystem_->getRawAsset(handle);
    ASSERT_NE(rawData, nullptr);

    auto* anyData = static_cast<std::any*>(rawData);
    ASSERT_TRUE(anyData->has_value());
    ASSERT_TRUE(anyData->type() == typeid(BehaviorTreeData));

    const BehaviorTreeData& btData = std::any_cast<const BehaviorTreeData&>(*anyData);
    EXPECT_TRUE(btData.isJson);
    EXPECT_FALSE(btData.rawText.empty());
    EXPECT_EQ(btData.path, "../../../tests/testdata/test_behaviortree.json");

    // Verify JSON was parsed correctly
    EXPECT_TRUE(btData.treeData.contains("name"));
    EXPECT_EQ(btData.treeData["name"], "TestBehaviorTree");
    EXPECT_TRUE(btData.treeData.contains("root"));
    EXPECT_EQ(btData.treeData["root"]["type"], "Selector");
}

TEST_F(AssetSystemTest, LoadBehaviorTreeAssetNonJSON) {
    AssetHandle handle = assetSystem_->registerAsset(AssetType::BehaviorTree, "../../../tests/testdata/test_behaviortree.txt");

    assetSystem_->loadAsset(handle);

    EXPECT_TRUE(assetSystem_->isLoaded(handle));
    EXPECT_EQ(assetSystem_->getAssetState(handle), AssetState::Loaded);

    void* rawData = assetSystem_->getRawAsset(handle);
    ASSERT_NE(rawData, nullptr);

    auto* anyData = static_cast<std::any*>(rawData);
    const BehaviorTreeData& btData = std::any_cast<const BehaviorTreeData&>(*anyData);

    EXPECT_FALSE(btData.isJson);  // Not JSON
    EXPECT_FALSE(btData.rawText.empty());
    EXPECT_TRUE(btData.rawText.find("tree TestBehaviorTree") != std::string::npos);
}

TEST_F(AssetSystemTest, LoadBehaviorTreeNonExistentFileFails) {
    AssetHandle handle = assetSystem_->registerAsset(AssetType::BehaviorTree, "ai/nonexistent.bt");

    assetSystem_->loadAsset(handle);

    EXPECT_FALSE(assetSystem_->isLoaded(handle));
    EXPECT_EQ(assetSystem_->getAssetState(handle), AssetState::Failed);

    AssetMetadata metadata = assetSystem_->getAssetMetadata(handle);
    EXPECT_TRUE(metadata.errorMessage.has_value());
}

TEST_F(AssetSystemTest, LoadBehaviorTreeAndUnload) {
    AssetHandle handle = assetSystem_->registerAsset(AssetType::BehaviorTree, "../../../tests/testdata/test_behaviortree.json");

    assetSystem_->loadAsset(handle);
    EXPECT_TRUE(assetSystem_->isLoaded(handle));

    assetSystem_->unloadAsset(handle);
    EXPECT_FALSE(assetSystem_->isLoaded(handle));

    void* rawData = assetSystem_->getRawAsset(handle);
    EXPECT_EQ(rawData, nullptr);
}

TEST_F(AssetSystemTest, ReloadBehaviorTreeAsset) {
    AssetHandle handle = assetSystem_->registerAsset(AssetType::BehaviorTree, "../../../tests/testdata/test_behaviortree.json");

    assetSystem_->loadAsset(handle);
    EXPECT_TRUE(assetSystem_->isLoaded(handle));

    assetSystem_->reloadAsset(handle);
    EXPECT_TRUE(assetSystem_->isLoaded(handle));

    // Verify data is still accessible
    void* rawData = assetSystem_->getRawAsset(handle);
    ASSERT_NE(rawData, nullptr);

    auto* anyData = static_cast<std::any*>(rawData);
    const BehaviorTreeData& btData = std::any_cast<const BehaviorTreeData&>(*anyData);
    EXPECT_TRUE(btData.isJson);
    EXPECT_EQ(btData.treeData["name"], "TestBehaviorTree");
}

}  // namespace jframe::tests

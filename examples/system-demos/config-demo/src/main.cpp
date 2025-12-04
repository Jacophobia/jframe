// config-demo/src/main.cpp
// Comprehensive demonstration of Bestow Config System API

// MSVC C++23 module compatibility for sol2 globals
#include <bestow/sol2_compat.hpp>

import std;
import bestow.types;
import bestow.config;
import bestow.config.impl;

using namespace bestow;

//==============================================================================
// Helper Functions
//==============================================================================

void printSectionHeader(const std::string& title) {
    std::cout << "\n";
    std::cout << "========================================\n";
    std::cout << title << "\n";
    std::cout << "========================================\n";
}

void printSubsection(const std::string& title) {
    std::cout << "\n--- " << title << " ---\n";
}

//==============================================================================
// Demo Functions - Each exercises specific API methods
//==============================================================================

void demoLifecycle(IConfigSystem& config) {
    printSectionHeader("1. LIFECYCLE MANAGEMENT");

    std::cout << "Initializing config system...\n";
    bool initialized = config.initialize();
    std::cout << "  Result: " << (initialized ? "SUCCESS" : "FAILED") << "\n";

    std::cout << "\nNote: update() and shutdown() will be called at the end\n";
}

void demoLoadingConfigs(IConfigSystem& config) {
    printSectionHeader("2. LOADING CONFIGURATION FILES");

    printSubsection("Loading game.lua");
    bool gameLoaded = config.loadConfig("data/game.lua");
    std::cout << "  Result: " << (gameLoaded ? "SUCCESS" : "FAILED") << "\n";

    printSubsection("Loading player.lua");
    bool playerLoaded = config.loadConfig("data/player.lua");
    std::cout << "  Result: " << (playerLoaded ? "SUCCESS" : "FAILED") << "\n";

    printSubsection("Attempting to load non-existent file");
    bool badLoad = config.loadConfig("data/nonexistent.lua");
    std::cout << "  Result: " << (badLoad ? "SUCCESS" : "FAILED (expected)") << "\n";
}

void demoSimpleValueAccess(IConfigSystem& config) {
    printSectionHeader("3. TYPE-SAFE VALUE ACCESS");

    printSubsection("Reading Float Values");
    if (auto brightness = config.getFloat("graphics.brightness")) {
        std::cout << "  graphics.brightness = " << *brightness << "\n";
    }
    if (auto moveSpeed = config.getFloat("movement.moveSpeed")) {
        std::cout << "  movement.moveSpeed = " << *moveSpeed << "\n";
    }
    if (auto pi = config.getFloat("computed.pi")) {
        std::cout << "  computed.pi = " << *pi << "\n";
    }

    printSubsection("Reading Integer Values");
    if (auto width = config.getInt("window.width")) {
        std::cout << "  window.width = " << *width << "\n";
    }
    if (auto health = config.getInt("stats.health")) {
        std::cout << "  stats.health = " << *health << "\n";
    }
    if (auto lives = config.getInt("gameplay.startingLives")) {
        std::cout << "  gameplay.startingLives = " << *lives << "\n";
    }

    printSubsection("Reading Boolean Values");
    if (auto fullscreen = config.getBool("window.fullscreen")) {
        std::cout << "  window.fullscreen = " << (*fullscreen ? "true" : "false") << "\n";
    }
    if (auto vsync = config.getBool("window.vsync")) {
        std::cout << "  window.vsync = " << (*vsync ? "true" : "false") << "\n";
    }
    if (auto muted = config.getBool("audio.muted")) {
        std::cout << "  audio.muted = " << (*muted ? "true" : "false") << "\n";
    }

    printSubsection("Reading String Values");
    if (auto title = config.getString("title")) {
        std::cout << "  title = \"" << *title << "\"\n";
    }
    if (auto playerName = config.getString("name")) {
        std::cout << "  name = \"" << *playerName << "\"\n";
    }
    if (auto logLevel = config.getString("debug.logLevel")) {
        std::cout << "  debug.logLevel = \"" << *logLevel << "\"\n";
    }
}

void demoDefaultValues(IConfigSystem& config) {
    printSectionHeader("4. DEFAULT VALUE FALLBACKS");

    printSubsection("Using getFloatOr() - Existing and Missing Keys");
    float fps = config.getFloatOr("graphics.targetFPS", 30.0f);
    std::cout << "  graphics.targetFPS (exists) = " << fps << "\n";

    float missing = config.getFloatOr("nonexistent.value", 99.9f);
    std::cout << "  nonexistent.value (missing) = " << missing << " (default)\n";

    printSubsection("Using getIntOr() - Existing and Missing Keys");
    int width = config.getIntOr("window.width", 800);
    std::cout << "  window.width (exists) = " << width << "\n";

    int missing2 = config.getIntOr("missing.number", 42);
    std::cout << "  missing.number (missing) = " << missing2 << " (default)\n";

    printSubsection("Using getBoolOr() - Existing and Missing Keys");
    bool vsync = config.getBoolOr("window.vsync", false);
    std::cout << "  window.vsync (exists) = " << (vsync ? "true" : "false") << "\n";

    bool missing3 = config.getBoolOr("missing.flag", true);
    std::cout << "  missing.flag (missing) = " << (missing3 ? "true" : "false") << " (default)\n";

    printSubsection("Using getStringOr() - Existing and Missing Keys");
    std::string version = config.getStringOr("version", "0.0.0");
    std::cout << "  version (exists) = \"" << version << "\"\n";

    std::string missing4 = config.getStringOr("missing.text", "default value");
    std::cout << "  missing.text (missing) = \"" << missing4 << "\" (default)\n";
}

void demoArrayAccess(IConfigSystem& config) {
    printSectionHeader("5. ARRAY/LIST ACCESS");

    printSubsection("Reading Integer Arrays");
    auto levelIds = config.getIntArray("levelIds");
    std::cout << "  levelIds: [";
    for (std::size_t i = 0; i < levelIds.size(); i++) {
        std::cout << levelIds[i];
        if (i < levelIds.size() - 1) std::cout << ", ";
    }
    std::cout << "]\n";

    auto abilities = config.getIntArray("unlockedAbilities");
    std::cout << "  unlockedAbilities: [";
    for (std::size_t i = 0; i < abilities.size(); i++) {
        std::cout << abilities[i];
        if (i < abilities.size() - 1) std::cout << ", ";
    }
    std::cout << "]\n";

    printSubsection("Reading Float Arrays");
    auto spawnTimes = config.getFloatArray("spawnTimes");
    std::cout << "  spawnTimes: [";
    for (std::size_t i = 0; i < spawnTimes.size(); i++) {
        std::cout << spawnTimes[i];
        if (i < spawnTimes.size() - 1) std::cout << ", ";
    }
    std::cout << "]\n";

    auto statMods = config.getFloatArray("statModifiers");
    std::cout << "  statModifiers: [";
    for (std::size_t i = 0; i < statMods.size(); i++) {
        std::cout << statMods[i];
        if (i < statMods.size() - 1) std::cout << ", ";
    }
    std::cout << "]\n";

    printSubsection("Reading String Arrays");
    auto levelNames = config.getStringArray("levelNames");
    std::cout << "  levelNames: [";
    for (std::size_t i = 0; i < levelNames.size(); i++) {
        std::cout << "\"" << levelNames[i] << "\"";
        if (i < levelNames.size() - 1) std::cout << ", ";
    }
    std::cout << "]\n";

    auto equipped = config.getStringArray("equippedSlots");
    std::cout << "  equippedSlots: [";
    for (std::size_t i = 0; i < equipped.size(); i++) {
        std::cout << "\"" << equipped[i] << "\"";
        if (i < equipped.size() - 1) std::cout << ", ";
    }
    std::cout << "]\n";

    printSubsection("Empty Array (Missing Key)");
    auto emptyArray = config.getIntArray("does.not.exist");
    std::cout << "  does.not.exist: [] (size=" << emptyArray.size() << ")\n";
}

void demoRuntimeModification(IConfigSystem& config) {
    printSectionHeader("6. RUNTIME VALUE MODIFICATION");

    printSubsection("Modifying Float Values");
    std::cout << "  Original audio.masterVolume = "
              << config.getFloatOr("audio.masterVolume", 0.0f) << "\n";
    config.setFloat("audio.masterVolume", 0.5f);
    std::cout << "  After setFloat(0.5): "
              << config.getFloatOr("audio.masterVolume", 0.0f) << "\n";

    printSubsection("Modifying Integer Values");
    std::cout << "  Original stats.health = "
              << config.getIntOr("stats.health", 0) << "\n";
    config.setInt("stats.health", 75);
    std::cout << "  After setInt(75): "
              << config.getIntOr("stats.health", 0) << "\n";

    printSubsection("Modifying Boolean Values");
    std::cout << "  Original window.fullscreen = "
              << (config.getBoolOr("window.fullscreen", false) ? "true" : "false") << "\n";
    config.setBool("window.fullscreen", true);
    std::cout << "  After setBool(true): "
              << (config.getBoolOr("window.fullscreen", false) ? "true" : "false") << "\n";

    printSubsection("Modifying String Values");
    std::cout << "  Original name = "
              << config.getStringOr("name", "") << "\n";
    config.setString("name", "Modified Hero");
    std::cout << "  After setString(\"Modified Hero\"): "
              << config.getStringOr("name", "") << "\n";

    printSubsection("Creating New Runtime Values");
    config.setFloat("runtime.timestamp", 123.456f);
    config.setInt("runtime.frameCount", 9999);
    config.setBool("runtime.paused", true);
    config.setString("runtime.state", "RUNNING");
    std::cout << "  Created runtime.timestamp = " << config.getFloatOr("runtime.timestamp", 0.0f) << "\n";
    std::cout << "  Created runtime.frameCount = " << config.getIntOr("runtime.frameCount", 0) << "\n";
    std::cout << "  Created runtime.paused = " << (config.getBoolOr("runtime.paused", false) ? "true" : "false") << "\n";
    std::cout << "  Created runtime.state = " << config.getStringOr("runtime.state", "") << "\n";
}

void demoStateQueries(IConfigSystem& config) {
    printSectionHeader("7. STATE QUERIES");

    printSubsection("Checking if Keys Exist");
    std::cout << "  hasKey(\"window.width\") = "
              << (config.hasKey("window.width") ? "true" : "false") << "\n";
    std::cout << "  hasKey(\"stats.health\") = "
              << (config.hasKey("stats.health") ? "true" : "false") << "\n";
    std::cout << "  hasKey(\"nonexistent.key\") = "
              << (config.hasKey("nonexistent.key") ? "true" : "false") << "\n";

    printSubsection("Getting Keys With Prefix");
    auto windowKeys = config.getKeysWithPrefix("window.");
    std::cout << "  Keys with prefix \"window.\":\n";
    for (const auto& key : windowKeys) {
        std::cout << "    - " << key << "\n";
    }

    auto statsKeys = config.getKeysWithPrefix("stats.");
    std::cout << "\n  Keys with prefix \"stats.\":\n";
    for (const auto& key : statsKeys) {
        std::cout << "    - " << key << "\n";
    }

    auto runtimeKeys = config.getKeysWithPrefix("runtime.");
    std::cout << "\n  Keys with prefix \"runtime.\" (created at runtime):\n";
    for (const auto& key : runtimeKeys) {
        std::cout << "    - " << key << "\n";
    }

    printSubsection("Getting Loaded Config Files");
    auto loadedConfigs = config.getLoadedConfigs();
    std::cout << "  Loaded config files:\n";
    for (const auto& path : loadedConfigs) {
        std::cout << "    - " << path << "\n";
    }

    printSubsection("Getting Config Metadata");
    for (const auto& path : loadedConfigs) {
        auto metadata = config.getMetadata(path);
        std::cout << "  Metadata for " << path << ":\n";
        std::cout << "    sourcePath: " << metadata.sourcePath << "\n";
        std::cout << "    loadTime: " << metadata.loadTime << "\n";
        std::cout << "    isDirty: " << (metadata.isDirty ? "true" : "false") << "\n";
    }
}

void demoHotReload(IConfigSystem& config) {
    printSectionHeader("8. HOT RELOAD SYSTEM");

    printSubsection("Checking Hot Reload Status");
    bool initialState = config.isHotReloadEnabled();
    std::cout << "  Initial hot reload state: "
              << (initialState ? "ENABLED" : "DISABLED") << "\n";

    printSubsection("Enabling Hot Reload");
    config.enableHotReload(true);
    std::cout << "  Hot reload state after enable: "
              << (config.isHotReloadEnabled() ? "ENABLED" : "DISABLED") << "\n";

    printSubsection("Disabling Hot Reload");
    config.enableHotReload(false);
    std::cout << "  Hot reload state after disable: "
              << (config.isHotReloadEnabled() ? "ENABLED" : "DISABLED") << "\n";

    printSubsection("Re-enabling for Demo");
    config.enableHotReload(true);
    std::cout << "  Hot reload re-enabled for demonstration\n";

    std::cout << "\nNote: Hot reload checks happen in update() calls\n";
    std::cout << "      Modify a .lua file and call update() to trigger reload\n";
}

void demoChangeNotifications(IConfigSystem& config) {
    printSectionHeader("9. CHANGE NOTIFICATIONS");

    printSubsection("Subscribe to All Config Changes");

    int globalChangeCount = 0;
    SubscriptionId globalSub = config.onConfigChanged([&](const ConfigKey& key) {
        globalChangeCount++;
        std::cout << "  [GLOBAL] Config changed: " << key << "\n";
    });
    std::cout << "  Subscribed with ID: " << globalSub << "\n";

    printSubsection("Subscribe to Specific Key Prefix");

    int windowChangeCount = 0;
    SubscriptionId windowSub = config.onKeyChanged("window.", [&](const ConfigKey& key) {
        windowChangeCount++;
        std::cout << "  [WINDOW] Window config changed: " << key << "\n";
    });
    std::cout << "  Subscribed to \"window.\" prefix with ID: " << windowSub << "\n";

    int statsChangeCount = 0;
    SubscriptionId statsSub = config.onKeyChanged("stats.", [&](const ConfigKey& key) {
        statsChangeCount++;
        std::cout << "  [STATS] Stats changed: " << key << "\n";
    });
    std::cout << "  Subscribed to \"stats.\" prefix with ID: " << statsSub << "\n";

    printSubsection("Triggering Changes to Test Callbacks");

    std::cout << "\nChanging window.width (should trigger GLOBAL and WINDOW callbacks):\n";
    config.setInt("window.width", 1920);

    std::cout << "\nChanging stats.health (should trigger GLOBAL and STATS callbacks):\n";
    config.setInt("stats.health", 50);

    std::cout << "\nChanging audio.masterVolume (should trigger only GLOBAL callback):\n";
    config.setFloat("audio.masterVolume", 0.75f);

    printSubsection("Callback Statistics");
    std::cout << "  Global callback invoked: " << globalChangeCount << " times\n";
    std::cout << "  Window callback invoked: " << windowChangeCount << " times\n";
    std::cout << "  Stats callback invoked: " << statsChangeCount << " times\n";

    printSubsection("Unsubscribing from Notifications");
    config.unsubscribe(windowSub);
    std::cout << "  Unsubscribed from window changes\n";

    std::cout << "\nChanging window.height (should only trigger GLOBAL, not WINDOW):\n";
    int beforeUnsubscribe = windowChangeCount;
    config.setInt("window.height", 1080);
    std::cout << "  Window callback count unchanged: " << beforeUnsubscribe
              << " -> " << windowChangeCount << "\n";

    printSubsection("Cleanup Remaining Subscriptions");
    config.unsubscribe(globalSub);
    config.unsubscribe(statsSub);
    std::cout << "  All subscriptions removed\n";
}

void demoReloading(IConfigSystem& config) {
    printSectionHeader("10. CONFIGURATION RELOADING");

    printSubsection("Current Value Before Reload");
    int healthBefore = config.getIntOr("stats.health", 0);
    std::cout << "  stats.health = " << healthBefore << "\n";

    printSubsection("Reloading Specific Config");
    std::cout << "  Reloading player.lua...\n";
    bool reloadSuccess = config.reloadConfig("data/player.lua");
    std::cout << "  Result: " << (reloadSuccess ? "SUCCESS" : "FAILED") << "\n";

    int healthAfter = config.getIntOr("stats.health", 0);
    std::cout << "  stats.health after reload = " << healthAfter << "\n";
    std::cout << "  (Value reset to original from file)\n";

    printSubsection("Reloading All Configs");
    std::cout << "  Reloading all loaded configurations...\n";
    bool reloadAllSuccess = config.reloadAll();
    std::cout << "  Result: " << (reloadAllSuccess ? "SUCCESS" : "FAILED") << "\n";

    auto loadedConfigs = config.getLoadedConfigs();
    std::cout << "  Reloaded " << loadedConfigs.size() << " config files\n";
}

void demoUpdateCycle(IConfigSystem& config) {
    printSectionHeader("11. UPDATE CYCLE (Hot Reload Check)");

    std::cout << "Simulating game loop update calls...\n";
    std::cout << "Note: Hot reload is enabled, so update() checks for file modifications\n\n";

    config.enableHotReload(true);

    for (int i = 0; i < 5; i++) {
        std::cout << "  Frame " << (i + 1) << ": Calling update(0.016) [~60 FPS]\n";
        config.update(0.016f);  // Simulate 16ms frame time
    }

    std::cout << "\nIf any config files were modified, they would be reloaded automatically\n";
}

void demoNestedAccess(IConfigSystem& config) {
    printSectionHeader("12. NESTED TABLE ACCESS");

    printSubsection("Deep Nesting with Dot Notation");
    std::cout << "  graphics.backgroundColor.r = "
              << config.getIntOr("graphics.backgroundColor.r", 0) << "\n";
    std::cout << "  graphics.backgroundColor.g = "
              << config.getIntOr("graphics.backgroundColor.g", 0) << "\n";
    std::cout << "  graphics.backgroundColor.b = "
              << config.getIntOr("graphics.backgroundColor.b", 0) << "\n";

    printSubsection("Accessing Animation Data");
    std::cout << "  animations.idleFrames = "
              << config.getIntOr("animations.idleFrames", 0) << "\n";
    std::cout << "  animations.runFrames = "
              << config.getIntOr("animations.runFrames", 0) << "\n";
    std::cout << "  animations.attackFPS = "
              << config.getFloatOr("animations.attackFPS", 0.0f) << "\n";

    printSubsection("Movement Physics Nested Values");
    std::cout << "  movement.moveSpeed = "
              << config.getFloatOr("movement.moveSpeed", 0.0f) << "\n";
    std::cout << "  movement.jumpVelocity = "
              << config.getFloatOr("movement.jumpVelocity", 0.0f) << "\n";
    std::cout << "  movement.canWallSlide = "
              << (config.getBoolOr("movement.canWallSlide", false) ? "true" : "false") << "\n";
}

void demoLoadConfigAsset(IConfigSystem& config) {
    printSectionHeader("13. ASSET SYSTEM INTEGRATION");

    printSubsection("loadConfigAsset() - Asset Handle Loading");
    std::cout << "  Attempting to load config via AssetHandle...\n";

    AssetHandle dummyHandle{.uuid = 42, .type = AssetType::Data};  // Placeholder
    bool result = config.loadConfigAsset(dummyHandle);

    std::cout << "  Result: " << (result ? "SUCCESS" : "NOT IMPLEMENTED (expected)") << "\n";
    std::cout << "\n  Note: This method requires integration with the Asset System\n";
    std::cout << "        It will be fully functional once IAssetSystem is implemented\n";
}

//==============================================================================
// Main Entry Point
//==============================================================================

int main() {
    std::println("Bestow Config System Demo");
    std::println("Comprehensive API coverage test\n");

    // Create config system instance
    ConfigSystem config;

    try {
        // Run all demos in sequence
        demoLifecycle(config);
        demoLoadingConfigs(config);
        demoSimpleValueAccess(config);
        demoDefaultValues(config);
        demoArrayAccess(config);
        demoRuntimeModification(config);
        demoStateQueries(config);
        demoHotReload(config);
        demoChangeNotifications(config);
        demoReloading(config);
        demoUpdateCycle(config);
        demoNestedAccess(config);
        demoLoadConfigAsset(config);

        // Summary
        printSectionHeader("DEMO COMPLETE");
        std::cout << "All IConfigSystem API methods have been exercised:\n\n";

        std::cout << "Lifecycle:\n";
        std::cout << "  [X] initialize()\n";
        std::cout << "  [X] update()\n";
        std::cout << "  [X] shutdown() (see below)\n\n";

        std::cout << "Loading:\n";
        std::cout << "  [X] loadConfig()\n";
        std::cout << "  [X] loadConfigAsset()\n";
        std::cout << "  [X] reloadAll()\n";
        std::cout << "  [X] reloadConfig()\n\n";

        std::cout << "Type-Safe Access:\n";
        std::cout << "  [X] getFloat()\n";
        std::cout << "  [X] getInt()\n";
        std::cout << "  [X] getBool()\n";
        std::cout << "  [X] getString()\n";
        std::cout << "  [X] getFloatOr()\n";
        std::cout << "  [X] getIntOr()\n";
        std::cout << "  [X] getBoolOr()\n";
        std::cout << "  [X] getStringOr()\n\n";

        std::cout << "Arrays:\n";
        std::cout << "  [X] getIntArray()\n";
        std::cout << "  [X] getFloatArray()\n";
        std::cout << "  [X] getStringArray()\n\n";

        std::cout << "Runtime Modification:\n";
        std::cout << "  [X] setFloat()\n";
        std::cout << "  [X] setInt()\n";
        std::cout << "  [X] setBool()\n";
        std::cout << "  [X] setString()\n\n";

        std::cout << "State Queries:\n";
        std::cout << "  [X] hasKey()\n";
        std::cout << "  [X] getKeysWithPrefix()\n";
        std::cout << "  [X] getLoadedConfigs()\n";
        std::cout << "  [X] getMetadata()\n\n";

        std::cout << "Hot Reload:\n";
        std::cout << "  [X] enableHotReload()\n";
        std::cout << "  [X] isHotReloadEnabled()\n\n";

        std::cout << "Change Notifications:\n";
        std::cout << "  [X] onConfigChanged()\n";
        std::cout << "  [X] onKeyChanged()\n";
        std::cout << "  [X] unsubscribe()\n\n";

        // Cleanup
        printSectionHeader("CLEANUP");
        std::cout << "Shutting down config system...\n";
        config.shutdown();
        std::cout << "Demo completed successfully!\n";

        return 0;

    } catch (const std::exception& e) {
        std::println("ERROR: Exception during demo: {}", e.what());
        config.shutdown();
        return 1;
    }
}

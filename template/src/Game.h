// template/src/Game.h
// Bestow Template Game - Main game class header
// Demonstrates integration of all Bestow systems

#pragma once

import std;
import bestow;
import bestow.core;
import bestow.gas;
import bestow.camera;
import bestow.blueprints;
import bestow.config.impl;

namespace template_game {

// Simple tag component to identify the player entity
// Note: Must have at least one member for EnTT compatibility
struct PlayerTag {
    int _dummy = 0;  // EnTT requires non-empty components
};

// Simple tag component to identify platform entities
struct PlatformTag {
    int _dummy = 0;  // EnTT requires non-empty components
};

class Game : public bestow::core::Application {
public:
    Game() = default;
    ~Game() override = default;

    // Application lifecycle callbacks
    bool initialize(bestow::core::Engine& engine) override;
    void updateFixed(bestow::DeltaTime dt) override;
    void render(float alpha) override;
    void shutdown() override;

private:
    // System setup functions
    void setupConfig();
    void setupInput();
    void setupBlueprints();
    void setupGAS();
    void setupAudio();
    void setupCamera();

    // Entity creation
    void createPlayer();
    void loadLevel();

    // Game logic
    void handlePlayerInput(bestow::DeltaTime dt);
    void updatePlayerMovement(bestow::DeltaTime dt);
    void updateCamera(bestow::DeltaTime dt);

    // UI rendering
    void renderUI();

    // Audio helpers
    void playSound(const std::string& soundName);
    void playMusic(const std::string& musicName);

    // Engine reference (provided by Bestow)
    bestow::core::Engine* engine_ = nullptr;

    // Additional systems (created separately from engine)
    std::unique_ptr<bestow::IConfigSystem> config_;           // Config loading
    std::unique_ptr<bestow::IGASSystem> gas_;                 // Gameplay Ability System
    std::unique_ptr<bestow::IBlueprintFactory> blueprints_;   // Entity factory
    std::unique_ptr<bestow::ICameraSystem> camera_;           // Camera control

    // Game entities
    bestow::Entity player_;

    // Level tracking
    bestow::LevelId currentLevelId_{};
    bestow::AssetHandle levelAssetHandle_;

    // Player configuration (loaded from Lua)
    float playerMoveSpeed_ = 200.0f;
    float playerJumpForce_ = 400.0f;
    float playerPhysicsWidth_ = 30.0f;
    float playerPhysicsHeight_ = 50.0f;

    // GAS attribute and ability IDs
    bestow::AttributeId healthAttr_ = 0;
    bestow::AttributeId staminaAttr_ = 0;
    bestow::AbilityId jumpAbility_ = 0;

    // Player state
    bool isGrounded_ = false;

    // Audio
    std::unordered_map<std::string, bestow::AssetHandle> soundAssets_;
    bool audioEnabled_ = true;
    static constexpr bestow::Channel MUSIC_CHANNEL = 0;
    static constexpr bestow::Channel SFX_CHANNEL = 1;
};

}  // namespace template_game

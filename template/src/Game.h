// template/src/Game.h
// JFrame Template Game - Main game class header
// Demonstrates integration of all JFrame systems

#pragma once

import std;
import jframe;
import jframe.core;
import jframe.gas;
import jframe.camera;
import jframe.blueprints;
import jframe.config.impl;

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

class Game : public jframe::core::Application {
public:
    Game() = default;
    ~Game() override = default;

    // Application lifecycle callbacks
    bool initialize(jframe::core::Engine& engine) override;
    void updateFixed(jframe::DeltaTime dt) override;
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
    void handlePlayerInput(jframe::DeltaTime dt);
    void updatePlayerMovement(jframe::DeltaTime dt);
    void updateCamera(jframe::DeltaTime dt);

    // UI rendering
    void renderUI();

    // Audio helpers
    void playSound(const std::string& soundName);
    void playMusic(const std::string& musicName);

    // Engine reference (provided by JFrame)
    jframe::core::Engine* engine_ = nullptr;

    // Additional systems (created separately from engine)
    std::unique_ptr<jframe::IConfigSystem> config_;           // Config loading
    std::unique_ptr<jframe::IGASSystem> gas_;                 // Gameplay Ability System
    std::unique_ptr<jframe::IBlueprintFactory> blueprints_;   // Entity factory
    std::unique_ptr<jframe::ICameraSystem> camera_;           // Camera control

    // Game entities
    jframe::Entity player_;

    // Level tracking
    jframe::LevelId currentLevelId_{};
    jframe::AssetHandle levelAssetHandle_;

    // Player configuration (loaded from Lua)
    float playerMoveSpeed_ = 200.0f;
    float playerJumpForce_ = 400.0f;
    float playerPhysicsWidth_ = 30.0f;
    float playerPhysicsHeight_ = 50.0f;

    // GAS attribute and ability IDs
    jframe::AttributeId healthAttr_ = 0;
    jframe::AttributeId staminaAttr_ = 0;
    jframe::AbilityId jumpAbility_ = 0;

    // Player state
    bool isGrounded_ = false;

    // Audio
    std::unordered_map<std::string, jframe::AssetHandle> soundAssets_;
    bool audioEnabled_ = true;
    static constexpr jframe::Channel MUSIC_CHANNEL = 0;
    static constexpr jframe::Channel SFX_CHANNEL = 1;
};

}  // namespace template_game

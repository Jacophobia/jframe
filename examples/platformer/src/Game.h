// examples/platformer/src/Game.h
// Platformer game class declaration

#pragma once

import jframe;
import jframe.core;

#if defined(JFRAME_DEV_TOOLS)
import jframe.dev;
#endif

#include "Components.h"

namespace platformer {

class Game : public jframe::core::Application {
public:
    Game() = default;
    ~Game() override = default;

    // Application lifecycle
    bool initialize(jframe::core::Engine& engine) override;
    void updateFixed(jframe::DeltaTime dt) override;
    void render(float alpha) override;
    void shutdown() override;

private:
    void setupInputMappings();
    void loadLevel();
    void createPlayer();
    void handlePlayerInput(jframe::DeltaTime dt);
    void updatePlayerMovement(jframe::DeltaTime dt);
    void updateCamera(jframe::DeltaTime dt);

    // New system integration methods
    void spawnEnemies(jframe::LevelId levelId);
    void spawnCollectibles(jframe::LevelId levelId);
    void updateEnemyAI(jframe::DeltaTime dt);
    void checkCollectiblePickup();
    void handleCollisionEvent(const jframe::EventData& data);
    void saveGame();
    void loadGame();

    jframe::core::Engine* engine_ = nullptr;
    jframe::Entity player_;
    jframe::Entity camera_;

    // Level and Asset handles
    jframe::AssetHandle levelAsset_;
    jframe::LevelId currentLevel_;

    // Enemy entities (AI System)
    std::vector<jframe::Entity> enemies_;

    // Collectible entities
    std::vector<jframe::Entity> collectibles_;

    // Audio handles (stub-safe)
    jframe::AssetHandle jumpSoundAsset_;
    jframe::AssetHandle coinSoundAsset_;
    jframe::AssetHandle hurtSoundAsset_;
    jframe::AssetHandle musicAsset_;

    // Event subscriptions
    jframe::SubscriptionId collisionSubscription_;

    // Game state
    GameState gameState_;

#if defined(JFRAME_DEV_TOOLS)
    jframe::dev::HotReloadManager hotReload_;
    jframe::dev::DevOverlay devOverlay_;
#endif
};

}  // namespace platformer

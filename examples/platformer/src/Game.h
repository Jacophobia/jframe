// examples/platformer/src/Game.h
// Platformer game class declaration

#pragma once

import bestow;
import bestow.core;

#if defined(BESTOW_DEV_TOOLS)
import bestow.dev;
#endif

#include "Components.h"

namespace platformer {

class Game : public bestow::core::Application {
public:
    Game() = default;
    ~Game() override = default;

    // Application lifecycle
    bool initialize(bestow::core::Engine& engine) override;
    void updateFixed(bestow::DeltaTime dt) override;
    void render(float alpha) override;
    void shutdown() override;

private:
    void setupInputMappings();
    void loadLevel();
    void createPlayer();
    void handlePlayerInput(bestow::DeltaTime dt);
    void updatePlayerMovement(bestow::DeltaTime dt);
    void updateCamera(bestow::DeltaTime dt);

    // New system integration methods
    void spawnEnemies(bestow::LevelId levelId);
    void spawnCollectibles(bestow::LevelId levelId);
    void updateEnemyAI(bestow::DeltaTime dt);
    void checkCollectiblePickup();
    void handleCollisionEvent(const bestow::EventData& data);
    void saveGame();
    void loadGame();

    bestow::core::Engine* engine_ = nullptr;
    bestow::Entity player_;
    bestow::Entity camera_;

    // Level and Asset handles
    bestow::AssetHandle levelAsset_;
    bestow::LevelId currentLevel_;

    // Enemy entities (AI System)
    std::vector<bestow::Entity> enemies_;

    // Collectible entities
    std::vector<bestow::Entity> collectibles_;

    // Audio handles (stub-safe)
    bestow::AssetHandle jumpSoundAsset_;
    bestow::AssetHandle coinSoundAsset_;
    bestow::AssetHandle hurtSoundAsset_;
    bestow::AssetHandle musicAsset_;

    // Event subscriptions
    bestow::SubscriptionId collisionSubscription_;

    // Game state
    GameState gameState_;

#if defined(BESTOW_DEV_TOOLS)
    bestow::dev::HotReloadManager hotReload_;
    bestow::dev::DevOverlay devOverlay_;
#endif
};

}  // namespace platformer

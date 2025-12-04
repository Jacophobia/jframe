// examples/endless-runner/src/Game.cpp
// Endless runner demo - auto-run, procedural generation, despawn

import std;
import bestow;
import bestow.core;
import bestow.camera.impl;
import bestow.components;
import bestow.config.impl;

#if defined(BESTOW_DEV_TOOLS)
import bestow.dev;
#endif

#include "Game.h"

namespace endless_runner {

bool Game::initialize(bestow::core::Engine& engine) {
    engine_ = &engine;
    auto& sys = engine.systems();
    bestow::core::logInfo("Initializing endless runner");

    config_ = bestow::createConfigSystem();
    config_->initialize();
    if (!config_->loadConfig("data/config/game.lua")) {
        bestow::core::logError("Failed to load game config");
    }

    sys.physics->setGravity({0.0f, config_->getFloatOr("physics.gravity", 980.0f)});

    auto windowSize = sys.graphics->getWindowSize();
    cameraSystem_ = std::make_unique<bestow::CameraSystem>(windowSize);
    cameraSystem_->setZoom(config_->getFloatOr("camera.zoom", 1.0f));
    cameraSystem_->setFollowSmoothing(config_->getFloatOr("camera.followSmoothing", 5.0f));

    runSpeed_ = config_->getFloatOr("game.runSpeed", 300.0f);
    sectionWidth_ = config_->getFloatOr("game.sectionWidth", 800.0f);
    jumpForce_ = config_->getFloatOr("player.jumpForce", 700.0f);
    groundY_ = config_->getFloatOr("game.groundY", 550.0f);
    playerBodyWidth_ = config_->getFloatOr("physics.player.width", 50.0f);
    playerBodyHeight_ = config_->getFloatOr("physics.player.height", 80.0f);

    // Load assets
    playerTextureHandle_ = sys.assets->registerAsset(bestow::AssetType::Texture, "data/textures/player_spritesheet.png");
    coinTextureHandle_ = sys.assets->registerAsset(bestow::AssetType::Texture, "data/textures/coinGold.png");
    enemyTextureHandle_ = sys.assets->registerAsset(bestow::AssetType::Texture, "data/textures/enemyWalking_1.png");
    platformTextureHandle_ = sys.assets->registerAsset(bestow::AssetType::Texture, "data/textures/block.png");
    fontHandle_ = sys.assets->registerAsset(bestow::AssetType::Font, "data/fonts/PressStart2P-Regular.ttf");
    jumpSoundHandle_ = sys.assets->registerAsset(bestow::AssetType::Sound, "data/audio/sfx/phaseJump1.ogg");
    coinSoundHandle_ = sys.assets->registerAsset(bestow::AssetType::Sound, "data/audio/sfx/pepSound1.ogg");
    hurtSoundHandle_ = sys.assets->registerAsset(bestow::AssetType::Sound, "data/audio/sfx/impactBell_heavy_000.ogg");
    levelAssetHandle_ = sys.assets->registerAsset(bestow::AssetType::Level, "data/levels/endless.lua");

    sys.assets->loadAsset(playerTextureHandle_);
    sys.assets->loadAsset(coinTextureHandle_);
    sys.assets->loadAsset(enemyTextureHandle_);
    sys.assets->loadAsset(platformTextureHandle_);
    sys.assets->loadAsset(fontHandle_);
    sys.assets->loadAsset(jumpSoundHandle_);
    sys.assets->loadAsset(coinSoundHandle_);
    sys.assets->loadAsset(hurtSoundHandle_);
    sys.assets->loadAsset(levelAssetHandle_);

    auto levelResult = sys.levels->loadLevel(levelAssetHandle_);
    if (levelResult.has_value()) {
        currentLevelId_ = levelResult.value();
        sys.levels->setActiveLevel(currentLevelId_);
    }

    // Setup sprite sheets
    playerSheet_ = bestow::SpriteSheet{
        .texture = playerTextureHandle_,
        .frameWidth = 66, .frameHeight = 92,
        .columns = 9, .rows = 7
    };
    playerSprite_.sheet = playerSheet_;
    playerSprite_.playing = true;

    bestow::Animation runAnim;
    runAnim.name = "run";
    runAnim.looping = true;
    runAnim.frames = {{9, 0.08f}, {10, 0.08f}, {11, 0.08f}, {12, 0.08f}, {13, 0.08f}, {14, 0.08f}};
    playerSprite_.animations["run"] = runAnim;

    bestow::Animation jumpAnim;
    jumpAnim.name = "jump";
    jumpAnim.looping = false;
    jumpAnim.frames = {{18, 0.1f}};
    playerSprite_.animations["jump"] = jumpAnim;

    playerSprite_.play("run");

    coinSheet_ = bestow::SpriteSheet{.texture = coinTextureHandle_, .frameWidth = 128, .frameHeight = 128, .columns = 1, .rows = 1};
    enemySheet_ = bestow::SpriteSheet{.texture = enemyTextureHandle_, .frameWidth = 32, .frameHeight = 44, .columns = 1, .rows = 1};
    platformSheet_ = bestow::SpriteSheet{.texture = platformTextureHandle_, .frameWidth = 226, .frameHeight = 148, .columns = 1, .rows = 1};

    sys.input->registerMapping(bestow::InputMapping{
        .binding = bestow::InputBinding{.deviceType = bestow::InputDeviceType::Keyboard, .keyCode = 32},
        .action = "jump"
    });
    sys.input->registerMapping(bestow::InputMapping{
        .binding = bestow::InputBinding{.deviceType = bestow::InputDeviceType::Keyboard, .keyCode = 265},
        .action = "jump"
    });

    triggerSubscription_ = sys.events->subscribe(bestow::Events::TriggerEnter,
        [this](const bestow::EventData& data) { onTriggerEnter(data); });
    collisionSubscription_ = sys.events->subscribe(bestow::Events::Collision,
        [this](const bestow::EventData& data) { onCollision(data); });

    setupPlayer();
    loadInitialSection();

    bestow::core::logInfo("Endless runner initialized");
    return true;
}

void Game::setupPlayer() {
    auto& sys = engine_->systems();
    player_ = sys.entities->createEntity();

    float spawnX = 200.0f;
    float spawnY = groundY_ - 100.0f;

    sys.entities->emplace<bestow::Transform2D>(player_, bestow::Transform2D{
        .x = spawnX, .y = spawnY,
        .scaleX = playerBodyWidth_ / static_cast<float>(playerSheet_.frameWidth),
        .scaleY = playerBodyHeight_ / static_cast<float>(playerSheet_.frameHeight)
    });

    sys.entities->emplace<bestow::components::Health>(player_, bestow::components::Health{.current = 100, .maximum = 100});
    sys.entities->emplace<bestow::components::Score>(player_, bestow::components::Score{.value = 0});
    sys.entities->emplace<bestow::components::GroundDetector>(player_);
    sys.entities->emplace<bestow::components::JumpState>(player_, bestow::components::JumpState{
        .jumpsRemaining = 1, .maxJumps = 1, .jumpForce = jumpForce_
    });
    sys.entities->getRegistry().emplace<bestow::components::PlayerTag>(player_);

    bestow::PhysicsBodyDef bodyDef{
        .type = bestow::BodyType::Dynamic,
        .transform = {.x = spawnX, .y = spawnY},
        .size = {playerBodyWidth_, playerBodyHeight_},
        .fixedRotation = true,
        .density = 1.0f, .friction = 0.0f, .restitution = 0.0f
    };
    sys.physics->createBody(player_, bodyDef);
    cameraSystem_->setTarget(player_);
}

void Game::loadInitialSection() {
    auto& sys = engine_->systems();

    // Create initial ground platform
    createPlatform(400.0f, groundY_, 1600.0f, 50.0f);
    nextSectionX_ = 800.0f;

    // Load entities from Lua (uses procedural generator function)
    auto entityDefs = sys.levels->getEntityDefs(currentLevelId_);
    for (const auto& def : entityDefs) {
        if (def.type == "platform") {
            float width = 150.0f, height = 30.0f;
            if (auto it = def.properties.find("width"); it != def.properties.end())
                width = std::any_cast<double>(it->second);
            if (auto it = def.properties.find("height"); it != def.properties.end())
                height = std::any_cast<double>(it->second);
            createPlatform(def.transform.x, def.transform.y, width, height);
        } else if (def.type == "coin") {
            int value = 10;
            if (auto it = def.properties.find("value"); it != def.properties.end())
                value = static_cast<int>(std::any_cast<double>(it->second));
            createCoin(def.transform.x, def.transform.y, value);
        } else if (def.type == "enemy") {
            createEnemy(def.transform.x, def.transform.y);
        }
    }
    sectionsGenerated_ = 1;
}

void Game::restartGame() {
    auto& sys = engine_->systems();
    bestow::core::logInfo("Restarting game...");

    // Collect entities to destroy (can't modify while iterating)
    std::vector<bestow::Entity> toDestroy;

    sys.entities->view<bestow::components::PlatformTag>().each([&toDestroy](auto entity) {
        toDestroy.push_back(entity);
    });

    sys.entities->view<bestow::components::CollectibleTag>().each([&toDestroy](auto entity) {
        toDestroy.push_back(entity);
    });

    sys.entities->view<bestow::components::EnemyTag>().each([&toDestroy](auto entity) {
        toDestroy.push_back(entity);
    });

    toDestroy.push_back(player_);

    // Destroy all collected entities
    for (auto entity : toDestroy) {
        sys.entities->destroyEntity(entity);
    }

    // Reset state
    score_ = 0;
    health_ = 100;
    gameOver_ = false;
    distance_ = 0.0f;
    nextSectionX_ = 0.0f;
    sectionsGenerated_ = 0;

    // Recreate player and initial section
    setupPlayer();
    loadInitialSection();

    playerSprite_.play("run");
    bestow::core::logInfo("Game restarted!");
}

void Game::generateNextSection() {
    auto& sys = engine_->systems();

    float baseX = nextSectionX_;

    // Ground extension
    createPlatform(baseX + sectionWidth_ / 2.0f, groundY_, sectionWidth_, 50.0f);

    // Random platforms using simple rand() - Lua generator shows pattern
    int numPlatforms = 1 + (std::rand() % 3);
    for (int i = 0; i < numPlatforms; ++i) {
        float px = baseX + 100.0f + static_cast<float>(std::rand() % static_cast<int>(sectionWidth_ - 200.0f));
        float py = groundY_ - 80.0f - static_cast<float>(std::rand() % 200);
        float pw = 100.0f + static_cast<float>(std::rand() % 100);
        createPlatform(px, py, pw, 30.0f);
    }

    // Random coins
    int numCoins = 2 + (std::rand() % 4);
    for (int i = 0; i < numCoins; ++i) {
        float cx = baseX + 50.0f + static_cast<float>(std::rand() % static_cast<int>(sectionWidth_ - 100.0f));
        float cy = groundY_ - 100.0f - static_cast<float>(std::rand() % 250);
        createCoin(cx, cy, 10);
    }

    // Random enemies (harder as you progress)
    int maxEnemies = std::min(1 + sectionsGenerated_ / 3, 3);
    int numEnemies = std::rand() % (maxEnemies + 1);
    for (int i = 0; i < numEnemies; ++i) {
        float ex = baseX + 200.0f + static_cast<float>(std::rand() % static_cast<int>(sectionWidth_ - 300.0f));
        createEnemy(ex, groundY_ - 50.0f);
    }

    nextSectionX_ += sectionWidth_;
    sectionsGenerated_++;
    bestow::core::logInfo("Generated section " + std::to_string(sectionsGenerated_));
}

void Game::despawnBehindCamera() {
    auto& sys = engine_->systems();
    auto camera = cameraSystem_->getCamera();
    float despawnThreshold = camera.transform.x - camera.viewportSize.width;

    // Despawn platforms behind camera
    auto platformView = sys.entities->view<bestow::components::PlatformTag, bestow::Transform2D>();
    std::vector<bestow::Entity> toDestroy;
    for (auto entity : platformView) {
        auto& transform = platformView.get<bestow::Transform2D>(entity);
        if (transform.x < despawnThreshold) {
            toDestroy.push_back(entity);
        }
    }

    // Despawn coins
    auto coinView = sys.entities->view<bestow::components::CollectibleTag, bestow::Transform2D>();
    for (auto entity : coinView) {
        auto& transform = coinView.get<bestow::Transform2D>(entity);
        if (transform.x < despawnThreshold) {
            toDestroy.push_back(entity);
        }
    }

    // Despawn enemies
    auto enemyView = sys.entities->view<bestow::components::EnemyTag, bestow::Transform2D>();
    for (auto entity : enemyView) {
        auto& transform = enemyView.get<bestow::Transform2D>(entity);
        if (transform.x < despawnThreshold) {
            toDestroy.push_back(entity);
        }
    }

    for (auto entity : toDestroy) {
        sys.entities->destroyEntity(entity);
    }
}

void Game::createPlatform(float x, float y, float width, float height) {
    auto& sys = engine_->systems();
    auto platform = sys.entities->createEntity();

    sys.entities->emplace<bestow::Transform2D>(platform, bestow::Transform2D{
        .x = x, .y = y,
        .scaleX = width / static_cast<float>(platformSheet_.frameWidth),
        .scaleY = height / static_cast<float>(platformSheet_.frameHeight)
    });
    sys.entities->getRegistry().emplace<bestow::components::PlatformTag>(platform);

    bestow::PhysicsBodyDef bodyDef{
        .type = bestow::BodyType::Static,
        .transform = {.x = x, .y = y},
        .size = {width, height},
        .density = 0.0f, .friction = 0.5f
    };
    sys.physics->createBody(platform, bodyDef);
}

void Game::createCoin(float x, float y, int value) {
    auto& sys = engine_->systems();
    auto coin = sys.entities->createEntity();

    float size = 30.0f;
    sys.entities->emplace<bestow::Transform2D>(coin, bestow::Transform2D{
        .x = x, .y = y,
        .scaleX = size / static_cast<float>(coinSheet_.frameWidth),
        .scaleY = size / static_cast<float>(coinSheet_.frameHeight)
    });
    sys.entities->getRegistry().emplace<bestow::components::CollectibleTag>(coin);
    sys.entities->emplace<bestow::components::Score>(coin, bestow::components::Score{.value = value});

    bestow::PhysicsBodyDef bodyDef{
        .type = bestow::BodyType::Static,
        .transform = {.x = x, .y = y},
        .size = {size, size},
        .density = 0.0f, .isSensor = true
    };
    sys.physics->createBody(coin, bodyDef);
}

void Game::createEnemy(float x, float y) {
    auto& sys = engine_->systems();
    auto enemy = sys.entities->createEntity();

    float size = 40.0f;
    sys.entities->emplace<bestow::Transform2D>(enemy, bestow::Transform2D{
        .x = x, .y = y,
        .scaleX = size / static_cast<float>(enemySheet_.frameWidth),
        .scaleY = size / static_cast<float>(enemySheet_.frameHeight)
    });
    sys.entities->getRegistry().emplace<bestow::components::EnemyTag>(enemy);
    sys.entities->emplace<bestow::components::Damage>(enemy, bestow::components::Damage{.amount = 50});

    bestow::PhysicsBodyDef bodyDef{
        .type = bestow::BodyType::Dynamic,
        .transform = {.x = x, .y = y},
        .size = {size, size},
        .fixedRotation = true,
        .density = 1.0f, .friction = 0.3f
    };
    sys.physics->createBody(enemy, bodyDef);
}

void Game::updateFixed(bestow::DeltaTime dt) {
    auto& sys = engine_->systems();

    if (gameOver_) {
        // Check for restart
        if (sys.input->wasActionJustPressed("jump")) {
            restartGame();
        }
        return;
    }

    handlePlayerInput(dt);
    syncPhysicsToTransforms();
    updateCamera(dt);

    // Update distance/score based on player position
    auto playerPos = sys.physics->getPosition(player_);
    distance_ = std::max(distance_, playerPos.x);
    score_ = static_cast<int>(distance_ / 10.0f);

    // Generate new sections as player progresses
    if (playerPos.x > nextSectionX_ - sectionWidth_) {
        generateNextSection();
    }

    // Despawn entities behind camera
    despawnBehindCamera();

    // Check death (fall off screen)
    if (playerPos.y > groundY_ + 200.0f) {
        gameOver_ = true;
        bestow::core::logInfo("Game Over - fell off!");
    }

    // Update health
    if (auto* health = sys.entities->tryGet<bestow::components::Health>(player_)) {
        health->update(dt);
        health_ = health->current;
        if (health->isDead() && !gameOver_) {
            gameOver_ = true;
            bestow::core::logInfo("Game Over!");
        }
    }

    playerSprite_.update(dt);
}

void Game::handlePlayerInput(bestow::DeltaTime dt) {
    auto& sys = engine_->systems();
    auto velocity = sys.physics->getVelocity(player_);

    // Check if stuck (velocity.x is too low) - but only after traveling some distance
    // to give physics time to apply initial velocity
    if (distance_ > 100.0f && velocity.x < runSpeed_ * 0.3f) {
        gameOver_ = true;
        bestow::core::logInfo("Game Over - stuck!");
        return;
    }

    // Auto-run: constant horizontal speed
    velocity.x = runSpeed_;

    // Simple ground detection based on vertical velocity
    bool isGrounded = std::abs(velocity.y) < 10.0f;

    // Jump handling - only when grounded
    if (sys.input->wasActionJustPressed("jump") && isGrounded) {
        velocity.y = -jumpForce_;
        sys.audio->playPositional(bestow::PositionalSound{.asset = jumpSoundHandle_, .volume = 0.8f});
        playerSprite_.play("jump");
    }

    sys.physics->setVelocity(player_, velocity);

    // Update animation
    if (isGrounded && playerSprite_.currentAnimation != "run") {
        playerSprite_.play("run");
    }
}

void Game::updateCamera(bestow::DeltaTime dt) {
    auto& sys = engine_->systems();
    auto playerTransform = sys.entities->get<bestow::Transform2D>(player_);

    // Camera follows player (same as platformer-demo)
    cameraSystem_->update(dt, playerTransform.position());
    sys.graphics->setCamera(cameraSystem_->getCamera());
}

void Game::syncPhysicsToTransforms() {
    auto& sys = engine_->systems();

    auto playerPos = sys.physics->getPosition(player_);
    auto& playerTransform = sys.entities->get<bestow::Transform2D>(player_);
    playerTransform.x = playerPos.x;
    playerTransform.y = playerPos.y;

    auto enemyView = sys.entities->view<bestow::components::EnemyTag, bestow::Transform2D>();
    for (auto entity : enemyView) {
        auto pos = sys.physics->getPosition(entity);
        auto& transform = enemyView.get<bestow::Transform2D>(entity);
        transform.x = pos.x;
        transform.y = pos.y;
    }
}

void Game::onTriggerEnter(const bestow::EventData& data) {
    auto& sys = engine_->systems();
    auto& triggerData = std::get<bestow::TriggerEvent>(data);

    bool playerInvolved = (triggerData.entityA == player_ || triggerData.entityB == player_);
    if (!playerInvolved) return;

    bestow::Entity other = (triggerData.entityA == player_) ? triggerData.entityB : triggerData.entityA;

    if (sys.entities->anyOf<bestow::components::CollectibleTag>(other)) {
        if (auto* coinScore = sys.entities->tryGet<bestow::components::Score>(other)) {
            score_ += coinScore->value;
            sys.audio->playPositional(bestow::PositionalSound{.asset = coinSoundHandle_, .volume = 0.7f});
            sys.entities->destroyEntity(other);
        }
    }
}

void Game::onCollision(const bestow::EventData& data) {
    auto& sys = engine_->systems();
    auto& collisionData = std::get<bestow::CollisionEvent>(data);

    bool playerInvolved = (collisionData.entityA == player_ || collisionData.entityB == player_);
    if (!playerInvolved) return;

    bestow::Entity other = (collisionData.entityA == player_) ? collisionData.entityB : collisionData.entityA;

    if (sys.entities->anyOf<bestow::components::EnemyTag>(other)) {
        if (auto* health = sys.entities->tryGet<bestow::components::Health>(player_)) {
            if (auto* damage = sys.entities->tryGet<bestow::components::Damage>(other)) {
                health->takeDamage(damage->amount);
                if (!health->isDead()) {
                    sys.audio->playPositional(bestow::PositionalSound{.asset = hurtSoundHandle_, .volume = 1.0f});
                    cameraSystem_->shake(15.0f, 0.3f);
                }
            }
        }
    }
}

void Game::render(float alpha) {
    renderEntities();
    renderHUD();
}

void Game::renderEntities() {
    auto& sys = engine_->systems();

    auto playerTransform = sys.entities->get<bestow::Transform2D>(player_);
    sys.graphics->drawAnimatedSprite(playerSprite_, playerTransform);

    auto platformView = sys.entities->view<bestow::components::PlatformTag, bestow::Transform2D>();
    for (auto entity : platformView) {
        auto& transform = platformView.get<bestow::Transform2D>(entity);
        sys.graphics->drawSprite(platformSheet_, 0, transform);
    }

    auto coinView = sys.entities->view<bestow::components::CollectibleTag, bestow::Transform2D>();
    for (auto entity : coinView) {
        auto& transform = coinView.get<bestow::Transform2D>(entity);
        sys.graphics->drawSprite(coinSheet_, 0, transform);
    }

    auto enemyView = sys.entities->view<bestow::components::EnemyTag, bestow::Transform2D>();
    for (auto entity : enemyView) {
        auto& transform = enemyView.get<bestow::Transform2D>(entity);
        sys.graphics->drawSprite(enemySheet_, 0, transform);
    }
}

void Game::renderHUD() {
    auto& sys = engine_->systems();
    auto camera = cameraSystem_->getCamera();
    float screenLeft = camera.transform.x - camera.viewportSize.width / 2.0f;
    float screenTop = camera.transform.y - camera.viewportSize.height / 2.0f;

    // Health bar
    float healthPercent = static_cast<float>(health_) / 100.0f;
    bestow::Canvas healthBg{{static_cast<int>(screenLeft + 10), static_cast<int>(screenTop + 10)}, {200, 20}};
    bestow::Canvas healthFill{{static_cast<int>(screenLeft + 10), static_cast<int>(screenTop + 10)}, {static_cast<int>(200 * healthPercent), 20}};
    sys.graphics->drawRect(healthBg, {50, 50, 50, 255}, true);
    sys.graphics->drawRect(healthFill, {0, 255, 0, 255}, true);

    // Score/Distance
    std::string scoreText = "SCORE: " + std::to_string(score_);
    bestow::Vec2 scorePos{screenLeft + 620.0f, screenTop + 26.0f};
    sys.graphics->drawText(scoreText, scorePos, fontHandle_, 16.0f, {255, 215, 0, 255});

    // Distance
    std::string distText = "DIST: " + std::to_string(static_cast<int>(distance_)) + "m";
    bestow::Vec2 distPos{screenLeft + 10.0f, screenTop + 56.0f};
    sys.graphics->drawText(distText, distPos, fontHandle_, 12.0f, {255, 255, 255, 255});

    if (gameOver_) {
        bestow::Vec2 gameOverPos{camera.transform.x, camera.transform.y};
        sys.graphics->drawTextCentered("GAME OVER", gameOverPos, fontHandle_, 32.0f, {200, 0, 0, 255});

        std::string finalScore = "Final Score: " + std::to_string(score_);
        bestow::Vec2 finalPos{camera.transform.x, camera.transform.y + 50.0f};
        sys.graphics->drawTextCentered(finalScore, finalPos, fontHandle_, 16.0f, {255, 255, 255, 255});

        bestow::Vec2 restartPos{camera.transform.x, camera.transform.y + 90.0f};
        sys.graphics->drawTextCentered("Press SPACE to restart", restartPos, fontHandle_, 12.0f, {180, 180, 180, 255});
    }
}

void Game::shutdown() {
    bestow::core::logInfo("Shutting down endless runner");
    auto& sys = engine_->systems();
    sys.events->unsubscribe(triggerSubscription_);
    sys.events->unsubscribe(collisionSubscription_);
    if (config_) config_->shutdown();
}

}  // namespace endless_runner

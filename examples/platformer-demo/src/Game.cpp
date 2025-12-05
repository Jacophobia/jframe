// examples/platformer-demo/src/Game.cpp
// Complete platformer demo showcasing Bestow features

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

namespace platformer_demo {

//==============================================================================
// Initialization
//==============================================================================

bool Game::initialize(bestow::core::Engine& engine) {
    engine_ = &engine;
    auto& sys = engine.systems();
    bestow::core::logInfo("Initializing platformer demo");

    // Initialize config system
    config_ = bestow::createConfigSystem();
    config_->initialize();
    if (!config_->loadConfig("data/config/game.lua")) {
        bestow::core::logError("Failed to load game config");
    }

    // Set gravity for screen coordinates (positive Y is down)
    sys.physics->setGravity({0.0f, config_->getFloatOr("physics.gravity", 980.0f)});

    // Initialize camera system
    auto windowSize = sys.graphics->getWindowSize();
    cameraSystem_ = std::make_unique<bestow::CameraSystem>(windowSize);
    cameraSystem_->setZoom(config_->getFloatOr("camera.zoom", 1.0f));
    cameraSystem_->setFollowSmoothing(config_->getFloatOr("camera.followSmoothing", 0.0f));
    cameraSystem_->setDeadzone({
        config_->getFloatOr("camera.deadzone.x", 0.0f),
        config_->getFloatOr("camera.deadzone.y", 0.0f)
    });

    // Load gameplay config values
    playerMoveSpeed_ = config_->getFloatOr("player.moveSpeed", 400.0f);
    playerJumpForce_ = config_->getFloatOr("player.jump.jumpForce", 800.0f);
    playerMaxJumps_ = config_->getIntOr("player.jump.maxJumps", 2);
    groundedVelocityThreshold_ = config_->getFloatOr("player.groundDetection.velocityThreshold", 50.0f);
    runningAnimThreshold_ = config_->getFloatOr("player.animation.runningThreshold", 10.0f);
    jumpingAnimThreshold_ = config_->getFloatOr("player.animation.jumpingThreshold", -10.0f);

    // Load audio config
    jumpSoundVolume_ = config_->getFloatOr("audio.sfx.jump.volume", 0.8f);
    coinSoundVolume_ = config_->getFloatOr("audio.sfx.coin.volume", 0.7f);
    hurtSoundVolume_ = config_->getFloatOr("audio.sfx.hurt.volume", 1.0f);

    // Load camera shake config
    cameraShakeMagnitude_ = config_->getFloatOr("camera.shake.magnitude", 10.0f);
    cameraShakeDuration_ = config_->getFloatOr("camera.shake.duration", 0.3f);

    // Load physics config
    playerBodyWidth_ = config_->getFloatOr("physics.player.size.width", 50.0f);
    playerBodyHeight_ = config_->getFloatOr("physics.player.size.height", 80.0f);
    playerDensity_ = config_->getFloatOr("physics.player.density", 1.0f);
    playerFriction_ = config_->getFloatOr("physics.player.friction", 0.0f);
    playerRestitution_ = config_->getFloatOr("physics.player.restitution", 0.0f);
    coinBodySize_ = config_->getFloatOr("physics.coin.size.width", 30.0f);
    enemyBodySize_ = config_->getFloatOr("physics.enemy.size.width", 40.0f);
    enemyDensity_ = config_->getFloatOr("physics.enemy.density", 1.0f);
    enemyFriction_ = config_->getFloatOr("physics.enemy.friction", 0.3f);
    platformFriction_ = config_->getFloatOr("physics.platform.friction", 0.5f);

    // Load enemy config
    enemyDamage_ = config_->getIntOr("enemy.damage", 20);
    enemyPatrolSpeed_ = config_->getFloatOr("enemy.patrolSpeed", 50.0f);

    // Load HUD config
    hudHealthBarX_ = config_->getIntOr("hud.healthBar.position.x", 10);
    hudHealthBarY_ = config_->getIntOr("hud.healthBar.position.y", 10);
    hudHealthBarWidth_ = config_->getIntOr("hud.healthBar.size.width", 200);
    hudHealthBarHeight_ = config_->getIntOr("hud.healthBar.size.height", 20);
    hudHealthBarBgColor_ = bestow::Color{
        static_cast<std::uint8_t>(config_->getIntOr("hud.healthBar.colors.background.r", 50)),
        static_cast<std::uint8_t>(config_->getIntOr("hud.healthBar.colors.background.g", 50)),
        static_cast<std::uint8_t>(config_->getIntOr("hud.healthBar.colors.background.b", 50)),
        static_cast<std::uint8_t>(config_->getIntOr("hud.healthBar.colors.background.a", 255))
    };
    hudHealthBarFillColor_ = bestow::Color{
        static_cast<std::uint8_t>(config_->getIntOr("hud.healthBar.colors.fill.r", 0)),
        static_cast<std::uint8_t>(config_->getIntOr("hud.healthBar.colors.fill.g", 255)),
        static_cast<std::uint8_t>(config_->getIntOr("hud.healthBar.colors.fill.b", 0)),
        static_cast<std::uint8_t>(config_->getIntOr("hud.healthBar.colors.fill.a", 255))
    };
    // Score text config
    hudScoreX_ = config_->getIntOr("hud.score.position.x", 620);
    hudScoreY_ = config_->getIntOr("hud.score.position.y", 10);
    hudScoreFontSize_ = config_->getFloatOr("hud.score.fontSize", 16.0f);
    hudScorePrefix_ = config_->getStringOr("hud.score.prefix", "SCORE: ");
    hudScoreColor_ = bestow::Color{
        static_cast<std::uint8_t>(config_->getIntOr("hud.score.color.r", 255)),
        static_cast<std::uint8_t>(config_->getIntOr("hud.score.color.g", 215)),
        static_cast<std::uint8_t>(config_->getIntOr("hud.score.color.b", 0)),
        static_cast<std::uint8_t>(config_->getIntOr("hud.score.color.a", 255))
    };
    // Game over text config
    hudGameOverText_ = config_->getStringOr("hud.gameOver.text", "GAME OVER");
    hudGameOverFontSize_ = config_->getFloatOr("hud.gameOver.fontSize", 32.0f);
    hudGameOverColor_ = bestow::Color{
        static_cast<std::uint8_t>(config_->getIntOr("hud.gameOver.color.r", 200)),
        static_cast<std::uint8_t>(config_->getIntOr("hud.gameOver.color.g", 0)),
        static_cast<std::uint8_t>(config_->getIntOr("hud.gameOver.color.b", 0)),
        static_cast<std::uint8_t>(config_->getIntOr("hud.gameOver.color.a", 255))
    };

    // Load assets
    playerTextureHandle_ = sys.assets->registerAsset(
        bestow::AssetType::Texture, config_->getStringOr("assets.textures.player", "data/textures/player_spritesheet.png"));
    coinTextureHandle_ = sys.assets->registerAsset(
        bestow::AssetType::Texture, config_->getStringOr("assets.textures.coin", "data/textures/coinGold.png"));
    enemyTextureHandle_ = sys.assets->registerAsset(
        bestow::AssetType::Texture, config_->getStringOr("assets.textures.enemy", "data/textures/enemyWalking_1.png"));
    platformTextureHandle_ = sys.assets->registerAsset(
        bestow::AssetType::Texture, config_->getStringOr("assets.textures.platform", "data/textures/block.png"));

    // Load audio
    jumpSoundHandle_ = sys.assets->registerAsset(
        bestow::AssetType::Sound, config_->getStringOr("assets.sounds.jump", "data/audio/sfx/phaseJump1.ogg"));
    coinSoundHandle_ = sys.assets->registerAsset(
        bestow::AssetType::Sound, config_->getStringOr("assets.sounds.coin", "data/audio/sfx/pepSound1.ogg"));
    hurtSoundHandle_ = sys.assets->registerAsset(
        bestow::AssetType::Sound, config_->getStringOr("assets.sounds.hurt", "data/audio/sfx/impactBell_heavy_000.ogg"));

    // Load font asset
    fontHandle_ = sys.assets->registerAsset(
        bestow::AssetType::Font, config_->getStringOr("assets.fonts.main", "data/fonts/PressStart2P-Regular.ttf"));

    // Load level asset
    levelAssetHandle_ = sys.assets->registerAsset(
        bestow::AssetType::Level, config_->getStringOr("assets.levels.level1", "data/levels/level1.lua"));

    // Load all assets (synchronous for demo - in real game use async)
    sys.assets->loadAsset(playerTextureHandle_);
    sys.assets->loadAsset(coinTextureHandle_);
    sys.assets->loadAsset(enemyTextureHandle_);
    sys.assets->loadAsset(platformTextureHandle_);
    sys.assets->loadAsset(fontHandle_);
    sys.assets->loadAsset(jumpSoundHandle_);
    sys.assets->loadAsset(coinSoundHandle_);
    sys.assets->loadAsset(hurtSoundHandle_);
    sys.assets->loadAsset(levelAssetHandle_);

    // Load level into Level System
    auto levelResult = sys.levels->loadLevel(levelAssetHandle_);
    if (levelResult.has_value()) {
        currentLevelId_ = levelResult.value();
        sys.levels->setActiveLevel(currentLevelId_);
    } else {
        bestow::core::logError("Failed to load level1.lua");
    }

    // Setup player sprite sheet
    playerSheet_ = bestow::SpriteSheet{
        .texture = playerTextureHandle_,
        .frameWidth = config_->getIntOr("sprites.player.frameWidth", 66),
        .frameHeight = config_->getIntOr("sprites.player.frameHeight", 92),
        .columns = config_->getIntOr("sprites.player.columns", 9),
        .rows = config_->getIntOr("sprites.player.rows", 7),
        .padding = config_->getIntOr("sprites.player.padding", 0)
    };

    playerSprite_.sheet = playerSheet_;
    playerSprite_.playing = true;

    // Define animations
    bestow::Animation idleAnim;
    idleAnim.name = "idle";
    idleAnim.looping = true;
    idleAnim.frames = {{0, 0.1f}, {1, 0.1f}, {2, 0.1f}, {3, 0.1f}};
    playerSprite_.animations["idle"] = idleAnim;

    bestow::Animation runAnim;
    runAnim.name = "run";
    runAnim.looping = true;
    runAnim.frames = {{9, 0.1f}, {10, 0.1f}, {11, 0.1f}, {12, 0.1f}, {13, 0.1f}, {14, 0.1f}};
    playerSprite_.animations["run"] = runAnim;

    bestow::Animation jumpAnim;
    jumpAnim.name = "jump";
    jumpAnim.looping = false;
    jumpAnim.frames = {{18, 0.1f}};
    playerSprite_.animations["jump"] = jumpAnim;

    bestow::Animation fallAnim;
    fallAnim.name = "fall";
    fallAnim.looping = false;
    fallAnim.frames = {{27, 0.1f}};
    playerSprite_.animations["fall"] = fallAnim;

    playerSprite_.play("idle");

    // Setup static sprite sheets
    coinSheet_ = bestow::SpriteSheet{
        .texture = coinTextureHandle_,
        .frameWidth = config_->getIntOr("sprites.coin.frameWidth", 128),
        .frameHeight = config_->getIntOr("sprites.coin.frameHeight", 128),
        .columns = config_->getIntOr("sprites.coin.columns", 1),
        .rows = config_->getIntOr("sprites.coin.rows", 1)
    };

    enemySheet_ = bestow::SpriteSheet{
        .texture = enemyTextureHandle_,
        .frameWidth = config_->getIntOr("sprites.enemy.frameWidth", 32),
        .frameHeight = config_->getIntOr("sprites.enemy.frameHeight", 44),
        .columns = config_->getIntOr("sprites.enemy.columns", 1),
        .rows = config_->getIntOr("sprites.enemy.rows", 1)
    };

    platformSheet_ = bestow::SpriteSheet{
        .texture = platformTextureHandle_,
        .frameWidth = config_->getIntOr("sprites.platform.frameWidth", 226),
        .frameHeight = config_->getIntOr("sprites.platform.frameHeight", 148),
        .columns = config_->getIntOr("sprites.platform.columns", 1),
        .rows = config_->getIntOr("sprites.platform.rows", 1)
    };

    // Setup input mappings
    sys.input->registerMapping(bestow::InputMapping{
        .binding = bestow::InputBinding{.deviceType = bestow::InputDeviceType::Keyboard, .keyCode = config_->getIntOr("input.moveLeft.primary", 65)},
        .action = "move_left"
    });
    sys.input->registerMapping(bestow::InputMapping{
        .binding = bestow::InputBinding{.deviceType = bestow::InputDeviceType::Keyboard, .keyCode = config_->getIntOr("input.moveRight.primary", 69)},
        .action = "move_right"
    });
    sys.input->registerMapping(bestow::InputMapping{
        .binding = bestow::InputBinding{.deviceType = bestow::InputDeviceType::Keyboard, .keyCode = config_->getIntOr("input.jump.primary", 32)},
        .action = "jump"
    });
    // Secondary keys (arrow keys)
    sys.input->registerMapping(bestow::InputMapping{
        .binding = bestow::InputBinding{.deviceType = bestow::InputDeviceType::Keyboard, .keyCode = config_->getIntOr("input.moveLeft.secondary", 263)},
        .action = "move_left"
    });
    sys.input->registerMapping(bestow::InputMapping{
        .binding = bestow::InputBinding{.deviceType = bestow::InputDeviceType::Keyboard, .keyCode = config_->getIntOr("input.moveRight.secondary", 262)},
        .action = "move_right"
    });

    // Subscribe to events
    triggerSubscription_ = sys.events->subscribe(
        bestow::Events::TriggerEnter,
        [this](const bestow::EventData& data) { onTriggerEnter(data); }
    );

    collisionSubscription_ = sys.events->subscribe(
        bestow::Events::Collision,
        [this](const bestow::EventData& data) { onCollision(data); }
    );

    // Setup game world
    setupPlayer();
    loadLevel();

    bestow::core::logInfo("Platformer demo initialized successfully");
    return true;
}

void Game::setupPlayer() {
    auto& sys = engine_->systems();
    player_ = sys.entities->createEntity();

    // Get spawn point from level (fallback from config if not found)
    float spawnX = config_->getFloatOr("player.spawnFallback.x", 100.0f);
    float spawnY = config_->getFloatOr("player.spawnFallback.y", 400.0f);
    if (auto spawnPoint = sys.levels->getSpawnPoint(currentLevelId_, "player")) {
        spawnX = spawnPoint->x;
        spawnY = spawnPoint->y;
    }

    // Transform component - scale sprite to match physics body (from config)
    sys.entities->emplace<bestow::Transform2D>(player_, bestow::Transform2D{
        .x = spawnX,
        .y = spawnY,
        .rotation = 0.0f,
        .scaleX = playerBodyWidth_ / static_cast<float>(playerSheet_.frameWidth),
        .scaleY = playerBodyHeight_ / static_cast<float>(playerSheet_.frameHeight)
    });

    // Components from bestow.components (values from config)
    int initialHealth = config_->getIntOr("player.health.initial", 100);
    int maxHealth = config_->getIntOr("player.health.maximum", 100);
    sys.entities->emplace<bestow::components::Health>(player_, bestow::components::Health{
        .current = initialHealth,
        .maximum = maxHealth
    });
    health_ = initialHealth;

    int initialScore = config_->getIntOr("player.initialScore", 0);
    sys.entities->emplace<bestow::components::Score>(player_, bestow::components::Score{
        .value = initialScore
    });
    score_ = initialScore;

    sys.entities->emplace<bestow::components::GroundDetector>(player_);

    sys.entities->emplace<bestow::components::JumpState>(player_, bestow::components::JumpState{
        .jumpsRemaining = 1,
        .maxJumps = playerMaxJumps_,
        .jumpForce = playerJumpForce_
    });

    sys.entities->getRegistry().emplace<bestow::components::PlayerTag>(player_);

    // Physics body (all values from config)
    bestow::PhysicsBodyDef bodyDef{
        .type = bestow::BodyType::Dynamic,
        .transform = {.x = spawnX, .y = spawnY},
        .size = {playerBodyWidth_, playerBodyHeight_},
        .fixedRotation = config_->getBoolOr("physics.player.fixedRotation", true),
        .density = playerDensity_,
        .friction = playerFriction_,
        .restitution = playerRestitution_
    };

    sys.physics->createBody(player_, bodyDef);

    // Set camera target to player
    cameraSystem_->setTarget(player_);
}

void Game::loadLevel() {
    auto& sys = engine_->systems();

    // Get entity definitions from the Level System (parsed from level1.lua)
    auto entityDefs = sys.levels->getEntityDefs(currentLevelId_);

    for (const auto& def : entityDefs) {
        if (def.type == "platform") {
            // Extract width/height from properties (default to reasonable values)
            float width = 100.0f;
            float height = 30.0f;

            if (auto it = def.properties.find("width"); it != def.properties.end()) {
                width = std::any_cast<double>(it->second);
            }
            if (auto it = def.properties.find("height"); it != def.properties.end()) {
                height = std::any_cast<double>(it->second);
            }

            createPlatform(def.transform.x, def.transform.y, width, height);
        }
        else if (def.type == "coin") {
            // Extract value from properties (default)
            int value = 10;
            if (auto it = def.properties.find("value"); it != def.properties.end()) {
                value = static_cast<int>(std::any_cast<double>(it->second));
            }

            createCoin(def.transform.x, def.transform.y, value);
        }
        else if (def.type == "enemy") {
            // Extract patrolRange from properties
            float patrolRange = 100.0f;
            if (auto it = def.properties.find("patrolRange"); it != def.properties.end()) {
                patrolRange = static_cast<float>(std::any_cast<double>(it->second));
            }

            createEnemy(def.transform.x, def.transform.y, patrolRange);
        }
        else {
            bestow::core::logWarn("Unknown entity type: " + def.type);
        }
    }
}

void Game::createPlatform(float x, float y, float width, float height) {
    auto& sys = engine_->systems();
    auto platform = sys.entities->createEntity();

    // Scale sprite to match physics body
    sys.entities->emplace<bestow::Transform2D>(platform, bestow::Transform2D{
        .x = x,
        .y = y,
        .rotation = 0.0f,
        .scaleX = width / static_cast<float>(platformSheet_.frameWidth),
        .scaleY = height / static_cast<float>(platformSheet_.frameHeight)
    });

    sys.entities->getRegistry().emplace<bestow::components::PlatformTag>(platform);

    // Static physics body (friction from config)
    bestow::PhysicsBodyDef bodyDef{
        .type = bestow::BodyType::Static,
        .transform = {.x = x, .y = y},
        .size = {width, height},
        .density = 0.0f,
        .friction = platformFriction_
    };

    sys.physics->createBody(platform, bodyDef);
}

void Game::createCoin(float x, float y, int value) {
    auto& sys = engine_->systems();
    auto coin = sys.entities->createEntity();

    // Scale sprite to match physics body (size from config)
    sys.entities->emplace<bestow::Transform2D>(coin, bestow::Transform2D{
        .x = x,
        .y = y,
        .rotation = 0.0f,
        .scaleX = coinBodySize_ / static_cast<float>(coinSheet_.frameWidth),
        .scaleY = coinBodySize_ / static_cast<float>(coinSheet_.frameHeight)
    });

    sys.entities->getRegistry().emplace<bestow::components::CollectibleTag>(coin);

    sys.entities->emplace<bestow::components::Score>(coin, bestow::components::Score{
        .value = value
    });

    // Sensor body (size from config)
    bestow::PhysicsBodyDef bodyDef{
        .type = bestow::BodyType::Static,
        .transform = {.x = x, .y = y},
        .size = {coinBodySize_, coinBodySize_},
        .density = 0.0f,
        .isSensor = config_->getBoolOr("physics.coin.isSensor", true)
    };

    sys.physics->createBody(coin, bodyDef);
}

void Game::createEnemy(float x, float y, float patrolRange) {
    auto& sys = engine_->systems();
    auto enemy = sys.entities->createEntity();

    // Scale sprite to match physics body (size from config)
    sys.entities->emplace<bestow::Transform2D>(enemy, bestow::Transform2D{
        .x = x,
        .y = y,
        .rotation = 0.0f,
        .scaleX = enemyBodySize_ / static_cast<float>(enemySheet_.frameWidth),
        .scaleY = enemyBodySize_ / static_cast<float>(enemySheet_.frameHeight)
    });

    sys.entities->getRegistry().emplace<bestow::components::EnemyTag>(enemy);

    sys.entities->emplace<bestow::components::Damage>(enemy, bestow::components::Damage{
        .amount = enemyDamage_
    });

    // Dynamic body (all values from config)
    bestow::PhysicsBodyDef bodyDef{
        .type = bestow::BodyType::Dynamic,
        .transform = {.x = x, .y = y},
        .size = {enemyBodySize_, enemyBodySize_},
        .fixedRotation = config_->getBoolOr("physics.enemy.fixedRotation", true),
        .density = enemyDensity_,
        .friction = enemyFriction_
    };

    sys.physics->createBody(enemy, bodyDef);

    // Configure patrol behavior via AI System (speed from config)
    sys.ai->setPatrolBehavior(enemy, bestow::PatrolBehavior{
        .startX = x,
        .range = patrolRange,
        .speed = enemyPatrolSpeed_,
        .movingRight = true
    });
}

//==============================================================================
// Update
//==============================================================================

void Game::updateFixed(bestow::DeltaTime dt) {
    if (gameOver_) return;

    auto& sys = engine_->systems();

    handlePlayerInput(dt);

    // Update AI system (handles enemy patrol behavior)
    sys.ai->update(dt);

    // Sync physics positions to Transform2D components for rendering
    syncPhysicsToTransforms();

    updatePlayerAnimation();
    updateCamera(dt);
    checkTriggers();

    // Update health component invincibility timer
    if (auto* health = sys.entities->tryGet<bestow::components::Health>(player_)) {
        health->update(dt);
        health_ = health->current;

        if (health->isDead() && !gameOver_) {
            gameOver_ = true;
            bestow::core::logInfo("Game Over!");
        }
    }

    // Update player animation sprite
    playerSprite_.update(dt);

// Note: HotReload requires asset system watcher to be active
// #if defined(BESTOW_DEV_TOOLS)
//     hotReload_.update();
// #endif
}

void Game::handlePlayerInput(bestow::DeltaTime dt) {
    auto& sys = engine_->systems();

    // Get player velocity
    auto velocity = sys.physics->getVelocity(player_);

    // Horizontal movement (speed from config)
    if (sys.input->isActionActive("move_left")) {
        velocity.x = -playerMoveSpeed_;
    }
    else if (sys.input->isActionActive("move_right")) {
        velocity.x = playerMoveSpeed_;
    }
    else {
        velocity.x = 0.0f;
    }

    // Jump handling with double jump support
    if (sys.input->wasActionJustPressed("jump")) {
        auto* groundDetector = sys.entities->tryGet<bestow::components::GroundDetector>(player_);
        auto* jumpState = sys.entities->tryGet<bestow::components::JumpState>(player_);

        if (groundDetector && jumpState) {
            // Reset jumps when grounded
            if (groundDetector->isGrounded) {
                jumpState->reset();
            }

            // Can jump if grounded (coyote time) or have jumps remaining (double jump)
            if (groundDetector->canJump() || jumpState->canJump()) {
                velocity.y = -jumpState->jumpForce;  // NEGATIVE = up (Y-down screen coords)
                jumpState->jump();
                groundDetector->consumeJump();

                // Play jump sound (volume from config)
                sys.audio->playPositional(bestow::PositionalSound{.asset = jumpSoundHandle_, .volume = jumpSoundVolume_});

                bestow::core::logInfo("Jump! Remaining jumps: " + std::to_string(jumpState->jumpsRemaining));
            }
        }
    }

    // Apply velocity
    sys.physics->setVelocity(player_, velocity);

    // Update ground detection (threshold from config)
    if (auto* groundDetector = sys.entities->tryGet<bestow::components::GroundDetector>(player_)) {
        bool currentlyGrounded = std::abs(velocity.y) < groundedVelocityThreshold_;
        groundDetector->update(dt, currentlyGrounded);
    }
}

void Game::updatePlayerAnimation() {
    auto& sys = engine_->systems();
    auto velocity = sys.physics->getVelocity(player_);
    auto* groundDetector = sys.entities->tryGet<bestow::components::GroundDetector>(player_);

    PlayerState newState = playerState_;

    if (groundDetector && groundDetector->isGrounded) {
        // Use animation thresholds from config
        if (std::abs(velocity.x) > runningAnimThreshold_) {
            newState = PlayerState::Running;
        } else {
            newState = PlayerState::Idle;
        }
    } else {
        // jumpingAnimThreshold_ is negative (e.g., -10.0f means velocity.y must be < -10)
        if (velocity.y < jumpingAnimThreshold_) {
            newState = PlayerState::Jumping;
        } else {
            newState = PlayerState::Falling;
        }
    }

    // Update animation if state changed
    if (newState != playerState_) {
        playerState_ = newState;

        switch (playerState_) {
            case PlayerState::Idle:
                playerSprite_.play("idle");
                break;
            case PlayerState::Running:
                playerSprite_.play("run");
                break;
            case PlayerState::Jumping:
                playerSprite_.play("jump");
                break;
            case PlayerState::Falling:
                playerSprite_.play("fall");
                break;
        }
    }
}

void Game::updateCamera(bestow::DeltaTime dt) {
    auto& sys = engine_->systems();

    // Get player position
    auto playerTransform = sys.entities->get<bestow::Transform2D>(player_);

    // Update camera to follow player
    cameraSystem_->update(dt, playerTransform.position());

    // Apply camera to graphics system
    sys.graphics->setCamera(cameraSystem_->getCamera());
}

void Game::syncPhysicsToTransforms() {
    auto& sys = engine_->systems();

    // Sync player transform from physics
    auto playerPos = sys.physics->getPosition(player_);
    auto& playerTransform = sys.entities->get<bestow::Transform2D>(player_);
    playerTransform.x = playerPos.x;
    playerTransform.y = playerPos.y;

    // Sync enemy transforms from physics using ECS view
    auto enemyView = sys.entities->view<bestow::components::EnemyTag, bestow::Transform2D>();
    for (auto entity : enemyView) {
        auto pos = sys.physics->getPosition(entity);
        auto& transform = enemyView.get<bestow::Transform2D>(entity);
        transform.x = pos.x;
        transform.y = pos.y;
    }
}

void Game::checkTriggers() {
    // This is handled by physics events now (TriggerEnter)
}

//==============================================================================
// Event Handlers
//==============================================================================

void Game::onTriggerEnter(const bestow::EventData& data) {
    auto& sys = engine_->systems();
    auto& triggerData = std::get<bestow::TriggerEvent>(data);

    // Check if player touched a collectible
    bool playerInvolved = (triggerData.entityA == player_ || triggerData.entityB == player_);
    if (!playerInvolved) return;

    bestow::Entity other = (triggerData.entityA == player_) ? triggerData.entityB : triggerData.entityA;

    // Check if it's a coin
    if (sys.entities->anyOf<bestow::components::CollectibleTag>(other)) {
        // Get coin value
        if (auto* coinScore = sys.entities->tryGet<bestow::components::Score>(other)) {
            // Add to player score
            if (auto* playerScore = sys.entities->tryGet<bestow::components::Score>(player_)) {
                playerScore->add(coinScore->value);
                score_ = playerScore->value;

                bestow::core::logInfo("Collected coin! Score: " + std::to_string(score_));

                // Play coin sound (volume from config)
                sys.audio->playPositional(bestow::PositionalSound{.asset = coinSoundHandle_, .volume = coinSoundVolume_});

                // Destroy coin entity (ECS handles cleanup automatically)
                sys.entities->destroyEntity(other);
            }
        }
    }
}

void Game::onCollision(const bestow::EventData& data) {
    auto& sys = engine_->systems();
    auto& collisionData = std::get<bestow::CollisionEvent>(data);

    // Check if player hit an enemy
    bool playerInvolved = (collisionData.entityA == player_ || collisionData.entityB == player_);
    if (!playerInvolved) return;

    bestow::Entity other = (collisionData.entityA == player_) ? collisionData.entityB : collisionData.entityA;

    // Check if it's an enemy
    if (sys.entities->anyOf<bestow::components::EnemyTag>(other)) {
        if (auto* health = sys.entities->tryGet<bestow::components::Health>(player_)) {
            if (auto* damage = sys.entities->tryGet<bestow::components::Damage>(other)) {
                health->takeDamage(damage->amount);

                if (!health->isDead()) {
                    // Play hurt sound (volume from config)
                    sys.audio->playPositional(bestow::PositionalSound{.asset = hurtSoundHandle_, .volume = hurtSoundVolume_});

                    // Camera shake on hit (values from config)
                    cameraSystem_->shake(cameraShakeMagnitude_, cameraShakeDuration_);

                    bestow::core::logInfo("Player hit! Health: " + std::to_string(health->current));
                }
            }
        }
    }
}

//==============================================================================
// Rendering
//==============================================================================

void Game::render(float alpha) {
    // Note: Engine::run() already calls beginFrame()/endFrame() around this method
    // so we just render directly - no double buffer swap!

    renderEntities();
    renderHUD();

// Note: DevOverlay requires ImGui to be initialized first
// #if defined(BESTOW_DEV_TOOLS)
//     devOverlay_.render();
// #endif
}

void Game::renderEntities() {
    auto& sys = engine_->systems();

    // Get player transform and velocity for facing direction
    auto playerTransform = sys.entities->get<bestow::Transform2D>(player_);
    auto velocity = sys.physics->getVelocity(player_);

    // Flip sprite based on movement direction while preserving base scale
    bestow::Transform2D playerRenderTransform = playerTransform;
    float flipDir = (velocity.x < -0.1f) ? -1.0f : 1.0f;
    playerRenderTransform.scaleX = std::abs(playerTransform.scaleX) * flipDir;

    // Draw player using animated sprite
    sys.graphics->drawAnimatedSprite(playerSprite_, playerRenderTransform);

    // Render platforms using sprite sheet
    auto platformView = sys.entities->view<bestow::components::PlatformTag, bestow::Transform2D>();
    for (auto entity : platformView) {
        auto& transform = platformView.get<bestow::Transform2D>(entity);
        sys.graphics->drawSprite(platformSheet_, 0, transform);
    }

    // Render coins using sprite sheet (scale is already set in createCoin)
    auto coinView = sys.entities->view<bestow::components::CollectibleTag, bestow::Transform2D>();
    for (auto entity : coinView) {
        auto& transform = coinView.get<bestow::Transform2D>(entity);
        sys.graphics->drawSprite(coinSheet_, 0, transform);
    }

    // Render enemies using sprite sheet
    auto enemyRenderView = sys.entities->view<bestow::components::EnemyTag, bestow::Transform2D>();
    for (auto entity : enemyRenderView) {
        auto& transform = enemyRenderView.get<bestow::Transform2D>(entity);
        // Flip sprite based on patrol direction from AI system
        bestow::Transform2D enemyRenderTransform = transform;
        float flipDir = 1.0f;
        if (auto patrol = sys.ai->getPatrolBehavior(entity)) {
            flipDir = patrol->movingRight ? 1.0f : -1.0f;
        }
        enemyRenderTransform.scaleX = std::abs(transform.scaleX) * flipDir;
        sys.graphics->drawSprite(enemySheet_, 0, enemyRenderTransform);
    }
}

void Game::renderHUD() {
    auto& sys = engine_->systems();

    // HUD needs to be drawn relative to camera position (world coordinates)
    auto camera = cameraSystem_->getCamera();
    float camX = camera.transform.x;
    float camY = camera.transform.y;
    float halfWidth = camera.viewportSize.width / 2.0f;
    float halfHeight = camera.viewportSize.height / 2.0f;

    // Calculate top-left corner of screen in world coordinates
    float screenLeft = camX - halfWidth;
    float screenTop = camY - halfHeight;

    // Health bar (all values from config)
    float healthPercent = static_cast<float>(health_) / 100.0f;
    bestow::Canvas healthBarBg{
        .origin = {
            static_cast<int>(screenLeft + hudHealthBarX_),
            static_cast<int>(screenTop + hudHealthBarY_)
        },
        .size = {hudHealthBarWidth_, hudHealthBarHeight_}
    };
    bestow::Canvas healthBarFill{
        .origin = {
            static_cast<int>(screenLeft + hudHealthBarX_),
            static_cast<int>(screenTop + hudHealthBarY_)
        },
        .size = {
            static_cast<int>(hudHealthBarWidth_ * healthPercent),
            hudHealthBarHeight_
        }
    };

    sys.graphics->drawRect(healthBarBg, hudHealthBarBgColor_, true);
    sys.graphics->drawRect(healthBarFill, hudHealthBarFillColor_, true);

    // Score text (rendered using font)
    std::string scoreText = hudScorePrefix_ + std::to_string(score_);
    bestow::Vec2 scorePos{
        screenLeft + static_cast<float>(hudScoreX_),
        screenTop + static_cast<float>(hudScoreY_) + hudScoreFontSize_  // Adjust for baseline
    };
    sys.graphics->drawText(scoreText, scorePos, fontHandle_, hudScoreFontSize_, hudScoreColor_);

    // Game Over message (rendered using font)
    if (gameOver_) {
        bestow::Vec2 gameOverPos{camX, camY};
        sys.graphics->drawTextCentered(hudGameOverText_, gameOverPos, fontHandle_, hudGameOverFontSize_, hudGameOverColor_);
    }
}

//==============================================================================
// Shutdown
//==============================================================================

void Game::shutdown() {
    bestow::core::logInfo("Shutting down platformer demo");

    auto& sys = engine_->systems();

    // Unsubscribe from events
    sys.events->unsubscribe(triggerSubscription_);
    sys.events->unsubscribe(collisionSubscription_);

    // Shutdown config system
    if (config_) {
        config_->shutdown();
    }

    bestow::core::logInfo("Platformer demo shutdown complete");
}

}  // namespace platformer_demo

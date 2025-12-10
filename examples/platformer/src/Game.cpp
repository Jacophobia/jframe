// examples/platformer/src/Game.cpp
// Platformer game implementation - Showcases ALL 10 Bestow Engine Systems

// Include compatibility headers before module imports to avoid MSVC C++23 module issues
#include <bestow/entt_compat.hpp>
#include <bestow/sol2_compat.hpp>

import std;
import bestow;

#if defined(BESTOW_DEV_TOOLS)
import bestow.dev;
#endif

#include "Game.h"
#include "Components.h"

namespace platformer {

bool Game::initialize(bestow::core::Engine& engine) {
    bestow::core::logInfo("Initializing Platformer Game - Full System Integration");

    engine_ = &engine;
    auto& sys = engine.systems();

    // ===== EVENTS SYSTEM =====
    // Subscribe to collision events
    collisionSubscription_ = sys.events->subscribe(bestow::Events::Collision,
        [this](const bestow::EventData& data) {
            handleCollisionEvent(data);
        });

    // ===== INPUT SYSTEM =====
    setupInputMappings();

    // ===== ASSETS SYSTEM =====
    // Register level asset (use Data type for Lua files)
    levelAsset_ = sys.assets->registerAsset(bestow::AssetType::Data, "data/levels/level1.lua");
    sys.assets->loadAsset(levelAsset_);

    // Wait for asset to finish loading (synchronous load should be done, but check state)
    auto assetState = sys.assets->getAssetState(levelAsset_);
    if (assetState != bestow::AssetState::Loaded) {
        auto metadata = sys.assets->getAssetMetadata(levelAsset_);
        std::string errorMsg = metadata.errorMessage.value_or("Unknown error");
        bestow::core::logError(std::format("Level asset failed to load (state: {}): {}",
            static_cast<int>(assetState), errorMsg));
        return false;
    }

    // Register audio assets (stub-safe - will work even without FMOD)
    jumpSoundAsset_ = sys.assets->registerAsset(bestow::AssetType::Sound, "data/audio/jump.wav");
    coinSoundAsset_ = sys.assets->registerAsset(bestow::AssetType::Sound, "data/audio/coin.wav");
    hurtSoundAsset_ = sys.assets->registerAsset(bestow::AssetType::Sound, "data/audio/hurt.wav");
    musicAsset_ = sys.assets->registerAsset(bestow::AssetType::Music, "data/audio/music.ogg");

    // ===== LEVEL SYSTEM =====
    // Load the level
    auto levelResult = sys.levels->loadLevel(levelAsset_);
    if (levelResult) {
        currentLevel_ = *levelResult;
        sys.levels->setActiveLevel(currentLevel_);
        bestow::core::logInfo("Level loaded successfully");
    } else {
        bestow::core::logError("Failed to load level");
        return false;
    }

    // ===== ENTITY SYSTEM =====
    // Create player entity
    createPlayer();

    // Create camera entity
    camera_ = sys.entities->createEntity();
    sys.entities->emplace<Camera2D>(camera_, Camera2D{
        .target = player_,
        .smoothing = 0.1f,
        .offset = {0.0f, 100.0f},
        .position = {400.0f, 300.0f}
    });

    // ===== LEVEL SYSTEM + PHYSICS SYSTEM =====
    // Load level geometry (platforms) from Lua
    loadLevel();

    // ===== AI SYSTEM + PHYSICS SYSTEM =====
    // Spawn enemies based on level data
    spawnEnemies(currentLevel_);

    // ===== Spawn collectibles =====
    spawnCollectibles(currentLevel_);

    // ===== AUDIO SYSTEM =====
    // Start background music (stub-safe)
    sys.audio->playOnChannel(bestow::Channels::Music, bestow::ChannelSound{
        .asset = musicAsset_,
        .volume = 0.5f,
        .looping = true
    });

    // ===== SAVE SYSTEM =====
    // Try to load previous save
    loadGame();

#if defined(BESTOW_DEV_TOOLS)
    hotReload_.watchDirectory("data/");

    hotReload_.onLevelChanged = [this](const auto& path) {
        bestow::core::logInfo("Level changed: " + path.string());
        auto& sys = engine_->systems();
        sys.assets->reloadAsset(levelAsset_);
        loadLevel();
    };

    hotReload_.onBlueprintChanged = [this](const auto& path) {
        bestow::core::logInfo("Blueprint changed: " + path.string());
        // TODO: Update blueprint registry when asset system supports it
    };
#endif

    bestow::core::logInfo("Platformer game initialized successfully - All 10 systems active!");
    return true;
}

void Game::updateFixed(bestow::DeltaTime dt) {
#if defined(BESTOW_DEV_TOOLS)
    hotReload_.update();
#endif

    auto& sys = engine_->systems();

    // Update game playtime
    gameState_.playtime += dt;

    // ===== INPUT SYSTEM =====
    handlePlayerInput(dt);

    // ===== PHYSICS SYSTEM =====
    updatePlayerMovement(dt);

    // ===== AI SYSTEM =====
    updateEnemyAI(dt);

    // ===== Check collectible pickup (custom collision logic) =====
    checkCollectiblePickup();

    // ===== Update camera =====
    updateCamera(dt);

    // ===== EVENTS SYSTEM =====
    // Process queued events
    sys.events->processQueue();

    // ===== SAVE SYSTEM =====
    // Quick save on checkpoint (demo purposes - check if S key pressed)
    if (sys.input->wasActionJustPressed("save_game")) {
        saveGame();
        bestow::core::logInfo("Game saved!");
    }

    // Quick load (demo purposes - check if L key pressed)
    if (sys.input->wasActionJustPressed("load_game")) {
        loadGame();
        bestow::core::logInfo("Game loaded!");
    }
}

void Game::render(float alpha) {
    auto& sys = engine_->systems();

    // Get camera for rendering
    auto* cam = sys.entities->tryGet<Camera2D>(camera_);
    if (cam) {
        bestow::Camera gameCamera;
        gameCamera.transform.x = cam->position.x;
        gameCamera.transform.y = cam->position.y;
        gameCamera.zoom = 1.0f;
        gameCamera.viewportSize = {800, 600};
        sys.graphics->setCamera(gameCamera);
    }

    // Render all sprite entities
    auto& registry = sys.entities->getRegistry();
    auto view = registry.view<bestow::Sprite>();
    for (auto [entity, sprite] : view.each()) {
        sys.graphics->draw(sprite);
    }

    // Debug rendering - draw player as a green rectangle
    if (sys.entities->isValid(player_) && sys.physics->hasBody(player_)) {
        bestow::Vec2 pos = sys.physics->getPosition(player_);
        sys.graphics->drawRect(
            {static_cast<int>(pos.x - 12), static_cast<int>(pos.y - 22), 24, 44},
            bestow::Color::green()
        );
    }

    // Debug rendering - draw enemies as red rectangles
    for (auto enemy : enemies_) {
        if (sys.entities->isValid(enemy) && sys.physics->hasBody(enemy)) {
            bestow::Vec2 pos = sys.physics->getPosition(enemy);
            sys.graphics->drawRect(
                {static_cast<int>(pos.x - 10), static_cast<int>(pos.y - 10), 20, 20},
                bestow::Color::red()
            );
        }
    }

    // Debug rendering - draw collectibles as yellow circles (rectangles for now)
    for (auto collectible : collectibles_) {
        if (sys.entities->isValid(collectible)) {
            auto* coll = sys.entities->tryGet<Collectible>(collectible);
            if (coll && !coll->collected && sys.physics->hasBody(collectible)) {
                bestow::Vec2 pos = sys.physics->getPosition(collectible);
                bestow::Color color = (coll->type == Collectible::Type::Coin)
                    ? bestow::Color{255, 215, 0, 255}  // Gold
                    : bestow::Color{255, 0, 255, 255}; // Magenta for health
                sys.graphics->drawRect(
                    {static_cast<int>(pos.x - 8), static_cast<int>(pos.y - 8), 16, 16},
                    color
                );
            }
        }
    }

    // Draw UI - Score and Health
    // TODO: Text rendering when available
}

void Game::shutdown() {
    auto& sys = engine_->systems();

    // ===== EVENTS SYSTEM =====
    // Unsubscribe from events
    sys.events->unsubscribe(collisionSubscription_);

    // ===== AUDIO SYSTEM =====
    // Stop all audio
    sys.audio->stopAll();

    bestow::core::logInfo("Shutting down Platformer Game");
}

void Game::setupInputMappings() {
    auto& input = *engine_->systems().input;

    // Movement mappings
    input.registerMapping(bestow::InputMapping{
        .binding = bestow::InputBinding{
            .deviceType = bestow::InputDeviceType::Keyboard,
            .keyCode = 65,  // A key
            .scale = -1.0f
        },
        .action = "move_left"
    });

    input.registerMapping(bestow::InputMapping{
        .binding = bestow::InputBinding{
            .deviceType = bestow::InputDeviceType::Keyboard,
            .keyCode = 68,  // D key
            .scale = 1.0f
        },
        .action = "move_right"
    });

    input.registerMapping(bestow::InputMapping{
        .binding = bestow::InputBinding{
            .deviceType = bestow::InputDeviceType::Keyboard,
            .keyCode = 32  // Space
        },
        .action = "jump"
    });

    // Arrow keys as alternatives
    input.registerMapping(bestow::InputMapping{
        .binding = bestow::InputBinding{
            .deviceType = bestow::InputDeviceType::Keyboard,
            .keyCode = 263,  // Left arrow
            .scale = -1.0f
        },
        .action = "move_left"
    });

    input.registerMapping(bestow::InputMapping{
        .binding = bestow::InputBinding{
            .deviceType = bestow::InputDeviceType::Keyboard,
            .keyCode = 262,  // Right arrow
            .scale = 1.0f
        },
        .action = "move_right"
    });

    // Save/Load mappings
    input.registerMapping(bestow::InputMapping{
        .binding = bestow::InputBinding{
            .deviceType = bestow::InputDeviceType::Keyboard,
            .keyCode = 83  // S key
        },
        .action = "save_game"
    });

    input.registerMapping(bestow::InputMapping{
        .binding = bestow::InputBinding{
            .deviceType = bestow::InputDeviceType::Keyboard,
            .keyCode = 76  // L key
        },
        .action = "load_game"
    });

    bestow::core::logInfo("Input mappings configured");
}

void Game::loadLevel() {
    auto& sys = engine_->systems();

    // Get level metadata to access level data
    auto metadata = sys.levels->getLevelMetadata(currentLevel_);

    // Load level from Lua file using AssetSystem
    const void* rawAsset = sys.assets->getRawAsset(levelAsset_);
    if (rawAsset == nullptr) {
        bestow::core::logError("Level asset not loaded");
        return;
    }

    try {
        // Parse the Lua level file manually (since LevelSystem only parses metadata)
        const std::any* assetAny = static_cast<const std::any*>(rawAsset);
        const auto& dataAsset = std::any_cast<const bestow::DataAsset&>(*assetAny);

        sol::state lua;
        lua.open_libraries(sol::lib::base, sol::lib::math, sol::lib::table);

        // Execute the level Lua file
        // Use script_pass_on_error so invalid Lua returns an error result instead of throwing
        auto result = lua.safe_script(dataAsset.rawText, sol::script_pass_on_error);
        if (!result.valid()) {
            bestow::core::logError("Failed to parse level Lua");
            return;
        }

        sol::table levelTable = result;

        // ===== PHYSICS SYSTEM - Create platforms from level data =====
        if (levelTable["platforms"].valid()) {
            sol::table platforms = levelTable["platforms"];
            for (size_t i = 1; i <= platforms.size(); ++i) {
                sol::table platform = platforms[i];

                float x = platform["x"].get_or(0.0f);
                float y = platform["y"].get_or(0.0f);
                float width = platform["width"].get_or(100.0f);
                float height = platform["height"].get_or(20.0f);

                bestow::Entity platformEntity = sys.entities->createEntity();
                sys.entities->emplace<PlatformTag>(platformEntity, PlatformTag{});

                bestow::PhysicsBodyDef platformDef{
                    .type = bestow::BodyType::Static,
                    .transform = {.x = x, .y = y},
                    .size = {width, height},
                    .fixedRotation = true
                };
                sys.physics->createBody(platformEntity, platformDef);
            }
        }

        bestow::core::logInfo("Level geometry loaded from Lua");

    } catch (const std::exception& e) {
        bestow::core::logError(std::format("Failed to load level: {}", e.what()));
    }
}

void Game::createPlayer() {
    auto& sys = engine_->systems();

    // Get spawn point from Level System
    auto spawnPoint = sys.levels->getSpawnPoint(currentLevel_, "player");
    bestow::Vec2 startPos = spawnPoint
        ? bestow::Vec2{spawnPoint->x, spawnPoint->y}
        : bestow::Vec2{100.0f, 300.0f};

    player_ = sys.entities->createEntity();
    sys.entities->emplace<PlayerTag>(player_, PlayerTag{});
    sys.entities->emplace<PlayerController>(player_, PlayerController{
        .moveSpeed = 200.0f,
        .jumpForce = 450.0f,
        .airControl = 0.3f
    });
    sys.entities->emplace<Velocity>(player_);
    sys.entities->emplace<Health>(player_, Health{
        .current = 100,
        .maximum = 100
    });

    // Create physics body for player
    bestow::PhysicsBodyDef playerDef{
        .type = bestow::BodyType::Dynamic,
        .transform = {.x = startPos.x, .y = startPos.y},
        .size = {24.0f, 44.0f},
        .fixedRotation = true,
        .linearDamping = 0.0f
    };
    sys.physics->createBody(player_, playerDef);

    bestow::core::logInfo(std::format("Player created at ({}, {})", startPos.x, startPos.y));
}

void Game::spawnEnemies(bestow::LevelId levelId) {
    auto& sys = engine_->systems();

    // Load level data to get enemy definitions
    const void* rawAsset = sys.assets->getRawAsset(levelAsset_);
    if (rawAsset == nullptr) return;

    try {
        const std::any* assetAny = static_cast<const std::any*>(rawAsset);
        const auto& dataAsset = std::any_cast<const bestow::DataAsset&>(*assetAny);

        sol::state lua;
        lua.open_libraries(sol::lib::base, sol::lib::math, sol::lib::table);

        // Use script_pass_on_error so invalid Lua returns an error result instead of throwing
        auto result = lua.safe_script(dataAsset.rawText, sol::script_pass_on_error);
        if (!result.valid()) return;

        sol::table levelTable = result;

        // ===== AI SYSTEM + PHYSICS SYSTEM - Spawn enemies =====
        if (levelTable["enemies"].valid()) {
            sol::table enemiesTable = levelTable["enemies"];
            for (size_t i = 1; i <= enemiesTable.size(); ++i) {
                sol::table enemyDef = enemiesTable[i];

                std::string spawnPointName = enemyDef["spawnPoint"].get_or(std::string("enemy1"));
                auto spawnPoint = sys.levels->getSpawnPoint(levelId, spawnPointName);
                if (!spawnPoint) continue;

                bestow::Entity enemy = sys.entities->createEntity();
                sys.entities->emplace<EnemyTag>(enemy, EnemyTag{});
                sys.entities->emplace<EnemyAI>(enemy, EnemyAI{
                    .patrolStart = {spawnPoint->x, spawnPoint->y},
                    .patrolRange = enemyDef["patrolRange"].get_or(150.0f),
                    .moveSpeed = enemyDef["moveSpeed"].get_or(50.0f),
                    .damage = enemyDef["damage"].get_or(10),
                    .movingRight = true
                });

                // Create physics body
                bestow::PhysicsBodyDef enemyBodyDef{
                    .type = bestow::BodyType::Dynamic,
                    .transform = {.x = spawnPoint->x, .y = spawnPoint->y},
                    .size = {20.0f, 20.0f},
                    .fixedRotation = true
                };
                sys.physics->createBody(enemy, enemyBodyDef);

                enemies_.push_back(enemy);
            }

            bestow::core::logInfo(std::format("Spawned {} enemies", enemies_.size()));
        }

    } catch (const std::exception& e) {
        bestow::core::logError(std::format("Failed to spawn enemies: {}", e.what()));
    }
}

void Game::spawnCollectibles(bestow::LevelId levelId) {
    auto& sys = engine_->systems();

    // Load level data to get collectible definitions
    const void* rawAsset = sys.assets->getRawAsset(levelAsset_);
    if (rawAsset == nullptr) return;

    try {
        const std::any* assetAny = static_cast<const std::any*>(rawAsset);
        const auto& dataAsset = std::any_cast<const bestow::DataAsset&>(*assetAny);

        sol::state lua;
        lua.open_libraries(sol::lib::base, sol::lib::math, sol::lib::table);

        // Use script_pass_on_error so invalid Lua returns an error result instead of throwing
        auto result = lua.safe_script(dataAsset.rawText, sol::script_pass_on_error);
        if (!result.valid()) return;

        sol::table levelTable = result;

        if (levelTable["collectibles"].valid()) {
            sol::table collectiblesTable = levelTable["collectibles"];
            for (size_t i = 1; i <= collectiblesTable.size(); ++i) {
                sol::table collectibleDef = collectiblesTable[i];

                float x = collectibleDef["x"].get_or(0.0f);
                float y = collectibleDef["y"].get_or(0.0f);
                std::string typeStr = collectibleDef["type"].get_or(std::string("coin"));
                int value = collectibleDef["value"].get_or(10);

                Collectible::Type type = (typeStr == "health")
                    ? Collectible::Type::Health
                    : Collectible::Type::Coin;

                bestow::Entity collectible = sys.entities->createEntity();
                sys.entities->emplace<CollectibleTag>(collectible, CollectibleTag{});
                sys.entities->emplace<Collectible>(collectible, Collectible{
                    .type = type,
                    .value = value,
                    .collected = false
                });

                // Create physics body (as sensor/trigger)
                bestow::PhysicsBodyDef collectibleBodyDef{
                    .type = bestow::BodyType::Static,
                    .transform = {.x = x, .y = y},
                    .size = {16.0f, 16.0f},
                    .fixedRotation = true
                };
                sys.physics->createBody(collectible, collectibleBodyDef);

                collectibles_.push_back(collectible);
            }

            bestow::core::logInfo(std::format("Spawned {} collectibles", collectibles_.size()));
        }

    } catch (const std::exception& e) {
        bestow::core::logError(std::format("Failed to spawn collectibles: {}", e.what()));
    }
}

void Game::updateEnemyAI(bestow::DeltaTime dt) {
    auto& sys = engine_->systems();

    // ===== AI SYSTEM - Simple patrol behavior =====
    for (auto enemy : enemies_) {
        if (!sys.entities->isValid(enemy)) continue;

        auto* ai = sys.entities->tryGet<EnemyAI>(enemy);
        if (!ai || !sys.physics->hasBody(enemy)) continue;

        bestow::Vec2 currentPos = sys.physics->getPosition(enemy);

        // Simple patrol logic - move back and forth
        float distanceFromStart = currentPos.x - ai->patrolStart.x;

        if (ai->movingRight) {
            if (distanceFromStart >= ai->patrolRange) {
                ai->movingRight = false;
            }
        } else {
            if (distanceFromStart <= -ai->patrolRange) {
                ai->movingRight = true;
            }
        }

        // Apply velocity
        float moveDirection = ai->movingRight ? 1.0f : -1.0f;
        bestow::Vec2 velocity = sys.physics->getVelocity(enemy);
        velocity.x = moveDirection * ai->moveSpeed;
        sys.physics->setVelocity(enemy, velocity);

        // ===== AI SYSTEM - Line of sight check (demonstration) =====
        if (sys.physics->hasBody(player_)) {
            bestow::Vec2 playerPos = sys.physics->getPosition(player_);
            bool canSeePlayer = sys.ai->hasLineOfSight(currentPos, playerPos, 0xFFFF);
            // In a real game, we'd change behavior here (chase player, etc.)
            // For now, just continue patrolling
        }
    }
}

void Game::checkCollectiblePickup() {
    auto& sys = engine_->systems();

    if (!sys.physics->hasBody(player_)) return;
    bestow::Vec2 playerPos = sys.physics->getPosition(player_);

    // Check distance to collectibles
    for (auto collectible : collectibles_) {
        if (!sys.entities->isValid(collectible)) continue;

        auto* coll = sys.entities->tryGet<Collectible>(collectible);
        if (!coll || coll->collected || !sys.physics->hasBody(collectible)) continue;

        bestow::Vec2 collPos = sys.physics->getPosition(collectible);
        float distance = std::sqrt(
            (playerPos.x - collPos.x) * (playerPos.x - collPos.x) +
            (playerPos.y - collPos.y) * (playerPos.y - collPos.y)
        );

        if (distance < 30.0f) {
            // Collect the item
            coll->collected = true;

            if (coll->type == Collectible::Type::Coin) {
                gameState_.score += coll->value;
                gameState_.coinsCollected++;

                // ===== AUDIO SYSTEM - Play coin sound =====
                sys.audio->playPositional(bestow::PositionalSound{
                    .asset = coinSoundAsset_,
                    .position = {collPos.x, collPos.y, 0.0f},
                    .volume = 0.7f
                });

                // ===== EVENTS SYSTEM - Publish coin collected event =====
                sys.events->queue(bestow::Events::ItemCollected,
                    bestow::EntityEventData{collectible, player_});

            } else if (coll->type == Collectible::Type::Health) {
                auto* health = sys.entities->tryGet<Health>(player_);
                if (health) {
                    health->current = std::min(health->current + coll->value, health->maximum);
                }
            }

            bestow::core::logInfo(std::format("Collected item! Score: {}", gameState_.score));
        }
    }
}

void Game::handleCollisionEvent(const bestow::EventData& data) {
    auto& sys = engine_->systems();

    // ===== EVENTS SYSTEM - Handle collision events =====
    try {
        const auto& collision = std::get<bestow::CollisionEvent>(data);

        // Check if player collided with enemy
        bool playerHit = false;
        bestow::Entity hitEnemy;

        if (collision.entityA == player_) {
            if (sys.entities->tryGet<EnemyTag>(collision.entityB)) {
                playerHit = true;
                hitEnemy = collision.entityB;
            }
        } else if (collision.entityB == player_) {
            if (sys.entities->tryGet<EnemyTag>(collision.entityA)) {
                playerHit = true;
                hitEnemy = collision.entityA;
            }
        }

        if (playerHit) {
            auto* health = sys.entities->tryGet<Health>(player_);
            auto* ai = sys.entities->tryGet<EnemyAI>(hitEnemy);

            if (health && ai && health->invincibilityTime <= 0.0f) {
                health->current -= ai->damage;
                health->invincibilityTime = health->invincibilityDuration;

                // ===== AUDIO SYSTEM - Play hurt sound =====
                sys.audio->playPositional(bestow::PositionalSound{
                    .asset = hurtSoundAsset_,
                    .position = {collision.contactPoint.x, collision.contactPoint.y, 0.0f},
                    .volume = 1.0f
                });

                // ===== EVENTS SYSTEM - Publish damage event =====
                sys.events->queue(bestow::Events::EntityDamaged,
                    bestow::DamageEventData{player_, hitEnemy, ai->damage, collision.normal});

                bestow::core::logInfo(std::format("Player hit! Health: {}", health->current));

                if (health->current <= 0) {
                    // ===== EVENTS SYSTEM - Publish death event =====
                    sys.events->publish(bestow::Events::PlayerDeath,
                        bestow::EntityEventData{player_, std::nullopt});
                    bestow::core::logInfo("Player died!");
                }
            }
        }

    } catch (const std::bad_variant_access&) {
        // Not a collision event
    }
}

void Game::handlePlayerInput(bestow::DeltaTime dt) {
    auto& sys = engine_->systems();

    auto* controller = sys.entities->tryGet<PlayerController>(player_);
    if (!controller || !sys.physics->hasBody(player_)) return;

    // Get horizontal input
    float moveLeft = sys.input->getActionValue("move_left");
    float moveRight = sys.input->getActionValue("move_right");
    float horizontal = moveLeft + moveRight;

    // Apply horizontal movement
    float moveForce = horizontal * controller->moveSpeed;
    bestow::Vec2 currentVel = sys.physics->getVelocity(player_);

    if (controller->isGrounded) {
        currentVel.x = moveForce;
    } else {
        // Air control
        currentVel.x += moveForce * controller->airControl * dt;
    }

    // Jump input
    bool jumpPressed = sys.input->wasActionJustPressed("jump");
    if (jumpPressed && (controller->isGrounded || controller->coyoteTime > 0.0f)) {
        currentVel.y = controller->jumpForce;
        controller->coyoteTime = 0.0f;

        // ===== AUDIO SYSTEM - Play jump sound =====
        bestow::Vec2 pos = sys.physics->getPosition(player_);
        sys.audio->playPositional(bestow::PositionalSound{
            .asset = jumpSoundAsset_,
            .position = {pos.x, pos.y, 0.0f},
            .volume = 0.8f
        });
    }

    sys.physics->setVelocity(player_, currentVel);
}

void Game::updatePlayerMovement(bestow::DeltaTime dt) {
    auto& sys = engine_->systems();

    auto* controller = sys.entities->tryGet<PlayerController>(player_);
    if (!controller || !sys.physics->hasBody(player_)) return;

    // Update invincibility
    auto* health = sys.entities->tryGet<Health>(player_);
    if (health && health->invincibilityTime > 0.0f) {
        health->invincibilityTime -= dt;
    }

    bestow::Vec2 velocity = sys.physics->getVelocity(player_);
    bestow::Vec2 position = sys.physics->getPosition(player_);

    // Simple ground detection
    bool wasGrounded = controller->isGrounded;
    controller->isGrounded = (std::abs(velocity.y) < 1.0f && position.y < 150.0f);

    // Update coyote time
    if (!controller->isGrounded && wasGrounded) {
        controller->coyoteTime = PlayerController::kCoyoteTimeMax;
    } else if (controller->isGrounded) {
        controller->coyoteTime = PlayerController::kCoyoteTimeMax;
    } else {
        controller->coyoteTime -= dt;
    }
}

void Game::updateCamera(bestow::DeltaTime dt) {
    auto& sys = engine_->systems();

    auto* cam = sys.entities->tryGet<Camera2D>(camera_);
    if (!cam) return;

    if (!sys.entities->isValid(cam->target)) return;
    if (!sys.physics->hasBody(cam->target)) return;

    bestow::Vec2 targetPosition = sys.physics->getPosition(cam->target);

    // Smooth camera follow
    bestow::Vec2 targetPos = {targetPosition.x + cam->offset.x, targetPosition.y + cam->offset.y};
    cam->position.x += (targetPos.x - cam->position.x) * cam->smoothing;
    cam->position.y += (targetPos.y - cam->position.y) * cam->smoothing;
}

void Game::saveGame() {
    auto& sys = engine_->systems();

    // ===== SAVE SYSTEM - Save game state =====
    if (!sys.physics->hasBody(player_)) return;

    bestow::Vec2 playerPos = sys.physics->getPosition(player_);
    auto* health = sys.entities->tryGet<Health>(player_);

    PlayerSaveData saveData{
        .position = playerPos,
        .health = health ? health->current : 100,
        .score = gameState_.score,
        .playtime = gameState_.playtime
    };

    auto result = sys.save->save(0, "Platformer Quick Save");
    if (result) {
        // ===== EVENTS SYSTEM - Publish save event =====
        sys.events->publish(bestow::Events::GameSaved, std::any{});
        bestow::core::logInfo("Game saved successfully!");
    } else {
        bestow::core::logError("Failed to save game");
    }
}

void Game::loadGame() {
    auto& sys = engine_->systems();

    // ===== SAVE SYSTEM - Load game state =====
    if (!sys.save->saveExists(0)) {
        bestow::core::logInfo("No save file found");
        return;
    }

    auto result = sys.save->load(0);
    if (result) {
        // TODO: Restore player position and state from save data
        // For now, just log that load succeeded

        // ===== EVENTS SYSTEM - Publish load event =====
        sys.events->publish(bestow::Events::GameLoaded, std::any{});
        bestow::core::logInfo("Game loaded successfully!");
    } else {
        bestow::core::logError("Failed to load game");
    }
}

}  // namespace platformer

// template/src/Game.cpp
// JFrame Template Game - Implementation
// This file demonstrates how to use all JFrame systems together

// Standard library headers with iterator/comparison support must come before EnTT
#include <algorithm>
#include <any>
#include <compare>
#include <fstream>
#include <iterator>
#include <memory>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

// EnTT after standard headers for MSVC C++23 module compatibility
#include <entt/entt.hpp>
import jframe;
import jframe.core;
import jframe.gas;
import jframe.gas.impl;
import jframe.camera;
import jframe.camera.impl;
import jframe.blueprints;
import jframe.blueprints.impl;
import jframe.config.impl;
import jframe.builders;
import jframe.luaconfig;

#include "Game.h"

namespace template_game {

// ============================================================================
// Application Lifecycle
// ============================================================================

bool Game::initialize(jframe::core::Engine& engine) {
    jframe::core::logInfo("Initializing Template Game");

    engine_ = &engine;

    // Setup all systems in order
    setupConfig();
    setupInput();
    setupBlueprints();
    setupGAS();
    setupAudio();
    setupCamera();

    // Create game entities
    createPlayer();
    loadLevel();

    // Start background music
    playMusic("background");

    jframe::core::logInfo("Template Game initialized successfully!");
    jframe::core::logInfo("Controls:");
    jframe::core::logInfo("  A/D or Left/Right Arrow - Move");
    jframe::core::logInfo("  W/Space or Up Arrow - Jump");

    return true;
}

void Game::updateFixed(jframe::DeltaTime dt) {
    auto& sys = engine_->systems();

    // Update GAS system (cooldowns, effects, etc.)
    gas_->update(dt);

    // Handle player input
    handlePlayerInput(dt);

    // Update player movement state
    updatePlayerMovement(dt);

    // Update camera to follow player
    updateCamera(dt);
}

void Game::render(float alpha) {
    auto& sys = engine_->systems();

    // Sync physics positions to Transform2D for rendering
    // This ensures visuals match physics simulation
    for (jframe::Entity entity : sys.entities->view<jframe::Transform2D>()) {
        if (sys.physics->hasBody(entity)) {
            jframe::Vec2 pos = sys.physics->getPosition(entity);
            auto* transform = sys.entities->tryGet<jframe::Transform2D>(entity);
            if (transform) {
                transform->x = pos.x;
                transform->y = pos.y;
            }
        }
    }

    // Apply camera transform for world rendering
    if (camera_) {
        jframe::Camera gameCamera = camera_->getCamera();
        sys.graphics->setCamera(gameCamera);
    }

    // Render all entities with DebugRect/DebugCircle components
    // This handles platforms, player, and any other visual entities
    sys.graphics->renderEntities(*sys.entities);

    // Render UI (health bar, etc.)
    renderUI();
}

void Game::shutdown() {
    jframe::core::logInfo("Shutting down Template Game");

    // Shutdown systems in reverse order of initialization
    // Note: Audio system is already being shut down by Engine
    camera_.reset();
    blueprints_.reset();
    config_.reset();
    gas_.reset();

    jframe::core::logInfo("Template Game shutdown complete");
}

// ============================================================================
// System Setup
// ============================================================================

void Game::setupConfig() {
    config_ = jframe::createConfigSystem();
    config_->initialize();

    // Load game configuration from Lua
    if (!config_->loadConfig("data/config/game.lua")) {
        jframe::core::logWarn("Could not load game config, using defaults");
        return;
    }

    // Extract player configuration values
    playerMoveSpeed_ = config_->getFloatOr("player.moveSpeed", 200.0f);
    playerJumpForce_ = config_->getFloatOr("player.jumpForce", 400.0f);
    playerPhysicsWidth_ = config_->getFloatOr("player.physics.width", 30.0f);
    playerPhysicsHeight_ = config_->getFloatOr("player.physics.height", 50.0f);

    jframe::core::logInfo("Game configuration loaded");
}

void Game::setupInput() {
    auto& input = *engine_->systems().input;

    // Use InputMappingBuilder for clean input configuration
    // Actions are defined in code, but could also be loaded from Lua
    jframe::InputMappingBuilder(input)
        // Movement
        .action("move_left")
            .key(jframe::Keys::A)
            .key(jframe::Keys::Left)
            .button(jframe::ControllerButtons::DPadLeft)
        .action("move_right")
            .key(jframe::Keys::D)
            .key(jframe::Keys::Right)
            .button(jframe::ControllerButtons::DPadRight)

        // Jump
        .action("jump")
            .key(jframe::Keys::Space)
            .key(jframe::Keys::W)
            .key(jframe::Keys::Up)
            .button(jframe::ControllerButtons::A)

        .apply();

    jframe::core::logInfo("Input mappings configured");
}

void Game::setupBlueprints() {
    auto& sys = engine_->systems();

    // Create blueprint factory for data-driven entity creation
    blueprints_ = jframe::createBlueprintFactory(*sys.entities, sys.physics);

    // Register custom components (DebugRect/DebugCircle are built-in)
    blueprints_->registerComponent("PlayerTag",
        [](jframe::Entity e, jframe::IEntitySystem& sys, const jframe::PropertyMap&) {
            sys.emplace<PlayerTag>(e);
        });

    blueprints_->registerComponent("PlatformTag",
        [](jframe::Entity e, jframe::IEntitySystem& sys, const jframe::PropertyMap&) {
            sys.emplace<PlatformTag>(e);
        });

    // Load blueprints from Lua file
    std::ifstream file("data/blueprints/entities.lua");
    if (!file) {
        jframe::core::logError("Could not open data/blueprints/entities.lua");
        return;
    }

    std::stringstream buffer;
    buffer << file.rdbuf();

    if (blueprints_->loadBlueprints(buffer.str())) {
        jframe::core::logInfo("Loaded " +
            std::to_string(blueprints_->getBlueprintNames().size()) + " blueprints");
    } else {
        jframe::core::logError("Failed to parse blueprints");
    }
}

void Game::setupGAS() {
    // Create Gameplay Ability System
    gas_ = jframe::createGASSystem();
    if (!gas_) {
        jframe::core::logError("Failed to create GAS system");
        return;
    }

    // For this simple template, we'll create a basic jump ability in code
    // In a real game, you'd load this from Lua like in ability-demo

    // Create basic attributes first (abilities may depend on them)
    jframe::AttributeDef healthDef;
    healthDef.name = "Health";
    healthDef.baseValue = 100.0f;
    healthAttr_ = gas_->registerAttribute(healthDef);

    jframe::AttributeDef staminaDef;
    staminaDef.name = "Stamina";
    staminaDef.baseValue = 100.0f;
    staminaAttr_ = gas_->registerAttribute(staminaDef);

    // Create jump ability definition
    jframe::AbilityDef jumpDef;
    jumpDef.name = "Jump";
    jumpDef.cooldown = 0.0f;  // No cooldown for basic jump
    // Note: costs is a vector of AbilityCost, empty = no cost
    jumpDef.costs = {};  // No resource costs for basic jump

    // Register ability with GAS
    jumpAbility_ = gas_->registerAbility(jumpDef);

    jframe::core::logInfo("GAS system initialized");
}

void Game::setupAudio() {
    auto& sys = engine_->systems();

    // Register audio assets
    // Note: These files don't exist yet - the audio system handles missing files gracefully
    std::vector<std::pair<std::string, std::string>> sounds = {
        {"background", "data/audio/background_music.ogg"},
        {"jump", "data/audio/jump.wav"},
    };

    for (const auto& [name, path] : sounds) {
        auto handle = sys.assets->registerAsset(jframe::AssetType::Sound, path);
        soundAssets_[name] = handle;
        // Load asynchronously (won't block game startup)
        sys.assets->loadAssetAsync(handle, [](jframe::AssetHandle, jframe::AssetState) {});
    }

    jframe::core::logInfo("Audio system initialized");
}

void Game::setupCamera() {
    // Create camera system with viewport size
    camera_ = jframe::createCameraSystem(jframe::Size{800, 600});

    // Configure camera following behavior
    camera_->setFollowSmoothing(0.1f);  // 0.0 = instant, 0.9 = very smooth
    camera_->setOffset({0.0f, -50.0f}); // Look slightly ahead of player
    camera_->setTarget(player_);        // Follow the player entity

    jframe::core::logInfo("Camera system initialized");
}

// ============================================================================
// Entity Creation
// ============================================================================

void Game::createPlayer() {
    auto& sys = engine_->systems();

    // Load level first to get spawn point
    levelAssetHandle_ = sys.assets->registerAsset(
        jframe::AssetType::Level, "data/levels/main.lua");
    sys.assets->loadAsset(levelAssetHandle_);

    auto levelResult = sys.levels->loadLevel(levelAssetHandle_);
    if (levelResult.has_value()) {
        currentLevelId_ = levelResult.value();
        sys.levels->setActiveLevel(currentLevelId_);
    }

    // Get spawn point from level
    float spawnX = 400.0f;
    float spawnY = 300.0f;
    if (auto spawnPoint = sys.levels->getSpawnPoint(currentLevelId_, "player")) {
        spawnX = spawnPoint->x;
        spawnY = spawnPoint->y;
    }

    // Create player using blueprint factory
    player_ = blueprints_->create("Player", spawnX, spawnY,
                                   playerPhysicsWidth_, playerPhysicsHeight_);

    // Initialize GAS component for player
    gas_->initializeComponent(player_);
    gas_->initializeAttribute(player_, healthAttr_, 100.0f);
    gas_->initializeAttribute(player_, staminaAttr_, 100.0f);
    gas_->grantAbility(player_, jumpAbility_);

    jframe::core::logInfo("Player created at (" +
        std::to_string(spawnX) + ", " + std::to_string(spawnY) + ")");
}

void Game::loadLevel() {
    auto& sys = engine_->systems();

    // Get entity definitions from level
    auto entityDefs = sys.levels->getEntityDefs(currentLevelId_);

    // Helper to extract numbers from Lua std::any
    auto getNumber = [](const std::any& val, float defaultVal) -> float {
        try {
            return static_cast<float>(std::any_cast<double>(val));
        } catch (const std::bad_any_cast&) {
            try {
                return static_cast<float>(std::any_cast<long long>(val));
            } catch (const std::bad_any_cast&) {
                try {
                    return static_cast<float>(std::any_cast<int>(val));
                } catch (const std::bad_any_cast&) {
                    return defaultVal;
                }
            }
        }
    };

    // Create entities from level definitions
    int entityCount = 0;
    for (const auto& def : entityDefs) {
        // Map level entity types to blueprint names
        std::string blueprintName;
        if (def.type == "platform") {
            blueprintName = "Platform";
        } else {
            jframe::core::logWarn("Unknown entity type: " + def.type);
            continue;
        }

        // Extract dimensions from properties
        float width = 100.0f;
        float height = 20.0f;
        if (auto it = def.properties.find("width"); it != def.properties.end()) {
            width = getNumber(it->second, 100.0f);
        }
        if (auto it = def.properties.find("height"); it != def.properties.end()) {
            height = getNumber(it->second, 20.0f);
        }

        // Create entity using blueprint
        jframe::Entity entity = blueprints_->create(
            blueprintName, def.transform.x, def.transform.y, width, height);

        if (sys.entities->isValid(entity)) {
            // Set platform collision layer
            sys.physics->setCollisionLayer(entity, jframe::CollisionLayers::Ground);
            entityCount++;
        }
    }

    size_t platformCount = sys.entities->groupCount<PlatformTag>();
    jframe::core::logInfo("Level loaded with " +
        std::to_string(platformCount) + " platforms");
}

// ============================================================================
// Game Logic
// ============================================================================

void Game::handlePlayerInput(jframe::DeltaTime dt) {
    auto& sys = engine_->systems();

    if (!sys.physics->hasBody(player_)) return;

    // Horizontal movement
    float moveInput = 0.0f;
    if (sys.input->isActionActive("move_left")) {
        moveInput -= 1.0f;
    }
    if (sys.input->isActionActive("move_right")) {
        moveInput += 1.0f;
    }

    // Apply horizontal velocity
    jframe::Vec2 velocity = sys.physics->getVelocity(player_);
    velocity.x = moveInput * playerMoveSpeed_;
    sys.physics->setVelocity(player_, velocity);

    // Jump (only when grounded)
    if (sys.input->wasActionJustPressed("jump") && isGrounded_) {
        velocity = sys.physics->getVelocity(player_);
        velocity.y = -playerJumpForce_;
        sys.physics->setVelocity(player_, velocity);

        // Try to activate jump ability through GAS (for cooldown/cost tracking)
        if (gas_->canActivateAbility(player_, jumpAbility_)) {
            gas_->tryActivateAbility(player_, jumpAbility_);
            gas_->endAbility(player_, jumpAbility_);
        }

        playSound("jump");
        jframe::core::logInfo("Jump!");
    }
}

void Game::updatePlayerMovement(jframe::DeltaTime dt) {
    auto& sys = engine_->systems();

    if (!sys.physics->hasBody(player_)) return;

    // Check if player is grounded using physics system
    auto groundResult = sys.physics->checkGrounded(player_);
    isGrounded_ = groundResult.grounded;
}

void Game::updateCamera(jframe::DeltaTime dt) {
    auto& sys = engine_->systems();

    if (!camera_ || !sys.entities->isValid(player_)) return;
    if (!sys.physics->hasBody(player_)) return;

    // Update camera to follow player position
    jframe::Vec2 targetPos = sys.physics->getPosition(player_);
    camera_->update(dt, targetPos);
}

void Game::renderUI() {
    auto& sys = engine_->systems();

    if (!sys.entities->isValid(player_)) return;

    // Reset camera to screen space for UI rendering
    jframe::Camera uiCamera;
    uiCamera.transform.x = 400.0f;  // Center of 800x600 viewport
    uiCamera.transform.y = 300.0f;
    uiCamera.zoom = 1.0f;
    uiCamera.viewportSize = {800, 600};
    sys.graphics->setCamera(uiCamera);

    // Get player health from GAS
    float health = gas_->getAttributeValue(player_, healthAttr_);

    // Draw health bar (top-left corner)
    int healthBarWidth = static_cast<int>((health / 100.0f) * 200.0f);
    sys.graphics->drawRect({10, 10, 200, 20}, jframe::Color{50, 50, 50, 255}, true);
    sys.graphics->drawRect({10, 10, healthBarWidth, 20}, jframe::Color{255, 0, 0, 255}, true);
    sys.graphics->drawRect({10, 10, 200, 20}, jframe::Color::white(), false);

    // Draw simple instructions
    // (In a real game, you'd use text rendering here)
}

// ============================================================================
// Audio
// ============================================================================

void Game::playSound(const std::string& soundName) {
    if (!audioEnabled_) return;

    auto& sys = engine_->systems();
    auto it = soundAssets_.find(soundName);
    if (it == soundAssets_.end()) return;

    // Check if asset is loaded
    if (sys.assets->getAssetState(it->second) != jframe::AssetState::Loaded) {
        return;
    }

    jframe::ChannelSound sound{
        .asset = it->second,
        .volume = 0.8f,
        .pitch = 1.0f,
        .looping = false,
        .fadeInTime = 0.0f
    };

    sys.audio->playOnChannel(SFX_CHANNEL, sound);
}

void Game::playMusic(const std::string& musicName) {
    if (!audioEnabled_) return;

    auto& sys = engine_->systems();
    auto it = soundAssets_.find(musicName);
    if (it == soundAssets_.end()) return;

    if (sys.assets->getAssetState(it->second) != jframe::AssetState::Loaded) {
        return;
    }

    jframe::ChannelSound sound{
        .asset = it->second,
        .volume = 0.5f,
        .pitch = 1.0f,
        .looping = true,
        .fadeInTime = 1.0f
    };

    sys.audio->playOnChannel(MUSIC_CHANNEL, sound);
}

}  // namespace template_game

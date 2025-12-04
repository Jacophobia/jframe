// examples/ability-demo/src/Game.cpp
// Gameplay Ability System demonstration implementation

// Use compatibility header for MSVC C++23 module support
#include <jframe/entt_compat.hpp>

#include <fstream>
#include <sstream>
#include <cmath>

import std;
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
#include "Components.h"

namespace abilitydemo {

bool Game::initialize(jframe::core::Engine& engine) {
    jframe::core::logInfo("Initializing GAS Demo Game");

    engine_ = &engine;
    auto& sys = engine.systems();

    // Initialize config system
    config_ = jframe::createConfigSystem();
    config_->initialize();
    if (!config_->loadConfig("data/config/player.lua")) {
        jframe::core::logWarn("Could not load player config, using defaults");
    }

    // Load player config values
    playerMoveSpeed_ = config_->getFloatOr("movement.speed", 200.0f);
    playerJumpForce_ = config_->getFloatOr("movement.jumpForce", 400.0f);
    playerPhysicsWidth_ = config_->getFloatOr("physics.width", 30.0f);
    playerPhysicsHeight_ = config_->getFloatOr("physics.height", 50.0f);
    staminaRegenRate_ = config_->getFloatOr("staminaRegen", 20.0f);

    // Create the GAS system (it's separate from JFrameEngine)
    gas_ = jframe::createGASSystem();
    if (!gas_) {
        jframe::core::logError("Failed to create GAS system");
        return false;
    }

    // Load level asset
    levelAssetHandle_ = sys.assets->registerAsset(
        jframe::AssetType::Level, "data/levels/level1.lua");
    sys.assets->loadAsset(levelAssetHandle_);

    // Load level into Level System
    auto levelResult = sys.levels->loadLevel(levelAssetHandle_);
    if (levelResult.has_value()) {
        currentLevelId_ = levelResult.value();
        sys.levels->setActiveLevel(currentLevelId_);
    } else {
        jframe::core::logError("Failed to load level1.lua");
    }

    // Setup systems
    setupInputMappings();
    setupGAS();
    setupBlueprints();
    setupAudio();
    createPlayer();
    loadLevel();

    // Start exploration music
    playMusic("musicExploration");

    // Setup camera system (using the new jframe camera module)
    camera_ = jframe::createCameraSystem(jframe::Size{800, 600});
    // Smoothing: 0.0 = instant snap, 0.9 = very slow following
    // Values > 1.0 are clamped to 1.0 which means NO movement!
    camera_->setFollowSmoothing(0.1f);
    camera_->setOffset({0.0f, -50.0f});
    camera_->setTarget(player_);

    // Grant abilities to player (only Jump at start, Dash requires collectable)
    gas_->grantAbility(player_, jumpAbility_);

    jframe::core::logInfo("GAS Demo initialized successfully!");
    jframe::core::logInfo("Controls:");
    jframe::core::logInfo("  A/E or Arrow Keys - Move");
    jframe::core::logInfo("  SPACE - Jump (when grounded)");
    jframe::core::logInfo("  LEFT SHIFT - Dash (requires cyan collectable)");
    jframe::core::logInfo("  F - Sword Attack (requires collectable)");
    jframe::core::logInfo("  H - Apply health regen");
    jframe::core::logInfo("  Down/S - Ground Pound (in air, requires collectable)");

    return true;
}

void Game::updateFixed(jframe::DeltaTime dt) {
    auto& sys = engine_->systems();
    testTimer_ += dt;

    // Update GAS system
    gas_->update(dt);

    // Update ground and wall detection
    updateGroundCheck();
    updateWallCheck();

    // Handle input
    handlePlayerInput(dt);

    // Update player movement
    updatePlayerMovement(dt);

    // Handle combat
    handleCombat(dt);

    // Update enemies
    updateEnemies(dt);

    // Update boss
    updateBoss(dt);

    // Update moving platforms
    updateMovingPlatforms(dt);

    // Update projectiles
    updateProjectiles(dt);

    // Update switches
    updateSwitches();

    // Check trigger zones
    checkTriggerZones();

    // Check health pickups
    checkHealthPickups();

    // Check checkpoints
    checkCheckpoints();

    // Handle advanced movement abilities
    handleDoubleJump();
    handleWallJump();
    handleGroundPound(dt);

    // Update camera
    updateCamera(dt);

    // Check for collectable pickups
    checkCollectablePickups();

    // Check if player is in jump zone (using Entity Query System!)
    if (sys.physics->hasBody(player_)) {
        jframe::Vec2 playerPos = sys.physics->getPosition(player_);
        auto jumpZoneTag = gas_->findTag("State.InJumpZone");

        bool wasInZone = inJumpZone_;
        inJumpZone_ = false;

        // Use entity query instead of manual vector!
        for (jframe::Entity zone : sys.entities->collect<JumpZoneTag>()) {
            if (sys.physics->hasBody(zone)) {
                jframe::Vec2 zonePos = sys.physics->getPosition(zone);
                float dx = playerPos.x - zonePos.x;
                float dy = playerPos.y - zonePos.y;
                float distance = std::sqrt(dx * dx + dy * dy);

                if (distance < 100.0f) {
                    inJumpZone_ = true;
                    break;
                }
            }
        }

        // Update player's jump zone tag
        if (inJumpZone_ && !wasInZone && jumpZoneTag) {
            gas_->addTag(player_, *jumpZoneTag);
            jframe::core::logInfo("Entered jump zone - you can jump now!");
        } else if (!inJumpZone_ && wasInZone && jumpZoneTag) {
            gas_->removeTag(player_, *jumpZoneTag);
            jframe::core::logInfo("Left jump zone - jumping disabled");
        }
    }
}

void Game::render(float alpha) {
    auto& sys = engine_->systems();

    // Sync physics positions to Transform2D for ALL entities with physics bodies
    // This keeps visuals aligned with collision at all times
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

    // Debug: count entities with DebugRect (first 5 frames)
    static int debugFrames = 0;
    if (debugFrames < 5) {
        if (debugFrames == 0) {
            size_t withDebugRect = 0;
            size_t withBoth = 0;
            for (jframe::Entity e : sys.entities->view<jframe::DebugRect>()) {
                withDebugRect++;
                if (sys.entities->tryGet<jframe::Transform2D>(e)) {
                    withBoth++;
                }
            }
            jframe::core::logInfo("DEBUG: Entities with DebugRect: " + std::to_string(withDebugRect));
            jframe::core::logInfo("DEBUG: Entities with Transform2D + DebugRect: " + std::to_string(withBoth));
        }

        // Print camera info each frame
        if (camera_) {
            auto cam = camera_->getCamera();
            jframe::core::logInfo("DEBUG frame " + std::to_string(debugFrames) + ": Camera pos: (" +
                std::to_string(cam.transform.x) + ", " + std::to_string(cam.transform.y) + ")");
        }
        debugFrames++;
    }

    // Get camera for rendering (using new camera system)
    if (camera_) {
        jframe::Camera gameCamera = camera_->getCamera();
        sys.graphics->setCamera(gameCamera);
    }

    // Render all entities with DebugRect/DebugCircle components automatically
    // This replaces ~400 lines of manual rendering code!
    sys.graphics->renderEntities(*sys.entities);

    // Render UI
    renderUI();
}

void Game::shutdown() {
    jframe::core::logInfo("Shutting down GAS Demo");

    // Note: Don't call stopMusic() here - the audio system is already being
    // torn down by the Engine before Game::shutdown() is called.

    // Clear sound assets
    soundAssets_.clear();

    // Shutdown systems in reverse order of initialization
    if (blueprints_) {
        blueprints_.reset();
    }

    if (camera_) {
        camera_.reset();
    }

    if (audioConfig_) {
        audioConfig_->shutdown();
        audioConfig_.reset();
    }

    if (config_) {
        config_->shutdown();
        config_.reset();
    }

    // Reset GAS last
    gas_.reset();

    jframe::core::logInfo("GAS Demo shutdown complete");
}

// ============================================================================
// Audio System
// ============================================================================

void Game::setupAudio() {
    auto& sys = engine_->systems();

    // Load audio config
    audioConfig_ = jframe::createConfigSystem();
    audioConfig_->initialize();
    if (!audioConfig_->loadConfig("data/config/audio.lua")) {
        jframe::core::logWarn("Could not load audio config, sounds disabled");
        audioEnabled_ = false;
        return;
    }

    // Register sound assets (we'll create simple placeholder paths)
    // The audio system will gracefully handle missing files
    std::vector<std::pair<std::string, std::string>> sounds = {
        {"jump", "data/audio/jump.wav"},
        {"doubleJump", "data/audio/double_jump.wav"},
        {"dash", "data/audio/dash.wav"},
        {"wallJump", "data/audio/wall_jump.wav"},
        {"groundPound", "data/audio/ground_pound.wav"},
        {"land", "data/audio/land.wav"},
        {"swordSwing1", "data/audio/sword_swing1.wav"},
        {"swordSwing2", "data/audio/sword_swing2.wav"},
        {"swordSwing3", "data/audio/sword_swing3.wav"},
        {"enemyHit", "data/audio/enemy_hit.wav"},
        {"enemyDeath", "data/audio/enemy_death.wav"},
        {"playerHurt", "data/audio/player_hurt.wav"},
        {"shieldBlock", "data/audio/shield_block.wav"},
        {"projectileFire", "data/audio/projectile_fire.wav"},
        {"collectAbility", "data/audio/collect_ability.wav"},
        {"collectHealth", "data/audio/collect_health.wav"},
        {"checkpoint", "data/audio/checkpoint.wav"},
        {"switchActivate", "data/audio/switch_activate.wav"},
        {"doorOpen", "data/audio/door_open.wav"},
        {"breakableDestroy", "data/audio/breakable_destroy.wav"},
        {"bossRoar", "data/audio/boss_roar.wav"},
        {"bossAttack", "data/audio/boss_attack.wav"},
        {"bossHurt", "data/audio/boss_hurt.wav"},
        {"bossDefeat", "data/audio/boss_defeat.wav"},
        {"musicExploration", "data/audio/music_exploration.ogg"},
        {"musicCombat", "data/audio/music_combat.ogg"},
        {"musicBoss", "data/audio/music_boss.ogg"},
    };

    for (const auto& [name, path] : sounds) {
        auto handle = sys.assets->registerAsset(jframe::AssetType::Sound, path);
        soundAssets_[name] = handle;
        // Async load - sounds will be ready when needed
        sys.assets->loadAssetAsync(handle, [](jframe::AssetHandle, jframe::AssetState) {});
    }

    jframe::core::logInfo("Audio system initialized with " + std::to_string(sounds.size()) + " sounds");
}

void Game::playSound(const std::string& soundName) {
    if (!audioEnabled_) return;

    auto& sys = engine_->systems();
    auto it = soundAssets_.find(soundName);
    if (it == soundAssets_.end()) {
        return;  // Sound not registered
    }

    // Check if asset is loaded
    if (sys.assets->getAssetState(it->second) != jframe::AssetState::Loaded) {
        return;  // Not loaded yet
    }

    // Determine which channel to use based on sound type
    jframe::Channel channel = SFX_CHANNEL;
    if (soundName.starts_with("sword") || soundName.starts_with("enemy") ||
        soundName.starts_with("player") || soundName.starts_with("boss") ||
        soundName.starts_with("shield") || soundName.starts_with("projectile")) {
        channel = COMBAT_CHANNEL;
    } else if (soundName == "jump" || soundName == "doubleJump" ||
               soundName == "dash" || soundName == "wallJump" ||
               soundName == "groundPound" || soundName == "land") {
        channel = PLAYER_CHANNEL;
    }

    // Get volume from config (default 0.8)
    float volume = audioConfig_->getFloatOr("sounds." + soundName + ".volume", 0.8f);
    float pitch = audioConfig_->getFloatOr("sounds." + soundName + ".pitch", 1.0f);

    jframe::ChannelSound sound{
        .asset = it->second,
        .volume = volume,
        .pitch = pitch,
        .looping = false,
        .fadeInTime = 0.0f
    };

    sys.audio->playOnChannel(channel, sound);
}

void Game::playMusic(const std::string& musicName) {
    if (!audioEnabled_) return;

    auto& sys = engine_->systems();
    auto it = soundAssets_.find(musicName);
    if (it == soundAssets_.end()) {
        return;
    }

    if (sys.assets->getAssetState(it->second) != jframe::AssetState::Loaded) {
        return;
    }

    float volume = audioConfig_->getFloatOr("sounds." + musicName + ".volume", 0.5f);

    jframe::ChannelSound sound{
        .asset = it->second,
        .volume = volume,
        .pitch = 1.0f,
        .looping = true,
        .fadeInTime = 1.0f
    };

    sys.audio->playOnChannel(MUSIC_CHANNEL, sound);
}

void Game::stopMusic() {
    if (!audioEnabled_) return;
    engine_->systems().audio->stopChannel(MUSIC_CHANNEL, 0.5f);
}

void Game::setupInputMappings() {
    auto& input = *engine_->systems().input;

    // Use the new InputMappingBuilder for cleaner input configuration
    jframe::InputMappingBuilder(input)
        // Movement (A/E for left/right, with arrow key alternatives)
        .action("move_left")
            .key(jframe::Keys::A)
            .key(jframe::Keys::Left)
            .button(jframe::ControllerButtons::DPadLeft)
        .action("move_right")
            .key(jframe::Keys::E)  // E for right (non-standard but keeping existing behavior)
            .key(jframe::Keys::Right)
            .button(jframe::ControllerButtons::DPadRight)

        // Jump
        .action("jump")
            .key(jframe::Keys::Space)
            .key(jframe::Keys::W)  // W also jumps
            .key(jframe::Keys::Up)
            .button(jframe::ControllerButtons::A)

        // Dash
        .action("dash")
            .key(jframe::Keys::LeftShift)
            .button(jframe::ControllerButtons::RightShoulder)

        // Combat abilities
        .action("attack")
            .key(jframe::Keys::F)
            .button(jframe::ControllerButtons::X)
        .action("shield")
            .key(jframe::Keys::G)
            .button(jframe::ControllerButtons::B)
        .action("ranged")
            .key(jframe::Keys::R)
            .button(jframe::ControllerButtons::Y)

        // Ground pound (Down arrow or S)
        .action("ground_pound")
            .key(jframe::Keys::Down)
            .key(jframe::Keys::S)
            .button(jframe::ControllerButtons::DPadDown)

        // Test effects
        .action("apply_health_regen")
            .key(jframe::Keys::H)
        .action("apply_stun")
            .key(jframe::Keys::T)

        .apply();
}

void Game::setupGAS() {
    // Load GAS definitions from Lua
    jframe::core::logInfo("Loading GAS definitions from Lua...");
    std::ifstream file("data/config/abilities.lua");
    if (!file) {
        jframe::core::logError("Could not open data/config/abilities.lua");
        return;
    }

    std::stringstream buffer;
    buffer << file.rdbuf();

    if (!gas_->loadDefinitionsFromLua(buffer.str())) {
        jframe::core::logError("Failed to load GAS definitions");
        return;
    }

    // Get attribute IDs
    auto healthDef = gas_->getAttributeDef("Health");
    auto staminaDef = gas_->getAttributeDef("Stamina");
    auto speedDef = gas_->getAttributeDef("MoveSpeed");

    if (healthDef && staminaDef && speedDef) {
        healthAttr_ = healthDef->id;
        staminaAttr_ = staminaDef->id;
        moveSpeedAttr_ = speedDef->id;
        jframe::core::logInfo("GAS attributes loaded");
    }

    // Get ability IDs
    auto dashDef = gas_->getAbilityDef("Dash");
    auto jumpDef = gas_->getAbilityDef("Jump");
    auto doubleJumpDef = gas_->getAbilityDef("DoubleJump");
    auto wallJumpDef = gas_->getAbilityDef("WallJump");
    auto groundPoundDef = gas_->getAbilityDef("GroundPound");
    auto swordAttackDef = gas_->getAbilityDef("SwordAttack");
    auto swordCombo2Def = gas_->getAbilityDef("SwordCombo2");
    auto swordCombo3Def = gas_->getAbilityDef("SwordCombo3");
    auto shieldDef = gas_->getAbilityDef("Shield");
    auto rangedDef = gas_->getAbilityDef("RangedAttack");

    if (dashDef && jumpDef) {
        dashAbility_ = dashDef->id;
        jumpAbility_ = jumpDef->id;
        jframe::core::logInfo("GAS abilities loaded");
    }

    if (doubleJumpDef) doubleJumpAbility_ = doubleJumpDef->id;
    if (wallJumpDef) wallJumpAbility_ = wallJumpDef->id;
    if (groundPoundDef) groundPoundAbility_ = groundPoundDef->id;
    if (swordAttackDef) swordAttackAbility_ = swordAttackDef->id;
    if (swordCombo2Def) swordCombo2Ability_ = swordCombo2Def->id;
    if (swordCombo3Def) swordCombo3Ability_ = swordCombo3Def->id;
    if (shieldDef) shieldAbility_ = shieldDef->id;
    if (rangedDef) rangedAbility_ = rangedDef->id;

    // Get effect IDs
    auto regenDef = gas_->getEffectDef("HealthRegen");
    auto stunDef = gas_->getEffectDef("Stun");
    auto shieldEffectDef = gas_->getEffectDef("ShieldEffect");
    auto invincibilityDef = gas_->getEffectDef("Invincibility");
    auto comboWindow1Def = gas_->getEffectDef("ComboWindow1");
    auto comboWindow2Def = gas_->getEffectDef("ComboWindow2");

    if (regenDef && stunDef) {
        healthRegenEffect_ = regenDef->id;
        stunEffect_ = stunDef->id;
        jframe::core::logInfo("GAS effects loaded");
    }

    if (shieldEffectDef) shieldEffect_ = shieldEffectDef->id;
    if (invincibilityDef) invincibilityEffect_ = invincibilityDef->id;
    if (comboWindow1Def) comboWindow1Effect_ = comboWindow1Def->id;
    if (comboWindow2Def) comboWindow2Effect_ = comboWindow2Def->id;

    // Set up GAS callbacks
    gas_->setAbilityActivatedCallback([](const jframe::AbilityActivatedEvent& e) {
        jframe::core::logInfo("Ability activated!");
    });
}

void Game::setupBlueprints() {
    auto& sys = engine_->systems();

    // Create the blueprint factory with entity and physics systems
    blueprints_ = jframe::createBlueprintFactory(*sys.entities, sys.physics);

    // Helper functions for safe any_cast
    auto safeGetInt = [](const jframe::PropertyMap& props, const std::string& key, int defaultVal) -> int {
        auto it = props.find(key);
        if (it == props.end()) return defaultVal;
        try {
            return std::any_cast<int>(it->second);
        } catch (...) {
            try { return static_cast<int>(std::any_cast<double>(it->second)); } catch (...) {}
            try { return static_cast<int>(std::any_cast<long long>(it->second)); } catch (...) {}
        }
        return defaultVal;
    };

    auto safeGetFloat = [](const jframe::PropertyMap& props, const std::string& key, float defaultVal) -> float {
        auto it = props.find(key);
        if (it == props.end()) return defaultVal;
        try {
            return std::any_cast<float>(it->second);
        } catch (...) {
            try { return static_cast<float>(std::any_cast<double>(it->second)); } catch (...) {}
            try { return static_cast<float>(std::any_cast<int>(it->second)); } catch (...) {}
        }
        return defaultVal;
    };

    auto safeGetString = [](const jframe::PropertyMap& props, const std::string& key, const std::string& defaultVal) -> std::string {
        auto it = props.find(key);
        if (it == props.end()) return defaultVal;
        try {
            return std::any_cast<std::string>(it->second);
        } catch (...) {
            try { return std::string(std::any_cast<const char*>(it->second)); } catch (...) {}
        }
        return defaultVal;
    };

    auto safeGetBool = [](const jframe::PropertyMap& props, const std::string& key, bool defaultVal) -> bool {
        auto it = props.find(key);
        if (it == props.end()) return defaultVal;
        try {
            return std::any_cast<bool>(it->second);
        } catch (...) {
            try { return std::any_cast<int>(it->second) != 0; } catch (...) {}
        }
        return defaultVal;
    };

    // Register game-specific components
    blueprints_->registerComponent("PlayerTag",
        [](jframe::Entity e, jframe::IEntitySystem& sys, const jframe::PropertyMap&) {
            sys.emplace<PlayerTag>(e);
        });

    blueprints_->registerComponent("PlatformTag",
        [](jframe::Entity e, jframe::IEntitySystem& sys, const jframe::PropertyMap&) {
            sys.emplace<PlatformTag>(e);
        });

    blueprints_->registerComponent("JumpZoneTag",
        [](jframe::Entity e, jframe::IEntitySystem& sys, const jframe::PropertyMap&) {
            sys.emplace<JumpZoneTag>(e);
        });

    blueprints_->registerComponent("EnemyTag",
        [safeGetString, safeGetInt](jframe::Entity e, jframe::IEntitySystem& sys, const jframe::PropertyMap& props) {
            EnemyTag tag;
            tag.type = safeGetString(props, "type", "walker");
            tag.health = safeGetInt(props, "health", 100);
            tag.damage = safeGetInt(props, "damage", 10);
            sys.emplace<EnemyTag>(e, tag);
        });

    blueprints_->registerComponent("WalkerAI",
        [safeGetFloat](jframe::Entity e, jframe::IEntitySystem& sys, const jframe::PropertyMap& props) {
            WalkerAI ai;
            ai.speed = safeGetFloat(props, "speed", 50.0f);
            sys.emplace<WalkerAI>(e, ai);
        });

    blueprints_->registerComponent("CollectableTag",
        [safeGetString](jframe::Entity e, jframe::IEntitySystem& sys, const jframe::PropertyMap& props) {
            CollectableTag tag;
            tag.abilityToGrant = safeGetString(props, "abilityToGrant", "Dash");
            sys.emplace<CollectableTag>(e, tag);
        });

    blueprints_->registerComponent("HealthPickup",
        [safeGetInt](jframe::Entity e, jframe::IEntitySystem& sys, const jframe::PropertyMap& props) {
            HealthPickup hp;
            hp.healAmount = safeGetInt(props, "healAmount", 25);
            sys.emplace<HealthPickup>(e, hp);
        });

    blueprints_->registerComponent("Checkpoint",
        [](jframe::Entity e, jframe::IEntitySystem& sys, const jframe::PropertyMap&) {
            sys.emplace<Checkpoint>(e);
        });

    blueprints_->registerComponent("SwitchTag",
        [safeGetInt](jframe::Entity e, jframe::IEntitySystem& sys, const jframe::PropertyMap& props) {
            SwitchTag sw;
            sw.targetDoorId = safeGetInt(props, "targetDoorId", 1);
            sys.emplace<SwitchTag>(e, sw);
        });

    blueprints_->registerComponent("DoorTag",
        [safeGetInt](jframe::Entity e, jframe::IEntitySystem& sys, const jframe::PropertyMap& props) {
            DoorTag door;
            door.doorId = safeGetInt(props, "doorId", 1);
            sys.emplace<DoorTag>(e, door);
        });

    blueprints_->registerComponent("BreakableTag",
        [safeGetInt](jframe::Entity e, jframe::IEntitySystem& sys, const jframe::PropertyMap& props) {
            BreakableTag br;
            br.hits = safeGetInt(props, "hits", 3);
            sys.emplace<BreakableTag>(e, br);
        });

    blueprints_->registerComponent("MovingPlatform",
        [safeGetFloat](jframe::Entity e, jframe::IEntitySystem& sys, const jframe::PropertyMap& props) {
            MovingPlatform mp;
            mp.speed = safeGetFloat(props, "speed", 50.0f);
            sys.emplace<MovingPlatform>(e, mp);
        });

    blueprints_->registerComponent("TriggerZone",
        [safeGetString](jframe::Entity e, jframe::IEntitySystem& sys, const jframe::PropertyMap& props) {
            TriggerZone tz;
            tz.event = safeGetString(props, "event", "default");
            sys.emplace<TriggerZone>(e, tz);
        });

    blueprints_->registerComponent("BossTag",
        [safeGetInt](jframe::Entity e, jframe::IEntitySystem& sys, const jframe::PropertyMap& props) {
            BossTag boss;
            boss.maxHealth = safeGetInt(props, "maxHealth", 500);
            sys.emplace<BossTag>(e, boss);
        });

    blueprints_->registerComponent("Projectile",
        [safeGetInt, safeGetBool](jframe::Entity e, jframe::IEntitySystem& sys, const jframe::PropertyMap& props) {
            Projectile proj;
            proj.damage = safeGetInt(props, "damage", 10);
            proj.isEnemyProjectile = safeGetBool(props, "isEnemyProjectile", false);
            sys.emplace<Projectile>(e, proj);
        });

    blueprints_->registerComponent("PlayerController",
        [safeGetFloat](jframe::Entity e, jframe::IEntitySystem& sys, const jframe::PropertyMap& props) {
            PlayerController pc;
            pc.moveSpeed = safeGetFloat(props, "moveSpeed", 200.0f);
            pc.jumpForce = safeGetFloat(props, "jumpForce", 400.0f);
            sys.emplace<PlayerController>(e, pc);
        });

    blueprints_->registerComponent("SwordHitbox",
        [safeGetInt](jframe::Entity e, jframe::IEntitySystem& sys, const jframe::PropertyMap& props) {
            SwordHitbox sw;
            sw.damage = safeGetInt(props, "damage", 25);
            sys.emplace<SwordHitbox>(e, sw);
        });

    // NOTE: DebugRect and DebugCircle are registered by BlueprintFactory's built-in
    // registration, which correctly handles the `size = {w, h}` array format from Lua.
    // Do NOT re-register them here as it would overwrite the correct implementation.

    // Load blueprints from Lua file
    std::ifstream file("data/blueprints/entities.lua");
    if (!file) {
        jframe::core::logError("Could not open data/blueprints/entities.lua");
        return;
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string luaSource = buffer.str();

    if (blueprints_->loadBlueprints(luaSource)) {
        jframe::core::logInfo("Loaded " + std::to_string(blueprints_->getBlueprintNames().size()) + " blueprints");
    } else {
        jframe::core::logError("Failed to parse blueprints from entities.lua");
    }
}

void Game::createPlayer() {
    auto& sys = engine_->systems();

    // Get spawn point from level (fallback to center if not found)
    float spawnX = 400.0f;
    float spawnY = 200.0f;
    if (auto spawnPoint = sys.levels->getSpawnPoint(currentLevelId_, "player")) {
        spawnX = spawnPoint->x;
        spawnY = spawnPoint->y;
    }

    player_ = sys.entities->createEntity();
    sys.entities->emplace<PlayerTag>(player_);
    sys.entities->emplace<Size2D>(player_, Size2D{playerPhysicsWidth_, playerPhysicsHeight_});
    sys.entities->emplace<PlayerController>(player_, PlayerController{
        .moveSpeed = playerMoveSpeed_,
        .jumpForce = playerJumpForce_,
        .isGrounded = false
    });

    // Add Transform2D for rendering (position will be synced from physics)
    sys.entities->emplace<jframe::Transform2D>(player_, jframe::Transform2D{
        .x = spawnX,
        .y = spawnY,
        .rotation = 0.0f,
        .scaleX = 1.0f,
        .scaleY = 1.0f
    });

    // Add DebugRect for visual representation (green player)
    sys.entities->emplace<jframe::DebugRect>(player_, jframe::DebugRect{
        .size = {playerPhysicsWidth_, playerPhysicsHeight_},
        .fillColor = jframe::Color{50, 200, 50, 255},      // Green
        .outlineColor = jframe::Color{25, 100, 25, 255},   // Dark green outline
        .outlineWidth = 2.0f,
        .layer = 40,  // Player layer (above platforms)
        .filled = true
    });

    // Create physics body using the new PhysicsBodyBuilder
    jframe::physics::character(*sys.physics, player_, spawnX, spawnY,
                               playerPhysicsWidth_, playerPhysicsHeight_)
        .fixedRotation(config_->getBoolOr("physics.fixedRotation", true))
        .linearDamping(config_->getFloatOr("physics.linearDamping", 0.0f))
        .friction(config_->getFloatOr("physics.friction", 0.0f))  // Zero friction prevents wall sticking
        .layer(jframe::CollisionLayers::Player)
        .create();

    // Initialize GAS component and attributes using config values
    float initialHealth = config_->getFloatOr("attributes.health", 100.0f);
    float initialStamina = config_->getFloatOr("attributes.stamina", 100.0f);
    float initialSpeed = config_->getFloatOr("attributes.moveSpeed", 200.0f);

    gas_->initializeComponent(player_);
    gas_->initializeAttribute(player_, healthAttr_, initialHealth);
    gas_->initializeAttribute(player_, staminaAttr_, initialStamina);
    gas_->initializeAttribute(player_, moveSpeedAttr_, initialSpeed);

    jframe::core::logInfo("Player created at (" + std::to_string(spawnX) + ", " + std::to_string(spawnY) + ")");
}

void Game::loadLevel() {
    auto& sys = engine_->systems();

    // Get entity definitions from the Level System (parsed from level1.lua)
    auto entityDefs = sys.levels->getEntityDefs(currentLevelId_);

    // Helper to extract number from std::any (handles both int and double from Lua)
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

    // Helper to extract string from std::any (handles different string types from Lua)
    auto getString = [](const std::any& val, const std::string& defaultVal) -> std::string {
        try {
            return std::any_cast<std::string>(val);
        } catch (const std::bad_any_cast&) {
            try {
                return std::string(std::any_cast<const char*>(val));
            } catch (const std::bad_any_cast&) {
                return defaultVal;
            }
        }
    };

    // Map level entity types to blueprint names
    static const std::unordered_map<std::string, std::string> typeToBlueprint = {
        {"platform", "Platform"},
        {"jump_zone", "JumpZone"},
        {"collectable", "Collectable"},
        {"enemy_walker", "WalkerEnemy"},
        {"enemy_jumper", "JumperEnemy"},
        {"enemy_shooter", "ShooterEnemy"},
        {"enemy_flying", "FlyingEnemy"},
        {"switch", "Switch"},
        {"door", "Door"},
        {"breakable", "Breakable"},
        {"moving_platform", "MovingPlatform"},
        {"health_pickup", "HealthPickup"},
        {"checkpoint", "Checkpoint"},
        {"trigger_zone", "TriggerZone"},
        {"boss", "Boss"}
    };

    int entityCount = 0;
    for (const auto& def : entityDefs) {
        // Find blueprint name for this entity type
        auto it = typeToBlueprint.find(def.type);
        if (it == typeToBlueprint.end()) {
            jframe::core::logWarn("Unknown entity type: " + def.type);
            continue;
        }

        const std::string& blueprintName = it->second;

        // Extract width/height from properties
        float width = 100.0f;
        float height = 20.0f;

        if (auto propIt = def.properties.find("width"); propIt != def.properties.end()) {
            width = getNumber(propIt->second, 100.0f);
        }
        if (auto propIt = def.properties.find("height"); propIt != def.properties.end()) {
            height = getNumber(propIt->second, 20.0f);
        }

        // Create entity using blueprint factory
        jframe::Entity entity = blueprints_->create(blueprintName, def.transform.x, def.transform.y, width, height);

        if (!sys.entities->isValid(entity)) {
            jframe::core::logWarn("Failed to create entity from blueprint: " + blueprintName);
            continue;
        }

        // Apply entity-specific overrides from level properties
        if (def.type == "collectable") {
            std::string abilityName = "Dash";
            if (auto propIt = def.properties.find("ability"); propIt != def.properties.end()) {
                abilityName = getString(propIt->second, "Dash");
            }
            if (auto* tag = sys.entities->tryGet<CollectableTag>(entity)) {
                tag->abilityToGrant = abilityName;
            }
        }
        else if (def.type == "switch") {
            int targetDoorId = 1;
            if (auto propIt = def.properties.find("targetDoorId"); propIt != def.properties.end()) {
                targetDoorId = static_cast<int>(getNumber(propIt->second, 1.0f));
            }
            if (auto* tag = sys.entities->tryGet<SwitchTag>(entity)) {
                tag->targetDoorId = targetDoorId;
            }
        }
        else if (def.type == "door") {
            int doorId = 1;
            if (auto propIt = def.properties.find("doorId"); propIt != def.properties.end()) {
                doorId = static_cast<int>(getNumber(propIt->second, 1.0f));
            }
            if (auto* tag = sys.entities->tryGet<DoorTag>(entity)) {
                tag->doorId = doorId;
            }
        }
        else if (def.type == "moving_platform") {
            float startX = def.transform.x;
            float startY = def.transform.y;
            float endX = startX + getNumber(def.properties.count("moveX") ? def.properties.at("moveX") : std::any{}, 100.0f);
            float endY = startY + getNumber(def.properties.count("moveY") ? def.properties.at("moveY") : std::any{}, 0.0f);
            float speed = getNumber(def.properties.count("speed") ? def.properties.at("speed") : std::any{}, 50.0f);

            if (auto* mp = sys.entities->tryGet<MovingPlatform>(entity)) {
                mp->startX = startX;
                mp->startY = startY;
                mp->endX = endX;
                mp->endY = endY;
                mp->speed = speed;
            }
        }
        else if (def.type == "trigger_zone") {
            std::string event = "default";
            if (auto propIt = def.properties.find("event"); propIt != def.properties.end()) {
                event = getString(propIt->second, "default");
            }
            if (auto* tz = sys.entities->tryGet<TriggerZone>(entity)) {
                tz->event = event;
            }
        }

        // Set collision layer for platforms (so checkGrounded() works)
        if (def.type == "platform" || def.type == "moving_platform") {
            sys.physics->setCollisionLayer(entity, jframe::CollisionLayers::Ground);
        }

        entityCount++;
    }

    // Count entities by type using entity queries
    size_t platformCount = sys.entities->groupCount<PlatformTag>();
    size_t jumpZoneCount = sys.entities->groupCount<JumpZoneTag>();
    size_t collectableCount = sys.entities->groupCount<CollectableTag>();

    jframe::core::logInfo("Level loaded with " + std::to_string(platformCount) +
                          " platforms, " + std::to_string(jumpZoneCount) + " jump zones, and " +
                          std::to_string(collectableCount) + " collectables");
}

void Game::checkCollectablePickups() {
    auto& sys = engine_->systems();

    if (!sys.physics->hasBody(player_)) return;
    jframe::Vec2 playerPos = sys.physics->getPosition(player_);

    // Use entity query to find collectables (collect first for safe deletion)
    auto collectables = sys.entities->collect<CollectableTag>();
    for (jframe::Entity collectable : collectables) {
        if (!sys.physics->hasBody(collectable)) {
            continue;
        }

        jframe::Vec2 collectPos = sys.physics->getPosition(collectable);
        float dx = playerPos.x - collectPos.x;
        float dy = playerPos.y - collectPos.y;
        float distance = std::sqrt(dx * dx + dy * dy);

        // Pickup radius
        if (distance < 40.0f) {
            auto* collectableTag = sys.entities->tryGet<CollectableTag>(collectable);
            if (collectableTag) {
                // Find and grant the ability
                auto abilityDef = gas_->getAbilityDef(collectableTag->abilityToGrant);
                if (abilityDef) {
                    gas_->grantAbility(player_, abilityDef->id);
                    jframe::core::logInfo("Collected " + collectableTag->abilityToGrant + " ability!");
                    playSound("collectAbility");

                    if (collectableTag->abilityToGrant == "Dash") {
                        hasDashAbility_ = true;
                    }
                }
            }

            // Remove the collectable
            sys.physics->destroyBody(collectable);
            sys.entities->destroyEntity(collectable);
        }
    }
}

void Game::handlePlayerInput(jframe::DeltaTime dt) {
    auto& sys = engine_->systems();

    if (!sys.physics->hasBody(player_)) return;

    // Horizontal movement
    float moveInput = 0.0f;
    if (sys.input->isActionActive("move_left")) {
        moveInput -= 1.0f;
        facingDirection_ = -1.0f;  // Track facing left
    }
    if (sys.input->isActionActive("move_right")) {
        moveInput += 1.0f;
        facingDirection_ = 1.0f;  // Track facing right
    }

    // Get move speed from GAS attribute
    float moveSpeed = gas_->getAttributeValue(player_, moveSpeedAttr_);

    jframe::Vec2 velocity = sys.physics->getVelocity(player_);
    velocity.x = moveInput * moveSpeed;
    sys.physics->setVelocity(player_, velocity);

    // Dash ability
    if (sys.input->wasActionJustPressed("dash") && !isDashing_) {
        if (gas_->canActivateAbility(player_, dashAbility_)) {
            gas_->tryActivateAbility(player_, dashAbility_);

            // Start dash animation in facing direction
            jframe::Vec2 pos = sys.physics->getPosition(player_);
            dashStartX_ = pos.x;
            dashStartY_ = pos.y;  // Store Y to keep dash level
            dashEndX_ = pos.x + facingDirection_ * DASH_DISTANCE;
            dashProgress_ = 0.0f;
            isDashing_ = true;
            jframe::core::logInfo("Dash!");
            playSound("dash");

            // End the GAS ability immediately (cooldown starts now)
            gas_->endAbility(player_, dashAbility_);
        } else {
            float cooldown = gas_->getAbilityCooldown(player_, dashAbility_);
            if (cooldown > 0.0f) {
                jframe::core::logInfo("Dash on cooldown");
            } else if (!hasDashAbility_) {
                jframe::core::logInfo("Dash not unlocked - find the cyan collectable");
            } else {
                jframe::core::logInfo("Cannot dash (not enough stamina or stunned)");
            }
        }
    }

    // Update dash animation
    if (isDashing_) {
        dashProgress_ += dt / DASH_DURATION;
        if (dashProgress_ >= 1.0f) {
            dashProgress_ = 1.0f;
            isDashing_ = false;
        }

        // Lerp position horizontally, keep Y fixed (no gravity during dash)
        float currentX = dashStartX_ + (dashEndX_ - dashStartX_) * dashProgress_;
        sys.physics->setPosition(player_, {currentX, dashStartY_});
        sys.physics->setVelocity(player_, {0.0f, 0.0f});  // Zero velocity during dash
    }

    // Jump ability - use GAS system for activation checks (stamina, grounded, tags)
    if (sys.input->isActionActive("jump")) {
        if (gas_->canActivateAbility(player_, jumpAbility_)) {
            gas_->tryActivateAbility(player_, jumpAbility_);

            // Apply jump physics - negative Y velocity = up in our Y-down coordinate system
            jframe::Vec2 vel = sys.physics->getVelocity(player_);
            vel.y = -playerJumpForce_;
            sys.physics->setVelocity(player_, vel);

            gas_->endAbility(player_, jumpAbility_);
            playSound("jump");
        }
    }

    // Test: Apply health regen
    if (sys.input->wasActionJustPressed("apply_health_regen")) {
        gas_->applyEffect(player_, healthRegenEffect_, player_);
        jframe::core::logInfo("Health regen applied!");
    }

    // Test: Apply stun
    if (sys.input->wasActionJustPressed("apply_stun")) {
        gas_->applyEffect(player_, stunEffect_, player_);
        jframe::core::logInfo("Stunned!");
    }
}

void Game::updatePlayerMovement(jframe::DeltaTime dt) {
    auto& sys = engine_->systems();

    auto* controller = sys.entities->tryGet<PlayerController>(player_);
    if (!controller || !sys.physics->hasBody(player_)) return;

    // Simple ground detection
    jframe::Vec2 velocity = sys.physics->getVelocity(player_);
    jframe::Vec2 position = sys.physics->getPosition(player_);

    controller->isGrounded = (std::abs(velocity.y) < 1.0f && position.y > 500.0f);

    // Stamina regeneration (rate from config)
    float currentStamina = gas_->getAttributeValue(player_, staminaAttr_);
    if (currentStamina < 100.0f) {
        gas_->modifyAttribute(player_, staminaAttr_, staminaRegenRate_ * dt);
    }
}

void Game::updateCamera(jframe::DeltaTime dt) {
    auto& sys = engine_->systems();

    // Use the new camera system for smooth following
    if (!camera_ || !sys.entities->isValid(player_)) return;
    if (!sys.physics->hasBody(player_)) return;

    jframe::Vec2 targetPos = sys.physics->getPosition(player_);
    camera_->update(dt, targetPos);
}

void Game::renderUI() {
    auto& sys = engine_->systems();

    if (!sys.entities->isValid(player_)) return;

    // Get player attributes
    float health = gas_->getAttributeValue(player_, healthAttr_);
    float stamina = gas_->getAttributeValue(player_, staminaAttr_);

    // Reset camera to screen space for UI
    jframe::Camera uiCamera;
    uiCamera.transform.x = 400.0f;
    uiCamera.transform.y = 300.0f;
    uiCamera.zoom = 1.0f;
    uiCamera.viewportSize = {800, 600};
    sys.graphics->setCamera(uiCamera);

    // Health bar
    int healthBarWidth = static_cast<int>((health / 100.0f) * 200.0f);
    sys.graphics->drawRect({10, 10, 200, 20}, jframe::Color{50, 50, 50, 255}, true);
    sys.graphics->drawRect({10, 10, healthBarWidth, 20}, jframe::Color{255, 0, 0, 255}, true);
    sys.graphics->drawRect({10, 10, 200, 20}, jframe::Color::white(), false);

    // Stamina bar
    int staminaBarWidth = static_cast<int>((stamina / 100.0f) * 200.0f);
    sys.graphics->drawRect({10, 35, 200, 20}, jframe::Color{50, 50, 50, 255}, true);
    sys.graphics->drawRect({10, 35, staminaBarWidth, 20}, jframe::Color{0, 255, 255, 255}, true);
    sys.graphics->drawRect({10, 35, 200, 20}, jframe::Color::white(), false);

    // Cooldown indicators
    float dashCooldown = gas_->getAbilityCooldown(player_, dashAbility_);
    if (dashCooldown > 0.0f) {
        int cooldownHeight = static_cast<int>((dashCooldown / 2.0f) * 50.0f);
        sys.graphics->drawRect({10, 60, 50, 50}, jframe::Color{100, 100, 100, 255}, true);
        sys.graphics->drawRect({10, 60, 50, cooldownHeight}, jframe::Color{255, 0, 0, 128}, true);
    } else {
        sys.graphics->drawRect({10, 60, 50, 50}, jframe::Color{0, 255, 0, 255}, true);
    }
    sys.graphics->drawRect({10, 60, 50, 50}, jframe::Color::white(), false);

    // Active effects indicator
    auto activeEffects = gas_->getActiveEffects(player_);
    int effectY = 120;
    for (const auto& effect : activeEffects) {
        auto effectDef = gas_->getEffectDef(effect.defId);
        if (effectDef) {
            jframe::Color effectColor = jframe::Color{128, 128, 255, 255};
            if (effectDef->name == "DashSpeedBoost") {
                effectColor = jframe::Color{0, 255, 255, 255};
            } else if (effectDef->name == "HealthRegen") {
                effectColor = jframe::Color{0, 255, 0, 255};
            } else if (effectDef->name == "Stun") {
                effectColor = jframe::Color{128, 0, 128, 255};
            }

            sys.graphics->drawRect({10, effectY, 100, 20}, effectColor, true);
            sys.graphics->drawRect({10, effectY, 100, 20}, jframe::Color::white(), false);

            effectY += 25;
        }
    }
}

void Game::updateSwitches() {
    auto& sys = engine_->systems();
    if (!sys.physics->hasBody(player_)) return;

    jframe::Vec2 playerPos = sys.physics->getPosition(player_);

    // Use entity query to iterate switches
    for (jframe::Entity sw : sys.entities->view<SwitchTag>()) {
        auto* switchTag = sys.entities->tryGet<SwitchTag>(sw);
        if (!switchTag || switchTag->activated) continue;
        if (!sys.physics->hasBody(sw)) continue;

        jframe::Vec2 swPos = sys.physics->getPosition(sw);
        float dx = playerPos.x - swPos.x;
        float dy = playerPos.y - swPos.y;
        float dist = std::sqrt(dx * dx + dy * dy);

        if (dist < 30.0f) {
            switchTag->activated = true;
            playSound("switchActivate");

            // Find and open the door using entity query
            for (jframe::Entity door : sys.entities->view<DoorTag>()) {
                auto* doorTag = sys.entities->tryGet<DoorTag>(door);
                if (doorTag && doorTag->doorId == switchTag->targetDoorId) {
                    doorTag->isOpen = true;
                    // Remove physics body to allow passage
                    if (sys.physics->hasBody(door)) {
                        sys.physics->destroyBody(door);
                    }
                    playSound("doorOpen");
                    jframe::core::logInfo("Door " + std::to_string(doorTag->doorId) + " opened!");
                }
            }
            jframe::core::logInfo("Switch activated!");
        }
    }
}

void Game::updateDoors() {
    // Doors are handled in updateSwitches
}

void Game::updateMovingPlatforms(jframe::DeltaTime dt) {
    auto& sys = engine_->systems();

    // Use entity query to iterate moving platforms
    for (jframe::Entity plat : sys.entities->view<MovingPlatform>()) {
        auto* moving = sys.entities->tryGet<MovingPlatform>(plat);
        if (!moving || !sys.physics->hasBody(plat)) continue;

        // Update progress
        float moveSpeed = moving->speed * dt / 100.0f;  // Normalize
        if (moving->forward) {
            moving->progress += moveSpeed;
            if (moving->progress >= 1.0f) {
                moving->progress = 1.0f;
                moving->forward = false;
            }
        } else {
            moving->progress -= moveSpeed;
            if (moving->progress <= 0.0f) {
                moving->progress = 0.0f;
                moving->forward = true;
            }
        }

        // Interpolate position
        float newX = moving->startX + (moving->endX - moving->startX) * moving->progress;
        float newY = moving->startY + (moving->endY - moving->startY) * moving->progress;
        sys.physics->setPosition(plat, {newX, newY});
    }
}

void Game::updateProjectiles(jframe::DeltaTime dt) {
    auto& sys = engine_->systems();

    // Collect projectiles first for safe deletion during iteration
    auto projectiles = sys.entities->collect<Projectile>();
    for (jframe::Entity proj : projectiles) {
        auto* projData = sys.entities->tryGet<Projectile>(proj);

        if (!projData || !sys.physics->hasBody(proj)) {
            continue;
        }

        // Update lifetime
        projData->lifetime -= dt;
        if (projData->lifetime <= 0.0f) {
            sys.physics->destroyBody(proj);
            sys.entities->destroyEntity(proj);
            continue;
        }

        // Move projectile
        jframe::Vec2 pos = sys.physics->getPosition(proj);
        pos.x += projData->velocityX * dt;
        pos.y += projData->velocityY * dt;
        sys.physics->setPosition(proj, pos);

        // Check collisions
        bool shouldDestroy = false;
        if (projData->isEnemyProjectile) {
            // Check player collision
            if (sys.physics->hasBody(player_)) {
                jframe::Vec2 playerPos = sys.physics->getPosition(player_);
                float dx = pos.x - playerPos.x;
                float dy = pos.y - playerPos.y;
                if (std::sqrt(dx * dx + dy * dy) < 25.0f) {
                    playerTakeDamage(projData->damage);
                    shouldDestroy = true;
                }
            }
        } else {
            // Check enemy collisions using entity query
            for (jframe::Entity enemy : sys.entities->view<EnemyTag>()) {
                if (!sys.physics->hasBody(enemy)) continue;
                jframe::Vec2 enemyPos = sys.physics->getPosition(enemy);
                float dx = pos.x - enemyPos.x;
                float dy = pos.y - enemyPos.y;
                if (std::sqrt(dx * dx + dy * dy) < 25.0f) {
                    enemyTakeDamage(enemy, projData->damage);
                    shouldDestroy = true;
                    break;
                }
            }
        }

        if (shouldDestroy) {
            sys.physics->destroyBody(proj);
            sys.entities->destroyEntity(proj);
        }
    }
}

void Game::checkTriggerZones() {
    auto& sys = engine_->systems();
    if (!sys.physics->hasBody(player_)) return;

    jframe::Vec2 playerPos = sys.physics->getPosition(player_);

    // Use entity query to iterate trigger zones
    for (jframe::Entity trigger : sys.entities->view<TriggerZone>()) {
        auto* zone = sys.entities->tryGet<TriggerZone>(trigger);
        if (!zone || zone->triggered) continue;
        if (!sys.physics->hasBody(trigger)) continue;

        jframe::Vec2 triggerPos = sys.physics->getPosition(trigger);
        auto* size = sys.entities->tryGet<Size2D>(trigger);
        if (!size) continue;

        float halfW = size->width / 2.0f;
        float halfH = size->height / 2.0f;

        if (playerPos.x >= triggerPos.x - halfW && playerPos.x <= triggerPos.x + halfW &&
            playerPos.y >= triggerPos.y - halfH && playerPos.y <= triggerPos.y + halfH) {

            zone->triggered = true;

            if (zone->event == "boss_start") {
                bossActive_ = true;
                playSound("bossRoar");
                playMusic("musicBoss");  // Switch to boss music
                jframe::core::logInfo("BOSS FIGHT STARTED!");
            } else if (zone->event == "victory") {
                jframe::core::logInfo("VICTORY! You beat the game!");
            }

            jframe::core::logInfo("Trigger: " + zone->event);
        }
    }
}

void Game::checkHealthPickups() {
    auto& sys = engine_->systems();
    if (!sys.physics->hasBody(player_)) return;

    jframe::Vec2 playerPos = sys.physics->getPosition(player_);

    // Collect health pickups first for safe deletion during iteration
    auto healthPickups = sys.entities->collect<HealthPickup>();
    for (jframe::Entity pickup : healthPickups) {
        auto* hp = sys.entities->tryGet<HealthPickup>(pickup);
        if (!hp || !sys.physics->hasBody(pickup)) {
            continue;
        }

        jframe::Vec2 pickupPos = sys.physics->getPosition(pickup);
        float dx = playerPos.x - pickupPos.x;
        float dy = playerPos.y - pickupPos.y;

        if (std::sqrt(dx * dx + dy * dy) < 30.0f) {
            playerHealth_ = std::min(100, playerHealth_ + hp->healAmount);
            sys.physics->destroyBody(pickup);
            sys.entities->destroyEntity(pickup);
            playSound("collectHealth");
            jframe::core::logInfo("Health restored! Health: " + std::to_string(playerHealth_));
        }
    }
}

void Game::checkCheckpoints() {
    auto& sys = engine_->systems();
    if (!sys.physics->hasBody(player_)) return;

    jframe::Vec2 playerPos = sys.physics->getPosition(player_);

    // Use entity query to iterate checkpoints
    for (jframe::Entity cp : sys.entities->view<Checkpoint>()) {
        auto* checkpoint = sys.entities->tryGet<Checkpoint>(cp);
        if (!checkpoint || checkpoint->activated) continue;
        if (!sys.physics->hasBody(cp)) continue;

        jframe::Vec2 cpPos = sys.physics->getPosition(cp);
        float dx = playerPos.x - cpPos.x;
        float dy = playerPos.y - cpPos.y;

        if (std::sqrt(dx * dx + dy * dy) < 40.0f) {
            checkpoint->activated = true;
            lastCheckpoint_ = cpPos;
            playSound("checkpoint");
            jframe::core::logInfo("Checkpoint activated at (" + std::to_string(cpPos.x) + ", " + std::to_string(cpPos.y) + ")");
        }
    }
}

void Game::updateGroundCheck() {
    auto& sys = engine_->systems();
    if (!sys.physics->hasBody(player_)) return;

    auto result = sys.physics->checkGrounded(player_);
    isGrounded_ = result.grounded;

    if (isGrounded_) {
        isInAir_ = false;
        hasUsedDoubleJump_ = false;
        airJumpCount_ = 0;

        // Remove InAir tag, add grounded
        auto inAirTag = gas_->findTag("State.InAir");
        if (inAirTag && gas_->hasTag(player_, *inAirTag)) {
            gas_->removeTag(player_, *inAirTag);
        }
    } else {
        isInAir_ = true;
        auto inAirTag = gas_->findTag("State.InAir");
        if (inAirTag && !gas_->hasTag(player_, *inAirTag)) {
            gas_->addTag(player_, *inAirTag);
        }
    }
}

void Game::updateWallCheck() {
    auto& sys = engine_->systems();
    if (!sys.physics->hasBody(player_)) return;

    jframe::Vec2 pos = sys.physics->getPosition(player_);

    // Simple wall check: raycast in facing direction
    auto hit = sys.physics->raycast(pos, {facingDirection_, 0.0f}, 20.0f);
    isTouchingWall_ = hit.has_value();

    auto wallTag = gas_->findTag("State.TouchingWall");
    if (wallTag) {
        if (isTouchingWall_ && !isGrounded_) {
            if (!gas_->hasTag(player_, *wallTag)) {
                gas_->addTag(player_, *wallTag);
            }
        } else {
            if (gas_->hasTag(player_, *wallTag)) {
                gas_->removeTag(player_, *wallTag);
            }
        }
    }
}

void Game::handleDoubleJump() {
    auto& sys = engine_->systems();

    if (!hasDoubleJumpAbility_ || !isInAir_ || hasUsedDoubleJump_) return;

    if (sys.input->wasActionJustPressed("jump")) {
        if (gas_->canActivateAbility(player_, doubleJumpAbility_)) {
            gas_->tryActivateAbility(player_, doubleJumpAbility_);

            jframe::Vec2 vel = sys.physics->getVelocity(player_);
            vel.y = -350.0f;  // Slightly weaker than normal jump
            sys.physics->setVelocity(player_, vel);

            hasUsedDoubleJump_ = true;
            playSound("doubleJump");
            jframe::core::logInfo("Double Jump!");
            gas_->endAbility(player_, doubleJumpAbility_);
        }
    }
}

void Game::handleWallJump() {
    auto& sys = engine_->systems();

    if (!hasWallJumpAbility_ || !isTouchingWall_ || isGrounded_) return;

    if (sys.input->wasActionJustPressed("jump")) {
        if (gas_->canActivateAbility(player_, wallJumpAbility_)) {
            gas_->tryActivateAbility(player_, wallJumpAbility_);

            jframe::Vec2 vel = sys.physics->getVelocity(player_);
            vel.y = -350.0f;
            vel.x = -facingDirection_ * 200.0f;  // Push away from wall
            sys.physics->setVelocity(player_, vel);

            facingDirection_ = -facingDirection_;  // Turn around
            playSound("wallJump");
            jframe::core::logInfo("Wall Jump!");
            gas_->endAbility(player_, wallJumpAbility_);
        }
    }
}

void Game::handleGroundPound(jframe::DeltaTime dt) {
    auto& sys = engine_->systems();

    if (!hasGroundPoundAbility_) return;

    // Start ground pound
    if (sys.input->wasActionJustPressed("ground_pound") && isInAir_ && !isGroundPounding_) {
        if (gas_->canActivateAbility(player_, groundPoundAbility_)) {
            gas_->tryActivateAbility(player_, groundPoundAbility_);
            isGroundPounding_ = true;

            jframe::Vec2 vel = sys.physics->getVelocity(player_);
            vel.x = 0.0f;
            vel.y = groundPoundVelocity_;
            sys.physics->setVelocity(player_, vel);

            playSound("groundPound");
            jframe::core::logInfo("Ground Pound!");
            gas_->endAbility(player_, groundPoundAbility_);
        }
    }

    // Check for ground pound landing
    if (isGroundPounding_ && isGrounded_) {
        isGroundPounding_ = false;

        // Damage nearby enemies using entity query
        jframe::Vec2 playerPos = sys.physics->getPosition(player_);
        for (jframe::Entity enemy : sys.entities->view<EnemyTag>()) {
            if (!sys.physics->hasBody(enemy)) continue;
            jframe::Vec2 enemyPos = sys.physics->getPosition(enemy);
            float dx = playerPos.x - enemyPos.x;
            float dy = playerPos.y - enemyPos.y;
            if (std::sqrt(dx * dx + dy * dy) < 100.0f) {
                enemyTakeDamage(enemy, 30);
            }
        }

        // Break breakables - collect first for safe deletion
        auto breakables = sys.entities->collect<BreakableTag>();
        for (jframe::Entity br : breakables) {
            if (!sys.physics->hasBody(br)) continue;
            jframe::Vec2 brPos = sys.physics->getPosition(br);
            float dx = playerPos.x - brPos.x;
            float dy = playerPos.y - brPos.y;
            if (std::sqrt(dx * dx + dy * dy) < 80.0f) {
                sys.physics->destroyBody(br);
                sys.entities->destroyEntity(br);
                playSound("breakableDestroy");
                jframe::core::logInfo("Breakable destroyed!");
            }
        }

        jframe::core::logInfo("Ground Pound impact!");
    }
}

void Game::createProjectile(float x, float y, float dirX, float dirY, bool isEnemy) {
    auto& sys = engine_->systems();

    // Use blueprint factory to create projectile
    std::string blueprintName = isEnemy ? "EnemyProjectile" : "PlayerProjectile";
    jframe::Entity proj = blueprints_->create(blueprintName, x, y, 12.0f, 12.0f);

    if (!sys.entities->isValid(proj)) {
        jframe::core::logWarn("Failed to create projectile");
        return;
    }

    // Set projectile velocity
    float speed = 300.0f;
    if (auto* projData = sys.entities->tryGet<Projectile>(proj)) {
        projData->velocityX = dirX * speed;
        projData->velocityY = dirY * speed;
        projData->isEnemyProjectile = isEnemy;
    }
}

void Game::handleCombat(jframe::DeltaTime dt) {
    auto& sys = engine_->systems();

    // Update attack timer
    if (isAttacking_) {
        attackTimer_ -= dt;
        if (attackTimer_ <= 0.0f) {
            isAttacking_ = false;
            swordHitboxActive_ = false;
        }
    }

    // Update combo timer
    if (comboTimer_ > 0.0f) {
        comboTimer_ -= dt;
        if (comboTimer_ <= 0.0f) {
            comboCount_ = 0;
        }
    }

    // Update invincibility
    if (invincibilityTimer_ > 0.0f) {
        invincibilityTimer_ -= dt;
    }

    // Sword attack input
    if (sys.input->wasActionJustPressed("attack") && !isAttacking_ && hasSwordAbility_) {
        // Determine which attack based on combo
        jframe::AbilityId attackAbility = swordAttackAbility_;
        if (comboCount_ == 1 && comboTimer_ > 0.0f) {
            attackAbility = swordCombo2Ability_;
        } else if (comboCount_ == 2 && comboTimer_ > 0.0f) {
            attackAbility = swordCombo3Ability_;
        }

        if (gas_->canActivateAbility(player_, attackAbility)) {
            gas_->tryActivateAbility(player_, attackAbility);
            isAttacking_ = true;
            attackTimer_ = ATTACK_DURATION;
            swordHitboxActive_ = true;
            comboCount_ = (comboCount_ + 1) % 4;
            if (comboCount_ == 0) comboCount_ = 1;
            comboTimer_ = COMBO_WINDOW;

            updateSwordHitbox();
            // Play combo-appropriate sword sound
            if (comboCount_ == 1) playSound("swordSwing1");
            else if (comboCount_ == 2) playSound("swordSwing2");
            else playSound("swordSwing3");
            jframe::core::logInfo("Sword attack! Combo: " + std::to_string(comboCount_));
            gas_->endAbility(player_, attackAbility);
        }
    }

    // Shield input
    if (sys.input->wasActionJustPressed("shield") && hasShieldAbility_) {
        if (gas_->canActivateAbility(player_, shieldAbility_)) {
            gas_->tryActivateAbility(player_, shieldAbility_);
            jframe::core::logInfo("Shield activated!");
            gas_->endAbility(player_, shieldAbility_);
        }
    }

    // Ranged attack input
    if (sys.input->wasActionJustPressed("ranged") && hasRangedAbility_) {
        if (gas_->canActivateAbility(player_, rangedAbility_)) {
            gas_->tryActivateAbility(player_, rangedAbility_);
            jframe::Vec2 pos = sys.physics->getPosition(player_);
            createProjectile(pos.x + facingDirection_ * 30.0f, pos.y, facingDirection_, 0.0f, false);
            playSound("projectileFire");
            jframe::core::logInfo("Ranged attack!");
            gas_->endAbility(player_, rangedAbility_);
        }
    }

    // Check sword collisions
    if (swordHitboxActive_) {
        checkSwordCollisions();
    }
}

void Game::updateSwordHitbox() {
    auto& sys = engine_->systems();
    if (!sys.physics->hasBody(player_)) return;

    jframe::Vec2 playerPos = sys.physics->getPosition(player_);
    float hitboxX = playerPos.x + facingDirection_ * 40.0f;
    float hitboxY = playerPos.y;

    // Store hitbox position for collision checking
    // (In a full implementation, you'd create an actual physics body)
}

void Game::checkSwordCollisions() {
    auto& sys = engine_->systems();
    if (!sys.physics->hasBody(player_)) return;

    jframe::Vec2 playerPos = sys.physics->getPosition(player_);
    float hitboxX = playerPos.x + facingDirection_ * 40.0f;

    int damage = 10 * (comboCount_ == 3 ? 2 : comboCount_ == 2 ? 1.5f : 1);

    // Check enemies using entity query
    for (jframe::Entity enemy : sys.entities->view<EnemyTag>()) {
        if (!sys.physics->hasBody(enemy)) continue;

        // Skip boss - handle separately
        if (sys.entities->tryGet<BossTag>(enemy)) continue;

        jframe::Vec2 enemyPos = sys.physics->getPosition(enemy);
        float dx = hitboxX - enemyPos.x;
        float dy = playerPos.y - enemyPos.y;
        float dist = std::sqrt(dx * dx + dy * dy);

        if (dist < 50.0f) {
            enemyTakeDamage(enemy, damage);
        }
    }

    // Check boss using entity query
    if (bossActive_) {
        auto bossOpt = sys.entities->first<BossTag>();
        if (bossOpt && sys.physics->hasBody(*bossOpt)) {
            jframe::Vec2 bossPos = sys.physics->getPosition(*bossOpt);
            float dx = hitboxX - bossPos.x;
            float dy = playerPos.y - bossPos.y;
            float dist = std::sqrt(dx * dx + dy * dy);

            if (dist < 80.0f) {
                bossTakeDamage(damage);
            }
        }
    }
}

void Game::playerTakeDamage(int amount) {
    if (invincibilityTimer_ > 0.0f) return;

    // Check for shield
    auto shieldTag = gas_->findTag("State.Shielded");
    if (shieldTag && gas_->hasTag(player_, *shieldTag)) {
        amount /= 2;  // Shield reduces damage
        playSound("shieldBlock");
        jframe::core::logInfo("Shield blocked some damage!");
    }

    playerHealth_ -= amount;
    invincibilityTimer_ = INVINCIBILITY_DURATION;
    playSound("playerHurt");

    jframe::core::logInfo("Player took " + std::to_string(amount) + " damage! Health: " + std::to_string(playerHealth_));

    if (playerHealth_ <= 0) {
        // Respawn at checkpoint
        playerHealth_ = 100;
        auto& sys = engine_->systems();
        sys.physics->setPosition(player_, lastCheckpoint_);
        sys.physics->setVelocity(player_, {0.0f, 0.0f});
        jframe::core::logInfo("Player died! Respawning at checkpoint...");
    }
}

void Game::enemyTakeDamage(jframe::Entity enemy, int amount) {
    auto& sys = engine_->systems();
    auto* enemyTag = sys.entities->tryGet<EnemyTag>(enemy);
    if (!enemyTag) return;

    enemyTag->health -= amount;
    playSound("enemyHit");
    jframe::core::logInfo("Enemy took " + std::to_string(amount) + " damage! Health: " + std::to_string(enemyTag->health));

    if (enemyTag->health <= 0) {
        playSound("enemyDeath");
        sys.physics->destroyBody(enemy);
        sys.entities->destroyEntity(enemy);
        // No need to track in vector - entity query handles it
        enemiesDefeated_++;
        jframe::core::logInfo("Enemy defeated! Total: " + std::to_string(enemiesDefeated_));
    }
}

void Game::bossTakeDamage(int amount) {
    bossHealth_ -= amount;
    playSound("bossHurt");
    jframe::core::logInfo("Boss took " + std::to_string(amount) + " damage! Health: " + std::to_string(bossHealth_));

    // Phase transitions
    if (bossHealth_ <= 350 && bossPhase_ == 1) {
        bossPhase_ = 2;
        jframe::core::logInfo("Boss entering phase 2!");
    } else if (bossHealth_ <= 150 && bossPhase_ == 2) {
        bossPhase_ = 3;
        jframe::core::logInfo("Boss entering phase 3!");
    }

    if (bossHealth_ <= 0) {
        auto& sys = engine_->systems();
        // Find and destroy boss using entity query
        auto bossOpt = sys.entities->first<BossTag>();
        if (bossOpt) {
            sys.physics->destroyBody(*bossOpt);
            sys.entities->destroyEntity(*bossOpt);
        }
        bossActive_ = false;
        playSound("bossDefeat");
        playMusic("musicExploration");  // Return to exploration music
        jframe::core::logInfo("BOSS DEFEATED! Victory!");
    }
}

void Game::updateEnemies(jframe::DeltaTime dt) {
    auto& sys = engine_->systems();

    // Use entity query to iterate enemies (excluding boss)
    for (jframe::Entity enemy : sys.entities->view<EnemyTag>()) {
        if (!sys.entities->isValid(enemy) || !sys.physics->hasBody(enemy)) continue;
        // Skip boss entity - handled separately in updateBoss()
        if (sys.entities->tryGet<BossTag>(enemy)) continue;
        updateEnemy(enemy, dt);
    }
}

void Game::updateEnemy(jframe::Entity enemy, jframe::DeltaTime dt) {
    auto& sys = engine_->systems();

    auto* enemyTag = sys.entities->tryGet<EnemyTag>(enemy);
    if (!enemyTag) return;

    jframe::Vec2 enemyPos = sys.physics->getPosition(enemy);
    jframe::Vec2 playerPos = sys.physics->getPosition(player_);

    float dx = playerPos.x - enemyPos.x;
    float dy = playerPos.y - enemyPos.y;
    float dist = std::sqrt(dx * dx + dy * dy);

    // Check if player is in range
    bool playerInRange = dist < enemyTag->detectionRange;

    // Contact damage
    if (dist < 30.0f) {
        playerTakeDamage(enemyTag->damage);
    }

    if (enemyTag->type == "walker") {
        auto* walker = sys.entities->tryGet<WalkerAI>(enemy);
        if (walker) {
            jframe::Vec2 vel = sys.physics->getVelocity(enemy);
            if (walker->movingRight) {
                vel.x = walker->speed;
                if (enemyPos.x >= walker->patrolRight) walker->movingRight = false;
            } else {
                vel.x = -walker->speed;
                if (enemyPos.x <= walker->patrolLeft) walker->movingRight = true;
            }
            sys.physics->setVelocity(enemy, vel);
        }
    } else if (enemyTag->type == "jumper") {
        auto* jumper = sys.entities->tryGet<JumperAI>(enemy);
        if (jumper && playerInRange) {
            jumper->jumpTimer -= dt;
            if (jumper->jumpTimer <= 0.0f) {
                jframe::Vec2 vel = sys.physics->getVelocity(enemy);
                vel.y = -jumper->jumpForce;
                vel.x = (dx > 0 ? 1 : -1) * 100.0f;
                sys.physics->setVelocity(enemy, vel);
                jumper->jumpTimer = jumper->jumpCooldown;
            }
        }
    } else if (enemyTag->type == "shooter") {
        auto* shooter = sys.entities->tryGet<ShooterAI>(enemy);
        if (shooter && playerInRange) {
            shooter->fireTimer -= dt;
            if (shooter->fireTimer <= 0.0f) {
                float dirX = dx / dist;
                float dirY = dy / dist;
                createProjectile(enemyPos.x, enemyPos.y, dirX, dirY, true);
                shooter->fireTimer = 1.0f / shooter->fireRate;
            }
        }
    } else if (enemyTag->type == "flying") {
        auto* flying = sys.entities->tryGet<FlyingAI>(enemy);
        if (flying) {
            if (flying->movingUp) {
                enemyPos.y -= flying->speed * dt;
                if (enemyPos.y <= flying->topY) flying->movingUp = false;
            } else {
                enemyPos.y += flying->speed * dt;
                if (enemyPos.y >= flying->bottomY) flying->movingUp = true;
            }
            sys.physics->setPosition(enemy, enemyPos);
        }
    }
}

void Game::updateBoss(jframe::DeltaTime dt) {
    if (!bossActive_) return;
    auto& sys = engine_->systems();

    // Find boss using entity query
    auto bossOpt = sys.entities->first<BossTag>();
    if (!bossOpt || !sys.physics->hasBody(*bossOpt)) return;
    jframe::Entity boss = *bossOpt;

    bossAttackTimer_ -= dt;
    bossPatternTimer_ -= dt;

    jframe::Vec2 bossPos = sys.physics->getPosition(boss);
    jframe::Vec2 playerPos = sys.physics->getPosition(player_);

    // Contact damage
    float dx = playerPos.x - bossPos.x;
    float dy = playerPos.y - bossPos.y;
    float dist = std::sqrt(dx * dx + dy * dy);
    if (dist < 60.0f) {
        playerTakeDamage(25);
    }

    // Attack patterns based on phase
    if (bossAttackTimer_ <= 0.0f) {
        float cooldown = 2.0f / bossPhase_;  // Faster in later phases

        int pattern = static_cast<int>(bossPatternTimer_ * 10) % 3;
        if (pattern == 0) {
            bossPatternAttack();
        } else if (pattern == 1) {
            bossPatternJump();
        } else if (bossPhase_ >= 2) {
            bossPatternSummon();
        }

        bossAttackTimer_ = cooldown;
    }

    // Move toward player
    jframe::Vec2 vel = sys.physics->getVelocity(boss);
    vel.x = (dx > 0 ? 1 : -1) * 50.0f * bossPhase_;
    sys.physics->setVelocity(boss, vel);
}

void Game::bossPatternAttack() {
    auto& sys = engine_->systems();

    // Find boss using entity query
    auto bossOpt = sys.entities->first<BossTag>();
    if (!bossOpt || !sys.physics->hasBody(*bossOpt)) return;
    jframe::Vec2 bossPos = sys.physics->getPosition(*bossOpt);

    // Fire projectiles in multiple directions
    int numProjectiles = 3 + bossPhase_;
    playSound("bossAttack");
    for (int i = 0; i < numProjectiles; i++) {
        float angle = (static_cast<float>(i) / numProjectiles) * 3.14159f * 2.0f;
        createProjectile(bossPos.x, bossPos.y, std::cos(angle), std::sin(angle), true);
    }
    jframe::core::logInfo("Boss attack pattern!");
}

void Game::bossPatternJump() {
    auto& sys = engine_->systems();

    // Find boss using entity query
    auto bossOpt = sys.entities->first<BossTag>();
    if (!bossOpt || !sys.physics->hasBody(*bossOpt)) return;

    jframe::Vec2 vel = sys.physics->getVelocity(*bossOpt);
    vel.y = -500.0f;
    sys.physics->setVelocity(*bossOpt, vel);
    jframe::core::logInfo("Boss jump!");
}

void Game::bossPatternSummon() {
    auto& sys = engine_->systems();

    // Find boss using entity query
    auto bossOpt = sys.entities->first<BossTag>();
    if (!bossOpt || !sys.physics->hasBody(*bossOpt)) return;
    jframe::Vec2 bossPos = sys.physics->getPosition(*bossOpt);

    // Summon minions using blueprint factory
    blueprints_->create("WalkerEnemy", bossPos.x - 100.0f, bossPos.y, 32.0f, 32.0f);
    blueprints_->create("WalkerEnemy", bossPos.x + 100.0f, bossPos.y, 32.0f, 32.0f);
    jframe::core::logInfo("Boss summoned minions!");
}

}  // namespace abilitydemo

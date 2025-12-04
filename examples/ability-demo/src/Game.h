// examples/ability-demo/src/Game.h
// Gameplay Ability System demonstration game

#pragma once

import std;
import bestow;
import bestow.core;
import bestow.gas;
import bestow.camera;
import bestow.blueprints;
import bestow.config.impl;

namespace abilitydemo {

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
    void setupGAS();
    void setupBlueprints();
    void createPlayer();
    void loadLevel();
    void checkCollectablePickups();

    void handlePlayerInput(bestow::DeltaTime dt);
    void updatePlayerMovement(bestow::DeltaTime dt);
    void updateCamera(bestow::DeltaTime dt);
    void renderUI();

    // Entity creation via blueprints (simplified)
    void createProjectile(float x, float y, float dirX, float dirY, bool isEnemy);

    // Combat
    void handleCombat(bestow::DeltaTime dt);
    void updateSwordHitbox();
    void checkSwordCollisions();
    void playerTakeDamage(int amount);
    void enemyTakeDamage(bestow::Entity enemy, int amount);
    void bossTakeDamage(int amount);

    // Enemy AI
    void updateEnemies(bestow::DeltaTime dt);
    void updateEnemy(bestow::Entity enemy, bestow::DeltaTime dt);
    void updateBoss(bestow::DeltaTime dt);

    // Level mechanics
    void updateSwitches();
    void updateDoors();
    void updateMovingPlatforms(bestow::DeltaTime dt);
    void updateProjectiles(bestow::DeltaTime dt);
    void checkTriggerZones();
    void checkHealthPickups();
    void checkCheckpoints();

    // Advanced movement
    void updateGroundCheck();
    void updateWallCheck();
    void handleDoubleJump();
    void handleWallJump();
    void handleGroundPound(bestow::DeltaTime dt);

    // Boss patterns
    void bossPatternAttack();
    void bossPatternJump();
    void bossPatternSummon();

    // Audio
    void setupAudio();
    void playSound(const std::string& soundName);
    void playMusic(const std::string& musicName);
    void stopMusic();

    bestow::core::Engine* engine_ = nullptr;

    // Config system
    std::unique_ptr<bestow::IConfigSystem> config_;

    // GAS system (created separately, not part of BestowEngine)
    std::unique_ptr<bestow::IGASSystem> gas_;

    // Blueprint factory for data-driven entity creation
    std::unique_ptr<bestow::IBlueprintFactory> blueprints_;

    // Camera system for smooth following
    std::unique_ptr<bestow::ICameraSystem> camera_;

    bestow::Entity player_;

    // Level tracking
    bestow::LevelId currentLevelId_{};
    bestow::AssetHandle levelAssetHandle_;

    // NOTE: Entity vectors removed - using Entity Query System instead:
    // - Use sys.entities->collect<PlatformTag>() for platforms
    // - Use sys.entities->collect<EnemyTag>() for enemies
    // - Use sys.entities->first<BossTag>() for boss
    // - etc.

    // GAS attribute IDs
    bestow::AttributeId healthAttr_ = 0;
    bestow::AttributeId staminaAttr_ = 0;
    bestow::AttributeId moveSpeedAttr_ = 0;

    // GAS ability IDs
    bestow::AbilityId dashAbility_ = 0;
    bestow::AbilityId jumpAbility_ = 0;
    bestow::AbilityId doubleJumpAbility_ = 0;
    bestow::AbilityId wallJumpAbility_ = 0;
    bestow::AbilityId groundPoundAbility_ = 0;
    bestow::AbilityId swordAttackAbility_ = 0;
    bestow::AbilityId swordCombo2Ability_ = 0;
    bestow::AbilityId swordCombo3Ability_ = 0;
    bestow::AbilityId shieldAbility_ = 0;
    bestow::AbilityId rangedAbility_ = 0;

    // GAS effect IDs
    bestow::EffectId healthRegenEffect_ = 0;
    bestow::EffectId stunEffect_ = 0;
    bestow::EffectId shieldEffect_ = 0;
    bestow::EffectId invincibilityEffect_ = 0;
    bestow::EffectId comboWindow1Effect_ = 0;
    bestow::EffectId comboWindow2Effect_ = 0;

    // Config-driven player values
    float playerMoveSpeed_ = 200.0f;
    float playerJumpForce_ = 400.0f;
    float playerPhysicsWidth_ = 30.0f;
    float playerPhysicsHeight_ = 50.0f;
    float staminaRegenRate_ = 20.0f;

    // Test state
    float testTimer_ = 0.0f;
    bool inJumpZone_ = false;
    bool hasDashAbility_ = false;
    float facingDirection_ = 1.0f;  // 1.0 = right, -1.0 = left

    // Dash animation state
    bool isDashing_ = false;
    float dashStartX_ = 0.0f;
    float dashStartY_ = 0.0f;  // Store Y to keep dash level (no gravity)
    float dashEndX_ = 0.0f;
    float dashProgress_ = 0.0f;
    static constexpr float DASH_DURATION = 0.15f;  // seconds
    static constexpr float DASH_DISTANCE = 200.0f;  // pixels

    // Combat state
    bool isAttacking_ = false;
    float attackTimer_ = 0.0f;
    int comboCount_ = 0;
    float comboTimer_ = 0.0f;
    static constexpr float COMBO_WINDOW = 0.5f;
    static constexpr float ATTACK_DURATION = 0.2f;

    // Sword hitbox
    bestow::Entity swordHitbox_;
    bool swordHitboxActive_ = false;

    // Player combat
    int playerHealth_ = 100;
    float invincibilityTimer_ = 0.0f;
    static constexpr float INVINCIBILITY_DURATION = 1.0f;

    // Advanced movement
    bool isGrounded_ = false;
    bool isTouchingWall_ = false;
    bool isInAir_ = false;
    int airJumpCount_ = 0;
    bool hasUsedDoubleJump_ = false;

    // Ground pound
    bool isGroundPounding_ = false;
    float groundPoundVelocity_ = 800.0f;

    // Ability unlocks
    bool hasDoubleJumpAbility_ = false;
    bool hasWallJumpAbility_ = false;
    bool hasGroundPoundAbility_ = false;
    bool hasSwordAbility_ = false;
    bool hasShieldAbility_ = false;
    bool hasRangedAbility_ = false;

    // Boss fight
    bool bossActive_ = false;
    int bossHealth_ = 500;
    int bossPhase_ = 1;
    float bossAttackTimer_ = 0.0f;
    float bossPatternTimer_ = 0.0f;

    // Level progression
    int currentZone_ = 1;
    bestow::Vec2 lastCheckpoint_ = {100.0f, 200.0f};
    int enemiesDefeated_ = 0;
    int totalEnemiesInZone_ = 0;

    // Audio
    std::unique_ptr<bestow::IConfigSystem> audioConfig_;
    std::unordered_map<std::string, bestow::AssetHandle> soundAssets_;
    bool audioEnabled_ = true;
    static constexpr bestow::Channel MUSIC_CHANNEL = 0;
    static constexpr bestow::Channel SFX_CHANNEL = 1;
    static constexpr bestow::Channel PLAYER_CHANNEL = 2;
    static constexpr bestow::Channel COMBAT_CHANNEL = 3;
};

}  // namespace abilitydemo

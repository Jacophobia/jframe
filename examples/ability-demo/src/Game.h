// examples/ability-demo/src/Game.h
// Gameplay Ability System demonstration game

#pragma once

import std;
import jframe;
import jframe.core;
import jframe.gas;
import jframe.camera;
import jframe.blueprints;
import jframe.config.impl;

namespace abilitydemo {

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
    void setupGAS();
    void setupBlueprints();
    void createPlayer();
    void loadLevel();
    void checkCollectablePickups();

    void handlePlayerInput(jframe::DeltaTime dt);
    void updatePlayerMovement(jframe::DeltaTime dt);
    void updateCamera(jframe::DeltaTime dt);
    void renderUI();

    // Entity creation via blueprints (simplified)
    void createProjectile(float x, float y, float dirX, float dirY, bool isEnemy);

    // Combat
    void handleCombat(jframe::DeltaTime dt);
    void updateSwordHitbox();
    void checkSwordCollisions();
    void playerTakeDamage(int amount);
    void enemyTakeDamage(jframe::Entity enemy, int amount);
    void bossTakeDamage(int amount);

    // Enemy AI
    void updateEnemies(jframe::DeltaTime dt);
    void updateEnemy(jframe::Entity enemy, jframe::DeltaTime dt);
    void updateBoss(jframe::DeltaTime dt);

    // Level mechanics
    void updateSwitches();
    void updateDoors();
    void updateMovingPlatforms(jframe::DeltaTime dt);
    void updateProjectiles(jframe::DeltaTime dt);
    void checkTriggerZones();
    void checkHealthPickups();
    void checkCheckpoints();

    // Advanced movement
    void updateGroundCheck();
    void updateWallCheck();
    void handleDoubleJump();
    void handleWallJump();
    void handleGroundPound(jframe::DeltaTime dt);

    // Boss patterns
    void bossPatternAttack();
    void bossPatternJump();
    void bossPatternSummon();

    // Audio
    void setupAudio();
    void playSound(const std::string& soundName);
    void playMusic(const std::string& musicName);
    void stopMusic();

    jframe::core::Engine* engine_ = nullptr;

    // Config system
    std::unique_ptr<jframe::IConfigSystem> config_;

    // GAS system (created separately, not part of JFrameEngine)
    std::unique_ptr<jframe::IGASSystem> gas_;

    // Blueprint factory for data-driven entity creation
    std::unique_ptr<jframe::IBlueprintFactory> blueprints_;

    // Camera system for smooth following
    std::unique_ptr<jframe::ICameraSystem> camera_;

    jframe::Entity player_;

    // Level tracking
    jframe::LevelId currentLevelId_{};
    jframe::AssetHandle levelAssetHandle_;

    // NOTE: Entity vectors removed - using Entity Query System instead:
    // - Use sys.entities->collect<PlatformTag>() for platforms
    // - Use sys.entities->collect<EnemyTag>() for enemies
    // - Use sys.entities->first<BossTag>() for boss
    // - etc.

    // GAS attribute IDs
    jframe::AttributeId healthAttr_ = 0;
    jframe::AttributeId staminaAttr_ = 0;
    jframe::AttributeId moveSpeedAttr_ = 0;

    // GAS ability IDs
    jframe::AbilityId dashAbility_ = 0;
    jframe::AbilityId jumpAbility_ = 0;
    jframe::AbilityId doubleJumpAbility_ = 0;
    jframe::AbilityId wallJumpAbility_ = 0;
    jframe::AbilityId groundPoundAbility_ = 0;
    jframe::AbilityId swordAttackAbility_ = 0;
    jframe::AbilityId swordCombo2Ability_ = 0;
    jframe::AbilityId swordCombo3Ability_ = 0;
    jframe::AbilityId shieldAbility_ = 0;
    jframe::AbilityId rangedAbility_ = 0;

    // GAS effect IDs
    jframe::EffectId healthRegenEffect_ = 0;
    jframe::EffectId stunEffect_ = 0;
    jframe::EffectId shieldEffect_ = 0;
    jframe::EffectId invincibilityEffect_ = 0;
    jframe::EffectId comboWindow1Effect_ = 0;
    jframe::EffectId comboWindow2Effect_ = 0;

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
    jframe::Entity swordHitbox_;
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
    jframe::Vec2 lastCheckpoint_ = {100.0f, 200.0f};
    int enemiesDefeated_ = 0;
    int totalEnemiesInZone_ = 0;

    // Audio
    std::unique_ptr<jframe::IConfigSystem> audioConfig_;
    std::unordered_map<std::string, jframe::AssetHandle> soundAssets_;
    bool audioEnabled_ = true;
    static constexpr jframe::Channel MUSIC_CHANNEL = 0;
    static constexpr jframe::Channel SFX_CHANNEL = 1;
    static constexpr jframe::Channel PLAYER_CHANNEL = 2;
    static constexpr jframe::Channel COMBAT_CHANNEL = 3;
};

}  // namespace abilitydemo

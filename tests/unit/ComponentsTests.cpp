#include <gtest/gtest.h>
import jframe.components;
import jframe.types;
import std;

using namespace jframe;
using namespace jframe::components;

//==========================================================================
// Health Component Tests
//==========================================================================

TEST(ComponentsTests, Health_InitialState) {
    Health health{100, 100, 0.0f, 1.0f};

    EXPECT_EQ(health.current, 100);
    EXPECT_EQ(health.maximum, 100);
    EXPECT_FALSE(health.isDead());
    EXPECT_FALSE(health.isInvincible());
    EXPECT_FLOAT_EQ(health.healthPercent(), 1.0f);
}

TEST(ComponentsTests, Health_TakeDamage) {
    Health health{100, 100, 0.0f, 1.0f};

    health.takeDamage(30);

    EXPECT_EQ(health.current, 70);
    EXPECT_FLOAT_EQ(health.healthPercent(), 0.7f);
    EXPECT_FALSE(health.isDead());
    EXPECT_TRUE(health.isInvincible());  // Should be invincible after taking damage
}

TEST(ComponentsTests, Health_InvincibilityPreventsSecondDamage) {
    Health health{100, 100, 0.0f, 1.0f};

    health.takeDamage(30);
    EXPECT_EQ(health.current, 70);

    // Try to damage again while invincible
    health.takeDamage(20);
    EXPECT_EQ(health.current, 70);  // Should not take damage
}

TEST(ComponentsTests, Health_InvincibilityDecays) {
    Health health{100, 100, 0.0f, 1.0f};

    health.takeDamage(30);
    EXPECT_TRUE(health.isInvincible());

    // Update for 0.5 seconds
    health.update(0.5f);
    EXPECT_TRUE(health.isInvincible());

    // Update for another 0.6 seconds (total 1.1s)
    health.update(0.6f);
    EXPECT_FALSE(health.isInvincible());

    // Now can take damage again
    health.takeDamage(20);
    EXPECT_EQ(health.current, 50);
}

TEST(ComponentsTests, Health_Death) {
    Health health{10, 100, 0.0f, 1.0f};

    health.takeDamage(20);

    EXPECT_EQ(health.current, 0);
    EXPECT_TRUE(health.isDead());
}

TEST(ComponentsTests, Health_Heal) {
    Health health{50, 100, 0.0f, 1.0f};

    health.heal(30);
    EXPECT_EQ(health.current, 80);

    // Cannot heal beyond maximum
    health.heal(50);
    EXPECT_EQ(health.current, 100);
}

//==========================================================================
// Timer Component Tests
//==========================================================================

TEST(ComponentsTests, Timer_BasicCountdown) {
    Timer timer;
    timer.start(2.0f);

    EXPECT_FLOAT_EQ(timer.remaining, 2.0f);
    EXPECT_FLOAT_EQ(timer.duration, 2.0f);
    EXPECT_FLOAT_EQ(timer.progress(), 0.0f);
    EXPECT_FALSE(timer.isComplete());

    timer.update(1.0f);
    EXPECT_FLOAT_EQ(timer.remaining, 1.0f);
    EXPECT_FLOAT_EQ(timer.progress(), 0.5f);
    EXPECT_FALSE(timer.isComplete());

    timer.update(1.5f);
    EXPECT_LE(timer.remaining, 0.0f);
    EXPECT_TRUE(timer.isComplete());
}

TEST(ComponentsTests, Timer_CallbackFires) {
    Timer timer;
    int callbackCount = 0;

    timer.onComplete = [&callbackCount]() { callbackCount++; };
    timer.start(1.0f);

    timer.update(0.5f);
    EXPECT_EQ(callbackCount, 0);

    timer.update(0.6f);  // Should complete
    EXPECT_EQ(callbackCount, 1);
}

TEST(ComponentsTests, Timer_RepeatingTimer) {
    Timer timer;
    int callbackCount = 0;

    timer.repeating = true;
    timer.onComplete = [&callbackCount]() { callbackCount++; };
    timer.start(1.0f);

    timer.update(1.1f);
    EXPECT_EQ(callbackCount, 1);
    EXPECT_FALSE(timer.isComplete());  // Should not be complete if repeating

    timer.update(1.1f);
    EXPECT_EQ(callbackCount, 2);
}

TEST(ComponentsTests, Timer_PauseResume) {
    Timer timer;
    timer.start(2.0f);

    timer.update(1.0f);
    EXPECT_FLOAT_EQ(timer.remaining, 1.0f);

    timer.paused = true;
    timer.update(0.5f);
    EXPECT_FLOAT_EQ(timer.remaining, 1.0f);  // Should not change

    timer.paused = false;
    timer.update(0.5f);
    EXPECT_FLOAT_EQ(timer.remaining, 0.5f);
}

//==========================================================================
// GroundDetector Component Tests
//==========================================================================

TEST(ComponentsTests, GroundDetector_InitiallyNotGrounded) {
    GroundDetector detector;

    EXPECT_FALSE(detector.isGrounded);
    EXPECT_FALSE(detector.canJump());
}

TEST(ComponentsTests, GroundDetector_BecomesGrounded) {
    GroundDetector detector;

    detector.update(0.016f, true);

    EXPECT_TRUE(detector.isGrounded);
    EXPECT_TRUE(detector.canJump());
    EXPECT_GT(detector.coyoteTime, 0.0f);
}

TEST(ComponentsTests, GroundDetector_CoyoteTime) {
    GroundDetector detector;

    // Become grounded
    detector.update(0.016f, true);
    EXPECT_TRUE(detector.canJump());

    // Leave ground
    detector.update(0.016f, false);
    EXPECT_FALSE(detector.isGrounded);
    EXPECT_TRUE(detector.canJump());  // Still can jump due to coyote time

    // Wait for coyote time to expire
    detector.update(0.05f, false);
    detector.update(0.05f, false);
    detector.update(0.05f, false);

    EXPECT_FALSE(detector.canJump());  // Coyote time expired
}

TEST(ComponentsTests, GroundDetector_ConsumeJump) {
    GroundDetector detector;

    detector.update(0.016f, true);
    EXPECT_TRUE(detector.canJump());

    detector.consumeJump();
    EXPECT_EQ(detector.coyoteTime, 0.0f);
}

//==========================================================================
// JumpState Component Tests
//==========================================================================

TEST(ComponentsTests, JumpState_SingleJump) {
    JumpState jump{1, 1, 400.0f};

    EXPECT_TRUE(jump.canJump());

    jump.jump();
    EXPECT_EQ(jump.jumpsRemaining, 0);
    EXPECT_FALSE(jump.canJump());

    jump.reset();
    EXPECT_EQ(jump.jumpsRemaining, 1);
    EXPECT_TRUE(jump.canJump());
}

TEST(ComponentsTests, JumpState_DoubleJump) {
    JumpState jump{2, 2, 400.0f};

    EXPECT_TRUE(jump.canJump());

    jump.jump();
    EXPECT_EQ(jump.jumpsRemaining, 1);
    EXPECT_TRUE(jump.canJump());

    jump.jump();
    EXPECT_EQ(jump.jumpsRemaining, 0);
    EXPECT_FALSE(jump.canJump());
}

//==========================================================================
// StateMachine Component Tests
//==========================================================================

enum class PlayerState {
    Idle,
    Running,
    Jumping,
    Falling
};

TEST(ComponentsTests, StateMachine_InitialState) {
    StateMachine<PlayerState> sm{PlayerState::Idle, PlayerState::Idle, 0.0f};

    EXPECT_TRUE(sm.isState(PlayerState::Idle));
    EXPECT_EQ(sm.stateTime, 0.0f);
}

TEST(ComponentsTests, StateMachine_StateTransition) {
    StateMachine<PlayerState> sm{PlayerState::Idle, PlayerState::Idle, 0.0f};

    sm.setState(PlayerState::Running);

    EXPECT_TRUE(sm.isState(PlayerState::Running));
    EXPECT_EQ(sm.previousState, PlayerState::Idle);
    EXPECT_EQ(sm.stateTime, 0.0f);
}

TEST(ComponentsTests, StateMachine_StateTimeProgresses) {
    StateMachine<PlayerState> sm{PlayerState::Idle, PlayerState::Idle, 0.0f};

    sm.update(0.5f);
    EXPECT_FLOAT_EQ(sm.stateTime, 0.5f);

    sm.update(0.3f);
    EXPECT_FLOAT_EQ(sm.stateTime, 0.8f);
}

TEST(ComponentsTests, StateMachine_StateTimeResetsOnTransition) {
    StateMachine<PlayerState> sm{PlayerState::Idle, PlayerState::Idle, 0.0f};

    sm.update(0.5f);
    EXPECT_FLOAT_EQ(sm.stateTime, 0.5f);

    sm.setState(PlayerState::Running);
    EXPECT_EQ(sm.stateTime, 0.0f);
}

TEST(ComponentsTests, StateMachine_JustEntered) {
    StateMachine<PlayerState> sm{PlayerState::Idle, PlayerState::Idle, 0.0f};

    EXPECT_TRUE(sm.justEntered());

    sm.update(0.016f);
    EXPECT_FALSE(sm.justEntered());

    sm.setState(PlayerState::Running);
    EXPECT_TRUE(sm.justEntered());
}

//==========================================================================
// Lifetime Component Tests
//==========================================================================

TEST(ComponentsTests, Lifetime_CountsDown) {
    Lifetime lifetime{2.0f};

    EXPECT_FALSE(lifetime.isExpired());

    lifetime.update(1.0f);
    EXPECT_FLOAT_EQ(lifetime.remaining, 1.0f);
    EXPECT_FALSE(lifetime.isExpired());

    lifetime.update(1.5f);
    EXPECT_TRUE(lifetime.isExpired());
}

//==========================================================================
// Score Component Tests
//==========================================================================

TEST(ComponentsTests, Score_BasicScoring) {
    Score score{0, 1};

    score.add(100);
    EXPECT_EQ(score.value, 100);

    score.add(50);
    EXPECT_EQ(score.value, 150);
}

TEST(ComponentsTests, Score_Multiplier) {
    Score score{0, 2};

    score.add(100);
    EXPECT_EQ(score.value, 200);

    score.multiplier = 3;
    score.add(50);
    EXPECT_EQ(score.value, 350);
}

//==========================================================================
// Velocity Component Tests
//==========================================================================

TEST(ComponentsTests, Velocity_DefaultConstruction) {
    Velocity vel;

    EXPECT_FLOAT_EQ(vel.linear.x, 0.0f);
    EXPECT_FLOAT_EQ(vel.linear.y, 0.0f);
    EXPECT_FLOAT_EQ(vel.angular, 0.0f);
}

//==========================================================================
// Damage Component Tests
//==========================================================================

TEST(ComponentsTests, Damage_DefaultConstruction) {
    Damage damage;

    EXPECT_EQ(damage.amount, 10);
    EXPECT_FALSE(damage.destroyOnHit);
}

#include <gtest/gtest.h>
import jframe.components;
import jframe.builders;
import jframe.luaconfig;
import jframe.types;
import jframe.input;
import jframe.physics;
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

//==========================================================================
// Acceleration Component Tests
//==========================================================================

TEST(ComponentsTests, Acceleration_DefaultConstruction) {
    Acceleration accel;

    EXPECT_FLOAT_EQ(accel.value.x, 0.0f);
    EXPECT_FLOAT_EQ(accel.value.y, 0.0f);
}

TEST(ComponentsTests, Acceleration_CustomConstruction) {
    Acceleration accel{{100.0f, -200.0f}};

    EXPECT_FLOAT_EQ(accel.value.x, 100.0f);
    EXPECT_FLOAT_EQ(accel.value.y, -200.0f);
}

//==========================================================================
// MaxSpeed Component Tests
//==========================================================================

TEST(ComponentsTests, MaxSpeed_DefaultConstruction) {
    MaxSpeed maxSpeed;

    EXPECT_FLOAT_EQ(maxSpeed.linear, 500.0f);
    EXPECT_FLOAT_EQ(maxSpeed.angular, 360.0f);
}

TEST(ComponentsTests, MaxSpeed_CustomConstruction) {
    MaxSpeed maxSpeed{300.0f, 180.0f};

    EXPECT_FLOAT_EQ(maxSpeed.linear, 300.0f);
    EXPECT_FLOAT_EQ(maxSpeed.angular, 180.0f);
}

//==========================================================================
// Health Component Edge Cases
//==========================================================================

TEST(ComponentsTests, Health_ZeroMaximum) {
    Health health{0, 0, 0.0f, 1.0f};

    EXPECT_TRUE(health.isDead());
    EXPECT_FLOAT_EQ(health.healthPercent(), 0.0f);  // Avoid division by zero
}

TEST(ComponentsTests, Health_NegativeDamage) {
    Health health{100, 100, 0.0f, 1.0f};

    health.takeDamage(-50);
    EXPECT_EQ(health.current, 100);  // Should not heal via negative damage
}

TEST(ComponentsTests, Health_HealDead) {
    Health health{0, 100, 0.0f, 1.0f};

    EXPECT_TRUE(health.isDead());
    health.heal(50);
    EXPECT_EQ(health.current, 50);
    EXPECT_FALSE(health.isDead());
}

TEST(ComponentsTests, Health_NegativeHeal) {
    Health health{50, 100, 0.0f, 1.0f};

    health.heal(-30);
    EXPECT_EQ(health.current, 50);  // Should not damage via negative heal
}

TEST(ComponentsTests, Health_InvincibilityZeroDuration) {
    Health health{100, 100, 0.0f, 0.0f};

    health.takeDamage(30);
    EXPECT_EQ(health.current, 70);
    EXPECT_FALSE(health.isInvincible());  // Zero duration = no invincibility

    health.takeDamage(20);
    EXPECT_EQ(health.current, 50);  // Should take damage immediately
}

//==========================================================================
// Timer Component Edge Cases
//==========================================================================

TEST(ComponentsTests, Timer_NegativeDuration) {
    Timer timer;
    timer.start(-1.0f);

    EXPECT_TRUE(timer.isComplete());  // Negative duration should be complete
}

TEST(ComponentsTests, Timer_ZeroDuration) {
    Timer timer;
    timer.start(0.0f);

    EXPECT_TRUE(timer.isComplete());
}

TEST(ComponentsTests, Timer_CallbackFiresOnceForNonRepeating) {
    Timer timer;
    int callbackCount = 0;

    timer.onComplete = [&callbackCount]() { callbackCount++; };
    timer.start(1.0f);

    timer.update(1.5f);
    EXPECT_EQ(callbackCount, 1);

    timer.update(1.0f);  // Should not fire again
    EXPECT_EQ(callbackCount, 1);
}

TEST(ComponentsTests, Timer_RepeatingMultipleCycles) {
    Timer timer;
    int callbackCount = 0;

    timer.repeating = true;
    timer.onComplete = [&callbackCount]() { callbackCount++; };
    timer.start(1.0f);

    timer.update(3.5f);  // Should fire 3 times
    EXPECT_EQ(callbackCount, 3);
    EXPECT_FALSE(timer.isComplete());
}

TEST(ComponentsTests, Timer_ProgressClamping) {
    Timer timer;
    timer.start(1.0f);

    timer.update(2.0f);  // Over-update
    EXPECT_GE(timer.progress(), 1.0f);  // Progress can exceed 1.0
}

//==========================================================================
// GroundDetector Edge Cases
//==========================================================================

TEST(ComponentsTests, GroundDetector_ImmediateConsumeJump) {
    GroundDetector detector;

    detector.consumeJump();
    EXPECT_FALSE(detector.canJump());
    EXPECT_EQ(detector.coyoteTime, 0.0f);
}

TEST(ComponentsTests, GroundDetector_GroundedWhileInAir) {
    GroundDetector detector;

    detector.update(0.016f, false);
    EXPECT_FALSE(detector.isGrounded);
    EXPECT_FALSE(detector.canJump());

    detector.update(0.016f, true);
    EXPECT_TRUE(detector.isGrounded);
    EXPECT_TRUE(detector.canJump());
}

//==========================================================================
// JumpState Edge Cases
//==========================================================================

TEST(ComponentsTests, JumpState_ZeroMaxJumps) {
    JumpState jump{0, 0, 400.0f};

    EXPECT_FALSE(jump.canJump());
    jump.jump();
    EXPECT_EQ(jump.jumpsRemaining, 0);
}

TEST(ComponentsTests, JumpState_TripleJump) {
    JumpState jump{3, 3, 400.0f};

    EXPECT_TRUE(jump.canJump());
    jump.jump();
    EXPECT_EQ(jump.jumpsRemaining, 2);
    jump.jump();
    EXPECT_EQ(jump.jumpsRemaining, 1);
    jump.jump();
    EXPECT_EQ(jump.jumpsRemaining, 0);
    EXPECT_FALSE(jump.canJump());

    jump.reset();
    EXPECT_EQ(jump.jumpsRemaining, 3);
}

TEST(ComponentsTests, JumpState_JumpWhenCannotJump) {
    JumpState jump{0, 1, 400.0f};

    EXPECT_FALSE(jump.canJump());
    jump.jump();  // Should not decrement below 0
    EXPECT_EQ(jump.jumpsRemaining, 0);
}

//==========================================================================
// StateMachine Edge Cases
//==========================================================================

TEST(ComponentsTests, StateMachine_SetSameState) {
    StateMachine<PlayerState> sm{PlayerState::Idle, PlayerState::Idle, 0.0f};

    sm.update(0.5f);
    EXPECT_FLOAT_EQ(sm.stateTime, 0.5f);

    sm.setState(PlayerState::Idle);  // Same state
    EXPECT_FLOAT_EQ(sm.stateTime, 0.5f);  // Should NOT reset time
    EXPECT_EQ(sm.previousState, PlayerState::Idle);
}

TEST(ComponentsTests, StateMachine_RapidTransitions) {
    StateMachine<PlayerState> sm{PlayerState::Idle, PlayerState::Idle, 0.0f};

    sm.setState(PlayerState::Running);
    EXPECT_EQ(sm.previousState, PlayerState::Idle);
    EXPECT_TRUE(sm.isState(PlayerState::Running));

    sm.setState(PlayerState::Jumping);
    EXPECT_EQ(sm.previousState, PlayerState::Running);
    EXPECT_TRUE(sm.isState(PlayerState::Jumping));

    sm.setState(PlayerState::Falling);
    EXPECT_EQ(sm.previousState, PlayerState::Jumping);
    EXPECT_TRUE(sm.isState(PlayerState::Falling));
}

//==========================================================================
// Lifetime Edge Cases
//==========================================================================

TEST(ComponentsTests, Lifetime_NegativeRemaining) {
    Lifetime lifetime{1.0f};

    lifetime.update(2.0f);
    EXPECT_TRUE(lifetime.isExpired());
    EXPECT_LT(lifetime.remaining, 0.0f);
}

TEST(ComponentsTests, Lifetime_ZeroInitial) {
    Lifetime lifetime{0.0f};

    EXPECT_TRUE(lifetime.isExpired());
}

//==========================================================================
// Score Edge Cases
//==========================================================================

TEST(ComponentsTests, Score_ZeroMultiplier) {
    Score score{0, 0};

    score.add(100);
    EXPECT_EQ(score.value, 0);
}

TEST(ComponentsTests, Score_NegativeMultiplier) {
    Score score{0, -2};

    score.add(100);
    EXPECT_EQ(score.value, -200);
}

TEST(ComponentsTests, Score_NegativePoints) {
    Score score{100, 1};

    score.add(-50);
    EXPECT_EQ(score.value, 50);
}

TEST(ComponentsTests, Score_Overflow) {
    Score score{2147483647, 1};  // INT_MAX

    score.add(1);
    // Behavior is platform-dependent, but should not crash
    EXPECT_NE(score.value, 2147483647);
}

//==========================================================================
// Mock Classes for Builder Tests
//==========================================================================

class MockInputSystem : public IInputSystem {
public:
    std::vector<InputMapping> registeredMappings;

    void registerMapping(const InputMapping& mapping) override {
        registeredMappings.push_back(mapping);
    }

    void removeMapping(const InputBinding& binding) override {}
    void clearMappings() override {}
    std::vector<InputMapping> getMappings() const override { return registeredMappings; }

    ActionState getActionState(const Action& action) const override { return {}; }
    std::vector<ActionState> getAllActionStates() const override { return {}; }
    bool isActionActive(const Action& action) const override { return false; }
    bool wasActionJustPressed(const Action& action) const override { return false; }
    bool wasActionJustReleased(const Action& action) const override { return false; }
    float getActionValue(const Action& action) const override { return 0.0f; }

    std::optional<InputBinding> getLastInput() const override { return std::nullopt; }
    bool isListeningForInput() const override { return false; }
    void startListeningForInput() override {}
    void stopListeningForInput() override {}

    Vec2 getMousePosition() const override { return {0, 0}; }
    Vec2 getMouseDelta() const override { return {0, 0}; }
    bool isMouseButtonDown(int button) const override { return false; }

    int getConnectedControllerCount() const override { return 0; }
    bool isControllerConnected(int index) const override { return false; }
    std::string getControllerName(int index) const override { return ""; }

    void update() override {}
};

class MockPhysicsSystem : public IPhysicsSystem {
public:
    struct BodyRecord {
        Entity entity;
        PhysicsBodyDef def;
        CollisionLayer layer = 0xFFFF;
        CollisionMask mask = 0xFFFF;
    };

    std::vector<BodyRecord> createdBodies;

    void createBody(Entity entity, const PhysicsBodyDef& def) override {
        BodyRecord record;
        record.entity = entity;
        record.def = def;
        createdBodies.push_back(record);
    }

    void destroyBody(Entity entity) override {}
    bool hasBody(Entity entity) const override { return false; }

    void setBodyType(Entity entity, BodyType type) override {}
    BodyType getBodyType(Entity entity) const override { return BodyType::Dynamic; }

    void setPosition(Entity entity, Vec2 position) override {}
    Vec2 getPosition(Entity entity) const override { return {0, 0}; }

    void setRotation(Entity entity, float radians) override {}
    float getRotation(Entity entity) const override { return 0.0f; }

    void setVelocity(Entity entity, Vec2 velocity) override {}
    Vec2 getVelocity(Entity entity) const override { return {0, 0}; }

    void setAngularVelocity(Entity entity, float velocity) override {}
    float getAngularVelocity(Entity entity) const override { return 0.0f; }

    Vec2 getBodySize(Entity entity) const override { return {0, 0}; }

    void applyForce(Entity entity, Vec2 force, Vec2 point = Vec2{0, 0}) override {}
    void applyImpulse(Entity entity, Vec2 impulse, Vec2 point = Vec2{0, 0}) override {}
    void applyTorque(Entity entity, float torque) override {}

    void setCollisionLayer(Entity entity, CollisionLayer layer) override {
        for (auto& body : createdBodies) {
            if (body.entity == entity) {
                body.layer = layer;
                break;
            }
        }
    }

    void setCollisionMask(Entity entity, CollisionMask mask) override {
        for (auto& body : createdBodies) {
            if (body.entity == entity) {
                body.mask = mask;
                break;
            }
        }
    }

    void setSensor(Entity entity, bool isSensor) override {}

    std::vector<Entity> queryAABB(Vec2 min, Vec2 max) const override { return {}; }
    std::vector<Entity> queryCircle(Vec2 center, float radius) const override { return {}; }
    std::optional<RaycastHit> raycast(Vec2 origin, Vec2 direction, float maxDistance, CollisionMask mask = 0xFFFF) const override { return std::nullopt; }
    std::vector<RaycastHit> raycastAll(Vec2 origin, Vec2 direction, float maxDistance, CollisionMask mask = 0xFFFF) const override { return {}; }

    void setGravity(Vec2 gravity) override {}
    Vec2 getGravity() const override { return {0, -980}; }

    void setCollisionCallback(CollisionCallback callback) override {}

    GroundCheckResult checkGrounded(Entity entity, const GroundCheckParams& params = {}) const override { return {}; }

    CollisionLayer getCollisionLayer(Entity entity) const override { return 0xFFFF; }

    void update(DeltaTime dt) override {}
};

//==========================================================================
// InputMappingBuilder Tests
//==========================================================================

TEST(ComponentsTests, InputMappingBuilder_SingleKeyAction) {
    MockInputSystem input;
    InputMappingBuilder builder(input);

    builder.action("jump")
        .key(32)  // Space
        .apply();

    ASSERT_EQ(input.registeredMappings.size(), 1);
    EXPECT_EQ(input.registeredMappings[0].action, "jump");
    EXPECT_EQ(input.registeredMappings[0].binding.deviceType, InputDeviceType::Keyboard);
    EXPECT_EQ(input.registeredMappings[0].binding.keyCode, 32);
    EXPECT_FLOAT_EQ(input.registeredMappings[0].binding.scale, 1.0f);
}

TEST(ComponentsTests, InputMappingBuilder_MultipleKeysPerAction) {
    MockInputSystem input;
    InputMappingBuilder builder(input);

    builder.action("move_horizontal")
        .key(68, 1.0f)   // D
        .key(65, -1.0f)  // A
        .apply();

    ASSERT_EQ(input.registeredMappings.size(), 2);
    EXPECT_EQ(input.registeredMappings[0].action, "move_horizontal");
    EXPECT_FLOAT_EQ(input.registeredMappings[0].binding.scale, 1.0f);
    EXPECT_EQ(input.registeredMappings[1].action, "move_horizontal");
    EXPECT_FLOAT_EQ(input.registeredMappings[1].binding.scale, -1.0f);
}

TEST(ComponentsTests, InputMappingBuilder_ControllerButton) {
    MockInputSystem input;
    InputMappingBuilder builder(input);

    builder.action("jump")
        .button(0, 0, 1.0f)  // A button, controller 0
        .apply();

    ASSERT_EQ(input.registeredMappings.size(), 1);
    EXPECT_EQ(input.registeredMappings[0].binding.deviceType, InputDeviceType::Controller);
    EXPECT_EQ(input.registeredMappings[0].binding.keyCode, 0);
    EXPECT_EQ(input.registeredMappings[0].binding.deviceIndex, 0);
    EXPECT_FLOAT_EQ(input.registeredMappings[0].binding.deadzone, 0.1f);
}

TEST(ComponentsTests, InputMappingBuilder_ControllerAxis) {
    MockInputSystem input;
    InputMappingBuilder builder(input);

    builder.action("move_horizontal")
        .axis(0, 0, 0.2f)  // Left stick X, controller 0, 0.2 deadzone
        .apply();

    ASSERT_EQ(input.registeredMappings.size(), 1);
    EXPECT_EQ(input.registeredMappings[0].binding.deviceType, InputDeviceType::Controller);
    EXPECT_EQ(input.registeredMappings[0].binding.keyCode, 0x8000);  // Axis flag
    EXPECT_FLOAT_EQ(input.registeredMappings[0].binding.deadzone, 0.2f);
}

TEST(ComponentsTests, InputMappingBuilder_MouseButton) {
    MockInputSystem input;
    InputMappingBuilder builder(input);

    builder.action("shoot")
        .mouseButton(0, 1.0f)  // Left mouse button
        .apply();

    ASSERT_EQ(input.registeredMappings.size(), 1);
    EXPECT_EQ(input.registeredMappings[0].binding.deviceType, InputDeviceType::Mouse);
    EXPECT_EQ(input.registeredMappings[0].binding.keyCode, 0);
}

TEST(ComponentsTests, InputMappingBuilder_ChainedActions) {
    MockInputSystem input;
    InputMappingBuilder builder(input);

    builder.action("jump")
        .key(32)
        .button(0)
        .action("attack")
        .key(88)  // X
        .apply();

    ASSERT_EQ(input.registeredMappings.size(), 3);
    EXPECT_EQ(input.registeredMappings[0].action, "jump");
    EXPECT_EQ(input.registeredMappings[1].action, "jump");
    EXPECT_EQ(input.registeredMappings[2].action, "attack");
}

TEST(ComponentsTests, InputMappingBuilder_DeadzoneModifier) {
    MockInputSystem input;
    InputMappingBuilder builder(input);

    builder.action("move")
        .axis(0)
        .deadzone(0.3f)
        .apply();

    ASSERT_EQ(input.registeredMappings.size(), 1);
    EXPECT_FLOAT_EQ(input.registeredMappings[0].binding.deadzone, 0.3f);
}

TEST(ComponentsTests, InputMappingBuilder_Clear) {
    MockInputSystem input;
    InputMappingBuilder builder(input);

    builder.action("jump")
        .key(32)
        .clear();

    EXPECT_EQ(builder.getMappings().size(), 0);
}

TEST(ComponentsTests, InputMappingBuilder_GetMappingsBeforeApply) {
    MockInputSystem input;
    InputMappingBuilder builder(input);

    builder.action("jump")
        .key(32)
        .button(0);

    const auto& mappings = builder.getMappings();
    EXPECT_EQ(mappings.size(), 2);
    EXPECT_EQ(input.registeredMappings.size(), 0);  // Not applied yet
}

TEST(ComponentsTests, InputMappingBuilder_NoActionSet) {
    MockInputSystem input;
    InputMappingBuilder builder(input);

    builder.key(32)  // No action set
        .apply();

    EXPECT_EQ(input.registeredMappings.size(), 0);  // Should not register
}

//==========================================================================
// PhysicsBodyBuilder Tests
//==========================================================================

TEST(ComponentsTests, PhysicsBodyBuilder_DynamicBody) {
    MockPhysicsSystem physics;
    Entity entity{123};

    PhysicsBodyBuilder(physics, entity)
        .dynamic()
        .position(100, 200)
        .size(32, 48)
        .create();

    ASSERT_EQ(physics.createdBodies.size(), 1);
    EXPECT_EQ(physics.createdBodies[0].entity, entity);
    EXPECT_EQ(physics.createdBodies[0].def.type, BodyType::Dynamic);
    EXPECT_FLOAT_EQ(physics.createdBodies[0].def.transform.x, 100.0f);
    EXPECT_FLOAT_EQ(physics.createdBodies[0].def.transform.y, 200.0f);
    EXPECT_FLOAT_EQ(physics.createdBodies[0].def.size.x, 32.0f);
    EXPECT_FLOAT_EQ(physics.createdBodies[0].def.size.y, 48.0f);
}

TEST(ComponentsTests, PhysicsBodyBuilder_StaticBody) {
    MockPhysicsSystem physics;
    Entity entity{456};

    PhysicsBodyBuilder(physics, entity)
        .staticBody()
        .position(0, 0)
        .size(100, 50)
        .create();

    ASSERT_EQ(physics.createdBodies.size(), 1);
    EXPECT_EQ(physics.createdBodies[0].def.type, BodyType::Static);
}

TEST(ComponentsTests, PhysicsBodyBuilder_KinematicBody) {
    MockPhysicsSystem physics;
    Entity entity{789};

    PhysicsBodyBuilder(physics, entity)
        .kinematic()
        .position(50, 50)
        .size(64, 64)
        .create();

    ASSERT_EQ(physics.createdBodies.size(), 1);
    EXPECT_EQ(physics.createdBodies[0].def.type, BodyType::Kinematic);
}

TEST(ComponentsTests, PhysicsBodyBuilder_PositionVec2) {
    MockPhysicsSystem physics;
    Entity entity{100};

    PhysicsBodyBuilder(physics, entity)
        .dynamic()
        .position(Vec2{150.0f, 250.0f})
        .size(32, 32)
        .create();

    ASSERT_EQ(physics.createdBodies.size(), 1);
    EXPECT_FLOAT_EQ(physics.createdBodies[0].def.transform.x, 150.0f);
    EXPECT_FLOAT_EQ(physics.createdBodies[0].def.transform.y, 250.0f);
}

TEST(ComponentsTests, PhysicsBodyBuilder_SizeVec2) {
    MockPhysicsSystem physics;
    Entity entity{101};

    PhysicsBodyBuilder(physics, entity)
        .dynamic()
        .position(0, 0)
        .size(Vec2{64.0f, 96.0f})
        .create();

    ASSERT_EQ(physics.createdBodies.size(), 1);
    EXPECT_FLOAT_EQ(physics.createdBodies[0].def.size.x, 64.0f);
    EXPECT_FLOAT_EQ(physics.createdBodies[0].def.size.y, 96.0f);
}

TEST(ComponentsTests, PhysicsBodyBuilder_Rotation) {
    MockPhysicsSystem physics;
    Entity entity{102};

    PhysicsBodyBuilder(physics, entity)
        .dynamic()
        .position(0, 0)
        .size(32, 32)
        .rotation(3.14159f)
        .create();

    ASSERT_EQ(physics.createdBodies.size(), 1);
    EXPECT_FLOAT_EQ(physics.createdBodies[0].def.transform.rotation, 3.14159f);
}

TEST(ComponentsTests, PhysicsBodyBuilder_FixedRotation) {
    MockPhysicsSystem physics;
    Entity entity{103};

    PhysicsBodyBuilder(physics, entity)
        .dynamic()
        .position(0, 0)
        .size(32, 32)
        .fixedRotation()
        .create();

    ASSERT_EQ(physics.createdBodies.size(), 1);
    EXPECT_TRUE(physics.createdBodies[0].def.fixedRotation);
}

TEST(ComponentsTests, PhysicsBodyBuilder_FixedRotationFalse) {
    MockPhysicsSystem physics;
    Entity entity{104};

    PhysicsBodyBuilder(physics, entity)
        .dynamic()
        .position(0, 0)
        .size(32, 32)
        .fixedRotation(false)
        .create();

    ASSERT_EQ(physics.createdBodies.size(), 1);
    EXPECT_FALSE(physics.createdBodies[0].def.fixedRotation);
}

TEST(ComponentsTests, PhysicsBodyBuilder_PhysicsProperties) {
    MockPhysicsSystem physics;
    Entity entity{105};

    PhysicsBodyBuilder(physics, entity)
        .dynamic()
        .position(0, 0)
        .size(32, 32)
        .density(2.5f)
        .friction(0.8f)
        .restitution(0.6f)
        .linearDamping(0.1f)
        .angularDamping(0.2f)
        .create();

    ASSERT_EQ(physics.createdBodies.size(), 1);
    EXPECT_FLOAT_EQ(physics.createdBodies[0].def.density, 2.5f);
    EXPECT_FLOAT_EQ(physics.createdBodies[0].def.friction, 0.8f);
    EXPECT_FLOAT_EQ(physics.createdBodies[0].def.restitution, 0.6f);
    EXPECT_FLOAT_EQ(physics.createdBodies[0].def.linearDamping, 0.1f);
    EXPECT_FLOAT_EQ(physics.createdBodies[0].def.angularDamping, 0.2f);
}

TEST(ComponentsTests, PhysicsBodyBuilder_Sensor) {
    MockPhysicsSystem physics;
    Entity entity{106};

    PhysicsBodyBuilder(physics, entity)
        .staticBody()
        .position(0, 0)
        .size(32, 32)
        .sensor()
        .create();

    ASSERT_EQ(physics.createdBodies.size(), 1);
    EXPECT_TRUE(physics.createdBodies[0].def.isSensor);
}

TEST(ComponentsTests, PhysicsBodyBuilder_LayerAndMask) {
    MockPhysicsSystem physics;
    Entity entity{107};

    PhysicsBodyBuilder(physics, entity)
        .dynamic()
        .position(0, 0)
        .size(32, 32)
        .layer(0x0001)
        .mask(0x0008)
        .create();

    ASSERT_EQ(physics.createdBodies.size(), 1);
    EXPECT_EQ(physics.createdBodies[0].layer, 0x0001);
    EXPECT_EQ(physics.createdBodies[0].mask, 0x0008);
}

TEST(ComponentsTests, PhysicsBodyBuilder_GetDef) {
    MockPhysicsSystem physics;
    Entity entity{108};

    PhysicsBodyBuilder builder(physics, entity);
    builder.dynamic()
        .position(100, 200)
        .size(32, 48);

    const auto& def = builder.getDef();
    EXPECT_EQ(def.type, BodyType::Dynamic);
    EXPECT_FLOAT_EQ(def.transform.x, 100.0f);
    EXPECT_FLOAT_EQ(def.transform.y, 200.0f);
    EXPECT_FLOAT_EQ(def.size.x, 32.0f);
    EXPECT_FLOAT_EQ(def.size.y, 48.0f);
}

TEST(ComponentsTests, PhysicsBodyBuilder_ReturnsEntity) {
    MockPhysicsSystem physics;
    Entity entity{109};

    Entity result = PhysicsBodyBuilder(physics, entity)
        .dynamic()
        .position(0, 0)
        .size(32, 32)
        .create();

    EXPECT_EQ(result, entity);
}

//==========================================================================
// Physics Factory Function Tests
//==========================================================================

TEST(ComponentsTests, PhysicsFactory_StaticBox) {
    MockPhysicsSystem physics;
    Entity entity{200};

    physics::staticBox(physics, entity, 100, 200, 64, 32).create();

    ASSERT_EQ(physics.createdBodies.size(), 1);
    EXPECT_EQ(physics.createdBodies[0].def.type, BodyType::Static);
    EXPECT_FLOAT_EQ(physics.createdBodies[0].def.transform.x, 100.0f);
    EXPECT_FLOAT_EQ(physics.createdBodies[0].def.transform.y, 200.0f);
    EXPECT_FLOAT_EQ(physics.createdBodies[0].def.size.x, 64.0f);
    EXPECT_FLOAT_EQ(physics.createdBodies[0].def.size.y, 32.0f);
}

TEST(ComponentsTests, PhysicsFactory_DynamicBox) {
    MockPhysicsSystem physics;
    Entity entity{201};

    physics::dynamicBox(physics, entity, 50, 100, 32, 48).create();

    ASSERT_EQ(physics.createdBodies.size(), 1);
    EXPECT_EQ(physics.createdBodies[0].def.type, BodyType::Dynamic);
    EXPECT_TRUE(physics.createdBodies[0].def.fixedRotation);
}

TEST(ComponentsTests, PhysicsFactory_KinematicBox) {
    MockPhysicsSystem physics;
    Entity entity{202};

    physics::kinematicBox(physics, entity, 25, 75, 16, 16).create();

    ASSERT_EQ(physics.createdBodies.size(), 1);
    EXPECT_EQ(physics.createdBodies[0].def.type, BodyType::Kinematic);
}

TEST(ComponentsTests, PhysicsFactory_Trigger) {
    MockPhysicsSystem physics;
    Entity entity{203};

    physics::trigger(physics, entity, 0, 0, 100, 100).create();

    ASSERT_EQ(physics.createdBodies.size(), 1);
    EXPECT_EQ(physics.createdBodies[0].def.type, BodyType::Static);
    EXPECT_TRUE(physics.createdBodies[0].def.isSensor);
}

TEST(ComponentsTests, PhysicsFactory_Character) {
    MockPhysicsSystem physics;
    Entity entity{204};

    physics::character(physics, entity, 100, 200, 32, 48).create();

    ASSERT_EQ(physics.createdBodies.size(), 1);
    EXPECT_EQ(physics.createdBodies[0].def.type, BodyType::Dynamic);
    EXPECT_TRUE(physics.createdBodies[0].def.fixedRotation);
    EXPECT_FLOAT_EQ(physics.createdBodies[0].def.friction, 0.0f);
    EXPECT_FLOAT_EQ(physics.createdBodies[0].def.linearDamping, 0.0f);
}

TEST(ComponentsTests, PhysicsFactory_Platform) {
    MockPhysicsSystem physics;
    Entity entity{205};

    physics::platform(physics, entity, 0, 0, 200, 32).create();

    ASSERT_EQ(physics.createdBodies.size(), 1);
    EXPECT_EQ(physics.createdBodies[0].def.type, BodyType::Static);
    EXPECT_FLOAT_EQ(physics.createdBodies[0].def.friction, 0.3f);
    EXPECT_EQ(physics.createdBodies[0].layer, CollisionLayers::Terrain | CollisionLayers::Ground);
}

//==========================================================================
// LuaInputLoader Tests
//==========================================================================

TEST(ComponentsTests, LuaInputLoader_SimpleAction) {
    const char* luaCode = R"(
        return {
            actions = {
                jump = {
                    { type = "key", code = 32 },
                }
            }
        }
    )";

    auto mappings = LuaInputLoader::parse(luaCode);

    ASSERT_EQ(mappings.size(), 1);
    EXPECT_EQ(mappings[0].action, "jump");
    EXPECT_EQ(mappings[0].binding.deviceType, InputDeviceType::Keyboard);
    EXPECT_EQ(mappings[0].binding.keyCode, 32);
}

TEST(ComponentsTests, LuaInputLoader_MultipleBindings) {
    const char* luaCode = R"(
        return {
            actions = {
                move_horizontal = {
                    { type = "key", code = 68, scale = 1.0 },
                    { type = "key", code = 65, scale = -1.0 },
                    { type = "axis", code = 0 },
                }
            }
        }
    )";

    auto mappings = LuaInputLoader::parse(luaCode);

    ASSERT_EQ(mappings.size(), 3);
    EXPECT_EQ(mappings[0].action, "move_horizontal");
    EXPECT_FLOAT_EQ(mappings[0].binding.scale, 1.0f);
    EXPECT_EQ(mappings[1].action, "move_horizontal");
    EXPECT_FLOAT_EQ(mappings[1].binding.scale, -1.0f);
    EXPECT_EQ(mappings[2].binding.deviceType, InputDeviceType::Controller);
    EXPECT_EQ(mappings[2].binding.keyCode, 0x8000);  // Axis flag
}

TEST(ComponentsTests, LuaInputLoader_ControllerButton) {
    const char* luaCode = R"(
        return {
            actions = {
                jump = {
                    { type = "button", code = 0, controller = 1 },
                }
            }
        }
    )";

    auto mappings = LuaInputLoader::parse(luaCode);

    ASSERT_EQ(mappings.size(), 1);
    EXPECT_EQ(mappings[0].binding.deviceType, InputDeviceType::Controller);
    EXPECT_EQ(mappings[0].binding.keyCode, 0);
    EXPECT_EQ(mappings[0].binding.deviceIndex, 1);
}

TEST(ComponentsTests, LuaInputLoader_CustomDeadzone) {
    const char* luaCode = R"(
        return {
            actions = {
                move = {
                    { type = "axis", code = 0, deadzone = 0.3 },
                }
            }
        }
    )";

    auto mappings = LuaInputLoader::parse(luaCode);

    ASSERT_EQ(mappings.size(), 1);
    EXPECT_FLOAT_EQ(mappings[0].binding.deadzone, 0.3f);
}

TEST(ComponentsTests, LuaInputLoader_MouseButton) {
    const char* luaCode = R"(
        return {
            actions = {
                shoot = {
                    { type = "mouse", code = 0 },
                }
            }
        }
    )";

    auto mappings = LuaInputLoader::parse(luaCode);

    ASSERT_EQ(mappings.size(), 1);
    EXPECT_EQ(mappings[0].binding.deviceType, InputDeviceType::Mouse);
}

TEST(ComponentsTests, LuaInputLoader_InvalidLua) {
    const char* luaCode = "this is not valid lua";

    auto mappings = LuaInputLoader::parse(luaCode);

    EXPECT_EQ(mappings.size(), 0);  // Should return empty on error
}

TEST(ComponentsTests, LuaInputLoader_EmptyTable) {
    const char* luaCode = "return {}";

    auto mappings = LuaInputLoader::parse(luaCode);

    EXPECT_EQ(mappings.size(), 0);
}

TEST(ComponentsTests, LuaInputLoader_MissingType) {
    const char* luaCode = R"(
        return {
            actions = {
                jump = {
                    { code = 32 },  -- No type field
                }
            }
        }
    )";

    auto mappings = LuaInputLoader::parse(luaCode);

    EXPECT_EQ(mappings.size(), 0);  // Should skip bindings without type
}

TEST(ComponentsTests, LuaInputLoader_MissingCode) {
    const char* luaCode = R"(
        return {
            actions = {
                jump = {
                    { type = "key" },  -- No code field
                }
            }
        }
    )";

    auto mappings = LuaInputLoader::parse(luaCode);

    ASSERT_EQ(mappings.size(), 1);
    EXPECT_EQ(mappings[0].binding.keyCode, 0);  // Default value
}

//==========================================================================
// LuaPhysicsLoader Tests
//==========================================================================

TEST(ComponentsTests, LuaPhysicsLoader_SimpleBody) {
    const char* luaCode = R"(
        return {
            bodies = {
                player = {
                    type = "dynamic",
                    width = 32,
                    height = 48,
                    fixedRotation = true,
                }
            }
        }
    )";

    auto configs = LuaPhysicsLoader::parse(luaCode);

    ASSERT_EQ(configs.size(), 1);
    ASSERT_TRUE(configs.count("player"));
    EXPECT_EQ(configs["player"].type, BodyType::Dynamic);
    EXPECT_FLOAT_EQ(configs["player"].width, 32.0f);
    EXPECT_FLOAT_EQ(configs["player"].height, 48.0f);
    EXPECT_TRUE(configs["player"].fixedRotation);
}

TEST(ComponentsTests, LuaPhysicsLoader_StaticBody) {
    const char* luaCode = R"(
        return {
            bodies = {
                platform = {
                    type = "static",
                    width = 200,
                    height = 32,
                }
            }
        }
    )";

    auto configs = LuaPhysicsLoader::parse(luaCode);

    ASSERT_EQ(configs.size(), 1);
    EXPECT_EQ(configs["platform"].type, BodyType::Static);
}

TEST(ComponentsTests, LuaPhysicsLoader_KinematicBody) {
    const char* luaCode = R"(
        return {
            bodies = {
                movingPlatform = {
                    type = "kinematic",
                    width = 100,
                    height = 20,
                }
            }
        }
    )";

    auto configs = LuaPhysicsLoader::parse(luaCode);

    ASSERT_EQ(configs.size(), 1);
    EXPECT_EQ(configs["movingPlatform"].type, BodyType::Kinematic);
}

TEST(ComponentsTests, LuaPhysicsLoader_PhysicsProperties) {
    const char* luaCode = R"(
        return {
            bodies = {
                ball = {
                    type = "dynamic",
                    width = 16,
                    height = 16,
                    density = 2.5,
                    friction = 0.8,
                    restitution = 0.9,
                    linearDamping = 0.1,
                    angularDamping = 0.2,
                    sensor = true,
                }
            }
        }
    )";

    auto configs = LuaPhysicsLoader::parse(luaCode);

    ASSERT_EQ(configs.size(), 1);
    EXPECT_FLOAT_EQ(configs["ball"].density, 2.5f);
    EXPECT_FLOAT_EQ(configs["ball"].friction, 0.8f);
    EXPECT_FLOAT_EQ(configs["ball"].restitution, 0.9f);
    EXPECT_FLOAT_EQ(configs["ball"].linearDamping, 0.1f);
    EXPECT_FLOAT_EQ(configs["ball"].angularDamping, 0.2f);
    EXPECT_TRUE(configs["ball"].isSensor);
}

TEST(ComponentsTests, LuaPhysicsLoader_CustomLayers) {
    const char* luaCode = R"(
        return {
            layers = {
                Player = 0x0001,
                Enemy = 0x0002,
                Terrain = 0x0008,
            },
            bodies = {
                player = {
                    type = "dynamic",
                    width = 32,
                    height = 48,
                    layer = "Player",
                    mask = { "Terrain", "Enemy" },
                }
            }
        }
    )";

    auto configs = LuaPhysicsLoader::parse(luaCode);

    ASSERT_EQ(configs.size(), 1);
    EXPECT_EQ(configs["player"].layer, 0x0001);
    EXPECT_EQ(configs["player"].mask, 0x0008 | 0x0002);  // Terrain | Enemy
}

TEST(ComponentsTests, LuaPhysicsLoader_SingleLayerString) {
    const char* luaCode = R"(
        return {
            layers = {
                Player = 0x0001,
            },
            bodies = {
                player = {
                    type = "dynamic",
                    width = 32,
                    height = 48,
                    layer = "Player",
                }
            }
        }
    )";

    auto configs = LuaPhysicsLoader::parse(luaCode);

    ASSERT_EQ(configs.size(), 1);
    EXPECT_EQ(configs["player"].layer, 0x0001);
}

TEST(ComponentsTests, LuaPhysicsLoader_NumericLayerMask) {
    const char* luaCode = R"(
        return {
            bodies = {
                player = {
                    type = "dynamic",
                    width = 32,
                    height = 48,
                    layer = 0x0001,
                    mask = 0x000F,
                }
            }
        }
    )";

    auto configs = LuaPhysicsLoader::parse(luaCode);

    ASSERT_EQ(configs.size(), 1);
    EXPECT_EQ(configs["player"].layer, 0x0001);
    EXPECT_EQ(configs["player"].mask, 0x000F);
}

TEST(ComponentsTests, LuaPhysicsLoader_DefaultLayers) {
    const char* luaCode = R"(
        return {
            bodies = {
                player = {
                    type = "dynamic",
                    width = 32,
                    height = 48,
                    layer = "Player",
                }
            }
        }
    )";

    auto configs = LuaPhysicsLoader::parse(luaCode);

    ASSERT_EQ(configs.size(), 1);
    EXPECT_EQ(configs["player"].layer, 0x0001);  // Default Player layer
}

TEST(ComponentsTests, LuaPhysicsLoader_MultipleBodies) {
    const char* luaCode = R"(
        return {
            bodies = {
                player = { type = "dynamic", width = 32, height = 48 },
                enemy = { type = "dynamic", width = 24, height = 32 },
                platform = { type = "static", width = 200, height = 32 },
            }
        }
    )";

    auto configs = LuaPhysicsLoader::parse(luaCode);

    EXPECT_EQ(configs.size(), 3);
    EXPECT_TRUE(configs.count("player"));
    EXPECT_TRUE(configs.count("enemy"));
    EXPECT_TRUE(configs.count("platform"));
}

TEST(ComponentsTests, LuaPhysicsLoader_ToBodyDef) {
    PhysicsBodyConfig config;
    config.type = BodyType::Dynamic;
    config.width = 32.0f;
    config.height = 48.0f;
    config.fixedRotation = true;
    config.density = 2.0f;
    config.friction = 0.5f;
    config.restitution = 0.3f;

    PhysicsBodyDef def = config.toBodyDef(100, 200);

    EXPECT_EQ(def.type, BodyType::Dynamic);
    EXPECT_FLOAT_EQ(def.transform.x, 100.0f);
    EXPECT_FLOAT_EQ(def.transform.y, 200.0f);
    EXPECT_FLOAT_EQ(def.size.x, 32.0f);
    EXPECT_FLOAT_EQ(def.size.y, 48.0f);
    EXPECT_TRUE(def.fixedRotation);
    EXPECT_FLOAT_EQ(def.density, 2.0f);
    EXPECT_FLOAT_EQ(def.friction, 0.5f);
    EXPECT_FLOAT_EQ(def.restitution, 0.3f);
}

TEST(ComponentsTests, LuaPhysicsLoader_InvalidLua) {
    const char* luaCode = "this is not valid lua";

    auto configs = LuaPhysicsLoader::parse(luaCode);

    EXPECT_EQ(configs.size(), 0);  // Should return empty on error
}

TEST(ComponentsTests, LuaPhysicsLoader_EmptyTable) {
    const char* luaCode = "return {}";

    auto configs = LuaPhysicsLoader::parse(luaCode);

    EXPECT_EQ(configs.size(), 0);
}

//==========================================================================
// Keys/ControllerButtons/ControllerAxes Namespace Tests
//==========================================================================

TEST(ComponentsTests, KeyConstants_SampleValues) {
    EXPECT_EQ(Keys::Space, 32);
    EXPECT_EQ(Keys::A, 65);
    EXPECT_EQ(Keys::Z, 90);
    EXPECT_EQ(Keys::Escape, 256);
    EXPECT_EQ(Keys::Enter, 257);
    EXPECT_EQ(Keys::Left, 263);
    EXPECT_EQ(Keys::Right, 262);
    EXPECT_EQ(Keys::Up, 265);
    EXPECT_EQ(Keys::Down, 264);
}

TEST(ComponentsTests, ControllerButtonConstants_SampleValues) {
    EXPECT_EQ(ControllerButtons::A, 0);
    EXPECT_EQ(ControllerButtons::B, 1);
    EXPECT_EQ(ControllerButtons::X, 2);
    EXPECT_EQ(ControllerButtons::Y, 3);
    EXPECT_EQ(ControllerButtons::Start, 6);
}

TEST(ComponentsTests, ControllerAxesConstants_SampleValues) {
    EXPECT_EQ(ControllerAxes::LeftX, 0);
    EXPECT_EQ(ControllerAxes::LeftY, 1);
    EXPECT_EQ(ControllerAxes::RightX, 2);
    EXPECT_EQ(ControllerAxes::RightY, 3);
}

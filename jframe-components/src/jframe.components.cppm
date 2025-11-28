module;

export module jframe.components;

import jframe.types;
import std;

export namespace jframe::components {

//==========================================================================
// Movement Components
//==========================================================================

struct Velocity {
    Vec2 linear{0.0f, 0.0f};
    float angular = 0.0f;
};

struct Acceleration {
    Vec2 value{0.0f, 0.0f};
};

struct MaxSpeed {
    float linear = 500.0f;
    float angular = 360.0f;  // degrees per second
};

//==========================================================================
// Combat Components
//==========================================================================

struct Health {
    int current = 100;
    int maximum = 100;
    float invincibilityTime = 0.0f;
    float invincibilityDuration = 1.0f;

    bool isDead() const { return current <= 0; }
    bool isInvincible() const { return invincibilityTime > 0.0f; }
    float healthPercent() const { return static_cast<float>(current) / maximum; }

    void takeDamage(int amount) {
        if (!isInvincible()) {
            current = std::max(0, current - amount);
            invincibilityTime = invincibilityDuration;
        }
    }

    void heal(int amount) {
        current = std::min(maximum, current + amount);
    }

    void update(float dt) {
        if (invincibilityTime > 0.0f) {
            invincibilityTime -= dt;
        }
    }
};

struct Damage {
    int amount = 10;
    bool destroyOnHit = false;
};

//==========================================================================
// Gameplay Components
//==========================================================================

struct Timer {
    float remaining = 0.0f;
    float duration = 1.0f;
    bool repeating = false;
    bool paused = false;
    std::function<void()> onComplete;

    void start(float time) {
        duration = time;
        remaining = time;
        paused = false;
    }

    void update(float dt) {
        if (paused || remaining <= 0.0f) return;

        remaining -= dt;
        if (remaining <= 0.0f) {
            if (onComplete) onComplete();
            if (repeating) remaining = duration;
        }
    }

    float progress() const { return 1.0f - (remaining / duration); }
    bool isComplete() const { return remaining <= 0.0f && !repeating; }
};

struct Lifetime {
    float remaining = 5.0f;

    bool isExpired() const { return remaining <= 0.0f; }
    void update(float dt) { remaining -= dt; }
};

struct Score {
    int value = 0;
    int multiplier = 1;

    void add(int points) { value += points * multiplier; }
};

//==========================================================================
// Physics Helper Components
//==========================================================================

struct GroundDetector {
    bool isGrounded = false;
    float coyoteTime = 0.0f;
    static constexpr float kCoyoteTimeMax = 0.1f;

    bool canJump() const { return isGrounded || coyoteTime > 0.0f; }

    void update(float dt, bool currentlyGrounded) {
        if (currentlyGrounded) {
            isGrounded = true;
            coyoteTime = kCoyoteTimeMax;
        } else {
            if (isGrounded) {
                // Just left ground, start coyote time
                coyoteTime = kCoyoteTimeMax;
            }
            isGrounded = false;
            coyoteTime = std::max(0.0f, coyoteTime - dt);
        }
    }

    void consumeJump() {
        coyoteTime = 0.0f;
    }
};

struct JumpState {
    int jumpsRemaining = 1;
    int maxJumps = 1;  // Set to 2 for double jump
    float jumpForce = 400.0f;

    bool canJump() const { return jumpsRemaining > 0; }
    void jump() { if (canJump()) jumpsRemaining--; }
    void reset() { jumpsRemaining = maxJumps; }
};

//==========================================================================
// Tag Components
//==========================================================================

struct PlayerTag {};
struct EnemyTag {};
struct CollectibleTag {};
struct ProjectileTag {};
struct PlatformTag {};
struct TriggerTag {};
struct DeadTag {};  // Mark for removal

//==========================================================================
// State Machine Component
//==========================================================================

template<typename StateEnum>
struct StateMachine {
    StateEnum currentState;
    StateEnum previousState;
    float stateTime = 0.0f;

    void setState(StateEnum newState) {
        if (currentState != newState) {
            previousState = currentState;
            currentState = newState;
            stateTime = 0.0f;
        }
    }

    void update(float dt) {
        stateTime += dt;
    }

    bool isState(StateEnum state) const { return currentState == state; }
    bool justEntered() const { return stateTime == 0.0f; }
};

}  // namespace jframe::components

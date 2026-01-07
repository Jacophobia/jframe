// bestow-components/src/bestow.builders.cppm
// Chainable builder APIs for reducing boilerplate

module;

export module bestow.builders;

import std;
import bestow.types;
import bestow.input;
import bestow.physics;

export namespace bestow {

//==========================================================================
// InputMappingBuilder - DEPRECATED: Use ActionBuilder from action_binding.cpp
//==========================================================================
// This builder uses the legacy polling-based input API.
// New code should use the event-driven ActionBuilder API:
//
//   bestow.action.builder()
//       :duringPhase("game")
//       :whenPressed(bestow.input.keys.Space)
//       :emitAction("Jump")
//       :discretely()
//
// Legacy Usage (deprecated):
//   InputMappingBuilder(input)
//       .action("jump")
//           .key(Key::Space)
//           .button(ControllerButton::A)
//       .apply();
//==========================================================================

class [[deprecated("Use ActionBuilder via bestow.action.builder() in Lua instead")]]
InputMappingBuilder {
public:
    explicit InputMappingBuilder(IInputSystem& input)
        : input_(input) {}

    // Start defining a new action
    InputMappingBuilder& action(const std::string& actionName) {
        currentAction_ = actionName;
        return *this;
    }

    // Bind a keyboard key to the current action (using KeyCode)
    InputMappingBuilder& key(KeyCode keyCode, float scale = 1.0f) {
        if (!currentAction_.empty()) {
            InputMapping mapping{
                .binding = {
                    .source = InputSource::Keyboard,
                    .deviceIndex = 0,
                    .input = keyCode,
                    .requiredModifiers = ModifierKey::None,
                    .scale = scale,
                    .deadzone = 0.0f
                },
                .action = currentAction_
            };
            pendingMappings_.push_back(mapping);
        }
        return *this;
    }

    // Legacy int-based key binding (deprecated)
    [[deprecated("Use key(KeyCode) instead")]]
    InputMappingBuilder& key(int keyCode, float scale = 1.0f) {
        return key(static_cast<KeyCode>(keyCode), scale);
    }

    // Bind a mouse button to the current action
    InputMappingBuilder& mouseButton(MouseButton button, float scale = 1.0f) {
        if (!currentAction_.empty()) {
            InputMapping mapping{
                .binding = {
                    .source = InputSource::Mouse,
                    .deviceIndex = 0,
                    .input = button,
                    .requiredModifiers = ModifierKey::None,
                    .scale = scale,
                    .deadzone = 0.0f
                },
                .action = currentAction_
            };
            pendingMappings_.push_back(mapping);
        }
        return *this;
    }

    // Legacy int-based mouse button binding (deprecated)
    [[deprecated("Use mouseButton(MouseButton) instead")]]
    InputMappingBuilder& mouseButton(int button, float scale = 1.0f) {
        return mouseButton(static_cast<MouseButton>(button), scale);
    }

    // Bind a controller button to the current action
    InputMappingBuilder& button(GamepadButton buttonCode, int controllerIndex = 0, float scale = 1.0f) {
        if (!currentAction_.empty()) {
            InputMapping mapping{
                .binding = {
                    .source = InputSource::Gamepad,
                    .deviceIndex = controllerIndex,
                    .input = buttonCode,
                    .requiredModifiers = ModifierKey::None,
                    .scale = scale,
                    .deadzone = 0.1f
                },
                .action = currentAction_
            };
            pendingMappings_.push_back(mapping);
        }
        return *this;
    }

    // Legacy int-based button binding (deprecated)
    [[deprecated("Use button(GamepadButton) instead")]]
    InputMappingBuilder& button(int buttonCode, int controllerIndex = 0, float scale = 1.0f) {
        return button(static_cast<GamepadButton>(buttonCode), controllerIndex, scale);
    }

    // Bind a controller axis to the current action
    InputMappingBuilder& axis(GamepadAxis axisCode, int controllerIndex = 0, float deadzone = 0.1f) {
        if (!currentAction_.empty()) {
            InputMapping mapping{
                .binding = {
                    .source = InputSource::Gamepad,
                    .deviceIndex = controllerIndex,
                    .input = axisCode,
                    .requiredModifiers = ModifierKey::None,
                    .scale = 1.0f,
                    .deadzone = deadzone
                },
                .action = currentAction_
            };
            pendingMappings_.push_back(mapping);
        }
        return *this;
    }

    // Legacy int-based axis binding (deprecated)
    [[deprecated("Use axis(GamepadAxis) instead")]]
    InputMappingBuilder& axis(int axisCode, int controllerIndex = 0, float deadzone = 0.1f) {
        return axis(static_cast<GamepadAxis>(axisCode), controllerIndex, deadzone);
    }

    // Set deadzone for the last added binding
    InputMappingBuilder& deadzone(float dz) {
        if (!pendingMappings_.empty()) {
            pendingMappings_.back().binding.deadzone = dz;
        }
        return *this;
    }

    // Apply all pending mappings to the input system
    void apply() {
        for (const auto& mapping : pendingMappings_) {
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
            input_.registerMapping(mapping);
#pragma clang diagnostic pop
        }
        pendingMappings_.clear();
        currentAction_.clear();
    }

    // Clear without applying
    void clear() {
        pendingMappings_.clear();
        currentAction_.clear();
    }

    // Get pending mappings (for inspection/serialization)
    const std::vector<InputMapping>& getMappings() const {
        return pendingMappings_;
    }

private:
    IInputSystem& input_;
    std::string currentAction_;
    std::vector<InputMapping> pendingMappings_;
};

//==========================================================================
// PhysicsBodyBuilder - Chainable API for physics body creation
//==========================================================================
// Usage:
//   PhysicsBodyBuilder(physics, entity)
//       .dynamic()
//       .position(100, 200)
//       .size(32, 48)
//       .fixedRotation()
//       .layer(CollisionLayers::Player)
//       .create();
//==========================================================================

class PhysicsBodyBuilder {
public:
    PhysicsBodyBuilder(IPhysicsSystem& physics, Entity entity)
        : physics_(physics), entity_(entity) {}

    // Body type setters
    PhysicsBodyBuilder& dynamic() {
        def_.type = BodyType::Dynamic;
        return *this;
    }

    PhysicsBodyBuilder& staticBody() {
        def_.type = BodyType::Static;
        return *this;
    }

    PhysicsBodyBuilder& kinematic() {
        def_.type = BodyType::Kinematic;
        return *this;
    }

    // Position
    PhysicsBodyBuilder& position(float x, float y) {
        def_.transform.x = x;
        def_.transform.y = y;
        return *this;
    }

    PhysicsBodyBuilder& position(Vec2 pos) {
        def_.transform.x = pos.x;
        def_.transform.y = pos.y;
        return *this;
    }

    // Size
    PhysicsBodyBuilder& size(float width, float height) {
        def_.size = {width, height};
        return *this;
    }

    PhysicsBodyBuilder& size(Vec2 sz) {
        def_.size = sz;
        return *this;
    }

    // Rotation
    PhysicsBodyBuilder& rotation(float radians) {
        def_.transform.rotation = radians;
        return *this;
    }

    PhysicsBodyBuilder& fixedRotation(bool fixed = true) {
        def_.fixedRotation = fixed;
        return *this;
    }

    // Physics properties
    PhysicsBodyBuilder& density(float d) {
        def_.density = d;
        return *this;
    }

    PhysicsBodyBuilder& friction(float f) {
        def_.friction = f;
        return *this;
    }

    PhysicsBodyBuilder& restitution(float r) {
        def_.restitution = r;
        return *this;
    }

    PhysicsBodyBuilder& linearDamping(float damping) {
        def_.linearDamping = damping;
        return *this;
    }

    PhysicsBodyBuilder& angularDamping(float damping) {
        def_.angularDamping = damping;
        return *this;
    }

    // Sensor (trigger volume)
    PhysicsBodyBuilder& sensor(bool isSensor = true) {
        def_.isSensor = isSensor;
        return *this;
    }

    // Collision layer this body belongs to
    PhysicsBodyBuilder& layer(CollisionLayer l) {
        layer_ = l;
        return *this;
    }

    // Collision mask (what this body collides with)
    PhysicsBodyBuilder& mask(CollisionMask m) {
        mask_ = m;
        return *this;
    }

    // Create the body and return the entity
    Entity create() {
        physics_.createBody(entity_, def_);

        // Apply layer and mask if set
        if (layer_ != 0xFFFF) {
            physics_.setCollisionLayer(entity_, layer_);
        }
        if (mask_ != 0xFFFF) {
            physics_.setCollisionMask(entity_, mask_);
        }

        return entity_;
    }

    // Get the definition (for inspection)
    const PhysicsBodyDef& getDef() const { return def_; }

private:
    IPhysicsSystem& physics_;
    Entity entity_;
    PhysicsBodyDef def_;
    CollisionLayer layer_ = 0xFFFF;  // Default: belongs to all layers
    CollisionMask mask_ = 0xFFFF;    // Default: collides with everything
};

//==========================================================================
// Convenience factory functions for common body types
//==========================================================================

namespace physics {

// Create a static box (platforms, walls, etc.)
inline PhysicsBodyBuilder staticBox(IPhysicsSystem& physics, Entity entity,
                                    float x, float y, float width, float height) {
    return PhysicsBodyBuilder(physics, entity)
        .staticBody()
        .position(x, y)
        .size(width, height);
}

// Create a dynamic box (player, enemies, etc.)
inline PhysicsBodyBuilder dynamicBox(IPhysicsSystem& physics, Entity entity,
                                     float x, float y, float width, float height) {
    return PhysicsBodyBuilder(physics, entity)
        .dynamic()
        .position(x, y)
        .size(width, height)
        .fixedRotation();
}

// Create a kinematic box (moving platforms, etc.)
inline PhysicsBodyBuilder kinematicBox(IPhysicsSystem& physics, Entity entity,
                                       float x, float y, float width, float height) {
    return PhysicsBodyBuilder(physics, entity)
        .kinematic()
        .position(x, y)
        .size(width, height);
}

// Create a sensor/trigger (checkpoints, pickups, etc.)
inline PhysicsBodyBuilder trigger(IPhysicsSystem& physics, Entity entity,
                                  float x, float y, float width, float height) {
    return PhysicsBodyBuilder(physics, entity)
        .staticBody()
        .position(x, y)
        .size(width, height)
        .sensor();
}

// Create a character body (dynamic with typical platformer settings)
inline PhysicsBodyBuilder character(IPhysicsSystem& physics, Entity entity,
                                    float x, float y, float width, float height) {
    return PhysicsBodyBuilder(physics, entity)
        .dynamic()
        .position(x, y)
        .size(width, height)
        .fixedRotation()
        .friction(0.0f)
        .linearDamping(0.0f);
}

// Create a platform (static with ground layer)
inline PhysicsBodyBuilder platform(IPhysicsSystem& physics, Entity entity,
                                   float x, float y, float width, float height) {
    return PhysicsBodyBuilder(physics, entity)
        .staticBody()
        .position(x, y)
        .size(width, height)
        .friction(0.3f)
        .layer(CollisionLayers::Terrain | CollisionLayers::Ground);
}

}  // namespace physics

//==========================================================================
// DEPRECATED: Legacy input helper functions
// Use ActionBuilder via bestow.action.builder() in Lua instead
//==========================================================================

namespace input {

// DEPRECATED: Create a standard platformer input configuration
// Use bestow.action.builder() in inputs.lua instead
[[deprecated("Use ActionBuilder via bestow.action.builder() in Lua instead")]]
inline void setupPlatformerControls(IInputSystem& input,
                                   KeyCode keyLeft, KeyCode keyRight, KeyCode keyJump,
                                   KeyCode keyDown = KeyCode::Unknown, KeyCode keyAttack = KeyCode::Unknown) {
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
    InputMappingBuilder builder(input);

    builder.action("move_horizontal")
        .key(keyRight, 1.0f)
        .key(keyLeft, -1.0f)
        .axis(GamepadAxis::LeftX);

    builder.action("jump")
        .key(keyJump)
        .button(GamepadButton::A);

    if (keyDown != KeyCode::Unknown) {
        builder.action("crouch")
            .key(keyDown)
            .axis(GamepadAxis::LeftY);
    }

    if (keyAttack != KeyCode::Unknown) {
        builder.action("attack")
            .key(keyAttack)
            .button(GamepadButton::X);
    }

    builder.apply();
#pragma clang diagnostic pop
}

// DEPRECATED: Create WASD + Arrow key configuration
// Use bestow.action.builder() in inputs.lua instead
[[deprecated("Use ActionBuilder via bestow.action.builder() in Lua instead")]]
inline void setupWASDControls(IInputSystem& input, KeyCode jumpKey, KeyCode attackKey = KeyCode::Unknown) {
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
    InputMappingBuilder builder(input);

    // WASD movement
    builder.action("move_horizontal")
        .key(KeyCode::D, 1.0f)
        .key(KeyCode::A, -1.0f)
        .key(KeyCode::Right, 1.0f)
        .key(KeyCode::Left, -1.0f)
        .axis(GamepadAxis::LeftX);

    builder.action("move_vertical")
        .key(KeyCode::S, 1.0f)   // S (down is positive)
        .key(KeyCode::W, -1.0f)  // W
        .key(KeyCode::Down, 1.0f)
        .key(KeyCode::Up, -1.0f)
        .axis(GamepadAxis::LeftY);

    builder.action("jump")
        .key(jumpKey)
        .button(GamepadButton::A);

    if (attackKey != KeyCode::Unknown) {
        builder.action("attack")
            .key(attackKey)
            .button(GamepadButton::X);
    }

    builder.apply();
#pragma clang diagnostic pop
}

}  // namespace input

}  // namespace bestow

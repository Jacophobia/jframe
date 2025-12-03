// jframe-components/src/jframe.builders.cppm
// Chainable builder APIs for reducing boilerplate

module;

#include <string>
#include <vector>

export module jframe.builders;

import jframe.types;
import jframe.input;
import jframe.physics;

export namespace jframe {

//==========================================================================
// InputMappingBuilder - Chainable API for input configuration
//==========================================================================
// Usage:
//   InputMappingBuilder(input)
//       .action("jump")
//           .key(Key::Space)
//           .button(ControllerButton::A)
//       .action("move_horizontal")
//           .key(Key::D, 1.0f)
//           .key(Key::A, -1.0f)
//           .axis(ControllerAxis::LeftX)
//       .apply();
//==========================================================================

class InputMappingBuilder {
public:
    explicit InputMappingBuilder(IInputSystem& input)
        : input_(input) {}

    // Start defining a new action
    InputMappingBuilder& action(const std::string& actionName) {
        currentAction_ = actionName;
        return *this;
    }

    // Bind a keyboard key to the current action
    InputMappingBuilder& key(int keyCode, float scale = 1.0f) {
        if (!currentAction_.empty()) {
            InputMapping mapping{
                .binding = {
                    .deviceType = InputDeviceType::Keyboard,
                    .deviceIndex = 0,
                    .keyCode = keyCode,
                    .scale = scale,
                    .deadzone = 0.0f
                },
                .action = currentAction_
            };
            pendingMappings_.push_back(mapping);
        }
        return *this;
    }

    // Bind a mouse button to the current action
    InputMappingBuilder& mouseButton(int button, float scale = 1.0f) {
        if (!currentAction_.empty()) {
            InputMapping mapping{
                .binding = {
                    .deviceType = InputDeviceType::Mouse,
                    .deviceIndex = 0,
                    .keyCode = button,
                    .scale = scale,
                    .deadzone = 0.0f
                },
                .action = currentAction_
            };
            pendingMappings_.push_back(mapping);
        }
        return *this;
    }

    // Bind a controller button to the current action
    InputMappingBuilder& button(int buttonCode, int controllerIndex = 0, float scale = 1.0f) {
        if (!currentAction_.empty()) {
            InputMapping mapping{
                .binding = {
                    .deviceType = InputDeviceType::Controller,
                    .deviceIndex = controllerIndex,
                    .keyCode = buttonCode,
                    .scale = scale,
                    .deadzone = 0.1f
                },
                .action = currentAction_
            };
            pendingMappings_.push_back(mapping);
        }
        return *this;
    }

    // Bind a controller axis to the current action
    InputMappingBuilder& axis(int axisCode, int controllerIndex = 0, float deadzone = 0.1f) {
        if (!currentAction_.empty()) {
            InputMapping mapping{
                .binding = {
                    .deviceType = InputDeviceType::Controller,
                    .deviceIndex = controllerIndex,
                    .keyCode = axisCode | 0x8000,  // Flag to indicate axis
                    .scale = 1.0f,
                    .deadzone = deadzone
                },
                .action = currentAction_
            };
            pendingMappings_.push_back(mapping);
        }
        return *this;
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
            input_.registerMapping(mapping);
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
// Convenience factory functions for common input patterns
//==========================================================================

namespace input {

// Create a standard platformer input configuration
inline void setupPlatformerControls(IInputSystem& input,
                                   int keyLeft, int keyRight, int keyJump,
                                   int keyDown = -1, int keyAttack = -1) {
    InputMappingBuilder builder(input);

    builder.action("move_horizontal")
        .key(keyRight, 1.0f)
        .key(keyLeft, -1.0f)
        .axis(0);  // Left stick X

    builder.action("jump")
        .key(keyJump)
        .button(0);  // A button (SDL_CONTROLLER_BUTTON_A)

    if (keyDown >= 0) {
        builder.action("crouch")
            .key(keyDown)
            .axis(1);  // Left stick Y (down = positive)
    }

    if (keyAttack >= 0) {
        builder.action("attack")
            .key(keyAttack)
            .button(2);  // X button (SDL_CONTROLLER_BUTTON_X)
    }

    builder.apply();
}

// Create WASD + Arrow key configuration
inline void setupWASDControls(IInputSystem& input, int jumpKey, int attackKey = -1) {
    InputMappingBuilder builder(input);

    // WASD movement
    builder.action("move_horizontal")
        .key(68, 1.0f)   // D
        .key(65, -1.0f)  // A
        .key(262, 1.0f)  // Right arrow
        .key(263, -1.0f) // Left arrow
        .axis(0);

    builder.action("move_vertical")
        .key(83, 1.0f)   // S (down is positive)
        .key(87, -1.0f)  // W
        .key(264, 1.0f)  // Down arrow
        .key(265, -1.0f) // Up arrow
        .axis(1);

    builder.action("jump")
        .key(jumpKey)
        .button(0);  // A button

    if (attackKey >= 0) {
        builder.action("attack")
            .key(attackKey)
            .button(2);  // X button
    }

    builder.apply();
}

}  // namespace input

}  // namespace jframe

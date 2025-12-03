// examples/platformer/src/systems/PlayerMovementSystem.cpp
// Player movement system

import jframe;

namespace platformer {

class PlayerMovementSystem {
public:
    PlayerMovementSystem(jframe::JFrameEngine& engine) : engine_(engine) {}

    void update(jframe::DeltaTime dt) {
        // Get input
        float horizontal = engine_.input->getActionValue("move_horizontal");
        bool jump = engine_.input->wasActionJustPressed("jump");

        // Apply movement to player entity
        // This would use the physics system and entity system
    }

private:
    jframe::JFrameEngine& engine_;
};

}  // namespace platformer

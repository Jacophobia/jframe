// examples/platformer/src/systems/PlayerMovementSystem.cpp
// Player movement system

import std;
import bestow;

namespace platformer {

class PlayerMovementSystem {
public:
    PlayerMovementSystem(bestow::BestowEngine& engine) : engine_(engine) {}

    void update(bestow::DeltaTime dt) {
        // Get input
        float horizontal = engine_.input->getActionValue("move_horizontal");
        bool jump = engine_.input->wasActionJustPressed("jump");

        // Apply movement to player entity
        // This would use the physics system and entity system
    }

private:
    bestow::BestowEngine& engine_;
};

}  // namespace platformer

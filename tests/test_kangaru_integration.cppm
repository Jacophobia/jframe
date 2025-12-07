// Test: Verify Kangaru works with types imported from C++20 modules
// This tests the actual integration scenario where types come from bestow modules

module;

#include <kangaru/kangaru.hpp>

export module test.kangaru_integration;

import std;
import bestow.types;  // Import from our module

export namespace kangaru_test {

// Test using a module-defined type (Vec3 from bestow.types)
struct Position {
    bestow::Vec3 pos{0.0f, 0.0f, 0.0f};
};

struct PositionService : kgr::single_service<Position> {};

// Class depending on module types
struct GameWorld {
    Position& position;

    void printPos() {
        std::cout << "Position: " << position.pos.x << ", "
                  << position.pos.y << ", " << position.pos.z << "\n";
    }
};

struct GameWorldService : kgr::service<GameWorld, kgr::dependency<PositionService>> {};

inline int runTest() {
    std::cout << "Testing Kangaru with C++20 module types...\n";

    kgr::container container;

    // Test module-defined types
    auto& pos = container.service<PositionService>();
    pos.pos = bestow::Vec3{1.0f, 2.0f, 3.0f};

    auto world = container.service<GameWorldService>();
    world.printPos();

    std::cout << "SUCCESS: Kangaru works with C++20 module types!\n";
    return 0;
}

} // namespace kangaru_test

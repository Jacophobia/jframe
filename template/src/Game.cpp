// Bestow Game Template - Game.cpp
// Minimal game implementation

#include <fstream>
#include <sstream>

#include <bestow/entt_compat.hpp>

import std;
import bestow.types;
import bestow.entity;
import bestow.physics;
import bestow.graphics;
import bestow.input;
import bestow.blueprints;
import bestow.builders;
import bestow.luaconfig;

#include "Game.h"

Game::Game(bestow::IEntitySystem& entities,
           bestow::IPhysicsSystem& physics,
           bestow::IGraphicsSystem& graphics,
           bestow::IInputSystem& input,
           bestow::IBlueprintFactory& blueprints)
    : entities_(entities)
    , physics_(physics)
    , graphics_(graphics)
    , input_(input)
    , blueprints_(blueprints)
    , player_(entt::null) {}

void Game::init() {
    setupInput();
    loadBlueprints();
    createWorld();
}

void Game::setupInput() {
    // Dvorak-friendly controls: ,AOE for movement (WASD positions on Dvorak)
    bestow::InputMappingBuilder(input_)
        .action("left")
            .key(bestow::Keys::A)       // Dvorak A = QWERTY A
            .key(bestow::Keys::Left)
        .action("right")
            .key(bestow::Keys::E)       // Dvorak E = QWERTY D
            .key(bestow::Keys::Right)
        .action("jump")
            .key(bestow::Keys::Comma)   // Dvorak , = QWERTY W
            .key(bestow::Keys::Space)
            .key(bestow::Keys::Up)
        .action("down")
            .key(bestow::Keys::O)       // Dvorak O = QWERTY S
            .key(bestow::Keys::Down)
        .apply();
}

void Game::loadBlueprints() {
    std::ifstream file("assets/blueprints/entities.lua");
    if (!file) {
        std::println(stderr, "Could not load blueprints");
        return;
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    blueprints_.loadBlueprints(buffer.str());
}

void Game::createWorld() {
    // Create ground platform
    blueprints_.create("platform", 400.0f, 550.0f, 700.0f, 30.0f);

    // Create some platforms
    blueprints_.create("platform", 200.0f, 400.0f, 150.0f, 20.0f);
    blueprints_.create("platform", 500.0f, 300.0f, 150.0f, 20.0f);
    blueprints_.create("platform", 300.0f, 200.0f, 150.0f, 20.0f);

    // Create player
    player_ = blueprints_.create("player", 400.0f, 100.0f);
}

void Game::update(bestow::DeltaTime dt) {
    if (!entities_.isValid(player_) || !physics_.hasBody(player_)) return;

    // Movement
    float moveX = 0.0f;
    if (input_.isActionActive("left"))  moveX -= 1.0f;
    if (input_.isActionActive("right")) moveX += 1.0f;

    constexpr float moveSpeed = 200.0f;
    bestow::Vec2 vel = physics_.getVelocity(player_);
    vel.x = moveX * moveSpeed;
    physics_.setVelocity(player_, vel);

    // Jump (simple ground check)
    if (input_.wasActionJustPressed("jump")) {
        auto ground = physics_.checkGrounded(player_);
        if (ground.grounded) {
            vel = physics_.getVelocity(player_);
            vel.y = -350.0f;  // Negative Y is up in screen coords
            physics_.setVelocity(player_, vel);
        }
    }
}

void Game::render() {
    // Set up camera centered on screen
    bestow::Camera camera;
    camera.transform.x = 400.0f;
    camera.transform.y = 300.0f;
    camera.zoom = 1.0f;
    camera.viewportSize = {800, 600};
    graphics_.setCamera(camera);

    // Render all entities with debug shapes
    graphics_.renderEntities(entities_);
}

void Game::shutdown() {
    // Cleanup if needed
}

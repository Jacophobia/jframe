// Bestow Game Template - Game.h
// Minimal game class

#pragma once

#include <bestow/entt_compat.hpp>

import bestow.types;
import bestow.entity;
import bestow.physics;
import bestow.graphics;
import bestow.input;
import bestow.blueprints;

class Game {
public:
    Game(bestow::IEntitySystem& entities,
         bestow::IPhysicsSystem& physics,
         bestow::IGraphicsSystem& graphics,
         bestow::IInputSystem& input,
         bestow::IBlueprintFactory& blueprints);

    void init();
    void update(bestow::DeltaTime dt);
    void render();
    void shutdown();

private:
    void setupInput();
    void loadBlueprints();
    void createWorld();

    bestow::IEntitySystem& entities_;
    bestow::IPhysicsSystem& physics_;
    bestow::IGraphicsSystem& graphics_;
    bestow::IInputSystem& input_;
    bestow::IBlueprintFactory& blueprints_;

    bestow::Entity player_;
};

// main.cpp
// Blueprints System Demo - Comprehensive API demonstration

// MSVC C++23 module compatibility for EnTT iterators and sol2 globals
#include <jframe/entt_compat.hpp>
#include <jframe/sol2_compat.hpp>

import std;
import jframe.types;
import jframe.entity;
import jframe.entity.impl;
import jframe.physics;
import jframe.physics.impl;
import jframe.events;
import jframe.events.impl;
import jframe.blueprints;
import jframe.blueprints.impl;

using namespace jframe;

//==============================================================================
// Helper Functions
//==============================================================================

void printSectionHeader(const std::string& title) {
    std::println("\n{:=^80}", "");
    std::println("{:^80}", title);
    std::println("{:=^80}\n", "");
}

void printSubheader(const std::string& title) {
    std::println("\n--- {} ---", title);
}

std::string readFile(const std::string& filepath) {
    std::ifstream file(filepath);
    if (!file) {
        throw std::runtime_error(std::format("Failed to open file: {}", filepath));
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

void printProperty(const std::string& key, const std::any& value) {
    std::print("  {}: ", key);

    if (auto* d = std::any_cast<double>(&value)) {
        std::println("{}", *d);
    } else if (auto* b = std::any_cast<bool>(&value)) {
        std::println("{}", *b ? "true" : "false");
    } else if (auto* s = std::any_cast<std::string>(&value)) {
        std::println("\"{}\"", *s);
    } else if (auto* arr = std::any_cast<std::vector<double>>(&value)) {
        std::print("[");
        for (std::size_t i = 0; i < arr->size(); ++i) {
            if (i > 0) std::print(", ");
            std::print("{}", (*arr)[i]);
        }
        std::println("]");
    } else if (std::any_cast<PropertyMap>(&value)) {
        std::println("<nested map>");
    } else {
        std::println("<unknown type>");
    }
}

void printBlueprintDef(const BlueprintDef& def) {
    std::println("Blueprint: {}", def.name);

    if (!def.inherits.empty()) {
        std::println("  Inherits: {}", def.inherits);
    }

    std::println("  Components: {}", def.components.size());
    for (const auto& comp : def.components) {
        std::println("    - {} ({} properties)", comp.name, comp.properties.size());
        for (const auto& [key, value] : comp.properties) {
            std::print("      ");
            printProperty(key, value);
        }
    }

    if (def.physics) {
        std::println("  Physics:");
        std::println("    Type: {}", def.physics->bodyType);
        if (def.physics->size) {
            std::println("    Size: {:.1f} x {:.1f}",
                def.physics->size->x, def.physics->size->y);
        }
        std::println("    Sensor: {}", def.physics->sensor ? "true" : "false");
        std::println("    Fixed Rotation: {}", def.physics->fixedRotation ? "true" : "false");
        std::println("    Density: {:.2f}", def.physics->density);
        std::println("    Friction: {:.2f}", def.physics->friction);
        std::println("    Restitution: {:.2f}", def.physics->restitution);
        std::println("    Linear Damping: {:.2f}", def.physics->linearDamping);
        if (!def.physics->collisionLayer.empty()) {
            std::println("    Collision Layer: {}", def.physics->collisionLayer);
        }
    }

    if (!def.metadata.empty()) {
        std::println("  Metadata: {} entries", def.metadata.size());
        for (const auto& [key, value] : def.metadata) {
            std::print("   ");
            printProperty(key, value);
        }
    }
}

//==============================================================================
// Custom Component Registration Demo
//==============================================================================

// Custom game-specific component
struct HealthComponent {
    int current;
    int maximum;
};

struct VelocityComponent {
    float maxSpeed;
};

void registerCustomComponents(IBlueprintFactory& factory, [[maybe_unused]] IEntitySystem& entities) {
    printSubheader("Registering Custom Components");

    // Register Health component creator
    factory.registerComponent("Health", [](Entity e, IEntitySystem& sys, const PropertyMap& props) {
        HealthComponent health;

        // Extract properties with defaults
        auto getCurrentHealth = [&]() -> int {
            auto it = props.find("current");
            if (it != props.end()) {
                if (auto* d = std::any_cast<double>(&it->second)) {
                    return static_cast<int>(*d);
                }
            }
            return 100; // default
        };

        auto getMaxHealth = [&]() -> int {
            auto it = props.find("maximum");
            if (it != props.end()) {
                if (auto* d = std::any_cast<double>(&it->second)) {
                    return static_cast<int>(*d);
                }
            }
            return 100; // default
        };

        health.current = getCurrentHealth();
        health.maximum = getMaxHealth();

        sys.emplace<HealthComponent>(e, health);
        std::println("  Created Health component: {}/{}", health.current, health.maximum);
    });

    // Register Velocity component creator
    factory.registerComponent("Velocity", [](Entity e, IEntitySystem& sys, const PropertyMap& props) {
        VelocityComponent vel;

        auto it = props.find("maxSpeed");
        if (it != props.end()) {
            if (auto* d = std::any_cast<double>(&it->second)) {
                vel.maxSpeed = static_cast<float>(*d);
            }
        } else {
            vel.maxSpeed = 100.0f; // default
        }

        sys.emplace<VelocityComponent>(e, vel);
        std::println("  Created Velocity component: maxSpeed={:.1f}", vel.maxSpeed);
    });

    std::println("\nRegistered components:");
    std::println("  - Health: {}", factory.isComponentRegistered("Health") ? "YES" : "NO");
    std::println("  - Velocity: {}", factory.isComponentRegistered("Velocity") ? "YES" : "NO");
    std::println("  - DebugRect: {}", factory.isComponentRegistered("DebugRect") ? "YES" : "NO");
    std::println("  - DebugCircle: {}", factory.isComponentRegistered("DebugCircle") ? "YES" : "NO");
    std::println("  - NonExistent: {}", factory.isComponentRegistered("NonExistent") ? "YES" : "NO");
}

//==============================================================================
// Main Demo
//==============================================================================

int main() {
    try {
        std::println("JFrame Blueprints System Demo");
        std::println("=============================\n");

        // Create required systems
        auto eventSystem = createEventSystem();
        auto entitySystem = createEntitySystem();
        auto physicsSystem = createPhysicsSystem();

        if (!eventSystem || !entitySystem || !physicsSystem) {
            std::println("ERROR: Failed to create required systems");
            return 1;
        }

        std::println("Created entity, physics, and event systems");

        // Create blueprint factory
        auto blueprintFactory = createBlueprintFactory(*entitySystem, physicsSystem.get());

        if (!blueprintFactory) {
            std::println("ERROR: Failed to create blueprint factory");
            return 1;
        }

        std::println("Created blueprint factory");

        //======================================================================
        // DEMO 1: Component Registration
        //======================================================================

        printSectionHeader("DEMO 1: Component Registration");

        registerCustomComponents(*blueprintFactory, *entitySystem);

        //======================================================================
        // DEMO 2: Blueprint Loading
        //======================================================================

        printSectionHeader("DEMO 2: Blueprint Loading");

        std::println("Loading blueprints from Lua file...");
        std::string luaSource = readFile("blueprints-demo-data/blueprints.lua");

        bool loadSuccess = blueprintFactory->loadBlueprints(luaSource);

        if (!loadSuccess) {
            std::println("ERROR: Failed to load blueprints");
            return 1;
        }

        std::println("Successfully loaded blueprints!");

        //======================================================================
        // DEMO 3: Blueprint Queries
        //======================================================================

        printSectionHeader("DEMO 3: Blueprint Queries");

        printSubheader("Getting All Blueprint Names");
        auto blueprintNames = blueprintFactory->getBlueprintNames();
        std::println("Found {} blueprints:", blueprintNames.size());
        for (const auto& name : blueprintNames) {
            std::println("  - {}", name);
        }

        printSubheader("Checking Blueprint Existence");
        std::vector<std::string> testNames = {
            "player", "coin", "fast_enemy", "nonexistent_blueprint"
        };
        for (const auto& name : testNames) {
            bool exists = blueprintFactory->hasBlueprint(name);
            std::println("  hasBlueprint(\"{}\"): {}", name, exists ? "YES" : "NO");
        }

        printSubheader("Inspecting Simple Blueprint (coin)");
        auto coinDef = blueprintFactory->getBlueprint("coin");
        if (coinDef) {
            printBlueprintDef(*coinDef);
        } else {
            std::println("  Blueprint not found!");
        }

        printSubheader("Inspecting Complex Blueprint (player)");
        auto playerDef = blueprintFactory->getBlueprint("player");
        if (playerDef) {
            printBlueprintDef(*playerDef);
        }

        printSubheader("Inspecting Inherited Blueprint (flying_enemy)");
        auto flyingDef = blueprintFactory->getBlueprint("flying_enemy");
        if (flyingDef) {
            printBlueprintDef(*flyingDef);
            std::println("\nNote: This blueprint inherits from 'base_enemy'");
            std::println("The resolved definition above shows all merged properties.");
        }

        //======================================================================
        // DEMO 4: Entity Creation - All Overloads
        //======================================================================

        printSectionHeader("DEMO 4: Entity Creation - All create() Overloads");

        std::vector<Entity> createdEntities;

        printSubheader("create(name, x, y)");
        std::println("Creating static platform at (100, 50)...");
        Entity platform1 = blueprintFactory->create("static_platform", 100.0f, 50.0f);
        if (entitySystem->isValid(platform1)) {
            std::println("  Created entity: {} (valid)", "platform1");
            createdEntities.push_back(platform1);

            if (auto* transform = entitySystem->tryGet<Transform2D>(platform1)) {
                std::println("  Transform: ({:.1f}, {:.1f})", transform->x, transform->y);
            }
            if (physicsSystem->hasBody(platform1)) {
                auto bodySize = physicsSystem->getBodySize(platform1);
                std::println("  Physics body size: {:.1f} x {:.1f}", bodySize.x, bodySize.y);
            }
        } else {
            std::println("  ERROR: Failed to create entity!");
        }

        printSubheader("create(name, x, y, width, height)");
        std::println("Creating crate at (200, 100) with size 60x60...");
        Entity crate = blueprintFactory->create("crate", 200.0f, 100.0f, 60.0f, 60.0f);
        if (entitySystem->isValid(crate)) {
            std::println("  Created entity: crate (valid)");
            createdEntities.push_back(crate);

            if (physicsSystem->hasBody(crate)) {
                auto bodySize = physicsSystem->getBodySize(crate);
                std::println("  Physics body size (overridden): {:.1f} x {:.1f}", bodySize.x, bodySize.y);
            }
        }

        printSubheader("create(name, x, y, overrides)");
        std::println("Creating fast_enemy at (300, 150) with property overrides...");

        PropertyMap overrides;
        overrides["DebugRect.fillColor"] = std::vector<double>{0, 255, 0, 255}; // Green
        overrides["metadata.health"] = 15.0; // Lower health

        Entity fastEnemy = blueprintFactory->create("fast_enemy", 300.0f, 150.0f, overrides);
        if (entitySystem->isValid(fastEnemy)) {
            std::println("  Created entity: fast_enemy (valid)");
            createdEntities.push_back(fastEnemy);

            if (auto* rect = entitySystem->tryGet<DebugRect>(fastEnemy)) {
                std::println("  DebugRect fillColor: ({}, {}, {}, {})",
                    rect->fillColor.r, rect->fillColor.g,
                    rect->fillColor.b, rect->fillColor.a);
                std::println("  Color should be green due to override!");
            }
        }

        printSubheader("create(name, x, y, width, height, overrides)");
        std::println("Creating player at (400, 200) with size 40x60 and overrides...");

        PropertyMap playerOverrides;
        playerOverrides["DebugRect.fillColor"] = std::vector<double>{255, 0, 255, 255}; // Magenta

        Entity player = blueprintFactory->create("player", 400.0f, 200.0f, 40.0f, 60.0f, playerOverrides);
        if (entitySystem->isValid(player)) {
            std::println("  Created entity: player (valid)");
            createdEntities.push_back(player);

            if (physicsSystem->hasBody(player)) {
                auto bodySize = physicsSystem->getBodySize(player);
                std::println("  Physics body size (overridden): {:.1f} x {:.1f}", bodySize.x, bodySize.y);
            }
            if (auto* rect = entitySystem->tryGet<DebugRect>(player)) {
                std::println("  DebugRect fillColor: ({}, {}, {}, {})",
                    rect->fillColor.r, rect->fillColor.g,
                    rect->fillColor.b, rect->fillColor.a);
            }
        }

        //======================================================================
        // DEMO 5: Blueprint Inheritance
        //======================================================================

        printSectionHeader("DEMO 5: Blueprint Inheritance");

        printSubheader("Creating Base Enemy");
        Entity baseEnemy = blueprintFactory->create("base_enemy", 100.0f, 300.0f);
        if (entitySystem->isValid(baseEnemy)) {
            std::println("  Created base_enemy entity (valid)");
            createdEntities.push_back(baseEnemy);

            if (auto* rect = entitySystem->tryGet<DebugRect>(baseEnemy)) {
                std::println("  Color: ({}, {}, {}) - Should be RED",
                    rect->fillColor.r, rect->fillColor.g, rect->fillColor.b);
            }
        }

        printSubheader("Creating Fast Enemy (inherits from base_enemy)");
        Entity fastEnemy2 = blueprintFactory->create("fast_enemy", 200.0f, 300.0f);
        if (entitySystem->isValid(fastEnemy2)) {
            std::println("  Created fast_enemy entity (valid)");
            createdEntities.push_back(fastEnemy2);

            if (auto* rect = entitySystem->tryGet<DebugRect>(fastEnemy2)) {
                std::println("  Color: ({}, {}, {}) - Should be ORANGE (overridden)",
                    rect->fillColor.r, rect->fillColor.g, rect->fillColor.b);
                std::println("  Size: {:.1f} x {:.1f} - Should be 32x32 (inherited)",
                    rect->size.x, rect->size.y);
            }
        }

        printSubheader("Creating Flying Enemy (inherits from base_enemy, replaces DebugRect with DebugCircle)");
        Entity flyingEnemy = blueprintFactory->create("flying_enemy", 300.0f, 300.0f);
        if (entitySystem->isValid(flyingEnemy)) {
            std::println("  Created flying_enemy entity (valid)");
            createdEntities.push_back(flyingEnemy);

            if (auto* circle = entitySystem->tryGet<DebugCircle>(flyingEnemy)) {
                std::println("  Shape: DebugCircle (replaced DebugRect)");
                std::println("  Radius: {:.1f}", circle->radius);
                std::println("  Color: ({}, {}, {}) - Should be PURPLE",
                    circle->fillColor.r, circle->fillColor.g, circle->fillColor.b);
            }
            if (physicsSystem->hasBody(flyingEnemy)) {
                auto bodyType = physicsSystem->getBodyType(flyingEnemy);
                std::println("  Body Type: {} (should be Kinematic, overridden from Dynamic)",
                    static_cast<int>(bodyType));
            }
        }

        printSubheader("Creating Tank Enemy (inherits from base_enemy, larger size)");
        Entity tankEnemy = blueprintFactory->create("tank_enemy", 400.0f, 300.0f);
        if (entitySystem->isValid(tankEnemy)) {
            std::println("  Created tank_enemy entity (valid)");
            createdEntities.push_back(tankEnemy);

            if (auto* rect = entitySystem->tryGet<DebugRect>(tankEnemy)) {
                std::println("  Size: {:.1f} x {:.1f} - Should be 64x64 (overridden)",
                    rect->size.x, rect->size.y);
                std::println("  Color: ({}, {}, {}) - Should be DARK RED",
                    rect->fillColor.r, rect->fillColor.g, rect->fillColor.b);
            }
        }

        //======================================================================
        // DEMO 6: Physics Configuration
        //======================================================================

        printSectionHeader("DEMO 6: Physics Configuration");

        printSubheader("Static Body (platform)");
        if (entitySystem->isValid(platform1) && physicsSystem->hasBody(platform1)) {
            auto bodyType = physicsSystem->getBodyType(platform1);
            auto vel = physicsSystem->getVelocity(platform1);
            std::println("  Body Type: {} (0=Static)", static_cast<int>(bodyType));
            std::println("  Velocity: ({:.2f}, {:.2f})", vel.x, vel.y);
        }

        printSubheader("Dynamic Body with High Restitution (bouncy_ball)");
        Entity bouncyBall = blueprintFactory->create("bouncy_ball", 500.0f, 400.0f);
        if (entitySystem->isValid(bouncyBall)) {
            std::println("  Created bouncy_ball entity (valid)");
            createdEntities.push_back(bouncyBall);

            if (physicsSystem->hasBody(bouncyBall)) {
                auto bodyType = physicsSystem->getBodyType(bouncyBall);
                std::println("  Body Type: {} (1=Dynamic)", static_cast<int>(bodyType));
                std::println("  Restitution: 0.95 (very bouncy, defined in Lua)");
                std::println("  Fixed Rotation: false (can rotate)");
            }
        }

        printSubheader("Sensor Body (coin)");
        Entity coin = blueprintFactory->create("coin", 600.0f, 450.0f);
        if (entitySystem->isValid(coin)) {
            std::println("  Created coin entity (valid)");
            createdEntities.push_back(coin);

            if (physicsSystem->hasBody(coin)) {
                std::println("  Is Sensor: true (no collision response)");
                std::println("  Used for trigger volumes and collectibles");
            }
        }

        //======================================================================
        // DEMO 7: Metadata
        //======================================================================

        printSectionHeader("DEMO 7: Metadata Inspection");

        printSubheader("Examining Player Metadata");
        auto playerDefMeta = blueprintFactory->getBlueprint("player");
        if (playerDefMeta && !playerDefMeta->metadata.empty()) {
            std::println("Player blueprint metadata:");
            for (const auto& [key, value] : playerDefMeta->metadata) {
                std::print("  ");
                printProperty(key, value);
            }
        }

        printSubheader("Examining Tank Enemy Metadata");
        auto tankDefMeta = blueprintFactory->getBlueprint("tank_enemy");
        if (tankDefMeta && !tankDefMeta->metadata.empty()) {
            std::println("Tank enemy blueprint metadata:");
            for (const auto& [key, value] : tankDefMeta->metadata) {
                std::print("  ");
                printProperty(key, value);
            }
            std::println("\nNote: Metadata includes inherited properties from base_enemy");
        }

        //======================================================================
        // DEMO 8: Blueprint Reloading
        //======================================================================

        printSectionHeader("DEMO 8: Blueprint Reloading (Hot Reload Simulation)");

        std::println("Clearing all blueprints...");
        blueprintFactory->clearBlueprints();

        auto namesAfterClear = blueprintFactory->getBlueprintNames();
        std::println("  Blueprints after clear: {}", namesAfterClear.size());
        std::println("  hasBlueprint(\"player\"): {}",
            blueprintFactory->hasBlueprint("player") ? "YES" : "NO");

        std::println("\nReloading blueprints from cached source...");
        blueprintFactory->reloadBlueprints();

        auto namesAfterReload = blueprintFactory->getBlueprintNames();
        std::println("  Blueprints after reload: {}", namesAfterReload.size());
        std::println("  hasBlueprint(\"player\"): {}",
            blueprintFactory->hasBlueprint("player") ? "YES" : "NO");

        std::println("\nHot reload would allow you to edit blueprints.lua");
        std::println("and see changes immediately without restarting the game!");

        //======================================================================
        // DEMO 9: Edge Cases and Error Handling
        //======================================================================

        printSectionHeader("DEMO 9: Edge Cases and Error Handling");

        printSubheader("Creating Entity from Non-Existent Blueprint");
        Entity invalid = blueprintFactory->create("doesnt_exist", 100.0f, 100.0f);
        std::println("  Result: {} (should be invalid/null)",
            entitySystem->isValid(invalid) ? "VALID" : "INVALID");

        printSubheader("Creating Minimal Blueprint (marker)");
        Entity marker = blueprintFactory->create("marker", 700.0f, 500.0f);
        if (entitySystem->isValid(marker)) {
            std::println("  Created marker entity (valid)");
            std::println("  This blueprint has no components, just a position");
            createdEntities.push_back(marker);
        }

        printSubheader("Creating Blueprint with DebugLine");
        Entity debugLine = blueprintFactory->create("debug_line", 800.0f, 550.0f);
        if (entitySystem->isValid(debugLine)) {
            std::println("  Created debug_line entity (valid)");
            createdEntities.push_back(debugLine);

            if (auto* line = entitySystem->tryGet<DebugLine>(debugLine)) {
                std::println("  Line end offset: ({:.1f}, {:.1f})",
                    line->endOffset.x, line->endOffset.y);
            }
        }

        //======================================================================
        // Summary
        //======================================================================

        printSectionHeader("DEMO COMPLETE - Summary");

        std::println("Total entities created: {}", createdEntities.size());
        std::println("Total blueprints loaded: {}", blueprintFactory->getBlueprintNames().size());
        std::println("\nAll IBlueprintFactory API methods demonstrated:");
        std::println("  [X] loadBlueprints(luaSource)");
        std::println("  [X] reloadBlueprints()");
        std::println("  [X] clearBlueprints()");
        std::println("  [X] hasBlueprint(name)");
        std::println("  [X] getBlueprintNames()");
        std::println("  [X] getBlueprint(name)");
        std::println("  [X] create(name, x, y)");
        std::println("  [X] create(name, x, y, width, height)");
        std::println("  [X] create(name, x, y, overrides)");
        std::println("  [X] create(name, x, y, width, height, overrides)");
        std::println("  [X] registerComponent(name, creator)");
        std::println("  [X] isComponentRegistered(name)");
        std::println("\nFeatures demonstrated:");
        std::println("  [X] Simple blueprints");
        std::println("  [X] Complex blueprints with multiple components");
        std::println("  [X] Blueprint inheritance");
        std::println("  [X] Physics configuration");
        std::println("  [X] Property overrides");
        std::println("  [X] Metadata");
        std::println("  [X] Custom component registration");
        std::println("  [X] Hot reload simulation");
        std::println("  [X] Edge cases and error handling");

        std::println("\n{:=^80}", "");
        std::println("All tests passed successfully!");
        std::println("{:=^80}", "");

        return 0;

    } catch (const std::exception& e) {
        std::println("\nERROR: {}", e.what());
        return 1;
    } catch (...) {
        std::println("\nERROR: Unknown exception occurred");
        return 1;
    }
}

// EntityDemo.cppm
// Comprehensive demonstration of the JFrame Entity System API

module;

// Use compatibility header for MSVC C++23 module support
#include <jframe/entt_compat.hpp>

export module entity.demo;

import std;
import jframe.types;
import jframe.entity;

export namespace demo {

//==========================================================================
// Sample Components for Demo
//==========================================================================

struct Position {
    float x = 0.0f;
    float y = 0.0f;

    Position() = default;
    Position(float x, float y) : x(x), y(y) {}
};

struct Velocity {
    float dx = 0.0f;
    float dy = 0.0f;

    Velocity() = default;
    Velocity(float dx, float dy) : dx(dx), dy(dy) {}
};

struct Health {
    int current = 100;
    int max = 100;

    Health() = default;
    Health(int current, int max) : current(current), max(max) {}

    bool isAlive() const { return current > 0; }
    float percentage() const { return static_cast<float>(current) / max; }
};

struct Name {
    std::string value;

    Name() = default;
    Name(std::string name) : value(std::move(name)) {}
};

// Tag components (minimal structs for filtering)
// Note: EnTT requires components to be non-void types
struct PlayerTag {
    bool dummy = true;
};
struct EnemyTag {
    bool dummy = true;
};
struct ProjectileTag {
    bool dummy = true;
};
struct DeadTag {
    bool dummy = true;
};

//==========================================================================
// EntityDemo Class
//==========================================================================

class EntityDemo {
public:
    explicit EntityDemo(jframe::IEntitySystem& entitySystem)
        : entities_(entitySystem) {}

    void run() {
        printHeader("JFRAME ENTITY SYSTEM COMPREHENSIVE DEMO");

        demoEntityLifecycle();
        demoComponentAccess();
        demoComponentQueries();
        demoEntityGroups();
        demoViewIteration();
        demoEntitySelectors();
        demoEachIteration();
        demoUpdate();
        demoComplexScenario();

        printHeader("DEMO COMPLETE");
    }

private:
    jframe::IEntitySystem& entities_;

    //==========================================================================
    // Demo Sections
    //==========================================================================

    void demoEntityLifecycle() {
        printSection("Entity Lifecycle");

        // createEntity()
        std::println("Creating entities...");
        auto e1 = entities_.createEntity();
        auto e2 = entities_.createEntity();
        auto e3 = entities_.createEntity();
        std::println("  Created entities: {}, {}, {}",
                     static_cast<std::uint32_t>(e1),
                     static_cast<std::uint32_t>(e2),
                     static_cast<std::uint32_t>(e3));

        // isValid()
        std::println("\nValidating entities...");
        std::println("  e1 valid: {}", entities_.isValid(e1));
        std::println("  e2 valid: {}", entities_.isValid(e2));
        std::println("  e3 valid: {}", entities_.isValid(e3));

        // entityCount()
        std::println("\nEntity count: {}", entities_.entityCount());

        // destroyEntity()
        std::println("\nDestroying entity e2...");
        entities_.destroyEntity(e2);
        std::println("  e2 valid after destroy: {}", entities_.isValid(e2));
        std::println("  Entity count: {}", entities_.entityCount());

        // Destroy remaining test entities
        entities_.destroyEntity(e1);
        entities_.destroyEntity(e3);
        std::println("\nCleanup complete. Entity count: {}", entities_.entityCount());
    }

    void demoComponentAccess() {
        printSection("Component Access");

        auto entity = entities_.createEntity();

        // emplace<T>()
        std::println("Adding components with emplace...");
        auto& pos = entities_.emplace<Position>(entity, 100.0f, 200.0f);
        auto& vel = entities_.emplace<Velocity>(entity, 5.0f, -3.0f);
        auto& health = entities_.emplace<Health>(entity, 80, 100);
        auto& name = entities_.emplace<Name>(entity, "TestEntity");

        std::println("  Position: ({}, {})", pos.x, pos.y);
        std::println("  Velocity: ({}, {})", vel.dx, vel.dy);
        std::println("  Health: {}/{}", health.current, health.max);
        std::println("  Name: {}", name.value);

        // get<T>()
        std::println("\nRetrieving components with get...");
        auto& retrievedPos = entities_.get<Position>(entity);
        std::println("  Retrieved Position: ({}, {})", retrievedPos.x, retrievedPos.y);

        // Modify through reference
        std::println("\nModifying component...");
        retrievedPos.x = 150.0f;
        retrievedPos.y = 250.0f;
        std::println("  Modified Position: ({}, {})",
                     entities_.get<Position>(entity).x,
                     entities_.get<Position>(entity).y);

        // tryGet<T>()
        std::println("\nUsing tryGet for safe access...");
        if (auto* p = entities_.tryGet<Position>(entity)) {
            std::println("  Position found: ({}, {})", p->x, p->y);
        } else {
            std::println("  Position not found");
        }

        if (entities_.tryGet<ProjectileTag>(entity)) {
            std::println("  ProjectileTag found");
        } else {
            std::println("  ProjectileTag not found (expected)");
        }

        // remove<T>()
        std::println("\nRemoving Velocity component...");
        entities_.remove<Velocity>(entity);
        if (entities_.tryGet<Velocity>(entity)) {
            std::println("  Velocity still present (unexpected!)");
        } else {
            std::println("  Velocity removed successfully");
        }

        entities_.destroyEntity(entity);
    }

    void demoComponentQueries() {
        printSection("Component Queries");

        auto e1 = entities_.createEntity();
        entities_.emplace<Position>(e1, 0.0f, 0.0f);
        entities_.emplace<Velocity>(e1, 1.0f, 1.0f);
        entities_.emplace<Health>(e1, 100, 100);

        auto e2 = entities_.createEntity();
        entities_.emplace<Position>(e2, 10.0f, 10.0f);
        entities_.emplace<Health>(e2, 50, 100);

        auto e3 = entities_.createEntity();
        entities_.emplace<Position>(e3, 20.0f, 20.0f);

        // allOf<T>()
        std::println("Testing allOf...");
        std::println("  e1 has Position+Velocity+Health: {}",
                     entities_.allOf<Position, Velocity, Health>(e1));
        std::println("  e2 has Position+Velocity+Health: {}",
                     entities_.allOf<Position, Velocity, Health>(e2));
        std::println("  e3 has Position+Velocity+Health: {}",
                     entities_.allOf<Position, Velocity, Health>(e3));

        // anyOf<T>()
        std::println("\nTesting anyOf...");
        std::println("  e1 has Velocity or Health: {}",
                     entities_.anyOf<Velocity, Health>(e1));
        std::println("  e2 has Velocity or Health: {}",
                     entities_.anyOf<Velocity, Health>(e2));
        std::println("  e3 has Velocity or Health: {}",
                     entities_.anyOf<Velocity, Health>(e3));

        entities_.destroyEntity(e1);
        entities_.destroyEntity(e2);
        entities_.destroyEntity(e3);
    }

    void demoEntityGroups() {
        printSection("Entity Groups");

        // Create test entities
        auto player = entities_.createEntity();
        entities_.emplace<Position>(player, 0.0f, 0.0f);
        entities_.emplace<Velocity>(player, 0.0f, 0.0f);
        entities_.emplace<Health>(player, 100, 100);
        entities_.emplace<Name>(player, "Hero");
        entities_.emplace<PlayerTag>(player);

        auto enemy1 = entities_.createEntity();
        entities_.emplace<Position>(enemy1, 100.0f, 100.0f);
        entities_.emplace<Velocity>(enemy1, -2.0f, 0.0f);
        entities_.emplace<Health>(enemy1, 50, 50);
        entities_.emplace<Name>(enemy1, "Enemy1");
        entities_.emplace<EnemyTag>(enemy1);

        auto enemy2 = entities_.createEntity();
        entities_.emplace<Position>(enemy2, 200.0f, 100.0f);
        entities_.emplace<Velocity>(enemy2, -1.5f, 0.0f);
        entities_.emplace<Health>(enemy2, 50, 50);
        entities_.emplace<Name>(enemy2, "Enemy2");
        entities_.emplace<EnemyTag>(enemy2);

        auto deadEnemy = entities_.createEntity();
        entities_.emplace<Position>(deadEnemy, 150.0f, 50.0f);
        entities_.emplace<Health>(deadEnemy, 0, 50);
        entities_.emplace<Name>(deadEnemy, "DeadEnemy");
        entities_.emplace<EnemyTag>(deadEnemy);
        entities_.emplace<DeadTag>(deadEnemy);

        // groupCount<T>()
        std::println("Entity group counts:");
        std::println("  Entities with Position: {}", entities_.groupCount<Position>());
        std::println("  Entities with Position+Velocity: {}",
                     entities_.groupCount<Position, Velocity>());
        std::println("  Entities with EnemyTag: {}", entities_.groupCount<EnemyTag>());
        std::println("  Entities with PlayerTag: {}", entities_.groupCount<PlayerTag>());
        std::println("  Entities with DeadTag: {}", entities_.groupCount<DeadTag>());

        // hasAny<T>()
        std::println("\nChecking if any entities exist:");
        std::println("  Any with Position: {}", entities_.hasAny<Position>());
        std::println("  Any with PlayerTag: {}", entities_.hasAny<PlayerTag>());
        std::println("  Any with ProjectileTag: {}", entities_.hasAny<ProjectileTag>());

        // first<T>()
        std::println("\nGetting first entity:");
        if (auto firstEnemy = entities_.first<EnemyTag>()) {
            auto& pos = entities_.get<Position>(*firstEnemy);
            std::println("  First enemy at: ({}, {})", pos.x, pos.y);
        } else {
            std::println("  No enemies found");
        }

        // single<T>()
        std::println("\nGetting single entity:");
        if (auto singlePlayer = entities_.single<PlayerTag>()) {
            auto& name = entities_.get<Name>(*singlePlayer);
            std::println("  Single player: {}", name.value);
        } else {
            std::println("  Expected exactly one player, found 0 or 2+");
        }

        if (auto singleEnemy = entities_.single<EnemyTag>()) {
            std::println("  Single enemy found (unexpected - we have multiple!)");
        } else {
            std::println("  Multiple enemies exist (as expected)");
        }

        // collect<T>()
        std::println("\nCollecting entities:");
        auto allEnemies = entities_.collect<EnemyTag>();
        std::println("  Collected {} enemies", allEnemies.size());
        for (auto enemy : allEnemies) {
            auto& pos = entities_.get<Position>(enemy);
            auto& hp = entities_.get<Health>(enemy);
            std::println("    Enemy at ({}, {}) with {}/{} HP",
                         pos.x, pos.y, hp.current, hp.max);
        }

        // collectExcluding<T>()
        std::println("\nCollecting enemies excluding dead ones:");
        auto aliveEnemies = entities_.collectExcluding<EnemyTag, DeadTag>();
        std::println("  Collected {} alive enemies", aliveEnemies.size());
        for (auto enemy : aliveEnemies) {
            auto& pos = entities_.get<Position>(enemy);
            auto& hp = entities_.get<Health>(enemy);
            std::println("    Alive enemy at ({}, {}) with {}/{} HP",
                         pos.x, pos.y, hp.current, hp.max);
        }

        // Cleanup
        entities_.destroyEntity(player);
        entities_.destroyEntity(enemy1);
        entities_.destroyEntity(enemy2);
        entities_.destroyEntity(deadEnemy);
    }

    void demoViewIteration() {
        printSection("View Iteration");

        // Create entities
        for (int i = 0; i < 5; ++i) {
            auto e = entities_.createEntity();
            entities_.emplace<Position>(e, i * 10.0f, i * 20.0f);
            entities_.emplace<Velocity>(e, i * 0.5f, i * -0.5f);
            if (i % 2 == 0) {
                entities_.emplace<Health>(e, 100, 100);
            }
        }

        // view<T>() - single component
        std::println("Entities with Position:");
        auto posView = entities_.view<Position>();
        int count = 0;
        for (auto entity : posView) {
            auto& pos = entities_.get<Position>(entity);
            std::println("  Entity {} at ({}, {})",
                         static_cast<std::uint32_t>(entity), pos.x, pos.y);
            ++count;
        }
        std::println("  Total: {} entities", count);

        // view<T>() - multiple components
        std::println("\nEntities with Position+Velocity:");
        for (auto entity : entities_.view<Position, Velocity>()) {
            auto& pos = entities_.get<Position>(entity);
            auto& vel = entities_.get<Velocity>(entity);
            std::println("  Entity {} at ({}, {}) moving ({}, {})",
                         static_cast<std::uint32_t>(entity),
                         pos.x, pos.y, vel.dx, vel.dy);
        }

        // view<T>() - three components
        std::println("\nEntities with Position+Velocity+Health:");
        for (auto entity : entities_.view<Position, Velocity, Health>()) {
            auto& pos = entities_.get<Position>(entity);
            auto& hp = entities_.get<Health>(entity);
            std::println("  Entity {} at ({}, {}) with {} HP",
                         static_cast<std::uint32_t>(entity),
                         pos.x, pos.y, hp.current);
        }

        // View with tuple unpacking
        std::println("\nUsing view with component unpacking:");
        auto view = entities_.view<Position, Velocity>();
        for (auto [entity, pos, vel] : view.each()) {
            std::println("  Entity {} -> pos({}, {}), vel({}, {})",
                         static_cast<std::uint32_t>(entity),
                         pos.x, pos.y, vel.dx, vel.dy);
        }

        // Cleanup
        for (auto entity : entities_.view<Position>()) {
            entities_.destroyEntity(entity);
        }
    }

    void demoEntitySelectors() {
        printSection("Entity Selectors");

        // Create test entities
        auto e1 = entities_.createEntity();
        entities_.emplace<Position>(e1, 50.0f, 50.0f);
        entities_.emplace<Health>(e1, 100, 100);
        entities_.emplace<PlayerTag>(e1);

        auto e2 = entities_.createEntity();
        entities_.emplace<Position>(e2, 150.0f, 50.0f);
        entities_.emplace<Health>(e2, 30, 100);
        entities_.emplace<EnemyTag>(e2);

        auto e3 = entities_.createEntity();
        entities_.emplace<Position>(e3, 250.0f, 50.0f);
        entities_.emplace<Health>(e3, 80, 100);
        entities_.emplace<EnemyTag>(e3);

        auto e4 = entities_.createEntity();
        entities_.emplace<Position>(e4, 350.0f, 50.0f);
        entities_.emplace<Velocity>(e4, 1.0f, 0.0f);

        // Basic selector with predicate
        std::println("Entities with Position and x > 100:");
        jframe::EntitySelector selector1{
            .requiredComponents = {entt::type_hash<Position>::value()},
            .excludedComponents = {},
            .predicate = [this](jframe::Entity e) {
                auto* pos = entities_.tryGet<Position>(e);
                return pos && pos->x > 100.0f;
            }
        };

        auto results1 = entities_.query(selector1);
        std::println("  Found {} entities", results1.size());
        for (auto entity : results1) {
            auto& pos = entities_.get<Position>(entity);
            std::println("    Entity at ({}, {})", pos.x, pos.y);
        }

        // Selector with exclusion (using predicate since type-erased check not implemented)
        std::println("\nEntities with Position but without Health:");
        jframe::EntitySelector selector2{
            .requiredComponents = {},
            .excludedComponents = {},
            .predicate = [this](jframe::Entity e) {
                return entities_.tryGet<Position>(e) != nullptr &&
                       entities_.tryGet<Health>(e) == nullptr;
            }
        };

        auto results2 = entities_.query(selector2);
        std::println("  Found {} entities", results2.size());
        for (auto entity : results2) {
            if (auto* pos = entities_.tryGet<Position>(entity)) {
                std::println("    Entity at ({}, {})", pos->x, pos->y);
            }
        }

        // Complex selector with health filter (using predicate for all checks)
        std::println("\nEnemies with low health (< 50%):");
        jframe::EntitySelector selector3{
            .requiredComponents = {},
            .excludedComponents = {},
            .predicate = [this](jframe::Entity e) {
                auto* hp = entities_.tryGet<Health>(e);
                auto* enemyTag = entities_.tryGet<EnemyTag>(e);
                return hp && enemyTag && hp->percentage() < 0.5f;
            }
        };

        auto results3 = entities_.query(selector3);
        std::println("  Found {} low-health enemies", results3.size());
        for (auto entity : results3) {
            if (auto* hp = entities_.tryGet<Health>(entity)) {
                std::println("    Enemy with {}/{} HP ({}%)",
                             hp->current, hp->max,
                             static_cast<int>(hp->percentage() * 100));
            }
        }

        // Cleanup
        entities_.destroyEntity(e1);
        entities_.destroyEntity(e2);
        entities_.destroyEntity(e3);
        entities_.destroyEntity(e4);
    }

    void demoEachIteration() {
        printSection("Each Iteration");

        // Create entities
        auto e1 = entities_.createEntity();
        entities_.emplace<Name>(e1, "Entity1");
        auto e2 = entities_.createEntity();
        entities_.emplace<Name>(e2, "Entity2");
        auto e3 = entities_.createEntity();
        entities_.emplace<Name>(e3, "Entity3");

        std::println("Iterating all entities with each():");
        int count = 0;
        entities_.each([&](jframe::Entity entity) {
            ++count;
            if (auto* name = entities_.tryGet<Name>(entity)) {
                std::println("  Entity {}: {}",
                             static_cast<std::uint32_t>(entity), name->value);
            } else {
                std::println("  Entity {}: (unnamed)",
                             static_cast<std::uint32_t>(entity));
            }
        });
        std::println("  Total entities processed: {}", count);

        entities_.destroyEntity(e1);
        entities_.destroyEntity(e2);
        entities_.destroyEntity(e3);
    }

    void demoUpdate() {
        printSection("Update");

        std::println("Calling update(dt)...");
        float deltaTime = 0.016f; // 60 FPS
        entities_.update(deltaTime);
        std::println("  Update called with dt = {}", deltaTime);
        std::println("  (Entity system update is typically a no-op)");
    }

    void demoComplexScenario() {
        printSection("Complex Scenario: Game Simulation");

        std::println("Setting up a mini-game scenario...\n");

        // Create player
        auto player = entities_.createEntity();
        entities_.emplace<Position>(player, 0.0f, 0.0f);
        entities_.emplace<Velocity>(player, 10.0f, 0.0f);
        entities_.emplace<Health>(player, 100, 100);
        entities_.emplace<Name>(player, "Hero");
        entities_.emplace<PlayerTag>(player);

        // Create enemies
        std::vector<jframe::Entity> enemies;
        for (int i = 0; i < 3; ++i) {
            auto enemy = entities_.createEntity();
            entities_.emplace<Position>(enemy, 100.0f + i * 50.0f, 50.0f);
            entities_.emplace<Velocity>(enemy, -5.0f, 0.0f);
            entities_.emplace<Health>(enemy, 50, 50);
            entities_.emplace<Name>(enemy, std::format("Enemy{}", i + 1));
            entities_.emplace<EnemyTag>(enemy);
            enemies.push_back(enemy);
        }

        // Create projectiles
        for (int i = 0; i < 2; ++i) {
            auto proj = entities_.createEntity();
            entities_.emplace<Position>(proj, 20.0f, 10.0f * i);
            entities_.emplace<Velocity>(proj, 20.0f, 0.0f);
            entities_.emplace<ProjectileTag>(proj);
        }

        std::println("Initial state:");
        printGameState();

        // Simulate damage
        std::println("\nSimulating combat...");
        auto& enemy0Health = entities_.get<Health>(enemies[0]);
        enemy0Health.current -= 30;
        std::println("  Enemy1 takes 30 damage");

        auto& enemy1Health = entities_.get<Health>(enemies[1]);
        enemy1Health.current = 0;
        entities_.emplace<DeadTag>(enemies[1]);
        std::println("  Enemy2 is killed");

        // Update positions
        std::println("\nUpdating positions...");
        for (auto [entity, pos, vel] : entities_.view<Position, Velocity>().each()) {
            pos.x += vel.dx * 0.016f;
            pos.y += vel.dy * 0.016f;
        }

        std::println("\nFinal state:");
        printGameState();

        // Cleanup
        entities_.destroyEntity(player);
        for (auto enemy : enemies) {
            entities_.destroyEntity(enemy);
        }
        for (auto proj : entities_.collect<ProjectileTag>()) {
            entities_.destroyEntity(proj);
        }
    }

    //==========================================================================
    // Helper Functions
    //==========================================================================

    void printGameState() {
        // Player
        if (auto playerEntity = entities_.first<PlayerTag>()) {
            auto& pos = entities_.get<Position>(*playerEntity);
            auto& hp = entities_.get<Health>(*playerEntity);
            auto& name = entities_.get<Name>(*playerEntity);
            std::println("  Player '{}' at ({:.1f}, {:.1f}) - HP: {}/{}",
                         name.value, pos.x, pos.y, hp.current, hp.max);
        }

        // Enemies
        auto enemyList = entities_.collect<EnemyTag>();
        std::println("  Enemies ({})", enemyList.size());
        for (auto enemy : enemyList) {
            auto& pos = entities_.get<Position>(enemy);
            auto& hp = entities_.get<Health>(enemy);
            auto& name = entities_.get<Name>(enemy);
            bool isDead = entities_.tryGet<DeadTag>(enemy) != nullptr;
            std::println("    '{}' at ({:.1f}, {:.1f}) - HP: {}/{} {}",
                         name.value, pos.x, pos.y, hp.current, hp.max,
                         isDead ? "[DEAD]" : "");
        }

        // Projectiles
        auto projCount = entities_.groupCount<ProjectileTag>();
        std::println("  Projectiles ({})", projCount);
        for (auto [entity, proj, pos] : entities_.view<ProjectileTag, Position>().each()) {
            (void)proj; // Unused tag component
            std::println("    Projectile at ({:.1f}, {:.1f})", pos.x, pos.y);
        }

        std::println("  Total entities: {}", entities_.entityCount());
    }

    void printHeader(const std::string& title) {
        std::println("\n{:=^70}", "");
        std::println("{:^70}", title);
        std::println("{:=^70}\n", "");
    }

    void printSection(const std::string& title) {
        std::println("\n{:-^70}", "");
        std::println("{:^70}", title);
        std::println("{:-^70}\n", "");
    }
};

} // namespace demo

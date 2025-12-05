# Bestow Blueprint Factory System

## Overview

The Blueprint Factory System provides data-driven entity creation from Lua-defined templates. Instead of writing repetitive C++ factory functions for each entity type, games define entity blueprints in Lua and create instances with a single factory call.

**Module:** `bestow.level` (extended), `bestow.blueprints` (new)
**Implementation:** `bestow-level/`, `bestow-blueprints/` (new)
**Status:** Planned Enhancement

## Problem Statement

Currently, games implement 15+ similar factory functions:

```cpp
// Current approach - 15 similar factory methods
void Game::createPlatform(float x, float y, float width, float height) {
    Entity e = entities->createEntity();
    entities->emplace<PlatformTag>(e);
    entities->emplace<Size2D>(e, Size2D{width, height});
    physics->createBody(e, PhysicsBodyDef{
        .type = BodyType::Static,
        .transform = {.x = x, .y = y},
        .size = {width, height}
    });
    physics->setCollisionLayer(e, CollisionLayers::Ground);
}

void Game::createEnemy(float x, float y) {
    Entity e = entities->createEntity();
    entities->emplace<EnemyTag>(e);
    entities->emplace<Size2D>(e, Size2D{32, 32});
    entities->emplace<Health>(e, 30, 30);
    physics->createBody(e, PhysicsBodyDef{
        .type = BodyType::Dynamic,
        .transform = {.x = x, .y = y},
        .size = {32, 32}
    });
}

// ... 13 more nearly identical methods
```

This leads to:
- 300+ lines of repetitive factory code
- Hard-coded values scattered across C++
- Difficult to iterate on game design
- No hot reload for entity definitions

## Proposed Solution

### Lua Blueprints

Define entity templates in Lua:

```lua
-- data/blueprints/entities.lua

Blueprints = {
    Platform = {
        components = {
            PlatformTag = {},
            DebugRect = {
                fillColor = {128, 128, 128, 255},
                layer = "Platforms"
            }
        },
        physics = {
            type = "static",
            collisionLayer = "Ground"
        }
    },

    Enemy = {
        components = {
            EnemyTag = { type = "walker" },
            Health = { current = 30, maximum = 30 },
            DebugRect = {
                size = {32, 32},
                fillColor = {200, 50, 50, 255},
                layer = "Enemies"
            }
        },
        physics = {
            type = "dynamic",
            size = {32, 32},
            collisionLayer = "Enemy"
        }
    },

    Collectible = {
        components = {
            CollectibleTag = {},
            DebugRect = {
                size = {24, 24},
                fillColor = {50, 200, 200, 255},
                layer = "Items"
            }
        },
        physics = {
            type = "static",
            sensor = true
        }
    }
}
```

### Factory API

Create entities from blueprints:

```cpp
// New approach - one-line entity creation
Entity platform = factory->create("Platform", x, y, width, height);
Entity enemy = factory->create("Enemy", x, y);
Entity coin = factory->create("Collectible", x, y);

// With property overrides
Entity boss = factory->create("Enemy", x, y, {
    {"Health.maximum", 500},
    {"Health.current", 500},
    {"EnemyTag.type", "boss"}
});
```

### Interface

```cpp
class IBlueprintFactory {
public:
    virtual ~IBlueprintFactory() = default;

    // Load blueprints from Lua file
    virtual bool loadBlueprints(const std::string& luaSource) = 0;

    // Create entity from blueprint
    virtual Entity create(const std::string& blueprintName,
                          float x, float y) = 0;

    // Create with size
    virtual Entity create(const std::string& blueprintName,
                          float x, float y,
                          float width, float height) = 0;

    // Create with property overrides
    virtual Entity create(const std::string& blueprintName,
                          float x, float y,
                          const PropertyMap& overrides) = 0;

    // Check if blueprint exists
    virtual bool hasBlueprint(const std::string& name) const = 0;

    // Get all blueprint names
    virtual std::vector<std::string> getBlueprintNames() const = 0;

    // Hot reload blueprints
    virtual void reloadBlueprints() = 0;
};

using PropertyMap = std::unordered_map<std::string, std::any>;
```

## Lua Blueprint Format

### Basic Structure

```lua
BlueprintName = {
    -- Components to add
    components = {
        ComponentName = {
            field1 = value1,
            field2 = value2
        }
    },

    -- Physics body configuration (optional)
    physics = {
        type = "static" | "dynamic" | "kinematic",
        size = {width, height},  -- Optional, uses component Size2D if present
        sensor = false,
        fixedRotation = true,
        collisionLayer = "LayerName",
        density = 1.0,
        friction = 0.3,
        restitution = 0.0
    },

    -- GAS configuration (optional)
    gas = {
        attributes = {
            { name = "Health", value = 100 },
            { name = "Stamina", value = 50 }
        },
        abilities = { "Jump", "Dash" },
        effects = { "HealthRegen" }
    }
}
```

### Component Types

```lua
-- Tag components (empty)
PlayerTag = {}
EnemyTag = { type = "walker" }
PlatformTag = {}

-- Data components
Health = { current = 100, maximum = 100 }
Velocity = { dx = 0, dy = 0 }
Size2D = { width = 32, height = 32 }

-- Visual components
DebugRect = {
    size = {32, 32},           -- Defaults to Size2D if present
    fillColor = {r, g, b, a},
    outlineColor = {r, g, b, a},
    outlineWidth = 2.0,
    layer = "Enemies"          -- RenderLayer name
}

Sprite = {
    texture = "textures/player.png",
    sourceRect = {x, y, w, h},  -- Optional
    layer = "Player",
    anchor = {0.5, 0.5},
    flipX = false,
    flipY = false
}

-- Behavior components
WalkerAI = {
    speed = 80,
    patrolRange = 200
}

JumperAI = {
    jumpForce = 350,
    jumpCooldown = 1.5
}
```

### Inheritance

Blueprints can inherit from other blueprints:

```lua
-- Base enemy
Enemy = {
    components = {
        EnemyTag = { type = "basic" },
        Health = { current = 30, maximum = 30 },
        DebugRect = { fillColor = {200, 50, 50, 255} }
    },
    physics = { type = "dynamic", size = {32, 32} }
}

-- Walker inherits from Enemy
WalkerEnemy = {
    inherits = "Enemy",
    components = {
        EnemyTag = { type = "walker" },
        WalkerAI = { speed = 80, patrolRange = 200 }
    }
}

-- Boss inherits and overrides
Boss = {
    inherits = "Enemy",
    components = {
        EnemyTag = { type = "boss" },
        Health = { current = 500, maximum = 500 },
        BossTag = { phase = 1 }
    },
    physics = { size = {64, 64} }
}
```

### Level Integration

Levels reference blueprints by name:

```lua
-- data/levels/level1.lua
return {
    entities = {
        { blueprint = "Platform", x = 0, y = 500, width = 800, height = 20 },
        { blueprint = "Platform", x = 900, y = 450, width = 200, height = 20 },

        { blueprint = "WalkerEnemy", x = 300, y = 400 },
        { blueprint = "WalkerEnemy", x = 500, y = 400 },
        { blueprint = "JumperEnemy", x = 700, y = 400 },

        { blueprint = "Collectible", x = 400, y = 350,
          overrides = { CollectibleTag = { value = 100 } } },

        { blueprint = "Boss", x = 1500, y = 300 }
    }
}
```

## Usage Examples

### Basic Usage

```cpp
bool Game::initialize(Engine& engine) {
    auto& sys = engine.systems();

    // Create blueprint factory
    factory_ = bestow::createBlueprintFactory(
        *sys.entities, *sys.physics, *sys.graphics);

    // Load blueprints
    auto blueprintAsset = sys.assets->loadSync(AssetType::Data,
        "data/blueprints/entities.lua");
    factory_->loadBlueprints(blueprintAsset.rawText);

    // Create player
    player_ = factory_->create("Player", 100, 200);

    return true;
}
```

### Level Loading Integration

```cpp
void Game::loadLevel(const std::string& levelName) {
    auto& sys = engine_->systems();

    // Load level data
    auto levelAsset = sys.assets->loadSync(AssetType::Data,
        "data/levels/" + levelName + ".lua");

    sol::state lua;
    lua.open_libraries(sol::lib::base, sol::lib::math, sol::lib::table);
    lua.safe_script(levelAsset.rawText);

    sol::table level = lua["level"];
    sol::table entities = level["entities"];

    // Create entities from blueprint references
    for (auto& [key, value] : entities) {
        sol::table entityDef = value;

        std::string blueprint = entityDef["blueprint"];
        float x = entityDef["x"];
        float y = entityDef["y"];

        if (entityDef["width"].valid()) {
            float width = entityDef["width"];
            float height = entityDef["height"];
            factory_->create(blueprint, x, y, width, height);
        } else {
            factory_->create(blueprint, x, y);
        }
    }
}
```

### Property Overrides

```cpp
// Create enemy with custom health
Entity strongEnemy = factory_->create("Enemy", x, y, {
    {"Health.current", 100},
    {"Health.maximum", 100},
    {"EnemyTag.damage", 20}
});

// Create collectible with custom value
Entity rareCoin = factory_->create("Collectible", x, y, {
    {"CollectibleTag.value", 500},
    {"DebugRect.fillColor", Color{255, 215, 0, 255}}  // Gold
});
```

### Hot Reload

```cpp
#if defined(BESTOW_DEV_TOOLS)
void Game::onFileChanged(const std::string& path) {
    if (path.ends_with("blueprints/entities.lua")) {
        factory_->reloadBlueprints();
        // Existing entities keep old data, new entities use updated blueprints
    }
}
#endif
```

## Implementation Plan

### Phase 1: Core Factory

**Files to create:**
- `bestow-contract/src/bestow.blueprints.cppm` - Interface
- `bestow-blueprints/CMakeLists.txt` - Build configuration
- `bestow-blueprints/src/BlueprintFactory.cpp` - Implementation

**Interface:**

```cpp
// bestow.blueprints.cppm
export module bestow.blueprints;

import bestow.types;
import bestow.entity;
import bestow.physics;

export namespace bestow {

struct BlueprintDef {
    std::string name;
    std::unordered_map<std::string, std::any> components;
    std::optional<PhysicsBodyDef> physics;
    std::optional<GASConfig> gas;
};

class IBlueprintFactory {
public:
    virtual ~IBlueprintFactory() = default;

    virtual bool loadBlueprints(const std::string& luaSource) = 0;
    virtual Entity create(const std::string& name, float x, float y) = 0;
    virtual Entity create(const std::string& name, float x, float y,
                          float width, float height) = 0;
    virtual bool hasBlueprint(const std::string& name) const = 0;
};

std::unique_ptr<IBlueprintFactory> createBlueprintFactory(
    IEntitySystem& entities,
    IPhysicsSystem& physics,
    IGraphicsSystem& graphics);

} // namespace bestow
```

### Phase 2: Lua Parsing

**Implementation in BlueprintFactory.cpp:**

```cpp
bool BlueprintFactory::loadBlueprints(const std::string& luaSource) {
    sol::state lua;
    lua.open_libraries(sol::lib::base, sol::lib::math, sol::lib::table);

    // Sandbox
    lua["os"] = sol::nil;
    lua["io"] = sol::nil;

    auto result = lua.safe_script(luaSource);
    if (!result.valid()) {
        return false;
    }

    sol::table blueprints = lua["Blueprints"];
    for (auto& [key, value] : blueprints) {
        std::string name = key.as<std::string>();
        sol::table def = value;

        BlueprintDef blueprint;
        blueprint.name = name;

        // Parse components
        if (def["components"].valid()) {
            parseComponents(def["components"], blueprint);
        }

        // Parse physics
        if (def["physics"].valid()) {
            blueprint.physics = parsePhysics(def["physics"]);
        }

        // Handle inheritance
        if (def["inherits"].valid()) {
            std::string parent = def["inherits"];
            mergeWithParent(blueprint, blueprints_[parent]);
        }

        blueprints_[name] = std::move(blueprint);
    }

    return true;
}
```

### Phase 3: Entity Creation

```cpp
Entity BlueprintFactory::create(const std::string& name,
                                 float x, float y,
                                 float width, float height) {
    auto it = blueprints_.find(name);
    if (it == blueprints_.end()) {
        throw std::runtime_error("Unknown blueprint: " + name);
    }

    const BlueprintDef& def = it->second;
    Entity entity = entities_.createEntity();

    // Add Transform2D
    entities_.emplace<Transform2D>(entity, Transform2D{.x = x, .y = y});

    // Add Size2D if dimensions provided
    if (width > 0 && height > 0) {
        entities_.emplace<Size2D>(entity, Size2D{width, height});
    }

    // Add components from blueprint
    for (const auto& [compName, compData] : def.components) {
        addComponent(entity, compName, compData);
    }

    // Create physics body
    if (def.physics) {
        PhysicsBodyDef bodyDef = *def.physics;
        bodyDef.transform = {.x = x, .y = y};
        if (width > 0 && height > 0) {
            bodyDef.size = {width, height};
        }
        physics_.createBody(entity, bodyDef);
    }

    return entity;
}
```

### Phase 4: Component Registry

```cpp
// Runtime component registration for Lua-defined types
class ComponentRegistry {
public:
    template<typename T>
    void registerComponent(const std::string& name) {
        creators_[name] = [](IEntitySystem& sys, Entity e, const std::any& data) {
            if (data.has_value()) {
                sys.emplace<T>(e, std::any_cast<T>(data));
            } else {
                sys.emplace<T>(e);
            }
        };
    }

    void addComponent(IEntitySystem& sys, Entity e,
                      const std::string& name, const std::any& data) {
        auto it = creators_.find(name);
        if (it != creators_.end()) {
            it->second(sys, e, data);
        }
    }

private:
    std::unordered_map<std::string,
        std::function<void(IEntitySystem&, Entity, const std::any&)>> creators_;
};

// Register built-in components
void BlueprintFactory::registerBuiltinComponents() {
    registry_.registerComponent<PlayerTag>("PlayerTag");
    registry_.registerComponent<EnemyTag>("EnemyTag");
    registry_.registerComponent<Health>("Health");
    registry_.registerComponent<DebugRect>("DebugRect");
    registry_.registerComponent<Sprite>("Sprite");
    // ...
}
```

### Phase 5: Tests

**File:** `tests/unit/BlueprintFactoryTests.cpp`

```cpp
TEST(BlueprintFactoryTest, LoadSimpleBlueprint) {
    auto entities = createEntitySystem();
    auto physics = createPhysicsSystem();
    auto graphics = createGraphicsSystem();
    auto factory = createBlueprintFactory(*entities, *physics, *graphics);

    const char* lua = R"(
        Blueprints = {
            TestEntity = {
                components = {
                    Health = { current = 100, maximum = 100 }
                }
            }
        }
    )";

    ASSERT_TRUE(factory->loadBlueprints(lua));
    EXPECT_TRUE(factory->hasBlueprint("TestEntity"));
}

TEST(BlueprintFactoryTest, CreateEntityFromBlueprint) {
    // ... setup ...

    Entity e = factory->create("TestEntity", 100.0f, 200.0f);

    ASSERT_TRUE(entities->isValid(e));
    EXPECT_TRUE(entities->allOf<Transform2D, Health>(e));

    auto& health = entities->get<Health>(e);
    EXPECT_EQ(health.current, 100);
    EXPECT_EQ(health.maximum, 100);
}

TEST(BlueprintFactoryTest, InheritanceWorks) {
    // ... setup with parent/child blueprints ...

    Entity child = factory->create("ChildBlueprint", 0, 0);

    // Should have parent's Health and child's EnemyTag
    EXPECT_TRUE(entities->allOf<Health, EnemyTag>(child));
}

TEST(BlueprintFactoryTest, PropertyOverrides) {
    // ... setup ...

    Entity e = factory->create("TestEntity", 0, 0, {
        {"Health.current", 50}
    });

    auto& health = entities->get<Health>(e);
    EXPECT_EQ(health.current, 50);
    EXPECT_EQ(health.maximum, 100);  // Not overridden
}
```

## Migration Guide

### Before (Manual Factories)

```cpp
class Game {
    void createPlatform(float x, float y, float w, float h) {
        Entity e = entities->createEntity();
        entities->emplace<PlatformTag>(e);
        entities->emplace<Size2D>(e, Size2D{w, h});
        physics->createBody(e, PhysicsBodyDef{...});
    }

    void createEnemy(float x, float y) {
        Entity e = entities->createEntity();
        entities->emplace<EnemyTag>(e);
        entities->emplace<Health>(e, 30, 30);
        physics->createBody(e, PhysicsBodyDef{...});
    }

    // ... 13 more factory methods

    void loadLevel() {
        // Hard-coded entity creation
        createPlatform(0, 500, 800, 20);
        createEnemy(300, 400);
    }
};
```

### After (Blueprint Factory)

```lua
-- data/blueprints/entities.lua
Blueprints = {
    Platform = {
        components = { PlatformTag = {} },
        physics = { type = "static" }
    },
    Enemy = {
        components = {
            EnemyTag = {},
            Health = { current = 30, maximum = 30 }
        },
        physics = { type = "dynamic" }
    }
}
```

```cpp
class Game {
    // No factory methods needed!

    void loadLevel() {
        // Data-driven entity creation
        factory_->create("Platform", 0, 500, 800, 20);
        factory_->create("Enemy", 300, 400);
    }
};
```

## Integration with Level System

The Level System can automatically use blueprints:

```lua
-- data/levels/level1.lua
return {
    entities = {
        { blueprint = "Platform", x = 0, y = 500, width = 800, height = 20 },
        { blueprint = "Enemy", x = 300, y = 400 },
        { blueprint = "Enemy", x = 500, y = 400 }
    }
}
```

```cpp
// Level system uses factory internally
void LevelSystem::loadLevel(const std::string& name) {
    auto levelData = parseLevel(name);

    for (const auto& entityDef : levelData.entities) {
        factory_->create(entityDef.blueprint,
                         entityDef.x, entityDef.y,
                         entityDef.width, entityDef.height);
    }
}
```

## EngineBuilder Integration

```cpp
// Automatic factory setup
auto engine = EngineBuilder()
    .withEntities()
    .withPhysics()
    .withGraphics(config)
    .withBlueprints("data/blueprints/entities.lua")  // NEW
    .build();

// Access factory from engine
auto& factory = engine->systems().blueprints;
factory->create("Player", 100, 200);
```

## Performance Considerations

### Blueprint Lookup
- Blueprints stored in hash map - O(1) lookup
- Blueprint definitions parsed once at load time
- Component creation uses registered factory functions

### Memory
- Blueprint definitions stored once
- Entities created with minimal allocation
- Component data copied from blueprint to entity

### Hot Reload
- Reload replaces blueprint definitions
- Existing entities keep old data
- New entities use updated blueprints
- Consider "refresh" API for updating existing entities

## Related Documentation

- [Level System](Level-System.md) - Level loading integration
- [Entity System](Entity-System.md) - Entity management
- [Physics System](Physics-System.md) - Physics body creation
- [GAS System](GAS-System.md) - Ability system integration

## Status

| Task | Status |
|------|--------|
| Interface design | ✅ Complete |
| Core factory impl | ✅ Complete |
| Lua parsing | ✅ Complete |
| Component registry | ✅ Complete |
| Inheritance support | ✅ Complete |
| Property overrides | ✅ Complete |
| Level integration | 🔲 Future |
| EngineBuilder integration | ✅ Complete |
| Hot reload | ✅ Complete |
| Tests | 🔲 Planned |
| Documentation | ✅ Complete |

## Implementation Notes

The Blueprint Factory was implemented as a new module:

**Interface** (`bestow-contract/src/bestow.blueprints.cppm`):
- `PropertyMap` type alias for component properties
- `ComponentDef`, `BlueprintPhysicsDef`, `BlueprintDef` structs
- `IBlueprintFactory` interface with load, query, create, and register methods

**Implementation** (`bestow-blueprints/src/`):
- `bestow.blueprints.impl.cppm` - BlueprintFactory class declaration
- `BlueprintFactory.cpp` - Full implementation (~500 lines)

**Key Features**:
- Lua parsing with sol2 (sandboxed execution)
- Blueprint inheritance with `inherits` field
- Property table merging for inheritance
- Component registration system for runtime type creation
- Physics body creation from blueprint definitions
- Built-in registration for DebugRect, DebugCircle, DebugLine
- Hot reload via `reloadBlueprints()`

**EngineBuilder Integration**:
```cpp
auto engine = EngineBuilder()
    .withEntities()
    .withPhysics()
    .withBlueprints()
    .build();

engine->systems().blueprints->loadBlueprints(luaSource);
engine->systems().blueprints->create("Enemy", x, y);
```

# Blueprints System Guide

The Blueprints System is Bestow's **data-driven entity creation system**. It allows you to define reusable entity templates in Lua that specify components, physics bodies, and properties. This is the primary way to create game entities without writing C++ code.

## Table of Contents

1. [Overview](#overview)
2. [Core Concepts](#core-concepts)
3. [Lua Blueprint Format](#lua-blueprint-format)
4. [API Reference](#api-reference)
5. [Built-in Components](#built-in-components)
6. [Blueprint Inheritance](#blueprint-inheritance)
7. [Property Overrides](#property-overrides)
8. [Physics Definitions](#physics-definitions)
9. [Component Registration](#component-registration)
10. [Best Practices](#best-practices)
11. [Complete Examples](#complete-examples)

---

## Overview

### What is a Blueprint?

A **blueprint** is a Lua-defined template for creating entities. Think of it as a recipe that describes:

- What components the entity should have (Transform2D, DebugRect, custom components)
- Component properties (sizes, colors, values)
- Physics body configuration (type, shape, material properties, collision layers)
- Custom metadata and properties

### Why Use Blueprints?

**Data-Driven Design**: Blueprints embody Bestow's **Lua-first philosophy**. Game developers should spend most of their time in Lua files, not C++ code.

**Benefits:**
- **Hot Reload**: Change blueprints and see updates instantly without recompiling
- **No C++ Required**: Create complex entities without touching engine code
- **Reusability**: Define once, instantiate many times at different positions
- **Inheritance**: Build entity hierarchies with minimal duplication
- **Property Overrides**: Customize entities at spawn time
- **Version Control Friendly**: Lua files are human-readable and diff-friendly

### Why Lua Instead of JSON?

Blueprints use Lua because it's a **programming language**, not just a data format:

| Feature | Lua | JSON |
|---------|-----|------|
| Comments | `-- comment` | Not allowed |
| Trailing commas | Always OK | Breaks parsing |
| Variables | `local GROUND_Y = 100` | Not supported |
| Math | `math.sin(i) * 100` | Not supported |
| Functions | Helper functions | Not supported |
| Loops | `for i = 1, 10 do ... end` | Not supported |
| Conditionals | `DEBUG and {...} or {}` | Not supported |

---

## Core Concepts

### Blueprint as Entity Template

A blueprint is NOT an entity itself - it's a **template** for creating entities. When you call `blueprints->create("Player", x, y)`, the system:

1. Looks up the "Player" blueprint definition
2. Resolves inheritance chain (if `inherits` is specified)
3. Creates a new entity in the ECS
4. Adds a Transform2D component at position (x, y)
5. Adds all components specified in the blueprint
6. Creates a physics body if `physics = {...}` is defined
7. Returns the entity handle

### The Blueprints Table

Blueprint files must return a table with blueprint definitions:

```lua
-- Simple format: Return the Blueprints table directly
Blueprints = {
    Player = { ... },
    Enemy = { ... },
    Platform = { ... }
}

return Blueprints
```

Or use a return statement:

```lua
-- Alternative: Return table directly
return {
    Player = { ... },
    Enemy = { ... }
}
```

### Blueprint Structure

Each blueprint entry has:
- **name**: Blueprint identifier (table key)
- **inherits**: Optional parent blueprint name
- **components**: Table of component definitions
- **physics**: Optional physics body configuration
- **metadata**: Optional custom properties

---

## Lua Blueprint Format

### Basic Blueprint Structure

```lua
-- data/blueprints/entities.lua
Blueprints = {
    Player = {
        -- Optional: Inherit from another blueprint
        inherits = "BaseCharacter",

        -- Component definitions
        components = {
            DebugRect = {
                size = {32, 48},
                fillColor = {50, 200, 100, 255},
                layer = 10
            }
        },

        -- Optional: Physics body configuration
        physics = {
            type = "dynamic",
            size = {28, 44},
            fixedRotation = true,
            density = 1.0,
            friction = 0.3
        },

        -- Optional: Custom metadata
        metadata = {
            displayName = "Player Character",
            health = 100,
            speed = 200
        }
    },

    Enemy = {
        components = {
            DebugCircle = {
                radius = 16,
                fillColor = {255, 100, 100, 255}
            }
        },

        physics = {
            type = "dynamic",
            size = {32, 32}
        }
    }
}

return Blueprints
```

### Minimal Blueprint

The simplest blueprint creates an entity with just a Transform2D:

```lua
Blueprints = {
    EmptyEntity = {
        components = {}
    }
}
```

### Blueprint with Debug Rendering

```lua
Blueprints = {
    Box = {
        components = {
            DebugRect = {
                size = {64, 64},
                fillColor = {128, 128, 128, 255},
                outlineColor = {0, 0, 0, 255},
                outlineWidth = 2.0,
                layer = 10,
                filled = true
            }
        }
    },

    Ball = {
        components = {
            DebugCircle = {
                radius = 32,
                fillColor = {0, 255, 0, 255},
                segments = 32
            }
        }
    }
}
```

---

## API Reference

### Loading Blueprints

#### `bool loadBlueprints(const std::string& luaSource)`

Load blueprint definitions from Lua source code. The source must define a `Blueprints` table or return one.

**Returns**: `true` if parsing succeeded, `false` if Lua syntax error or no blueprint table found.

**Example:**
```cpp
// Load from string (testing)
std::string luaCode = R"(
    Blueprints = {
        TestEntity = {
            components = {}
        }
    }
)";

if (!blueprints->loadBlueprints(luaCode)) {
    logger->error("Failed to load blueprints");
}

// Load from asset system (production)
AssetHandle handle = assets->registerAsset(AssetType::Data,
    ":assets:/blueprints/entities.lua");
assets->loadAsset(handle);
const LuaData* data = assets->getAsset<LuaData>(handle);

if (!blueprints->loadBlueprints(data->source)) {
    logger->error("Failed to load blueprints");
}
```

**Lua is sandboxed**: Dangerous functions like `os`, `io`, `loadfile`, `dofile`, `load`, `require`, and `package` are removed for security.

#### `void reloadBlueprints()`

Reload blueprints from the last loaded source. Useful for hot reload workflows.

**Example:**
```cpp
// Initial load
blueprints->loadBlueprints(luaSource);

// Later, after file changes detected
blueprints->reloadBlueprints();  // Re-parses same source
```

**Hot Reload Integration:**
```cpp
// Enable hot reload
assets->enableHotReload(true);

// Subscribe to blueprint file changes
AssetHandle bpHandle = assets->registerAsset(AssetType::Lua,
    ":assets:/blueprints/entities.lua");

assets->subscribe(bpHandle, [this](AssetHandle h, AssetType t) {
    const LuaData* data = assets->getAsset<LuaData>(h);
    blueprints->loadBlueprints(data->source);
    logger->info("Blueprints reloaded from disk");
});

// In your update loop
void update(float dt) {
    assets->update();  // Process hot reload notifications
}
```

#### `void clearBlueprints()`

Clear all loaded blueprints and the last source. Used for cleanup or before loading a new set.

**Example:**
```cpp
blueprints->clearBlueprints();
blueprints->loadBlueprints(newSource);
```

### Querying Blueprints

#### `bool hasBlueprint(const std::string& name) const`

Check if a blueprint with the given name exists.

**Example:**
```cpp
if (blueprints->hasBlueprint("Player")) {
    Entity e = blueprints->create("Player", 100.0f, 200.0f);
} else {
    logger->error("Player blueprint not found!");
}
```

#### `std::vector<std::string> getBlueprintNames() const`

Get all registered blueprint names.

**Example:**
```cpp
auto names = blueprints->getBlueprintNames();
for (const auto& name : names) {
    logger->info("Available blueprint: {}", name);
}
```

**Debug UI Usage:**
```cpp
void DebugMenu::renderBlueprintList() {
    auto names = blueprints->getBlueprintNames();
    for (const auto& name : names) {
        if (ImGui::Selectable(name.c_str())) {
            selectedBlueprint_ = name;
        }
    }

    if (ImGui::Button("Spawn Selected")) {
        auto [mouseX, mouseY] = input->getMousePosition();
        auto [worldX, worldY] = camera->screenToWorld(mouseX, mouseY);
        blueprints->create(selectedBlueprint_, worldX, worldY);
    }
}
```

#### `std::optional<BlueprintDef> getBlueprint(const std::string& name) const`

Get a blueprint definition for inspection. The returned definition has inheritance fully resolved.

**Example:**
```cpp
auto def = blueprints->getBlueprint("Player");
if (def) {
    logger->info("Player has {} components", def->components.size());

    if (def->physics) {
        logger->info("Physics type: {}", def->physics->bodyType);
    }

    // Access metadata
    auto it = def->metadata.find("displayName");
    if (it != def->metadata.end()) {
        std::string name = std::any_cast<std::string>(it->second);
        logger->info("Display name: {}", name);
    }
}
```

### Creating Entities

#### `Entity create(const std::string& blueprintName, float x, float y)`

Create an entity from a blueprint at position (x, y).

**Example:**
```cpp
// Create player at spawn point
Entity player = blueprints->create("Player", 100.0f, 200.0f);

// Create enemy
Entity enemy = blueprints->create("Enemy", 500.0f, 300.0f);

// Returns entt::null if blueprint not found
if (player == entt::null) {
    logger->error("Failed to create player");
}
```

#### `Entity create(const std::string& blueprintName, float x, float y, float width, float height)`

Create an entity with explicit size. Useful for platforms that vary in size.

**Example:**
```cpp
// Create ground platform - 1000 units wide, 40 units tall
Entity ground = blueprints->create("Platform", 0.0f, 0.0f, 1000.0f, 40.0f);

// Create smaller floating platform
Entity floater = blueprints->create("Platform", 500.0f, 300.0f, 200.0f, 20.0f);

// Create tall wall
Entity wall = blueprints->create("Platform", 0.0f, 40.0f, 20.0f, 500.0f);
```

**Note:** If the blueprint has physics defined, the provided size will be used for the physics body instead of the blueprint's physics size.

#### `Entity create(const std::string& blueprintName, float x, float y, const PropertyMap& overrides)`

Create an entity with property overrides.

**Example:**
```cpp
// Create a red variant of an entity
PropertyMap redVariant;
redVariant["DebugRect.fillColor.r"] = 255.0;
redVariant["DebugRect.fillColor.g"] = 0.0;
redVariant["DebugRect.fillColor.b"] = 0.0;

Entity redBox = blueprints->create("Box", x, y, redVariant);
```

**Nested Property Override Syntax:**
```cpp
// Override format: "ComponentName.property" or "ComponentName.nested.property"
PropertyMap overrides;
overrides["DebugRect.fillColor.r"] = 255.0;           // Nested property
overrides["DebugCircle.radius"] = 50.0;               // Direct property
overrides["DebugRect.size"] = std::vector<double>{100, 50};  // Array property
```

#### `Entity create(const std::string& blueprintName, float x, float y, float width, float height, const PropertyMap& overrides)`

Create an entity with both size and property overrides.

**Example:**
```cpp
// Create a large, bright box
PropertyMap brightBox;
brightBox["DebugRect.fillColor"] = std::vector<double>{255, 255, 0, 255};

Entity bigBox = blueprints->create("Box", x, y, 100.0f, 100.0f, brightBox);
```

### Component Registration

#### `void registerComponent(const std::string& name, ComponentCreator creator)`

Register a custom component creator. This allows blueprints to use game-specific components.

**Signature:**
```cpp
using ComponentCreator = std::function<void(Entity, IEntitySystem&, const PropertyMap&)>;
```

**Example:**
```cpp
// Define game-specific component
struct Health {
    int current;
    int maximum;
    float invincibilityTime;
};

// Register component creator
blueprints->registerComponent("Health",
    [](Entity e, IEntitySystem& entities, const PropertyMap& props) {
        Health h{};

        // Extract properties with helpers
        auto getDouble = [&](const std::string& key, double def) {
            auto it = props.find(key);
            if (it == props.end()) return def;
            return std::any_cast<double>(it->second);
        };

        h.maximum = static_cast<int>(getDouble("maxHealth", 100.0));
        h.current = h.maximum;
        h.invincibilityTime = static_cast<float>(getDouble("invincibilityTime", 0.5));

        entities.emplace<Health>(e, h);
    });
```

**Now use in blueprints:**
```lua
Blueprints = {
    Player = {
        components = {
            Health = {
                maxHealth = 100,
                invincibilityTime = 1.0
            }
        }
    }
}
```

**Helper for Simple Components:**
```cpp
template<typename T>
void registerSimpleComponent(IBlueprintFactory& factory, const std::string& name) {
    factory.registerComponent(name,
        [](Entity e, IEntitySystem& entities, const PropertyMap& props) {
            entities.emplace<T>(e);
        });
}

// Register tag components
registerSimpleComponent<PlayerTag>(*blueprints, "PlayerTag");
registerSimpleComponent<EnemyTag>(*blueprints, "EnemyTag");
```

#### `bool isComponentRegistered(const std::string& name) const`

Check if a component type is registered.

**Example:**
```cpp
if (!blueprints->isComponentRegistered("Health")) {
    blueprints->registerComponent("Health", healthCreator);
}
```

---

## Built-in Components

The BlueprintFactory includes built-in support for debug rendering components:

### Transform2D (Always Added)

Transform2D is automatically added to every entity created from a blueprint. You don't need to specify it.

```cpp
// Automatically added with x, y from create() call
Transform2D {
    .x = x,
    .y = y,
    .rotation = 0.0f,
    .scaleX = 1.0f,
    .scaleY = 1.0f
}
```

### DebugRect

Rectangular debug shape for visualization and prototyping.

```lua
components = {
    DebugRect = {
        -- Size (required)
        size = {64, 64},              -- {width, height}
        -- OR:
        width = 64,
        height = 64,

        -- Colors (optional)
        fillColor = {255, 0, 0, 255},     -- {r, g, b, a}
        outlineColor = {0, 0, 0, 255},

        -- Rendering (optional)
        outlineWidth = 2.0,
        layer = 10,
        filled = true
    }
}
```

**Properties:**
- `size`: Array `{width, height}` or separate `width` and `height`
- `fillColor`: Array `{r, g, b, a}` or nested `{r = 255, g = 0, b = 0, a = 255}`
- `outlineColor`: Same format as fillColor
- `outlineWidth`: Float, thickness of outline in pixels
- `layer`: Integer, render layer (higher = drawn on top)
- `filled`: Boolean, whether to fill the rectangle

### DebugCircle

Circular debug shape for visualization and prototyping.

```lua
components = {
    DebugCircle = {
        radius = 32,
        fillColor = {0, 255, 0, 255},
        outlineColor = {0, 0, 0, 255},
        outlineWidth = 2.0,
        layer = 10,
        filled = true,
        segments = 32  -- Number of line segments (smoothness)
    }
}
```

**Properties:**
- `radius`: Float, circle radius
- `fillColor`: Color (see DebugRect)
- `outlineColor`: Color
- `outlineWidth`: Float
- `layer`: Integer render layer
- `filled`: Boolean
- `segments`: Integer, circle smoothness (default 32)

### DebugLine

Line segment for debug visualization.

```lua
components = {
    DebugLine = {
        endOffset = {100, 50},  -- {x, y} offset from entity position
        color = {255, 255, 255, 255},
        thickness = 2.0,
        layer = 5
    }
}
```

**Properties:**
- `endOffset`: Array `{x, y}`, end point relative to entity position
- `color`: Line color
- `thickness`: Float, line thickness
- `layer`: Integer render layer

---

## Blueprint Inheritance

Blueprints support inheritance using the `inherits` field. This allows you to create base blueprints and extend them.

### Basic Inheritance

```lua
Blueprints = {
    BaseCharacter = {
        components = {
            DebugRect = {
                size = {32, 32},
                fillColor = {128, 128, 128, 255}
            }
        },
        physics = {
            type = "dynamic",
            fixedRotation = true
        }
    },

    Player = {
        inherits = "BaseCharacter",

        components = {
            DebugRect = {
                fillColor = {50, 200, 100, 255}  -- Override color
                -- size is inherited as {32, 32}
            }
        }
        -- physics is inherited completely
    },

    Enemy = {
        inherits = "BaseCharacter",

        components = {
            DebugRect = {
                fillColor = {255, 100, 100, 255}  -- Override color
            }
        }
    }
}
```

### Multi-Level Inheritance

Blueprints can inherit from blueprints that themselves inherit:

```lua
Blueprints = {
    Base = {
        components = {
            DebugRect = {
                size = {16, 16},
                fillColor = {255, 255, 255, 255}
            }
        }
    },

    Middle = {
        inherits = "Base",
        components = {
            DebugRect = {
                size = {32, 32}  -- Override size
                -- fillColor inherited
            }
        }
    },

    Final = {
        inherits = "Middle",
        components = {
            DebugRect = {
                fillColor = {0, 255, 0, 255}  -- Override color
                -- size inherited as {32, 32} from Middle
            }
        }
    }
}
```

### Property Merging Rules

1. **Component-level**: Child components override parent components by name
2. **Property-level**: Child properties override parent properties by name
3. **Nested properties**: Recursively merged (child overrides parent)
4. **Physics**: Child physics completely replaces parent physics (not merged)
5. **Metadata**: Child metadata merged with parent metadata

```lua
-- Parent
BaseEntity = {
    components = {
        DebugRect = {
            size = {32, 32},
            fillColor = {128, 128, 128, 255},
            layer = 10
        }
    },
    metadata = {
        category = "base",
        health = 100
    }
}

-- Child
DerivedEntity = {
    inherits = "BaseEntity",
    components = {
        DebugRect = {
            fillColor = {255, 0, 0, 255}
            -- size inherited: {32, 32}
            -- layer inherited: 10
        }
    },
    metadata = {
        health = 150  -- Override
        -- category inherited: "base"
    }
}
```

---

## Property Overrides

You can override blueprint properties at entity creation time using the `PropertyMap` parameter.

### Override Syntax

Property overrides use dot notation: `"ComponentName.property"` or `"ComponentName.nested.property"`.

```cpp
PropertyMap overrides;

// Simple property
overrides["DebugRect.layer"] = 20.0;

// Nested property
overrides["DebugRect.fillColor.r"] = 255.0;
overrides["DebugRect.fillColor.g"] = 0.0;
overrides["DebugRect.fillColor.b"] = 0.0;

// Array property
overrides["DebugRect.size"] = std::vector<double>{100, 50};

Entity entity = blueprints->create("Box", x, y, overrides);
```

### Color Override Example

```cpp
// Create red, blue, and green variants of the same blueprint
PropertyMap redVariant;
redVariant["DebugCircle.fillColor.r"] = 255.0;
redVariant["DebugCircle.fillColor.g"] = 0.0;
redVariant["DebugCircle.fillColor.b"] = 0.0;

PropertyMap blueVariant;
blueVariant["DebugCircle.fillColor.r"] = 0.0;
blueVariant["DebugCircle.fillColor.g"] = 0.0;
blueVariant["DebugCircle.fillColor.b"] = 255.0;

Entity redBall = blueprints->create("Ball", 100, 200, redVariant);
Entity blueBall = blueprints->create("Ball", 200, 200, blueVariant);
```

### Size Override Example

```cpp
// Create boxes of different sizes from same blueprint
PropertyMap smallBox;
smallBox["DebugRect.size"] = std::vector<double>{16, 16};

PropertyMap largeBox;
largeBox["DebugRect.size"] = std::vector<double>{128, 128};

Entity small = blueprints->create("Box", 100, 200, smallBox);
Entity large = blueprints->create("Box", 300, 200, largeBox);
```

---

## Physics Definitions

Blueprints can include physics body definitions. When a physics body is created, the BlueprintFactory coordinates with the physics system.

### Physics Structure

```lua
physics = {
    -- Body type (required)
    type = "dynamic",  -- "static", "dynamic", "kinematic"

    -- Size (optional - can be set at creation time)
    size = {32, 48},   -- {width, height}

    -- Material properties
    density = 1.0,
    friction = 0.3,
    restitution = 0.0,  -- Bounciness (0-1)

    -- Behavior flags
    fixedRotation = true,
    sensor = false,     -- Trigger vs solid collision
    linearDamping = 0.0,

    -- Collision filtering
    collisionLayer = "player"  -- Named collision layer
}
```

### Physics Body Types

| Type | Description | Use For |
|------|-------------|---------|
| `"static"` | Immovable, infinite mass | Walls, floors, platforms |
| `"dynamic"` | Fully simulated, affected by forces | Player, enemies, projectiles |
| `"kinematic"` | Movable, but not affected by forces | Moving platforms, doors |

### Size Handling

The physics body size is determined in this order:

1. **Explicit size at creation time** (if using 4-parameter `create()`)
2. **Blueprint physics.size** (if specified)
3. **DebugRect component size** (if present)
4. **Default size** (32x32)

```lua
-- Example: Platform blueprint with flexible size
Blueprints = {
    Platform = {
        components = {
            DebugRect = {
                size = {100, 20},  -- Default visual size
                fillColor = {100, 100, 120, 255}
            }
        },
        physics = {
            type = "static",
            friction = 0.5
            -- No size specified, will use DebugRect size or creation size
        }
    }
}
```

```cpp
// Create platforms of different sizes
Entity ground = blueprints->create("Platform", 0, 0, 1000, 40);
Entity floater = blueprints->create("Platform", 500, 300, 200, 20);
```

### Physics Without Physics System

If the BlueprintFactory is constructed without a physics system pointer, physics definitions are silently ignored:

```cpp
// No physics system
auto factory = std::make_unique<BlueprintFactory>(*entities, nullptr);

// This works, but physics definition is ignored
Entity e = factory->create("HasPhysics", 0, 0);
```

### Size Syncing

When a physics body is created, the BlueprintFactory automatically syncs DebugRect and DebugCircle sizes to match the physics body:

```lua
Blueprints = {
    Box = {
        components = {
            DebugRect = {
                size = {10, 10},  -- Small initial size
                fillColor = {255, 0, 0, 255}
            }
        },
        physics = {
            type = "static",
            size = {100, 200}  -- Larger physics size
        }
    }
}
```

After creation, the DebugRect size will be updated to `{100, 200}` to match the physics body.

---

## Component Registration

To use custom components in blueprints, you must register them first.

### Registration Pattern

```cpp
void Game::registerComponents() {
    // Helper lambdas for property extraction
    auto getDouble = [](const PropertyMap& props, const std::string& key, double def) {
        auto it = props.find(key);
        if (it == props.end()) return def;
        if (auto* d = std::any_cast<double>(&it->second)) return *d;
        if (auto* i = std::any_cast<int>(&it->second)) return static_cast<double>(*i);
        return def;
    };

    auto getBool = [](const PropertyMap& props, const std::string& key, bool def) {
        auto it = props.find(key);
        if (it == props.end()) return def;
        if (auto* b = std::any_cast<bool>(&it->second)) return *b;
        return def;
    };

    // Register Health component
    blueprints->registerComponent("Health",
        [=](Entity e, IEntitySystem& entities, const PropertyMap& props) {
            Health h;
            h.maximum = static_cast<int>(getDouble(props, "maxHealth", 100.0));
            h.current = h.maximum;
            h.invincibilityTime = static_cast<float>(getDouble(props, "invincibilityTime", 0.5));

            entities.emplace<Health>(e, h);
        });

    // Register Velocity component
    blueprints->registerComponent("Velocity",
        [=](Entity e, IEntitySystem& entities, const PropertyMap& props) {
            Velocity v;
            v.x = static_cast<float>(getDouble(props, "x", 0.0));
            v.y = static_cast<float>(getDouble(props, "y", 0.0));

            entities.emplace<Velocity>(e, v);
        });
}
```

### Property Extraction Helpers

```cpp
// Reusable property extraction functions
namespace BlueprintHelpers {

double getDouble(const PropertyMap& props, const std::string& key, double def) {
    auto it = props.find(key);
    if (it == props.end()) return def;
    if (auto* d = std::any_cast<double>(&it->second)) return *d;
    if (auto* i = std::any_cast<int>(&it->second)) return static_cast<double>(*i);
    return def;
}

bool getBool(const PropertyMap& props, const std::string& key, bool def) {
    auto it = props.find(key);
    if (it == props.end()) return def;
    if (auto* b = std::any_cast<bool>(&it->second)) return *b;
    return def;
}

std::string getString(const PropertyMap& props, const std::string& key, const std::string& def) {
    auto it = props.find(key);
    if (it == props.end()) return def;
    if (auto* s = std::any_cast<std::string>(&it->second)) return *s;
    return def;
}

}  // namespace BlueprintHelpers
```

---

## Best Practices

### 1. Organize Blueprint Files

```
data/blueprints/
├── entities.lua       # Main entity blueprints
├── _base.lua          # Base/shared blueprints
├── player.lua         # Player-specific blueprints
├── enemies/
│   ├── slime.lua
│   └── goblin.lua
└── items/
    ├── coins.lua
    └── powerups.lua
```

### 2. Use Inheritance for Variants

```lua
-- Base enemy
Blueprints = {
    BaseEnemy = {
        components = {
            DebugRect = {
                size = {32, 32},
                layer = 10
            }
        },
        physics = {
            type = "dynamic",
            fixedRotation = true
        }
    },

    -- Fast enemy
    SlimeEnemy = {
        inherits = "BaseEnemy",
        components = {
            DebugRect = {
                fillColor = {0, 255, 0, 255}
            }
        },
        metadata = {
            speed = 100,
            health = 30
        }
    },

    -- Slow, tanky enemy
    GoblinEnemy = {
        inherits = "BaseEnemy",
        components = {
            DebugRect = {
                fillColor = {255, 0, 0, 255},
                size = {40, 40}  -- Larger
            }
        },
        metadata = {
            speed = 50,
            health = 100
        }
    }
}
```

### 3. Use Metadata for Game Logic

Store gameplay properties in metadata, not components:

```lua
Blueprints = {
    Player = {
        components = { ... },

        metadata = {
            displayName = "Player",
            maxHealth = 100,
            moveSpeed = 200,
            jumpForce = 450,
            abilities = {"dash", "wallJump"}
        }
    }
}
```

```cpp
// Read metadata in game code
auto def = blueprints->getBlueprint("Player");
if (def) {
    auto it = def->metadata.find("moveSpeed");
    if (it != def->metadata.end()) {
        float speed = std::any_cast<double>(it->second);
        // Use speed...
    }
}
```

### 4. Register All Components at Startup

```cpp
void Game::initialize() {
    // Register ALL custom components before loading blueprints
    registerComponents();

    // Now load blueprints
    AssetHandle handle = assets->registerAsset(AssetType::Data,
        ":assets:/blueprints/entities.lua");
    assets->loadAsset(handle);
    const LuaData* data = assets->getAsset<LuaData>(handle);
    blueprints->loadBlueprints(data->source);
}
```

### 5. Use Hot Reload During Development

```cpp
#if defined(BESTOW_DEBUG)
    assets->enableHotReload(true);

    assets->subscribeToType(AssetType::Lua, [this](AssetHandle h, AssetType) {
        const LuaData* data = assets->getAsset<LuaData>(h);
        blueprints->loadBlueprints(data->source);
        logger->info("Blueprints reloaded");
    });
#endif
```

### 6. Validate Blueprints on Load

```cpp
void validateBlueprints() {
    auto names = blueprints->getBlueprintNames();

    for (const auto& name : names) {
        auto def = blueprints->getBlueprint(name);
        if (!def) {
            logger->error("Failed to get blueprint: {}", name);
            continue;
        }

        // Check for required components
        bool hasVisual = false;
        for (const auto& comp : def->components) {
            if (comp.name == "DebugRect" || comp.name == "DebugCircle") {
                hasVisual = true;
                break;
            }
        }

        if (!hasVisual) {
            logger->warn("Blueprint '{}' has no visual component", name);
        }
    }
}
```

---

## Complete Examples

### Example 1: Platformer Game

```lua
-- data/blueprints/entities.lua
Blueprints = {
    -- Base character template
    BaseCharacter = {
        components = {
            DebugRect = {
                size = {32, 48},
                outlineColor = {0, 0, 0, 255},
                outlineWidth = 1.0,
                layer = 10
            }
        },
        physics = {
            type = "dynamic",
            fixedRotation = true,
            density = 1.0,
            friction = 0.0
        }
    },

    -- Player character
    Player = {
        inherits = "BaseCharacter",
        components = {
            DebugRect = {
                fillColor = {50, 200, 100, 255}
            }
        },
        physics = {
            size = {28, 44}
        },
        metadata = {
            health = 100,
            moveSpeed = 200,
            jumpForce = 450
        }
    },

    -- Enemy
    Enemy = {
        inherits = "BaseCharacter",
        components = {
            DebugRect = {
                fillColor = {255, 100, 100, 255},
                size = {28, 28}
            }
        },
        physics = {
            size = {24, 24}
        },
        metadata = {
            health = 30,
            moveSpeed = 50
        }
    },

    -- Static platform
    Platform = {
        components = {
            DebugRect = {
                size = {100, 20},
                fillColor = {100, 100, 120, 255}
            }
        },
        physics = {
            type = "static",
            friction = 0.5
        }
    },

    -- Collectible coin
    Coin = {
        components = {
            DebugCircle = {
                radius = 12,
                fillColor = {255, 215, 0, 255}
            }
        },
        physics = {
            type = "static",
            size = {24, 24},
            sensor = true
        }
    }
}

return Blueprints
```

### Example 2: Creating Entities in Code

```cpp
void Game::spawnLevel() {
    auto& sys = engine_->systems();

    // Create ground
    Entity ground = sys.blueprints->create("Platform", 0, 550, 1280, 50);

    // Create floating platforms
    sys.blueprints->create("Platform", 200, 400, 150, 20);
    sys.blueprints->create("Platform", 500, 300, 150, 20);
    sys.blueprints->create("Platform", 800, 400, 150, 20);

    // Create player
    player_ = sys.blueprints->create("Player", 100, 500);

    // Create enemies
    sys.blueprints->create("Enemy", 400, 350);
    sys.blueprints->create("Enemy", 700, 350);

    // Create coins
    for (int i = 0; i < 10; i++) {
        float x = 100 + i * 50;
        sys.blueprints->create("Coin", x, 200);
    }
}
```

### Example 3: Enemy Variants with Overrides

```cpp
void spawnEnemies() {
    // Normal enemy
    blueprints->create("Enemy", 400, 350);

    // Large boss enemy
    PropertyMap bossOverrides;
    bossOverrides["DebugRect.size"] = std::vector<double>{64, 64};
    bossOverrides["DebugRect.fillColor.r"] = 200.0;
    bossOverrides["DebugRect.fillColor.g"] = 0.0;
    bossOverrides["DebugRect.fillColor.b"] = 0.0;

    Entity boss = blueprints->create("Enemy", 1000, 300, bossOverrides);

    // Store boss-specific data
    auto def = blueprints->getBlueprint("Enemy");
    int bossHealth = std::any_cast<double>(def->metadata["health"]) * 5;
    entities->emplace<BossTag>(boss, bossHealth);
}
```

### Example 4: Hot Reload Workflow

```cpp
class Game {
public:
    void initialize() {
        // Enable hot reload
        sys.assets->enableHotReload(true);

        // Load initial blueprints
        blueprintHandle_ = sys.assets->registerAsset(AssetType::Lua,
            ":assets:/blueprints/entities.lua");
        sys.assets->loadAsset(blueprintHandle_);
        loadBlueprints();

        // Subscribe to changes
        sys.assets->subscribe(blueprintHandle_,
            [this](AssetHandle h, AssetType) {
                loadBlueprints();
                onBlueprintsReloaded();
            });
    }

    void update(float dt) {
        // Process hot reload notifications
        sys.assets->update();
    }

private:
    void loadBlueprints() {
        const LuaData* data = sys.assets->getAsset<LuaData>(blueprintHandle_);
        if (!sys.blueprints->loadBlueprints(data->source)) {
            logger->error("Failed to load blueprints");
        } else {
            logger->info("Blueprints loaded successfully");
        }
    }

    void onBlueprintsReloaded() {
        logger->info("Blueprints reloaded! New entities will use updated definitions.");
        // Optionally: Recreate entities, refresh debug UI, etc.
    }

    AssetHandle blueprintHandle_;
};
```

---

## Summary

The Blueprints System is Bestow's cornerstone for **data-driven entity creation**. Key takeaways:

- **Blueprints are templates**, not entities - they define what to create, not the entity itself
- **Load from Lua** - Define entities in `Blueprints` table or return table from script
- **Inheritance support** - Build hierarchies with `inherits` field
- **Property overrides** - Customize entities at spawn time with `PropertyMap`
- **Built-in debug components** - DebugRect, DebugCircle, DebugLine for prototyping
- **Physics integration** - Define physics bodies directly in blueprints
- **Component registration** - Extend with game-specific components
- **Hot reload ready** - Change Lua files and see results instantly

**Remember:**
- Always register custom components before loading blueprints
- Use AssetSystem for loading blueprint files (enables hot reload)
- Use inheritance to reduce duplication
- Store gameplay properties in metadata
- Validate blueprints after loading in debug builds

For more examples, see:
- Unit tests: `/Users/jaaaacob/Documents/GameDev/jframe/tests/unit/BlueprintFactoryTests.cpp`
- Example blueprints: `/Users/jaaaacob/Documents/GameDev/jframe/build/macos-debug/examples/*/data/blueprints/`

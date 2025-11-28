# BlueprintFactory API

The `BlueprintFactory` provides data-driven entity creation from Lua blueprints.

## Overview

```cpp
auto& blueprints = sys.blueprints;

// Load blueprints from Lua
std::string luaSource = loadFile("blueprints/entities.lua");
blueprints->loadBlueprints(luaSource);

// Create entity from blueprint
Entity enemy = blueprints->create("enemy_slime", 400.0f, 300.0f);

// Create with overrides
Entity boss = blueprints->create("enemy_slime", 800.0f, 300.0f, {
    {"health", 500},
    {"size", 2.0f}
});
```

## Blueprint Loading

### loadBlueprints(const std::string& luaSource)

```cpp
bool loadBlueprints(const std::string& luaSource);
```

Loads blueprint definitions from Lua source code.

**Returns:** `true` if parsing succeeded, `false` on error

**Example:**

```cpp
std::ifstream file("blueprints/entities.lua");
std::string source((std::istreambuf_iterator<char>(file)),
                    std::istreambuf_iterator<char>());

if (!blueprints->loadBlueprints(source)) {
    logError("Failed to load blueprints");
}
```

---

### reloadBlueprints()

```cpp
void reloadBlueprints();
```

Reloads blueprints from the previously loaded source (for hot-reload).

---

### clearBlueprints()

```cpp
void clearBlueprints();
```

Clears all loaded blueprints.

---

## Blueprint Queries

### hasBlueprint(const std::string& name)

```cpp
bool hasBlueprint(const std::string& name) const;
```

Checks if a blueprint exists.

---

### getBlueprintNames()

```cpp
std::vector<std::string> getBlueprintNames() const;
```

Returns all blueprint names.

---

### getBlueprint(const std::string& name)

```cpp
std::optional<BlueprintDef> getBlueprint(const std::string& name) const;
```

Returns a blueprint definition for inspection.

---

## Entity Creation

### create(blueprintName, x, y)

```cpp
Entity create(const std::string& blueprintName, float x, float y);
```

Creates an entity from a blueprint at the specified position.

**Example:**

```cpp
Entity coin = blueprints->create("collectible_coin", 500.0f, 400.0f);
```

---

### create(blueprintName, x, y, width, height)

```cpp
Entity create(const std::string& blueprintName,
              float x, float y,
              float width, float height);
```

Creates an entity with a specific size (overrides blueprint size).

**Example:**

```cpp
// Create large platform
Entity platform = blueprints->create("platform", 400.0f, 600.0f, 200.0f, 32.0f);
```

---

### create(blueprintName, x, y, overrides)

```cpp
Entity create(const std::string& blueprintName,
              float x, float y,
              const PropertyMap& overrides);
```

Creates an entity with property overrides.

**Example:**

```cpp
PropertyMap props;
props["health"] = 500;
props["damage"] = 50;
props["color"] = Color::red();

Entity boss = blueprints->create("enemy_slime", 800.0f, 300.0f, props);
```

---

### create(blueprintName, x, y, width, height, overrides)

```cpp
Entity create(const std::string& blueprintName,
              float x, float y,
              float width, float height,
              const PropertyMap& overrides);
```

Creates an entity with both size and property overrides.

---

## Component Registration

### registerComponent(name, creator)

```cpp
using ComponentCreator = std::function<void(Entity, IEntitySystem&, const PropertyMap&)>;
void registerComponent(const std::string& name, ComponentCreator creator);
```

Registers a custom component creator function.

**Example:**

```cpp
// Register custom Health component
blueprints->registerComponent("Health", [](Entity e, IEntitySystem& entities, const PropertyMap& props) {
    int maxHealth = std::any_cast<int>(props.at("max"));
    int current = props.contains("current") ?
        std::any_cast<int>(props.at("current")) : maxHealth;

    entities.emplace<Health>(e, current, maxHealth);
});

// Register custom AI component
blueprints->registerComponent("PatrolAI", [](Entity e, IEntitySystem& entities, const PropertyMap& props) {
    float distance = std::any_cast<float>(props.at("distance"));
    float speed = std::any_cast<float>(props.at("speed"));

    entities.emplace<PatrolAI>(e, distance, speed);
});
```

---

### isComponentRegistered(const std::string& name)

```cpp
bool isComponentRegistered(const std::string& name) const;
```

Checks if a component type is registered.

---

## Lua Blueprint Format

Blueprints are defined in Lua files:

```lua
-- blueprints/entities.lua

return {
    -- Simple platform
    platform = {
        components = {
            {
                name = "DebugRect",
                properties = {
                    fillColor = {r = 100, g = 100, b = 100, a = 255},
                    layer = 0
                }
            }
        },
        physics = {
            bodyType = "static",
            friction = 0.8,
            collisionLayer = "Terrain"
        }
    },

    -- Collectible coin
    collectible_coin = {
        components = {
            {
                name = "DebugCircle",
                properties = {
                    radius = 16,
                    fillColor = {r = 255, g = 215, b = 0, a = 255},
                    layer = 10
                }
            },
            {
                name = "Collectible",
                properties = {
                    points = 10
                }
            }
        },
        physics = {
            bodyType = "static",
            sensor = true,
            collisionLayer = "Collectible"
        }
    },

    -- Enemy with inheritance
    enemy_base = {
        components = {
            {
                name = "Health",
                properties = {
                    max = 100
                }
            },
            {
                name = "Enemy",
                properties = {
                    damage = 10
                }
            }
        },
        physics = {
            bodyType = "dynamic",
            fixedRotation = true,
            density = 1.0,
            collisionLayer = "Enemy"
        }
    },

    enemy_slime = {
        inherits = "enemy_base",
        components = {
            {
                name = "DebugRect",
                properties = {
                    size = {x = 32, y = 32},
                    fillColor = {r = 0, g = 255, b = 0, a = 255},
                    layer = 20
                }
            },
            {
                name = "PatrolAI",
                properties = {
                    distance = 100,
                    speed = 50
                }
            }
        }
    },

    enemy_bat = {
        inherits = "enemy_base",
        components = {
            {
                name = "DebugCircle",
                properties = {
                    radius = 16,
                    fillColor = {r = 128, g = 0, b = 128, a = 255},
                    layer = 20
                }
            },
            {
                name = "FlyingAI",
                properties = {
                    altitude = 100,
                    speed = 80
                }
            }
        },
        physics = {
            bodyType = "kinematic",
            sensor = true
        }
    }
}
```

---

## Blueprint Definition Structure

```cpp
struct BlueprintDef {
    std::string name;
    std::string inherits;  // Parent blueprint name (optional)
    std::vector<ComponentDef> components;
    std::optional<BlueprintPhysicsDef> physics;
    PropertyMap metadata;
};

struct ComponentDef {
    std::string name;              // Component type name
    PropertyMap properties;        // Component properties
};

struct BlueprintPhysicsDef {
    std::string bodyType = "dynamic";  // "static", "dynamic", "kinematic"
    std::optional<Vec2> size;          // Override size
    bool sensor = false;
    bool fixedRotation = true;
    float density = 1.0f;
    float friction = 0.3f;
    float restitution = 0.0f;
    float linearDamping = 0.0f;
    std::string collisionLayer;        // Named collision layer
};
```

---

## Built-in Component Types

The blueprint factory pre-registers these component types:

### Transform2D

```lua
{
    name = "Transform2D",
    properties = {
        x = 0,
        y = 0,
        rotation = 0,
        scaleX = 1.0,
        scaleY = 1.0
    }
}
```

Note: Transform is set automatically from `create()` position parameters.

---

### DebugRect

```lua
{
    name = "DebugRect",
    properties = {
        size = {x = 32, y = 32},
        fillColor = {r = 128, g = 128, b = 128, a = 255},
        outlineColor = {r = 0, g = 0, b = 0, a = 0},
        outlineWidth = 0,
        layer = 0,
        filled = true
    }
}
```

---

### DebugCircle

```lua
{
    name = "DebugCircle",
    properties = {
        radius = 16,
        fillColor = {r = 128, g = 128, b = 128, a = 255},
        outlineColor = {r = 0, g = 0, b = 0, a = 0},
        outlineWidth = 0,
        layer = 0,
        filled = true,
        segments = 32
    }
}
```

---

### DebugLine

```lua
{
    name = "DebugLine",
    properties = {
        endOffset = {x = 32, y = 0},
        color = {r = 255, g = 255, b = 255, a = 255},
        thickness = 1,
        layer = 0
    }
}
```

---

## Common Patterns

### Registering Game Components

```cpp
void registerGameComponents(BlueprintFactory& blueprints) {
    // Health component
    blueprints.registerComponent("Health", [](Entity e, IEntitySystem& entities, const PropertyMap& props) {
        int max = std::any_cast<int>(props.at("max"));
        entities.emplace<Health>(e, max, max);
    });

    // Enemy AI
    blueprints.registerComponent("PatrolAI", [](Entity e, IEntitySystem& entities, const PropertyMap& props) {
        float distance = std::any_cast<float>(props.at("distance"));
        float speed = std::any_cast<float>(props.at("speed"));
        entities.emplace<PatrolAI>(e, distance, speed);
    });

    // Collectible
    blueprints.registerComponent("Collectible", [](Entity e, IEntitySystem& entities, const PropertyMap& props) {
        int points = std::any_cast<int>(props.at("points"));
        entities.emplace<Collectible>(e, points);
    });
}
```

---

### Creating Entities from Level Data

```cpp
// Level loading with blueprints
void loadLevelEntities(LevelId levelId) {
    auto entityDefs = levels->getEntityDefs(levelId);

    for (const auto& def : entityDefs) {
        // Create entity from blueprint
        Entity e = blueprints->create(
            def.type,
            def.transform.x,
            def.transform.y,
            def.properties  // Pass level properties as overrides
        );

        // Store level association (for cleanup)
        entities->emplace<LevelEntity>(e, levelId);
    }
}
```

---

### Blueprint Variants

```lua
-- Create variations using inheritance

enemy_goblin_base = {
    components = {
        {name = "Health", properties = {max = 50}},
        {name = "Enemy", properties = {damage = 5}}
    }
}

enemy_goblin_warrior = {
    inherits = "enemy_goblin_base",
    components = {
        {name = "Health", properties = {max = 100}},  -- Override
        {name = "Weapon", properties = {type = "sword"}}
    }
}

enemy_goblin_archer = {
    inherits = "enemy_goblin_base",
    components = {
        {name = "Weapon", properties = {type = "bow"}},
        {name = "RangedAI", properties = {range = 200}}
    }
}
```

---

### Procedural Entity Creation

```lua
-- Generate blueprint on the fly
local function generateRandomEnemy()
    local types = {"enemy_slime", "enemy_bat", "enemy_goblin"}
    local blueprintName = types[math.random(#types)]

    return {
        type = blueprintName,
        x = math.random(100, 1900),
        y = 600,
        properties = {
            health = math.random(50, 150)
        }
    }
end

-- Level with procedural enemies
return {
    entities = function()
        local ents = {}
        for i = 1, 20 do
            table.insert(ents, generateRandomEnemy())
        end
        return ents
    end()
}
```

---

### Hot-Reload Blueprints

```cpp
#if defined(JFRAME_DEV_TOOLS)
void onBlueprintFileChanged(const std::string& path) {
    std::ifstream file(path);
    std::string source((std::istreambuf_iterator<char>(file)),
                        std::istreambuf_iterator<char>());

    blueprints->reloadBlueprints();
    logInfo("Blueprints reloaded");
}
#endif
```

---

## Property Type Conversions

When creating component creators, extract properties using `std::any_cast`:

```cpp
blueprints->registerComponent("MyComponent", [](Entity e, IEntitySystem& entities, const PropertyMap& props) {
    // Extract properties
    int intValue = std::any_cast<int>(props.at("intProp"));
    float floatValue = std::any_cast<float>(props.at("floatProp"));
    std::string strValue = std::any_cast<std::string>(props.at("strProp"));

    // Lua tables become nested PropertyMaps
    auto& subTable = std::any_cast<PropertyMap&>(props.at("nestedProp"));
    int nestedInt = std::any_cast<int>(subTable.at("value"));

    // Colors
    auto& colorTable = std::any_cast<PropertyMap&>(props.at("color"));
    Color color{
        static_cast<uint8_t>(std::any_cast<int>(colorTable.at("r"))),
        static_cast<uint8_t>(std::any_cast<int>(colorTable.at("g"))),
        static_cast<uint8_t>(std::any_cast<int>(colorTable.at("b"))),
        static_cast<uint8_t>(std::any_cast<int>(colorTable.at("a")))
    };

    // Vec2
    auto& vec2Table = std::any_cast<PropertyMap&>(props.at("position"));
    Vec2 pos{
        std::any_cast<float>(vec2Table.at("x")),
        std::any_cast<float>(vec2Table.at("y"))
    };

    entities.emplace<MyComponent>(e, /* ... */);
});
```

---

## Performance Tips

1. **Preload blueprints** - Load during level load, not at spawn time
2. **Use inheritance** - Reduce duplication in blueprint definitions
3. **Cache blueprint queries** - Don't look up the same blueprint repeatedly
4. **Register components once** - During initialization, not per-entity
5. **Batch entity creation** - Create multiple entities in one frame

## See Also

- [EntitySystem](EntitySystem.md) - Component management
- [LevelSystem](LevelSystem.md) - Loading levels with blueprints
- [PhysicsSystem](PhysicsSystem.md) - Automatic physics body creation
- [Data-Driven Design](../Data-Driven-Design.md) - Blueprint design patterns

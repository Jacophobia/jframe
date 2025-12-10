# Blueprints System Guide

The Blueprints System is Bestow's **data-driven entity creation system**. It allows you to define reusable entity templates in Lua files, complete with components, physics bodies, and inheritance hierarchies. This is the primary way to create game entities without writing C++ code.

## Table of Contents

1. [Overview](#overview)
2. [Core Concepts](#core-concepts)
3. [Lua Blueprint Format](#lua-blueprint-format)
4. [API Reference](#api-reference)
5. [Component Types](#component-types)
6. [Inheritance System](#inheritance-system)
7. [Best Practices](#best-practices)
8. [Complete Examples](#complete-examples)

---

## Overview

### What is a Blueprint?

A **blueprint** is a Lua-defined template for creating entities. Think of it as a recipe that describes:

- What components the entity should have (DebugRect, DebugCircle, custom tags)
- What those components' properties are (size, color, layer)
- Whether the entity has a physics body (static, dynamic, kinematic)
- Physics properties (density, friction, restitution)
- Optional metadata for game-specific logic

### Why Use Blueprints?

**Data-Driven Design**: Blueprints embody Bestow's **Lua-first philosophy**. Game developers should spend most of their time in Lua files, not C++ code.

**Benefits:**
- **Hot Reload**: Change blueprints and see updates instantly without recompiling
- **No C++ Required**: Create complex entities without touching engine code
- **Inheritance**: Define base blueprints and extend them with variations
- **Reusability**: Define once, instantiate many times with different positions/sizes
- **Separation of Concerns**: Keep entity definitions separate from game logic
- **Version Control Friendly**: Lua files are human-readable and diff-friendly

---

## Core Concepts

### Blueprint as Entity Template

A blueprint is NOT an entity itself - it's a **template** for creating entities. When you call `blueprints->create("player", x, y)`, the system:

1. Looks up the "player" blueprint definition
2. Creates a new entity in the ECS
3. Adds all components specified in the blueprint
4. Creates a physics body if `physics = {...}` is defined
5. Sets the entity's position to (x, y)

### Inheritance with `inherits`

Blueprints can inherit from other blueprints using the `inherits` field:

```lua
Blueprints = {
    BaseEnemy = {
        components = {
            DebugRect = { size = {32, 32}, fillColor = {255, 0, 0} }
        },
        physics = { type = "dynamic", fixedRotation = true }
    },

    FastEnemy = {
        inherits = "BaseEnemy",  -- Extends BaseEnemy
        components = {
            DebugRect = {
                fillColor = {255, 255, 0}  -- Override color to yellow
            }
        }
    }
}
```

When `FastEnemy` is instantiated:
- It gets all of `BaseEnemy`'s components and physics
- Child properties **override** parent properties (color becomes yellow)
- Properties not overridden are inherited (size remains {32, 32})

### Component Definitions

Components are defined in the `components = {...}` table. Each component has:
- **Name**: The component type (e.g., "DebugRect", "DebugCircle")
- **Properties**: A table of property values (e.g., `size = {64, 64}`)

The BlueprintFactory looks up registered component creators and calls them with the properties.

### Property Overrides at Creation Time

You can override blueprint properties when creating an entity:

```lua
-- In C++:
PropertyMap overrides;
overrides["DebugRect.fillColor.r"] = 255;
overrides["DebugRect.fillColor.g"] = 0;
Entity e = blueprints->create("player", x, y, overrides);
```

This allows runtime customization without modifying the blueprint file.

---

## Lua Blueprint Format

### Basic Structure

```lua
Blueprints = {
    -- Blueprint name as key
    MyEntity = {
        -- Optional: inherit from another blueprint
        inherits = "ParentBlueprint",

        -- Components to add to the entity
        components = {
            ComponentName = {
                property1 = value1,
                property2 = value2
            }
        },

        -- Optional: physics body configuration
        physics = {
            type = "dynamic",  -- "static", "dynamic", "kinematic"
            size = {width, height},
            density = 1.0,
            friction = 0.5,
            restitution = 0.0,
            fixedRotation = true,
            sensor = false,
            linearDamping = 0.0,
            collisionLayer = "Player"
        },

        -- Optional: custom metadata (not processed by engine)
        metadata = {
            score = 100,
            description = "A collectible coin"
        }
    }
}
```

### Minimal Blueprint

The simplest blueprint just creates an entity with a Transform2D:

```lua
Blueprints = {
    Empty = {
        components = {}
    }
}
```

### Blueprint with Visual Component

```lua
Blueprints = {
    RedBox = {
        components = {
            DebugRect = {
                size = {64, 64},
                fillColor = {255, 0, 0, 255},      -- RGBA
                outlineColor = {0, 0, 0, 255},     -- Black outline
                outlineWidth = 2,
                layer = 10,
                filled = true
            }
        }
    }
}
```

### Blueprint with Physics

```lua
Blueprints = {
    DynamicBox = {
        components = {
            DebugRect = {
                size = {32, 32},
                fillColor = {128, 128, 128, 255}
            }
        },
        physics = {
            type = "dynamic",
            size = {32, 32},           -- Physics body size
            density = 1.0,             -- Mass per unit area
            friction = 0.5,            -- Surface friction
            restitution = 0.2,         -- Bounciness (0-1)
            fixedRotation = true,      -- Prevent rotation
            linearDamping = 0.0,       -- Air resistance
            sensor = false,            -- Trigger vs solid
            collisionLayer = "Player"  -- Named collision group
        }
    }
}
```

### Blueprint with Inheritance

```lua
Blueprints = {
    -- Base blueprint
    BasePlatform = {
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

    -- Child blueprint (extends BasePlatform)
    IcePlatform = {
        inherits = "BasePlatform",
        components = {
            DebugRect = {
                fillColor = {200, 220, 255, 255}  -- Override color to light blue
            }
        },
        physics = {
            friction = 0.0  -- Override friction (slippery ice)
        }
    }
}
```

### Lua Features: Variables, Math, Functions

Blueprints are Lua code, so you can use all Lua features:

```lua
-- Define constants
local PLAYER_SIZE = {32, 48}
local PLAYER_COLOR = {50, 200, 100, 255}
local GROUND_LAYER = "Ground"

-- Use math
local JUMP_FORCE = 800
local DOUBLE_JUMP_FORCE = JUMP_FORCE * 0.8

Blueprints = {
    Player = {
        components = {
            DebugRect = {
                size = PLAYER_SIZE,
                fillColor = PLAYER_COLOR
            }
        },
        physics = {
            type = "dynamic",
            size = PLAYER_SIZE,
            collisionLayer = GROUND_LAYER
        },
        metadata = {
            jumpForce = JUMP_FORCE,
            doubleJumpForce = DOUBLE_JUMP_FORCE
        }
    }
}
```

---

## API Reference

### Loading Blueprints

#### `bool loadBlueprints(const std::string& luaSource)`

Load blueprint definitions from Lua source code.

**Returns**: `true` if parsing succeeded, `false` if Lua syntax error or missing `Blueprints` table.

**Example:**
```cpp
std::string luaCode = R"(
    Blueprints = {
        TestEntity = {
            components = {
                DebugRect = { size = {32, 32} }
            }
        }
    }
)";

if (!blueprints->loadBlueprints(luaCode)) {
    // Handle error
}
```

**Typical Usage Pattern:**
```cpp
// Load from asset system
AssetHandle handle = assets->registerAsset(AssetType::Lua, ":assets:/blueprints/entities.lua");
assets->loadAsset(handle);
const LuaData* data = assets->getAsset<LuaData>(handle);

if (!blueprints->loadBlueprints(data->source)) {
    logger->error("Failed to load blueprints");
}
```

#### `void reloadBlueprints()`

Reload blueprints from the last loaded source. Useful for hot reload workflows.

**Example:**
```cpp
// Initial load
blueprints->loadBlueprints(luaSource);

// Later, after file changes detected
blueprints->reloadBlueprints();  // Re-parses same source
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
if (blueprints->hasBlueprint("player")) {
    Entity e = blueprints->create("player", 100.0f, 200.0f);
}
```

#### `std::vector<std::string> getBlueprintNames() const`

Get all registered blueprint names.

**Example:**
```cpp
auto names = blueprints->getBlueprintNames();
for (const auto& name : names) {
    logger->info("Blueprint: {}", name);
}
```

#### `std::optional<BlueprintDef> getBlueprint(const std::string& name) const`

Get a blueprint definition for inspection (includes resolved inheritance).

**Example:**
```cpp
auto def = blueprints->getBlueprint("player");
if (def) {
    logger->info("Player blueprint has {} components", def->components.size());
    if (def->physics) {
        logger->info("Player has physics: {}", def->physics->bodyType);
    }
}
```

### Creating Entities

#### `Entity create(const std::string& blueprintName, float x, float y)`

Create an entity from a blueprint at position (x, y).

**Example:**
```cpp
Entity player = blueprints->create("player", 100.0f, 200.0f);
```

#### `Entity create(const std::string& blueprintName, float x, float y, float width, float height)`

Create an entity with explicit size (useful for platforms that vary in size).

**Example:**
```cpp
// Create a platform 200 units wide, 20 units tall
Entity platform = blueprints->create("platform", 50.0f, 100.0f, 200.0f, 20.0f);
```

**Note:** The provided size overrides the blueprint's physics size if physics is defined.

#### `Entity create(const std::string& blueprintName, float x, float y, const PropertyMap& overrides)`

Create an entity with property overrides.

**Example:**
```cpp
PropertyMap overrides;
overrides["DebugRect.fillColor.r"] = 255;
overrides["DebugRect.fillColor.g"] = 0;
overrides["DebugRect.fillColor.b"] = 0;

Entity redBox = blueprints->create("box", 100.0f, 200.0f, overrides);
```

#### `Entity create(const std::string& blueprintName, float x, float y, float width, float height, const PropertyMap& overrides)`

Create an entity with both size and property overrides.

**Example:**
```cpp
PropertyMap overrides;
overrides["DebugRect.fillColor"] = Color{255, 255, 0, 255};

Entity yellowPlatform = blueprints->create("platform", 0.0f, 0.0f, 300.0f, 30.0f, overrides);
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
struct Health {
    int current;
    int max;
};

blueprints->registerComponent("Health",
    [](Entity e, IEntitySystem& entities, const PropertyMap& props) {
        int max = 100;
        auto it = props.find("max");
        if (it != props.end()) {
            max = static_cast<int>(std::any_cast<double>(it->second));
        }

        entities.emplace<Health>(e, Health{max, max});
    });
```

Now you can use it in blueprints:
```lua
Blueprints = {
    Player = {
        components = {
            Health = { max = 100 }
        }
    }
}
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

## Component Types

The BlueprintFactory includes several built-in component types for rapid prototyping.

### DebugRect

A filled or outlined rectangle for 2D rendering.

**Properties:**
```lua
DebugRect = {
    size = {width, height},          -- Vec2 or {width, height}
    width = 64,                      -- Alternative: individual values
    height = 32,
    fillColor = {r, g, b, a},       -- Color as array or nested table
    outlineColor = {r, g, b, a},    -- Outline color (default: transparent)
    outlineWidth = 2.0,             -- Outline thickness (default: 0)
    layer = 10,                      -- Render layer (default: 0)
    filled = true                    -- Whether to fill (default: true)
}
```

**Example:**
```lua
components = {
    DebugRect = {
        size = {64, 64},
        fillColor = {255, 128, 0, 200},
        outlineColor = {0, 0, 0, 255},
        outlineWidth = 2,
        layer = 15
    }
}
```

### DebugCircle

A filled or outlined circle for 2D rendering.

**Properties:**
```lua
DebugCircle = {
    radius = 32,                     -- Circle radius
    fillColor = {r, g, b, a},       -- Fill color
    outlineColor = {r, g, b, a},    -- Outline color
    outlineWidth = 2.0,             -- Outline thickness
    layer = 10,                      -- Render layer
    filled = true,                   -- Whether to fill
    segments = 32                    -- Number of segments (smoothness)
}
```

**Example:**
```lua
components = {
    DebugCircle = {
        radius = 16,
        fillColor = {255, 215, 0, 255},  -- Gold
        outlineColor = {200, 170, 0, 255},
        outlineWidth = 2,
        segments = 24
    }
}
```

### DebugLine

A line segment from the entity's position to an offset.

**Properties:**
```lua
DebugLine = {
    endOffset = {x, y},             -- End point relative to entity position
    color = {r, g, b, a},           -- Line color
    thickness = 1.0,                -- Line thickness
    layer = 0                        -- Render layer
}
```

**Example:**
```lua
components = {
    DebugLine = {
        endOffset = {100, 50},
        color = {255, 0, 0, 255},
        thickness = 3.0
    }
}
```

### Tag Components (Placeholder)

The system includes placeholder registrations for common tag components:

- `PlayerTag`
- `EnemyTag`
- `PlatformTag`

These do nothing by default. Games should register their own implementations:

```cpp
struct PlayerTag {};

blueprints->registerComponent("PlayerTag",
    [](Entity e, IEntitySystem& entities, const PropertyMap& props) {
        entities.emplace<PlayerTag>(e);
    });
```

### Custom Components

See [Component Registration](#component-registration) for how to register your own component types.

---

## Inheritance System

### How Inheritance Works

When a blueprint declares `inherits = "ParentName"`, the BlueprintFactory:

1. **Recursively resolves** the parent blueprint (parents can inherit too)
2. **Deep copies** the parent's definition
3. **Merges** the child's components onto the parent:
   - Components with the same name have their properties merged
   - Child properties override parent properties
   - New components in child are added
4. **Overwrites** physics if child defines it (no merge, full replace)
5. **Merges** metadata (child metadata overwrites parent metadata)

### Single-Level Inheritance

```lua
Blueprints = {
    BaseEnemy = {
        components = {
            DebugRect = {
                size = {32, 32},
                fillColor = {255, 0, 0, 255}
            }
        },
        physics = {
            type = "dynamic",
            fixedRotation = true
        }
    },

    FastEnemy = {
        inherits = "BaseEnemy",
        components = {
            DebugRect = {
                fillColor = {255, 255, 0, 255}  -- Override to yellow
            }
        },
        metadata = {
            speed = 200
        }
    }
}
```

**Result when creating "FastEnemy":**
- Size: {32, 32} (inherited from BaseEnemy)
- Color: {255, 255, 0, 255} (overridden to yellow)
- Physics: dynamic with fixedRotation (inherited)
- Metadata: speed = 200 (new)

### Multi-Level Inheritance

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
            }
        }
    },

    Final = {
        inherits = "Middle",
        components = {
            DebugRect = {
                fillColor = {0, 255, 0, 255}  -- Override color to green
            }
        }
    }
}
```

**Result when creating "Final":**
- Size: {32, 32} (from Middle, which overrode Base)
- Color: {0, 255, 0, 255} (from Final)

### Property Merging Rules

**Nested Properties:**

```lua
-- Parent
DebugRect = {
    fillColor = {128, 128, 128, 255}
}

-- Child
DebugRect = {
    fillColor = {255, 0, 0}  -- Only r, g, b (no alpha)
}

-- Result: {255, 0, 0, 255}
-- Child's r, g, b override parent, but alpha stays 255
```

**Physics Replacement:**

```lua
-- Parent
physics = {
    type = "dynamic",
    density = 1.0,
    friction = 0.5
}

-- Child
physics = {
    type = "static"
}

-- Result: ONLY type = "static"
-- Parent's density and friction are LOST
```

Physics tables are **replaced, not merged**. If you need to preserve parent physics properties, copy them in the child.

### Inheritance Best Practices

1. **Create Abstract Bases**: Define shared properties in base blueprints
   ```lua
   BaseEnemy = { /* common properties */ },
   Slime = { inherits = "BaseEnemy", /* slime specifics */ },
   Goblin = { inherits = "BaseEnemy", /* goblin specifics */ }
   ```

2. **Override Minimally**: Only override what needs to change
   ```lua
   -- Good: Only override color
   FastEnemy = {
       inherits = "BaseEnemy",
       components = { DebugRect = { fillColor = {...} } }
   }

   -- Bad: Copy entire component definition
   FastEnemy = {
       inherits = "BaseEnemy",
       components = { DebugRect = { size = {...}, fillColor = {...}, layer = ... } }
   }
   ```

3. **Physics is Special**: Remember physics is replaced, not merged
   ```lua
   -- If parent has physics and you want to keep it, don't define physics in child
   -- If you need different physics, you must redefine everything
   ```

4. **Use Metadata for Variants**: Store variant data in metadata
   ```lua
   BaseEnemy = {
       metadata = { speed = 100, damage = 10 }
   },
   FastEnemy = {
       inherits = "BaseEnemy",
       metadata = { speed = 200 }  -- Override speed, damage stays 10
   }
   ```

---

## Best Practices

### Blueprint Organization

**File Structure:**

```
assets/
  blueprints/
    entities.lua           # Main blueprint file
    player.lua             # Player-specific blueprints
    enemies/
      base.lua             # Base enemy blueprint
      slime.lua            # Slime enemy
      goblin.lua           # Goblin enemy
    items/
      collectibles.lua     # Coins, gems, power-ups
```

**Single File for Small Games:**

```lua
-- assets/blueprints/entities.lua
Blueprints = {
    player = { ... },
    platform = { ... },
    coin = { ... },
    enemy = { ... }
}
```

**Multiple Files for Large Games:**

```lua
-- assets/blueprints/entities.lua
local player = dofile("assets/blueprints/player.lua")
local enemies = dofile("assets/blueprints/enemies.lua")

Blueprints = {
    player = player,
    slime = enemies.slime,
    goblin = enemies.goblin
}
```

### Reusable Base Blueprints

Create abstract base blueprints that aren't used directly but serve as templates:

```lua
Blueprints = {
    -- Abstract base (not used directly)
    _BasePlatform = {
        components = {
            DebugRect = {
                size = {100, 20},
                layer = 0
            }
        },
        physics = {
            type = "static",
            friction = 0.5
        }
    },

    -- Concrete variants
    NormalPlatform = {
        inherits = "_BasePlatform",
        components = {
            DebugRect = { fillColor = {100, 100, 120, 255} }
        }
    },

    IcePlatform = {
        inherits = "_BasePlatform",
        components = {
            DebugRect = { fillColor = {200, 220, 255, 255} }
        },
        physics = {
            friction = 0.0  -- Slippery
        }
    }
}
```

### Hot Reload Workflow

**Setup:**

```cpp
// Enable hot reload on asset system
assets->enableHotReload(true);

// Subscribe to blueprint file changes
AssetHandle blueprintHandle = assets->registerAsset(AssetType::Lua, ":assets:/blueprints/entities.lua");
assets->subscribe(blueprintHandle, [this](AssetHandle h, AssetType t) {
    // File changed, reload blueprints
    const LuaData* data = assets->getAsset<LuaData>(h);
    blueprints->loadBlueprints(data->source);
    logger->info("Blueprints reloaded");
});
```

**Workflow:**
1. Edit `entities.lua` in your text editor
2. Save the file
3. Asset system detects change via efsw
4. Callback fires, reloading blueprints
5. New entities use updated definitions

**Note:** Hot reload does NOT modify existing entities. It only affects newly created entities.

### Separation of Concerns

**Blueprint = What, Not How**

Blueprints describe **what** an entity is (its components and properties), not **how** it behaves.

```lua
-- Good: Describes entity structure
Player = {
    components = {
        DebugRect = { size = {32, 48} }
    },
    metadata = {
        speed = 400,
        jumpForce = 800
    }
}
```

**Behavior = Systems**

Entity behavior is implemented in C++ systems or Lua scripts, not in blueprints.

```cpp
// Game logic in a system
void PlayerMovementSystem::update(float dt) {
    for (auto entity : entities.view<PlayerTag, Transform2D>()) {
        auto& transform = entities.get<Transform2D>(entity);

        if (input->isActionPressed("move_right")) {
            transform.x += playerSpeed * dt;
        }
    }
}
```

### Color Palette Constants

Define color palettes at the top of your blueprint file:

```lua
local Colors = {
    Player = {50, 200, 100, 255},
    PlayerOutline = {30, 150, 70, 255},
    Platform = {100, 100, 120, 255},
    Coin = {255, 215, 0, 255}
}

Blueprints = {
    player = {
        components = {
            DebugRect = { fillColor = Colors.Player }
        }
    }
}
```

### Render Layer Constants

Define layers as named constants:

```lua
local Layers = {
    Background = -100,
    Platforms = 0,
    Items = 20,
    Player = 40,
    UI = 100
}

Blueprints = {
    player = {
        components = {
            DebugRect = { layer = Layers.Player }
        }
    }
}
```

---

## Complete Examples

### Example 1: Simple Platformer Entities

```lua
-- assets/blueprints/entities.lua

local Colors = {
    Player = {50, 200, 100, 255},
    Platform = {100, 100, 120, 255},
    Coin = {255, 215, 0, 255}
}

local Layers = {
    Platforms = 0,
    Items = 20,
    Player = 40
}

Blueprints = {
    -- Player character
    player = {
        components = {
            DebugRect = {
                size = {32, 48},
                fillColor = Colors.Player,
                outlineColor = {30, 150, 70, 255},
                outlineWidth = 2,
                layer = Layers.Player
            }
        },
        physics = {
            type = "dynamic",
            size = {32, 48},
            fixedRotation = true,
            density = 1.0,
            friction = 0.0,
            collisionLayer = "Player"
        }
    },

    -- Static platform (size specified at creation time)
    platform = {
        components = {
            DebugRect = {
                fillColor = Colors.Platform,
                outlineColor = {60, 60, 80, 255},
                outlineWidth = 1,
                layer = Layers.Platforms
            }
        },
        physics = {
            type = "static",
            friction = 0.5,
            collisionLayer = "Ground"
        }
    },

    -- Collectible coin
    coin = {
        components = {
            DebugCircle = {
                radius = 12,
                fillColor = Colors.Coin,
                outlineColor = {200, 170, 0, 255},
                outlineWidth = 2,
                layer = Layers.Items
            }
        },
        physics = {
            type = "static",
            size = {24, 24},
            sensor = true,
            collisionLayer = "Pickup"
        }
    }
}
```

**Usage in C++:**

```cpp
// Create player at spawn point
Entity player = blueprints->create("player", 100.0f, 500.0f);

// Create ground platform (300 units wide, 20 units tall)
Entity ground = blueprints->create("platform", 0.0f, 0.0f, 300.0f, 20.0f);

// Create coins at various positions
for (int i = 0; i < 10; i++) {
    float x = 50.0f + i * 40.0f;
    Entity coin = blueprints->create("coin", x, 200.0f);
}
```

### Example 2: Enemies with Inheritance

```lua
-- assets/blueprints/enemies.lua

local Colors = {
    BaseEnemy = {255, 100, 100, 255},
    FastEnemy = {255, 255, 100, 255},
    TankEnemy = {150, 50, 50, 255}
}

Blueprints = {
    -- Base enemy template (not used directly)
    _BaseEnemy = {
        components = {
            DebugRect = {
                size = {32, 32},
                fillColor = Colors.BaseEnemy,
                layer = 30
            }
        },
        physics = {
            type = "dynamic",
            size = {32, 32},
            fixedRotation = true,
            density = 1.0,
            friction = 0.3,
            collisionLayer = "Enemy"
        },
        metadata = {
            health = 50,
            damage = 10,
            speed = 100
        }
    },

    -- Fast enemy variant
    fast_enemy = {
        inherits = "_BaseEnemy",
        components = {
            DebugRect = {
                size = {24, 24},  -- Smaller
                fillColor = Colors.FastEnemy
            }
        },
        metadata = {
            health = 30,   -- Less health
            speed = 200    -- Faster
        }
    },

    -- Tank enemy variant
    tank_enemy = {
        inherits = "_BaseEnemy",
        components = {
            DebugRect = {
                size = {48, 48},  -- Bigger
                fillColor = Colors.TankEnemy
            }
        },
        metadata = {
            health = 150,  -- More health
            speed = 50     // Slower
        }
    }
}
```

**Usage in C++:**

```cpp
// Spawn different enemy types
Entity fast = blueprints->create("fast_enemy", 200.0f, 300.0f);
Entity tank = blueprints->create("tank_enemy", 400.0f, 300.0f);

// Read metadata for game logic
auto fastDef = blueprints->getBlueprint("fast_enemy");
if (fastDef && !fastDef->metadata.empty()) {
    auto speedIt = fastDef->metadata.find("speed");
    if (speedIt != fastDef->metadata.end()) {
        double speed = std::any_cast<double>(speedIt->second);
        // Use speed value...
    }
}
```

### Example 3: Multi-Size Platforms

```lua
-- assets/blueprints/platforms.lua

local PLATFORM_COLOR = {100, 100, 120, 255}
local PLATFORM_OUTLINE = {60, 60, 80, 255}

Blueprints = {
    -- Base platform (size specified at creation)
    platform = {
        components = {
            DebugRect = {
                fillColor = PLATFORM_COLOR,
                outlineColor = PLATFORM_OUTLINE,
                outlineWidth = 1
            }
        },
        physics = {
            type = "static",
            friction = 0.5,
            collisionLayer = "Ground"
        }
    },

    -- Pre-sized small platform
    small_platform = {
        inherits = "platform",
        components = {
            DebugRect = { size = {64, 16} }
        },
        physics = {
            size = {64, 16}
        }
    },

    -- Pre-sized large platform
    large_platform = {
        inherits = "platform",
        components = {
            DebugRect = { size = {256, 32} }
        },
        physics = {
            size = {256, 32}
        }
    }
}
```

**Usage in C++:**

```cpp
// Method 1: Create with explicit size
Entity customPlatform = blueprints->create("platform", 100.0f, 200.0f, 150.0f, 20.0f);

// Method 2: Use pre-sized variants
Entity small = blueprints->create("small_platform", 300.0f, 150.0f);
Entity large = blueprints->create("large_platform", 500.0f, 100.0f);
```

### Example 4: Property Overrides

```lua
-- Blueprint with customizable properties
Blueprints = {
    box = {
        components = {
            DebugRect = {
                size = {32, 32},
                fillColor = {128, 128, 128, 255}
            }
        }
    }
}
```

**Usage in C++:**

```cpp
// Create default gray box
Entity grayBox = blueprints->create("box", 0.0f, 0.0f);

// Create red box with property override
PropertyMap redOverride;
redOverride["DebugRect.fillColor.r"] = 255;
redOverride["DebugRect.fillColor.g"] = 0;
redOverride["DebugRect.fillColor.b"] = 0;
Entity redBox = blueprints->create("box", 100.0f, 0.0f, redOverride);

// Create large blue box
PropertyMap blueOverride;
blueOverride["DebugRect.fillColor"] = std::vector<double>{0, 0, 255, 255};
Entity blueBox = blueprints->create("box", 200.0f, 0.0f, 64.0f, 64.0f, blueOverride);
```

### Example 5: Custom Component Registration

**Define custom component:**

```cpp
// Game-specific health component
struct Health {
    int current;
    int max;
};
```

**Register with BlueprintFactory:**

```cpp
blueprints->registerComponent("Health",
    [](Entity e, IEntitySystem& entities, const PropertyMap& props) {
        int max = 100;  // Default

        auto it = props.find("max");
        if (it != props.end()) {
            if (auto* d = std::any_cast<double>(&it->second)) {
                max = static_cast<int>(*d);
            }
        }

        entities.emplace<Health>(e, Health{max, max});
    });
```

**Use in blueprints:**

```lua
Blueprints = {
    player = {
        components = {
            Health = { max = 100 },
            DebugRect = { size = {32, 48} }
        }
    },

    boss = {
        components = {
            Health = { max = 500 },
            DebugRect = { size = {64, 64} }
        }
    }
}
```

---

## Summary

The Blueprints System is Bestow's cornerstone for **data-driven game development**. By defining entities in Lua:

- **Rapid Iteration**: Change blueprints and see results instantly via hot reload
- **No C++ Required**: Game designers can create complex entities without engine code
- **Reusability**: Define once, instantiate many times
- **Inheritance**: Build hierarchies of entity types with minimal duplication
- **Flexibility**: Override properties at creation time for variants

**Remember:**
- Blueprints are **templates**, not entities
- Use `inherits` to build entity hierarchies
- Register custom components for game-specific data
- Keep blueprints focused on **structure**, not **behavior**
- Leverage Lua features (variables, math, functions) to reduce duplication

For more examples, see:
- `/Users/jaaaacob/Documents/GameDev/jframe/build/macos-debug/template/assets/blueprints/entities.lua`
- `/Users/jaaaacob/Documents/GameDev/jframe/tests/unit/BlueprintFactoryTests.cpp`

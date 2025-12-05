# Bestow Gameplay Ability System (GAS) Guide

The Gameplay Ability System provides a flexible, data-driven framework for implementing gameplay mechanics like abilities, buffs, debuffs, and attribute management.

## Core Concepts

### GameplayTags

Hierarchical identifiers for categorizing and querying gameplay state.

```cpp
import bestow.gas;
import bestow.gas.impl;

auto gas = bestow::createGASSystem();

// Register tags (hierarchical with dots)
auto stunTag = gas->registerTag("State.Debuff.Stunned");
auto dashTag = gas->registerTag("Ability.Movement.Dash");
auto movementTag = gas->registerTag("State.Movement");

// Check hierarchy
gas->isParentOf(movementTag, dashTag);  // false - different roots
auto stateTag = gas->registerTag("State");
gas->isParentOf(stateTag, stunTag);     // true - State is parent of State.Debuff.Stunned

// Find existing tags
auto found = gas->findTag("State.Debuff.Stunned");  // Returns optional<GameplayTag>
```

### GameplayTagContainer

Collections of tags with query operations.

```cpp
bestow::GameplayTagContainer container;
container.addTag(stunTag);
container.addTag(dashTag);

// Queries
container.hasTag(stunTag);          // true
container.hasAny(otherContainer);   // true if any overlap
container.hasAll(otherContainer);   // true if contains all from other
container.matchesQuery(required, blocked);  // has all required, none of blocked
```

### Attributes

Numeric values attached to entities with base/current separation.

```cpp
// Define an attribute
bestow::AttributeDef healthDef{
    .name = "Health",
    .baseValue = 100.0f,
    .minValue = 0.0f,
    .maxValue = 100.0f,
    .clampEnabled = true
};
auto healthId = gas->registerAttribute(healthDef);

// Initialize on entity
bestow::Entity player = /* ... */;
gas->initializeComponent(player);
gas->initializeAttribute(player, healthId, 100.0f);

// Query and modify
float current = gas->getAttributeValue(player, healthId);
float base = gas->getAttributeBaseValue(player, healthId);
gas->modifyAttribute(player, healthId, -25.0f);  // Take 25 damage
gas->setAttributeBaseValue(player, healthId, 150.0f);  // Increase max via base
```

### Effects

Modifiers applied to attributes with duration control.

```cpp
// Define a poison effect
bestow::EffectDef poisonDef{
    .name = "Poison",
    .durationType = bestow::EffectDurationType::Duration,
    .duration = 5.0f,
    .period = 1.0f,  // Tick every second
    .modifiers = {{
        .attribute = healthId,
        .op = bestow::EffectModifierOp::Add,
        .value = -10.0f  // -10 HP per tick
    }},
    .stackable = true,
    .maxStacks = 3
};
auto poisonId = gas->registerEffect(poisonDef);

// Apply effect
gas->applyEffect(target, poisonId, source);

// Query effects
bool isPoisoned = gas->hasEffect(target, poisonId);
auto activeEffects = gas->getActiveEffects(target);
```

**Duration Types:**
- `Instant` - Apply modifiers once, then remove
- `Duration` - Apply for set time, then remove (reverts non-periodic modifiers)
- `Infinite` - Apply until explicitly removed

**Modifier Operations:**
- `Add` - Add value to current (stacks multiply: value * stacks)
- `Multiply` - Multiply current by value (stacks: value^stacks)
- `Override` - Set current to value exactly

**Effect Tags:**
```cpp
bestow::EffectDef buffDef{
    .name = "SpeedBoost",
    .grantedTags = /* tags added while effect active */,
    .applicationRequiredTags = /* target must have these to apply */,
    .applicationBlockedTags = /* target must NOT have these to apply */,
    .removalTags = /* remove effect if target gains any of these */
};
```

### Abilities

Activatable capabilities with costs, cooldowns, and tag requirements.

```cpp
bestow::AbilityDef dashDef{
    .name = "Dash",
    .activationPolicy = bestow::AbilityActivationPolicy::OnInputPressed,
    .cooldown = 2.0f,
    .costs = {{
        .attribute = staminaId,
        .cost = 25.0f
    }},
    .activationRequiredTags = /* owner must have */,
    .activationBlockedTags = /* owner must NOT have (e.g., stunned) */,
    .abilityTags = /* tags this ability has */,
    .cancelAbilitiesWithTags = /* cancel other abilities with these tags */,
    .blockAbilitiesWithTags = /* block abilities with these tags while active */,
    .effectsToApplyOnActivate = {speedBoostId},
    .effectsToApplyOnEnd = {}
};
auto dashId = gas->registerAbility(dashDef);

// Grant ability to entity
gas->grantAbility(player, dashId);

// Check and activate
if (gas->canActivateAbility(player, dashId)) {
    gas->tryActivateAbility(player, dashId);
}

// Query state
bool active = gas->isAbilityActive(player, dashId);
float cooldown = gas->getAbilityCooldown(player, dashId);

// End ability (for WhileInputHeld or manual control)
gas->endAbility(player, dashId);
```

**Activation Policies:**
- `OnInputPressed` - Activate once when input pressed
- `OnInputReleased` - Activate once when input released
- `WhileInputHeld` - Active while input held, ends on release
- `Passive` - Always active (auras, permanent buffs)

## Lua Definitions

Define all GAS data in Lua for data-driven design:

```lua
-- data/config/abilities.lua

Tags = {
    "State.Stunned",
    "State.Invulnerable",
    "State.Movement.Dashing",
    "Ability.Movement",
    "Ability.Attack",
}

Attributes = {
    {
        name = "Health",
        baseValue = 100,
        minValue = 0,
        maxValue = 100,
    },
    {
        name = "Stamina",
        baseValue = 100,
        minValue = 0,
        maxValue = 100,
    },
    {
        name = "MoveSpeed",
        baseValue = 200,
        minValue = 0,
        maxValue = 1000,
    },
}

Effects = {
    {
        name = "DashSpeedBoost",
        durationType = "duration",
        duration = 0.3,
        modifiers = {
            { attribute = "MoveSpeed", op = "multiply", value = 2.0 },
        },
        grantedTags = { "State.Movement.Dashing" },
    },
    {
        name = "HealthRegen",
        durationType = "duration",
        duration = 10,
        period = 1.0,
        modifiers = {
            { attribute = "Health", op = "add", value = 5 },
        },
    },
    {
        name = "Stun",
        durationType = "duration",
        duration = 2,
        grantedTags = { "State.Stunned" },
    },
}

Abilities = {
    {
        name = "Dash",
        activationPolicy = "onInputPressed",
        cooldown = 2.0,
        costs = {
            { attribute = "Stamina", cost = 25 },
        },
        activationBlockedTags = { "State.Stunned" },
        abilityTags = { "Ability.Movement" },
        effectsToApplyOnActivate = { "DashSpeedBoost" },
    },
    {
        name = "HealingAura",
        activationPolicy = "passive",
        effectsToApplyOnActivate = { "HealthRegen" },
    },
}
```

Load in C++:
```cpp
#include <fstream>
#include <sstream>

std::ifstream file("data/config/abilities.lua");
std::stringstream buffer;
buffer << file.rdbuf();

gas->loadDefinitionsFromLua(buffer.str());

// Now use by name
auto dashDef = gas->getAbilityDef("Dash");
auto healthDef = gas->getAttributeDef("Health");
```

## Callbacks

React to gameplay events:

```cpp
gas->setAttributeChangeCallback([](const bestow::AttributeChangeEvent& e) {
    if (e.newValue <= 0) {
        // Entity died
    }
});

gas->setEffectAppliedCallback([](const bestow::EffectAppliedEvent& e) {
    // Play VFX, sound, etc.
});

gas->setAbilityActivatedCallback([](const bestow::AbilityActivatedEvent& e) {
    // Trigger animations, sounds
});
```

## Update Loop

Call `update()` each frame to process effect durations and ability cooldowns:

```cpp
void gameLoop(float deltaTime) {
    gas->update(deltaTime);
    // ... rest of game logic
}
```

## Entity Lifecycle

```cpp
// Setup
gas->initializeComponent(entity);
gas->initializeAttribute(entity, healthId, 100.0f);
gas->initializeAttribute(entity, staminaId, 100.0f);
gas->grantAbility(entity, dashId);

// Cleanup
gas->removeAllEffects(entity);
gas->removeComponent(entity);
```

## Common Patterns

### Damage with Armor

```cpp
void dealDamage(Entity target, float rawDamage, Entity source) {
    float armor = gas->getAttributeValue(target, armorId);
    float reduction = armor / (armor + 100.0f);  // Diminishing returns
    float finalDamage = rawDamage * (1.0f - reduction);
    gas->modifyAttribute(target, healthId, -finalDamage);
}
```

### Conditional Abilities

```cpp
// Can only dash when grounded
dashDef.activationRequiredTags.addTag(groundedTag);
dashDef.activationBlockedTags.addTag(stunnedTag);
dashDef.activationBlockedTags.addTag(dashingTag);  // No double-dash
```

### Temporary Buffs on Pickup

```cpp
void onPickup(Entity player, Entity pickup) {
    // Apply effect directly
    gas->applyEffect(player, speedBoostEffectId, pickup);
}
```

### Room-Based Effects

```cpp
void onEnterRoom(Entity entity, Room& room) {
    if (room.hasTag("JumpZone")) {
        gas->addTag(entity, jumpAllowedTag);
    }
}

void onExitRoom(Entity entity, Room& room) {
    if (room.hasTag("JumpZone")) {
        gas->removeTag(entity, jumpAllowedTag);
    }
}

// Jump ability requires tag
jumpAbilityDef.activationRequiredTags.addTag(jumpAllowedTag);
```

## Best Practices

1. **Define in Lua** - Keep gameplay data in Lua files for easy iteration
2. **Use tags liberally** - Tags are cheap and enable flexible queries
3. **Separate concerns** - Effects modify attributes, abilities trigger effects
4. **Test incrementally** - Use unit tests to verify effect math and ability logic
5. **Callback sparingly** - Use callbacks for VFX/audio, not game logic

## EngineBuilder Integration (Planned)

Currently, the GAS System is created manually in game code. A planned enhancement will integrate it into the EngineBuilder:

```cpp
// Current approach - manual creation
auto engine = EngineBuilder()
    .withEntities()
    .build();

// Manual GAS setup
auto gas = bestow::createGASSystem();
gas->loadDefinitionsFromLua(abilitiesLua);

// In game loop - must call manually
void Game::update(DeltaTime dt) {
    gas->update(dt);  // Easy to forget!
}

// Planned approach - automatic integration
auto engine = EngineBuilder()
    .withEntities()
    .withGAS("data/config/abilities.lua")  // NEW - automatic GAS system
    .build();

// Access via engine systems
auto& gas = engine->systems().gas;
gas->grantAbility(player_, dashAbility);

// GAS automatically updated by engine - no manual update() needed
```

### Planned Features

| Feature | Description | Status |
|---------|-------------|--------|
| `.withGAS(path)` | Add GAS system with Lua definitions | 🔲 Planned |
| Auto update | Engine calls `gas->update(dt)` automatically | 🔲 Planned |
| Hot reload | Reload ability definitions during development | 🔲 Planned |
| Blueprint integration | Blueprints can reference GAS attributes/abilities | 🔲 Planned |

### Blueprint Integration

With the Blueprint Factory system, entities can be created with GAS components pre-configured:

```lua
-- data/blueprints/entities.lua
Blueprints = {
    Player = {
        components = {
            PlayerTag = {},
            Health = { current = 100, maximum = 100 }
        },
        gas = {
            attributes = {
                { name = "Health", value = 100 },
                { name = "Stamina", value = 100 },
                { name = "MoveSpeed", value = 200 }
            },
            abilities = { "Jump", "Dash", "Attack" }
        }
    }
}
```

```cpp
// Player created with GAS component automatically initialized
Entity player = factory->create("Player", 100, 200);
// GAS attributes and abilities already set up!
```

See [Blueprint Factory](Blueprint-Factory.md) for more details.

## See Also

- [Entity System](Entity-System.md) - Entities for GAS components
- [Blueprint Factory](Blueprint-Factory.md) - Data-driven entity creation with GAS
- [Level System](Level-System.md) - Level-based ability grants
- [Events System](Events-System.md) - Event-driven ability triggers

# Gameplay Ability System

> **Visibility:** Public (Lua + C++ API)
> **Tier:** 3
> **Dependencies:** Types, Entity, Events
> **Lua Paths:** `bestow.gas` (high-level), `bestow.gas.core` (low-level)

## Purpose

The Gameplay Ability System (GAS) provides a data-driven framework for managing gameplay tags, numeric attributes, status effects, and abilities on entities. Inspired by Unreal Engine's GAS, it enables designers to define attributes (health, mana, stamina), effects that modify those attributes (buffs, debuffs, damage-over-time), and abilities with activation costs, cooldowns, and tag requirements -- all configurable from Lua. The high-level API uses string-based lookups for rapid prototyping, while the low-level API uses registered IDs for maximum performance and exposes the full registration, callback, and query surface.

## High-Level API: `IGASSystem`

The simplified GAS interface for common gameplay tasks. Name-based attribute/effect/ability access, entity-centric operations, and Lua definition loading. No lifecycle methods -- the engine calls `update()` internally.

### Component Setup

| Method | Returns | Description |
|--------|---------|-------------|
| `initializeComponent(Entity entity)` | `Result<void>` | Add the GAS component to an entity, enabling tags, attributes, effects, and abilities on it |

### Attributes

| Method | Returns | Description |
|--------|---------|-------------|
| `initializeAttribute(Entity entity, std::string_view name, float value)` | `Result<void>` | Initialize a named attribute on the entity with the given base value |
| `getAttribute(Entity entity, std::string_view name)` | `float` | Get the current computed value of a named attribute, including all active modifiers |
| `modifyAttribute(Entity entity, std::string_view name, float delta)` | `Result<void>` | Apply an additive delta to a named attribute's base value |

### Effects

| Method | Returns | Description |
|--------|---------|-------------|
| `applyEffect(Entity target, std::string_view effectName, Entity source)` | `Result<void>` | Apply a named effect to the target entity, optionally specifying the source entity |

### Abilities

| Method | Returns | Description |
|--------|---------|-------------|
| `grantAbility(Entity entity, std::string_view abilityName)` | `Result<void>` | Grant a named ability to the entity, making it available for activation |
| `tryActivateAbility(Entity entity, std::string_view abilityName)` | `Result<bool>` | Attempt to activate a named ability; returns true if activation succeeded, false if preconditions failed |

### Tags

| Method | Returns | Description |
|--------|---------|-------------|
| `addTag(Entity entity, std::string_view tag)` | `Result<void>` | Add a gameplay tag to the entity by its dot-separated name (e.g., "Status.Burning") |
| `hasTag(Entity entity, std::string_view tag)` | `bool` | Check whether the entity currently carries the given gameplay tag |

### Definitions

| Method | Returns | Description |
|--------|---------|-------------|
| `loadDefinitions(std::string_view luaPath)` | `Result<void>` | Load attribute, effect, ability, and tag definitions from a Lua file at the given path |

## Low-Level API: `IGASCore`

Full control API. Exposes tag/attribute/effect/ability registration with typed IDs, detailed entity component management, fine-grained attribute operations, effect lifecycle control, ability state queries, event callbacks, and Lua definition parsing.

### Lifecycle

| Method | Returns | Description |
|--------|---------|-------------|
| `update(DeltaTime dt)` | `void` | Tick all active effects (duration, periodic ticks), process cooldowns, and remove expired effects |

### Tag Registration

| Method | Returns | Description |
|--------|---------|-------------|
| `registerTag(std::string_view name)` | `GameplayTag` | Register a gameplay tag by its hierarchical name and return its typed handle |
| `findTag(std::string_view name)` | `std::optional<GameplayTag>` | Look up a previously registered tag by name, or return nullopt if not found |
| `isChildOf(GameplayTag child, GameplayTag parent)` | `bool` | Check whether a tag is a descendant of another in the tag hierarchy (e.g., "Status.Burning" is child of "Status") |

### Attribute Registration

| Method | Returns | Description |
|--------|---------|-------------|
| `registerAttribute(const AttributeDef& def)` | `AttributeId` | Register an attribute definition and return its typed ID for use in operations |
| `getAttributeDef(AttributeId id)` | `std::optional<AttributeDef>` | Retrieve an attribute definition by its ID |
| `getAttributeDef(std::string_view name)` | `std::optional<AttributeDef>` | Retrieve an attribute definition by its name |

### Effect Registration

| Method | Returns | Description |
|--------|---------|-------------|
| `registerEffect(const EffectDef& def)` | `EffectId` | Register an effect definition and return its typed ID |
| `getEffectDef(EffectId id)` | `std::optional<EffectDef>` | Retrieve an effect definition by its ID |

### Ability Registration

| Method | Returns | Description |
|--------|---------|-------------|
| `registerAbility(const AbilityDef& def)` | `AbilityId` | Register an ability definition and return its typed ID |
| `getAbilityDef(AbilityId id)` | `std::optional<AbilityDef>` | Retrieve an ability definition by its ID |

### Entity Component

| Method | Returns | Description |
|--------|---------|-------------|
| `initializeComponent(Entity entity)` | `Result<void>` | Add the GAS component to an entity, initializing internal storage for tags, attributes, effects, and abilities |
| `removeComponent(Entity entity)` | `Result<void>` | Remove the GAS component from an entity, cleaning up all active effects and granted abilities |
| `hasComponent(Entity entity)` | `bool` | Check whether an entity has a GAS component |

### Tag Operations

| Method | Returns | Description |
|--------|---------|-------------|
| `addTag(Entity entity, GameplayTag tag)` | `Result<void>` | Add a gameplay tag to the entity using its registered tag handle |
| `removeTag(Entity entity, GameplayTag tag)` | `Result<void>` | Remove a gameplay tag from the entity |
| `hasTag(Entity entity, GameplayTag tag)` | `bool` | Check whether the entity carries the given gameplay tag |
| `getTags(Entity entity)` | `std::vector<GameplayTag>` | Return all gameplay tags currently on the entity |

### Attribute Operations

| Method | Returns | Description |
|--------|---------|-------------|
| `initializeAttribute(Entity entity, AttributeId id, float baseValue)` | `Result<void>` | Initialize an attribute on the entity with the given base value |
| `getAttributeValue(Entity entity, AttributeId id)` | `float` | Get the current computed value of an attribute, after all active modifiers are applied |
| `getAttributeBaseValue(Entity entity, AttributeId id)` | `float` | Get the unmodified base value of an attribute, before any effects |
| `setAttributeBaseValue(Entity entity, AttributeId id, float value)` | `Result<void>` | Directly set the base value of an attribute, bypassing effect modifiers |
| `modifyAttribute(Entity entity, AttributeId id, float delta)` | `Result<void>` | Apply an additive change to the attribute's base value |

### Effect Operations

| Method | Returns | Description |
|--------|---------|-------------|
| `applyEffect(Entity target, EffectId effect, Entity source)` | `Result<void>` | Apply an effect to the target entity, optionally attributing it to a source entity |
| `removeEffect(Entity entity, EffectId effect)` | `Result<void>` | Remove all instances of a specific effect from the entity |
| `removeAllEffects(Entity entity)` | `void` | Remove every active effect from the entity |
| `hasEffect(Entity entity, EffectId effect)` | `bool` | Check whether the entity has at least one active instance of the given effect |
| `getActiveEffects(Entity entity)` | `std::vector<ActiveEffect>` | Return all active effects on the entity with their remaining durations and source information |

### Ability Operations

| Method | Returns | Description |
|--------|---------|-------------|
| `grantAbility(Entity entity, AbilityId ability)` | `Result<void>` | Grant an ability to the entity, making it available for activation |
| `removeAbility(Entity entity, AbilityId ability)` | `Result<void>` | Remove a granted ability from the entity; fails if the ability is currently active |
| `hasAbility(Entity entity, AbilityId ability)` | `bool` | Check whether the entity has been granted the given ability |
| `canActivateAbility(Entity entity, AbilityId ability)` | `bool` | Check whether all preconditions (cooldown, tags, cost) are met for ability activation |
| `tryActivateAbility(Entity entity, AbilityId ability)` | `Result<bool>` | Attempt to activate the ability; returns true if activation started, false if preconditions failed |
| `endAbility(Entity entity, AbilityId ability)` | `Result<void>` | Forcefully end an active ability, triggering its cleanup logic |
| `isAbilityActive(Entity entity, AbilityId ability)` | `bool` | Check whether the given ability is currently active on the entity |
| `getAbilityCooldown(Entity entity, AbilityId ability)` | `float` | Return the remaining cooldown time in seconds for the ability, or 0 if ready |

### Callbacks

| Method | Returns | Description |
|--------|---------|-------------|
| `onAttributeChanged(std::function<void(const AttributeChangeEvent&)> cb)` | `SubscriptionId` | Subscribe to notifications when any attribute value changes on any entity |
| `onEffectApplied(std::function<void(const EffectAppliedEvent&)> cb)` | `SubscriptionId` | Subscribe to notifications when any effect is applied to any entity |
| `onAbilityActivated(std::function<void(const AbilityActivatedEvent&)> cb)` | `SubscriptionId` | Subscribe to notifications when any ability is activated on any entity |
| `unsubscribe(SubscriptionId id)` | `void` | Remove a previously registered callback subscription |

### Lua Definitions

| Method | Returns | Description |
|--------|---------|-------------|
| `loadDefinitionsFromLua(std::string_view source)` | `Result<void>` | Parse and register attribute, effect, ability, and tag definitions from Lua source code |

## Types

### GameplayTag

A strong typed handle representing a registered gameplay tag. Tags use a hierarchical dot-separated naming convention (e.g., "Status.Debuff.Poison"). Child tags inherit from their parents for matching purposes.

```cpp
using GameplayTag = Handle<struct GameplayTagTag>;
```

| Field | Type | Default | Description |
|-------|------|---------|-------------|
| `id` | `std::uint64_t` | `0` | Internal identifier; 0 means invalid/unregistered |

### AttributeId

A strong typed handle representing a registered attribute type.

```cpp
using AttributeId = Handle<struct AttributeTag>;
```

| Field | Type | Default | Description |
|-------|------|---------|-------------|
| `id` | `std::uint64_t` | `0` | Internal identifier; 0 means invalid/unregistered |

### AttributeDef

Definition of a gameplay attribute, specifying its name, default range, and clamping behavior.

| Field | Type | Default | Description |
|-------|------|---------|-------------|
| `name` | `std::string` | `""` | Unique name of the attribute (e.g., "Health", "Mana", "Stamina") |
| `defaultValue` | `float` | `0.0f` | The initial base value when the attribute is first added to an entity |
| `minValue` | `float` | `0.0f` | The minimum allowed value after all modifiers are applied |
| `maxValue` | `float` | `100.0f` | The maximum allowed value after all modifiers are applied |
| `clamp` | `bool` | `true` | Whether to enforce min/max clamping on the computed value |

### EffectId

A strong typed handle representing a registered effect type.

```cpp
using EffectId = Handle<struct EffectTag>;
```

| Field | Type | Default | Description |
|-------|------|---------|-------------|
| `id` | `std::uint64_t` | `0` | Internal identifier; 0 means invalid/unregistered |

### EffectDef

Definition of a gameplay effect, specifying which attributes it modifies and how.

| Field | Type | Default | Description |
|-------|------|---------|-------------|
| `name` | `std::string` | `""` | Unique name of the effect (e.g., "PoisonDOT", "StrengthBuff") |
| `duration` | `float` | `0.0f` | Duration in seconds; 0 means instant, negative means infinite |
| `period` | `float` | `0.0f` | Tick interval in seconds for periodic effects; 0 means no periodic ticking |
| `stackable` | `bool` | `false` | Whether multiple instances of this effect can be active simultaneously |
| `maxStacks` | `int` | `1` | Maximum number of simultaneous stacks if stackable |
| `modifiers` | `std::vector<AttributeModifier>` | `{}` | List of attribute modifications applied by this effect |
| `requiredTags` | `std::vector<GameplayTag>` | `{}` | Tags the target must have for the effect to be applied |
| `blockedByTags` | `std::vector<GameplayTag>` | `{}` | Tags that prevent this effect from being applied |
| `grantedTags` | `std::vector<GameplayTag>` | `{}` | Tags added to the entity while this effect is active |

### AttributeModifier

A single modification operation within an effect definition.

| Field | Type | Default | Description |
|-------|------|---------|-------------|
| `attribute` | `AttributeId` | `{}` | The target attribute to modify |
| `operation` | `ModifierOp` | `Add` | The type of modification to apply |
| `magnitude` | `float` | `0.0f` | The numerical value of the modification |

### ModifierOp

```cpp
enum class ModifierOp : std::uint8_t {
    Add,        // Base value += magnitude
    Multiply,   // Base value *= magnitude
    Override    // Base value = magnitude
};
```

| Value | Description |
|-------|-------------|
| `Add` | Add the magnitude to the attribute's base value |
| `Multiply` | Multiply the attribute's base value by the magnitude |
| `Override` | Replace the attribute's base value with the magnitude |

### AbilityId

A strong typed handle representing a registered ability type.

```cpp
using AbilityId = Handle<struct AbilityTag>;
```

| Field | Type | Default | Description |
|-------|------|---------|-------------|
| `id` | `std::uint64_t` | `0` | Internal identifier; 0 means invalid/unregistered |

### AbilityDef

Definition of a gameplay ability, specifying activation costs, cooldowns, and tag requirements.

| Field | Type | Default | Description |
|-------|------|---------|-------------|
| `name` | `std::string` | `""` | Unique name of the ability (e.g., "Fireball", "Dash", "HealingAura") |
| `cooldown` | `float` | `0.0f` | Cooldown duration in seconds after activation before the ability can be used again |
| `costAttribute` | `AttributeId` | `{}` | The attribute consumed when the ability is activated (e.g., Mana) |
| `costAmount` | `float` | `0.0f` | The amount of the cost attribute consumed per activation |
| `duration` | `float` | `0.0f` | Duration of the ability in seconds; 0 means instantaneous |
| `requiredTags` | `std::vector<GameplayTag>` | `{}` | Tags the entity must have for the ability to activate |
| `blockedByTags` | `std::vector<GameplayTag>` | `{}` | Tags that prevent the ability from activating |
| `grantedTags` | `std::vector<GameplayTag>` | `{}` | Tags added to the entity while the ability is active |
| `cancelledByTags` | `std::vector<GameplayTag>` | `{}` | Tags that will force-cancel this ability if added to the entity |
| `appliedEffects` | `std::vector<EffectId>` | `{}` | Effects automatically applied to the entity when the ability activates |

### ActiveEffect

Runtime state of a currently active effect on an entity.

| Field | Type | Default | Description |
|-------|------|---------|-------------|
| `effectId` | `EffectId` | `{}` | The registered ID of the effect definition |
| `source` | `Entity` | `NullEntity` | The entity that applied this effect, or NullEntity if no source |
| `remainingDuration` | `float` | `0.0f` | Seconds remaining before this effect expires; negative means infinite |
| `elapsedTime` | `float` | `0.0f` | Total seconds this effect has been active |
| `stackCount` | `int` | `1` | Current number of stacks for this effect |

### AttributeChangeEvent

Event data emitted when an attribute value changes on an entity.

| Field | Type | Default | Description |
|-------|------|---------|-------------|
| `entity` | `Entity` | `NullEntity` | The entity whose attribute changed |
| `attributeId` | `AttributeId` | `{}` | The ID of the attribute that changed |
| `oldValue` | `float` | `0.0f` | The previous computed value before the change |
| `newValue` | `float` | `0.0f` | The new computed value after the change |
| `source` | `Entity` | `NullEntity` | The entity responsible for the change, if any |

### EffectAppliedEvent

Event data emitted when an effect is applied to an entity.

| Field | Type | Default | Description |
|-------|------|---------|-------------|
| `target` | `Entity` | `NullEntity` | The entity receiving the effect |
| `effectId` | `EffectId` | `{}` | The ID of the effect that was applied |
| `source` | `Entity` | `NullEntity` | The entity that applied the effect |

### AbilityActivatedEvent

Event data emitted when an ability is activated on an entity.

| Field | Type | Default | Description |
|-------|------|---------|-------------|
| `entity` | `Entity` | `NullEntity` | The entity that activated the ability |
| `abilityId` | `AbilityId` | `{}` | The ID of the ability that was activated |

## Lua Examples

```lua
-- Load all GAS definitions from a Lua file
bestow.gas.loadDefinitions("config/abilities.lua")

-- High-level: Set up a player entity
local player = bestow.entity.create()
bestow.gas.initializeComponent(player)
bestow.gas.initializeAttribute(player, "Health", 100)
bestow.gas.initializeAttribute(player, "Mana", 50)
bestow.gas.initializeAttribute(player, "Stamina", 80)

-- Read attributes
local hp = bestow.gas.getAttribute(player, "Health")
print("Player HP: " .. hp)

-- Modify attributes
bestow.gas.modifyAttribute(player, "Health", -25)

-- Apply effects by name
bestow.gas.applyEffect(player, "PoisonDOT", enemy)

-- Grant and use abilities
bestow.gas.grantAbility(player, "Fireball")
local activated, err = bestow.gas.tryActivateAbility(player, "Fireball")
if activated then
    print("Fireball cast!")
end

-- Tags
bestow.gas.addTag(player, "Status.Burning")
if bestow.gas.hasTag(player, "Status.Burning") then
    print("Player is on fire!")
end

-- Low-level: Registration and callbacks
local healthId = bestow.gas.core.registerAttribute({
    name = "Health",
    defaultValue = 100,
    minValue = 0,
    maxValue = 200,
    clamp = true,
})

local poisonTag = bestow.gas.core.registerTag("Status.Debuff.Poison")
local debuffTag = bestow.gas.core.findTag("Status.Debuff")
print(bestow.gas.core.isChildOf(poisonTag, debuffTag))  -- true

local sub = bestow.gas.core.onAttributeChanged(function(event)
    print(event.entity, event.attributeId, event.oldValue, event.newValue)
end)

-- Query active effects
local effects = bestow.gas.core.getActiveEffects(player)
for _, e in ipairs(effects) do
    print(e.effectId, e.remainingDuration, e.stackCount)
end

-- Ability state queries
local cd = bestow.gas.core.getAbilityCooldown(player, fireballId)
local canUse = bestow.gas.core.canActivateAbility(player, fireballId)

bestow.gas.core.unsubscribe(sub)
```

## C++ Examples

```cpp
// Register definitions
AttributeId healthId = gasCore->registerAttribute({
    .name = "Health",
    .defaultValue = 100.0f,
    .minValue = 0.0f,
    .maxValue = 200.0f,
    .clamp = true
});

GameplayTag burningTag = gasCore->registerTag("Status.Burning");
GameplayTag debuffTag = gasCore->registerTag("Status.Debuff");
bool isChild = gasCore->isChildOf(burningTag, debuffTag);

EffectId poisonEffect = gasCore->registerEffect({
    .name = "PoisonDOT",
    .duration = 10.0f,
    .period = 2.0f,
    .stackable = true,
    .maxStacks = 3,
    .modifiers = {{healthId, ModifierOp::Add, -5.0f}},
    .grantedTags = {burningTag}
});

AbilityId fireballAbility = gasCore->registerAbility({
    .name = "Fireball",
    .cooldown = 3.0f,
    .costAttribute = manaId,
    .costAmount = 25.0f,
    .duration = 0.0f,
    .appliedEffects = {burnEffect}
});

// Entity setup
gasCore->initializeComponent(player);
gasCore->initializeAttribute(player, healthId, 100.0f);

// Operations
gasCore->modifyAttribute(player, healthId, -25.0f);
float currentHP = gasCore->getAttributeValue(player, healthId);
float baseHP = gasCore->getAttributeBaseValue(player, healthId);

gasCore->applyEffect(player, poisonEffect, enemy);
bool poisoned = gasCore->hasEffect(player, poisonEffect);
auto activeEffects = gasCore->getActiveEffects(player);

gasCore->grantAbility(player, fireballAbility);
if (gasCore->canActivateAbility(player, fireballAbility)) {
    auto result = gasCore->tryActivateAbility(player, fireballAbility);
}
float cd = gasCore->getAbilityCooldown(player, fireballAbility);

// Callbacks
auto subId = gasCore->onAttributeChanged([](const AttributeChangeEvent& e) {
    if (e.newValue <= 0.0f) {
        // Entity died
    }
});

gasCore->unsubscribe(subId);

// High-level usage
gas->initializeComponent(player);
gas->initializeAttribute(player, "Health", 100.0f);
gas->applyEffect(player, "PoisonDOT", enemy);
gas->grantAbility(player, "Fireball");
auto activated = gas->tryActivateAbility(player, "Fireball");
gas->addTag(player, "Status.Burning");
```

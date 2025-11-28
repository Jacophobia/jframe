# GASSystem API

The Gameplay Ability System (GAS) provides tags, attributes, effects, and abilities for gameplay mechanics.

## Overview

```cpp
auto& gas = sys.gas;

// Register tag
GameplayTag stunned = gas->registerTag("State.StatusEffect.Stunned");

// Register attribute
AttributeId health = gas->registerAttribute({
    .name = "Health",
    .baseValue = 100.0f,
    .minValue = 0.0f,
    .maxValue = 100.0f
});

// Initialize entity
gas->initializeComponent(player);
gas->initializeAttribute(player, health, 100.0f);

// Apply effect
EffectId damageEffect = gas->registerEffect({
    .name = "Damage",
    .durationType = EffectDurationType::Instant,
    .modifiers = {{.attribute = health, .op = EffectModifierOp::Add, .value = -25.0f}}
});
gas->applyEffect(player, damageEffect, attacker);

// Grant ability
AbilityId dashAbility = gas->registerAbility({
    .name = "Dash",
    .cooldown = 2.0f
    // ...
});
gas->grantAbility(player, dashAbility);

// Activate ability
if (gas->canActivateAbility(player, dashAbility)) {
    gas->tryActivateAbility(player, dashAbility);
}
```

## Lifecycle

### update(DeltaTime dt)

```cpp
void update(DeltaTime dt);
```

Updates ability system (cooldowns, effect durations, periodic effects).

**Call in:** `Application::updateFixed()`

---

## Gameplay Tags

Tags are hierarchical identifiers for categorizing gameplay state.

### registerTag(const std::string& name)

```cpp
GameplayTag registerTag(const std::string& name);
```

Registers a tag with a hierarchical name (e.g., "State.Movement.Dashing").

**Example:**

```cpp
auto dashing = gas->registerTag("State.Movement.Dashing");
auto stunned = gas->registerTag("State.StatusEffect.Stunned");
auto invulnerable = gas->registerTag("State.StatusEffect.Invulnerable");
auto immune = gas->registerTag("Trait.Immune.Fire");
```

---

### findTag(const std::string& name)

```cpp
std::optional<GameplayTag> findTag(const std::string& name) const;
```

Finds a tag by name.

---

### isParentOf(parent, child)

```cpp
bool isParentOf(const GameplayTag& parent, const GameplayTag& child) const;
```

Checks if one tag is a parent of another in the hierarchy.

**Example:**

```cpp
auto movement = gas->registerTag("State.Movement");
auto dashing = gas->registerTag("State.Movement.Dashing");

gas->isParentOf(movement, dashing);  // true
```

---

### Tag Operations on Entities

```cpp
void addTag(Entity entity, const GameplayTag& tag);
void removeTag(Entity entity, const GameplayTag& tag);
bool hasTag(Entity entity, const GameplayTag& tag) const;
const GameplayTagContainer* getTags(Entity entity) const;
```

**Example:**

```cpp
// Add invulnerability during dash
gas->addTag(player, invulnerableTag);

// Check for stun
if (gas->hasTag(enemy, stunnedTag)) {
    // Enemy is stunned
}

// Remove tag
gas->removeTag(player, invulnerableTag);
```

---

## Attributes

Attributes are numeric values with base/current tracking.

### registerAttribute(const AttributeDef& def)

```cpp
AttributeId registerAttribute(const AttributeDef& def);
```

Registers an attribute definition.

**AttributeDef:**

```cpp
struct AttributeDef {
    AttributeId id = 0;
    std::string name;
    float baseValue = 0.0f;
    float minValue = 0.0f;
    float maxValue = std::numeric_limits<float>::max();
    bool clampEnabled = true;
};
```

**Example:**

```cpp
AttributeId health = gas->registerAttribute({
    .name = "Health",
    .baseValue = 100.0f,
    .minValue = 0.0f,
    .maxValue = 100.0f
});

AttributeId moveSpeed = gas->registerAttribute({
    .name = "MoveSpeed",
    .baseValue = 200.0f,
    .minValue = 0.0f,
    .maxValue = 500.0f
});

AttributeId jumpHeight = gas->registerAttribute({
    .name = "JumpHeight",
    .baseValue = 500.0f,
    .minValue = 100.0f,
    .maxValue = 1000.0f
});
```

---

### getAttributeDef

```cpp
std::optional<AttributeDef> getAttributeDef(AttributeId id) const;
std::optional<AttributeDef> getAttributeDef(const std::string& name) const;
```

---

### Attribute Operations on Entities

```cpp
void initializeAttribute(Entity entity, AttributeId id, float baseValue);
float getAttributeValue(Entity entity, AttributeId id) const;
float getAttributeBaseValue(Entity entity, AttributeId id) const;
void setAttributeBaseValue(Entity entity, AttributeId id, float value);
void modifyAttribute(Entity entity, AttributeId id, float delta);
```

**Example:**

```cpp
// Initialize player health
gas->initializeAttribute(player, health, 100.0f);

// Get current health
float currentHealth = gas->getAttributeValue(player, health);

// Take damage
gas->modifyAttribute(player, health, -25.0f);

// Permanent stat upgrade
float base = gas->getAttributeBaseValue(player, health);
gas->setAttributeBaseValue(player, health, base + 20.0f);
```

---

## Gameplay Effects

Effects modify attributes and grant temporary tags.

### registerEffect(const EffectDef& def)

```cpp
EffectId registerEffect(const EffectDef& def);
```

Registers an effect definition.

**EffectDef:**

```cpp
struct EffectDef {
    EffectId id = 0;
    std::string name;
    EffectDurationType durationType = EffectDurationType::Instant;
    float duration = 0.0f;          // For Duration type
    float period = 0.0f;            // If > 0, ticks periodically (DoT/HoT)
    std::vector<EffectModifier> modifiers;
    GameplayTagContainer grantedTags;
    GameplayTagContainer applicationRequiredTags;
    GameplayTagContainer applicationBlockedTags;
    GameplayTagContainer removalTags;
    bool stackable = false;
    int maxStacks = 1;
};

enum class EffectDurationType : uint8_t {
    Instant,    // Apply once and remove
    Duration,   // Apply for a set time
    Infinite    // Apply until explicitly removed
};

struct EffectModifier {
    AttributeId attribute;
    EffectModifierOp op = EffectModifierOp::Add;
    float value = 0.0f;
};

enum class EffectModifierOp : uint8_t {
    Add,        // +value
    Multiply,   // *value
    Override    // =value
};
```

**Example:**

```cpp
// Instant damage
EffectId damage = gas->registerEffect({
    .name = "Damage",
    .durationType = EffectDurationType::Instant,
    .modifiers = {{
        .attribute = health,
        .op = EffectModifierOp::Add,
        .value = -25.0f
    }}
});

// Damage over time (poison)
EffectId poison = gas->registerEffect({
    .name = "Poison",
    .durationType = EffectDurationType::Duration,
    .duration = 5.0f,
    .period = 1.0f,  // Tick every second
    .modifiers = {{
        .attribute = health,
        .op = EffectModifierOp::Add,
        .value = -5.0f
    }},
    .grantedTags = {poisonedTag}
});

// Speed boost
EffectId speedBoost = gas->registerEffect({
    .name = "SpeedBoost",
    .durationType = EffectDurationType::Duration,
    .duration = 3.0f,
    .modifiers = {{
        .attribute = moveSpeed,
        .op = EffectModifierOp::Multiply,
        .value = 1.5f  // 50% faster
    }},
    .grantedTags = {hasteTag}
});

// Stun (no attribute changes, just tags)
EffectId stun = gas->registerEffect({
    .name = "Stun",
    .durationType = EffectDurationType::Duration,
    .duration = 2.0f,
    .grantedTags = {stunnedTag},
    .applicationBlockedTags = {immuneToStunTag}
});
```

---

### Effect Operations

```cpp
void applyEffect(Entity target, EffectId effectId, Entity source);
void removeEffect(Entity entity, EffectId effectId);
void removeAllEffects(Entity entity);
bool hasEffect(Entity entity, EffectId effectId) const;
std::vector<ActiveEffect> getActiveEffects(Entity entity) const;
```

**Example:**

```cpp
// Apply damage
gas->applyEffect(enemy, damageEffect, player);

// Apply poison on hit
if (entities->allOf<PoisonWeapon>(weapon)) {
    gas->applyEffect(target, poisonEffect, attacker);
}

// Cleanse all effects
if (input->wasActionJustPressed("Cleanse")) {
    gas->removeAllEffects(player);
}

// Check for specific effect
if (gas->hasEffect(player, stunnedEffect)) {
    return;  // Can't act while stunned
}
```

---

## Gameplay Abilities

Abilities are activatable capabilities with costs and cooldowns.

### registerAbility(const AbilityDef& def)

```cpp
AbilityId registerAbility(const AbilityDef& def);
```

Registers an ability definition.

**AbilityDef:**

```cpp
struct AbilityDef {
    AbilityId id = 0;
    std::string name;
    AbilityActivationPolicy activationPolicy = AbilityActivationPolicy::OnInputPressed;
    float cooldown = 0.0f;
    std::vector<AbilityCost> costs;
    GameplayTagContainer activationRequiredTags;
    GameplayTagContainer activationBlockedTags;
    GameplayTagContainer abilityTags;
    GameplayTagContainer cancelAbilitiesWithTags;
    GameplayTagContainer blockAbilitiesWithTags;
    std::vector<EffectId> effectsToApplyOnActivate;
    std::vector<EffectId> effectsToApplyOnEnd;
};

enum class AbilityActivationPolicy : uint8_t {
    OnInputPressed,
    OnInputReleased,
    WhileInputHeld,
    Passive
};

struct AbilityCost {
    AttributeId attribute;
    float cost = 0.0f;
};
```

**Example:**

```cpp
// Dash ability
AbilityId dash = gas->registerAbility({
    .name = "Dash",
    .activationPolicy = AbilityActivationPolicy::OnInputPressed,
    .cooldown = 2.0f,
    .costs = {{.attribute = stamina, .cost = 20.0f}},
    .activationBlockedTags = {stunnedTag, dashingTag},
    .abilityTags = {dashAbilityTag},
    .effectsToApplyOnActivate = {dashSpeedEffect, invulnerabilityEffect}
});

// Heal ability
AbilityId heal = gas->registerAbility({
    .name = "Heal",
    .cooldown = 10.0f,
    .costs = {{.attribute = mana, .cost = 50.0f}},
    .effectsToApplyOnActivate = {healEffect}
});

// Ultimate ability
AbilityId ultimate = gas->registerAbility({
    .name = "Ultimate",
    .cooldown = 60.0f,
    .costs = {{.attribute = ultimate_charge, .cost = 100.0f}},
    .activationRequiredTags = {canUseUltimateTag},
    .cancelAbilitiesWithTags = {dashAbilityTag},  // Cancel dash if active
    .effectsToApplyOnActivate = {ultimateEffect}
});
```

---

### Ability Operations

```cpp
void grantAbility(Entity entity, AbilityId abilityId);
void removeAbility(Entity entity, AbilityId abilityId);
bool hasAbility(Entity entity, AbilityId abilityId) const;
bool canActivateAbility(Entity entity, AbilityId abilityId) const;
bool tryActivateAbility(Entity entity, AbilityId abilityId);
void endAbility(Entity entity, AbilityId abilityId);
bool isAbilityActive(Entity entity, AbilityId abilityId) const;
float getAbilityCooldown(Entity entity, AbilityId abilityId) const;
```

**Example:**

```cpp
// Grant ability on level up
gas->grantAbility(player, dashAbility);

// Input handling
if (input->wasActionJustPressed("Dash")) {
    if (gas->canActivateAbility(player, dashAbility)) {
        if (gas->tryActivateAbility(player, dashAbility)) {
            // Success - apply dash velocity
            physics->setVelocity(player, Vec2{400.0f, 0.0f});
        }
    }
}

// Check cooldown for UI
float cooldown = gas->getAbilityCooldown(player, dashAbility);
if (cooldown > 0.0f) {
    drawCooldownOverlay(cooldown);
}
```

---

## Component Management

### initializeComponent(Entity entity)

```cpp
void initializeComponent(Entity entity);
```

Initializes the GAS component on an entity.

---

### removeComponent(Entity entity)

```cpp
void removeComponent(Entity entity);
```

Removes the GAS component from an entity.

---

### hasComponent / getComponent

```cpp
bool hasComponent(Entity entity) const;
AbilitySystemComponent* getComponent(Entity entity);
const AbilitySystemComponent* getComponent(Entity entity) const;
```

---

## Callbacks

### setAttributeChangeCallback

```cpp
using AttributeChangeCallback = std::function<void(const AttributeChangeEvent&)>;
void setAttributeChangeCallback(AttributeChangeCallback callback);
```

**AttributeChangeEvent:**

```cpp
struct AttributeChangeEvent {
    Entity entity;
    AttributeId attribute;
    float oldValue;
    float newValue;
};
```

**Example:**

```cpp
gas->setAttributeChangeCallback([](const AttributeChangeEvent& e) {
    if (e.attribute == healthAttr && e.newValue <= 0.0f) {
        // Entity died
        handleDeath(e.entity);
    }
});
```

---

### setEffectAppliedCallback

```cpp
using EffectAppliedCallback = std::function<void(const EffectAppliedEvent&)>;
void setEffectAppliedCallback(EffectAppliedCallback callback);
```

---

### setAbilityActivatedCallback

```cpp
using AbilityActivatedCallback = std::function<void(const AbilityActivatedEvent&)>;
void setAbilityActivatedCallback(AbilityActivatedCallback callback);
```

---

## Lua Integration

### loadDefinitionsFromLua(const std::string& luaSource)

```cpp
bool loadDefinitionsFromLua(const std::string& luaSource);
```

Loads tags, attributes, effects, and abilities from Lua.

**Example Lua File:**

```lua
-- gas_definitions.lua

return {
    tags = {
        "State.Movement.Dashing",
        "State.StatusEffect.Stunned",
        "State.StatusEffect.Poisoned",
        "Trait.Immune.Fire"
    },

    attributes = {
        {name = "Health", baseValue = 100, minValue = 0, maxValue = 100},
        {name = "Stamina", baseValue = 100, minValue = 0, maxValue = 100},
        {name = "MoveSpeed", baseValue = 200, minValue = 0, maxValue = 500}
    },

    effects = {
        {
            name = "Damage",
            durationType = "instant",
            modifiers = {
                {attribute = "Health", op = "add", value = -25}
            }
        },
        {
            name = "Poison",
            durationType = "duration",
            duration = 5,
            period = 1,
            modifiers = {
                {attribute = "Health", op = "add", value = -5}
            },
            grantedTags = {"State.StatusEffect.Poisoned"}
        }
    },

    abilities = {
        {
            name = "Dash",
            cooldown = 2,
            costs = {{attribute = "Stamina", cost = 20}},
            blockedBy = {"State.StatusEffect.Stunned"}
        }
    }
}
```

---

## Common Patterns

See the full API reference for complete examples of:
- RPG stat system
- Buff/debuff effects
- Ability combos
- Status effect immunity
- Damage calculation

## See Also

- [EntitySystem](EntitySystem.md) - Component management
- [PhysicsSystem](PhysicsSystem.md) - Ability-driven movement
- [EventSystem](EventSystem.md) - Ability events

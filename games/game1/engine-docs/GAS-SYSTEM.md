# Gameplay Ability System (GAS)

The Gameplay Ability System provides a flexible, data-driven framework for implementing RPG-style mechanics: abilities, effects, attributes, and tags. Inspired by Unreal Engine's Gameplay Ability System, it enables complex character progression, status effects, and ability interactions with minimal code.

## Quick Start

```cpp
// Initialize GAS for an entity
Entity player = sys.entities->createEntity();
sys.gas->initializeComponent(player);

// Register and apply an attribute
AttributeDef healthDef{
    .name = "Health",
    .baseValue = 100.0f,
    .minValue = 0.0f,
    .maxValue = 100.0f
};
AttributeId healthId = sys.gas->registerAttribute(healthDef);
sys.gas->initializeAttribute(player, healthId, 100.0f);

// Grant an ability
AbilityDef dashDef{
    .name = "Dash",
    .cooldown = 3.0f
};
AbilityId dashId = sys.gas->registerAbility(dashDef);
sys.gas->grantAbility(player, dashId);

// Activate the ability
if (sys.gas->tryActivateAbility(player, dashId)) {
    // Dash activated!
}
```

## Core Concepts

### 1. Gameplay Tags

**Hierarchical identifiers** for categorizing abilities, effects, and states.

**Format:** Dot-separated hierarchy (e.g., `"State.Movement.Running"`)

**Use cases:**
- State tracking: `"State.Grounded"`, `"State.Stunned"`
- Ability categorization: `"Ability.Attack"`, `"Ability.Movement"`
- Effect filtering: Block effects on immune targets
- Requirements: Can only jump when grounded

```cpp
// Register tags
GameplayTag groundedTag = sys.gas->registerTag("State.Grounded");
GameplayTag stunned = sys.gas->registerTag("State.Combat.Stunned");

// Add/remove tags
sys.gas->addTag(player, groundedTag);
sys.gas->removeTag(player, groundedTag);

// Query tags
if (sys.gas->hasTag(player, stunned)) {
    // Player can't act while stunned
}

// Hierarchy
GameplayTag movement = sys.gas->registerTag("State.Movement");
GameplayTag running = sys.gas->registerTag("State.Movement.Running");
if (sys.gas->isParentOf(movement, running)) {
    // "State.Movement" is parent of "State.Movement.Running"
}
```

### 2. Attributes

**Numeric stats** with base/current value tracking and automatic modifier application.

**Properties:**
- **Base value** - The fundamental stat (strength, max health)
- **Current value** - After all modifiers (buffs, debuffs, equipment)
- **Min/Max** - Clamping boundaries
- **Callbacks** - Notifications on change

```cpp
// Define and register
AttributeDef manaDef{
    .name = "Mana",
    .baseValue = 50.0f,
    .minValue = 0.0f,
    .maxValue = 200.0f,
    .clampEnabled = true
};
AttributeId manaId = sys.gas->registerAttribute(manaDef);

// Initialize on entity
sys.gas->initializeAttribute(player, manaId, 100.0f);

// Read values
float current = sys.gas->getAttributeValue(player, manaId);
float base = sys.gas->getAttributeBaseValue(player, manaId);

// Modify
sys.gas->modifyAttribute(player, manaId, -30.0f);  // Spend mana
sys.gas->setAttributeBaseValue(player, manaId, 150.0f);  // Level up
```

### 3. Gameplay Effects

**Modifiers** that change attributes temporarily or permanently.

**Types:**
- **Instant** - Apply once and remove (damage, healing)
- **Duration** - Last for a set time (buffs, debuffs)
- **Infinite** - Persist until removed (auras, equipment)

**Features:**
- Periodic ticking (DoT/HoT)
- Stacking
- Tag requirements
- Granted tags

```cpp
// Instant heal
EffectDef healDef{
    .name = "SmallHeal",
    .durationType = EffectDurationType::Instant,
    .modifiers = {
        {.attribute = healthId, .op = EffectModifierOp::Add, .value = 50.0f}
    }
};
EffectId healId = sys.gas->registerEffect(healDef);

// Duration buff
EffectDef hasteDef{
    .name = "Haste",
    .durationType = EffectDurationType::Duration,
    .duration = 10.0f,
    .modifiers = {
        {.attribute = speedId, .op = EffectModifierOp::Multiply, .value = 1.5f}
    }
};
EffectId hasteId = sys.gas->registerEffect(hasteDef);

// Damage over time
EffectDef poisonDef{
    .name = "Poison",
    .durationType = EffectDurationType::Duration,
    .duration = 5.0f,
    .period = 1.0f,  // Tick every second
    .modifiers = {
        {.attribute = healthId, .op = EffectModifierOp::Add, .value = -10.0f}
    }
};
EffectId poisonId = sys.gas->registerEffect(poisonDef);

// Apply effects
sys.gas->applyEffect(target, healId, source);
sys.gas->applyEffect(player, hasteId, player);
```

**Modifier Operations:**
- `Add`: `currentValue += value`
- `Multiply`: `currentValue *= value`
- `Override`: `currentValue = value`

### 4. Gameplay Abilities

**Activatable actions** with cooldowns, costs, and requirements.

**Features:**
- Cooldown management
- Activation costs (mana, stamina)
- Tag requirements (must be grounded, can't be stunned)
- Tag blocking (can't attack while blocking)
- Effect application on activate/end
- Ability cancellation

```cpp
// Simple ability with cooldown
AbilityDef dashDef{
    .name = "Dash",
    .activationPolicy = AbilityActivationPolicy::OnInputPressed,
    .cooldown = 3.0f
};
AbilityId dashId = sys.gas->registerAbility(dashDef);

// Ability with cost
AbilityDef fireballDef{
    .name = "Fireball",
    .activationPolicy = AbilityActivationPolicy::OnInputPressed,
    .cooldown = 1.0f,
    .costs = {
        {.attribute = manaId, .cost = 30.0f}
    }
};
AbilityId fireballId = sys.gas->registerAbility(fireballDef);

// Grant and activate
sys.gas->grantAbility(player, dashId);
if (sys.gas->canActivateAbility(player, dashId)) {
    bool activated = sys.gas->tryActivateAbility(player, dashId);
}

// Query state
bool isActive = sys.gas->isAbilityActive(player, dashId);
float cooldown = sys.gas->getAbilityCooldown(player, dashId);
```

**Activation Policies:**
- `OnInputPressed` - Activate when input pressed
- `OnInputReleased` - Activate when input released
- `WhileInputHeld` - Active while held (blocking, charging)
- `Passive` - Always active (auras)

## Lua Integration

Define abilities, effects, and attributes in Lua for hot-reloadable, designer-friendly content.

### Lua Format: Tags

```lua
Tags = {
    "State.Movement.Running",
    "State.Movement.Jumping",
    "State.Combat.Attacking",
    "State.Status.Stunned",
    "Ability.Attack",
    "Ability.Movement"
}
```

### Lua Format: Attributes

```lua
Attributes = {
    {
        name = "Health",
        baseValue = 100,
        minValue = 0,
        maxValue = 100,
        clampEnabled = true
    },
    {
        name = "MoveSpeed",
        baseValue = 400,
        minValue = 0,
        maxValue = 1000
    }
}
```

### Lua Format: Effects

```lua
Effects = {
    -- Instant heal
    {
        name = "SmallHeal",
        durationType = "instant",
        modifiers = {
            {attribute = "Health", op = "add", value = 25}
        }
    },

    -- Duration buff
    {
        name = "Haste",
        durationType = "duration",
        duration = 10.0,
        modifiers = {
            {attribute = "MoveSpeed", op = "multiply", value = 1.5}
        }
    },

    -- Periodic damage
    {
        name = "Poison",
        durationType = "duration",
        duration = 5.0,
        period = 1.0,
        modifiers = {
            {attribute = "Health", op = "add", value = -5}
        }
    },

    -- Stackable buff
    {
        name = "StrengthBuff",
        durationType = "infinite",
        stackable = true,
        maxStacks = 5,
        modifiers = {
            {attribute = "Damage", op = "add", value = 10}
        },
        grantedTags = {"State.Buffed"}
    },

    -- Effect with tag requirements
    {
        name = "CriticalDamage",
        durationType = "instant",
        modifiers = {
            {attribute = "Health", op = "add", value = -100}
        },
        applicationRequiredTags = {"State.Vulnerable"},
        applicationBlockedTags = {"State.Invincible"}
    }
}
```

### Lua Format: Abilities

```lua
Abilities = {
    -- Simple dash
    {
        name = "Dash",
        activationPolicy = "onInputPressed",
        cooldown = 3.0
    },

    -- Spell with cost
    {
        name = "Fireball",
        activationPolicy = "onInputPressed",
        cooldown = 1.0,
        costs = {
            {attribute = "Mana", cost = 30}
        }
    },

    -- Ground-only jump
    {
        name = "Jump",
        activationPolicy = "onInputPressed",
        activationRequiredTags = {"State.Grounded"},
        activationBlockedTags = {"State.Airborne"}
    },

    -- Block ability
    {
        name = "Block",
        activationPolicy = "whileInputHeld",
        blockAbilitiesWithTags = {"Ability.Attack"},
        abilityTags = {"Ability.Defense"}
    },

    -- Ability with effects
    {
        name = "RageMode",
        activationPolicy = "onInputPressed",
        cooldown = 30.0,
        costs = {
            {attribute = "Stamina", cost = 50}
        },
        effectsToApplyOnActivate = {"StrengthBuff", "Haste"},
        effectsToApplyOnEnd = {"Exhaustion"}
    }
}
```

### Loading Definitions

```cpp
// From Lua file (via AssetSystem)
AssetHandle gasConfig = sys.assets->registerAsset(
    AssetType::Data, "config/abilities.lua"
);
sys.assets->loadAsset(gasConfig);
auto* data = sys.assets->getAsset<DataAsset>(gasConfig);
if (data) {
    sys.gas->loadDefinitionsFromLua(data->content);
}

// Or directly from string
std::string luaSource = R"(
    Tags = {"State.Grounded"}
    Attributes = {
        {name = "Health", baseValue = 100, minValue = 0, maxValue = 100}
    }
)";
sys.gas->loadDefinitionsFromLua(luaSource);
```

## Complete Examples

### Example 1: Character Setup

```cpp
// Initialize GAS component
Entity player = sys.entities->createEntity();
sys.gas->initializeComponent(player);

// Register attributes
AttributeDef healthDef{.name = "Health", .baseValue = 100.0f, .minValue = 0.0f, .maxValue = 100.0f};
AttributeDef manaDef{.name = "Mana", .baseValue = 50.0f, .minValue = 0.0f, .maxValue = 100.0f};
AttributeDef speedDef{.name = "MoveSpeed", .baseValue = 400.0f, .minValue = 0.0f, .maxValue = 1000.0f};

AttributeId healthId = sys.gas->registerAttribute(healthDef);
AttributeId manaId = sys.gas->registerAttribute(manaDef);
AttributeId speedId = sys.gas->registerAttribute(speedDef);

// Initialize attributes on player
sys.gas->initializeAttribute(player, healthId, 100.0f);
sys.gas->initializeAttribute(player, manaId, 50.0f);
sys.gas->initializeAttribute(player, speedId, 400.0f);

// Register tags
GameplayTag groundedTag = sys.gas->registerTag("State.Grounded");
GameplayTag stunnedTag = sys.gas->registerTag("State.Stunned");

// Add initial tags
sys.gas->addTag(player, groundedTag);
```

### Example 2: Buff/Debuff System

```cpp
// Register effect definitions
EffectDef speedBoost{
    .name = "SpeedBoost",
    .durationType = EffectDurationType::Duration,
    .duration = 5.0f,
    .modifiers = {
        {.attribute = speedId, .op = EffectModifierOp::Multiply, .value = 1.5f}
    }
};
EffectId speedBoostId = sys.gas->registerEffect(speedBoost);

EffectDef stun{
    .name = "Stun",
    .durationType = EffectDurationType::Duration,
    .duration = 2.0f,
    .modifiers = {
        {.attribute = speedId, .op = EffectModifierOp::Override, .value = 0.0f}
    }
};
GameplayTag stunnedTag = sys.gas->registerTag("State.Stunned");
stun.grantedTags.addTag(stunnedTag);
EffectId stunId = sys.gas->registerEffect(stun);

// Apply effects
sys.gas->applyEffect(player, speedBoostId, player);  // Speed boost
sys.gas->applyEffect(enemy, stunId, player);  // Stun enemy

// Check active effects
if (sys.gas->hasEffect(enemy, stunId)) {
    // Enemy is stunned
}

// Remove effect early
sys.gas->removeEffect(player, speedBoostId);
```

### Example 3: Ability with Cooldown

```cpp
// Define ability
AbilityDef dashDef{
    .name = "Dash",
    .activationPolicy = AbilityActivationPolicy::OnInputPressed,
    .cooldown = 3.0f,
    .costs = {
        {.attribute = staminaId, .cost = 20.0f}
    }
};

// Create speed boost effect applied on dash
EffectDef dashSpeed{
    .name = "DashSpeed",
    .durationType = EffectDurationType::Duration,
    .duration = 0.5f,
    .modifiers = {
        {.attribute = speedId, .op = EffectModifierOp::Multiply, .value = 3.0f}
    }
};
EffectId dashSpeedId = sys.gas->registerEffect(dashSpeed);

dashDef.effectsToApplyOnActivate = {dashSpeedId};
AbilityId dashId = sys.gas->registerAbility(dashDef);

// Grant to player
sys.gas->grantAbility(player, dashId);

// In input handler
if (sys.input->isKeyJustPressed(Key::Shift)) {
    if (sys.gas->canActivateAbility(player, dashId)) {
        bool activated = sys.gas->tryActivateAbility(player, dashId);
        if (activated) {
            // Play dash animation, particles, etc.
        }
    } else {
        float cooldown = sys.gas->getAbilityCooldown(player, dashId);
        // Show cooldown remaining on UI
    }
}
```

### Example 4: Lua-Based RPG System

```lua
-- data/config/rpg_system.lua

Tags = {
    "State.Grounded",
    "State.Combat.Attacking",
    "State.Status.Stunned",
    "State.Status.Invincible",
    "Ability.Attack",
    "Ability.Movement"
}

Attributes = {
    {name = "Health", baseValue = 100, minValue = 0, maxValue = 100},
    {name = "Mana", baseValue = 100, minValue = 0, maxValue = 100},
    {name = "Stamina", baseValue = 100, minValue = 0, maxValue = 100},
    {name = "MoveSpeed", baseValue = 400, minValue = 0, maxValue = 1000},
    {name = "AttackPower", baseValue = 10, minValue = 0, maxValue = 100}
}

Effects = {
    -- Damage effect
    {
        name = "MeleeDamage",
        durationType = "instant",
        modifiers = {
            {attribute = "Health", op = "add", value = -15}
        }
    },

    -- Health regeneration
    {
        name = "HealthRegen",
        durationType = "duration",
        duration = 10.0,
        period = 1.0,
        modifiers = {
            {attribute = "Health", op = "add", value = 5}
        }
    },

    -- Stun debuff
    {
        name = "Stun",
        durationType = "duration",
        duration = 2.0,
        modifiers = {
            {attribute = "MoveSpeed", op = "override", value = 0}
        },
        grantedTags = {"State.Status.Stunned"}
    }
}

Abilities = {
    -- Basic attack
    {
        name = "MeleeAttack",
        activationPolicy = "onInputPressed",
        cooldown = 0.5,
        costs = {
            {attribute = "Stamina", cost = 10}
        },
        abilityTags = {"Ability.Attack"},
        activationBlockedTags = {"State.Status.Stunned"},
        effectsToApplyOnActivate = {"MeleeDamage"}
    },

    -- Dash
    {
        name = "Dash",
        activationPolicy = "onInputPressed",
        cooldown = 3.0,
        costs = {
            {attribute = "Stamina", cost = 20}
        },
        abilityTags = {"Ability.Movement"}
    },

    -- Jump
    {
        name = "Jump",
        activationPolicy = "onInputPressed",
        activationRequiredTags = {"State.Grounded"},
        abilityTags = {"Ability.Movement"}
    },

    -- Healing spell
    {
        name = "Heal",
        activationPolicy = "onInputPressed",
        cooldown = 10.0,
        costs = {
            {attribute = "Mana", cost = 30}
        },
        effectsToApplyOnActivate = {"HealthRegen"}
    }
}
```

```cpp
// Load the system
AssetHandle rpgConfig = sys.assets->registerAsset(AssetType::Data, "config/rpg_system.lua");
sys.assets->loadAsset(rpgConfig);
auto* configData = sys.assets->getAsset<DataAsset>(rpgConfig);
sys.gas->loadDefinitionsFromLua(configData->content);

// Setup player
Entity player = sys.entities->createEntity();
sys.gas->initializeComponent(player);

// Find registered IDs
auto healthDef = sys.gas->getAttributeDef("Health");
auto manaDef = sys.gas->getAttributeDef("Mana");
auto dashAbilityDef = sys.gas->getAbilityDef("Dash");

// Initialize and grant
if (healthDef && manaDef && dashAbilityDef) {
    sys.gas->initializeAttribute(player, healthDef->id, 100.0f);
    sys.gas->initializeAttribute(player, manaDef->id, 100.0f);
    sys.gas->grantAbility(player, dashAbilityDef->id);
}
```

## Callbacks and Events

### Attribute Changes

```cpp
sys.gas->setAttributeChangeCallback([](const AttributeChangeEvent& event) {
    // event.entity - Entity whose attribute changed
    // event.attribute - Attribute ID
    // event.oldValue - Previous value
    // event.newValue - New value

    if (event.newValue <= 0.0f) {
        // Handle death
    }
});
```

### Effect Applications

```cpp
sys.gas->setEffectAppliedCallback([](const EffectAppliedEvent& event) {
    // event.target - Entity receiving effect
    // event.source - Entity applying effect
    // event.effect - Effect ID

    // Play visual effects, sounds, etc.
});
```

### Ability Activations

```cpp
sys.gas->setAbilityActivatedCallback([](const AbilityActivatedEvent& event) {
    // event.entity - Entity activating ability
    // event.ability - Ability ID

    // Play animations, particles, sounds
});
```

## Advanced Patterns

### Combo System

```cpp
// Define combo tags
GameplayTag combo1 = sys.gas->registerTag("Combo.Stage1");
GameplayTag combo2 = sys.gas->registerTag("Combo.Stage2");

// First attack grants combo1 tag
EffectDef grantCombo1{
    .name = "GrantCombo1",
    .durationType = EffectDurationType::Duration,
    .duration = 2.0f  // Combo window
};
grantCombo1.grantedTags.addTag(combo1);
EffectId grantCombo1Id = sys.gas->registerEffect(grantCombo1);

// First attack
AbilityDef attack1{
    .name = "Attack1",
    .effectsToApplyOnActivate = {grantCombo1Id}
};

// Second attack requires combo1
AbilityDef attack2{
    .name = "Attack2"
};
attack2.activationRequiredTags.addTag(combo1);
```

### Cleanse Mechanic

```cpp
// All debuffs grant "Status.Debuff" tag
GameplayTag debuffTag = sys.gas->registerTag("Status.Debuff");

// Debuff example
EffectDef poisonDef{
    .name = "Poison",
    .durationType = EffectDurationType::Duration,
    .duration = 5.0f,
    .period = 1.0f,
    .modifiers = {{.attribute = healthId, .op = EffectModifierOp::Add, .value = -5.0f}}
};
poisonDef.grantedTags.addTag(debuffTag);

// Cleanse removes effects with debuff tag
GameplayTag cleanseTag = sys.gas->registerTag("Action.Cleanse");
poisonDef.removalTags.addTag(cleanseTag);

// When player cleanses
sys.gas->addTag(player, cleanseTag);
sys.gas->update(dt);  // Effect removed during update
```

### Conditional Ability Availability

```cpp
bool canUseDesperationMove(Entity player) {
    auto healthOpt = sys.gas->getAttributeDef("Health");
    if (!healthOpt) return false;

    float health = sys.gas->getAttributeValue(player, healthOpt->id);
    float maxHealth = sys.gas->getAttributeBaseValue(player, healthOpt->id);

    // Only available below 30% health
    return (health / maxHealth) < 0.3f &&
           sys.gas->canActivateAbility(player, desperationMoveId);
}
```

## Integration with Other Systems

### Physics Integration

```cpp
// Apply knockback based on effect
sys.gas->setEffectAppliedCallback([&](const EffectAppliedEvent& event) {
    auto effectDef = sys.gas->getEffectDef(event.effect);
    if (effectDef && effectDef->name == "Knockback") {
        Vec2 direction = getDirection(event.source, event.target);
        sys.physics->applyImpulse(event.target, direction * 500.0f);
    }
});
```

### Animation Integration

```cpp
sys.gas->setAbilityActivatedCallback([&](const AbilityActivatedEvent& event) {
    auto abilityDef = sys.gas->getAbilityDef(event.ability);
    if (abilityDef) {
        if (abilityDef->name == "MeleeAttack") {
            // Play attack animation
        } else if (abilityDef->name == "Dash") {
            // Play dash animation
        }
    }
});
```

### UI Integration

```cpp
void updateUI() {
    // Health bar
    float health = sys.gas->getAttributeValue(player, healthId);
    float maxHealth = sys.gas->getAttributeBaseValue(player, healthId);
    ui->setHealthBar(health / maxHealth);

    // Ability cooldowns
    float dashCooldown = sys.gas->getAbilityCooldown(player, dashId);
    ui->setAbilityCooldown("Dash", dashCooldown);

    // Active effects
    auto effects = sys.gas->getActiveEffects(player);
    for (const auto& effect : effects) {
        auto def = sys.gas->getEffectDef(effect.defId);
        if (def) {
            ui->addStatusIcon(def->name, effect.remainingDuration);
        }
    }
}
```

## Best Practices

### Do's

1. **Initialize components** - Always call `initializeComponent()` before using GAS features
2. **Use Lua for content** - Define abilities, effects, and attributes in Lua files
3. **Use events for integration** - Callbacks for animations, VFX, sound
4. **Use tag hierarchies** - Organize with dot notation (`"State.Combat.Attacking"`)
5. **Keep attributes focused** - Single-purpose stats (`"Health"`, `"MoveSpeed"`)
6. **Compose effects** - Combine simple effects for complex behaviors
7. **Call update()** - Required for duration effects, cooldowns, and tag removal

### Don'ts

1. **Don't skip component initialization** - GAS won't work without it
2. **Don't create duplicate definitions** - Reuse registered tags, attributes, effects
3. **Don't forget to grant abilities** - Registration doesn't grant them to entities
4. **Don't hardcode IDs** - Use `getAttributeDef()`, `getEffectDef()`, etc. for lookups
5. **Don't forget clamping** - Set appropriate min/max for attributes
6. **Don't overuse periodic effects** - They have performance cost

## Troubleshooting

### Effect Not Applying

**Check tag requirements:**
```cpp
auto effectDef = sys.gas->getEffectDef(effectId);
const GameplayTagContainer* tags = sys.gas->getTags(target);
if (effectDef && tags) {
    bool meetsRequirements = tags->matchesQuery(
        effectDef->applicationRequiredTags,
        effectDef->applicationBlockedTags
    );
}
```

### Ability Can't Activate

**Debug checklist:**
```cpp
if (!sys.gas->canActivateAbility(player, abilityId)) {
    // 1. Is ability granted?
    if (!sys.gas->hasAbility(player, abilityId)) {
        // Need to grant first
    }

    // 2. On cooldown?
    float cd = sys.gas->getAbilityCooldown(player, abilityId);
    if (cd > 0.0f) {
        // Wait for cooldown
    }

    // 3. Tag requirements met?
    // 4. Enough resources for costs?
}
```

### Attribute Not Changing

**Verify effect is active:**
```cpp
if (!sys.gas->hasEffect(entity, effectId)) {
    // Effect may have expired or not been applied
}

// Check modifiers
auto effectDef = sys.gas->getEffectDef(effectId);
if (effectDef) {
    for (const auto& mod : effectDef->modifiers) {
        // Verify correct attribute ID
    }
}
```

## API Reference

### Component Management

```cpp
void initializeComponent(Entity entity);
void removeComponent(Entity entity);
bool hasComponent(Entity entity) const;
AbilitySystemComponent* getComponent(Entity entity);
```

### Tag Operations

```cpp
GameplayTag registerTag(const std::string& name);
std::optional<GameplayTag> findTag(const std::string& name) const;
bool isParentOf(const GameplayTag& parent, const GameplayTag& child) const;

void addTag(Entity entity, const GameplayTag& tag);
void removeTag(Entity entity, const GameplayTag& tag);
bool hasTag(Entity entity, const GameplayTag& tag) const;
const GameplayTagContainer* getTags(Entity entity) const;
```

### Attribute Operations

```cpp
AttributeId registerAttribute(const AttributeDef& def);
std::optional<AttributeDef> getAttributeDef(AttributeId id) const;
std::optional<AttributeDef> getAttributeDef(const std::string& name) const;

void initializeAttribute(Entity entity, AttributeId id, float baseValue);
float getAttributeValue(Entity entity, AttributeId id) const;
float getAttributeBaseValue(Entity entity, AttributeId id) const;
void setAttributeBaseValue(Entity entity, AttributeId id, float value);
void modifyAttribute(Entity entity, AttributeId id, float delta);
```

### Effect Operations

```cpp
EffectId registerEffect(const EffectDef& def);
std::optional<EffectDef> getEffectDef(EffectId id) const;
std::optional<EffectDef> getEffectDef(const std::string& name) const;

void applyEffect(Entity target, EffectId effectId, Entity source);
void removeEffect(Entity entity, EffectId effectId);
void removeAllEffects(Entity entity);
bool hasEffect(Entity entity, EffectId effectId) const;
std::vector<ActiveEffect> getActiveEffects(Entity entity) const;
```

### Ability Operations

```cpp
AbilityId registerAbility(const AbilityDef& def);
std::optional<AbilityDef> getAbilityDef(AbilityId id) const;
std::optional<AbilityDef> getAbilityDef(const std::string& name) const;

void grantAbility(Entity entity, AbilityId abilityId);
void removeAbility(Entity entity, AbilityId abilityId);
bool hasAbility(Entity entity, AbilityId abilityId) const;
bool canActivateAbility(Entity entity, AbilityId abilityId) const;
bool tryActivateAbility(Entity entity, AbilityId abilityId);
void endAbility(Entity entity, AbilityId abilityId);
bool isAbilityActive(Entity entity, AbilityId abilityId) const;
float getAbilityCooldown(Entity entity, AbilityId abilityId) const;
```

### Callbacks

```cpp
void setAttributeChangeCallback(AttributeChangeCallback callback);
void setEffectAppliedCallback(EffectAppliedCallback callback);
void setAbilityActivatedCallback(AbilityActivatedCallback callback);
```

### Lua Integration

```cpp
bool loadDefinitionsFromLua(const std::string& luaSource);
```

## Summary

The GAS system provides:
- **Tags** - Flexible categorization and state tracking
- **Attributes** - Dynamic stats with automatic modifier application
- **Effects** - Instant, duration, and periodic attribute modifications
- **Abilities** - Cooldown-based actions with costs and requirements
- **Lua Integration** - Hot-reloadable, designer-friendly content
- **Callbacks** - Integration with animation, audio, UI, physics

For more examples, see the test suite at `/Users/jaaaacob/Documents/GameDev/jframe/tests/unit/GASSystemTests.cpp`.

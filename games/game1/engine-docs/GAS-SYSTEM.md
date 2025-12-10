# Bestow Gameplay Ability System (GAS) Guide

## Overview

The Bestow Gameplay Ability System (GAS) is an Unreal Engine-inspired framework for implementing abilities, effects, and attributes in a data-driven, flexible way. It provides a robust foundation for RPG mechanics, character progression, status effects, and complex ability interactions.

**Key Features:**
- **Hierarchical Tag System** - Categorize and filter abilities, effects, and states
- **Attribute System** - Dynamic stats with base/current values and modifiers
- **Gameplay Effects** - Buffs, debuffs, damage-over-time, and instant modifications
- **Gameplay Abilities** - Cooldown-based actions with costs and requirements
- **Lua Integration** - Define abilities, effects, and attributes in Lua for hot-reloadable content
- **Event-Driven** - Callbacks for attribute changes, effect applications, and ability activations

## Core Concepts

### 1. Gameplay Tags

Tags are hierarchical identifiers used for categorization, filtering, and requirements throughout the GAS system.

**Hierarchy Example:**
```
State.Movement.Running
State.Movement.Jumping
State.Combat.Attacking
State.Combat.Stunned
```

Tags use dot-notation for hierarchy. `State.Movement` is a parent of `State.Movement.Running`.

**Common Use Cases:**
- State tracking (grounded, airborne, stunned, invincible)
- Ability categorization (attack abilities, movement abilities)
- Effect filtering (cleanse effects remove effects with certain tags)
- Conditional logic (can only jump when grounded)

### 2. Attributes

Attributes represent numeric stats like health, mana, speed, strength, etc. Each attribute has:
- **Base Value** - The fundamental value without modifiers
- **Current Value** - The computed value after all effects are applied
- **Min/Max Constraints** - Clamping boundaries
- **Change Callbacks** - Notifications when values change

### 3. Gameplay Effects

Effects modify attributes temporarily or permanently. They can:
- Apply instant changes (damage, healing)
- Apply duration-based modifiers (buffs that last 10 seconds)
- Apply infinite modifiers (passive auras)
- Tick periodically (damage-over-time, regeneration)
- Stack multiple times
- Grant temporary tags
- Require or block certain tags

### 4. Gameplay Abilities

Abilities are activatable actions with:
- Cooldowns
- Activation costs (mana, stamina, etc.)
- Tag-based requirements (must be grounded, can't be stunned)
- Tag-based blocking (can't attack while blocking)
- Effects applied on activation/end
- Ability cancellation logic

## Setting Up GAS

### Initialize the GAS System

```cpp
import bestow;

// In your game initialization
auto* gas = engine.systems().gas();
gas->initialize();
```

### Attach GAS Component to Entities

```cpp
Entity player = entities->createEntity();

// Initialize the Ability System Component
gas->initializeComponent(player);

// The entity can now use tags, attributes, effects, and abilities
```

## Working with Tags

### Register Tags (C++)

```cpp
GameplayTag groundedTag = gas->registerTag("State.Grounded");
GameplayTag stunnedTag = gas->registerTag("State.Stunned");
GameplayTag attackTag = gas->registerTag("Ability.Attack");
```

### Add/Remove Tags

```cpp
// Add tag to entity
gas->addTag(player, groundedTag);

// Check if entity has tag
if (gas->hasTag(player, stunnedTag)) {
    // Player is stunned
}

// Remove tag
gas->removeTag(player, groundedTag);
```

### Tag Queries

```cpp
GameplayTagContainer required;
required.addTag(groundedTag);

GameplayTagContainer blocked;
blocked.addTag(stunnedTag);

const GameplayTagContainer* playerTags = gas->getTags(player);

// Check if player has required tags and no blocked tags
if (playerTags->matchesQuery(required, blocked)) {
    // Can perform grounded action while not stunned
}
```

### Tag Hierarchy

```cpp
GameplayTag movement = gas->registerTag("State.Movement");
GameplayTag running = gas->registerTag("State.Movement.Running");

// Check parent relationship
if (gas->isParentOf(movement, running)) {
    // "State.Movement" is parent of "State.Movement.Running"
}
```

## Working with Attributes

### Define Attributes (C++)

```cpp
AttributeDef healthDef {
    .name = "Health",
    .baseValue = 100.0f,
    .minValue = 0.0f,
    .maxValue = 100.0f,
    .clampEnabled = true
};

AttributeId healthId = gas->registerAttribute(healthDef);
```

### Initialize Attributes on Entities

```cpp
// Set starting health to 100
gas->initializeAttribute(player, healthId, 100.0f);

// Get current value
float currentHealth = gas->getAttributeValue(player, healthId);

// Get base value (without modifiers)
float baseHealth = gas->getAttributeBaseValue(player, healthId);
```

### Modify Attributes

```cpp
// Reduce health by 25
gas->modifyAttribute(player, healthId, -25.0f);

// Heal for 50 (will be clamped to max)
gas->modifyAttribute(player, healthId, 50.0f);

// Set new base value (recalculates effects)
gas->setAttributeBaseValue(player, healthId, 150.0f);
```

### Attribute Change Callbacks

```cpp
gas->setAttributeChangeCallback([](const AttributeChangeEvent& event) {
    // event.entity - The entity whose attribute changed
    // event.attribute - The attribute ID that changed
    // event.oldValue - Value before change
    // event.newValue - Value after change

    if (event.newValue <= 0.0f) {
        // Entity died
    }
});
```

## Working with Effects

### Define Effects (C++)

```cpp
// Instant heal effect
EffectDef healDef {
    .name = "SmallHeal",
    .durationType = EffectDurationType::Instant,
    .modifiers = {
        { .attribute = healthId, .op = EffectModifierOp::Add, .value = 50.0f }
    }
};
EffectId healId = gas->registerEffect(healDef);

// Duration-based speed buff
EffectDef hasteDef {
    .name = "Haste",
    .durationType = EffectDurationType::Duration,
    .duration = 10.0f,  // Lasts 10 seconds
    .modifiers = {
        { .attribute = speedId, .op = EffectModifierOp::Multiply, .value = 1.5f }
    }
};
EffectId hasteId = gas->registerEffect(hasteDef);

// Periodic damage-over-time
EffectDef poisonDef {
    .name = "Poison",
    .durationType = EffectDurationType::Duration,
    .duration = 5.0f,   // Lasts 5 seconds
    .period = 1.0f,     // Ticks every 1 second
    .modifiers = {
        { .attribute = healthId, .op = EffectModifierOp::Add, .value = -10.0f }
    }
};
EffectId poisonId = gas->registerEffect(poisonDef);

// Infinite aura with granted tags
GameplayTag buffedTag = gas->registerTag("State.Buffed");
EffectDef auraDef {
    .name = "StrengthAura",
    .durationType = EffectDurationType::Infinite,
    .modifiers = {
        { .attribute = strengthId, .op = EffectModifierOp::Add, .value = 20.0f }
    },
    .grantedTags = { buffedTag }
};
auraDef.grantedTags.addTag(buffedTag);
EffectId auraId = gas->registerEffect(auraDef);
```

### Effect Modifier Operations

```cpp
// Add: currentValue += value
{ .attribute = healthId, .op = EffectModifierOp::Add, .value = 50.0f }

// Multiply: currentValue *= value
{ .attribute = damageId, .op = EffectModifierOp::Multiply, .value = 2.0f }

// Override: currentValue = value
{ .attribute = speedId, .op = EffectModifierOp::Override, .value = 0.0f }  // Stun
```

### Apply Effects

```cpp
Entity source = player;
Entity target = enemy;

// Apply instant damage
gas->applyEffect(target, damageEffectId, source);

// Apply duration buff
gas->applyEffect(player, hasteId, player);

// Check if entity has effect
if (gas->hasEffect(player, hasteId)) {
    // Player is currently hasted
}

// Remove specific effect
gas->removeEffect(player, hasteId);

// Remove all effects
gas->removeAllEffects(player);

// Get all active effects
std::vector<ActiveEffect> effects = gas->getActiveEffects(player);
```

### Effect Stacking

```cpp
EffectDef stackingBuff {
    .name = "DamageBoost",
    .durationType = EffectDurationType::Infinite,
    .modifiers = {
        { .attribute = damageId, .op = EffectModifierOp::Add, .value = 10.0f }
    },
    .stackable = true,
    .maxStacks = 5
};
EffectId boostId = gas->registerEffect(stackingBuff);

// Apply 3 times - each application adds a stack
gas->applyEffect(player, boostId, player);  // 1 stack: +10 damage
gas->applyEffect(player, boostId, player);  // 2 stacks: +20 damage
gas->applyEffect(player, boostId, player);  // 3 stacks: +30 damage
```

### Effect Tag Requirements

```cpp
GameplayTag vulnerableTag = gas->registerTag("State.Vulnerable");
GameplayTag immuneTag = gas->registerTag("State.Immune");

EffectDef criticalDamage {
    .name = "CriticalHit",
    .durationType = EffectDurationType::Instant,
    .modifiers = {
        { .attribute = healthId, .op = EffectModifierOp::Add, .value = -100.0f }
    },
    .applicationRequiredTags = { vulnerableTag },  // Only works on vulnerable targets
    .applicationBlockedTags = { immuneTag }        // Doesn't work on immune targets
};
criticalDamage.applicationRequiredTags.addTag(vulnerableTag);
criticalDamage.applicationBlockedTags.addTag(immuneTag);
```

### Effect Removal by Tag

```cpp
GameplayTag cleanseTag = gas->registerTag("Action.Cleanse");

EffectDef debuff {
    .name = "Slow",
    .durationType = EffectDurationType::Infinite,
    .modifiers = {
        { .attribute = speedId, .op = EffectModifierOp::Multiply, .value = 0.5f }
    },
    .removalTags = { cleanseTag }  // Removed if entity gains cleanse tag
};
debuff.removalTags.addTag(cleanseTag);

// Later, cleanse removes all effects with removalTags containing "Action.Cleanse"
gas->addTag(player, cleanseTag);
gas->update(deltaTime);  // Effect will be removed during update
```

### Effect Application Callbacks

```cpp
gas->setEffectAppliedCallback([](const EffectAppliedEvent& event) {
    // event.target - Entity that received the effect
    // event.source - Entity that applied the effect
    // event.effect - The effect ID that was applied

    // Play visual effect, sound, etc.
});
```

## Working with Abilities

### Define Abilities (C++)

```cpp
// Simple dash ability with cooldown
AbilityDef dashDef {
    .name = "Dash",
    .activationPolicy = AbilityActivationPolicy::OnInputPressed,
    .cooldown = 3.0f
};
AbilityId dashId = gas->registerAbility(dashDef);

// Spell with mana cost
AbilityDef fireballDef {
    .name = "Fireball",
    .activationPolicy = AbilityActivationPolicy::OnInputPressed,
    .cooldown = 1.0f,
    .costs = {
        { .attribute = manaId, .cost = 30.0f }
    }
};
AbilityId fireballId = gas->registerAbility(fireballDef);

// Ground-only jump ability
GameplayTag groundedTag = gas->registerTag("State.Grounded");
AbilityDef jumpDef {
    .name = "Jump",
    .activationPolicy = AbilityActivationPolicy::OnInputPressed,
    .activationRequiredTags = { groundedTag }
};
jumpDef.activationRequiredTags.addTag(groundedTag);
AbilityId jumpId = gas->registerAbility(jumpDef);

// Block ability that prevents attacks
GameplayTag attackAbilityTag = gas->registerTag("Ability.Attack");
AbilityDef blockDef {
    .name = "Block",
    .activationPolicy = AbilityActivationPolicy::WhileInputHeld,
    .blockAbilitiesWithTags = { attackAbilityTag }
};
blockDef.blockAbilitiesWithTags.addTag(attackAbilityTag);
AbilityId blockId = gas->registerAbility(blockDef);
```

### Activation Policies

```cpp
// Activate once when input is pressed
.activationPolicy = AbilityActivationPolicy::OnInputPressed

// Activate once when input is released
.activationPolicy = AbilityActivationPolicy::OnInputReleased

// Active while input is held (like blocking or charging)
.activationPolicy = AbilityActivationPolicy::WhileInputHeld

// Always active (passive abilities, auras)
.activationPolicy = AbilityActivationPolicy::Passive
```

### Grant and Use Abilities

```cpp
// Grant ability to entity
gas->grantAbility(player, dashId);
gas->grantAbility(player, fireballId);

// Check if entity has ability
if (gas->hasAbility(player, dashId)) {
    // Player has dash
}

// Check if ability can be activated
if (gas->canActivateAbility(player, dashId)) {
    // Not on cooldown, meets requirements
}

// Try to activate (returns true if successful)
if (gas->tryActivateAbility(player, dashId)) {
    // Dash activated successfully
}

// Check if ability is active
if (gas->isAbilityActive(player, blockId)) {
    // Player is blocking
}

// End ability manually
gas->endAbility(player, blockId);

// Get remaining cooldown
float cooldown = gas->getAbilityCooldown(player, dashId);
if (cooldown > 0.0f) {
    // Still on cooldown
}

// Remove ability from entity
gas->removeAbility(player, dashId);
```

### Abilities with Effects

```cpp
// Ability that applies speed boost when activated
EffectDef speedBoost {
    .name = "DashSpeedBoost",
    .durationType = EffectDurationType::Duration,
    .duration = 0.5f,
    .modifiers = {
        { .attribute = speedId, .op = EffectModifierOp::Multiply, .value = 3.0f }
    }
};
EffectId speedBoostId = gas->registerEffect(speedBoost);

AbilityDef dashDef {
    .name = "Dash",
    .cooldown = 3.0f,
    .effectsToApplyOnActivate = { speedBoostId }
};
AbilityId dashId = gas->registerAbility(dashDef);

// When dash activates, speed boost effect is automatically applied
gas->tryActivateAbility(player, dashId);
```

### Abilities with End Effects

```cpp
// Sprint drains stamina when it ends
EffectDef staminaDrain {
    .name = "SprintExhaustion",
    .durationType = EffectDurationType::Instant,
    .modifiers = {
        { .attribute = staminaId, .op = EffectModifierOp::Add, .value = -20.0f }
    }
};
EffectId drainId = gas->registerEffect(staminaDrain);

AbilityDef sprintDef {
    .name = "Sprint",
    .activationPolicy = AbilityActivationPolicy::WhileInputHeld,
    .effectsToApplyOnEnd = { drainId }
};
AbilityId sprintId = gas->registerAbility(sprintDef);
```

### Ability Cancellation

```cpp
GameplayTag movementTag = gas->registerTag("Ability.Movement");

AbilityDef dashDef {
    .name = "Dash",
    .abilityTags = { movementTag }
};
dashDef.abilityTags.addTag(movementTag);

AbilityDef sprintDef {
    .name = "Sprint",
    .abilityTags = { movementTag },
    .cancelAbilitiesWithTags = { movementTag }  // Cancels dash
};
sprintDef.abilityTags.addTag(movementTag);
sprintDef.cancelAbilitiesWithTags.addTag(movementTag);

// If dash is active, activating sprint will cancel dash
gas->tryActivateAbility(player, dashId);      // Dash active
gas->tryActivateAbility(player, sprintId);    // Dash cancelled, sprint active
```

### Ability Activation Callbacks

```cpp
gas->setAbilityActivatedCallback([](const AbilityActivatedEvent& event) {
    // event.entity - Entity that activated the ability
    // event.ability - The ability ID that was activated

    // Play animation, sound effect, spawn particles, etc.
});
```

## Lua Integration

### Loading Definitions from Lua

```cpp
// Load from Lua script file
std::string luaContent = readFile("abilities.lua");
gas->loadDefinitionsFromLua(luaContent);
```

### Lua Format: Tags

```lua
Tags = {
    "State.Movement.Running",
    "State.Movement.Jumping",
    "State.Movement.Dashing",
    "State.Combat.Attacking",
    "State.Combat.Blocking",
    "State.Status.Stunned",
    "State.Status.Invincible",
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
        name = "Mana",
        baseValue = 50,
        minValue = 0,
        maxValue = 200
    },
    {
        name = "MoveSpeed",
        baseValue = 400,
        minValue = 0,
        maxValue = 1000
    },
    {
        name = "Stamina",
        baseValue = 100,
        minValue = 0,
        maxValue = 100
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
            { attribute = "Health", op = "add", value = 25 }
        }
    },

    -- Duration buff
    {
        name = "Haste",
        durationType = "duration",
        duration = 10.0,
        modifiers = {
            { attribute = "MoveSpeed", op = "multiply", value = 1.5 }
        }
    },

    -- Damage over time (periodic)
    {
        name = "Poison",
        durationType = "duration",
        duration = 5.0,
        period = 1.0,  -- Ticks every second
        modifiers = {
            { attribute = "Health", op = "add", value = -5 }
        }
    },

    -- Stackable buff
    {
        name = "StrengthBuff",
        durationType = "infinite",
        stackable = true,
        maxStacks = 5,
        modifiers = {
            { attribute = "Damage", op = "add", value = 10 }
        },
        grantedTags = { "State.Status.Buffed" }
    },

    -- Effect with tag requirements
    {
        name = "CriticalDamage",
        durationType = "instant",
        modifiers = {
            { attribute = "Health", op = "add", value = -100 }
        },
        applicationRequiredTags = { "State.Status.Vulnerable" },
        applicationBlockedTags = { "State.Status.Invincible" }
    },

    -- Effect that can be cleansed
    {
        name = "Slow",
        durationType = "infinite",
        modifiers = {
            { attribute = "MoveSpeed", op = "multiply", value = 0.5 }
        },
        grantedTags = { "State.Status.Slowed" },
        removalTags = { "Action.Cleanse" }
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

    -- Spell with mana cost
    {
        name = "Fireball",
        activationPolicy = "onInputPressed",
        cooldown = 1.0,
        costs = {
            { attribute = "Mana", cost = 30 }
        }
    },

    -- Ground-only jump
    {
        name = "Jump",
        activationPolicy = "onInputPressed",
        activationRequiredTags = { "State.Grounded" },
        activationBlockedTags = { "State.Airborne" }
    },

    -- Ability with effects
    {
        name = "RageMode",
        activationPolicy = "onInputPressed",
        cooldown = 30.0,
        costs = {
            { attribute = "Stamina", cost = 50 }
        },
        abilityTags = { "Ability.Buff" },
        effectsToApplyOnActivate = { "StrengthBuff", "Haste" },
        effectsToApplyOnEnd = { "Exhaustion" }
    },

    -- Block that prevents attacks
    {
        name = "Block",
        activationPolicy = "whileInputHeld",
        blockAbilitiesWithTags = { "Ability.Attack" },
        abilityTags = { "Ability.Defense" }
    },

    -- Sprint that cancels other movement
    {
        name = "Sprint",
        activationPolicy = "whileInputHeld",
        abilityTags = { "Ability.Movement" },
        cancelAbilitiesWithTags = { "Ability.Movement" }
    }
}
```

### Complete Lua Example: RPG Character Abilities

```lua
-- Define all tags
Tags = {
    "State.Grounded",
    "State.Airborne",
    "State.Combat.Attacking",
    "State.Combat.Blocking",
    "State.Status.Stunned",
    "State.Status.Invincible",
    "Ability.Attack",
    "Ability.Movement",
    "Ability.Defense"
}

-- Define character stats
Attributes = {
    { name = "Health", baseValue = 100, minValue = 0, maxValue = 100 },
    { name = "Mana", baseValue = 100, minValue = 0, maxValue = 100 },
    { name = "Stamina", baseValue = 100, minValue = 0, maxValue = 100 },
    { name = "MoveSpeed", baseValue = 400, minValue = 0, maxValue = 1000 },
    { name = "AttackPower", baseValue = 10, minValue = 0, maxValue = 100 }
}

-- Define effects
Effects = {
    -- Combat effects
    {
        name = "MeleeAttackDamage",
        durationType = "instant",
        modifiers = {
            { attribute = "Health", op = "add", value = -15 }
        }
    },

    -- Movement buff
    {
        name = "DashSpeedBoost",
        durationType = "duration",
        duration = 0.5,
        modifiers = {
            { attribute = "MoveSpeed", op = "multiply", value = 3.0 }
        }
    },

    -- Mana regeneration
    {
        name = "ManaRegen",
        durationType = "duration",
        duration = 10.0,
        period = 1.0,
        modifiers = {
            { attribute = "Mana", op = "add", value = 5 }
        }
    },

    -- Block defense boost
    {
        name = "BlockDefense",
        durationType = "infinite",
        grantedTags = { "State.Combat.Blocking" }
    }
}

-- Define abilities
Abilities = {
    -- Basic melee attack
    {
        name = "MeleeAttack",
        activationPolicy = "onInputPressed",
        cooldown = 0.5,
        costs = {
            { attribute = "Stamina", cost = 10 }
        },
        abilityTags = { "Ability.Attack" },
        activationBlockedTags = { "State.Status.Stunned" },
        effectsToApplyOnActivate = { "MeleeAttackDamage" }
    },

    -- Dash ability
    {
        name = "Dash",
        activationPolicy = "onInputPressed",
        cooldown = 3.0,
        costs = {
            { attribute = "Stamina", cost = 20 }
        },
        abilityTags = { "Ability.Movement" },
        effectsToApplyOnActivate = { "DashSpeedBoost" }
    },

    -- Jump (ground only)
    {
        name = "Jump",
        activationPolicy = "onInputPressed",
        cooldown = 0.2,
        activationRequiredTags = { "State.Grounded" },
        abilityTags = { "Ability.Movement" }
    },

    -- Block (held)
    {
        name = "Block",
        activationPolicy = "whileInputHeld",
        blockAbilitiesWithTags = { "Ability.Attack" },
        abilityTags = { "Ability.Defense" },
        effectsToApplyOnActivate = { "BlockDefense" }
    },

    -- Mana potion
    {
        name = "DrinkManaPotion",
        activationPolicy = "onInputPressed",
        cooldown = 5.0,
        effectsToApplyOnActivate = { "ManaRegen" }
    }
}
```

## Best Practices

### 1. Attribute Design

**Keep attributes focused:**
```cpp
// Good - Clear, single-purpose attributes
"Health", "Mana", "Stamina", "MoveSpeed", "JumpHeight"

// Avoid - Vague or compound attributes
"Power", "Stats", "AllBuffs"
```

**Use appropriate min/max values:**
```cpp
// Resource attributes - min 0
{ name = "Health", minValue = 0.0f, maxValue = 100.0f }

// Percentage modifiers - 0 to 1
{ name = "DamageReduction", minValue = 0.0f, maxValue = 1.0f }

// Speed/movement - unbounded max
{ name = "MoveSpeed", minValue = 0.0f, maxValue = std::numeric_limits<float>::max() }
```

### 2. Effect Composition

**Combine simple effects for complex behaviors:**
```cpp
// Instead of one complex effect, use multiple simple effects
AbilityDef berserk {
    .name = "BerserkMode",
    .effectsToApplyOnActivate = {
        speedBoostId,      // Move faster
        damageBoostId,     // Hit harder
        defenseReductionId // Take more damage
    }
};
```

**Use tags to group related effects:**
```lua
Effects = {
    {
        name = "BleedingDOT",
        grantedTags = { "Status.Bleeding", "Status.Debuff" }
    },
    {
        name = "Poison",
        grantedTags = { "Status.Poisoned", "Status.Debuff" }
    }
}

-- Cleanse ability removes all debuffs
Abilities = {
    {
        name = "Cleanse",
        effectsToApplyOnActivate = { "CleanseBuff" }
    }
}

Effects = {
    {
        name = "CleanseBuff",
        durationType = "instant",
        removalTags = { "Status.Debuff" }
    }
}
```

### 3. Ability Organization

**Use hierarchical tags for filtering:**
```lua
Tags = {
    "Ability.Combat.Melee",
    "Ability.Combat.Ranged",
    "Ability.Movement.Dash",
    "Ability.Movement.Jump",
    "Ability.Utility.Heal"
}
```

**Group abilities by role:**
```lua
-- Tank abilities block attacks
Abilities = {
    { name = "Shield", blockAbilitiesWithTags = { "Ability.Attack" } },
    { name = "Taunt", blockAbilitiesWithTags = { "Ability.Movement" } }
}

-- DPS abilities cancel each other
Abilities = {
    { name = "RapidFire", cancelAbilitiesWithTags = { "Ability.Attack" } },
    { name = "PowerShot", cancelAbilitiesWithTags = { "Ability.Attack" } }
}
```

### 4. Performance Considerations

**Minimize periodic effects:**
```cpp
// Prefer instant or duration effects
// Periodic effects (DoT/HoT) trigger every period, costing performance

// Good for damage spikes
{ durationType = EffectDurationType::Instant }

// Good for temporary buffs
{ durationType = EffectDurationType::Duration }

// Use sparingly - ticks every period
{ durationType = EffectDurationType::Duration, period = 1.0f }
```

**Batch effect applications:**
```cpp
// Apply multiple effects at once instead of one-by-one
for (EffectId effectId : effectsToApply) {
    gas->applyEffect(target, effectId, source);
}
```

### 5. Debugging GAS

**Use callbacks to trace behavior:**
```cpp
gas->setAttributeChangeCallback([](const AttributeChangeEvent& event) {
    std::cout << "Attribute " << event.attribute
              << " changed from " << event.oldValue
              << " to " << event.newValue << "\n";
});

gas->setEffectAppliedCallback([](const EffectAppliedEvent& event) {
    std::cout << "Effect " << event.effect
              << " applied to entity " << event.target
              << " by " << event.source << "\n";
});

gas->setAbilityActivatedCallback([](const AbilityActivatedEvent& event) {
    std::cout << "Ability " << event.ability
              << " activated by " << event.entity << "\n";
});
```

**Query active state:**
```cpp
// Check what effects are active
auto effects = gas->getActiveEffects(player);
for (const auto& effect : effects) {
    auto def = gas->getEffectDef(effect.defId);
    std::cout << "Active: " << def->name
              << " (stacks: " << effect.stacks
              << ", remaining: " << effect.remainingDuration << ")\n";
}

// Check what tags entity has
const GameplayTagContainer* tags = gas->getTags(player);
for (const auto& tag : tags->getTags()) {
    std::cout << "Tag: " << tag.name << "\n";
}
```

## Common Patterns

### Pattern: Damage Calculation

```cpp
// Define base damage as attribute modifier
float baseDamage = 50.0f;
float attackPower = gas->getAttributeValue(attacker, attackPowerId);
float defense = gas->getAttributeValue(defender, defenseId);

float finalDamage = baseDamage + attackPower - defense;

// Apply as instant effect
EffectDef damageEffect {
    .durationType = EffectDurationType::Instant,
    .modifiers = {
        { .attribute = healthId, .op = EffectModifierOp::Add, .value = -finalDamage }
    }
};
EffectId damageId = gas->registerEffect(damageEffect);
gas->applyEffect(defender, damageId, attacker);
```

### Pattern: Combo System

```cpp
GameplayTag combo1Tag = gas->registerTag("Combo.Stage1");
GameplayTag combo2Tag = gas->registerTag("Combo.Stage2");
GameplayTag combo3Tag = gas->registerTag("Combo.Stage3");

// First attack grants combo tag
AbilityDef attack1 {
    .name = "Attack1",
    .effectsToApplyOnActivate = { grantCombo1TagId }
};

// Second attack requires combo1, grants combo2
AbilityDef attack2 {
    .name = "Attack2",
    .activationRequiredTags = { combo1Tag },
    .effectsToApplyOnActivate = { grantCombo2TagId }
};

// Third attack requires combo2, deals bonus damage
AbilityDef attack3 {
    .name = "Attack3",
    .activationRequiredTags = { combo2Tag },
    .effectsToApplyOnActivate = { finisherDamageId }
};
```

### Pattern: Buff/Debuff Management

```cpp
// All buffs grant "Status.Buff" tag
// All debuffs grant "Status.Debuff" tag

// Cleanse removes all debuffs
GameplayTag debuffTag = gas->registerTag("Status.Debuff");
EffectDef cleanseEffect {
    .name = "Cleanse",
    .durationType = EffectDurationType::Instant,
    .removalTags = { debuffTag }
};
cleanseEffect.removalTags.addTag(debuffTag);

// Dispel removes all buffs
GameplayTag buffTag = gas->registerTag("Status.Buff");
EffectDef dispelEffect {
    .name = "Dispel",
    .durationType = EffectDurationType::Instant,
    .removalTags = { buffTag }
};
dispelEffect.removalTags.addTag(buffTag);
```

### Pattern: State Machine with Tags

```cpp
// Movement states
GameplayTag idleTag = gas->registerTag("State.Idle");
GameplayTag runningTag = gas->registerTag("State.Running");
GameplayTag jumpingTag = gas->registerTag("State.Jumping");

// Transitions remove old state, add new state
void transitionToRunning(Entity entity) {
    gas->removeTag(entity, idleTag);
    gas->addTag(entity, runningTag);
}

// Abilities check state requirements
AbilityDef jumpAbility {
    .name = "Jump",
    .activationBlockedTags = { jumpingTag }  // Can't jump while jumping
};
```

### Pattern: Cooldown Reduction

```cpp
// Cooldown reduction as attribute
AttributeDef cooldownReduction {
    .name = "CooldownReduction",
    .baseValue = 0.0f,
    .minValue = 0.0f,
    .maxValue = 0.9f  // Max 90% CDR
};

// When ending ability, factor in CDR
float baseCooldown = 10.0f;
float cdr = gas->getAttributeValue(player, cdrId);
float actualCooldown = baseCooldown * (1.0f - cdr);

// Abilities use the calculated cooldown
// (Note: Current implementation uses fixed cooldown from AbilityDef,
//  you'd need custom logic to apply CDR modifier)
```

## Integration with Other Systems

### Physics Integration

```cpp
// Update movement speed based on GAS attribute
float speed = gas->getAttributeValue(player, moveSpeedId);
physics->setVelocity(player, direction * speed);

// Apply knockback effect
EffectDef knockback {
    .name = "Knockback",
    .durationType = EffectDurationType::Instant
};
gas->applyEffect(target, knockbackId, source);

// In update loop, check for knockback effect
if (gas->hasEffect(player, knockbackId)) {
    physics->applyImpulse(player, knockbackDirection * knockbackForce);
}
```

### Animation Integration

```cpp
gas->setAbilityActivatedCallback([&](const AbilityActivatedEvent& event) {
    auto def = gas->getAbilityDef(event.ability);

    if (def->name == "Attack") {
        animation->play(event.entity, "AttackAnim");
    } else if (def->name == "Block") {
        animation->play(event.entity, "BlockAnim");
    }
});
```

### UI Integration

```cpp
// Display cooldowns
float dashCooldown = gas->getAbilityCooldown(player, dashAbilityId);
ui->setCooldownBar("Dash", dashCooldown);

// Display attribute bars
float health = gas->getAttributeValue(player, healthId);
float maxHealth = gas->getAttributeBaseValue(player, healthId);
ui->setHealthBar(health / maxHealth);

// Display active buffs/debuffs
auto effects = gas->getActiveEffects(player);
for (const auto& effect : effects) {
    auto def = gas->getEffectDef(effect.defId);
    ui->addStatusIcon(def->name, effect.remainingDuration);
}
```

## Advanced Topics

### Dynamic Effect Creation

```cpp
// Create effects at runtime based on game state
float damageAmount = calculateDamage(attacker, defender);

EffectDef dynamicDamage {
    .name = "DynamicDamage_" + std::to_string(damageAmount),
    .durationType = EffectDurationType::Instant,
    .modifiers = {
        { .attribute = healthId, .op = EffectModifierOp::Add, .value = -damageAmount }
    }
};

EffectId id = gas->registerEffect(dynamicDamage);
gas->applyEffect(defender, id, attacker);
```

### Chained Effects

```cpp
// Effect that applies another effect
gas->setEffectAppliedCallback([&](const EffectAppliedEvent& event) {
    auto def = gas->getEffectDef(event.effect);

    if (def->name == "Explosion") {
        // Apply burning to nearby enemies
        for (Entity nearby : findNearbyEntities(event.target)) {
            gas->applyEffect(nearby, burningEffectId, event.source);
        }
    }
});
```

### Conditional Abilities

```cpp
bool canActivateSpecialMove(Entity player) {
    // Custom logic beyond tag requirements
    float health = gas->getAttributeValue(player, healthId);
    float maxHealth = gas->getAttributeBaseValue(player, healthId);

    return (health / maxHealth) < 0.3f &&  // Below 30% health
           gas->canActivateAbility(player, desperationMoveId);
}
```

## Troubleshooting

### Effect Not Applying

**Check tag requirements:**
```cpp
auto effectDef = gas->getEffectDef(effectId);
const GameplayTagContainer* targetTags = gas->getTags(target);

if (!targetTags->matchesQuery(effectDef->applicationRequiredTags,
                               effectDef->applicationBlockedTags)) {
    // Target doesn't meet tag requirements
}
```

### Ability Can't Activate

**Debug activation requirements:**
```cpp
if (!gas->canActivateAbility(player, abilityId)) {
    // Check each requirement
    if (!gas->hasAbility(player, abilityId)) {
        std::cout << "Ability not granted\n";
    }

    float cooldown = gas->getAbilityCooldown(player, abilityId);
    if (cooldown > 0.0f) {
        std::cout << "On cooldown: " << cooldown << "s remaining\n";
    }

    auto def = gas->getAbilityDef(abilityId);
    const GameplayTagContainer* tags = gas->getTags(player);
    if (!tags->matchesQuery(def->activationRequiredTags, def->activationBlockedTags)) {
        std::cout << "Tag requirements not met\n";
    }

    if (!checkCosts(player, *def)) {
        std::cout << "Insufficient resources\n";
    }
}
```

### Attribute Not Changing

**Verify effect is active:**
```cpp
if (!gas->hasEffect(entity, effectId)) {
    // Effect not active - may have been instant or expired
}

// Check if effect is modifying the right attribute
auto effectDef = gas->getEffectDef(effectId);
for (const auto& mod : effectDef->modifiers) {
    std::cout << "Modifies attribute " << mod.attribute << "\n";
}
```

## Summary

The Bestow GAS provides a powerful, data-driven framework for implementing RPG-style abilities and effects. Key takeaways:

- **Tags** provide flexible categorization and filtering
- **Attributes** track dynamic stats with automatic modifier application
- **Effects** modify attributes with support for instant, duration, and periodic changes
- **Abilities** are cooldown-based actions with costs, requirements, and effect application
- **Lua integration** enables hot-reloadable, designer-friendly content
- **Callbacks** allow integration with animation, audio, UI, and physics systems

For questions or issues, refer to the test suite at `/Users/jaaaacob/Documents/GameDev/jframe/tests/unit/GASSystemTests.cpp` for comprehensive usage examples.

// GAS System Demo - Comprehensive demonstration of IGASSystem interface
// This demo exercises every method and type in the Gameplay Ability System

// MSVC C++23 module compatibility for EnTT iterators
#include <jframe/entt_compat.hpp>

import std;
import jframe.types;
import jframe.gas;
import jframe.gas.impl;
import jframe.entity;
import jframe.entity.impl;
import jframe.events;
import jframe.events.impl;

using namespace jframe;

// Forward declarations
void printSection(const std::string& title);
void printSubSection(const std::string& title);
void printInfo(const std::string& message);
void printSuccess(const std::string& message);
void printValue(const std::string& label, const std::string& value);

// Demo functions
void demoTagRegistration(IGASSystem& gas);
void demoTagHierarchy(IGASSystem& gas);
void demoAttributeRegistration(IGASSystem& gas);
void demoEffectRegistration(IGASSystem& gas);
void demoAbilityRegistration(IGASSystem& gas);
void demoComponentManagement(IGASSystem& gas, IEntitySystem& entities);
void demoTagOperations(IGASSystem& gas, Entity entity);
void demoTagQueries(IGASSystem& gas, Entity entity);
void demoAttributeOperations(IGASSystem& gas, Entity entity);
void demoEffectOperations(IGASSystem& gas, Entity target, Entity source);
void demoAbilityOperations(IGASSystem& gas, Entity entity);
void demoCallbacks(IGASSystem& gas, Entity entity);
void demoLuaIntegration(IGASSystem& gas);
void demoUpdateLoop(IGASSystem& gas, Entity entity);

int main() {
    std::println("=================================================================");
    std::println("        JFrame GAS (Gameplay Ability System) Demo");
    std::println("=================================================================");
    std::println("");

    // Create concrete system implementations
    auto eventSystem = std::make_unique<EventSystem>();
    auto entitySystem = std::make_unique<EntitySystem>();
    auto gasSystem = std::make_unique<GASSystem>();

    // Initialize GAS system
    gasSystem->initialize();

    printInfo("This demo shows comprehensive usage of all GAS System APIs");
    std::println("");

    // Demonstrate all API sections
    try {
        // Registration APIs
        demoTagRegistration(*gasSystem);
        demoTagHierarchy(*gasSystem);
        demoAttributeRegistration(*gasSystem);
        demoEffectRegistration(*gasSystem);
        demoAbilityRegistration(*gasSystem);

        // Create test entities
        Entity player = entitySystem->createEntity();
        Entity enemy = entitySystem->createEntity();

        // Component management
        demoComponentManagement(*gasSystem, *entitySystem);

        // Tag operations
        demoTagOperations(*gasSystem, player);
        demoTagQueries(*gasSystem, player);

        // Attribute operations
        demoAttributeOperations(*gasSystem, player);

        // Effect operations
        demoEffectOperations(*gasSystem, player, enemy);

        // Ability operations
        demoAbilityOperations(*gasSystem, player);

        // Callbacks
        demoCallbacks(*gasSystem, player);

        // Lua integration
        demoLuaIntegration(*gasSystem);

        // Update loop
        demoUpdateLoop(*gasSystem, player);

        std::println("");
        printSuccess("All GAS System API demonstrations completed successfully!");

    } catch (const std::exception& e) {
        std::println("Error: {}", e.what());
        return 1;
    }

    return 0;
}

//=============================================================================
// Demo Implementations
//=============================================================================

void demoTagRegistration(IGASSystem& gas) {
    printSection("1. TAG REGISTRATION");

    printSubSection("registerTag()");
    GameplayTag stateTag = gas.registerTag("State");
    GameplayTag movementTag = gas.registerTag("State.Movement");
    GameplayTag dashingTag = gas.registerTag("State.Movement.Dashing");
    GameplayTag sprintingTag = gas.registerTag("State.Movement.Sprinting");
    GameplayTag combatTag = gas.registerTag("State.Combat");
    GameplayTag attackingTag = gas.registerTag("State.Combat.Attacking");
    GameplayTag blockingTag = gas.registerTag("State.Combat.Blocking");
    GameplayTag statusTag = gas.registerTag("Status");
    GameplayTag stunnedTag = gas.registerTag("Status.Stunned");
    GameplayTag poisonedTag = gas.registerTag("Status.Poisoned");
    GameplayTag invulnerableTag = gas.registerTag("Status.Invulnerable");

    printInfo("Registered hierarchical tags:");
    printValue("  State", stateTag.name);
    printValue("  State.Movement", movementTag.name);
    printValue("  State.Movement.Dashing", dashingTag.name);
    printValue("  State.Movement.Sprinting", sprintingTag.name);
    printValue("  State.Combat", combatTag.name);
    printValue("  State.Combat.Attacking", attackingTag.name);
    printValue("  State.Combat.Blocking", blockingTag.name);
    printValue("  Status", statusTag.name);
    printValue("  Status.Stunned", stunnedTag.name);
    printValue("  Status.Poisoned", poisonedTag.name);
    printValue("  Status.Invulnerable", invulnerableTag.name);

    printSubSection("findTag()");
    auto foundTag = gas.findTag("State.Movement.Dashing");
    if (foundTag.has_value()) {
        printSuccess(std::format("Found tag: {} (ID: {})", foundTag->name, foundTag->id));
    }

    auto notFoundTag = gas.findTag("NonExistent.Tag");
    if (!notFoundTag.has_value()) {
        printSuccess("Correctly returned empty optional for non-existent tag");
    }

    std::println("");
}

void demoTagHierarchy(IGASSystem& gas) {
    printSection("2. TAG HIERARCHY");

    printSubSection("isParentOf()");

    auto stateTag = gas.findTag("State");
    auto movementTag = gas.findTag("State.Movement");
    auto dashingTag = gas.findTag("State.Movement.Dashing");
    auto combatTag = gas.findTag("State.Combat");

    if (stateTag && movementTag && dashingTag && combatTag) {
        bool isParent1 = gas.isParentOf(*stateTag, *movementTag);
        printValue("State is parent of State.Movement", isParent1 ? "true" : "false");

        bool isParent2 = gas.isParentOf(*movementTag, *dashingTag);
        printValue("State.Movement is parent of State.Movement.Dashing", isParent2 ? "true" : "false");

        bool isParent3 = gas.isParentOf(*stateTag, *dashingTag);
        printValue("State is parent of State.Movement.Dashing (transitive)", isParent3 ? "true" : "false");

        bool notParent = gas.isParentOf(*combatTag, *dashingTag);
        printValue("State.Combat is parent of State.Movement.Dashing", notParent ? "true" : "false");
    }

    std::println("");
}

void demoAttributeRegistration(IGASSystem& gas) {
    printSection("3. ATTRIBUTE REGISTRATION");

    printSubSection("registerAttribute()");

    AttributeDef healthDef{
        .id = 0,  // Auto-assigned by system
        .name = "Health",
        .baseValue = 100.0f,
        .minValue = 0.0f,
        .maxValue = 100.0f,
        .clampEnabled = true
    };
    AttributeId healthId = gas.registerAttribute(healthDef);
    printValue("Registered Health", std::format("ID: {}", healthId));

    AttributeDef manaDef{
        .id = 0,
        .name = "Mana",
        .baseValue = 50.0f,
        .minValue = 0.0f,
        .maxValue = 50.0f,
        .clampEnabled = true
    };
    AttributeId manaId = gas.registerAttribute(manaDef);
    printValue("Registered Mana", std::format("ID: {}", manaId));

    AttributeDef moveSpeedDef{
        .id = 0,
        .name = "MoveSpeed",
        .baseValue = 300.0f,
        .minValue = 0.0f,
        .maxValue = 1000.0f,
        .clampEnabled = true
    };
    AttributeId moveSpeedId = gas.registerAttribute(moveSpeedDef);
    printValue("Registered MoveSpeed", std::format("ID: {}", moveSpeedId));

    AttributeDef attackPowerDef{
        .id = 0,
        .name = "AttackPower",
        .baseValue = 10.0f,
        .minValue = 0.0f,
        .maxValue = 999.0f,
        .clampEnabled = true
    };
    AttributeId attackPowerId = gas.registerAttribute(attackPowerDef);
    printValue("Registered AttackPower", std::format("ID: {}", attackPowerId));

    printSubSection("getAttributeDef() - by ID");
    auto healthDefById = gas.getAttributeDef(healthId);
    if (healthDefById.has_value()) {
        printSuccess(std::format("Retrieved {} attribute by ID", healthDefById->name));
        printValue("  Base Value", std::format("{}", healthDefById->baseValue));
        printValue("  Min Value", std::format("{}", healthDefById->minValue));
        printValue("  Max Value", std::format("{}", healthDefById->maxValue));
    }

    printSubSection("getAttributeDef() - by name");
    auto manaDefByName = gas.getAttributeDef("Mana");
    if (manaDefByName.has_value()) {
        printSuccess(std::format("Retrieved {} attribute by name", manaDefByName->name));
        printValue("  Base Value", std::format("{}", manaDefByName->baseValue));
    }

    std::println("");
}

void demoEffectRegistration(IGASSystem& gas) {
    printSection("4. EFFECT REGISTRATION");

    // Get attribute IDs for modifiers
    auto healthId = gas.getAttributeDef("Health")->id;
    auto manaId = gas.getAttributeDef("Mana")->id;
    auto moveSpeedId = gas.getAttributeDef("MoveSpeed")->id;
    auto attackPowerId = gas.getAttributeDef("AttackPower")->id;

    // Get tags for effect conditions
    auto stunnedTag = *gas.findTag("Status.Stunned");
    auto invulnerableTag = *gas.findTag("Status.Invulnerable");

    printSubSection("registerEffect() - Instant damage effect");
    EffectDef damageDef{
        .id = 0,
        .name = "Damage",
        .durationType = EffectDurationType::Instant,
        .duration = 0.0f,
        .period = 0.0f,
        .modifiers = {
            EffectModifier{
                .attribute = healthId,
                .op = EffectModifierOp::Add,
                .value = -25.0f
            }
        },
        .grantedTags = {},
        .applicationRequiredTags = {},
        .applicationBlockedTags = {},
        .removalTags = {},
        .stackable = false,
        .maxStacks = 1
    };
    EffectId damageId = gas.registerEffect(damageDef);
    printValue("Registered Damage effect", std::format("ID: {}", damageId));
    printValue("  Type", "Instant");
    printValue("  Modifier", "-25 Health");

    printSubSection("registerEffect() - Heal over time effect");
    EffectDef healDef{
        .id = 0,
        .name = "HealOverTime",
        .durationType = EffectDurationType::Duration,
        .duration = 5.0f,
        .period = 1.0f,  // Tick every second
        .modifiers = {
            EffectModifier{
                .attribute = healthId,
                .op = EffectModifierOp::Add,
                .value = 5.0f
            }
        },
        .grantedTags = {},
        .applicationRequiredTags = {},
        .applicationBlockedTags = {},
        .removalTags = {},
        .stackable = true,
        .maxStacks = 3
    };
    EffectId healId = gas.registerEffect(healDef);
    printValue("Registered HealOverTime effect", std::format("ID: {}", healId));
    printValue("  Type", "Duration (5 seconds)");
    printValue("  Period", "1 second");
    printValue("  Modifier", "+5 Health per tick");
    printValue("  Stackable", "true (max 3 stacks)");

    printSubSection("registerEffect() - Speed buff effect");
    EffectDef speedBuffDef{
        .id = 0,
        .name = "SpeedBuff",
        .durationType = EffectDurationType::Duration,
        .duration = 10.0f,
        .period = 0.0f,
        .modifiers = {
            EffectModifier{
                .attribute = moveSpeedId,
                .op = EffectModifierOp::Multiply,
                .value = 1.5f  // 150% speed
            }
        },
        .grantedTags = {},
        .applicationRequiredTags = {},
        .applicationBlockedTags = {},
        .removalTags = {},
        .stackable = false,
        .maxStacks = 1
    };
    EffectId speedBuffId = gas.registerEffect(speedBuffDef);
    printValue("Registered SpeedBuff effect", std::format("ID: {}", speedBuffId));
    printValue("  Type", "Duration (10 seconds)");
    printValue("  Modifier", "x1.5 MoveSpeed");

    printSubSection("registerEffect() - Stun effect with tag conditions");
    EffectDef stunDef{
        .id = 0,
        .name = "Stun",
        .durationType = EffectDurationType::Duration,
        .duration = 2.0f,
        .period = 0.0f,
        .modifiers = {
            EffectModifier{
                .attribute = moveSpeedId,
                .op = EffectModifierOp::Override,
                .value = 0.0f  // Cannot move
            }
        },
        .grantedTags = {},
        .applicationRequiredTags = {},
        .applicationBlockedTags = {},
        .removalTags = {},
        .stackable = false,
        .maxStacks = 1
    };
    stunDef.grantedTags.addTag(stunnedTag);
    stunDef.applicationBlockedTags.addTag(invulnerableTag);  // Can't stun invulnerable targets
    EffectId stunId = gas.registerEffect(stunDef);
    printValue("Registered Stun effect", std::format("ID: {}", stunId));
    printValue("  Type", "Duration (2 seconds)");
    printValue("  Modifier", "Override MoveSpeed to 0");
    printValue("  Granted Tags", "Status.Stunned");
    printValue("  Blocked by", "Status.Invulnerable");

    printSubSection("getEffectDef() - by ID");
    auto damageDefById = gas.getEffectDef(damageId);
    if (damageDefById.has_value()) {
        printSuccess(std::format("Retrieved {} effect by ID", damageDefById->name));
    }

    printSubSection("getEffectDef() - by name");
    auto healDefByName = gas.getEffectDef("HealOverTime");
    if (healDefByName.has_value()) {
        printSuccess(std::format("Retrieved {} effect by name", healDefByName->name));
    }

    std::println("");
}

void demoAbilityRegistration(IGASSystem& gas) {
    printSection("5. ABILITY REGISTRATION");

    auto healthId = gas.getAttributeDef("Health")->id;
    auto manaId = gas.getAttributeDef("Mana")->id;
    auto stunnedTag = *gas.findTag("Status.Stunned");
    auto attackingTag = *gas.findTag("State.Combat.Attacking");
    auto dashingTag = *gas.findTag("State.Movement.Dashing");

    auto damageId = gas.getEffectDef("Damage")->id;
    auto speedBuffId = gas.getEffectDef("SpeedBuff")->id;

    printSubSection("registerAbility() - Basic attack");
    AbilityDef attackDef{
        .id = 0,
        .name = "BasicAttack",
        .activationPolicy = AbilityActivationPolicy::OnInputPressed,
        .cooldown = 1.0f,
        .costs = {},
        .activationRequiredTags = {},
        .activationBlockedTags = {},
        .abilityTags = {},
        .cancelAbilitiesWithTags = {},
        .blockAbilitiesWithTags = {},
        .effectsToApplyOnActivate = {damageId},
        .effectsToApplyOnEnd = {}
    };
    attackDef.abilityTags.addTag(attackingTag);
    attackDef.activationBlockedTags.addTag(stunnedTag);  // Can't attack while stunned
    AbilityId attackId = gas.registerAbility(attackDef);
    printValue("Registered BasicAttack ability", std::format("ID: {}", attackId));
    printValue("  Activation", "OnInputPressed");
    printValue("  Cooldown", "1 second");
    printValue("  Blocked by", "Status.Stunned");
    printValue("  Effects", "Damage");

    printSubSection("registerAbility() - Dash with mana cost");
    AbilityDef dashDef{
        .id = 0,
        .name = "Dash",
        .activationPolicy = AbilityActivationPolicy::OnInputPressed,
        .cooldown = 3.0f,
        .costs = {
            AbilityCost{
                .attribute = manaId,
                .cost = 10.0f
            }
        },
        .activationRequiredTags = {},
        .activationBlockedTags = {},
        .abilityTags = {},
        .cancelAbilitiesWithTags = {},
        .blockAbilitiesWithTags = {},
        .effectsToApplyOnActivate = {speedBuffId},
        .effectsToApplyOnEnd = {}
    };
    dashDef.abilityTags.addTag(dashingTag);
    dashDef.activationBlockedTags.addTag(stunnedTag);
    AbilityId dashId = gas.registerAbility(dashDef);
    printValue("Registered Dash ability", std::format("ID: {}", dashId));
    printValue("  Activation", "OnInputPressed");
    printValue("  Cooldown", "3 seconds");
    printValue("  Cost", "10 Mana");
    printValue("  Blocked by", "Status.Stunned");
    printValue("  Effects", "SpeedBuff");

    printSubSection("registerAbility() - Passive regeneration");
    AbilityDef regenDef{
        .id = 0,
        .name = "Regeneration",
        .activationPolicy = AbilityActivationPolicy::Passive,
        .cooldown = 0.0f,
        .costs = {},
        .activationRequiredTags = {},
        .activationBlockedTags = {},
        .abilityTags = {},
        .cancelAbilitiesWithTags = {},
        .blockAbilitiesWithTags = {},
        .effectsToApplyOnActivate = {},
        .effectsToApplyOnEnd = {}
    };
    AbilityId regenId = gas.registerAbility(regenDef);
    printValue("Registered Regeneration ability", std::format("ID: {}", regenId));
    printValue("  Activation", "Passive (always active)");

    printSubSection("getAbilityDef() - by ID");
    auto attackDefById = gas.getAbilityDef(attackId);
    if (attackDefById.has_value()) {
        printSuccess(std::format("Retrieved {} ability by ID", attackDefById->name));
    }

    printSubSection("getAbilityDef() - by name");
    auto dashDefByName = gas.getAbilityDef("Dash");
    if (dashDefByName.has_value()) {
        printSuccess(std::format("Retrieved {} ability by name", dashDefByName->name));
    }

    std::println("");
}

void demoComponentManagement(IGASSystem& gas, IEntitySystem& entities) {
    printSection("6. COMPONENT MANAGEMENT");

    Entity player = entities.createEntity();
    Entity enemy = entities.createEntity();

    printSubSection("initializeComponent()");
    gas.initializeComponent(player);
    gas.initializeComponent(enemy);
    printSuccess("Initialized ability components for player and enemy");

    printSubSection("hasComponent()");
    bool playerHasComp = gas.hasComponent(player);
    bool enemyHasComp = gas.hasComponent(enemy);
    printValue("Player has component", playerHasComp ? "true" : "false");
    printValue("Enemy has component", enemyHasComp ? "true" : "false");

    printSubSection("getComponent()");
    AbilitySystemComponent* playerComp = gas.getComponent(player);
    const AbilitySystemComponent* enemyCompConst = gas.getComponent(enemy);
    if (playerComp) {
        printSuccess("Retrieved mutable player component");
        printValue("  Tags count", std::format("{}", playerComp->ownedTags.count()));
        printValue("  Attributes count", std::format("{}", playerComp->attributes.size()));
        printValue("  Active effects count", std::format("{}", playerComp->activeEffects.size()));
    }
    if (enemyCompConst) {
        printSuccess("Retrieved const enemy component");
    }

    printSubSection("removeComponent()");
    Entity tempEntity = entities.createEntity();
    gas.initializeComponent(tempEntity);
    printInfo("Created temp entity and initialized component");
    gas.removeComponent(tempEntity);
    bool hasCompAfterRemoval = gas.hasComponent(tempEntity);
    printValue("Temp entity has component after removal", hasCompAfterRemoval ? "true" : "false");

    std::println("");
}

void demoTagOperations(IGASSystem& gas, Entity entity) {
    printSection("7. TAG OPERATIONS");

    auto dashingTag = *gas.findTag("State.Movement.Dashing");
    auto attackingTag = *gas.findTag("State.Combat.Attacking");
    auto stunnedTag = *gas.findTag("Status.Stunned");

    printSubSection("addTag()");
    gas.addTag(entity, dashingTag);
    gas.addTag(entity, attackingTag);
    printSuccess("Added State.Movement.Dashing and State.Combat.Attacking tags");

    printSubSection("hasTag()");
    bool hasDashing = gas.hasTag(entity, dashingTag);
    bool hasAttacking = gas.hasTag(entity, attackingTag);
    bool hasStunned = gas.hasTag(entity, stunnedTag);
    printValue("Has State.Movement.Dashing", hasDashing ? "true" : "false");
    printValue("Has State.Combat.Attacking", hasAttacking ? "true" : "false");
    printValue("Has Status.Stunned", hasStunned ? "true" : "false");

    printSubSection("getTags()");
    const GameplayTagContainer* tags = gas.getTags(entity);
    if (tags) {
        auto tagList = tags->getTags();
        printSuccess(std::format("Entity has {} tags:", tagList.size()));
        for (const auto& tag : tagList) {
            printValue("  -", tag.name);
        }
    }

    printSubSection("removeTag()");
    gas.removeTag(entity, attackingTag);
    bool hasAttackingAfterRemoval = gas.hasTag(entity, attackingTag);
    printValue("Has State.Combat.Attacking after removal", hasAttackingAfterRemoval ? "true" : "false");

    std::println("");
}

void demoTagQueries(IGASSystem& gas, Entity entity) {
    printSection("8. TAG QUERIES (GameplayTagContainer)");

    auto dashingTag = *gas.findTag("State.Movement.Dashing");
    auto sprintingTag = *gas.findTag("State.Movement.Sprinting");
    auto stunnedTag = *gas.findTag("Status.Stunned");
    auto poisonedTag = *gas.findTag("Status.Poisoned");

    // Set up entity tags
    gas.addTag(entity, dashingTag);
    gas.addTag(entity, poisonedTag);

    printSubSection("hasAny() - Check if entity has any of multiple tags");
    GameplayTagContainer queryTags;
    queryTags.addTag(dashingTag);
    queryTags.addTag(stunnedTag);

    const GameplayTagContainer* entityTags = gas.getTags(entity);
    if (entityTags) {
        bool hasAny = entityTags->hasAny(queryTags);
        printValue("Entity has (Dashing OR Stunned)", hasAny ? "true" : "false");
    }

    printSubSection("hasAll() - Check if entity has all tags");
    GameplayTagContainer requiredTags;
    requiredTags.addTag(dashingTag);
    requiredTags.addTag(poisonedTag);

    if (entityTags) {
        bool hasAll = entityTags->hasAll(requiredTags);
        printValue("Entity has (Dashing AND Poisoned)", hasAll ? "true" : "false");

        requiredTags.addTag(stunnedTag);
        bool hasAllWithStun = entityTags->hasAll(requiredTags);
        printValue("Entity has (Dashing AND Poisoned AND Stunned)", hasAllWithStun ? "true" : "false");
    }

    printSubSection("matchesQuery() - Complex tag query");
    GameplayTagContainer required;
    required.addTag(dashingTag);

    GameplayTagContainer blocked;
    blocked.addTag(stunnedTag);

    if (entityTags) {
        bool matches = entityTags->matchesQuery(required, blocked);
        printInfo("Query: Must have Dashing, must NOT have Stunned");
        printValue("Entity matches query", matches ? "true" : "false");

        // Add stunned tag and check again
        gas.addTag(entity, stunnedTag);
        entityTags = gas.getTags(entity);  // Refresh pointer
        bool matchesWithStun = entityTags->matchesQuery(required, blocked);
        printInfo("After adding Stunned tag:");
        printValue("Entity matches query", matchesWithStun ? "true" : "false");
    }

    std::println("");
}

void demoAttributeOperations(IGASSystem& gas, Entity entity) {
    printSection("9. ATTRIBUTE OPERATIONS");

    auto healthId = gas.getAttributeDef("Health")->id;
    auto manaId = gas.getAttributeDef("Mana")->id;
    auto moveSpeedId = gas.getAttributeDef("MoveSpeed")->id;

    printSubSection("initializeAttribute()");
    gas.initializeAttribute(entity, healthId, 100.0f);
    gas.initializeAttribute(entity, manaId, 50.0f);
    gas.initializeAttribute(entity, moveSpeedId, 300.0f);
    printSuccess("Initialized Health=100, Mana=50, MoveSpeed=300");

    printSubSection("getAttributeValue() & getAttributeBaseValue()");
    float healthCurrent = gas.getAttributeValue(entity, healthId);
    float healthBase = gas.getAttributeBaseValue(entity, healthId);
    printValue("Health current value", std::format("{}", healthCurrent));
    printValue("Health base value", std::format("{}", healthBase));

    float manaCurrent = gas.getAttributeValue(entity, manaId);
    float manaBase = gas.getAttributeBaseValue(entity, manaId);
    printValue("Mana current value", std::format("{}", manaCurrent));
    printValue("Mana base value", std::format("{}", manaBase));

    printSubSection("modifyAttribute()");
    printInfo("Taking 25 damage...");
    gas.modifyAttribute(entity, healthId, -25.0f);
    healthCurrent = gas.getAttributeValue(entity, healthId);
    printValue("Health after damage", std::format("{}", healthCurrent));

    printInfo("Spending 10 mana...");
    gas.modifyAttribute(entity, manaId, -10.0f);
    manaCurrent = gas.getAttributeValue(entity, manaId);
    printValue("Mana after spending", std::format("{}", manaCurrent));

    printSubSection("setAttributeBaseValue()");
    printInfo("Leveling up: increasing base health to 150");
    gas.setAttributeBaseValue(entity, healthId, 150.0f);
    healthBase = gas.getAttributeBaseValue(entity, healthId);
    healthCurrent = gas.getAttributeValue(entity, healthId);
    printValue("Health base value", std::format("{}", healthBase));
    printValue("Health current value", std::format("{}", healthCurrent));

    printSubSection("AttributeValue struct methods");
    printInfo("Demonstrating AttributeValue struct:");
    AttributeValue attrValue;
    attrValue.setBase(100.0f, 0.0f, 200.0f);
    printValue("  After setBase(100)", std::format("base={}", attrValue.baseValue));

    attrValue.setCurrent(75.0f, 0.0f, 200.0f);
    printValue("  After setCurrent(75)", std::format("current={}", attrValue.currentValue));

    attrValue.modify(-25.0f, 0.0f, 200.0f);
    printValue("  After modify(-25)", std::format("current={}", attrValue.currentValue));

    attrValue.modify(500.0f, 0.0f, 200.0f);  // Should clamp to 200
    printValue("  After modify(500) with max=200", std::format("current={}", attrValue.currentValue));

    std::println("");
}

void demoEffectOperations(IGASSystem& gas, Entity target, Entity source) {
    printSection("10. EFFECT OPERATIONS");

    auto damageId = gas.getEffectDef("Damage")->id;
    auto healId = gas.getEffectDef("HealOverTime")->id;
    auto speedBuffId = gas.getEffectDef("SpeedBuff")->id;
    auto stunId = gas.getEffectDef("Stun")->id;
    auto healthId = gas.getAttributeDef("Health")->id;

    // Make sure target has component and attributes
    if (!gas.hasComponent(target)) {
        gas.initializeComponent(target);
    }
    gas.initializeAttribute(target, healthId, 100.0f);

    printSubSection("applyEffect() - Instant damage");
    float healthBefore = gas.getAttributeValue(target, healthId);
    printValue("Health before", std::format("{}", healthBefore));

    gas.applyEffect(target, damageId, source);
    float healthAfter = gas.getAttributeValue(target, healthId);
    printValue("Health after instant damage effect", std::format("{}", healthAfter));

    printSubSection("applyEffect() - Duration effects");
    gas.applyEffect(target, healId, source);
    gas.applyEffect(target, speedBuffId, source);
    gas.applyEffect(target, stunId, source);
    printSuccess("Applied HealOverTime, SpeedBuff, and Stun effects");

    printSubSection("hasEffect()");
    bool hasHeal = gas.hasEffect(target, healId);
    bool hasSpeed = gas.hasEffect(target, speedBuffId);
    bool hasStun = gas.hasEffect(target, stunId);
    printValue("Has HealOverTime", hasHeal ? "true" : "false");
    printValue("Has SpeedBuff", hasSpeed ? "true" : "false");
    printValue("Has Stun", hasStun ? "true" : "false");

    printSubSection("getActiveEffects()");
    auto activeEffects = gas.getActiveEffects(target);
    printSuccess(std::format("Entity has {} active effects:", activeEffects.size()));
    for (const auto& effect : activeEffects) {
        auto effectDef = gas.getEffectDef(effect.defId);
        if (effectDef.has_value()) {
            printValue("  -", std::format("{} (remaining: {:.1f}s, stacks: {})",
                effectDef->name, effect.remainingDuration, effect.stacks));
        }
    }

    printSubSection("Effect stacking");
    printInfo("Applying HealOverTime effect 2 more times (stackable, max 3)...");
    gas.applyEffect(target, healId, source);
    gas.applyEffect(target, healId, source);
    activeEffects = gas.getActiveEffects(target);
    for (const auto& effect : activeEffects) {
        if (effect.defId == healId) {
            printValue("HealOverTime stacks", std::format("{}", effect.stacks));
        }
    }

    printSubSection("removeEffect()");
    gas.removeEffect(target, speedBuffId);
    bool hasSpeedAfterRemoval = gas.hasEffect(target, speedBuffId);
    printValue("Has SpeedBuff after removal", hasSpeedAfterRemoval ? "true" : "false");

    printSubSection("removeAllEffects()");
    gas.removeAllEffects(target);
    activeEffects = gas.getActiveEffects(target);
    printValue("Active effects after removeAllEffects()", std::format("{}", activeEffects.size()));

    std::println("");
}

void demoAbilityOperations(IGASSystem& gas, Entity entity) {
    printSection("11. ABILITY OPERATIONS");

    auto attackId = gas.getAbilityDef("BasicAttack")->id;
    auto dashId = gas.getAbilityDef("Dash")->id;
    auto regenId = gas.getAbilityDef("Regeneration")->id;

    auto healthId = gas.getAttributeDef("Health")->id;
    auto manaId = gas.getAttributeDef("Mana")->id;

    // Initialize entity
    if (!gas.hasComponent(entity)) {
        gas.initializeComponent(entity);
    }
    gas.initializeAttribute(entity, healthId, 100.0f);
    gas.initializeAttribute(entity, manaId, 50.0f);

    printSubSection("grantAbility()");
    gas.grantAbility(entity, attackId);
    gas.grantAbility(entity, dashId);
    gas.grantAbility(entity, regenId);
    printSuccess("Granted BasicAttack, Dash, and Regeneration abilities");

    printSubSection("hasAbility()");
    bool hasAttack = gas.hasAbility(entity, attackId);
    bool hasDash = gas.hasAbility(entity, dashId);
    bool hasRegen = gas.hasAbility(entity, regenId);
    printValue("Has BasicAttack", hasAttack ? "true" : "false");
    printValue("Has Dash", hasDash ? "true" : "false");
    printValue("Has Regeneration", hasRegen ? "true" : "false");

    printSubSection("canActivateAbility()");
    bool canActivateAttack = gas.canActivateAbility(entity, attackId);
    bool canActivateDash = gas.canActivateAbility(entity, dashId);
    printValue("Can activate BasicAttack", canActivateAttack ? "true" : "false");
    printValue("Can activate Dash", canActivateDash ? "true" : "false");

    printSubSection("tryActivateAbility()");
    bool attackActivated = gas.tryActivateAbility(entity, attackId);
    printValue("BasicAttack activation result", attackActivated ? "SUCCESS" : "FAILED");

    if (attackActivated) {
        printInfo("BasicAttack is now on cooldown");
    }

    printSubSection("getAbilityCooldown()");
    float attackCooldown = gas.getAbilityCooldown(entity, attackId);
    float dashCooldown = gas.getAbilityCooldown(entity, dashId);
    printValue("BasicAttack cooldown remaining", std::format("{:.2f}s", attackCooldown));
    printValue("Dash cooldown remaining", std::format("{:.2f}s", dashCooldown));

    printSubSection("isAbilityActive()");
    bool attackActive = gas.isAbilityActive(entity, attackId);
    bool dashActive = gas.isAbilityActive(entity, dashId);
    printValue("BasicAttack is active", attackActive ? "true" : "false");
    printValue("Dash is active", dashActive ? "true" : "false");

    printSubSection("Ability with insufficient resources");
    printInfo("Setting mana to 5 (insufficient for Dash which costs 10)...");
    gas.setAttributeBaseValue(entity, manaId, 5.0f);
    float mana = gas.getAttributeValue(entity, manaId);
    printValue("Current mana", std::format("{}", mana));

    bool canActivateDashNoMana = gas.canActivateAbility(entity, dashId);
    printValue("Can activate Dash with insufficient mana", canActivateDashNoMana ? "true" : "false");

    printSubSection("endAbility()");
    // Activate dash first (restore mana)
    gas.setAttributeBaseValue(entity, manaId, 50.0f);
    gas.tryActivateAbility(entity, dashId);
    printInfo("Activated Dash ability");

    bool dashActiveBeforeEnd = gas.isAbilityActive(entity, dashId);
    printValue("Dash is active before endAbility()", dashActiveBeforeEnd ? "true" : "false");

    gas.endAbility(entity, dashId);
    bool dashActiveAfterEnd = gas.isAbilityActive(entity, dashId);
    printValue("Dash is active after endAbility()", dashActiveAfterEnd ? "true" : "false");

    printSubSection("removeAbility()");
    gas.removeAbility(entity, regenId);
    bool hasRegenAfterRemoval = gas.hasAbility(entity, regenId);
    printValue("Has Regeneration after removal", hasRegenAfterRemoval ? "true" : "false");

    std::println("");
}

void demoCallbacks(IGASSystem& gas, Entity entity) {
    printSection("12. CALLBACKS");

    auto healthId = gas.getAttributeDef("Health")->id;
    auto damageId = gas.getEffectDef("Damage")->id;
    auto attackId = gas.getAbilityDef("BasicAttack")->id;

    // Initialize entity
    if (!gas.hasComponent(entity)) {
        gas.initializeComponent(entity);
    }
    gas.initializeAttribute(entity, healthId, 100.0f);
    gas.grantAbility(entity, attackId);

    printSubSection("setAttributeChangeCallback()");
    gas.setAttributeChangeCallback([](const AttributeChangeEvent& event) {
        std::cout << std::format("  [CALLBACK] Attribute changed on entity {}\n", static_cast<std::uint32_t>(event.entity));
        std::cout << std::format("             Attribute ID: {}\n", event.attribute);
        std::cout << std::format("             Old value: {} -> New value: {}\n", event.oldValue, event.newValue);
        std::cout << std::format("             Delta: {}\n", event.newValue - event.oldValue);
    });
    printSuccess("Registered attribute change callback");

    printInfo("Triggering attribute change by modifying health...");
    gas.modifyAttribute(entity, healthId, -15.0f);

    printSubSection("setEffectAppliedCallback()");
    gas.setEffectAppliedCallback([](const EffectAppliedEvent& event) {
        std::cout << "  [CALLBACK] Effect applied!\n";
        std::cout << std::format("             Target: {}\n", static_cast<std::uint32_t>(event.target));
        std::cout << std::format("             Source: {}\n", static_cast<std::uint32_t>(event.source));
        std::cout << std::format("             Effect ID: {}\n", event.effect);
    });
    printSuccess("Registered effect applied callback");

    printInfo("Triggering effect callback by applying damage...");
    Entity attacker{42};  // Mock attacker
    gas.applyEffect(entity, damageId, attacker);

    printSubSection("setAbilityActivatedCallback()");
    gas.setAbilityActivatedCallback([](const AbilityActivatedEvent& event) {
        std::cout << "  [CALLBACK] Ability activated!\n";
        std::cout << std::format("             Entity: {}\n", static_cast<std::uint32_t>(event.entity));
        std::cout << std::format("             Ability ID: {}\n", event.ability);
    });
    printSuccess("Registered ability activated callback");

    printInfo("Triggering ability callback by activating BasicAttack...");
    gas.tryActivateAbility(entity, attackId);

    std::println("");
}

void demoLuaIntegration(IGASSystem& gas) {
    printSection("13. LUA INTEGRATION");

    printSubSection("loadDefinitionsFromLua()");

    std::string luaDefinitions = R"(
-- Define gameplay tags
tags = {
    "Ability.Attack.Melee",
    "Ability.Attack.Ranged",
    "Ability.Magic.Fire",
    "Ability.Magic.Ice",
}

-- Define attributes
attributes = {
    {
        name = "Stamina",
        baseValue = 100,
        minValue = 0,
        maxValue = 100,
        clampEnabled = true
    },
    {
        name = "Energy",
        baseValue = 50,
        minValue = 0,
        maxValue = 100,
        clampEnabled = true
    }
}

-- Define effects
effects = {
    {
        name = "FireBurn",
        durationType = "Duration",
        duration = 3.0,
        period = 0.5,
        modifiers = {
            {
                attribute = "Health",
                op = "Add",
                value = -5
            }
        },
        stackable = true,
        maxStacks = 5
    }
}

-- Define abilities
abilities = {
    {
        name = "Fireball",
        activationPolicy = "OnInputPressed",
        cooldown = 5.0,
        costs = {
            { attribute = "Mana", cost = 25 }
        },
        effectsToApplyOnActivate = { "FireBurn" }
    }
}
)";

    printInfo("Loading definitions from Lua script...");
    bool success = gas.loadDefinitionsFromLua(luaDefinitions);

    if (success) {
        printSuccess("Successfully loaded definitions from Lua");

        // Verify loaded definitions
        auto staminaAttr = gas.getAttributeDef("Stamina");
        if (staminaAttr.has_value()) {
            printValue("Loaded attribute", std::format("Stamina (base: {})", staminaAttr->baseValue));
        }

        auto fireBurnEffect = gas.getEffectDef("FireBurn");
        if (fireBurnEffect.has_value()) {
            printValue("Loaded effect", std::format("FireBurn (duration: {}s)", fireBurnEffect->duration));
        }

        auto fireballAbility = gas.getAbilityDef("Fireball");
        if (fireballAbility.has_value()) {
            printValue("Loaded ability", std::format("Fireball (cooldown: {}s)", fireballAbility->cooldown));
        }
    } else {
        printInfo("Lua integration requires concrete implementation");
    }

    std::println("");
}

void demoUpdateLoop(IGASSystem& gas, Entity entity) {
    printSection("14. UPDATE LOOP");

    auto healthId = gas.getAttributeDef("Health")->id;
    auto healId = gas.getEffectDef("HealOverTime")->id;
    auto attackId = gas.getAbilityDef("BasicAttack")->id;

    // Set up entity
    if (!gas.hasComponent(entity)) {
        gas.initializeComponent(entity);
    }
    gas.initializeAttribute(entity, healthId, 50.0f);
    gas.grantAbility(entity, attackId);

    printSubSection("Simulating game update loop");

    // Apply heal over time effect
    Entity healer{99};
    gas.applyEffect(entity, healId, healer);
    printInfo("Applied HealOverTime effect (+5 HP per second for 5 seconds)");

    // Activate ability to start cooldown
    gas.tryActivateAbility(entity, attackId);
    printInfo("Activated BasicAttack (1 second cooldown)");

    std::println("");
    printInfo("Simulating 6 seconds of game time with dt=1.0s...");
    std::println("");

    for (int i = 0; i < 6; ++i) {
        float dt = 1.0f;
        gas.update(dt);

        float health = gas.getAttributeValue(entity, healthId);
        float cooldown = gas.getAbilityCooldown(entity, attackId);
        bool hasHeal = gas.hasEffect(entity, healId);

        std::println("  Frame {} (t={}s):", i + 1, (i + 1));
        printValue("    Health", std::format("{:.1f}", health));
        printValue("    Attack cooldown", std::format("{:.1f}s", cooldown));
        printValue("    HealOverTime active", hasHeal ? "true" : "false");

        if (i == 4) {
            printInfo("    >> HealOverTime duration expired!");
        }
    }

    std::println("");
    printSuccess("Update loop demonstration complete");
    printInfo("The update() method processed:");
    printInfo("  - Periodic effect ticks (heal over time)");
    printInfo("  - Effect duration countdown and removal");
    printInfo("  - Ability cooldown reduction");

    std::println("");
}

//=============================================================================
// Utility Functions
//=============================================================================

void printSection(const std::string& title) {
    std::println("");
    std::println("=================================================================");
    std::println("{}", title);
    std::println("=================================================================");
    std::println("");
}

void printSubSection(const std::string& title) {
    std::println("");
    std::println("--- {} ---", title);
}

void printInfo(const std::string& message) {
    std::println("[INFO] {}", message);
}

void printSuccess(const std::string& message) {
    std::println("[SUCCESS] {}", message);
}

void printValue(const std::string& label, const std::string& value) {
    std::println("  {}: {}", label, value);
}

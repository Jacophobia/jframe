// tests/unit/GASSystemTests.cpp
// Unit tests for JFrame Gameplay Ability System

#include <cmath>
#include <limits>
#include <memory>
#include <string>
#include <vector>

#include <gtest/gtest.h>

import jframe.gas;
import jframe.gas.impl;
import jframe.types;

namespace jframe::tests {

class GASSystemTest : public ::testing::Test {
protected:
    void SetUp() override {
        gasSystem = createGASSystem();
        auto* impl = dynamic_cast<GASSystem*>(gasSystem.get());
        if (impl) {
            impl->initialize();
        }
    }

    std::unique_ptr<IGASSystem> gasSystem;
};

//=============================================================================
// Basic System Tests
//=============================================================================

TEST_F(GASSystemTest, CanCreateGASSystem) {
    EXPECT_NE(gasSystem, nullptr);
}

TEST_F(GASSystemTest, UpdateDoesNotCrashWithNoEntities) {
    gasSystem->update(1.0f / 60.0f);
}

//=============================================================================
// Tag Tests
//=============================================================================

TEST_F(GASSystemTest, RegisterTag) {
    GameplayTag tag = gasSystem->registerTag("State.Movement.Dashing");
    EXPECT_TRUE(tag.isValid());
    EXPECT_EQ(tag.name, "State.Movement.Dashing");
}

TEST_F(GASSystemTest, FindTag) {
    gasSystem->registerTag("State.Combat.Attacking");
    auto found = gasSystem->findTag("State.Combat.Attacking");
    EXPECT_TRUE(found.has_value());
    EXPECT_EQ(found->name, "State.Combat.Attacking");
}

TEST_F(GASSystemTest, FindTagNotFound) {
    auto found = gasSystem->findTag("NonExistent.Tag");
    EXPECT_FALSE(found.has_value());
}

TEST_F(GASSystemTest, DuplicateTagRegistration) {
    GameplayTag tag1 = gasSystem->registerTag("State.Buff.Speed");
    GameplayTag tag2 = gasSystem->registerTag("State.Buff.Speed");
    EXPECT_EQ(tag1.id, tag2.id);
}

TEST_F(GASSystemTest, TagHierarchyParent) {
    GameplayTag parent = gasSystem->registerTag("State.Movement");
    GameplayTag child = gasSystem->registerTag("State.Movement.Dashing");
    EXPECT_TRUE(gasSystem->isParentOf(parent, child));
}

TEST_F(GASSystemTest, TagHierarchyNotParent) {
    GameplayTag tag1 = gasSystem->registerTag("State.Movement");
    GameplayTag tag2 = gasSystem->registerTag("State.Combat");
    EXPECT_FALSE(gasSystem->isParentOf(tag1, tag2));
}

//=============================================================================
// Tag Container Tests
//=============================================================================

TEST_F(GASSystemTest, TagContainerAddRemove) {
    GameplayTagContainer container;
    GameplayTag tag = gasSystem->registerTag("Test.Tag");

    container.addTag(tag);
    EXPECT_TRUE(container.hasTag(tag));

    container.removeTag(tag);
    EXPECT_FALSE(container.hasTag(tag));
}

TEST_F(GASSystemTest, TagContainerHasAny) {
    GameplayTagContainer container1;
    GameplayTagContainer container2;

    GameplayTag tag1 = gasSystem->registerTag("Tag.One");
    GameplayTag tag2 = gasSystem->registerTag("Tag.Two");
    GameplayTag tag3 = gasSystem->registerTag("Tag.Three");

    container1.addTag(tag1);
    container1.addTag(tag2);

    container2.addTag(tag2);
    container2.addTag(tag3);

    EXPECT_TRUE(container1.hasAny(container2));
}

TEST_F(GASSystemTest, TagContainerHasAll) {
    GameplayTagContainer container1;
    GameplayTagContainer container2;

    GameplayTag tag1 = gasSystem->registerTag("Tag.A");
    GameplayTag tag2 = gasSystem->registerTag("Tag.B");

    container1.addTag(tag1);
    container1.addTag(tag2);

    container2.addTag(tag1);

    EXPECT_TRUE(container1.hasAll(container2));
    EXPECT_FALSE(container2.hasAll(container1));
}

TEST_F(GASSystemTest, TagContainerMatchesQuery) {
    GameplayTagContainer owned;
    GameplayTagContainer required;
    GameplayTagContainer blocked;

    GameplayTag goodTag = gasSystem->registerTag("State.Good");
    GameplayTag badTag = gasSystem->registerTag("State.Bad");

    owned.addTag(goodTag);
    required.addTag(goodTag);
    blocked.addTag(badTag);

    EXPECT_TRUE(owned.matchesQuery(required, blocked));

    owned.addTag(badTag);
    EXPECT_FALSE(owned.matchesQuery(required, blocked));
}

//=============================================================================
// Attribute Tests
//=============================================================================

TEST_F(GASSystemTest, RegisterAttribute) {
    AttributeDef def{
        .name = "Health",
        .baseValue = 100.0f,
        .minValue = 0.0f,
        .maxValue = 100.0f
    };

    AttributeId id = gasSystem->registerAttribute(def);
    EXPECT_NE(id, 0);

    auto retrievedDef = gasSystem->getAttributeDef(id);
    EXPECT_TRUE(retrievedDef.has_value());
    EXPECT_EQ(retrievedDef->name, "Health");
}

TEST_F(GASSystemTest, GetAttributeDefByName) {
    AttributeDef def{.name = "Mana", .baseValue = 50.0f};
    gasSystem->registerAttribute(def);

    auto retrieved = gasSystem->getAttributeDef("Mana");
    EXPECT_TRUE(retrieved.has_value());
    EXPECT_EQ(retrieved->baseValue, 50.0f);
}

TEST_F(GASSystemTest, InitializeAndGetAttribute) {
    Entity entity = static_cast<Entity>(1);
    gasSystem->initializeComponent(entity);

    AttributeDef def{.name = "Stamina", .baseValue = 100.0f};
    AttributeId id = gasSystem->registerAttribute(def);

    gasSystem->initializeAttribute(entity, id, 80.0f);

    EXPECT_FLOAT_EQ(gasSystem->getAttributeValue(entity, id), 80.0f);
    EXPECT_FLOAT_EQ(gasSystem->getAttributeBaseValue(entity, id), 80.0f);
}

TEST_F(GASSystemTest, ModifyAttribute) {
    Entity entity = static_cast<Entity>(2);
    gasSystem->initializeComponent(entity);

    AttributeDef def{.name = "Points", .baseValue = 0.0f};
    AttributeId id = gasSystem->registerAttribute(def);
    gasSystem->initializeAttribute(entity, id, 50.0f);

    gasSystem->modifyAttribute(entity, id, 10.0f);
    EXPECT_FLOAT_EQ(gasSystem->getAttributeValue(entity, id), 60.0f);

    gasSystem->modifyAttribute(entity, id, -20.0f);
    EXPECT_FLOAT_EQ(gasSystem->getAttributeValue(entity, id), 40.0f);
}

TEST_F(GASSystemTest, AttributeClamping) {
    Entity entity = static_cast<Entity>(3);
    gasSystem->initializeComponent(entity);

    AttributeDef def{
        .name = "ClampedAttr",
        .baseValue = 50.0f,
        .minValue = 0.0f,
        .maxValue = 100.0f,
        .clampEnabled = true
    };
    AttributeId id = gasSystem->registerAttribute(def);
    gasSystem->initializeAttribute(entity, id, 50.0f);

    gasSystem->modifyAttribute(entity, id, 1000.0f);
    EXPECT_FLOAT_EQ(gasSystem->getAttributeValue(entity, id), 100.0f);

    gasSystem->modifyAttribute(entity, id, -500.0f);
    EXPECT_FLOAT_EQ(gasSystem->getAttributeValue(entity, id), 0.0f);
}

TEST_F(GASSystemTest, AttributeChangeCallback) {
    Entity entity = static_cast<Entity>(4);
    gasSystem->initializeComponent(entity);

    AttributeDef def{.name = "WatchedAttr"};
    AttributeId id = gasSystem->registerAttribute(def);
    gasSystem->initializeAttribute(entity, id, 100.0f);

    bool callbackCalled = false;
    float capturedOldValue = 0.0f;
    float capturedNewValue = 0.0f;

    gasSystem->setAttributeChangeCallback([&](const AttributeChangeEvent& event) {
        callbackCalled = true;
        capturedOldValue = event.oldValue;
        capturedNewValue = event.newValue;
    });

    gasSystem->modifyAttribute(entity, id, -25.0f);

    EXPECT_TRUE(callbackCalled);
    EXPECT_FLOAT_EQ(capturedOldValue, 100.0f);
    EXPECT_FLOAT_EQ(capturedNewValue, 75.0f);
}

//=============================================================================
// Effect Tests
//=============================================================================

TEST_F(GASSystemTest, RegisterEffect) {
    EffectDef def{
        .name = "SpeedBoost",
        .durationType = EffectDurationType::Duration,
        .duration = 5.0f
    };

    EffectId id = gasSystem->registerEffect(def);
    EXPECT_NE(id, 0);

    auto retrieved = gasSystem->getEffectDef(id);
    EXPECT_TRUE(retrieved.has_value());
    EXPECT_EQ(retrieved->name, "SpeedBoost");
}

TEST_F(GASSystemTest, ApplyInstantEffect) {
    Entity entity = static_cast<Entity>(5);
    gasSystem->initializeComponent(entity);

    AttributeDef attrDef{.name = "Health"};
    AttributeId attrId = gasSystem->registerAttribute(attrDef);
    gasSystem->initializeAttribute(entity, attrId, 100.0f);

    EffectDef effectDef{
        .name = "Heal",
        .durationType = EffectDurationType::Instant,
        .modifiers = {{.attribute = attrId, .op = EffectModifierOp::Add, .value = 20.0f}}
    };
    EffectId effectId = gasSystem->registerEffect(effectDef);

    gasSystem->applyEffect(entity, effectId, entity);

    EXPECT_FLOAT_EQ(gasSystem->getAttributeValue(entity, attrId), 120.0f);
    EXPECT_FALSE(gasSystem->hasEffect(entity, effectId));  // Instant effects don't persist
}

TEST_F(GASSystemTest, ApplyDurationEffect) {
    Entity entity = static_cast<Entity>(6);
    gasSystem->initializeComponent(entity);

    AttributeDef attrDef{.name = "Speed"};
    AttributeId attrId = gasSystem->registerAttribute(attrDef);
    gasSystem->initializeAttribute(entity, attrId, 100.0f);

    EffectDef effectDef{
        .name = "Haste",
        .durationType = EffectDurationType::Duration,
        .duration = 2.0f,
        .modifiers = {{.attribute = attrId, .op = EffectModifierOp::Add, .value = 50.0f}}
    };
    EffectId effectId = gasSystem->registerEffect(effectDef);

    gasSystem->applyEffect(entity, effectId, entity);

    EXPECT_TRUE(gasSystem->hasEffect(entity, effectId));
    EXPECT_FLOAT_EQ(gasSystem->getAttributeValue(entity, attrId), 150.0f);
}

TEST_F(GASSystemTest, EffectExpiration) {
    Entity entity = static_cast<Entity>(7);
    gasSystem->initializeComponent(entity);

    AttributeDef attrDef{.name = "Power"};
    AttributeId attrId = gasSystem->registerAttribute(attrDef);
    gasSystem->initializeAttribute(entity, attrId, 100.0f);

    EffectDef effectDef{
        .name = "ShortBuff",
        .durationType = EffectDurationType::Duration,
        .duration = 1.0f,
        .modifiers = {{.attribute = attrId, .op = EffectModifierOp::Add, .value = 25.0f}}
    };
    EffectId effectId = gasSystem->registerEffect(effectDef);

    gasSystem->applyEffect(entity, effectId, entity);
    EXPECT_TRUE(gasSystem->hasEffect(entity, effectId));
    EXPECT_FLOAT_EQ(gasSystem->getAttributeValue(entity, attrId), 125.0f);

    // Update past duration
    gasSystem->update(1.5f);

    EXPECT_FALSE(gasSystem->hasEffect(entity, effectId));
    EXPECT_FLOAT_EQ(gasSystem->getAttributeValue(entity, attrId), 100.0f);
}

TEST_F(GASSystemTest, EffectWithGrantedTags) {
    Entity entity = static_cast<Entity>(8);
    gasSystem->initializeComponent(entity);

    GameplayTag dashingTag = gasSystem->registerTag("State.Dashing");

    EffectDef effectDef{
        .name = "DashBuff",
        .durationType = EffectDurationType::Infinite
    };
    effectDef.grantedTags.addTag(dashingTag);
    EffectId effectId = gasSystem->registerEffect(effectDef);

    EXPECT_FALSE(gasSystem->hasTag(entity, dashingTag));

    gasSystem->applyEffect(entity, effectId, entity);
    EXPECT_TRUE(gasSystem->hasTag(entity, dashingTag));

    gasSystem->removeEffect(entity, effectId);
    EXPECT_FALSE(gasSystem->hasTag(entity, dashingTag));
}

TEST_F(GASSystemTest, EffectStackable) {
    Entity entity = static_cast<Entity>(9);
    gasSystem->initializeComponent(entity);

    AttributeDef attrDef{.name = "Damage"};
    AttributeId attrId = gasSystem->registerAttribute(attrDef);
    gasSystem->initializeAttribute(entity, attrId, 100.0f);

    EffectDef effectDef{
        .name = "DamageStack",
        .durationType = EffectDurationType::Infinite,
        .modifiers = {{.attribute = attrId, .op = EffectModifierOp::Add, .value = 10.0f}},
        .stackable = true,
        .maxStacks = 3
    };
    EffectId effectId = gasSystem->registerEffect(effectDef);

    gasSystem->applyEffect(entity, effectId, entity);
    EXPECT_FLOAT_EQ(gasSystem->getAttributeValue(entity, attrId), 110.0f);

    gasSystem->applyEffect(entity, effectId, entity);
    EXPECT_FLOAT_EQ(gasSystem->getAttributeValue(entity, attrId), 120.0f);

    gasSystem->applyEffect(entity, effectId, entity);
    EXPECT_FLOAT_EQ(gasSystem->getAttributeValue(entity, attrId), 130.0f);

    // Max stacks reached
    gasSystem->applyEffect(entity, effectId, entity);
    EXPECT_FLOAT_EQ(gasSystem->getAttributeValue(entity, attrId), 130.0f);
}

//=============================================================================
// Ability Tests
//=============================================================================

TEST_F(GASSystemTest, RegisterAbility) {
    AbilityDef def{
        .name = "Dash",
        .activationPolicy = AbilityActivationPolicy::OnInputPressed,
        .cooldown = 2.0f
    };

    AbilityId id = gasSystem->registerAbility(def);
    EXPECT_NE(id, 0);

    auto retrieved = gasSystem->getAbilityDef(id);
    EXPECT_TRUE(retrieved.has_value());
    EXPECT_EQ(retrieved->name, "Dash");
}

TEST_F(GASSystemTest, GrantAndRemoveAbility) {
    Entity entity = static_cast<Entity>(10);
    gasSystem->initializeComponent(entity);

    AbilityDef def{.name = "Jump"};
    AbilityId id = gasSystem->registerAbility(def);

    EXPECT_FALSE(gasSystem->hasAbility(entity, id));

    gasSystem->grantAbility(entity, id);
    EXPECT_TRUE(gasSystem->hasAbility(entity, id));

    gasSystem->removeAbility(entity, id);
    EXPECT_FALSE(gasSystem->hasAbility(entity, id));
}

TEST_F(GASSystemTest, ActivateAbility) {
    Entity entity = static_cast<Entity>(11);
    gasSystem->initializeComponent(entity);

    AbilityDef def{.name = "Attack"};
    AbilityId id = gasSystem->registerAbility(def);
    gasSystem->grantAbility(entity, id);

    EXPECT_TRUE(gasSystem->canActivateAbility(entity, id));

    bool activated = gasSystem->tryActivateAbility(entity, id);
    EXPECT_TRUE(activated);
    EXPECT_TRUE(gasSystem->isAbilityActive(entity, id));
}

TEST_F(GASSystemTest, AbilityCooldown) {
    Entity entity = static_cast<Entity>(12);
    gasSystem->initializeComponent(entity);

    AbilityDef def{.name = "Fireball", .cooldown = 3.0f};
    AbilityId id = gasSystem->registerAbility(def);
    gasSystem->grantAbility(entity, id);

    gasSystem->tryActivateAbility(entity, id);
    gasSystem->endAbility(entity, id);

    EXPECT_FLOAT_EQ(gasSystem->getAbilityCooldown(entity, id), 3.0f);
    EXPECT_FALSE(gasSystem->canActivateAbility(entity, id));

    gasSystem->update(2.0f);
    EXPECT_FLOAT_EQ(gasSystem->getAbilityCooldown(entity, id), 1.0f);
    EXPECT_FALSE(gasSystem->canActivateAbility(entity, id));

    gasSystem->update(1.5f);
    EXPECT_TRUE(gasSystem->canActivateAbility(entity, id));
}

TEST_F(GASSystemTest, AbilityCost) {
    Entity entity = static_cast<Entity>(13);
    gasSystem->initializeComponent(entity);

    AttributeDef manaAttr{.name = "Mana"};
    AttributeId manaId = gasSystem->registerAttribute(manaAttr);
    gasSystem->initializeAttribute(entity, manaId, 50.0f);

    AbilityDef def{
        .name = "Spell",
        .costs = {{.attribute = manaId, .cost = 30.0f}}
    };
    AbilityId abilityId = gasSystem->registerAbility(def);
    gasSystem->grantAbility(entity, abilityId);

    EXPECT_TRUE(gasSystem->canActivateAbility(entity, abilityId));
    gasSystem->tryActivateAbility(entity, abilityId);
    gasSystem->endAbility(entity, abilityId);

    EXPECT_FLOAT_EQ(gasSystem->getAttributeValue(entity, manaId), 20.0f);

    // Not enough mana for second cast
    EXPECT_FALSE(gasSystem->canActivateAbility(entity, abilityId));
}

TEST_F(GASSystemTest, AbilityTagRequirements) {
    Entity entity = static_cast<Entity>(14);
    gasSystem->initializeComponent(entity);

    GameplayTag groundedTag = gasSystem->registerTag("State.Grounded");

    AbilityDef def{.name = "GroundSlam"};
    def.activationRequiredTags.addTag(groundedTag);
    AbilityId id = gasSystem->registerAbility(def);
    gasSystem->grantAbility(entity, id);

    // Can't use without required tag
    EXPECT_FALSE(gasSystem->canActivateAbility(entity, id));

    gasSystem->addTag(entity, groundedTag);
    EXPECT_TRUE(gasSystem->canActivateAbility(entity, id));
}

TEST_F(GASSystemTest, AbilityAppliesEffectOnActivation) {
    Entity entity = static_cast<Entity>(15);
    gasSystem->initializeComponent(entity);

    AttributeDef speedAttr{.name = "MoveSpeed"};
    AttributeId speedId = gasSystem->registerAttribute(speedAttr);
    gasSystem->initializeAttribute(entity, speedId, 100.0f);

    EffectDef dashEffect{
        .name = "DashSpeedBuff",
        .durationType = EffectDurationType::Infinite,
        .modifiers = {{.attribute = speedId, .op = EffectModifierOp::Multiply, .value = 2.0f}}
    };
    EffectId dashEffectId = gasSystem->registerEffect(dashEffect);

    AbilityDef dashAbility{
        .name = "Dash",
        .effectsToApplyOnActivate = {dashEffectId}
    };
    AbilityId dashId = gasSystem->registerAbility(dashAbility);
    gasSystem->grantAbility(entity, dashId);

    EXPECT_FLOAT_EQ(gasSystem->getAttributeValue(entity, speedId), 100.0f);

    gasSystem->tryActivateAbility(entity, dashId);

    EXPECT_FLOAT_EQ(gasSystem->getAttributeValue(entity, speedId), 200.0f);
}

//=============================================================================
// Component Management Tests
//=============================================================================

TEST_F(GASSystemTest, ComponentManagement) {
    Entity entity = static_cast<Entity>(16);

    EXPECT_FALSE(gasSystem->hasComponent(entity));

    gasSystem->initializeComponent(entity);
    EXPECT_TRUE(gasSystem->hasComponent(entity));

    gasSystem->removeComponent(entity);
    EXPECT_FALSE(gasSystem->hasComponent(entity));
}

TEST_F(GASSystemTest, EntityTags) {
    Entity entity = static_cast<Entity>(17);
    gasSystem->initializeComponent(entity);

    GameplayTag buffed = gasSystem->registerTag("State.Buffed");
    GameplayTag debuffed = gasSystem->registerTag("State.Debuffed");

    gasSystem->addTag(entity, buffed);
    EXPECT_TRUE(gasSystem->hasTag(entity, buffed));
    EXPECT_FALSE(gasSystem->hasTag(entity, debuffed));

    gasSystem->addTag(entity, debuffed);
    EXPECT_TRUE(gasSystem->hasTag(entity, debuffed));

    gasSystem->removeTag(entity, buffed);
    EXPECT_FALSE(gasSystem->hasTag(entity, buffed));
}

//=============================================================================
// Lua Integration Tests
//=============================================================================

TEST_F(GASSystemTest, LoadTagsFromLua) {
    const char* luaSource = R"(
        Tags = {
            "State.Movement.Running",
            "State.Movement.Jumping",
            "State.Combat.Attacking"
        }
    )";

    bool loaded = gasSystem->loadDefinitionsFromLua(luaSource);
    EXPECT_TRUE(loaded);

    auto runningTag = gasSystem->findTag("State.Movement.Running");
    EXPECT_TRUE(runningTag.has_value());

    auto jumpingTag = gasSystem->findTag("State.Movement.Jumping");
    EXPECT_TRUE(jumpingTag.has_value());
}

TEST_F(GASSystemTest, LoadAttributesFromLua) {
    const char* luaSource = R"(
        Attributes = {
            {
                name = "Health",
                baseValue = 100,
                minValue = 0,
                maxValue = 100
            },
            {
                name = "Mana",
                baseValue = 50,
                minValue = 0,
                maxValue = 200
            }
        }
    )";

    bool loaded = gasSystem->loadDefinitionsFromLua(luaSource);
    EXPECT_TRUE(loaded);

    auto healthDef = gasSystem->getAttributeDef("Health");
    EXPECT_TRUE(healthDef.has_value());
    EXPECT_FLOAT_EQ(healthDef->baseValue, 100.0f);
    EXPECT_FLOAT_EQ(healthDef->maxValue, 100.0f);

    auto manaDef = gasSystem->getAttributeDef("Mana");
    EXPECT_TRUE(manaDef.has_value());
    EXPECT_FLOAT_EQ(manaDef->baseValue, 50.0f);
}

TEST_F(GASSystemTest, LoadEffectsFromLua) {
    // First load attributes that the effect references
    const char* attrSource = R"(
        Attributes = {
            { name = "Health", baseValue = 100 }
        }
    )";
    gasSystem->loadDefinitionsFromLua(attrSource);

    const char* effectSource = R"(
        Effects = {
            {
                name = "SmallHeal",
                durationType = "instant",
                modifiers = {
                    { attribute = "Health", op = "add", value = 25 }
                }
            },
            {
                name = "Regeneration",
                durationType = "duration",
                duration = 10.0,
                period = 1.0,
                modifiers = {
                    { attribute = "Health", op = "add", value = 5 }
                }
            }
        }
    )";

    bool loaded = gasSystem->loadDefinitionsFromLua(effectSource);
    EXPECT_TRUE(loaded);

    auto healDef = gasSystem->getEffectDef("SmallHeal");
    EXPECT_TRUE(healDef.has_value());
    EXPECT_EQ(healDef->durationType, EffectDurationType::Instant);

    auto regenDef = gasSystem->getEffectDef("Regeneration");
    EXPECT_TRUE(regenDef.has_value());
    EXPECT_EQ(regenDef->durationType, EffectDurationType::Duration);
    EXPECT_FLOAT_EQ(regenDef->duration, 10.0f);
}

TEST_F(GASSystemTest, LoadAbilitiesFromLua) {
    const char* luaSource = R"(
        Abilities = {
            {
                name = "Dash",
                activationPolicy = "onInputPressed",
                cooldown = 2.0
            },
            {
                name = "Block",
                activationPolicy = "whileInputHeld",
                cooldown = 0
            }
        }
    )";

    bool loaded = gasSystem->loadDefinitionsFromLua(luaSource);
    EXPECT_TRUE(loaded);

    auto dashDef = gasSystem->getAbilityDef("Dash");
    EXPECT_TRUE(dashDef.has_value());
    EXPECT_EQ(dashDef->activationPolicy, AbilityActivationPolicy::OnInputPressed);
    EXPECT_FLOAT_EQ(dashDef->cooldown, 2.0f);

    auto blockDef = gasSystem->getAbilityDef("Block");
    EXPECT_TRUE(blockDef.has_value());
    EXPECT_EQ(blockDef->activationPolicy, AbilityActivationPolicy::WhileInputHeld);
}

}  // namespace jframe::tests

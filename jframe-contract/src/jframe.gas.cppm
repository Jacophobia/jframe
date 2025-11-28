// jframe-contract/src/jframe.gas.cppm
// Gameplay Ability System interface - tags, attributes, effects, abilities

module;

#include <functional>
#include <optional>
#include <vector>

export module jframe.gas;

import std;
import jframe.types;

export namespace jframe {

//=============================================================================
// Gameplay Tags - Hierarchical tag system for gameplay categorization
//=============================================================================

using GameplayTagId = std::uint32_t;

struct GameplayTag {
    GameplayTagId id = 0;
    std::string name;  // e.g., "State.Movement.Dashing"

    bool operator==(const GameplayTag&) const = default;
    bool isValid() const { return id != 0; }
    static GameplayTag invalid() { return {}; }
};

struct GameplayTagHash {
    std::size_t operator()(const GameplayTag& tag) const noexcept {
        return std::hash<GameplayTagId>{}(tag.id);
    }
};

class GameplayTagContainer {
public:
    void addTag(const GameplayTag& tag) {
        if (tag.isValid()) {
            tags_.insert(tag);
        }
    }

    void removeTag(const GameplayTag& tag) {
        tags_.erase(tag);
    }

    bool hasTag(const GameplayTag& tag) const {
        return tags_.contains(tag);
    }

    bool hasAny(const GameplayTagContainer& other) const {
        for (const auto& tag : other.tags_) {
            if (tags_.contains(tag)) return true;
        }
        return false;
    }

    bool hasAll(const GameplayTagContainer& other) const {
        for (const auto& tag : other.tags_) {
            if (!tags_.contains(tag)) return false;
        }
        return true;
    }

    bool matchesQuery(const GameplayTagContainer& required,
                      const GameplayTagContainer& blocked) const {
        // Must have all required tags
        if (!hasAll(required)) return false;
        // Must not have any blocked tags
        if (hasAny(blocked)) return false;
        return true;
    }

    void clear() { tags_.clear(); }
    bool empty() const { return tags_.empty(); }
    std::size_t count() const { return tags_.size(); }

    std::vector<GameplayTag> getTags() const {
        return std::vector<GameplayTag>(tags_.begin(), tags_.end());
    }

private:
    std::unordered_set<GameplayTag, GameplayTagHash> tags_;
};

//=============================================================================
// Gameplay Attributes - Numeric values with base/current tracking
//=============================================================================

using AttributeId = std::uint32_t;

struct AttributeDef {
    AttributeId id = 0;
    std::string name;      // e.g., "Health", "MoveSpeed", "JumpHeight"
    float baseValue = 0.0f;
    float minValue = 0.0f;
    float maxValue = std::numeric_limits<float>::max();
    bool clampEnabled = true;

    bool operator==(const AttributeDef&) const = default;
};

struct AttributeValue {
    float baseValue = 0.0f;
    float currentValue = 0.0f;

    void setBase(float value, float min = 0.0f, float max = std::numeric_limits<float>::max()) {
        baseValue = std::clamp(value, min, max);
    }

    void setCurrent(float value, float min = 0.0f, float max = std::numeric_limits<float>::max()) {
        currentValue = std::clamp(value, min, max);
    }

    void modify(float delta, float min = 0.0f, float max = std::numeric_limits<float>::max()) {
        currentValue = std::clamp(currentValue + delta, min, max);
    }
};

//=============================================================================
// Gameplay Effects - Modifiers applied to attributes
//=============================================================================

using EffectId = std::uint32_t;

enum class EffectDurationType : std::uint8_t {
    Instant,    // Apply once and remove
    Duration,   // Apply for a set time
    Infinite    // Apply until explicitly removed
};

enum class EffectModifierOp : std::uint8_t {
    Add,        // +value
    Multiply,   // *value
    Override    // =value
};

struct EffectModifier {
    AttributeId attribute;
    EffectModifierOp op = EffectModifierOp::Add;
    float value = 0.0f;
};

struct EffectDef {
    EffectId id = 0;
    std::string name;
    EffectDurationType durationType = EffectDurationType::Instant;
    float duration = 0.0f;  // Only used for Duration type
    float period = 0.0f;    // If > 0, effect ticks periodically (for DoT/HoT)
    std::vector<EffectModifier> modifiers;
    GameplayTagContainer grantedTags;      // Tags added while effect is active
    GameplayTagContainer applicationRequiredTags;  // Target must have these
    GameplayTagContainer applicationBlockedTags;   // Target must NOT have these
    GameplayTagContainer removalTags;      // Remove effect if target gains any of these
    bool stackable = false;
    int maxStacks = 1;
};

struct ActiveEffect {
    EffectId defId;
    float remainingDuration = 0.0f;
    float periodTimer = 0.0f;
    int stacks = 1;
    Entity source;  // Who applied this effect
};

//=============================================================================
// Gameplay Abilities - Activatable capabilities
//=============================================================================

using AbilityId = std::uint32_t;

enum class AbilityActivationPolicy : std::uint8_t {
    OnInputPressed,   // Activate when input is pressed
    OnInputReleased,  // Activate when input is released
    WhileInputHeld,   // Active while input is held
    Passive           // Always active (like an aura)
};

struct AbilityCost {
    AttributeId attribute;
    float cost = 0.0f;
};

struct AbilityDef {
    AbilityId id = 0;
    std::string name;
    AbilityActivationPolicy activationPolicy = AbilityActivationPolicy::OnInputPressed;
    float cooldown = 0.0f;
    std::vector<AbilityCost> costs;
    GameplayTagContainer activationRequiredTags;   // Owner must have these to activate
    GameplayTagContainer activationBlockedTags;    // Owner must NOT have these to activate
    GameplayTagContainer abilityTags;              // Tags this ability has (for canceling, etc.)
    GameplayTagContainer cancelAbilitiesWithTags;  // Cancel other abilities with these tags
    GameplayTagContainer blockAbilitiesWithTags;   // Block abilities with these tags while active
    std::vector<EffectId> effectsToApplyOnActivate;
    std::vector<EffectId> effectsToApplyOnEnd;
};

struct ActiveAbility {
    AbilityId defId;
    float cooldownRemaining = 0.0f;
    bool isActive = false;
};

//=============================================================================
// Ability System Component - Attached to entities
//=============================================================================

struct AbilitySystemComponent {
    GameplayTagContainer ownedTags;
    std::unordered_map<AttributeId, AttributeValue> attributes;
    std::vector<ActiveEffect> activeEffects;
    std::vector<AbilityId> grantedAbilities;
    std::unordered_map<AbilityId, ActiveAbility> abilityStates;
};

//=============================================================================
// Callbacks
//=============================================================================

struct AttributeChangeEvent {
    Entity entity;
    AttributeId attribute;
    float oldValue;
    float newValue;
};

struct EffectAppliedEvent {
    Entity target;
    Entity source;
    EffectId effect;
};

struct AbilityActivatedEvent {
    Entity entity;
    AbilityId ability;
};

using AttributeChangeCallback = std::function<void(const AttributeChangeEvent&)>;
using EffectAppliedCallback = std::function<void(const EffectAppliedEvent&)>;
using AbilityActivatedCallback = std::function<void(const AbilityActivatedEvent&)>;

//=============================================================================
// GAS System Interface
//=============================================================================

class IGASSystem {
public:
    virtual ~IGASSystem() = default;

    virtual void update(DeltaTime dt) = 0;

    //=========================================================================
    // Tag Registration
    //=========================================================================

    virtual GameplayTag registerTag(const std::string& name) = 0;
    virtual std::optional<GameplayTag> findTag(const std::string& name) const = 0;
    virtual bool isParentOf(const GameplayTag& parent, const GameplayTag& child) const = 0;

    //=========================================================================
    // Attribute Registration
    //=========================================================================

    virtual AttributeId registerAttribute(const AttributeDef& def) = 0;
    virtual std::optional<AttributeDef> getAttributeDef(AttributeId id) const = 0;
    virtual std::optional<AttributeDef> getAttributeDef(const std::string& name) const = 0;

    //=========================================================================
    // Effect Registration
    //=========================================================================

    virtual EffectId registerEffect(const EffectDef& def) = 0;
    virtual std::optional<EffectDef> getEffectDef(EffectId id) const = 0;
    virtual std::optional<EffectDef> getEffectDef(const std::string& name) const = 0;

    //=========================================================================
    // Ability Registration
    //=========================================================================

    virtual AbilityId registerAbility(const AbilityDef& def) = 0;
    virtual std::optional<AbilityDef> getAbilityDef(AbilityId id) const = 0;
    virtual std::optional<AbilityDef> getAbilityDef(const std::string& name) const = 0;

    //=========================================================================
    // Entity Component Management
    //=========================================================================

    virtual void initializeComponent(Entity entity) = 0;
    virtual void removeComponent(Entity entity) = 0;
    virtual bool hasComponent(Entity entity) const = 0;
    virtual AbilitySystemComponent* getComponent(Entity entity) = 0;
    virtual const AbilitySystemComponent* getComponent(Entity entity) const = 0;

    //=========================================================================
    // Tag Operations
    //=========================================================================

    virtual void addTag(Entity entity, const GameplayTag& tag) = 0;
    virtual void removeTag(Entity entity, const GameplayTag& tag) = 0;
    virtual bool hasTag(Entity entity, const GameplayTag& tag) const = 0;
    virtual const GameplayTagContainer* getTags(Entity entity) const = 0;

    //=========================================================================
    // Attribute Operations
    //=========================================================================

    virtual void initializeAttribute(Entity entity, AttributeId id, float baseValue) = 0;
    virtual float getAttributeValue(Entity entity, AttributeId id) const = 0;
    virtual float getAttributeBaseValue(Entity entity, AttributeId id) const = 0;
    virtual void setAttributeBaseValue(Entity entity, AttributeId id, float value) = 0;
    virtual void modifyAttribute(Entity entity, AttributeId id, float delta) = 0;

    //=========================================================================
    // Effect Operations
    //=========================================================================

    virtual void applyEffect(Entity target, EffectId effectId, Entity source) = 0;
    virtual void removeEffect(Entity entity, EffectId effectId) = 0;
    virtual void removeAllEffects(Entity entity) = 0;
    virtual bool hasEffect(Entity entity, EffectId effectId) const = 0;
    virtual std::vector<ActiveEffect> getActiveEffects(Entity entity) const = 0;

    //=========================================================================
    // Ability Operations
    //=========================================================================

    virtual void grantAbility(Entity entity, AbilityId abilityId) = 0;
    virtual void removeAbility(Entity entity, AbilityId abilityId) = 0;
    virtual bool hasAbility(Entity entity, AbilityId abilityId) const = 0;
    virtual bool canActivateAbility(Entity entity, AbilityId abilityId) const = 0;
    virtual bool tryActivateAbility(Entity entity, AbilityId abilityId) = 0;
    virtual void endAbility(Entity entity, AbilityId abilityId) = 0;
    virtual bool isAbilityActive(Entity entity, AbilityId abilityId) const = 0;
    virtual float getAbilityCooldown(Entity entity, AbilityId abilityId) const = 0;

    //=========================================================================
    // Callbacks
    //=========================================================================

    virtual void setAttributeChangeCallback(AttributeChangeCallback callback) = 0;
    virtual void setEffectAppliedCallback(EffectAppliedCallback callback) = 0;
    virtual void setAbilityActivatedCallback(AbilityActivatedCallback callback) = 0;

    //=========================================================================
    // Lua Integration
    //=========================================================================

    virtual bool loadDefinitionsFromLua(const std::string& luaSource) = 0;
};

}  // namespace jframe

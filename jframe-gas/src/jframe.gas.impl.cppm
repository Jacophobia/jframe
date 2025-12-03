// jframe-gas/src/jframe.gas.impl.cppm
// Gameplay Ability System implementation

module;

#include <sol/sol.hpp>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

export module jframe.gas.impl;
import jframe.gas;
import jframe.types;

export namespace jframe {

class GASSystem : public IGASSystem {
public:
    GASSystem();
    ~GASSystem() override;

    bool initialize();

    void update(DeltaTime dt) override;

    // Tag Registration
    GameplayTag registerTag(const std::string& name) override;
    std::optional<GameplayTag> findTag(const std::string& name) const override;
    bool isParentOf(const GameplayTag& parent, const GameplayTag& child) const override;

    // Attribute Registration
    AttributeId registerAttribute(const AttributeDef& def) override;
    std::optional<AttributeDef> getAttributeDef(AttributeId id) const override;
    std::optional<AttributeDef> getAttributeDef(const std::string& name) const override;

    // Effect Registration
    EffectId registerEffect(const EffectDef& def) override;
    std::optional<EffectDef> getEffectDef(EffectId id) const override;
    std::optional<EffectDef> getEffectDef(const std::string& name) const override;

    // Ability Registration
    AbilityId registerAbility(const AbilityDef& def) override;
    std::optional<AbilityDef> getAbilityDef(AbilityId id) const override;
    std::optional<AbilityDef> getAbilityDef(const std::string& name) const override;

    // Entity Component Management
    void initializeComponent(Entity entity) override;
    void removeComponent(Entity entity) override;
    bool hasComponent(Entity entity) const override;
    AbilitySystemComponent* getComponent(Entity entity) override;
    const AbilitySystemComponent* getComponent(Entity entity) const override;

    // Tag Operations
    void addTag(Entity entity, const GameplayTag& tag) override;
    void removeTag(Entity entity, const GameplayTag& tag) override;
    bool hasTag(Entity entity, const GameplayTag& tag) const override;
    const GameplayTagContainer* getTags(Entity entity) const override;

    // Attribute Operations
    void initializeAttribute(Entity entity, AttributeId id, float baseValue) override;
    float getAttributeValue(Entity entity, AttributeId id) const override;
    float getAttributeBaseValue(Entity entity, AttributeId id) const override;
    void setAttributeBaseValue(Entity entity, AttributeId id, float value) override;
    void modifyAttribute(Entity entity, AttributeId id, float delta) override;

    // Effect Operations
    void applyEffect(Entity target, EffectId effectId, Entity source) override;
    void removeEffect(Entity entity, EffectId effectId) override;
    void removeAllEffects(Entity entity) override;
    bool hasEffect(Entity entity, EffectId effectId) const override;
    std::vector<ActiveEffect> getActiveEffects(Entity entity) const override;

    // Ability Operations
    void grantAbility(Entity entity, AbilityId abilityId) override;
    void removeAbility(Entity entity, AbilityId abilityId) override;
    bool hasAbility(Entity entity, AbilityId abilityId) const override;
    bool canActivateAbility(Entity entity, AbilityId abilityId) const override;
    bool tryActivateAbility(Entity entity, AbilityId abilityId) override;
    void endAbility(Entity entity, AbilityId abilityId) override;
    bool isAbilityActive(Entity entity, AbilityId abilityId) const override;
    float getAbilityCooldown(Entity entity, AbilityId abilityId) const override;

    // Callbacks
    void setAttributeChangeCallback(AttributeChangeCallback callback) override;
    void setEffectAppliedCallback(EffectAppliedCallback callback) override;
    void setAbilityActivatedCallback(AbilityActivatedCallback callback) override;

    // Lua Integration
    bool loadDefinitionsFromLua(const std::string& luaSource) override;

private:
    void updateEffects(DeltaTime dt);
    void updateAbilityCooldowns(DeltaTime dt);
    void applyEffectModifiers(Entity entity, const EffectDef& def, int stacks);
    void removeEffectModifiers(Entity entity, const EffectDef& def, int stacks);
    void recalculateAttribute(Entity entity, AttributeId id);
    bool checkCosts(Entity entity, const AbilityDef& def) const;
    void payCosts(Entity entity, const AbilityDef& def);
    void notifyAttributeChange(Entity entity, AttributeId id, float oldValue, float newValue);

    // Tag parsing helper
    GameplayTagContainer parseTagContainer(const sol::table& table);

    // Registration data
    std::unordered_map<std::string, GameplayTag> tagsByName_;
    std::unordered_map<GameplayTagId, std::string> tagsById_;
    GameplayTagId nextTagId_ = 1;

    std::unordered_map<AttributeId, AttributeDef> attributeDefs_;
    std::unordered_map<std::string, AttributeId> attributesByName_;
    AttributeId nextAttributeId_ = 1;

    std::unordered_map<EffectId, EffectDef> effectDefs_;
    std::unordered_map<std::string, EffectId> effectsByName_;
    EffectId nextEffectId_ = 1;

    std::unordered_map<AbilityId, AbilityDef> abilityDefs_;
    std::unordered_map<std::string, AbilityId> abilitiesByName_;
    AbilityId nextAbilityId_ = 1;

    // Entity components
    std::unordered_map<std::uint32_t, AbilitySystemComponent> components_;

    // Callbacks
    AttributeChangeCallback attributeChangeCallback_;
    EffectAppliedCallback effectAppliedCallback_;
    AbilityActivatedCallback abilityActivatedCallback_;

    // Lua state for loading definitions
    sol::state lua_;
    bool initialized_ = false;
};

// Factory function
inline std::unique_ptr<IGASSystem> createGASSystem() {
    return std::make_unique<GASSystem>();
}

}  // namespace jframe

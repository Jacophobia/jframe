// bestow-gas/src/GASSystem.cpp
// Core GAS system implementation

module;

#include <bestow/sol2_compat.hpp>

module bestow.gas.impl;

import std;
import bestow.gas;
import bestow.types;

namespace bestow {

GASSystem::GASSystem() = default;

GASSystem::~GASSystem() = default;

bool GASSystem::initialize() {
    if (initialized_) return true;

    // Setup Lua state with sandboxing
    lua_.open_libraries(sol::lib::base, sol::lib::table, sol::lib::string, sol::lib::math);

    // Remove dangerous functions
    lua_["os"] = sol::nil;
    lua_["io"] = sol::nil;
    lua_["loadfile"] = sol::nil;
    lua_["dofile"] = sol::nil;
    lua_["load"] = sol::nil;
    lua_["require"] = sol::nil;
    lua_["package"] = sol::nil;
    lua_["debug"] = sol::nil;
    lua_["rawget"] = sol::nil;
    lua_["rawset"] = sol::nil;
    lua_["collectgarbage"] = sol::nil;

    initialized_ = true;
    return true;
}

void GASSystem::update(DeltaTime dt) {
    updateEffects(dt);
    updateAbilityCooldowns(dt);
}

void GASSystem::updateEffects(DeltaTime dt) {
    for (auto& [entityKey, component] : components_) {
        Entity entity = static_cast<Entity>(entityKey);
        std::vector<EffectId> effectsToRemove;

        for (auto& effect : component.activeEffects) {
            auto defIt = effectDefs_.find(effect.defId);
            if (defIt == effectDefs_.end()) continue;

            const EffectDef& def = defIt->second;

            // Handle periodic effects
            if (def.period > 0.0f) {
                effect.periodTimer += dt;
                while (effect.periodTimer >= def.period) {
                    effect.periodTimer -= def.period;
                    // Apply modifiers for this tick
                    for (const auto& mod : def.modifiers) {
                        if (mod.op == EffectModifierOp::Add) {
                            modifyAttribute(entity, mod.attribute, mod.value * effect.stacks);
                        }
                    }
                }
            }

            // Handle duration
            if (def.durationType == EffectDurationType::Duration) {
                effect.remainingDuration -= dt;
                if (effect.remainingDuration <= 0.0f) {
                    effectsToRemove.push_back(effect.defId);
                }
            }

            // Check removal tags
            if (!def.removalTags.empty()) {
                if (component.ownedTags.hasAny(def.removalTags)) {
                    effectsToRemove.push_back(effect.defId);
                }
            }
        }

        // Remove expired effects
        for (EffectId id : effectsToRemove) {
            removeEffect(entity, id);
        }
    }
}

void GASSystem::updateAbilityCooldowns(DeltaTime dt) {
    for (auto& [entityKey, component] : components_) {
        for (auto& [abilityId, state] : component.abilityStates) {
            if (state.cooldownRemaining > 0.0f) {
                state.cooldownRemaining -= dt;
                if (state.cooldownRemaining < 0.0f) {
                    state.cooldownRemaining = 0.0f;
                }
            }
        }
    }
}

void GASSystem::initializeComponent(Entity entity) {
    auto key = static_cast<std::uint32_t>(entity);
    if (components_.find(key) == components_.end()) {
        components_[key] = AbilitySystemComponent{};
    }
}

void GASSystem::removeComponent(Entity entity) {
    auto key = static_cast<std::uint32_t>(entity);
    components_.erase(key);
}

bool GASSystem::hasComponent(Entity entity) const {
    auto key = static_cast<std::uint32_t>(entity);
    return components_.find(key) != components_.end();
}

AbilitySystemComponent* GASSystem::getComponent(Entity entity) {
    auto key = static_cast<std::uint32_t>(entity);
    auto it = components_.find(key);
    return it != components_.end() ? &it->second : nullptr;
}

const AbilitySystemComponent* GASSystem::getComponent(Entity entity) const {
    auto key = static_cast<std::uint32_t>(entity);
    auto it = components_.find(key);
    return it != components_.end() ? &it->second : nullptr;
}

void GASSystem::applyEffectModifiers(Entity entity, const EffectDef& def, int stacks) {
    auto* comp = getComponent(entity);
    if (!comp) return;

    for (const auto& mod : def.modifiers) {
        auto attrIt = comp->attributes.find(mod.attribute);
        if (attrIt == comp->attributes.end()) continue;

        auto defIt = attributeDefs_.find(mod.attribute);
        float minVal = defIt != attributeDefs_.end() ? defIt->second.minValue : 0.0f;
        float maxVal = defIt != attributeDefs_.end() ? defIt->second.maxValue : std::numeric_limits<float>::max();

        float oldValue = attrIt->second.currentValue;

        switch (mod.op) {
            case EffectModifierOp::Add:
                attrIt->second.modify(mod.value * stacks, minVal, maxVal);
                break;
            case EffectModifierOp::Multiply:
                attrIt->second.setCurrent(
                    attrIt->second.currentValue * std::pow(mod.value, stacks), minVal, maxVal);
                break;
            case EffectModifierOp::Override:
                attrIt->second.setCurrent(mod.value, minVal, maxVal);
                break;
        }

        if (oldValue != attrIt->second.currentValue) {
            notifyAttributeChange(entity, mod.attribute, oldValue, attrIt->second.currentValue);
        }
    }
}

void GASSystem::removeEffectModifiers(Entity entity, const EffectDef& def, int stacks) {
    auto* comp = getComponent(entity);
    if (!comp) return;

    for (const auto& mod : def.modifiers) {
        auto attrIt = comp->attributes.find(mod.attribute);
        if (attrIt == comp->attributes.end()) continue;

        auto defIt = attributeDefs_.find(mod.attribute);
        float minVal = defIt != attributeDefs_.end() ? defIt->second.minValue : 0.0f;
        float maxVal = defIt != attributeDefs_.end() ? defIt->second.maxValue : std::numeric_limits<float>::max();

        float oldValue = attrIt->second.currentValue;

        switch (mod.op) {
            case EffectModifierOp::Add:
                attrIt->second.modify(-mod.value * stacks, minVal, maxVal);
                break;
            case EffectModifierOp::Multiply:
                if (mod.value != 0.0f) {
                    attrIt->second.setCurrent(
                        attrIt->second.currentValue / std::pow(mod.value, stacks), minVal, maxVal);
                }
                break;
            case EffectModifierOp::Override:
                // Revert to base for override
                attrIt->second.setCurrent(attrIt->second.baseValue, minVal, maxVal);
                break;
        }

        if (oldValue != attrIt->second.currentValue) {
            notifyAttributeChange(entity, mod.attribute, oldValue, attrIt->second.currentValue);
        }
    }
}

void GASSystem::recalculateAttribute(Entity entity, AttributeId id) {
    auto* comp = getComponent(entity);
    if (!comp) return;

    auto attrIt = comp->attributes.find(id);
    if (attrIt == comp->attributes.end()) return;

    // Start from base value
    float newValue = attrIt->second.baseValue;

    // Apply all active effect modifiers in order
    for (const auto& effect : comp->activeEffects) {
        auto defIt = effectDefs_.find(effect.defId);
        if (defIt == effectDefs_.end()) continue;

        for (const auto& mod : defIt->second.modifiers) {
            if (mod.attribute != id) continue;

            switch (mod.op) {
                case EffectModifierOp::Add:
                    newValue += mod.value * effect.stacks;
                    break;
                case EffectModifierOp::Multiply:
                    newValue *= std::pow(mod.value, effect.stacks);
                    break;
                case EffectModifierOp::Override:
                    newValue = mod.value;
                    break;
            }
        }
    }

    auto defIt = attributeDefs_.find(id);
    float minVal = defIt != attributeDefs_.end() ? defIt->second.minValue : 0.0f;
    float maxVal = defIt != attributeDefs_.end() ? defIt->second.maxValue : std::numeric_limits<float>::max();

    float oldValue = attrIt->second.currentValue;
    attrIt->second.setCurrent(newValue, minVal, maxVal);

    if (oldValue != attrIt->second.currentValue) {
        notifyAttributeChange(entity, id, oldValue, attrIt->second.currentValue);
    }
}

bool GASSystem::checkCosts(Entity entity, const AbilityDef& def) const {
    const auto* comp = getComponent(entity);
    if (!comp) return false;

    for (const auto& cost : def.costs) {
        auto attrIt = comp->attributes.find(cost.attribute);
        if (attrIt == comp->attributes.end()) return false;
        if (attrIt->second.currentValue < cost.cost) return false;
    }
    return true;
}

void GASSystem::payCosts(Entity entity, const AbilityDef& def) {
    for (const auto& cost : def.costs) {
        modifyAttribute(entity, cost.attribute, -cost.cost);
    }
}

void GASSystem::notifyAttributeChange(Entity entity, AttributeId id, float oldValue, float newValue) {
    if (attributeChangeCallback_) {
        AttributeChangeEvent event{
            .entity = entity,
            .attribute = id,
            .oldValue = oldValue,
            .newValue = newValue
        };
        attributeChangeCallback_(event);
    }
}

void GASSystem::setAttributeChangeCallback(AttributeChangeCallback callback) {
    attributeChangeCallback_ = std::move(callback);
}

void GASSystem::setEffectAppliedCallback(EffectAppliedCallback callback) {
    effectAppliedCallback_ = std::move(callback);
}

void GASSystem::setAbilityActivatedCallback(AbilityActivatedCallback callback) {
    abilityActivatedCallback_ = std::move(callback);
}

}  // namespace bestow

// jframe-gas/src/GameplayEffects.cpp
// Gameplay effect implementation

module jframe.gas.impl;

import std;
import jframe.gas;
import jframe.types;

namespace jframe {

//=============================================================================
// GASSystem Effect Registration
//=============================================================================

EffectId GASSystem::registerEffect(const EffectDef& def) {
    // Check if already registered by name
    auto nameIt = effectsByName_.find(def.name);
    if (nameIt != effectsByName_.end()) {
        return nameIt->second;
    }

    // Create with new ID or use provided
    EffectDef newDef = def;
    if (newDef.id == 0) {
        newDef.id = nextEffectId_++;
    } else if (newDef.id >= nextEffectId_) {
        nextEffectId_ = newDef.id + 1;
    }

    effectDefs_[newDef.id] = newDef;
    effectsByName_[newDef.name] = newDef.id;

    return newDef.id;
}

std::optional<EffectDef> GASSystem::getEffectDef(EffectId id) const {
    auto it = effectDefs_.find(id);
    if (it != effectDefs_.end()) {
        return it->second;
    }
    return std::nullopt;
}

std::optional<EffectDef> GASSystem::getEffectDef(const std::string& name) const {
    auto nameIt = effectsByName_.find(name);
    if (nameIt != effectsByName_.end()) {
        return getEffectDef(nameIt->second);
    }
    return std::nullopt;
}

//=============================================================================
// GASSystem Effect Operations
//=============================================================================

void GASSystem::applyEffect(Entity target, EffectId effectId, Entity source) {
    auto* comp = getComponent(target);
    if (!comp) return;

    auto defIt = effectDefs_.find(effectId);
    if (defIt == effectDefs_.end()) return;

    const EffectDef& def = defIt->second;

    // Check application requirements
    if (!comp->ownedTags.matchesQuery(def.applicationRequiredTags, def.applicationBlockedTags)) {
        return;
    }

    // Handle instant effects
    if (def.durationType == EffectDurationType::Instant) {
        // Apply modifiers immediately
        for (const auto& mod : def.modifiers) {
            auto attrIt = comp->attributes.find(mod.attribute);
            if (attrIt == comp->attributes.end()) continue;

            auto attrDefIt = attributeDefs_.find(mod.attribute);
            float minVal = attrDefIt != attributeDefs_.end() ? attrDefIt->second.minValue : 0.0f;
            float maxVal = attrDefIt != attributeDefs_.end() ? attrDefIt->second.maxValue : std::numeric_limits<float>::max();

            float oldValue = attrIt->second.currentValue;

            switch (mod.op) {
                case EffectModifierOp::Add:
                    attrIt->second.modify(mod.value, minVal, maxVal);
                    break;
                case EffectModifierOp::Multiply:
                    attrIt->second.setCurrent(attrIt->second.currentValue * mod.value, minVal, maxVal);
                    break;
                case EffectModifierOp::Override:
                    attrIt->second.setCurrent(mod.value, minVal, maxVal);
                    break;
            }

            if (oldValue != attrIt->second.currentValue) {
                notifyAttributeChange(target, mod.attribute, oldValue, attrIt->second.currentValue);
            }
        }

        // Notify callback
        if (effectAppliedCallback_) {
            EffectAppliedEvent event{
                .target = target,
                .source = source,
                .effect = effectId
            };
            effectAppliedCallback_(event);
        }

        return;
    }

    // Handle duration/infinite effects
    // Check for stacking
    if (def.stackable) {
        for (auto& existing : comp->activeEffects) {
            if (existing.defId == effectId) {
                // Add stack
                if (existing.stacks < def.maxStacks) {
                    int oldStacks = existing.stacks;
                    existing.stacks++;

                    // Reset duration if applicable
                    if (def.durationType == EffectDurationType::Duration) {
                        existing.remainingDuration = def.duration;
                    }

                    // Apply additional modifier for new stack
                    applyEffectModifiers(target, def, 1);

                    // Notify callback
                    if (effectAppliedCallback_) {
                        EffectAppliedEvent event{
                            .target = target,
                            .source = source,
                            .effect = effectId
                        };
                        effectAppliedCallback_(event);
                    }
                }
                return;
            }
        }
    } else {
        // Non-stackable - check if already has effect
        for (const auto& existing : comp->activeEffects) {
            if (existing.defId == effectId) {
                return;  // Already has effect, don't apply again
            }
        }
    }

    // Create new active effect
    ActiveEffect newEffect{
        .defId = effectId,
        .remainingDuration = def.durationType == EffectDurationType::Duration ? def.duration : 0.0f,
        .periodTimer = 0.0f,
        .stacks = 1,
        .source = source
    };

    comp->activeEffects.push_back(newEffect);

    // Apply granted tags
    for (const auto& tag : def.grantedTags.getTags()) {
        comp->ownedTags.addTag(tag);
    }

    // Apply modifiers (only for non-periodic effects - periodic effects apply on tick)
    if (def.period <= 0.0f) {
        applyEffectModifiers(target, def, 1);
    }

    // Notify callback
    if (effectAppliedCallback_) {
        EffectAppliedEvent event{
            .target = target,
            .source = source,
            .effect = effectId
        };
        effectAppliedCallback_(event);
    }
}

void GASSystem::removeEffect(Entity entity, EffectId effectId) {
    auto* comp = getComponent(entity);
    if (!comp) return;

    auto defIt = effectDefs_.find(effectId);
    if (defIt == effectDefs_.end()) return;

    const EffectDef& def = defIt->second;

    // Find and remove the effect
    for (auto it = comp->activeEffects.begin(); it != comp->activeEffects.end(); ++it) {
        if (it->defId == effectId) {
            // Remove granted tags
            for (const auto& tag : def.grantedTags.getTags()) {
                comp->ownedTags.removeTag(tag);
            }

            // Remove modifiers
            removeEffectModifiers(entity, def, it->stacks);

            comp->activeEffects.erase(it);
            return;
        }
    }
}

void GASSystem::removeAllEffects(Entity entity) {
    auto* comp = getComponent(entity);
    if (!comp) return;

    // Remove all effects in reverse order
    while (!comp->activeEffects.empty()) {
        removeEffect(entity, comp->activeEffects.back().defId);
    }
}

bool GASSystem::hasEffect(Entity entity, EffectId effectId) const {
    const auto* comp = getComponent(entity);
    if (!comp) return false;

    for (const auto& effect : comp->activeEffects) {
        if (effect.defId == effectId) {
            return true;
        }
    }
    return false;
}

std::vector<ActiveEffect> GASSystem::getActiveEffects(Entity entity) const {
    const auto* comp = getComponent(entity);
    if (!comp) return {};
    return comp->activeEffects;
}

}  // namespace jframe

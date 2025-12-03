// jframe-gas/src/GameplayAbilities.cpp
// Gameplay ability implementation

module;

#include <algorithm>
#include <compare>
#include <optional>
#include <string>

module jframe.gas.impl;

import jframe.gas;
import jframe.types;

namespace jframe {

//=============================================================================
// GASSystem Ability Registration
//=============================================================================

AbilityId GASSystem::registerAbility(const AbilityDef& def) {
    // Check if already registered by name
    auto nameIt = abilitiesByName_.find(def.name);
    if (nameIt != abilitiesByName_.end()) {
        return nameIt->second;
    }

    // Create with new ID or use provided
    AbilityDef newDef = def;
    if (newDef.id == 0) {
        newDef.id = nextAbilityId_++;
    } else if (newDef.id >= nextAbilityId_) {
        nextAbilityId_ = newDef.id + 1;
    }

    abilityDefs_[newDef.id] = newDef;
    abilitiesByName_[newDef.name] = newDef.id;

    return newDef.id;
}

std::optional<AbilityDef> GASSystem::getAbilityDef(AbilityId id) const {
    auto it = abilityDefs_.find(id);
    if (it != abilityDefs_.end()) {
        return it->second;
    }
    return std::nullopt;
}

std::optional<AbilityDef> GASSystem::getAbilityDef(const std::string& name) const {
    auto nameIt = abilitiesByName_.find(name);
    if (nameIt != abilitiesByName_.end()) {
        return getAbilityDef(nameIt->second);
    }
    return std::nullopt;
}

//=============================================================================
// GASSystem Ability Operations
//=============================================================================

void GASSystem::grantAbility(Entity entity, AbilityId abilityId) {
    auto* comp = getComponent(entity);
    if (!comp) return;

    // Check if already granted
    for (AbilityId id : comp->grantedAbilities) {
        if (id == abilityId) return;
    }

    comp->grantedAbilities.push_back(abilityId);

    // Initialize ability state
    comp->abilityStates[abilityId] = ActiveAbility{
        .defId = abilityId,
        .cooldownRemaining = 0.0f,
        .isActive = false
    };
}

void GASSystem::removeAbility(Entity entity, AbilityId abilityId) {
    auto* comp = getComponent(entity);
    if (!comp) return;

    // End ability if active
    if (isAbilityActive(entity, abilityId)) {
        endAbility(entity, abilityId);
    }

    // Remove from granted list
    auto& abilities = comp->grantedAbilities;
    abilities.erase(std::remove(abilities.begin(), abilities.end(), abilityId), abilities.end());

    // Remove state
    comp->abilityStates.erase(abilityId);
}

bool GASSystem::hasAbility(Entity entity, AbilityId abilityId) const {
    const auto* comp = getComponent(entity);
    if (!comp) return false;

    for (AbilityId id : comp->grantedAbilities) {
        if (id == abilityId) return true;
    }
    return false;
}

bool GASSystem::canActivateAbility(Entity entity, AbilityId abilityId) const {
    const auto* comp = getComponent(entity);
    if (!comp) return false;

    // Must have the ability
    if (!hasAbility(entity, abilityId)) return false;

    auto defIt = abilityDefs_.find(abilityId);
    if (defIt == abilityDefs_.end()) return false;

    const AbilityDef& def = defIt->second;
    auto stateIt = comp->abilityStates.find(abilityId);
    if (stateIt == comp->abilityStates.end()) return false;

    const ActiveAbility& state = stateIt->second;

    // Check if on cooldown
    if (state.cooldownRemaining > 0.0f) return false;

    // Check if already active (for non-toggle abilities)
    if (state.isActive && def.activationPolicy != AbilityActivationPolicy::WhileInputHeld) {
        return false;
    }

    // Check tag requirements
    if (!comp->ownedTags.matchesQuery(def.activationRequiredTags, def.activationBlockedTags)) {
        return false;
    }

    // Check if blocked by another ability's tags
    for (const auto& [otherId, otherState] : comp->abilityStates) {
        if (!otherState.isActive || otherId == abilityId) continue;

        auto otherDefIt = abilityDefs_.find(otherId);
        if (otherDefIt == abilityDefs_.end()) continue;

        // Check if this ability's tags are blocked
        if (def.abilityTags.hasAny(otherDefIt->second.blockAbilitiesWithTags)) {
            return false;
        }
    }

    // Check costs
    if (!checkCosts(entity, def)) return false;

    return true;
}

bool GASSystem::tryActivateAbility(Entity entity, AbilityId abilityId) {
    if (!canActivateAbility(entity, abilityId)) {
        return false;
    }

    auto* comp = getComponent(entity);
    if (!comp) return false;

    auto defIt = abilityDefs_.find(abilityId);
    if (defIt == abilityDefs_.end()) return false;

    const AbilityDef& def = defIt->second;
    auto stateIt = comp->abilityStates.find(abilityId);
    if (stateIt == comp->abilityStates.end()) return false;

    // Cancel abilities with matching tags
    for (auto& [otherId, otherState] : comp->abilityStates) {
        if (!otherState.isActive || otherId == abilityId) continue;

        auto otherDefIt = abilityDefs_.find(otherId);
        if (otherDefIt == abilityDefs_.end()) continue;

        if (otherDefIt->second.abilityTags.hasAny(def.cancelAbilitiesWithTags)) {
            endAbility(entity, otherId);
        }
    }

    // Pay costs
    payCosts(entity, def);

    // Mark as active
    stateIt->second.isActive = true;

    // Apply activation effects
    for (EffectId effectId : def.effectsToApplyOnActivate) {
        applyEffect(entity, effectId, entity);
    }

    // Notify callback
    if (abilityActivatedCallback_) {
        AbilityActivatedEvent event{
            .entity = entity,
            .ability = abilityId
        };
        abilityActivatedCallback_(event);
    }

    return true;
}

void GASSystem::endAbility(Entity entity, AbilityId abilityId) {
    auto* comp = getComponent(entity);
    if (!comp) return;

    auto stateIt = comp->abilityStates.find(abilityId);
    if (stateIt == comp->abilityStates.end()) return;

    if (!stateIt->second.isActive) return;

    auto defIt = abilityDefs_.find(abilityId);
    if (defIt == abilityDefs_.end()) return;

    const AbilityDef& def = defIt->second;

    // Apply end effects
    for (EffectId effectId : def.effectsToApplyOnEnd) {
        applyEffect(entity, effectId, entity);
    }

    // Start cooldown
    stateIt->second.cooldownRemaining = def.cooldown;
    stateIt->second.isActive = false;
}

bool GASSystem::isAbilityActive(Entity entity, AbilityId abilityId) const {
    const auto* comp = getComponent(entity);
    if (!comp) return false;

    auto stateIt = comp->abilityStates.find(abilityId);
    if (stateIt == comp->abilityStates.end()) return false;

    return stateIt->second.isActive;
}

float GASSystem::getAbilityCooldown(Entity entity, AbilityId abilityId) const {
    const auto* comp = getComponent(entity);
    if (!comp) return 0.0f;

    auto stateIt = comp->abilityStates.find(abilityId);
    if (stateIt == comp->abilityStates.end()) return 0.0f;

    return stateIt->second.cooldownRemaining;
}

}  // namespace jframe

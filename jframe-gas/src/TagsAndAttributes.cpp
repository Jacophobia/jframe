// jframe-gas/src/TagsAndAttributes.cpp
// Tag and attribute operations for GAS

module;

#include <compare>
#include <limits>
#include <optional>
#include <string>

module jframe.gas.impl;

import jframe.gas;
import jframe.types;

namespace jframe {

//=============================================================================
// Tag Registration
//=============================================================================

GameplayTag GASSystem::registerTag(const std::string& name) {
    // Check if already registered
    auto it = tagsByName_.find(name);
    if (it != tagsByName_.end()) {
        return it->second;
    }

    // Create new tag
    GameplayTag tag;
    tag.id = nextTagId_++;
    tag.name = name;

    tagsByName_[name] = tag;
    tagsById_[tag.id] = name;

    return tag;
}

std::optional<GameplayTag> GASSystem::findTag(const std::string& name) const {
    auto it = tagsByName_.find(name);
    if (it != tagsByName_.end()) {
        return it->second;
    }
    return std::nullopt;
}

bool GASSystem::isParentOf(const GameplayTag& parent, const GameplayTag& child) const {
    // Tags are hierarchical with dot-separated names
    // e.g., "State.Movement" is parent of "State.Movement.Dashing"
    if (!parent.isValid() || !child.isValid()) return false;
    if (parent.name == child.name) return false;

    // Check if child's name starts with parent's name followed by a dot
    return child.name.starts_with(parent.name + ".");
}

//=============================================================================
// Tag Operations on Entities
//=============================================================================

void GASSystem::addTag(Entity entity, const GameplayTag& tag) {
    auto* comp = getComponent(entity);
    if (comp && tag.isValid()) {
        comp->ownedTags.addTag(tag);
    }
}

void GASSystem::removeTag(Entity entity, const GameplayTag& tag) {
    auto* comp = getComponent(entity);
    if (comp) {
        comp->ownedTags.removeTag(tag);
    }
}

bool GASSystem::hasTag(Entity entity, const GameplayTag& tag) const {
    const auto* comp = getComponent(entity);
    if (!comp) return false;
    return comp->ownedTags.hasTag(tag);
}

const GameplayTagContainer* GASSystem::getTags(Entity entity) const {
    const auto* comp = getComponent(entity);
    return comp ? &comp->ownedTags : nullptr;
}

//=============================================================================
// Attribute Registration
//=============================================================================

AttributeId GASSystem::registerAttribute(const AttributeDef& def) {
    // Check if already registered by name
    auto it = attributesByName_.find(def.name);
    if (it != attributesByName_.end()) {
        return it->second;
    }

    // Create new attribute def with assigned ID
    AttributeDef newDef = def;
    newDef.id = nextAttributeId_++;

    attributeDefs_[newDef.id] = newDef;
    attributesByName_[newDef.name] = newDef.id;

    return newDef.id;
}

std::optional<AttributeDef> GASSystem::getAttributeDef(AttributeId id) const {
    auto it = attributeDefs_.find(id);
    if (it != attributeDefs_.end()) {
        return it->second;
    }
    return std::nullopt;
}

std::optional<AttributeDef> GASSystem::getAttributeDef(const std::string& name) const {
    auto nameIt = attributesByName_.find(name);
    if (nameIt != attributesByName_.end()) {
        auto defIt = attributeDefs_.find(nameIt->second);
        if (defIt != attributeDefs_.end()) {
            return defIt->second;
        }
    }
    return std::nullopt;
}

//=============================================================================
// Attribute Operations on Entities
//=============================================================================

void GASSystem::initializeAttribute(Entity entity, AttributeId id, float baseValue) {
    auto* comp = getComponent(entity);
    if (!comp) return;

    auto defIt = attributeDefs_.find(id);
    if (defIt == attributeDefs_.end()) return;

    const auto& def = defIt->second;
    AttributeValue value;
    value.setBase(baseValue, def.minValue, def.maxValue);
    value.setCurrent(baseValue, def.minValue, def.maxValue);
    comp->attributes[id] = value;
}

float GASSystem::getAttributeValue(Entity entity, AttributeId id) const {
    const auto* comp = getComponent(entity);
    if (!comp) return 0.0f;

    auto it = comp->attributes.find(id);
    return it != comp->attributes.end() ? it->second.currentValue : 0.0f;
}

float GASSystem::getAttributeBaseValue(Entity entity, AttributeId id) const {
    const auto* comp = getComponent(entity);
    if (!comp) return 0.0f;

    auto it = comp->attributes.find(id);
    return it != comp->attributes.end() ? it->second.baseValue : 0.0f;
}

void GASSystem::setAttributeBaseValue(Entity entity, AttributeId id, float value) {
    auto* comp = getComponent(entity);
    if (!comp) return;

    auto attrIt = comp->attributes.find(id);
    if (attrIt == comp->attributes.end()) return;

    auto defIt = attributeDefs_.find(id);
    float minVal = defIt != attributeDefs_.end() ? defIt->second.minValue : 0.0f;
    float maxVal = defIt != attributeDefs_.end() ? defIt->second.maxValue : std::numeric_limits<float>::max();

    float oldBase = attrIt->second.baseValue;
    attrIt->second.setBase(value, minVal, maxVal);

    // Recalculate current value based on new base
    if (oldBase != attrIt->second.baseValue) {
        recalculateAttribute(entity, id);
    }
}

void GASSystem::modifyAttribute(Entity entity, AttributeId id, float delta) {
    auto* comp = getComponent(entity);
    if (!comp) return;

    auto attrIt = comp->attributes.find(id);
    if (attrIt == comp->attributes.end()) return;

    auto defIt = attributeDefs_.find(id);
    float minVal = defIt != attributeDefs_.end() ? defIt->second.minValue : 0.0f;
    float maxVal = defIt != attributeDefs_.end() ? defIt->second.maxValue : std::numeric_limits<float>::max();

    float oldValue = attrIt->second.currentValue;
    attrIt->second.modify(delta, minVal, maxVal);

    if (oldValue != attrIt->second.currentValue) {
        notifyAttributeChange(entity, id, oldValue, attrIt->second.currentValue);
    }
}

}  // namespace jframe

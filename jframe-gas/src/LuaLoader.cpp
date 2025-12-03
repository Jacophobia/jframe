// jframe-gas/src/LuaLoader.cpp
// Lua definition loading for GAS

module;

#include <compare>
#include <limits>
#include <optional>
#include <string>

#include <sol/sol.hpp>

module jframe.gas.impl;

import jframe.gas;
import jframe.types;

namespace jframe {

namespace {

template<typename T>
T getOr(const sol::table& table, const char* key, T defaultValue) {
    sol::optional<T> val = table[key];
    return val.value_or(defaultValue);
}

std::string getStringOr(const sol::table& table, const char* key, const std::string& defaultValue) {
    sol::optional<std::string> val = table[key];
    return val.value_or(defaultValue);
}

}  // anonymous namespace

GameplayTagContainer GASSystem::parseTagContainer(const sol::table& table) {
    GameplayTagContainer container;

    for (auto& [key, value] : table) {
        if (value.is<std::string>()) {
            std::string tagName = value.as<std::string>();
            GameplayTag tag = registerTag(tagName);
            container.addTag(tag);
        }
    }

    return container;
}

bool GASSystem::loadDefinitionsFromLua(const std::string& luaSource) {
    if (!initialized_) {
        initialize();
    }

    try {
        // Use script_pass_on_error so invalid Lua returns an error result instead of throwing
        sol::protected_function_result result = lua_.safe_script(luaSource, sol::script_pass_on_error);
        if (!result.valid()) {
            sol::error err = result;
            return false;
        }

        // Load Tags
        sol::optional<sol::table> tagsTable = lua_["Tags"];
        if (tagsTable) {
            for (auto& [key, value] : *tagsTable) {
                if (value.is<std::string>()) {
                    registerTag(value.as<std::string>());
                }
            }
        }

        // Load Attributes
        sol::optional<sol::table> attributesTable = lua_["Attributes"];
        if (attributesTable) {
            for (auto& [key, value] : *attributesTable) {
                if (value.is<sol::table>()) {
                    sol::table attrTable = value.as<sol::table>();

                    AttributeDef def;
                    def.name = getStringOr(attrTable, "name", "");
                    def.baseValue = getOr(attrTable, "baseValue", 0.0f);
                    def.minValue = getOr(attrTable, "minValue", 0.0f);
                    def.maxValue = getOr(attrTable, "maxValue", std::numeric_limits<float>::max());
                    def.clampEnabled = getOr(attrTable, "clampEnabled", true);

                    if (!def.name.empty()) {
                        registerAttribute(def);
                    }
                }
            }
        }

        // Load Effects
        sol::optional<sol::table> effectsTable = lua_["Effects"];
        if (effectsTable) {
            for (auto& [key, value] : *effectsTable) {
                if (value.is<sol::table>()) {
                    sol::table effectTable = value.as<sol::table>();

                    EffectDef def;
                    def.name = getStringOr(effectTable, "name", "");

                    // Duration type
                    std::string durationType = getStringOr(effectTable, "durationType", "instant");
                    if (durationType == "instant") {
                        def.durationType = EffectDurationType::Instant;
                    } else if (durationType == "duration") {
                        def.durationType = EffectDurationType::Duration;
                    } else if (durationType == "infinite") {
                        def.durationType = EffectDurationType::Infinite;
                    }

                    def.duration = getOr(effectTable, "duration", 0.0f);
                    def.period = getOr(effectTable, "period", 0.0f);
                    def.stackable = getOr(effectTable, "stackable", false);
                    def.maxStacks = getOr(effectTable, "maxStacks", 1);

                    // Parse modifiers
                    sol::optional<sol::table> modifiersTable = effectTable["modifiers"];
                    if (modifiersTable) {
                        for (auto& [modKey, modValue] : *modifiersTable) {
                            if (modValue.is<sol::table>()) {
                                sol::table modTable = modValue.as<sol::table>();

                                EffectModifier mod;
                                std::string attrName = getStringOr(modTable, "attribute", "");

                                // Look up attribute ID
                                auto attrDef = getAttributeDef(attrName);
                                if (attrDef) {
                                    mod.attribute = attrDef->id;
                                }

                                std::string opStr = getStringOr(modTable, "op", "add");
                                if (opStr == "add") {
                                    mod.op = EffectModifierOp::Add;
                                } else if (opStr == "multiply") {
                                    mod.op = EffectModifierOp::Multiply;
                                } else if (opStr == "override") {
                                    mod.op = EffectModifierOp::Override;
                                }

                                mod.value = getOr(modTable, "value", 0.0f);

                                if (mod.attribute != 0) {
                                    def.modifiers.push_back(mod);
                                }
                            }
                        }
                    }

                    // Parse tag containers
                    sol::optional<sol::table> grantedTagsTable = effectTable["grantedTags"];
                    if (grantedTagsTable) {
                        def.grantedTags = parseTagContainer(*grantedTagsTable);
                    }

                    sol::optional<sol::table> requiredTagsTable = effectTable["applicationRequiredTags"];
                    if (requiredTagsTable) {
                        def.applicationRequiredTags = parseTagContainer(*requiredTagsTable);
                    }

                    sol::optional<sol::table> blockedTagsTable = effectTable["applicationBlockedTags"];
                    if (blockedTagsTable) {
                        def.applicationBlockedTags = parseTagContainer(*blockedTagsTable);
                    }

                    sol::optional<sol::table> removalTagsTable = effectTable["removalTags"];
                    if (removalTagsTable) {
                        def.removalTags = parseTagContainer(*removalTagsTable);
                    }

                    if (!def.name.empty()) {
                        registerEffect(def);
                    }
                }
            }
        }

        // Load Abilities
        sol::optional<sol::table> abilitiesTable = lua_["Abilities"];
        if (abilitiesTable) {
            for (auto& [key, value] : *abilitiesTable) {
                if (value.is<sol::table>()) {
                    sol::table abilityTable = value.as<sol::table>();

                    AbilityDef def;
                    def.name = getStringOr(abilityTable, "name", "");

                    // Activation policy
                    std::string policy = getStringOr(abilityTable, "activationPolicy", "onInputPressed");
                    if (policy == "onInputPressed") {
                        def.activationPolicy = AbilityActivationPolicy::OnInputPressed;
                    } else if (policy == "onInputReleased") {
                        def.activationPolicy = AbilityActivationPolicy::OnInputReleased;
                    } else if (policy == "whileInputHeld") {
                        def.activationPolicy = AbilityActivationPolicy::WhileInputHeld;
                    } else if (policy == "passive") {
                        def.activationPolicy = AbilityActivationPolicy::Passive;
                    }

                    def.cooldown = getOr(abilityTable, "cooldown", 0.0f);

                    // Parse costs
                    sol::optional<sol::table> costsTable = abilityTable["costs"];
                    if (costsTable) {
                        for (auto& [costKey, costValue] : *costsTable) {
                            if (costValue.is<sol::table>()) {
                                sol::table costTable = costValue.as<sol::table>();

                                AbilityCost cost;
                                std::string attrName = getStringOr(costTable, "attribute", "");
                                auto attrDef = getAttributeDef(attrName);
                                if (attrDef) {
                                    cost.attribute = attrDef->id;
                                }
                                cost.cost = getOr(costTable, "cost", 0.0f);

                                if (cost.attribute != 0) {
                                    def.costs.push_back(cost);
                                }
                            }
                        }
                    }

                    // Parse tag containers
                    sol::optional<sol::table> activationRequiredTable = abilityTable["activationRequiredTags"];
                    if (activationRequiredTable) {
                        def.activationRequiredTags = parseTagContainer(*activationRequiredTable);
                    }

                    sol::optional<sol::table> activationBlockedTable = abilityTable["activationBlockedTags"];
                    if (activationBlockedTable) {
                        def.activationBlockedTags = parseTagContainer(*activationBlockedTable);
                    }

                    sol::optional<sol::table> abilityTagsTable = abilityTable["abilityTags"];
                    if (abilityTagsTable) {
                        def.abilityTags = parseTagContainer(*abilityTagsTable);
                    }

                    sol::optional<sol::table> cancelTagsTable = abilityTable["cancelAbilitiesWithTags"];
                    if (cancelTagsTable) {
                        def.cancelAbilitiesWithTags = parseTagContainer(*cancelTagsTable);
                    }

                    sol::optional<sol::table> blockTagsTable = abilityTable["blockAbilitiesWithTags"];
                    if (blockTagsTable) {
                        def.blockAbilitiesWithTags = parseTagContainer(*blockTagsTable);
                    }

                    // Parse effects to apply
                    sol::optional<sol::table> activateEffectsTable = abilityTable["effectsToApplyOnActivate"];
                    if (activateEffectsTable) {
                        for (auto& [effectKey, effectValue] : *activateEffectsTable) {
                            if (effectValue.is<std::string>()) {
                                std::string effectName = effectValue.as<std::string>();
                                auto effectDef = getEffectDef(effectName);
                                if (effectDef) {
                                    def.effectsToApplyOnActivate.push_back(effectDef->id);
                                }
                            }
                        }
                    }

                    sol::optional<sol::table> endEffectsTable = abilityTable["effectsToApplyOnEnd"];
                    if (endEffectsTable) {
                        for (auto& [effectKey, effectValue] : *endEffectsTable) {
                            if (effectValue.is<std::string>()) {
                                std::string effectName = effectValue.as<std::string>();
                                auto effectDef = getEffectDef(effectName);
                                if (effectDef) {
                                    def.effectsToApplyOnEnd.push_back(effectDef->id);
                                }
                            }
                        }
                    }

                    if (!def.name.empty()) {
                        registerAbility(def);
                    }
                }
            }
        }

        return true;

    } catch (...) {
        return false;
    }
}

}  // namespace jframe

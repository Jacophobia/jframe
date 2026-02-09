// state_binding.cpp - State system Lua bindings
// Provides bestow.state.* API for Lua game scripts

module;

#include <bestow/sol2_compat.hpp>
#include <spdlog/spdlog.h>
#include <nlohmann/json.hpp>

module bestow.luabind;

import std;

using json = nlohmann::json;

namespace bestow {

//=============================================================================
// LuaStateManager — manages pending async callbacks
//=============================================================================

class LuaStateManager {
public:
    using CallbackId = std::uint64_t;

    explicit LuaStateManager(sol::state& lua) : lua_(&lua) {}

    CallbackId storeCallback(sol::protected_function fn) {
        CallbackId id = nextId_++;
        callbacks_[id] = std::move(fn);
        return id;
    }

    void fireCallback(CallbackId id, bool success, StateError error) {
        auto it = callbacks_.find(id);
        if (it == callbacks_.end()) return;

        auto fn = std::move(it->second);
        callbacks_.erase(it);

        std::string errMsg;
        if (!success) {
            switch (error) {
                case StateError::SlotNotFound: errMsg = "slot not found"; break;
                case StateError::DatabaseError: errMsg = "database error"; break;
                case StateError::IOError: errMsg = "I/O error"; break;
                case StateError::MigrationFailed: errMsg = "migration failed"; break;
                case StateError::JsonParseError: errMsg = "JSON parse error"; break;
                case StateError::BusyError: errMsg = "busy"; break;
                default: errMsg = "unknown error"; break;
            }
        }

        auto result = fn(success, success ? sol::nil : sol::make_object(*lua_, errMsg));
        if (!result.valid()) {
            sol::error err = result;
            spdlog::error("[State] Error in callback: {}", err.what());
        }
    }

    void cancelAll() {
        callbacks_.clear();
    }

private:
    sol::state* lua_;
    std::unordered_map<CallbackId, sol::protected_function> callbacks_;
    CallbackId nextId_ = 1;
};

// Global manager instance
static std::unique_ptr<LuaStateManager> g_luaStateManager;

//=============================================================================
// JSON ↔ Lua conversion helpers
//=============================================================================

static json luaTableToJson(sol::table tbl) {
    // Check if it's an array (sequential integer keys starting at 1)
    bool isArray = true;
    int maxKey = 0;
    int count = 0;
    for (auto& [k, v] : tbl) {
        count++;
        if (k.get_type() == sol::type::number) {
            int idx = k.as<int>();
            if (idx > maxKey) maxKey = idx;
        } else {
            isArray = false;
            break;
        }
    }
    if (isArray && maxKey > 0 && maxKey == count) {
        json arr = json::array();
        for (int i = 1; i <= maxKey; i++) {
            sol::object val = tbl[i];
            switch (val.get_type()) {
                case sol::type::number: arr.push_back(val.as<double>()); break;
                case sol::type::string: arr.push_back(val.as<std::string>()); break;
                case sol::type::boolean: arr.push_back(val.as<bool>()); break;
                case sol::type::table: arr.push_back(luaTableToJson(val.as<sol::table>())); break;
                default: arr.push_back(nullptr); break;
            }
        }
        return arr;
    }

    json obj = json::object();
    for (auto& [k, v] : tbl) {
        std::string key;
        if (k.get_type() == sol::type::string) {
            key = k.as<std::string>();
        } else if (k.get_type() == sol::type::number) {
            key = std::to_string(k.as<int>());
        } else {
            continue;
        }
        switch (v.get_type()) {
            case sol::type::number: obj[key] = v.as<double>(); break;
            case sol::type::string: obj[key] = v.as<std::string>(); break;
            case sol::type::boolean: obj[key] = v.as<bool>(); break;
            case sol::type::table: obj[key] = luaTableToJson(v.as<sol::table>()); break;
            default: obj[key] = nullptr; break;
        }
    }
    return obj;
}

static sol::object jsonToLua(sol::state& lua, const json& j) {
    if (j.is_number()) return sol::make_object(lua, j.get<double>());
    if (j.is_string()) return sol::make_object(lua, j.get<std::string>());
    if (j.is_boolean()) return sol::make_object(lua, j.get<bool>());
    if (j.is_null()) return sol::nil;
    if (j.is_array()) {
        sol::table tbl = lua.create_table();
        for (std::size_t i = 0; i < j.size(); i++) {
            tbl[static_cast<int>(i) + 1] = jsonToLua(lua, j[i]);
        }
        return tbl;
    }
    if (j.is_object()) {
        sol::table tbl = lua.create_table();
        for (auto& [key, val] : j.items()) {
            tbl[key] = jsonToLua(lua, val);
        }
        return tbl;
    }
    return sol::nil;
}

//=============================================================================
// Binding Function
//=============================================================================

void bindStateSystem(sol::state& lua, IStateSystem& state) {
    g_luaStateManager = std::make_unique<LuaStateManager>(lua);

    sol::table bestow = lua["bestow"];
    sol::table stateTable = lua.create_table();

    //=========================================================================
    // bestow.state.set(key, value)
    //=========================================================================

    stateTable["set"] = [&state, &lua](const std::string& key, sol::object value) {
        switch (value.get_type()) {
            case sol::type::number:
                state.setNumber(key, value.as<double>());
                break;
            case sol::type::string:
                state.setString(key, value.as<std::string>());
                break;
            case sol::type::boolean:
                state.setBool(key, value.as<bool>());
                break;
            case sol::type::table: {
                json j = luaTableToJson(value.as<sol::table>());
                state.setJsonData(key, j.dump());
                break;
            }
            case sol::type::lua_nil:
                state.removeData(key);
                break;
            default:
                luaError(lua.lua_state(), "bestow.state.set",
                    "Unsupported value type. Use number, string, boolean, table, or nil.");
                break;
        }
    };

    //=========================================================================
    // bestow.state.get(key, default)
    //=========================================================================

    stateTable["get"] = [&state, &lua](const std::string& key,
                                        sol::object defaultValue) -> sol::object {
        if (!state.hasData(key)) {
            return defaultValue.valid() ? defaultValue : sol::nil;
        }

        // Probe types by checking typed getters with sentinels.
        // getBool returns the default when the stored type is not Bool,
        // so if getBool(key, false) != getBool(key, true), it's NOT a bool.
        bool bf = state.getBool(key, false);
        bool bt = state.getBool(key, true);
        if (bf == bt) {
            // Both calls returned the same value — it IS a bool
            return sol::make_object(lua, bf);
        }

        // Check number (NaN sentinel)
        double sentinel = std::numeric_limits<double>::quiet_NaN();
        double num = state.getNumber(key, sentinel);
        if (!std::isnan(num)) {
            return sol::make_object(lua, num);
        }

        // Check JSON (returns "{}" for non-json, which is a valid default)
        std::string jsonStr = state.getJsonData(key);
        if (jsonStr != "{}") {
            auto parsed = json::parse(jsonStr, nullptr, false);
            if (!parsed.is_discarded()) {
                return jsonToLua(lua, parsed);
            }
        }

        // String (always succeeds for string type)
        return sol::make_object(lua, state.getString(key, ""));
    };

    //=========================================================================
    // bestow.state.has / .remove / .clear
    //=========================================================================

    stateTable["has"] = [&state](const std::string& key) -> bool {
        return state.hasData(key);
    };

    stateTable["remove"] = [&state](const std::string& key) {
        state.removeData(key);
    };

    stateTable["clear"] = [&state]() {
        state.clearData();
    };

    //=========================================================================
    // bestow.state.commit(slot, name, callback)
    //=========================================================================

    stateTable["commit"] = [&state](
        StateSlot slot,
        sol::optional<std::string> name,
        sol::optional<sol::protected_function> callback)
    {
        std::string saveName = name.value_or("");
        StateCallback cb;

        if (callback && callback->valid()) {
            auto id = g_luaStateManager->storeCallback(*callback);
            cb = [id](bool success, StateError err) {
                if (g_luaStateManager) g_luaStateManager->fireCallback(id, success, err);
            };
        }

        state.commit(slot, saveName, std::move(cb));
    };

    //=========================================================================
    // bestow.state.restore(slot, callback)
    //=========================================================================

    stateTable["restore"] = [&state](
        StateSlot slot,
        sol::optional<sol::protected_function> callback)
    {
        StateCallback cb;

        if (callback && callback->valid()) {
            auto id = g_luaStateManager->storeCallback(*callback);
            cb = [id](bool success, StateError err) {
                if (g_luaStateManager) g_luaStateManager->fireCallback(id, success, err);
            };
        }

        state.restore(slot, std::move(cb));
    };

    //=========================================================================
    // Quick commit/restore
    //=========================================================================

    stateTable["quickCommit"] = [&state](sol::optional<sol::protected_function> callback) {
        StateCallback cb;
        if (callback && callback->valid()) {
            auto id = g_luaStateManager->storeCallback(*callback);
            cb = [id](bool success, StateError err) {
                if (g_luaStateManager) g_luaStateManager->fireCallback(id, success, err);
            };
        }
        state.quickCommit(std::move(cb));
    };

    stateTable["quickRestore"] = [&state](sol::optional<sol::protected_function> callback) {
        StateCallback cb;
        if (callback && callback->valid()) {
            auto id = g_luaStateManager->storeCallback(*callback);
            cb = [id](bool success, StateError err) {
                if (g_luaStateManager) g_luaStateManager->fireCallback(id, success, err);
            };
        }
        state.quickRestore(std::move(cb));
    };

    //=========================================================================
    // Auto-commit
    //=========================================================================

    stateTable["enableAutoCommit"] = [&state](int seconds) {
        state.enableAutoCommit(std::chrono::seconds(seconds));
    };

    stateTable["disableAutoCommit"] = [&state]() {
        state.disableAutoCommit();
    };

    //=========================================================================
    // Metadata queries
    //=========================================================================

    stateTable["getAllSlots"] = [&state, &lua]() -> sol::table {
        auto slots = state.getAllSlotMetadata();
        sol::table result = lua.create_table();
        int i = 1;
        for (const auto& meta : slots) {
            sol::table entry = lua.create_table();
            entry["slot"] = meta.slot;
            entry["name"] = meta.name;
            entry["playtime"] = meta.playtimeSeconds;
            entry["version"] = meta.gameVersion;
            entry["completion"] = meta.completionPercentage;
            if (meta.levelName) entry["level"] = *meta.levelName;
            auto ts = std::chrono::duration_cast<std::chrono::seconds>(
                meta.timestamp.time_since_epoch()).count();
            entry["timestamp"] = ts;
            result[i++] = entry;
        }
        return result;
    };

    stateTable["getSlotMetadata"] = [&state, &lua](StateSlot slot) -> sol::object {
        auto meta = state.getSlotMetadata(slot);
        if (!meta) return sol::nil;
        sol::table entry = lua.create_table();
        entry["slot"] = meta->slot;
        entry["name"] = meta->name;
        entry["playtime"] = meta->playtimeSeconds;
        entry["version"] = meta->gameVersion;
        entry["completion"] = meta->completionPercentage;
        if (meta->levelName) entry["level"] = *meta->levelName;
        auto ts = std::chrono::duration_cast<std::chrono::seconds>(
            meta->timestamp.time_since_epoch()).count();
        entry["timestamp"] = ts;
        return entry;
    };

    stateTable["slotExists"] = [&state](StateSlot slot) -> bool {
        return state.slotExists(slot);
    };

    stateTable["deleteSlot"] = [&state](StateSlot slot) -> bool {
        return state.deleteSlot(slot);
    };

    //=========================================================================
    // Profile management
    //=========================================================================

    stateTable["setProfile"] = [&state](const std::string& profileId) {
        state.setActiveProfile(profileId);
    };

    stateTable["getProfile"] = [&state]() -> std::string {
        return state.getActiveProfile();
    };

    stateTable["getProfiles"] = [&state, &lua]() -> sol::table {
        auto profiles = state.getProfiles();
        sol::table result = lua.create_table();
        for (int i = 0; i < static_cast<int>(profiles.size()); i++) {
            result[i + 1] = profiles[i];
        }
        return result;
    };

    //=========================================================================
    // Game tracking
    //=========================================================================

    stateTable["setGameVersion"] = [&state](const std::string& version) {
        state.setGameVersion(version);
    };

    stateTable["setLevel"] = [&state](const std::string& level) {
        state.setCurrentLevel(level);
    };

    stateTable["setCompletion"] = [&state](float pct) {
        state.setCompletionPercentage(pct);
    };

    //=========================================================================
    // Migration
    //=========================================================================

    stateTable["setFormatVersion"] = [&state](int version) {
        state.setFormatVersion(version);
    };

    stateTable["registerMigration"] = [&state](
        int fromVersion, int toVersion, sol::protected_function fn)
    {
        sol::protected_function migFn = fn;
        state.registerMigration(fromVersion, toVersion, [migFn]() {
            auto result = migFn();
            if (!result.valid()) {
                sol::error err = result;
                spdlog::error("[State] Migration error: {}", err.what());
            }
        });
    };

    bestow["state"] = stateTable;

    spdlog::info("[State] Lua bindings initialized");
}

void cleanupStateBindings() {
    if (g_luaStateManager) {
        g_luaStateManager->cancelAll();
        g_luaStateManager.reset();
        spdlog::debug("[State] Bindings cleaned up");
    }
}

}  // namespace bestow

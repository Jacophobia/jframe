// bestow-luabind/src/bindings/events_binding.cpp
// Events system Lua bindings with hot-reload-safe callback pattern

module;

#include <bestow/sol2_compat.hpp>
#include <spdlog/spdlog.h>

module bestow.luabind;

import std;

namespace bestow {

//=============================================================================
// LuaMethodCallback - Hot-reload-safe wrapper for Lua method callbacks
//=============================================================================

/// Stores a table reference and method name for late binding.
/// When called, looks up the method by name (getting the current version)
/// and invokes it with the table as first argument.
struct LuaMethodCallback {
    sol::table table;         // Reference to the Lua table (e.g., self)
    std::string methodName;   // Name of the method to call
    sol::table pattern;       // Event filter pattern (optional)
    sol::state* lua = nullptr; // Lua state for scope creation

    /// Check if an event matches our pattern
    bool matchesPattern(const sol::table& eventData) const {
        if (!pattern.valid() || pattern.empty()) {
            return true;  // Empty pattern matches everything
        }

        // Check each pattern field against event data
        for (auto& pair : pattern) {
            sol::object patternKey = pair.first;
            sol::object patternValue = pair.second;

            if (!patternKey.is<std::string>()) continue;
            std::string key = patternKey.as<std::string>();

            sol::object eventValue = eventData[key];
            if (!eventValue.valid()) {
                return false;  // Event doesn't have required field
            }

            // Compare values (simple equality for numbers, strings, etc.)
            if (patternValue.get_type() != eventValue.get_type()) {
                return false;
            }

            if (patternValue.is<double>()) {
                if (patternValue.as<double>() != eventValue.as<double>()) {
                    return false;
                }
            } else if (patternValue.is<std::string>()) {
                if (patternValue.as<std::string>() != eventValue.as<std::string>()) {
                    return false;
                }
            } else if (patternValue.is<bool>()) {
                if (patternValue.as<bool>() != eventValue.as<bool>()) {
                    return false;
                }
            } else if (patternValue.is<std::int64_t>()) {
                if (patternValue.as<std::int64_t>() != eventValue.as<std::int64_t>()) {
                    return false;
                }
            }
            // For tables and other types, do reference comparison
            else if (patternValue.get_type() == sol::type::table) {
                if (patternValue.as<sol::table>().pointer() != eventValue.as<sol::table>().pointer()) {
                    return false;
                }
            }
        }

        return true;
    }

    /// Invoke the callback with late binding
    void operator()(const sol::table& eventData, const sol::table& scope) const {
        if (!table.valid()) {
            spdlog::warn("[Events] Callback invoked with invalid table reference");
            return;
        }

        // Check pattern match first
        if (!matchesPattern(eventData)) {
            return;  // Pattern doesn't match, skip this callback
        }

        // Late binding: look up method by name NOW (gets current version after hot reload)
        sol::object methodObj = table[methodName];
        if (!methodObj.valid() || methodObj.get_type() != sol::type::function) {
            spdlog::warn("[Events] Method '{}' not found on table (may have been removed during hot reload)",
                        methodName);
            return;
        }

        sol::protected_function method = methodObj;

        // Call: method(self, event, scope)
        sol::protected_function_result result = method(table, eventData, scope);

        if (!result.valid()) {
            sol::error err = result;
            spdlog::error("[Events] Error calling {}: {}", methodName, err.what());
        }
    }
};

//=============================================================================
// LuaEventSubscriptionManager - Manages Lua subscriptions with pattern matching
//=============================================================================

class LuaEventSubscriptionManager {
public:
    static constexpr int kMaxEventDepth = 16;  // Prevent infinite loops

    LuaEventSubscriptionManager(IEventSystem& events, sol::state& lua)
        : events_(&events), lua_(&lua) {}

    /// Subscribe with (eventType, pattern, table, methodName) signature
    /// This is the hot-reload-safe pattern
    SubscriptionId subscribe(
        const std::string& eventType,
        sol::table pattern,
        sol::table table,
        const std::string& methodName)
    {
        // Validate the method exists at subscription time
        sol::object methodObj = table[methodName];
        if (!methodObj.valid() || methodObj.get_type() != sol::type::function) {
            std::string msg = std::format(
                R"(Cannot subscribe to '{}': method '{}' not found on table.

Make sure the method exists:
  return {{
    init = function(self, scope)
      bestow.events.subscribe("{}", pattern, self, "{}")
    end,
    {} = function(self, event, scope)  -- This method must exist!
      -- handle event
    end,
  }}
)",
                eventType, methodName, eventType, methodName, methodName);
            throw std::runtime_error(msg);
        }

        // Create the late-binding callback wrapper
        LuaMethodCallback callback{
            .table = table,
            .methodName = methodName,
            .pattern = pattern,
            .lua = lua_
        };

        // Generate subscription ID
        SubscriptionId subId = nextId_++;

        // Store the subscription info for later dispatch
        subscriptions_[subId] = SubscriptionInfo{
            .eventType = eventType,
            .callback = std::move(callback),
            .active = true
        };

        // Subscribe to the underlying event system
        // We use a single dispatcher per event type that fans out to Lua callbacks
        ensureEventTypeSubscribed(eventType);

        spdlog::debug("[Events] Subscribed to '{}' with method '{}' (id: {})",
                     eventType, methodName, subId);

        return subId;
    }

    /// Unsubscribe by ID
    void unsubscribe(SubscriptionId id) {
        auto it = subscriptions_.find(id);
        if (it != subscriptions_.end()) {
            it->second.active = false;
            subscriptions_.erase(it);
            spdlog::debug("[Events] Unsubscribed (id: {})", id);
        }
    }

    /// Emit an event (will be dispatched to all matching Lua subscribers)
    void emit(const std::string& eventType, sol::table eventData) {
        // Re-entrancy guard: track dispatch depth per event type
        auto& depth = eventDepth_[eventType];
        if (depth >= kMaxEventDepth) {
            spdlog::error(
                "[Events] Maximum event dispatch depth ({}) exceeded for '{}'. "
                "This usually indicates an infinite loop where an event handler "
                "emits the same event type it's handling. Check your event handlers.",
                kMaxEventDepth, eventType);
            return;
        }

        // RAII guard to decrement depth on exit (even if exception)
        struct DepthGuard {
            int& d;
            DepthGuard(int& depth) : d(depth) { ++d; }
            ~DepthGuard() { --d; }
        } guard(depth);

        // Create scope table for callbacks
        sol::table scope = createScope();

        // Copy subscription IDs to iterate (handlers may modify subscriptions_)
        std::vector<SubscriptionId> subsToCall;
        for (auto& [id, sub] : subscriptions_) {
            if (sub.active && sub.eventType == eventType) {
                subsToCall.push_back(id);
            }
        }

        // Dispatch to all matching Lua subscriptions
        for (SubscriptionId id : subsToCall) {
            auto it = subscriptions_.find(id);
            if (it == subscriptions_.end() || !it->second.active) {
                continue;  // Subscription was removed during dispatch
            }

            try {
                it->second.callback(eventData, scope);
            } catch (const std::exception& e) {
                spdlog::error("[Events] Exception in event handler for '{}': {}",
                             eventType, e.what());
            }
        }
    }

    /// Unsubscribe all subscriptions owned by a specific table
    /// Call this when an entity/component is destroyed
    void unsubscribeByOwner(sol::table owner) {
        std::vector<SubscriptionId> toRemove;

        for (auto& [id, sub] : subscriptions_) {
            // Compare table pointers to check ownership
            if (sub.callback.table.pointer() == owner.pointer()) {
                toRemove.push_back(id);
            }
        }

        for (SubscriptionId id : toRemove) {
            subscriptions_.erase(id);
        }

        if (!toRemove.empty()) {
            spdlog::debug("[Events] Unsubscribed {} subscriptions by owner", toRemove.size());
        }
    }

    /// Get count of active subscriptions (for debugging)
    std::size_t subscriptionCount() const {
        return subscriptions_.size();
    }

private:
    /// Create a scope table for callbacks
    sol::table createScope() const {
        sol::table scope = lua_->create_table();
        // TODO: Populate with global, profile, level, entity based on context
        // For now, provide basic access methods
        scope["emit"] = [this](const std::string& type, sol::table data) {
            const_cast<LuaEventSubscriptionManager*>(this)->emit(type, data);
        };
        return scope;
    }

    void ensureEventTypeSubscribed(const std::string& eventType) {
        // Track which event types we've registered with the C++ event system
        if (subscribedEventTypes_.contains(eventType)) {
            return;
        }

        // Subscribe to the C++ event system for this event type
        // This bridges C++ events to Lua
        events_->subscribe(eventType, [this, eventType](const EventData& data) {
            // Convert EventData to Lua table and dispatch
            sol::table eventTable = convertEventDataToLua(data);
            eventTable["_type"] = eventType;

            sol::table scope = createScope();

            for (auto& [id, sub] : subscriptions_) {
                if (sub.active && sub.eventType == eventType) {
                    try {
                        sub.callback(eventTable, scope);
                    } catch (const std::exception& e) {
                        spdlog::error("[Events] Exception in C++ event bridge: {}", e.what());
                    }
                }
            }
        });

        subscribedEventTypes_.insert(eventType);
    }

    /// Convert C++ EventData variant to Lua table
    sol::table convertEventDataToLua(const EventData& data) const {
        sol::table result = lua_->create_table();

        std::visit([&result, this](auto&& arg) {
            using T = std::decay_t<decltype(arg)>;

            if constexpr (std::is_same_v<T, EntityEventData>) {
                result["entity"] = static_cast<std::uint32_t>(arg.entity);
                if (arg.otherEntity) {
                    result["otherEntity"] = static_cast<std::uint32_t>(*arg.otherEntity);
                }
            }
            else if constexpr (std::is_same_v<T, DamageEventData>) {
                result["target"] = static_cast<std::uint32_t>(arg.target);
                result["source"] = static_cast<std::uint32_t>(arg.source);
                result["amount"] = arg.amount;
                result["damageType"] = arg.damageType;
            }
            else if constexpr (std::is_same_v<T, ActionEventData>) {
                result["action"] = arg.action;
                result["phase"] = arg.phase;
                result["value"] = arg.value;
            }
            else if constexpr (std::is_same_v<T, PhaseEventData>) {
                result["oldPhase"] = arg.oldPhase;
                result["newPhase"] = arg.newPhase;
                result["phaseStack"] = sol::as_table(arg.phaseStack);
            }
            else if constexpr (std::is_same_v<T, std::any>) {
                // For std::any, try to extract as Lua-compatible types
                // This is used for custom event data
                result["_isAny"] = true;
            }
            // Add more conversions as needed
        }, data);

        return result;
    }

    struct SubscriptionInfo {
        std::string eventType;
        LuaMethodCallback callback;
        bool active = true;
    };

    IEventSystem* events_;
    sol::state* lua_;
    std::unordered_map<SubscriptionId, SubscriptionInfo> subscriptions_;
    std::unordered_set<std::string> subscribedEventTypes_;
    std::unordered_map<std::string, int> eventDepth_;  // Re-entrancy tracking per event type
    SubscriptionId nextId_ = 1;
};

// Global manager instance (will be properly managed in real implementation)
static std::unique_ptr<LuaEventSubscriptionManager> g_luaEventManager;

//=============================================================================
// Lua Binding Function
//=============================================================================

void bindEventSystem(sol::state& lua, IEventSystem& events) {
    // Create the event subscription manager
    g_luaEventManager = std::make_unique<LuaEventSubscriptionManager>(events, lua);

    sol::table bestow = lua["bestow"];
    sol::table eventsTable = lua.create_table();

    //=========================================================================
    // bestow.events.subscribe(eventType, pattern, table, methodName)
    //=========================================================================

    eventsTable["subscribe"] = [](
        const std::string& eventType,
        sol::object patternOrTable,
        sol::object tableOrMethod,
        sol::optional<std::string> methodNameOpt) -> SubscriptionId
    {
        // Detect which signature is being used

        // Check for OLD (unsafe) closure pattern and provide helpful error
        if (patternOrTable.get_type() == sol::type::function ||
            (patternOrTable.get_type() == sol::type::table &&
             tableOrMethod.get_type() == sol::type::function)) {

            std::string msg = R"(bestow.events.subscribe() does not accept function callbacks.

Closures capture references at creation time and become stale after hot reload.

WRONG (closure - breaks on hot reload):
  bestow.events.subscribe("Hit", pattern, function(event)
    self:onHit(event)  -- This closure captures OLD self
  end)

RIGHT (table + method name - hot reload safe):
  bestow.events.subscribe("Hit", pattern, self, "onHit")

The method is looked up by name at dispatch time, so it always calls
the current version of the code after hot reload.
)";
            throw std::runtime_error(msg);
        }

        // New signature: (eventType, pattern, table, methodName)
        if (!patternOrTable.is<sol::table>()) {
            throw std::runtime_error(
                "bestow.events.subscribe: second argument must be a pattern table (can be empty {})"
            );
        }

        if (!tableOrMethod.is<sol::table>()) {
            throw std::runtime_error(
                "bestow.events.subscribe: third argument must be the callback table (e.g., self)"
            );
        }

        if (!methodNameOpt.has_value()) {
            throw std::runtime_error(
                "bestow.events.subscribe: fourth argument must be the method name string"
            );
        }

        sol::table pattern = patternOrTable.as<sol::table>();
        sol::table table = tableOrMethod.as<sol::table>();
        std::string methodName = *methodNameOpt;

        return g_luaEventManager->subscribe(eventType, pattern, table, methodName);
    };

    //=========================================================================
    // bestow.events.unsubscribe(id)
    //=========================================================================

    eventsTable["unsubscribe"] = [](SubscriptionId id) {
        g_luaEventManager->unsubscribe(id);
    };

    //=========================================================================
    // bestow.events.unsubscribeAll(owner)
    // Unsubscribe all subscriptions owned by a table (for cleanup on destroy)
    //=========================================================================

    eventsTable["unsubscribeAll"] = [](sol::table owner) {
        g_luaEventManager->unsubscribeByOwner(owner);
    };

    //=========================================================================
    // bestow.events.count() - Get subscription count (for debugging)
    //=========================================================================

    eventsTable["count"] = []() -> std::size_t {
        return g_luaEventManager->subscriptionCount();
    };

    //=========================================================================
    // bestow.events.emit(eventType, data)
    //=========================================================================

    eventsTable["emit"] = [](const std::string& eventType, sol::table data) {
        g_luaEventManager->emit(eventType, data);
    };

    bestow["events"] = eventsTable;

    spdlog::info("[Events] Lua bindings initialized (hot-reload-safe pattern)");
}

} // namespace bestow

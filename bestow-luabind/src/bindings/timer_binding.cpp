// bestow-luabind/src/bindings/timer_binding.cpp
// Timer system Lua bindings with hot-reload-safe callback pattern

module;

#include <bestow/sol2_compat.hpp>
#include <spdlog/spdlog.h>

module bestow.luabind;

import std;

namespace bestow {

//=============================================================================
// LuaTimerCallback - Hot-reload-safe wrapper for Lua timer callbacks
//=============================================================================

/// Stores a table reference and method name for late binding.
/// When the timer fires, looks up the method by name (getting the current version)
/// and invokes it with the table as first argument plus any extra args.
struct LuaTimerCallback {
    sol::table table;              // Reference to the Lua table (e.g., self)
    std::string methodName;        // Name of the method to call
    std::vector<sol::object> args; // Extra arguments to pass
    sol::state* lua = nullptr;     // Lua state

    /// Invoke the callback with late binding
    void operator()() const {
        if (!table.valid()) {
            spdlog::warn("[Timer] Callback invoked with invalid table reference");
            return;
        }

        // Late binding: look up method by name NOW (gets current version after hot reload)
        sol::object methodObj = table[methodName];
        if (!methodObj.valid() || methodObj.get_type() != sol::type::function) {
            spdlog::warn("[Timer] Method '{}' not found on table (may have been removed during hot reload)",
                        methodName);
            return;
        }

        sol::protected_function method = methodObj;
        sol::protected_function_result result;

        // Call: method(self, ...args)
        switch (args.size()) {
            case 0:
                result = method(table);
                break;
            case 1:
                result = method(table, args[0]);
                break;
            case 2:
                result = method(table, args[0], args[1]);
                break;
            case 3:
                result = method(table, args[0], args[1], args[2]);
                break;
            case 4:
                result = method(table, args[0], args[1], args[2], args[3]);
                break;
            default:
                // For more args, use variadic call via apply
                spdlog::warn("[Timer] More than 4 extra args not supported, truncating");
                result = method(table, args[0], args[1], args[2], args[3]);
                break;
        }

        if (!result.valid()) {
            sol::error err = result;
            spdlog::error("[Timer] Error calling {}: {}", methodName, err.what());
        }
    }
};

//=============================================================================
// LuaTimerManager - Manages timer callbacks with hot-reload-safe pattern
//=============================================================================

class LuaTimerManager {
public:
    using TimerId = std::uint64_t;

    explicit LuaTimerManager(sol::state& lua) : lua_(&lua) {}

    /// Schedule a method to be called after a delay
    /// @param delay Time in seconds
    /// @param table The table containing the method (e.g., self)
    /// @param methodName Name of the method to call
    /// @param args Extra arguments to pass to the method
    /// @return Timer ID for cancellation
    TimerId after(
        float delay,
        sol::table table,
        const std::string& methodName,
        std::vector<sol::object> args = {})
    {
        // Validate the method exists at schedule time
        sol::object methodObj = table[methodName];
        if (!methodObj.valid() || methodObj.get_type() != sol::type::function) {
            luaNotFoundError(lua_->lua_state(), "bestow.timer.after",
                "Method", std::format("'{}' on the provided table. Make sure the method exists:\n\n"
                    "  return {{\n"
                    "    init = function(self, scope)\n"
                    "      bestow.timer.after(1.0, self, \"{}\")\n"
                    "    end,\n"
                    "    {} = function(self)  -- This method must exist!\n"
                    "      -- timer callback\n"
                    "    end,\n"
                    "  }}", methodName, methodName, methodName));
        }

        TimerId id = nextId_++;

        timers_[id] = TimerInfo{
            .remaining = delay,
            .callback = LuaTimerCallback{
                .table = table,
                .methodName = methodName,
                .args = std::move(args),
                .lua = lua_
            },
            .paused = false,
            .repeating = false,
            .interval = 0.0f
        };

        spdlog::debug("[Timer] Scheduled '{}' in {}s (id: {})", methodName, delay, id);
        return id;
    }

    /// Schedule a repeating method call
    /// @param interval Time between calls in seconds
    /// @param table The table containing the method
    /// @param methodName Name of the method to call
    /// @param args Extra arguments
    /// @return Timer ID for cancellation
    TimerId every(
        float interval,
        sol::table table,
        const std::string& methodName,
        std::vector<sol::object> args = {})
    {
        // Validate the method exists at schedule time
        sol::object methodObj = table[methodName];
        if (!methodObj.valid() || methodObj.get_type() != sol::type::function) {
            luaNotFoundError(lua_->lua_state(), "bestow.timer.every",
                "Method", std::format("'{}' on the provided table", methodName));
        }

        TimerId id = nextId_++;

        timers_[id] = TimerInfo{
            .remaining = interval,
            .callback = LuaTimerCallback{
                .table = table,
                .methodName = methodName,
                .args = std::move(args),
                .lua = lua_
            },
            .paused = false,
            .repeating = true,
            .interval = interval
        };

        spdlog::debug("[Timer] Scheduled repeating '{}' every {}s (id: {})", methodName, interval, id);
        return id;
    }

    /// Cancel a timer
    void cancel(TimerId id) {
        if (timers_.erase(id) > 0) {
            spdlog::debug("[Timer] Cancelled (id: {})", id);
        }
    }

    /// Pause a timer
    void pause(TimerId id) {
        auto it = timers_.find(id);
        if (it != timers_.end()) {
            it->second.paused = true;
        }
    }

    /// Resume a timer
    void resume(TimerId id) {
        auto it = timers_.find(id);
        if (it != timers_.end()) {
            it->second.paused = false;
        }
    }

    /// Check if a timer exists and is active
    bool isActive(TimerId id) const {
        auto it = timers_.find(id);
        return it != timers_.end() && !it->second.paused;
    }

    /// Update all timers - call this every frame
    void update(float dt) {
        // Collect IDs to remove (can't modify map during iteration)
        std::vector<TimerId> toRemove;

        for (auto& [id, timer] : timers_) {
            if (timer.paused) continue;

            timer.remaining -= dt;

            if (timer.remaining <= 0.0f) {
                // Fire the callback
                try {
                    timer.callback();
                } catch (const std::exception& e) {
                    spdlog::error("[Timer] Exception in callback: {}", e.what());
                }

                if (timer.repeating) {
                    // Reset for next interval
                    timer.remaining = timer.interval;
                } else {
                    // Mark for removal
                    toRemove.push_back(id);
                }
            }
        }

        // Remove completed one-shot timers
        for (TimerId id : toRemove) {
            timers_.erase(id);
        }
    }

    /// Cancel all timers
    void cancelAll() {
        timers_.clear();
        spdlog::debug("[Timer] Cancelled all timers");
    }

    /// Cancel all timers owned by a specific table
    /// Call this when an entity/component is destroyed
    void cancelByOwner(sol::table owner) {
        std::vector<TimerId> toRemove;

        for (auto& [id, timer] : timers_) {
            // Compare table pointers to check ownership
            if (timer.callback.table.pointer() == owner.pointer()) {
                toRemove.push_back(id);
            }
        }

        for (TimerId id : toRemove) {
            timers_.erase(id);
        }

        if (!toRemove.empty()) {
            spdlog::debug("[Timer] Cancelled {} timers by owner", toRemove.size());
        }
    }

    /// Get active timer count
    std::size_t count() const {
        return timers_.size();
    }

private:
    struct TimerInfo {
        float remaining;
        LuaTimerCallback callback;
        bool paused = false;
        bool repeating = false;
        float interval = 0.0f;
    };

    sol::state* lua_;
    std::unordered_map<TimerId, TimerInfo> timers_;
    TimerId nextId_ = 1;
};

// Global manager instance
static std::unique_ptr<LuaTimerManager> g_luaTimerManager;

//=============================================================================
// Lua Binding Function
//=============================================================================

void bindTimerSystem(sol::state& lua) {
    // Create the timer manager
    g_luaTimerManager = std::make_unique<LuaTimerManager>(lua);

    sol::table bestow = lua["bestow"];
    sol::table timerTable = lua.create_table();

    //=========================================================================
    // bestow.timer.after(delay, table, methodName, ...)
    //=========================================================================

    timerTable["after"] = [&lua](
        float delay,
        sol::object tableOrDelay,
        sol::object methodOrCallback,
        sol::variadic_args va) -> LuaTimerManager::TimerId
    {
        lua_State* L = lua.lua_state();

        // Check for OLD (unsafe) closure pattern and provide helpful error
        if (methodOrCallback.get_type() == sol::type::function) {
            luaError(L, "bestow.timer.after",
                R"(Function callbacks are not allowed (breaks hot reload).

Closures capture references at creation time and become stale after hot reload.

WRONG (closure - breaks on hot reload):
  bestow.timer.after(1.0, function()
    self.transient.invulnerable = false  -- 'self' captured at creation
  end)

RIGHT (table + method name - hot reload safe):
  bestow.timer.after(1.0, self, "clearInvulnerability")

  -- And define the method:
  clearInvulnerability = function(self)
    self.transient.invulnerable = false
  end,

The method is looked up by name when the timer fires, so it always calls
the current version of the code after hot reload.)");
        }

        // New signature: (delay, table, methodName, ...args)
        if (!tableOrDelay.is<sol::table>()) {
            luaTypeError(L, "bestow.timer.after", 2, "table (self)", tableOrDelay.get_type());
        }

        if (!methodOrCallback.is<std::string>()) {
            luaTypeError(L, "bestow.timer.after", 3, "string (method name)", methodOrCallback.get_type());
        }

        sol::table table = tableOrDelay.as<sol::table>();
        std::string methodName = methodOrCallback.as<std::string>();

        // Collect extra arguments
        std::vector<sol::object> args;
        for (auto v : va) {
            args.push_back(v.get<sol::object>());
        }

        return g_luaTimerManager->after(delay, table, methodName, std::move(args));
    };

    //=========================================================================
    // bestow.timer.every(interval, table, methodName, ...)
    //=========================================================================

    timerTable["every"] = [&lua](
        float interval,
        sol::object tableArg,
        sol::object methodArg,
        sol::variadic_args va) -> LuaTimerManager::TimerId
    {
        lua_State* L = lua.lua_state();

        // Check for closure pattern
        if (methodArg.get_type() == sol::type::function) {
            luaError(L, "bestow.timer.every",
                R"(Function callbacks are not allowed (breaks hot reload).

WRONG (closure - breaks on hot reload):
  bestow.timer.every(0.5, function()
    self:tick()  -- 'self' captured at creation
  end)

RIGHT (table + method name - hot reload safe):
  bestow.timer.every(0.5, self, "tick")

The method is looked up by name each time the timer fires.)");
        }

        if (!tableArg.is<sol::table>()) {
            luaTypeError(L, "bestow.timer.every", 2, "table (self)", tableArg.get_type());
        }

        if (!methodArg.is<std::string>()) {
            luaTypeError(L, "bestow.timer.every", 3, "string (method name)", methodArg.get_type());
        }

        sol::table table = tableArg.as<sol::table>();
        std::string methodName = methodArg.as<std::string>();

        std::vector<sol::object> args;
        for (auto v : va) {
            args.push_back(v.get<sol::object>());
        }

        return g_luaTimerManager->every(interval, table, methodName, std::move(args));
    };

    //=========================================================================
    // bestow.timer.cancel(id)
    //=========================================================================

    timerTable["cancel"] = [](LuaTimerManager::TimerId id) {
        g_luaTimerManager->cancel(id);
    };

    //=========================================================================
    // bestow.timer.pause(id) / resume(id)
    //=========================================================================

    timerTable["pause"] = [](LuaTimerManager::TimerId id) {
        g_luaTimerManager->pause(id);
    };

    timerTable["resume"] = [](LuaTimerManager::TimerId id) {
        g_luaTimerManager->resume(id);
    };

    //=========================================================================
    // bestow.timer.isActive(id)
    //=========================================================================

    timerTable["isActive"] = [](LuaTimerManager::TimerId id) -> bool {
        return g_luaTimerManager->isActive(id);
    };

    //=========================================================================
    // bestow.timer.cancelAll()
    //=========================================================================

    timerTable["cancelAll"] = []() {
        g_luaTimerManager->cancelAll();
    };

    //=========================================================================
    // bestow.timer.cancelFor(owner)
    // Cancel all timers owned by a table (for cleanup on destroy)
    //=========================================================================

    timerTable["cancelFor"] = [](sol::table owner) {
        g_luaTimerManager->cancelByOwner(owner);
    };

    //=========================================================================
    // bestow.timer.count()
    //=========================================================================

    timerTable["count"] = []() -> std::size_t {
        return g_luaTimerManager->count();
    };

    bestow["timer"] = timerTable;

    spdlog::info("[Timer] Lua bindings initialized (hot-reload-safe pattern)");
}

/// Update function to be called each frame from the game loop
void updateTimers(float dt) {
    if (g_luaTimerManager) {
        g_luaTimerManager->update(dt);
    }
}

/// Cleanup function - MUST be called before lua_close() to release Lua references
void cleanupTimerBindings() {
    if (g_luaTimerManager) {
        g_luaTimerManager->cancelAll();  // Clear all timers (releases sol::table refs)
        g_luaTimerManager.reset();       // Destroy the manager
        spdlog::debug("[Timer] Bindings cleaned up");
    }
}

} // namespace bestow

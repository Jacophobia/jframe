// bestow-runtime/src/LuaGameAPI.hpp
// High-level Game API for Lua scripts (PROTOTYPE)
//
// This provides the main interface for Lua games:
//   game = { title = "My Game", ... }
//   function game:init() ... end
//   function game:update(dt) ... end

#pragma once

#include <bestow/sol2_compat.hpp>
#include <bestow/entt_compat.hpp>

#include <string>
#include <memory>
#include <vector>
#include <cstdint>

namespace bestow::runtime {

class LuaEntity;
class LuaBindings;

/// Game configuration parsed from Lua
struct GameConfig {
    std::string title = "Bestow Game";
    int windowWidth = 1280;
    int windowHeight = 720;
    bool vsync = true;
    bool fullscreen = false;
    std::string renderer = "vulkan";
};

/// Timer callback for delayed/repeating actions
struct TimerCallback {
    std::uint64_t id;
    float interval;
    float remaining;
    bool repeating;
    sol::function callback;
};

/// High-level Game API exposed to Lua
class LuaGameAPI {
public:
    explicit LuaGameAPI(sol::state& lua);
    ~LuaGameAPI();

    /// Initialize the game API
    void initialize();

    /// Register the Game API in Lua
    void registerInLua();

    /// Parse game config from Lua table
    GameConfig parseConfig(sol::table configTable);

    // ========================================================================
    // Entity Management
    // ========================================================================

    /// Create a new entity
    LuaEntity createEntity();

    /// Query entities by tag
    std::vector<LuaEntity> withTag(const std::string& tag);

    /// Find first entity with tag
    sol::object find(const std::string& tag);

    // ========================================================================
    // Timers
    // ========================================================================

    /// Execute callback after delay
    std::uint64_t after(float seconds, sol::function callback);

    /// Execute callback repeatedly
    std::uint64_t every(float seconds, sol::function callback);

    /// Cancel a timer
    void cancelTimer(std::uint64_t timerId);

    /// Update timers (called by main loop)
    void updateTimers(float dt);

    // ========================================================================
    // Events
    // ========================================================================

    /// Subscribe to an event
    void on(const std::string& eventName, sol::function callback);

    /// Emit an event
    void emit(const std::string& eventName, sol::table data);

    // ========================================================================
    // Game Loop Integration
    // ========================================================================

    /// Update the game
    void update(float dt);

    /// Check if game should quit
    bool shouldQuit() const { return shouldQuit_; }

    /// Request game to quit
    void quit() { shouldQuit_ = true; }

    /// Get the entity registry
    entt::registry* getRegistry();

private:
    sol::state& lua_;

    // Game state
    bool shouldQuit_ = false;
    std::uint64_t nextTimerId_ = 1;
    std::vector<TimerCallback> timers_;

    // Event subscriptions
    struct EventSub {
        std::string eventName;
        sol::function callback;
    };
    std::vector<EventSub> eventSubscriptions_;

    // Lua bindings (owns the registry)
    std::unique_ptr<LuaBindings> bindings_;
};

}  // namespace bestow::runtime

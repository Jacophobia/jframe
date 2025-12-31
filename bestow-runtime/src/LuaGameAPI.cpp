// bestow-runtime/src/LuaGameAPI.cpp
// High-level Game API implementation (PROTOTYPE)

#include "LuaGameAPI.hpp"
#include "LuaBindings.hpp"

#include <spdlog/spdlog.h>
#include <algorithm>

namespace bestow::runtime {

LuaGameAPI::LuaGameAPI(sol::state& lua)
    : lua_(lua)
    , bindings_(std::make_unique<LuaBindings>(lua))
{
}

LuaGameAPI::~LuaGameAPI() = default;

void LuaGameAPI::initialize() {
    // Register Lua bindings
    bindings_->registerAll();

    spdlog::info("[LuaGameAPI] Initialized (prototype mode)");
}

void LuaGameAPI::registerInLua() {
    // Note: bindings_->registerAll() already creates the bestow namespace

    // Add game helper functions to bestow namespace
    sol::table bestow = lua_["bestow"];

    // Create entity function
    bestow["createEntity"] = [this]() {
        return this->createEntity();
    };

    // Query functions
    bestow["withTag"] = [this](const std::string& tag) {
        return this->withTag(tag);
    };

    bestow["find"] = [this](const std::string& tag) {
        return this->find(tag);
    };

    // Timer functions
    bestow["after"] = [this](float seconds, sol::function callback) {
        return this->after(seconds, callback);
    };

    bestow["every"] = [this](float seconds, sol::function callback) {
        return this->every(seconds, callback);
    };

    bestow["cancelTimer"] = [this](std::uint64_t id) {
        this->cancelTimer(id);
    };

    // Event functions
    bestow["on"] = [this](const std::string& event, sol::function callback) {
        this->on(event, callback);
    };

    bestow["emit"] = [this](const std::string& event, sol::table data) {
        this->emit(event, data);
    };

    spdlog::debug("[LuaGameAPI] Lua API registered");
}

GameConfig LuaGameAPI::parseConfig(sol::table configTable) {
    GameConfig config;

    if (configTable["title"].valid()) {
        config.title = configTable["title"].get<std::string>();
    }

    if (configTable["window"].valid()) {
        sol::table window = configTable["window"];
        if (window["width"].valid()) {
            config.windowWidth = window["width"].get<int>();
        }
        if (window["height"].valid()) {
            config.windowHeight = window["height"].get<int>();
        }
    }

    if (configTable["vsync"].valid()) {
        config.vsync = configTable["vsync"].get<bool>();
    }

    if (configTable["fullscreen"].valid()) {
        config.fullscreen = configTable["fullscreen"].get<bool>();
    }

    if (configTable["renderer"].valid()) {
        config.renderer = configTable["renderer"].get<std::string>();
    }

    return config;
}

// ============================================================================
// Entity Management
// ============================================================================

LuaEntity LuaGameAPI::createEntity() {
    auto* registry = bindings_->getRegistry();
    if (!registry) {
        spdlog::error("[LuaGameAPI] Cannot create entity - no registry");
        return LuaEntity(entt::null, nullptr, nullptr);
    }

    auto entity = registry->create();
    spdlog::debug("[LuaGameAPI] Created entity {}", static_cast<uint32_t>(entity));
    return LuaEntity(entity, registry, &lua_);
}

std::vector<LuaEntity> LuaGameAPI::withTag(const std::string& tag) {
    std::vector<LuaEntity> result;

    auto* registry = bindings_->getRegistry();
    if (!registry) {
        return result;
    }

    // Query entities with TagComponent
    auto view = registry->view<TagComponent>();
    for (auto entity : view) {
        const auto& tc = view.get<TagComponent>(entity);
        if (tc.hasTag(tag)) {
            result.emplace_back(entity, registry, &lua_);
        }
    }

    return result;
}

sol::object LuaGameAPI::find(const std::string& tag) {
    auto entities = withTag(tag);
    if (!entities.empty()) {
        return sol::make_object(lua_, entities[0]);
    }
    return sol::lua_nil;
}

entt::registry* LuaGameAPI::getRegistry() {
    return bindings_->getRegistry();
}

// ============================================================================
// Timers
// ============================================================================

std::uint64_t LuaGameAPI::after(float seconds, sol::function callback) {
    TimerCallback timer;
    timer.id = nextTimerId_++;
    timer.interval = seconds;
    timer.remaining = seconds;
    timer.repeating = false;
    timer.callback = std::move(callback);
    timers_.push_back(std::move(timer));
    spdlog::debug("[LuaGameAPI] Timer {} created: after {} seconds", timer.id, seconds);
    return timer.id;
}

std::uint64_t LuaGameAPI::every(float seconds, sol::function callback) {
    TimerCallback timer;
    timer.id = nextTimerId_++;
    timer.interval = seconds;
    timer.remaining = seconds;
    timer.repeating = true;
    timer.callback = std::move(callback);
    timers_.push_back(std::move(timer));
    spdlog::debug("[LuaGameAPI] Timer {} created: every {} seconds", timer.id, seconds);
    return timer.id;
}

void LuaGameAPI::cancelTimer(std::uint64_t timerId) {
    auto it = std::remove_if(timers_.begin(), timers_.end(),
        [timerId](const TimerCallback& t) { return t.id == timerId; });
    if (it != timers_.end()) {
        timers_.erase(it, timers_.end());
        spdlog::debug("[LuaGameAPI] Timer {} cancelled", timerId);
    }
}

void LuaGameAPI::updateTimers(float dt) {
    std::vector<std::uint64_t> toRemove;

    for (auto& timer : timers_) {
        timer.remaining -= dt;
        if (timer.remaining <= 0.0f) {
            // Execute callback
            auto result = timer.callback();
            if (!result.valid()) {
                sol::error err = result;
                spdlog::error("[Timer] Callback error: {}", err.what());
            }

            if (timer.repeating) {
                timer.remaining = timer.interval;
            } else {
                toRemove.push_back(timer.id);
            }
        }
    }

    for (auto id : toRemove) {
        cancelTimer(id);
    }
}

// ============================================================================
// Events
// ============================================================================

void LuaGameAPI::on(const std::string& eventName, sol::function callback) {
    eventSubscriptions_.push_back({eventName, std::move(callback)});
    spdlog::debug("[LuaGameAPI] Subscribed to event: {}", eventName);
}

void LuaGameAPI::emit(const std::string& eventName, sol::table data) {
    for (const auto& sub : eventSubscriptions_) {
        if (sub.eventName == eventName) {
            auto result = sub.callback(data);
            if (!result.valid()) {
                sol::error err = result;
                spdlog::error("[Event] Callback error for '{}': {}", eventName, err.what());
            }
        }
    }
}

// ============================================================================
// Game Loop Integration
// ============================================================================

void LuaGameAPI::update(float dt) {
    // Update timers
    updateTimers(dt);

    // Call Lua game:update(dt) if defined
    sol::table game = lua_["game"];
    if (game.valid() && game["update"].valid()) {
        sol::function updateFn = game["update"];
        auto result = updateFn(game, dt);
        if (!result.valid()) {
            sol::error err = result;
            spdlog::error("[Game] update() error: {}", err.what());
        }
    }
}

}  // namespace bestow::runtime

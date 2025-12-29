// bestow-lua/src/LuaBindings.cpp
// Engine system bindings for Lua

module;

#include <sol/sol.hpp>
#include <spdlog/spdlog.h>

module bestow.lua.impl;

import std;
import bestow.services;

namespace bestow {

//==============================================================================
// Binding Setup
//==============================================================================

void LuaRuntime::setupBindings() {
    // Create the engine namespace
    sol::table engine = lua_.create_named_table("engine");

    //==========================================================================
    // Core Types
    //==========================================================================

    // Vec2 type
    lua_.new_usertype<Vec2>("Vec2",
        sol::constructors<Vec2(), Vec2(float, float)>(),
        "x", &Vec2::x,
        "y", &Vec2::y,
        sol::meta_function::addition, [](const Vec2& a, const Vec2& b) {
            return Vec2{a.x + b.x, a.y + b.y};
        },
        sol::meta_function::subtraction, [](const Vec2& a, const Vec2& b) {
            return Vec2{a.x - b.x, a.y - b.y};
        },
        sol::meta_function::multiplication, [](const Vec2& a, float s) {
            return Vec2{a.x * s, a.y * s};
        },
        "length", [](const Vec2& v) {
            return std::sqrt(v.x * v.x + v.y * v.y);
        },
        "lengthSquared", [](const Vec2& v) {
            return v.x * v.x + v.y * v.y;
        },
        "normalized", [](const Vec2& v) {
            float len = std::sqrt(v.x * v.x + v.y * v.y);
            if (len < 0.0001f) return Vec2{0.0f, 0.0f};
            return Vec2{v.x / len, v.y / len};
        },
        "dot", [](const Vec2& a, const Vec2& b) {
            return a.x * b.x + a.y * b.y;
        }
    );

    // Vec3 type
    lua_.new_usertype<Vec3>("Vec3",
        sol::constructors<Vec3(), Vec3(float, float, float)>(),
        "x", &Vec3::x,
        "y", &Vec3::y,
        "z", &Vec3::z,
        sol::meta_function::addition, [](const Vec3& a, const Vec3& b) {
            return Vec3{a.x + b.x, a.y + b.y, a.z + b.z};
        },
        sol::meta_function::subtraction, [](const Vec3& a, const Vec3& b) {
            return Vec3{a.x - b.x, a.y - b.y, a.z - b.z};
        },
        sol::meta_function::multiplication, [](const Vec3& a, float s) {
            return Vec3{a.x * s, a.y * s, a.z * s};
        },
        "length", [](const Vec3& v) {
            return std::sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
        },
        "normalized", [](const Vec3& v) {
            float len = std::sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
            if (len < 0.0001f) return Vec3{0.0f, 0.0f, 0.0f};
            return Vec3{v.x / len, v.y / len, v.z / len};
        },
        "dot", [](const Vec3& a, const Vec3& b) {
            return a.x * b.x + a.y * b.y + a.z * b.z;
        },
        "cross", [](const Vec3& a, const Vec3& b) {
            return Vec3{
                a.y * b.z - a.z * b.y,
                a.z * b.x - a.x * b.z,
                a.x * b.y - a.y * b.x
            };
        }
    );

    // Color type (uint8_t components, but we provide float factory for Lua convenience)
    lua_.new_usertype<Color>("Color",
        sol::constructors<Color()>(),
        "r", &Color::r,
        "g", &Color::g,
        "b", &Color::b,
        "a", &Color::a,
        "fromFloat", sol::factories([](float r, float g, float b, float a) {
            return Color::fromFloat(r, g, b, a);
        }),
        "fromBytes", sol::factories([](int r, int g, int b, int a) {
            return Color{
                static_cast<std::uint8_t>(r),
                static_cast<std::uint8_t>(g),
                static_cast<std::uint8_t>(b),
                static_cast<std::uint8_t>(a)
            };
        }),
        "white", sol::var(Color::white()),
        "black", sol::var(Color::black()),
        "red", sol::var(Color::red()),
        "green", sol::var(Color::green()),
        "blue", sol::var(Color::blue())
    );

    // Rect type
    lua_.new_usertype<Rect>("Rect",
        sol::constructors<Rect(), Rect(float, float, float, float)>(),
        "x", &Rect::x,
        "y", &Rect::y,
        "width", &Rect::width,
        "height", &Rect::height,
        "contains", &Rect::contains,
        "intersects", &Rect::intersects
    );

    //==========================================================================
    // Entity System Bindings
    //==========================================================================

    engine["spawn"] = [this](std::string_view blueprintName, float x, float y) -> sol::object {
        auto result = spawn(blueprintName, x, y);
        if (result) {
            return sol::make_object(lua_, createEntityWrapper(*result));
        }
        spdlog::error("[Lua] spawn failed: {}", result.error().message);
        return sol::lua_nil;
    };

    engine["spawnWithSize"] = [this](std::string_view blueprintName,
                                      float x, float y,
                                      float width, float height) -> sol::object {
        auto result = spawn(blueprintName, x, y, width, height);
        if (result) {
            return sol::make_object(lua_, createEntityWrapper(*result));
        }
        spdlog::error("[Lua] spawn failed: {}", result.error().message);
        return sol::lua_nil;
    };

    engine["createEntity"] = [this]() -> sol::object {
        if (!pIEntitySystem_) {
            spdlog::error("[Lua] EntitySystem not available");
            return sol::lua_nil;
        }
        Entity entity = pIEntitySystem_->createEntity();
        return sol::make_object(lua_, createEntityWrapper(entity));
    };

    engine["destroyEntity"] = [this](sol::table entityWrapper) {
        if (!pIEntitySystem_) return;

        sol::optional<std::uint32_t> id = entityWrapper["_id"];
        if (id) {
            Entity entity = static_cast<Entity>(*id);
            detachAllBehaviors(entity);
            pIEntitySystem_->destroyEntity(entity);
        }
    };

    engine["findByTag"] = [this](std::string_view tag) -> sol::table {
        sol::table results = lua_.create_table();
        if (!pIEntitySystem_) return results;

        int index = 1;
        pIEntitySystem_->each([this, &results, &index, tag](Entity entity) {
            if (auto* t = pIEntitySystem_->tryGet<Tag>(entity)) {
                if (t->value == tag) {
                    results[index++] = createEntityWrapper(entity);
                }
            }
        });
        return results;
    };

    engine["findByName"] = [this](std::string_view name) -> sol::object {
        if (!pIEntitySystem_) return sol::lua_nil;

        Entity found = Entity{};
        bool foundOne = false;
        pIEntitySystem_->each([this, &found, &foundOne, name](Entity entity) {
            if (foundOne) return;
            if (auto* n = pIEntitySystem_->tryGet<Name>(entity)) {
                if (n->value == name) {
                    found = entity;
                    foundOne = true;
                }
            }
        });

        if (foundOne) {
            return sol::make_object(lua_, createEntityWrapper(found));
        }
        return sol::lua_nil;
    };

    //==========================================================================
    // Behavior System Bindings
    //==========================================================================

    engine["attachBehavior"] = [this](sol::table entityWrapper, std::string_view behaviorName) {
        sol::optional<std::uint32_t> id = entityWrapper["_id"];
        if (!id) {
            spdlog::error("[Lua] attachBehavior: invalid entity wrapper");
            return;
        }

        Entity entity = static_cast<Entity>(*id);
        auto result = attachBehavior(entity, behaviorName);
        if (!result) {
            spdlog::error("[Lua] attachBehavior failed: {}", result.error().message);
        }
    };

    engine["detachBehavior"] = [this](sol::table entityWrapper, std::string_view behaviorName) {
        sol::optional<std::uint32_t> id = entityWrapper["_id"];
        if (!id) return;

        Entity entity = static_cast<Entity>(*id);
        auto it = behaviorsByName_.find(std::string(behaviorName));
        if (it != behaviorsByName_.end()) {
            detachBehavior(entity, it->second);
        }
    };

    //==========================================================================
    // Input Bindings (if available)
    //==========================================================================

    sol::table input = lua_.create_named_table("input");

    // Placeholder input bindings - will be connected to actual input system
    input["isKeyDown"] = [](std::string_view key) -> bool {
        // Will be populated when InputSystem is connected
        return false;
    };

    input["isKeyPressed"] = [](std::string_view key) -> bool {
        return false;
    };

    input["isKeyReleased"] = [](std::string_view key) -> bool {
        return false;
    };

    input["getMousePosition"] = []() -> sol::table {
        // Will be populated when InputSystem is connected
        return sol::table();
    };

    input["isMouseDown"] = [](int button) -> bool {
        return false;
    };

    input["getAxis"] = [](std::string_view axis) -> float {
        return 0.0f;
    };

    //==========================================================================
    // Audio Bindings (if available)
    //==========================================================================

    sol::table audio = lua_.create_named_table("audio");

    audio["playSound"] = [](std::string_view soundPath) {
        // Will be populated when AudioSystem is connected
        spdlog::debug("[Lua] audio.playSound: {}", soundPath);
    };

    audio["playMusic"] = [](std::string_view musicPath) {
        spdlog::debug("[Lua] audio.playMusic: {}", musicPath);
    };

    audio["stopMusic"] = []() {
        spdlog::debug("[Lua] audio.stopMusic");
    };

    audio["setVolume"] = [](float volume) {
        spdlog::debug("[Lua] audio.setVolume: {}", volume);
    };

    //==========================================================================
    // Timer Utilities
    //==========================================================================

    sol::table timer = lua_.create_named_table("timer");

    // Simple timer registry
    static std::vector<std::tuple<float, float, sol::function, bool>> timers;
    static std::uint64_t nextTimerId = 1;

    timer["after"] = [this](float delay, sol::function callback) -> std::uint64_t {
        // Add to timer list (will be called during update)
        timers.push_back({delay, delay, callback, false});
        return nextTimerId++;
    };

    timer["every"] = [this](float interval, sol::function callback) -> std::uint64_t {
        // Repeating timer
        timers.push_back({interval, interval, callback, true});
        return nextTimerId++;
    };

    // Timer update is handled in the main update loop

    //==========================================================================
    // Math Utilities
    //==========================================================================

    sol::table mathExt = lua_.create_named_table("mathx");

    mathExt["lerp"] = [](float a, float b, float t) {
        return a + (b - a) * t;
    };

    mathExt["clamp"] = [](float value, float minVal, float maxVal) {
        return std::max(minVal, std::min(maxVal, value));
    };

    mathExt["sign"] = [](float value) {
        return value > 0.0f ? 1.0f : (value < 0.0f ? -1.0f : 0.0f);
    };

    mathExt["smoothstep"] = [](float edge0, float edge1, float x) {
        float t = std::clamp((x - edge0) / (edge1 - edge0), 0.0f, 1.0f);
        return t * t * (3.0f - 2.0f * t);
    };

    mathExt["distance"] = [](float x1, float y1, float x2, float y2) {
        float dx = x2 - x1;
        float dy = y2 - y1;
        return std::sqrt(dx * dx + dy * dy);
    };

    mathExt["angle"] = [](float x1, float y1, float x2, float y2) {
        return std::atan2(y2 - y1, x2 - x1);
    };

    mathExt["randomFloat"] = [](float min, float max) {
        static std::random_device rd;
        static std::mt19937 gen(rd());
        std::uniform_real_distribution<float> dist(min, max);
        return dist(gen);
    };

    mathExt["randomInt"] = [](int min, int max) {
        static std::random_device rd;
        static std::mt19937 gen(rd());
        std::uniform_int_distribution<int> dist(min, max);
        return dist(gen);
    };

    //==========================================================================
    // Debug Utilities
    //==========================================================================

    sol::table debug = lua_.create_named_table("debug");

    debug["log"] = [](std::string_view message) {
        spdlog::info("[Lua Debug] {}", message);
    };

    debug["warn"] = [](std::string_view message) {
        spdlog::warn("[Lua Debug] {}", message);
    };

    debug["error"] = [](std::string_view message) {
        spdlog::error("[Lua Debug] {}", message);
    };

    //==========================================================================
    // Config Access
    //==========================================================================

    sol::table config = lua_.create_named_table("config");

    config["getFloat"] = [this](std::string_view key, sol::optional<float> defaultVal) {
        return getFloatOr(key, defaultVal.value_or(0.0f));
    };

    config["getInt"] = [this](std::string_view key, sol::optional<int> defaultVal) {
        return getIntOr(key, defaultVal.value_or(0));
    };

    config["getBool"] = [this](std::string_view key, sol::optional<bool> defaultVal) {
        return getBoolOr(key, defaultVal.value_or(false));
    };

    config["getString"] = [this](std::string_view key, sol::optional<std::string> defaultVal) {
        return getStringOr(key, defaultVal.value_or(""));
    };

    config["set"] = [this](std::string_view key, sol::object value) {
        if (value.is<float>() || value.is<double>()) {
            setFloat(key, value.as<float>());
        } else if (value.is<int>() || value.is<std::int64_t>()) {
            setInt(key, value.as<int>());
        } else if (value.is<bool>()) {
            setBool(key, value.as<bool>());
        } else if (value.is<std::string>()) {
            setString(key, value.as<std::string>());
        }
    };

    //==========================================================================
    // Blueprint Access
    //==========================================================================

    engine["hasBlueprint"] = [this](std::string_view name) {
        return hasBlueprint(name);
    };

    engine["getBlueprintNames"] = [this]() {
        return getBlueprintNames();
    };

    //==========================================================================
    // Events (simple pub/sub)
    //==========================================================================

    sol::table events = lua_.create_named_table("events");

    // Store event listeners in Lua table to preserve them
    lua_["_eventListeners"] = lua_.create_table();

    events["on"] = [this](std::string eventName, sol::function callback) -> std::uint64_t {
        sol::table listeners = lua_["_eventListeners"];
        sol::optional<sol::table> eventTable = listeners[eventName];

        if (!eventTable) {
            listeners[eventName] = lua_.create_table();
        }

        static std::uint64_t nextEventId = 1;
        std::uint64_t id = nextEventId++;
        listeners[eventName][id] = callback;
        return id;
    };

    events["off"] = [this](std::string eventName, std::uint64_t id) {
        sol::table listeners = lua_["_eventListeners"];
        sol::optional<sol::table> eventTable = listeners[eventName];
        if (eventTable) {
            (*eventTable)[id] = sol::lua_nil;
        }
    };

    events["emit"] = [this](std::string eventName, sol::variadic_args args) {
        sol::table listeners = lua_["_eventListeners"];
        sol::optional<sol::table> eventTable = listeners[eventName];

        if (eventTable) {
            for (auto& [key, value] : *eventTable) {
                if (value.is<sol::function>()) {
                    sol::function fn = value.as<sol::function>();
                    auto result = fn(args);
                    if (!result.valid()) {
                        sol::error err = result;
                        spdlog::error("[Lua] Event '{}' handler error: {}",
                                      eventName, err.what());
                    }
                }
            }
        }
    };

    spdlog::debug("[LuaRuntime] Bindings configured");
}

}  // namespace bestow

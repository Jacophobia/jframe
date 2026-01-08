// bestow-luabind/src/LuaContractBinder.cpp
// Main LuaContractBinder implementation

module;

#include <bestow/sol2_compat.hpp>
#include <spdlog/spdlog.h>

#ifdef TRACY_ENABLE
#include <tracy/Tracy.hpp>
#endif

module bestow.luabind;

import std;
import bestow.utils;  // For FrameTimer

namespace bestow {

//=============================================================================
// Constructor
//=============================================================================

LuaContractBinder::LuaContractBinder(core::Engine& engine, sol::state& lua)
    : engine_(&engine), lua_(&lua) {}

//=============================================================================
// Binding Methods
//=============================================================================

void LuaContractBinder::bindAll() {
    if (initialized_) {
        spdlog::warn("[LuaContractBinder] Already initialized, rebinding...");
        boundSystems_.clear();
    }

    spdlog::info("[LuaContractBinder] Binding C++ contracts to Lua...");

    // Create the bestow table
    createBestowTable();

    // Bind core types first (Vec2, Vec3, Color, etc.)
    bindTypes(*lua_);
    spdlog::debug("[LuaContractBinder] Bound core types");

    // Bind core utilities (deltaTime, etc.)
    bindCoreUtilities();

    // Bind each registered system
    if (engine_->has<IInputSystem>()) {
        bindInputSystem(*lua_, engine_->get<IInputSystem>());
        boundSystems_.push_back("input");
        spdlog::debug("[LuaContractBinder] Bound IInputSystem -> bestow.input");

        // Bind ActionBuilder and event-driven input API (extends bestow.input)
        bindActionBuilder(*lua_, engine_->get<IInputSystem>());
        boundSystems_.push_back("action");
        spdlog::debug("[LuaContractBinder] Bound ActionBuilder -> bestow.action, bestow.phase");
    }

    if (engine_->has<IAudioSystem>()) {
        bindAudioSystem(*lua_, engine_->get<IAudioSystem>());
        boundSystems_.push_back("audio");
        spdlog::debug("[LuaContractBinder] Bound IAudioSystem -> bestow.audio");
    }

    if (engine_->has<IPhysicsSystem>()) {
        bindPhysicsSystem(*lua_, engine_->get<IPhysicsSystem>());
        boundSystems_.push_back("physics");
        spdlog::debug("[LuaContractBinder] Bound IPhysicsSystem -> bestow.physics");
    }

    // 3D Systems
    if (engine_->has<IPhysics3DSystem>()) {
        bindPhysics3DSystem(*lua_, engine_->get<IPhysics3DSystem>());
        boundSystems_.push_back("physics3d");
        spdlog::debug("[LuaContractBinder] Bound IPhysics3DSystem -> bestow.physics3d");
    }

    if (engine_->has<IGraphics3DSystem>()) {
        bindGraphics3DSystem(*lua_, engine_->get<IGraphics3DSystem>());
        boundSystems_.push_back("graphics3d");
        spdlog::debug("[LuaContractBinder] Bound IGraphics3DSystem -> bestow.graphics3d");
    }

    if (engine_->has<IAnimationSystem>()) {
        bindAnimationSystem(*lua_, engine_->get<IAnimationSystem>());
        boundSystems_.push_back("animation");
        spdlog::debug("[LuaContractBinder] Bound IAnimationSystem -> bestow.animation");
    }

    // Entity System (with reflection-based component access)
    if (engine_->has<IEntitySystem>()) {
        bindEntitySystem(*lua_, engine_->get<IEntitySystem>());
        boundSystems_.push_back("entity");
        spdlog::debug("[LuaContractBinder] Bound IEntitySystem -> bestow.entity");
    }

    // Config System
    if (engine_->has<IConfigSystem>()) {
        bindConfigSystem(*lua_, engine_->get<IConfigSystem>());
        boundSystems_.push_back("config");
        spdlog::debug("[LuaContractBinder] Bound IConfigSystem -> bestow.config");
    }

    // Asset System
    if (engine_->has<IAssetSystem>()) {
        bindAssetSystem(*lua_, engine_->get<IAssetSystem>());
        boundSystems_.push_back("assets");
        spdlog::debug("[LuaContractBinder] Bound IAssetSystem -> bestow.assets");
    }

    // Gameplay Ability System
    if (engine_->has<IGASSystem>()) {
        bindGASSystem(*lua_, engine_->get<IGASSystem>());
        boundSystems_.push_back("gas");
        spdlog::debug("[LuaContractBinder] Bound IGASSystem -> bestow.gas");
    }

    // Level System
    if (engine_->has<ILevelSystem>()) {
        bindLevelSystem(*lua_, engine_->get<ILevelSystem>());
        boundSystems_.push_back("level");
        spdlog::debug("[LuaContractBinder] Bound ILevelSystem -> bestow.level");
    }

    // Blueprint Factory
    if (engine_->has<IBlueprintFactory>()) {
        bindBlueprintFactory(*lua_, engine_->get<IBlueprintFactory>());
        boundSystems_.push_back("blueprints");
        spdlog::debug("[LuaContractBinder] Bound IBlueprintFactory -> bestow.blueprints");
    }

    // Game State System
    if (engine_->has<IGameStateSystem>()) {
        bindGameStateSystem(*lua_, engine_->get<IGameStateSystem>());
        boundSystems_.push_back("gamestate");
        spdlog::debug("[LuaContractBinder] Bound IGameStateSystem -> bestow.gamestate");
    }

    // Save System
    if (engine_->has<ISaveSystem>()) {
        bindSaveSystem(*lua_, engine_->get<ISaveSystem>());
        boundSystems_.push_back("save");
        spdlog::debug("[LuaContractBinder] Bound ISaveSystem -> bestow.save");
    }

    // Event System
    if (engine_->has<IEventSystem>()) {
        bindEventSystem(*lua_, engine_->get<IEventSystem>());
        boundSystems_.push_back("events");
        spdlog::debug("[LuaContractBinder] Bound IEventSystem -> bestow.events");
    }

    // UI System
    if (engine_->has<IUISystem>()) {
        bindUISystem(*lua_, engine_->get<IUISystem>());
        boundSystems_.push_back("ui");
        spdlog::debug("[LuaContractBinder] Bound IUISystem -> bestow.ui");
    }

    // Metrics/Profiling System (always available, doesn't require a contract)
    bindMetricsSystem(*lua_);
    boundSystems_.push_back("metrics");
    spdlog::debug("[LuaContractBinder] Bound MetricsCollector -> bestow.metrics");

    // Timer System (hot-reload-safe timers, always available)
    bindTimerSystem(*lua_);
    boundSystems_.push_back("timer");
    spdlog::debug("[LuaContractBinder] Bound TimerSystem -> bestow.timer");

    // Future bindings will be added here as they're implemented:
    // - IAISystem -> bestow.ai
    // - ICameraSystem -> bestow.camera

    initialized_ = true;
    spdlog::info("[LuaContractBinder] Bound {} systems to Lua", boundSystems_.size());
}

void LuaContractBinder::bindTypesOnly() {
    createBestowTable();
    bindTypes(*lua_);
    spdlog::info("[LuaContractBinder] Bound core types only");
}

//=============================================================================
// Private Helpers
//=============================================================================

void LuaContractBinder::createBestowTable() {
    // Create the bestow global table if it doesn't exist
    if (!(*lua_)["bestow"].valid()) {
        (*lua_)["bestow"] = lua_->create_table();
    }
}

void LuaContractBinder::bindCoreUtilities() {
    sol::table bestow = (*lua_)["bestow"];

    // Create bestow.core table for utility functions
    sol::table core = lua_->create_table();

    // Use a shared_ptr to FrameTimer so it persists for the lambda captures
    auto frameTimer = std::make_shared<utils::FrameTimer>();

    // deltaTime() returns time since last call (for game loop timing)
    core["deltaTime"] = [frameTimer]() -> float {
        return frameTimer->tick();
    };

    // frameCount() returns total frames elapsed
    core["frameCount"] = [frameTimer]() -> std::uint64_t {
        return frameTimer->frameCount();
    };

    // Version info
    core["version"] = "1.0.0";
    core["engine"] = "Bestow";

    bestow["core"] = core;

    // Create bestow.util table for safe utility functions
    sol::table util = lua_->create_table();

    // time() - returns current Unix timestamp (like os.time())
    util["time"] = []() -> std::int64_t {
        auto now = std::chrono::system_clock::now();
        return std::chrono::duration_cast<std::chrono::seconds>(
            now.time_since_epoch()).count();
    };

    // clock() - returns high-precision time for benchmarking
    util["clock"] = []() -> double {
        auto now = std::chrono::high_resolution_clock::now();
        return std::chrono::duration<double>(now.time_since_epoch()).count();
    };

    // date() - returns formatted date string
    util["date"] = [](sol::optional<std::string> format) -> std::string {
        auto now = std::chrono::system_clock::now();
        auto time_t = std::chrono::system_clock::to_time_t(now);
        std::tm tm = *std::localtime(&time_t);

        std::ostringstream oss;
        oss << std::put_time(&tm, format.value_or("%Y-%m-%d %H:%M:%S").c_str());
        return oss.str();
    };

    bestow["util"] = util;

    //-------------------------------------------------------------------------
    // Profiler table (Tracy integration)
    //-------------------------------------------------------------------------
    sol::table profiler = lua_->create_table();

    // frameMark() - Mark end of frame for Tracy
    profiler["frameMark"] = []() {
#ifdef TRACY_ENABLE
        FrameMark;
#endif
    };

    // plot(name, value) - Send named plot data to Tracy
    profiler["plot"] = [](const std::string& name, double value) {
#ifdef TRACY_ENABLE
        TracyPlot(name.c_str(), value);
#else
        (void)name;
        (void)value;
#endif
    };

    // message(text) - Send log message to Tracy
    profiler["message"] = [](const std::string& text) {
#ifdef TRACY_ENABLE
        TracyMessage(text.c_str(), text.size());
#else
        (void)text;
#endif
    };

    // isEnabled() - Check if Tracy profiling is enabled
    profiler["isEnabled"] = []() -> bool {
#ifdef TRACY_ENABLE
        return true;
#else
        return false;
#endif
    };

    bestow["profiler"] = profiler;

    // Helper to get Lua source location from call stack
    auto getLuaLocation = [this]() -> std::string {
        lua_Debug ar;
        // Level 2: skip the C function and our lambda wrapper
        if (lua_getstack(lua_->lua_state(), 2, &ar)) {
            lua_getinfo(lua_->lua_state(), "Sl", &ar);
            if (ar.currentline > 0) {
                std::string source = ar.source ? ar.source : "unknown";
                // Remove @ prefix if present (indicates filename)
                if (!source.empty() && source[0] == '@') {
                    source = source.substr(1);
                }
                // If source looks like file content (starts with -- or has newlines), use short_src
                if (source.find('\n') != std::string::npos ||
                    (source.size() > 2 && source[0] == '-' && source[1] == '-')) {
                    source = ar.short_src[0] != '\0' ? ar.short_src : "chunk";
                }
                // Extract just the filename from path
                auto lastSlash = source.find_last_of("/\\");
                if (lastSlash != std::string::npos) {
                    source = source.substr(lastSlash + 1);
                }
                return std::format("{}:{}", source, ar.currentline);
            }
        }
        return "unknown";
    };

    // Logging functions backed by spdlog
    // bestow.debug(...) - variadic, concatenates arguments with spaces
    bestow["debug"] = [this, getLuaLocation](sol::variadic_args va) {
        std::ostringstream oss;
        bool first = true;
        for (auto v : va) {
            if (!first) oss << " ";
            first = false;
            // Convert each argument to string
            sol::object obj = v;
            if (obj.is<std::string>()) {
                oss << obj.as<std::string>();
            } else if (obj.is<double>()) {
                oss << obj.as<double>();
            } else if (obj.is<bool>()) {
                oss << (obj.as<bool>() ? "true" : "false");
            } else if (obj.is<sol::nil_t>()) {
                oss << "nil";
            } else {
                oss << "[" << sol::type_name(lua_->lua_state(), obj.get_type()) << "]";
            }
        }
        spdlog::debug("[Lua:{}] {}", getLuaLocation(), oss.str());
    };

    // bestow.info(...) - informational logging
    bestow["info"] = [this, getLuaLocation](sol::variadic_args va) {
        std::ostringstream oss;
        bool first = true;
        for (auto v : va) {
            if (!first) oss << " ";
            first = false;
            sol::object obj = v;
            if (obj.is<std::string>()) {
                oss << obj.as<std::string>();
            } else if (obj.is<double>()) {
                oss << obj.as<double>();
            } else if (obj.is<bool>()) {
                oss << (obj.as<bool>() ? "true" : "false");
            } else if (obj.is<sol::nil_t>()) {
                oss << "nil";
            } else {
                oss << "[" << sol::type_name(lua_->lua_state(), obj.get_type()) << "]";
            }
        }
        spdlog::info("[Lua:{}] {}", getLuaLocation(), oss.str());
    };

    // bestow.warn(...) - warning logging
    bestow["warn"] = [this, getLuaLocation](sol::variadic_args va) {
        std::ostringstream oss;
        bool first = true;
        for (auto v : va) {
            if (!first) oss << " ";
            first = false;
            sol::object obj = v;
            if (obj.is<std::string>()) {
                oss << obj.as<std::string>();
            } else if (obj.is<double>()) {
                oss << obj.as<double>();
            } else if (obj.is<bool>()) {
                oss << (obj.as<bool>() ? "true" : "false");
            } else if (obj.is<sol::nil_t>()) {
                oss << "nil";
            } else {
                oss << "[" << sol::type_name(lua_->lua_state(), obj.get_type()) << "]";
            }
        }
        spdlog::warn("[Lua:{}] {}", getLuaLocation(), oss.str());
    };

    // bestow.error(...) - error logging
    bestow["error"] = [this, getLuaLocation](sol::variadic_args va) {
        std::ostringstream oss;
        bool first = true;
        for (auto v : va) {
            if (!first) oss << " ";
            first = false;
            sol::object obj = v;
            if (obj.is<std::string>()) {
                oss << obj.as<std::string>();
            } else if (obj.is<double>()) {
                oss << obj.as<double>();
            } else if (obj.is<bool>()) {
                oss << (obj.as<bool>() ? "true" : "false");
            } else if (obj.is<sol::nil_t>()) {
                oss << "nil";
            } else {
                oss << "[" << sol::type_name(lua_->lua_state(), obj.get_type()) << "]";
            }
        }
        spdlog::error("[Lua:{}] {}", getLuaLocation(), oss.str());
    };

    // Disable print() with helpful error message
    (*lua_)["print"] = [](sol::variadic_args) {
        throw std::runtime_error(
            R"(print() is disabled in Bestow.

Use the bestow logging functions instead:

  bestow.debug("Debug message", value)   -- For development debugging
  bestow.info("Info message", value)     -- For informational output
  bestow.warn("Warning message", value)  -- For warnings
  bestow.error("Error message", value)   -- For errors

These functions automatically include timestamps and source location.
Log level can be controlled with the --verbose flag.)");
    };
}

//=============================================================================
// Stub Generation
//=============================================================================

void LuaContractBinder::generateStubs(const std::filesystem::path& outputDir) {
    StubGenerator generator;
    generator.generate(outputDir);
}

//=============================================================================
// Queries
//=============================================================================

bool LuaContractBinder::isSystemBound(const std::string& systemName) const {
    return std::find(boundSystems_.begin(), boundSystems_.end(), systemName) != boundSystems_.end();
}

std::vector<std::string> LuaContractBinder::getBoundSystems() const {
    return boundSystems_;
}

}  // namespace bestow

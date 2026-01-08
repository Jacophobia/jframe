// bestow-luabind/src/LuaErrorUtils.cpp
// Utilities for better Lua error messages with source locations and stack traces

module;

#include <bestow/sol2_compat.hpp>
#include <spdlog/spdlog.h>

module bestow.luabind;

import std;

namespace bestow {

//=============================================================================
// LuaErrorContext - Captures Lua call context for error messages
//=============================================================================

LuaErrorContext LuaErrorContext::capture(lua_State* L, int startLevel) {
    LuaErrorContext ctx;
    lua_Debug ar;

    // Get the immediate caller location
    if (lua_getstack(L, startLevel, &ar)) {
        lua_getinfo(L, "Sln", &ar);

        if (ar.currentline > 0) {
            ctx.file = ar.source ? ar.source : "unknown";
            // Remove @ prefix if present
            if (!ctx.file.empty() && ctx.file[0] == '@') {
                ctx.file = ctx.file.substr(1);
            }
            // Use short_src if source looks like content
            if (ctx.file.find('\n') != std::string::npos) {
                ctx.file = ar.short_src[0] != '\0' ? ar.short_src : "chunk";
            }
            ctx.line = ar.currentline;
            ctx.functionName = ar.name ? ar.name : "";
        }
    }

    // Build stack trace (up to 10 levels)
    for (int level = startLevel; level < startLevel + 10; ++level) {
        if (!lua_getstack(L, level, &ar)) break;
        lua_getinfo(L, "Sln", &ar);

        std::string frame;
        std::string source = ar.source ? ar.source : "?";
        if (!source.empty() && source[0] == '@') {
            source = source.substr(1);
        }
        // Extract filename only
        auto lastSlash = source.find_last_of("/\\");
        if (lastSlash != std::string::npos) {
            source = source.substr(lastSlash + 1);
        }

        if (ar.currentline > 0) {
            if (ar.name) {
                frame = std::format("  {}:{}: in function '{}'", source, ar.currentline, ar.name);
            } else {
                frame = std::format("  {}:{}: in main chunk", source, ar.currentline);
            }
        } else if (ar.name) {
            frame = std::format("  [C]: in function '{}'", ar.name);
        } else {
            frame = "  [C]: in ?";
        }

        ctx.stackTrace.push_back(std::move(frame));
    }

    return ctx;
}

std::string LuaErrorContext::location() const {
    if (file.empty() || line == 0) {
        return "unknown location";
    }
    // Extract just filename
    std::string filename = file;
    auto lastSlash = filename.find_last_of("/\\");
    if (lastSlash != std::string::npos) {
        filename = filename.substr(lastSlash + 1);
    }
    return std::format("{}:{}", filename, line);
}

std::string LuaErrorContext::formatStackTrace() const {
    if (stackTrace.empty()) {
        return "";
    }
    std::ostringstream oss;
    oss << "Stack trace:\n";
    for (const auto& frame : stackTrace) {
        oss << frame << "\n";
    }
    return oss.str();
}

//=============================================================================
// luaError - Throw an error with full context
//=============================================================================

void luaError(lua_State* L, const std::string& message) {
    auto ctx = LuaErrorContext::capture(L);

    std::string fullMessage = std::format(
        "{}\n\n"
        "Location: {}\n"
        "{}",
        message,
        ctx.location(),
        ctx.formatStackTrace()
    );

    throw std::runtime_error(fullMessage);
}

void luaError(lua_State* L, const std::string& apiName, const std::string& message) {
    auto ctx = LuaErrorContext::capture(L);

    std::string fullMessage = std::format(
        "[{}] {}\n\n"
        "Location: {}\n"
        "{}",
        apiName,
        message,
        ctx.location(),
        ctx.formatStackTrace()
    );

    throw std::runtime_error(fullMessage);
}

//=============================================================================
// luaTypeError - For type mismatch errors
//=============================================================================

void luaTypeError(lua_State* L, const std::string& apiName, int argNum,
                  const std::string& expected, const std::string& actual) {
    auto ctx = LuaErrorContext::capture(L);

    std::string message = std::format(
        "[{}] Type error in argument #{}: expected {}, got {}\n\n"
        "Location: {}\n"
        "{}",
        apiName, argNum, expected, actual,
        ctx.location(),
        ctx.formatStackTrace()
    );

    throw std::runtime_error(message);
}

void luaTypeError(lua_State* L, const std::string& apiName, int argNum,
                  const std::string& expected, sol::type actual) {
    luaTypeError(L, apiName, argNum, expected, sol::type_name(L, actual));
}

//=============================================================================
// luaArgError - For argument validation errors
//=============================================================================

void luaArgError(lua_State* L, const std::string& apiName, int argNum,
                 const std::string& requirement) {
    auto ctx = LuaErrorContext::capture(L);

    std::string message = std::format(
        "[{}] Invalid argument #{}: {}\n\n"
        "Location: {}\n"
        "{}",
        apiName, argNum, requirement,
        ctx.location(),
        ctx.formatStackTrace()
    );

    throw std::runtime_error(message);
}

//=============================================================================
// luaRangeError - For out-of-range values
//=============================================================================

void luaRangeError(lua_State* L, const std::string& apiName,
                   const std::string& paramName, double value,
                   double minVal, double maxVal) {
    auto ctx = LuaErrorContext::capture(L);

    std::string message = std::format(
        "[{}] Value out of range: {} = {} (expected {} to {})\n\n"
        "Location: {}\n"
        "{}",
        apiName, paramName, value, minVal, maxVal,
        ctx.location(),
        ctx.formatStackTrace()
    );

    throw std::runtime_error(message);
}

//=============================================================================
// luaNotFoundError - For missing resources
//=============================================================================

void luaNotFoundError(lua_State* L, const std::string& apiName,
                      const std::string& resourceType, const std::string& name) {
    auto ctx = LuaErrorContext::capture(L);

    std::string message = std::format(
        "[{}] {} not found: '{}'\n\n"
        "Location: {}\n"
        "{}",
        apiName, resourceType, name,
        ctx.location(),
        ctx.formatStackTrace()
    );

    throw std::runtime_error(message);
}

//=============================================================================
// getLuaLocation - Simple helper for logging (returns "file:line")
//=============================================================================

std::string getLuaLocation(lua_State* L, int level) {
    auto ctx = LuaErrorContext::capture(L, level);
    return ctx.location();
}

} // namespace bestow

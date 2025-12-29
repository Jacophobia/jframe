# bestow-lua/CMakeLists.txt
# Unified Lua Runtime - replaces bestow-config and bestow-blueprints

add_library(bestow-lua)

target_sources(bestow-lua
    PUBLIC
        FILE_SET CXX_MODULES FILES
            src/bestow.lua.impl.cppm
    PRIVATE
        src/LuaRuntime.cpp
        src/LuaBindings.cpp
        src/LuaBehaviors.cpp
        src/LuaBlueprints.cpp
)

target_link_libraries(bestow-lua
    PUBLIC
        bestow-contract
    PRIVATE
        lua
        sol2::sol2
        spdlog::spdlog
        kangaru::kangaru
        nlohmann_json::nlohmann_json
)

target_compile_features(bestow-lua PUBLIC cxx_std_23)

# Enable module support
set_target_properties(bestow-lua PROPERTIES
    CXX_SCAN_FOR_MODULES ON
)

# Enable import std; support
target_use_std_module(bestow-lua)

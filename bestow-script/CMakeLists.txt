# bestow-script/CMakeLists.txt
# Script management system - Lua script loading with hot reload and app.* namespace

add_library(bestow-script)

target_sources(bestow-script
    PUBLIC
        FILE_SET CXX_MODULES FILES
            src/bestow.script.cppm
    PRIVATE
        src/ScriptManager.cpp
        src/ScriptLinter.cpp
)

target_link_libraries(bestow-script
    PUBLIC
        bestow-contract
    PRIVATE
        lua
        sol2::sol2
        spdlog::spdlog
        efsw
)

target_compile_features(bestow-script PUBLIC cxx_std_23)

# Enable module support
set_target_properties(bestow-script PROPERTIES
    CXX_SCAN_FOR_MODULES ON
)

# Enable import std; support
target_use_std_module(bestow-script)

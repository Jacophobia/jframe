# bestow-config/CMakeLists.txt
# Config system - Lua-based configuration loading

add_library(bestow-config)

target_sources(bestow-config
    PUBLIC
        FILE_SET CXX_MODULES FILES
            src/bestow.config.impl.cppm
    PRIVATE
        src/ConfigSystem.cpp
)

target_link_libraries(bestow-config
    PUBLIC
        bestow-contract
    PRIVATE
        lua
        sol2::sol2
        spdlog::spdlog
        kangaru::kangaru
)

target_compile_features(bestow-config PUBLIC cxx_std_23)

# Enable module support
set_target_properties(bestow-config PROPERTIES
    CXX_SCAN_FOR_MODULES ON
)

# Enable import std; support
target_use_std_module(bestow-config)

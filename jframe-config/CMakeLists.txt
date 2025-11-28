# jframe-config/CMakeLists.txt
# Config system - Lua-based configuration loading

add_library(jframe-config)

target_sources(jframe-config
    PUBLIC
        FILE_SET CXX_MODULES FILES
            src/jframe.config.impl.cppm
    PRIVATE
        src/ConfigSystem.cpp
)

target_link_libraries(jframe-config
    PUBLIC
        jframe-contract
    PRIVATE
        lua
        sol2::sol2
        spdlog::spdlog
)

target_compile_features(jframe-config PUBLIC cxx_std_23)

# Enable module support
set_target_properties(jframe-config PROPERTIES
    CXX_SCAN_FOR_MODULES ON
)

# Enable import std; support
target_use_std_module(jframe-config)

# bestow-luabind/CMakeLists.txt
# Lua contract bindings - Exposes C++ contracts to Lua via bestow.* namespace

add_library(bestow-luabind)

target_sources(bestow-luabind
    PUBLIC
        FILE_SET CXX_MODULES FILES
            src/bestow.luabind.cppm
    PRIVATE
        src/LuaContractBinder.cpp
        src/bindings/types_binding.cpp
        src/bindings/input_binding.cpp
        src/bindings/audio_binding.cpp
        src/bindings/physics_binding.cpp
        src/bindings/physics3d_binding.cpp
        src/bindings/graphics3d_binding.cpp
        src/bindings/animation_binding.cpp
        src/bindings/entity_binding.cpp
        src/bindings/config_binding.cpp
        src/bindings/assets_binding.cpp
        src/bindings/gas_binding.cpp
        src/bindings/level_binding.cpp
        src/bindings/blueprints_binding.cpp
        src/bindings/gamestate_binding.cpp
        src/bindings/save_binding.cpp
        src/bindings/events_binding.cpp
        src/bindings/metrics_binding.cpp
        src/stubs/StubGenerator.cpp
)

target_link_libraries(bestow-luabind
    PUBLIC
        bestow-contract
        bestow-core
    PRIVATE
        bestow-metrics   # For Tracy profiling support
        bestow-gas       # For GAS bindings
        bestow-level     # For Level bindings
        bestow-blueprints # For Blueprint bindings
        bestow-gamestate # For GameState bindings
        bestow-save      # For Save bindings
        bestow-events    # For Events bindings
        lua
        sol2::sol2
        spdlog::spdlog
)

# Tracy include directory for profiler bindings
if(BESTOW_ENABLE_TRACY AND TRACY_INCLUDE_DIR)
    target_include_directories(bestow-luabind PRIVATE "${TRACY_INCLUDE_DIR}")
endif()

target_compile_features(bestow-luabind PUBLIC cxx_std_23)

# Enable module support
set_target_properties(bestow-luabind PROPERTIES
    CXX_SCAN_FOR_MODULES ON
)

# Enable import std; support
target_use_std_module(bestow-luabind)

# bestow-luabind/CMakeLists.txt
# Lua contract bindings - Exposes C++ contracts to Lua via bestow.* namespace

add_library(bestow-luabind)

target_sources(bestow-luabind
    PUBLIC
        FILE_SET CXX_MODULES FILES
            src/bestow.luabind.cppm
    PRIVATE
        src/LuaContractBinder.cpp
        src/LuaErrorUtils.cpp
        src/bindings/types_binding.cpp
        src/bindings/input_binding.cpp
        src/bindings/action_binding.cpp
        src/bindings/audio_binding.cpp
        src/bindings/physics_binding.cpp
        src/bindings/physics3d_binding.cpp
        src/bindings/graphics3d_binding.cpp
        src/bindings/animation_binding.cpp
        src/bindings/character_binding.cpp
        src/bindings/entity_binding.cpp
        src/bindings/config_binding.cpp
        src/bindings/assets_binding.cpp
        src/bindings/gas_binding.cpp
        src/bindings/scene_binding.cpp
        src/bindings/blueprints_binding.cpp
        src/bindings/gamestate_binding.cpp
        src/bindings/state_binding.cpp
        src/bindings/events_binding.cpp
        src/bindings/ui_binding.cpp
        src/bindings/metrics_binding.cpp
        src/bindings/timer_binding.cpp
        src/stubs/StubGenerator.cpp
        src/docs/DocRegistry.cpp
        src/docs/DocFormatter.cpp
        src/docs/DocRegistration.cpp
        src/docs/types_binding_doc.cpp
        src/docs/core_binding_doc.cpp
        src/docs/input_binding_doc.cpp
        src/docs/action_binding_doc.cpp
        src/docs/audio_binding_doc.cpp
        src/docs/physics_binding_doc.cpp
        src/docs/physics3d_binding_doc.cpp
        src/docs/graphics3d_binding_doc.cpp
        src/docs/animation_binding_doc.cpp
        src/docs/character_binding_doc.cpp
        src/docs/entity_binding_doc.cpp
        src/docs/config_binding_doc.cpp
        src/docs/assets_binding_doc.cpp
        src/docs/gas_binding_doc.cpp
        src/docs/scene_binding_doc.cpp
        src/docs/blueprints_binding_doc.cpp
        src/docs/gamestate_binding_doc.cpp
        src/docs/state_binding_doc.cpp
        src/docs/events_binding_doc.cpp
        src/docs/ui_binding_doc.cpp
        src/docs/metrics_binding_doc.cpp
        src/docs/timer_binding_doc.cpp
)

target_link_libraries(bestow-luabind
    PUBLIC
        bestow-contract
        bestow-core
    PRIVATE
        bestow-metrics   # For Tracy profiling support
        bestow-gas       # For GAS bindings
        bestow-scene     # For Scene bindings
        bestow-blueprints # For Blueprint bindings
        bestow-gamestate # For GameState bindings
        bestow-state     # For State bindings
        bestow-events    # For Events bindings
        bestow-ui        # For UI bindings
        unofficial::luajit::luajit
        sol2::sol2
        spdlog::spdlog
)

# Configure sol2 for LuaJIT
target_compile_definitions(bestow-luabind PUBLIC
    SOL_LUAJIT=1
    SOL_USING_CXX_LUA=0
)

# Tracy include directory for profiler bindings
if(BESTOW_ENABLE_TRACY AND TRACY_INCLUDE_DIR)
    target_include_directories(bestow-luabind PRIVATE "${TRACY_INCLUDE_DIR}")
endif()

target_compile_features(bestow-luabind PUBLIC cxx_std_23)

# MSVC needs /bigobj for large translation units (sol2 templates)
if(MSVC)
    target_compile_options(bestow-luabind PRIVATE /bigobj)
endif()

# Enable module support
set_target_properties(bestow-luabind PROPERTIES
    CXX_SCAN_FOR_MODULES ON
)

# Enable import std; support
target_use_std_module(bestow-luabind)

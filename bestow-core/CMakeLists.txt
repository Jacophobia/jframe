add_library(bestow-core)
target_use_std_module(bestow-core)

target_sources(bestow-core
    PUBLIC FILE_SET CXX_MODULES FILES
        src/bestow.core.cppm
    PRIVATE
        src/Application.cpp
        src/Engine.cpp
        src/EngineBuilder.cpp
        src/JobSystem.cpp
        src/Easing.cpp
        src/Timer.cpp
)

target_link_libraries(bestow-core
    PUBLIC
        bestow-contract
        bestow-metrics
        spdlog::spdlog
        Taskflow::Taskflow
    PRIVATE
        bestow-events
        bestow-entity
        bestow-physics
        bestow-opengl
        bestow-audio
        bestow-input
        bestow-assets
        bestow-save
        bestow-level
        bestow-ai
        bestow-camera
        bestow-gas
        bestow-blueprints
)

# Tracy is already linked via bestow-metrics (PRIVATE), and TRACY_ENABLE
# definition propagates through bestow-metrics (PUBLIC)

target_compile_features(bestow-core PUBLIC cxx_std_23)

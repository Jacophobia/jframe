add_library(jframe-core)
target_use_std_module(jframe-core)

target_sources(jframe-core
    PUBLIC FILE_SET CXX_MODULES FILES
        src/jframe.core.cppm
    PRIVATE
        src/Application.cpp
        src/Engine.cpp
        src/EngineBuilder.cpp
        src/JobSystem.cpp
        src/Easing.cpp
        src/Timer.cpp
)

target_link_libraries(jframe-core
    PUBLIC
        jframe-contract
        spdlog::spdlog
        Taskflow::Taskflow
    PRIVATE
        jframe-events
        jframe-entity
        jframe-physics
        jframe-graphics
        jframe-audio
        jframe-input
        jframe-assets
        jframe-save
        jframe-level
        jframe-ai
        jframe-camera
        jframe-gas
        jframe-blueprints
)

if(JFRAME_ENABLE_TRACY)
    target_link_libraries(jframe-core PUBLIC tracy)
    target_compile_definitions(jframe-core PUBLIC TRACY_ENABLE)
endif()

target_compile_features(jframe-core PUBLIC cxx_std_23)

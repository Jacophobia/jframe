add_library(bestow-core)
target_use_std_module(bestow-core)

target_sources(bestow-core
    PUBLIC FILE_SET CXX_MODULES FILES
        src/bestow.core.cppm
    PRIVATE
        src/Application.cpp
        src/JobSystem.cpp
        src/Easing.cpp
        src/Timer.cpp
)

# Engine is now a simple composition root that uses Kangaru DI
# It doesn't need to link against all system implementations
# Clients import the implementations they need
target_link_libraries(bestow-core
    PUBLIC
        bestow-contract
        spdlog::spdlog
        Taskflow::Taskflow
)

# Tracy is already linked via bestow-metrics (PRIVATE), and TRACY_ENABLE
# definition propagates through bestow-metrics (PUBLIC)

target_compile_features(bestow-core PUBLIC cxx_std_23)

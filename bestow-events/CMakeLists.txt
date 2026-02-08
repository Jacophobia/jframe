add_library(bestow-events)
target_use_std_module(bestow-events)

target_sources(bestow-events
    PUBLIC FILE_SET CXX_MODULES FILES
        src/bestow.events.impl.cppm
    PRIVATE
        src/EventSystem.cpp
)

target_link_libraries(bestow-events
    PUBLIC
        bestow-contract
)

target_compile_features(bestow-events PUBLIC cxx_std_23)

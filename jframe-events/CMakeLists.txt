add_library(jframe-events)
target_use_std_module(jframe-events)

target_sources(jframe-events
    PUBLIC FILE_SET CXX_MODULES FILES
        src/jframe.events.impl.cppm
    PRIVATE
        src/EventSystem.cpp
)

target_link_libraries(jframe-events
    PUBLIC
        jframe-contract
)

target_compile_features(jframe-events PUBLIC cxx_std_23)

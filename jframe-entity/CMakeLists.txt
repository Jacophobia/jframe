add_library(jframe-entity)
target_use_std_module(jframe-entity)

target_sources(jframe-entity
    PUBLIC FILE_SET CXX_MODULES FILES
        src/jframe.entity.impl.cppm
    PRIVATE
        src/EntitySystem.cpp
)

target_link_libraries(jframe-entity
    PUBLIC
        jframe-contract
        EnTT::EnTT
)

target_compile_features(jframe-entity PUBLIC cxx_std_23)

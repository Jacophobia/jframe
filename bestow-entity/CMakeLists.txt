add_library(bestow-entity)
target_use_std_module(bestow-entity)

target_sources(bestow-entity
    PUBLIC FILE_SET CXX_MODULES FILES
        src/bestow.entity.impl.cppm
    PRIVATE
        src/EntitySystem.cpp
)

target_link_libraries(bestow-entity
    PUBLIC
        bestow-contract
        EnTT::EnTT
)

target_compile_features(bestow-entity PUBLIC cxx_std_23)

add_library(bestow-physics)
target_use_std_module(bestow-physics)

target_sources(bestow-physics
    PUBLIC FILE_SET CXX_MODULES FILES
        src/bestow.physics.impl.cppm
    PRIVATE
        src/Box2DPhysicsSystem.cpp
)

target_link_libraries(bestow-physics
    PUBLIC
        bestow-contract
        box2d::box2d
)

target_compile_features(bestow-physics PUBLIC cxx_std_23)

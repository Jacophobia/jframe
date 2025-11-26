add_library(jframe-physics)
target_use_std_module(jframe-physics)

target_sources(jframe-physics
    PUBLIC FILE_SET CXX_MODULES FILES
        src/jframe.physics.impl.cppm
    PRIVATE
        src/Box2DPhysicsSystem.cpp
)

target_link_libraries(jframe-physics
    PUBLIC
        jframe-contract
        box2d::box2d
)

target_compile_features(jframe-physics PUBLIC cxx_std_23)

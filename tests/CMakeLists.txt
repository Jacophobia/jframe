enable_testing()

add_executable(jframe-tests
    unit/CoreSystemTests.cpp
    unit/EntitySystemTests.cpp
    unit/SaveSystemTests.cpp
    unit/EventSystemTests.cpp
    unit/InputSystemTests.cpp
    unit/AssetSystemTests.cpp
    unit/AudioSystemTests.cpp
    unit/GraphicsSystemTests.cpp
    unit/PhysicsSystemTests.cpp
    unit/LevelSystemTests.cpp
)
target_use_std_module(jframe-tests)

target_link_libraries(jframe-tests
    PRIVATE
        jframe-contract
        jframe-core
        jframe-entity
        jframe-events
        jframe-assets
        jframe-save
        jframe-input
        jframe-audio
        jframe-graphics
        jframe-physics
        jframe-level
        GTest::gtest
        GTest::gtest_main
)

target_compile_features(jframe-tests PRIVATE cxx_std_23)

include(GoogleTest)
gtest_discover_tests(jframe-tests)

enable_testing()

# Include flaky tests configuration
include(${CMAKE_SOURCE_DIR}/cmake/FlakyTests.cmake)

# Configure the flaky test labeling script
configure_file(
    ${CMAKE_SOURCE_DIR}/cmake/LabelFlakyTests.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/LabelFlakyTests.cmake
    @ONLY
)

add_executable(jframe-tests
    unit/ComponentsTests.cpp
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
    unit/AISystemTests.cpp
    unit/CameraSystemTests.cpp
    unit/GASSystemTests.cpp
    unit/BlueprintFactoryTests.cpp
)
target_use_std_module(jframe-tests)

target_link_libraries(jframe-tests
    PRIVATE
        jframe-contract
        jframe-components
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
        jframe-ai
        jframe-camera
        jframe-gas
        jframe-blueprints
        GTest::gtest
        GTest::gtest_main
)

target_compile_features(jframe-tests PRIVATE cxx_std_23)

include(GoogleTest)

# Discover tests and apply flaky test labels
# The TEST_INCLUDE_FILES option includes our script after test discovery
# to label timing-dependent tests as "Flaky"
#
# Run tests excluding flaky ones (CI):  ctest --preset <preset> -LE Flaky
# Run all tests (local):                ctest --preset <preset>
# Run only flaky tests:                 ctest --preset <preset> -L Flaky
gtest_discover_tests(jframe-tests
    TEST_INCLUDE_FILES ${CMAKE_CURRENT_BINARY_DIR}/LabelFlakyTests.cmake
)

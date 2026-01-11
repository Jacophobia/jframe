enable_testing()

# Include flaky tests configuration
include(${CMAKE_SOURCE_DIR}/cmake/FlakyTests.cmake)

# Configure the flaky test labeling script
configure_file(
    ${CMAKE_SOURCE_DIR}/cmake/LabelFlakyTests.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/LabelFlakyTests.cmake
    @ONLY
)

add_executable(bestow-tests
    unit/ComponentsTests.cpp
    unit/CoreSystemTests.cpp
    unit/EntitySystemTests.cpp
    unit/SaveSystemTests.cpp
    unit/EventSystemTests.cpp
    unit/InputSystemTests.cpp
    unit/AssetSystemTests.cpp
    unit/AudioSystemTests.cpp
    unit/AnimationSystemTests.cpp
    unit/GraphicsSystemTests.cpp
    unit/Graphics3DSystemTests.cpp
    unit/PhysicsSystemTests.cpp
    unit/LevelSystemTests.cpp
    unit/AISystemTests.cpp
    unit/CameraSystemTests.cpp
    unit/GASSystemTests.cpp
    unit/BlueprintFactoryTests.cpp
    unit/UISystemTests.cpp
)
target_use_std_module(bestow-tests)

target_link_libraries(bestow-tests
    PRIVATE
        bestow-contract
        bestow-components
        bestow-core
        bestow-entity
        bestow-events
        bestow-assets
        bestow-save
        bestow-config
        bestow-input
        bestow-audio
        bestow-animation
        bestow-opengl
        bestow-physics
        bestow-level
        bestow-ai
        bestow-camera
        bestow-gas
        bestow-blueprints
        bestow-ui
        GTest::gtest
        GTest::gtest_main
)

# Note: No special linking needed for macOS - Xcode 16.2's SDK has native std::expected support

target_compile_features(bestow-tests PRIVATE cxx_std_23)

# MSVC workaround: force include entt_compat.hpp to fix ADL issues with C++23 modules
if(MSVC)
    target_compile_options(bestow-tests PRIVATE /FI"bestow/entt_compat.hpp")
    # Set working directory for Visual Studio debugger
    set_target_properties(bestow-tests PROPERTIES
        VS_DEBUGGER_WORKING_DIRECTORY "$<TARGET_FILE_DIR:bestow-tests>"
    )

    # Note: DLL deployment is handled via PATH in CI (see .github/workflows/ci.yml)
    # For local development, Visual Studio automatically deploys DLLs via vcpkg integration
endif()

include(GoogleTest)

# On Windows in CI, skip test discovery and run tests directly
# (Avoids test discovery timeout issues in CI environment)
# On other platforms, use normal test discovery
if(WIN32 AND DEFINED ENV{CI})
    # Add a single test that runs the executable directly with gtest filter
    # Exclude flaky/hardware-dependent tests (same as macOS/Linux)
    add_test(NAME bestow-tests-all
        COMMAND bestow-tests
            --gtest_filter=-FrameTimerTest.DeltaTimeConsistency:CameraSystemTest.ShakeDecaysOverTime:AssetSystemTest.CheckForReloadsDetectsModifiedFile:AssetSystemTest.HotReloadCallbackOnReload:AssetSystemTest.CheckForReloadsIgnoresAssetsBeingLoaded:EventSystemTest.UnsubscribeDuringCallback
    )
    set_tests_properties(bestow-tests-all PROPERTIES
        LABELS "AllTests"
    )
else()
    # Discover tests and apply flaky test labels
    # The TEST_INCLUDE_FILES option includes our script after test discovery
    # to label timing-dependent tests as "Flaky"
    #
    # Run tests excluding flaky ones (CI):  ctest --preset <preset> -LE Flaky
    # Run all tests (local):                ctest --preset <preset>
    # Run only flaky tests:                 ctest --preset <preset> -L Flaky

    # On macOS CI, use a simple add_test instead of gtest_discover_tests
    # to avoid test discovery timeout issues in headless environment
    if(APPLE AND DEFINED ENV{CI})
        # Pass gtest filter to exclude hardware-dependent tests
        # Same exclusions as Linux CI (see ci.yml)
        add_test(NAME bestow-tests-ci
            COMMAND bestow-tests
                --gtest_filter=-FrameTimerTest.DeltaTimeConsistency:CameraSystemTest.ShakeDecaysOverTime:AssetSystemTest.CheckForReloadsDetectsModifiedFile:AssetSystemTest.HotReloadCallbackOnReload:AssetSystemTest.CheckForReloadsIgnoresAssetsBeingLoaded:EventSystemTest.UnsubscribeDuringCallback:AudioSystemTest.InitializeReturnsTrue
        )
    else()
        gtest_discover_tests(bestow-tests
            TEST_INCLUDE_FILES ${CMAKE_CURRENT_BINARY_DIR}/LabelFlakyTests.cmake
        )
    endif()
endif()

# Kangaru DI compatibility test (basic)
add_executable(test-kangaru-basic
    test_kangaru_modules.cpp
)
target_link_libraries(test-kangaru-basic PRIVATE kangaru::kangaru)
target_compile_features(test-kangaru-basic PRIVATE cxx_std_23)
add_test(NAME KangaruBasicTest COMMAND test-kangaru-basic)

# Kangaru DI integration test with C++20 modules
add_executable(test-kangaru-integration
    test_kangaru_integration_main.cpp
)
target_sources(test-kangaru-integration
    PUBLIC
        FILE_SET CXX_MODULES FILES
            test_kangaru_integration.cppm
)
target_use_std_module(test-kangaru-integration)
target_link_libraries(test-kangaru-integration
    PRIVATE
        kangaru::kangaru
        bestow-contract
)
target_compile_features(test-kangaru-integration PRIVATE cxx_std_23)
add_test(NAME KangaruIntegrationTest COMMAND test-kangaru-integration)

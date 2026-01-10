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
    expected_vtables.cpp  # macOS: Provide missing std::expected exception vtables
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

# On macOS with Clang, explicitly link c++abi for exception vtables
if(APPLE AND CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    target_link_libraries(bestow-tests PRIVATE c++abi)
endif()

target_compile_features(bestow-tests PRIVATE cxx_std_23)

# MSVC workaround: force include entt_compat.hpp to fix ADL issues with C++23 modules
if(MSVC)
    target_compile_options(bestow-tests PRIVATE /FI"bestow/entt_compat.hpp")
    # Enable vcpkg DLL deployment
    set_target_properties(bestow-tests PROPERTIES
        VS_DEBUGGER_WORKING_DIRECTORY "$<TARGET_FILE_DIR:bestow-tests>"
    )

    # On Windows in CI, copy vcpkg DLLs to test executable directory
    # This ensures DLLs can be found when running tests
    if(DEFINED ENV{CI})
        # Copy both debug and release DLLs to be safe
        add_custom_command(TARGET bestow-tests POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E echo "Copying vcpkg debug DLLs to test directory..."
            COMMAND ${CMAKE_COMMAND} -E copy_directory
                "${CMAKE_BINARY_DIR}/vcpkg_installed/x64-windows/debug/bin"
                "$<TARGET_FILE_DIR:bestow-tests>"
            COMMAND ${CMAKE_COMMAND} -E echo "Copying vcpkg release DLLs to test directory..."
            COMMAND ${CMAKE_COMMAND} -E copy_directory
                "${CMAKE_BINARY_DIR}/vcpkg_installed/x64-windows/bin"
                "$<TARGET_FILE_DIR:bestow-tests>"
            COMMENT "Copying vcpkg DLLs for Windows CI tests"
            VERBATIM
        )
    endif()
endif()

include(GoogleTest)

# On Windows in CI, skip test discovery (DLL issues) and run tests directly
# On other platforms, use normal test discovery
if(WIN32 AND DEFINED ENV{CI})
    # Add a single test that runs the executable directly
    # DLLs are copied to the executable directory via POST_BUILD command above
    add_test(NAME bestow-tests-all COMMAND bestow-tests)
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
    gtest_discover_tests(bestow-tests
        TEST_INCLUDE_FILES ${CMAKE_CURRENT_BINARY_DIR}/LabelFlakyTests.cmake
    )
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

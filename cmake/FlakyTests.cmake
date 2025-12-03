# cmake/FlakyTests.cmake
# Define flaky tests that should be excluded from CI runs but run locally
#
# These tests are timing-dependent or otherwise unreliable in CI environments
# due to VM scheduling, resource contention, or other non-deterministic factors.
#
# To add a flaky test:
#   1. Add the test name pattern to JFRAME_FLAKY_TESTS below
#   2. Test names use Google Test format: TestSuiteName.TestName
#
# To run tests excluding flaky ones (CI mode):
#   ctest --preset <preset> -LE Flaky
#
# To run only flaky tests (debugging):
#   ctest --preset <preset> -L Flaky
#
# To run all tests including flaky ones (local development):
#   ctest --preset <preset>

set(JFRAME_FLAKY_TESTS
    # Timing-dependent tests that fail on CI due to VM scheduling variance
    "FrameTimerTest.DeltaTimeConsistency"

    # Add more flaky tests here as needed:
    # "TestSuiteName.TestName"
)

# Function to check if a test name matches any flaky test pattern
function(jframe_is_flaky_test test_name result_var)
    set(${result_var} FALSE PARENT_SCOPE)
    foreach(flaky_pattern IN LISTS JFRAME_FLAKY_TESTS)
        if(test_name MATCHES "${flaky_pattern}")
            set(${result_var} TRUE PARENT_SCOPE)
            return()
        endif()
    endforeach()
endfunction()

# Function to label discovered tests as flaky
# Call this after gtest_discover_tests to add "Flaky" label
function(jframe_label_flaky_tests test_target)
    # Get all tests from the target
    get_property(all_tests DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY TESTS)

    foreach(test_name IN LISTS all_tests)
        jframe_is_flaky_test("${test_name}" is_flaky)
        if(is_flaky)
            set_tests_properties("${test_name}" PROPERTIES LABELS "Flaky")
            message(STATUS "Marked test as flaky: ${test_name}")
        endif()
    endforeach()
endfunction()

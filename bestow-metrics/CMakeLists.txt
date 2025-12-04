add_library(bestow-metrics)
target_use_std_module(bestow-metrics)

# Base sources
set(BESTOW_METRICS_SOURCES
    src/MetricsCollector.cpp
)

# Tracy integration - compile Tracy directly into bestow-metrics
# This avoids creating a separate tracy target which causes CMake export issues
if(BESTOW_ENABLE_TRACY AND TRACY_CLIENT_SOURCE)
    list(APPEND BESTOW_METRICS_SOURCES "${TRACY_CLIENT_SOURCE}")
endif()

target_sources(bestow-metrics
    PUBLIC FILE_SET CXX_MODULES FILES
        src/bestow.metrics.cppm
    PRIVATE
        ${BESTOW_METRICS_SOURCES}
)

target_include_directories(bestow-metrics
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

# Tracy include directory (only needed at build time, not install)
if(BESTOW_ENABLE_TRACY AND TRACY_INCLUDE_DIR)
    target_include_directories(bestow-metrics PRIVATE "${TRACY_INCLUDE_DIR}")
endif()

target_link_libraries(bestow-metrics
    PUBLIC
        bestow-contract
        spdlog::spdlog
)

# Tracy dependencies
if(BESTOW_ENABLE_TRACY)
    target_compile_definitions(bestow-metrics PUBLIC TRACY_ENABLE)
    target_link_libraries(bestow-metrics PRIVATE Threads::Threads)
    if(UNIX AND NOT APPLE)
        target_link_libraries(bestow-metrics PRIVATE dl)
    endif()
endif()

target_compile_features(bestow-metrics PUBLIC cxx_std_23)

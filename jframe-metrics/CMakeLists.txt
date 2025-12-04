add_library(jframe-metrics)
target_use_std_module(jframe-metrics)

# Base sources
set(JFRAME_METRICS_SOURCES
    src/MetricsCollector.cpp
)

# Tracy integration - compile Tracy directly into jframe-metrics
# This avoids creating a separate tracy target which causes CMake export issues
if(JFRAME_ENABLE_TRACY AND TRACY_CLIENT_SOURCE)
    list(APPEND JFRAME_METRICS_SOURCES "${TRACY_CLIENT_SOURCE}")
endif()

target_sources(jframe-metrics
    PUBLIC FILE_SET CXX_MODULES FILES
        src/jframe.metrics.cppm
    PRIVATE
        ${JFRAME_METRICS_SOURCES}
)

target_include_directories(jframe-metrics
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

# Tracy include directory (only needed at build time, not install)
if(JFRAME_ENABLE_TRACY AND TRACY_INCLUDE_DIR)
    target_include_directories(jframe-metrics PRIVATE "${TRACY_INCLUDE_DIR}")
endif()

target_link_libraries(jframe-metrics
    PUBLIC
        jframe-contract
        spdlog::spdlog
)

# Tracy dependencies
if(JFRAME_ENABLE_TRACY)
    target_compile_definitions(jframe-metrics PUBLIC TRACY_ENABLE)
    target_link_libraries(jframe-metrics PRIVATE Threads::Threads)
    if(UNIX AND NOT APPLE)
        target_link_libraries(jframe-metrics PRIVATE dl)
    endif()
endif()

target_compile_features(jframe-metrics PUBLIC cxx_std_23)

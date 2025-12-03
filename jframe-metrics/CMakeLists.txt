add_library(jframe-metrics)
target_use_std_module(jframe-metrics)

target_sources(jframe-metrics
    PUBLIC FILE_SET CXX_MODULES FILES
        src/jframe.metrics.cppm
    PRIVATE
        src/MetricsCollector.cpp
)

target_include_directories(jframe-metrics
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

target_link_libraries(jframe-metrics
    PUBLIC
        jframe-contract
        spdlog::spdlog
)

# Tracy integration
if(JFRAME_ENABLE_TRACY)
    target_link_libraries(jframe-metrics PUBLIC tracy)
    target_compile_definitions(jframe-metrics PUBLIC TRACY_ENABLE)
endif()

target_compile_features(jframe-metrics PUBLIC cxx_std_23)

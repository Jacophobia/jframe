# bestow-runtime - Simple user-facing game API
# Links all implementation libraries and provides bestow::run<T>()

add_library(bestow-runtime)

target_sources(bestow-runtime
    PUBLIC FILE_SET CXX_MODULES FILES
        src/bestow.runtime.cppm
)

# Link all implementation libraries
target_link_libraries(bestow-runtime
    PUBLIC
        bestow-contract
        bestow-vulkan
        bestow-audio
        bestow-entity
        bestow-input
        bestow-events
        bestow-assets
        bestow-config
)

target_compile_features(bestow-runtime PUBLIC cxx_std_23)

# Enable import std; support
target_use_std_module(bestow-runtime)

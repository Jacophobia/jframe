# bestow-engine/CMakeLists.txt
# Minimal Public API with Custom Lightweight DI

add_library(bestow-engine STATIC)

target_sources(bestow-engine
    PUBLIC
        FILE_SET CXX_MODULES FILES
            src/bestow.engine.cppm
    PRIVATE
        src/bestow.engine.impl.cpp
)

target_link_libraries(bestow-engine
    PUBLIC
        bestow-contract
    PRIVATE
        bestow-entity
        bestow-graphics3d
        bestow-shader
        bestow-assets
        bestow-audio
        bestow-input
        bestow-physics3d
        bestow-events
        bestow-gamestate
        bestow-level
        bestow-config
        bestow-ui
        glm::glm
        spdlog::spdlog
)

target_compile_features(bestow-engine PUBLIC cxx_std_23)

# Enable import std; support
target_use_std_module(bestow-engine)

# Hot reload support in debug builds
if(CMAKE_BUILD_TYPE MATCHES "Debug" OR CMAKE_BUILD_TYPE MATCHES "RelWithDebInfo")
    target_compile_definitions(bestow-engine PRIVATE BESTOW_HOT_RELOAD=1)
endif()

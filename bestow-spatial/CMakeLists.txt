add_library(bestow-spatial)
target_use_std_module(bestow-spatial)

target_sources(bestow-spatial
    PUBLIC FILE_SET CXX_MODULES FILES
        src/bestow.spatial.impl.cppm
    PRIVATE
        src/FloatingOriginSystem.cpp
        src/GravitySystem.cpp
        src/PlanetaryLODSystem.cpp
)

target_link_libraries(bestow-spatial
    PUBLIC
        bestow-contract
    PRIVATE
        kangaru::kangaru
        spdlog::spdlog
        glm::glm
)

target_compile_features(bestow-spatial PUBLIC cxx_std_23)

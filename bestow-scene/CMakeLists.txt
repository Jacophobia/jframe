add_library(bestow-scene)
target_use_std_module(bestow-scene)

target_sources(bestow-scene
    PUBLIC FILE_SET CXX_MODULES FILES
        src/bestow.scene.impl.cppm
    PRIVATE
        src/SceneSystem.cpp
)

target_link_libraries(bestow-scene
    PUBLIC
        bestow-contract
        unofficial::luajit::luajit
        sol2::sol2
        spdlog::spdlog
)

target_compile_features(bestow-scene PUBLIC cxx_std_23)

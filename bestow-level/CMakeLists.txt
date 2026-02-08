add_library(bestow-level)
target_use_std_module(bestow-level)

target_sources(bestow-level
    PUBLIC FILE_SET CXX_MODULES FILES
        src/bestow.level.impl.cppm
    PRIVATE
        src/LevelSystem.cpp
        src/LevelLoader.cpp
)

target_link_libraries(bestow-level
    PUBLIC
        bestow-contract
        unofficial::luajit::luajit
        sol2::sol2
)

target_compile_features(bestow-level PUBLIC cxx_std_23)

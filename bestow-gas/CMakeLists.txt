add_library(bestow-gas)
target_use_std_module(bestow-gas)

target_sources(bestow-gas
    PUBLIC FILE_SET CXX_MODULES FILES
        src/bestow.gas.impl.cppm
    PRIVATE
        src/GASSystem.cpp
        src/TagsAndAttributes.cpp
        src/GameplayEffects.cpp
        src/GameplayAbilities.cpp
        src/LuaLoader.cpp
)

target_link_libraries(bestow-gas
    PUBLIC
        bestow-contract
        unofficial::luajit::luajit
        sol2::sol2
)

target_compile_features(bestow-gas PUBLIC cxx_std_23)

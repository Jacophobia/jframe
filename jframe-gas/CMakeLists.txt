add_library(jframe-gas)
target_use_std_module(jframe-gas)

target_sources(jframe-gas
    PUBLIC FILE_SET CXX_MODULES FILES
        src/jframe.gas.impl.cppm
    PRIVATE
        src/GASSystem.cpp
        src/TagsAndAttributes.cpp
        src/GameplayEffects.cpp
        src/GameplayAbilities.cpp
        src/LuaLoader.cpp
)

target_link_libraries(jframe-gas
    PUBLIC
        jframe-contract
        lua
        sol2::sol2
)

target_compile_features(jframe-gas PUBLIC cxx_std_23)

add_library(bestow-components STATIC)
target_use_std_module(bestow-components)

target_sources(bestow-components
    PUBLIC
        FILE_SET CXX_MODULES FILES
        src/bestow.components.cppm
        src/bestow.builders.cppm
        src/bestow.luaconfig.cppm
)

target_link_libraries(bestow-components
    PUBLIC
        bestow-contract
    PRIVATE
        lua
        sol2::sol2
)

target_compile_features(bestow-components PUBLIC cxx_std_23)

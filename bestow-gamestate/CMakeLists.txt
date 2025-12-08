# bestow-gamestate - Game State Management System

add_library(bestow-gamestate)
target_use_std_module(bestow-gamestate)

target_sources(bestow-gamestate
    PUBLIC FILE_SET CXX_MODULES FILES
        src/bestow.gamestate.impl.cppm
)

target_link_libraries(bestow-gamestate
    PUBLIC
        bestow-contract
        kangaru::kangaru
)

target_compile_features(bestow-gamestate PUBLIC cxx_std_23)

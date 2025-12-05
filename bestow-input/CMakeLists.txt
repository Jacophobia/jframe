add_library(bestow-input)
target_use_std_module(bestow-input)

target_sources(bestow-input
    PUBLIC FILE_SET CXX_MODULES FILES
        src/bestow.input.impl.cppm
    PRIVATE
        src/InputSystem.cpp
)

target_link_libraries(bestow-input
    PUBLIC
        bestow-contract
        glfw
        SDL2::SDL2
)

target_compile_features(bestow-input PUBLIC cxx_std_23)

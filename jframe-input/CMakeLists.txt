add_library(jframe-input)
target_use_std_module(jframe-input)

target_sources(jframe-input
    PUBLIC FILE_SET CXX_MODULES FILES
        src/jframe.input.impl.cppm
    PRIVATE
        src/InputSystem.cpp
)

target_link_libraries(jframe-input
    PUBLIC
        jframe-contract
        glfw
        SDL2::SDL2
)

target_compile_features(jframe-input PUBLIC cxx_std_23)

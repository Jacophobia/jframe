add_library(jframe-components STATIC)
target_use_std_module(jframe-components)

target_sources(jframe-components
    PUBLIC
        FILE_SET CXX_MODULES FILES
        src/jframe.components.cppm
)

target_link_libraries(jframe-components
    PUBLIC
        jframe-contract
)

target_compile_features(jframe-components PUBLIC cxx_std_23)

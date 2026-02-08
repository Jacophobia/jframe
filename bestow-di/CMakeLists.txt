add_library(bestow-di)
target_use_std_module(bestow-di)

target_sources(bestow-di
    PUBLIC FILE_SET CXX_MODULES FILES
        src/bestow.di.cppm
)

target_compile_features(bestow-di PUBLIC cxx_std_23)

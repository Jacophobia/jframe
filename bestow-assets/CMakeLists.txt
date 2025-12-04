add_library(bestow-assets)
target_use_std_module(bestow-assets)

target_sources(bestow-assets
    PUBLIC FILE_SET CXX_MODULES FILES
        src/bestow.assets.impl.cppm
    PRIVATE
        src/AssetSystem.cpp
        src/loaders/TextureLoader.cpp
        src/loaders/SoundLoader.cpp
        src/loaders/FontLoader.cpp
)

target_link_libraries(bestow-assets
    PUBLIC
        bestow-contract
        Taskflow::Taskflow
        $<TARGET_NAME_IF_EXISTS:zstd::libzstd_shared>
)

if(TARGET stb)
    target_link_libraries(bestow-assets PRIVATE stb)
endif()

target_compile_features(bestow-assets PUBLIC cxx_std_23)

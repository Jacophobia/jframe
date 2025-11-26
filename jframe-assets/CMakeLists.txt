add_library(jframe-assets)
target_use_std_module(jframe-assets)

target_sources(jframe-assets
    PUBLIC FILE_SET CXX_MODULES FILES
        src/jframe.assets.impl.cppm
    PRIVATE
        src/AssetSystem.cpp
        src/loaders/TextureLoader.cpp
        src/loaders/SoundLoader.cpp
        src/loaders/FontLoader.cpp
)

target_link_libraries(jframe-assets
    PUBLIC
        jframe-contract
        Taskflow::Taskflow
        $<TARGET_NAME_IF_EXISTS:zstd::libzstd_shared>
)

if(TARGET stb)
    target_link_libraries(jframe-assets PRIVATE stb)
endif()

target_compile_features(jframe-assets PUBLIC cxx_std_23)

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
        src/loaders/ModelLoader.cpp
)

target_link_libraries(bestow-assets
    PUBLIC
        bestow-contract
        Taskflow::Taskflow
        kangaru::kangaru
        $<TARGET_NAME_IF_EXISTS:zstd::libzstd_shared>
    PRIVATE
        efsw::efsw
        spdlog::spdlog
        unofficial::shaderc::shaderc
        unofficial::luajit::luajit
        sol2::sol2
)

if(TARGET stb)
    target_link_libraries(bestow-assets PRIVATE stb)
endif()

# assimp for FBX, glTF, OBJ model loading
find_package(assimp CONFIG REQUIRED)
target_link_libraries(bestow-assets PRIVATE assimp::assimp)

target_compile_features(bestow-assets PUBLIC cxx_std_23)

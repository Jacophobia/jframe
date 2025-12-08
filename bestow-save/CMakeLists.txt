add_library(bestow-save)
target_use_std_module(bestow-save)

target_sources(bestow-save
    PUBLIC FILE_SET CXX_MODULES FILES
        src/bestow.save.impl.cppm
    PRIVATE
        src/SaveSystem.cpp
        src/SaveSerializer.cpp
        src/SaveMigrator.cpp
        src/ProfileManager.cpp
)

target_link_libraries(bestow-save
    PUBLIC
        bestow-contract
        cereal::cereal
        nlohmann_json::nlohmann_json
        $<TARGET_NAME_IF_EXISTS:zstd::libzstd_shared>
        kangaru::kangaru
)

target_compile_features(bestow-save PUBLIC cxx_std_23)

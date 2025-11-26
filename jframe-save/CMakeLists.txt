add_library(jframe-save)
target_use_std_module(jframe-save)

target_sources(jframe-save
    PUBLIC FILE_SET CXX_MODULES FILES
        src/jframe.save.impl.cppm
    PRIVATE
        src/SaveSystem.cpp
        src/SaveSerializer.cpp
        src/SaveMigrator.cpp
        src/ProfileManager.cpp
)

target_link_libraries(jframe-save
    PUBLIC
        jframe-contract
        cereal::cereal
        nlohmann_json::nlohmann_json
        $<TARGET_NAME_IF_EXISTS:zstd::libzstd_shared>
)

target_compile_features(jframe-save PUBLIC cxx_std_23)

add_library(bestow-state)
target_use_std_module(bestow-state)

target_sources(bestow-state
    PUBLIC FILE_SET CXX_MODULES FILES
        src/bestow.state.impl.cppm
    PRIVATE
        src/StateSystem.cpp
        src/StateDatabase.cpp
        src/StateWorker.cpp
        src/JsonExporter.cpp
)

target_link_libraries(bestow-state
    PUBLIC
        bestow-contract
        nlohmann_json::nlohmann_json
    PRIVATE
        unofficial::sqlite3::sqlite3
        spdlog::spdlog
)

target_compile_features(bestow-state PUBLIC cxx_std_23)

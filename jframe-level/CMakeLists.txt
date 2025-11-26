add_library(jframe-level)
target_use_std_module(jframe-level)

target_sources(jframe-level
    PUBLIC FILE_SET CXX_MODULES FILES
        src/jframe.level.impl.cppm
    PRIVATE
        src/LevelSystem.cpp
        src/LevelLoader.cpp
)

target_link_libraries(jframe-level
    PUBLIC
        jframe-contract
        ${LUA_LIBRARIES}
        sol2::sol2
)

target_include_directories(jframe-level PRIVATE ${LUA_INCLUDE_DIR})

target_compile_features(jframe-level PUBLIC cxx_std_23)

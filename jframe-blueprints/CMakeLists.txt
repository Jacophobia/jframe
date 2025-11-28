# jframe-blueprints/CMakeLists.txt
# Blueprint Factory for data-driven entity creation

add_library(jframe-blueprints STATIC)

target_sources(jframe-blueprints
    PUBLIC
        FILE_SET CXX_MODULES FILES
            src/jframe.blueprints.impl.cppm
    PRIVATE
        src/BlueprintFactory.cpp
)

target_use_std_module(jframe-blueprints)

target_link_libraries(jframe-blueprints
    PUBLIC
        jframe-contract
        jframe-entity
        jframe-physics
    PRIVATE
        sol2::sol2
        ${LUA_LIBRARIES}
)

target_include_directories(jframe-blueprints
    PRIVATE
        ${LUA_INCLUDE_DIR}
)

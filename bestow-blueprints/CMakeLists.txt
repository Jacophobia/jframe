# bestow-blueprints/CMakeLists.txt
# Blueprint Factory for data-driven entity creation

add_library(bestow-blueprints STATIC)

target_sources(bestow-blueprints
    PUBLIC
        FILE_SET CXX_MODULES FILES
            src/bestow.blueprints.impl.cppm
    PRIVATE
        src/BlueprintFactory.cpp
)

target_use_std_module(bestow-blueprints)

target_link_libraries(bestow-blueprints
    PUBLIC
        bestow-contract
        bestow-entity
        bestow-physics
    PRIVATE
        sol2::sol2
        ${LUA_LIBRARIES}
)

target_include_directories(bestow-blueprints
    PRIVATE
        ${LUA_INCLUDE_DIR}
)

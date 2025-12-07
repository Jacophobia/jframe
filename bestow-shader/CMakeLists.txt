# bestow-shader - Dynamic Shader System with Hot Reload and Lua Materials

add_library(bestow-shader)
target_use_std_module(bestow-shader)

target_sources(bestow-shader
    PUBLIC FILE_SET CXX_MODULES FILES
        src/bestow.shader.impl.cppm
)

target_link_libraries(bestow-shader
    PUBLIC
        bestow-contract
        glad::glad
        glfw
        glm::glm
)

# sol2 for Lua material parsing
if(TARGET sol2)
    target_link_libraries(bestow-shader PRIVATE sol2)
    target_compile_definitions(bestow-shader PRIVATE BESTOW_HAS_SOL2)
    message(STATUS "sol2 found - Lua material loading enabled")
elseif(TARGET sol2::sol2)
    target_link_libraries(bestow-shader PRIVATE sol2::sol2)
    target_compile_definitions(bestow-shader PRIVATE BESTOW_HAS_SOL2)
    message(STATUS "sol2::sol2 found - Lua material loading enabled")
endif()

# Lua for sol2
if(TARGET lua)
    target_link_libraries(bestow-shader PRIVATE lua)
elseif(TARGET lua::lua)
    target_link_libraries(bestow-shader PRIVATE lua::lua)
endif()

# efsw for hot reload file watching
if(TARGET efsw::efsw)
    target_link_libraries(bestow-shader PRIVATE efsw::efsw)
    target_compile_definitions(bestow-shader PRIVATE BESTOW_HAS_EFSW)
    message(STATUS "efsw found - Shader hot reload enabled")
endif()

target_compile_features(bestow-shader PUBLIC cxx_std_23)

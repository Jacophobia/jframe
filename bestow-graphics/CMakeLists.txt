add_library(bestow-graphics)
target_use_std_module(bestow-graphics)

target_sources(bestow-graphics
    PUBLIC FILE_SET CXX_MODULES FILES
        src/bestow.graphics.impl.cppm
    PRIVATE
        src/GraphicsSystem.cpp
        src/Renderer.cpp
        src/SpriteRenderer.cpp
        src/TextRenderer.cpp
        src/DebugRenderer.cpp
)

target_link_libraries(bestow-graphics
    PUBLIC
        bestow-contract
        glfw
        glad::glad
        glm::glm
        Freetype::Freetype
        imgui::imgui
)

if(TARGET stb)
    target_link_libraries(bestow-graphics PRIVATE stb)
endif()

target_compile_features(bestow-graphics PUBLIC cxx_std_23)

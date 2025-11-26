add_library(jframe-graphics)
target_use_std_module(jframe-graphics)

target_sources(jframe-graphics
    PUBLIC FILE_SET CXX_MODULES FILES
        src/jframe.graphics.impl.cppm
    PRIVATE
        src/GraphicsSystem.cpp
        src/Renderer.cpp
        src/SpriteRenderer.cpp
        src/TextRenderer.cpp
        src/DebugRenderer.cpp
)

target_link_libraries(jframe-graphics
    PUBLIC
        jframe-contract
        glfw
        glad::glad
        glm::glm
        Freetype::Freetype
        imgui::imgui
)

if(TARGET stb)
    target_link_libraries(jframe-graphics PRIVATE stb)
endif()

target_compile_features(jframe-graphics PUBLIC cxx_std_23)

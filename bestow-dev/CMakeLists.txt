add_library(bestow-dev)
target_use_std_module(bestow-dev)

target_sources(bestow-dev
    PUBLIC FILE_SET CXX_MODULES FILES
        src/bestow.dev.cppm
    PRIVATE
        src/ImGuiBackend.cpp
        src/ComponentRegistry.cpp
        src/HotReloadManager.cpp
        src/DevOverlay.cpp
        src/EntityInspector.cpp
)

target_link_libraries(bestow-dev
    PUBLIC
        bestow-contract
        bestow-core
        bestow-metrics
        imgui::imgui
        efsw::efsw
        glfw
        glad::glad
        lua
        sol2::sol2
)

target_compile_definitions(bestow-dev PUBLIC BESTOW_DEV_TOOLS)

target_compile_features(bestow-dev PUBLIC cxx_std_23)

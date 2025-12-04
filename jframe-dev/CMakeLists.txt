add_library(jframe-dev)
target_use_std_module(jframe-dev)

target_sources(jframe-dev
    PUBLIC FILE_SET CXX_MODULES FILES
        src/jframe.dev.cppm
    PRIVATE
        src/ImGuiBackend.cpp
        src/ComponentRegistry.cpp
        src/HotReloadManager.cpp
        src/DevOverlay.cpp
        src/EntityInspector.cpp
)

target_link_libraries(jframe-dev
    PUBLIC
        jframe-contract
        jframe-core
        jframe-metrics
        imgui::imgui
        efsw::efsw
        glfw
        glad::glad
        ${LUA_LIBRARIES}
        sol2::sol2
)

target_include_directories(jframe-dev PRIVATE ${LUA_INCLUDE_DIR})

target_compile_definitions(jframe-dev PUBLIC JFRAME_DEV_TOOLS)

target_compile_features(jframe-dev PUBLIC cxx_std_23)

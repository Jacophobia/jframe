# bestow-opengl - OpenGL rendering backend (2D and 3D)

add_library(bestow-opengl)
target_use_std_module(bestow-opengl)

target_sources(bestow-opengl
    PUBLIC FILE_SET CXX_MODULES FILES
        src/bestow.opengl.impl.cppm
    PRIVATE
        # 2D Graphics implementation files
        src/GraphicsSystem.cpp
        src/Renderer.cpp
        src/SpriteRenderer.cpp
        src/TextRenderer.cpp
        src/DebugRenderer.cpp
)

target_link_libraries(bestow-opengl
    PUBLIC
        bestow-contract
        kangaru::kangaru
        glfw
        glad::glad
        glm::glm
        Freetype::Freetype
        imgui::imgui
    PRIVATE
        spdlog::spdlog
)

if(TARGET stb)
    target_link_libraries(bestow-opengl PRIVATE stb)
endif()

# Optional mesh loading libraries (supports both vcpkg and FetchContent)
if(TARGET tinyobjloader::tinyobjloader)
    target_link_libraries(bestow-opengl PRIVATE tinyobjloader::tinyobjloader)
    target_compile_definitions(bestow-opengl PRIVATE BESTOW_HAS_TINYOBJLOADER)
    message(STATUS "tinyobjloader (vcpkg) - OBJ mesh loading enabled")
elseif(TARGET tinyobjloader)
    target_link_libraries(bestow-opengl PRIVATE tinyobjloader)
    target_compile_definitions(bestow-opengl PRIVATE BESTOW_HAS_TINYOBJLOADER)
    message(STATUS "tinyobjloader (FetchContent) - OBJ mesh loading enabled")
endif()

# tinygltf (supports both vcpkg and FetchContent)
# tinygltf is header-only, so we just need include directories
if(TARGET tinygltf)
    get_target_property(TINYGLTF_IMPORTED tinygltf IMPORTED)
    if(TINYGLTF_IMPORTED)
        # vcpkg imported target - safe to link
        target_link_libraries(bestow-opengl PRIVATE tinygltf)
    else()
        # FetchContent target - just use include directories to avoid export issues
        get_target_property(TINYGLTF_INCLUDE_DIRS tinygltf INTERFACE_INCLUDE_DIRECTORIES)
        if(TINYGLTF_INCLUDE_DIRS)
            target_include_directories(bestow-opengl PRIVATE ${TINYGLTF_INCLUDE_DIRS})
        endif()
    endif()
    target_compile_definitions(bestow-opengl PRIVATE BESTOW_HAS_TINYGLTF)
    message(STATUS "tinygltf - glTF mesh loading enabled")
else()
    # Check if header exists (vcpkg installs as header-only)
    find_file(TINYGLTF_HEADER tiny_gltf.h
        PATHS ${CMAKE_PREFIX_PATH}
        PATH_SUFFIXES include
        NO_DEFAULT_PATH
    )
    if(TINYGLTF_HEADER)
        target_compile_definitions(bestow-opengl PRIVATE BESTOW_HAS_TINYGLTF)
        message(STATUS "tinygltf (header-only) - glTF mesh loading enabled")
    endif()
endif()

target_compile_features(bestow-opengl PUBLIC cxx_std_23)

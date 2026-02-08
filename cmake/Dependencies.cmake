# Dependencies.cmake
# Finds and configures all third-party dependencies

include(FetchContent)

function(find_dependencies)
    # vcpkg dependencies
    find_package(glfw3 CONFIG REQUIRED)
    find_package(glad CONFIG REQUIRED)
    find_package(SDL2 CONFIG REQUIRED)
    find_package(glm CONFIG REQUIRED)
    find_package(spdlog CONFIG REQUIRED)
    find_package(fmt CONFIG REQUIRED)

    # LuaJIT - vcpkg provides pkgconfig on Unix, direct lib on Windows
    if(NOT TARGET unofficial::luajit::luajit)
        if(WIN32)
            # On Windows, vcpkg installs LuaJIT directly without cmake config
            # Headers are in luajit-2.1/ subdirectory, library is lua51.lib
            add_library(unofficial::luajit::luajit INTERFACE IMPORTED)
            target_include_directories(unofficial::luajit::luajit INTERFACE
                "${_VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/include/luajit"
            )
            target_link_libraries(unofficial::luajit::luajit INTERFACE
                "${_VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/lib/lua51.lib"
            )
            set(LUAJIT_INCLUDE_DIRS "${_VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/include/luajit")
        else()
            # On Unix, use pkg-config
            find_package(PkgConfig REQUIRED)
            pkg_check_modules(LUAJIT REQUIRED IMPORTED_TARGET luajit)
            add_library(unofficial::luajit::luajit INTERFACE IMPORTED)
            target_link_libraries(unofficial::luajit::luajit INTERFACE PkgConfig::LUAJIT)
            # Add luajit include path so sol2 can find lua.h
            target_include_directories(unofficial::luajit::luajit INTERFACE
                "${LUAJIT_INCLUDE_DIRS}"
            )
        endif()
    endif()
    # Also add to global include path for sol2 compatibility
    include_directories(SYSTEM "${LUAJIT_INCLUDE_DIRS}")

    find_package(sol2 CONFIG REQUIRED)
    find_package(nlohmann_json CONFIG REQUIRED)
    find_package(cereal CONFIG REQUIRED)
    find_package(EnTT CONFIG REQUIRED)
    find_package(box2d CONFIG REQUIRED)
    find_package(Freetype REQUIRED)
    find_package(zstd CONFIG REQUIRED)
    find_package(GTest CONFIG REQUIRED)
    find_package(imgui CONFIG REQUIRED)
    find_package(Taskflow CONFIG REQUIRED)
    find_package(RecastNavigation CONFIG REQUIRED)
    find_package(BehaviorTree CONFIG)
    find_package(unofficial-shaderc CONFIG REQUIRED)

    # File watcher (needed for hot reload support in assets/scripts)
    find_package(efsw CONFIG REQUIRED)

    # 3D Graphics dependencies (with FetchContent fallback)
    find_package(tinyobjloader CONFIG QUIET)
    if(NOT tinyobjloader_FOUND)
        message(STATUS "tinyobjloader not found via vcpkg, fetching from GitHub...")
        FetchContent_Declare(
            tinyobjloader
            GIT_REPOSITORY https://github.com/tinyobjloader/tinyobjloader.git
            GIT_TAG v2.0.0rc13
            GIT_SHALLOW TRUE
        )
        FetchContent_MakeAvailable(tinyobjloader)
    endif()

    find_package(tinygltf CONFIG QUIET)
    if(NOT tinygltf_FOUND)
        message(STATUS "tinygltf not found via vcpkg, fetching from GitHub...")
        FetchContent_Declare(
            tinygltf
            GIT_REPOSITORY https://github.com/syoyo/tinygltf.git
            GIT_TAG v2.9.3
            GIT_SHALLOW TRUE
        )
        set(TINYGLTF_HEADER_ONLY ON CACHE BOOL "" FORCE)
        set(TINYGLTF_INSTALL OFF CACHE BOOL "" FORCE)
        FetchContent_MakeAvailable(tinygltf)
    endif()

    # 3D Physics - Jolt Physics (with FetchContent fallback)
    find_package(unofficial-joltphysics CONFIG QUIET)
    if(NOT unofficial-joltphysics_FOUND)
        find_package(Jolt CONFIG QUIET)
    endif()
    if(NOT unofficial-joltphysics_FOUND AND NOT Jolt_FOUND)
        message(STATUS "Jolt Physics not found via vcpkg, fetching from GitHub...")
        FetchContent_Declare(
            JoltPhysics
            GIT_REPOSITORY https://github.com/jrouwe/JoltPhysics.git
            GIT_TAG v5.2.0
            GIT_SHALLOW TRUE
            SOURCE_SUBDIR Build
        )
        set(TARGET_UNIT_TESTS OFF CACHE BOOL "" FORCE)
        set(TARGET_HELLO_WORLD OFF CACHE BOOL "" FORCE)
        set(TARGET_PERFORMANCE_TEST OFF CACHE BOOL "" FORCE)
        set(TARGET_SAMPLES OFF CACHE BOOL "" FORCE)
        set(TARGET_VIEWER OFF CACHE BOOL "" FORCE)
        FetchContent_MakeAvailable(JoltPhysics)

        # Create alias to match vcpkg naming
        if(TARGET Jolt)
            add_library(unofficial::joltphysics::Jolt ALIAS Jolt)
        endif()
    endif()

    # Tracy profiler (optional) - automatically fetched via FetchContent
    # We don't create a separate tracy target to avoid CMake export issues.
    # Instead, we set variables that bestow-metrics uses to compile Tracy directly.
    if(BESTOW_ENABLE_TRACY)
        # Check if Tracy is already available locally
        if(EXISTS "${CMAKE_SOURCE_DIR}/external/tracy/public/TracyClient.cpp")
            set(TRACY_SOURCE_DIR "${CMAKE_SOURCE_DIR}/external/tracy" CACHE PATH "Tracy source directory")
            message(STATUS "Tracy profiler: using local copy at ${TRACY_SOURCE_DIR}")
        else()
            # Fetch Tracy from GitHub
            FetchContent_Declare(
                tracy
                GIT_REPOSITORY https://github.com/wolfpld/tracy.git
                GIT_TAG v0.11.1  # Pin to specific version for reproducibility
                GIT_SHALLOW TRUE
                GIT_PROGRESS TRUE
            )
            FetchContent_MakeAvailable(tracy)
            set(TRACY_SOURCE_DIR "${tracy_SOURCE_DIR}" CACHE PATH "Tracy source directory")
            message(STATUS "Tracy profiler: fetched from GitHub to ${TRACY_SOURCE_DIR}")
        endif()

        # Set variables for bestow-metrics to use (no separate target to avoid export issues)
        set(TRACY_CLIENT_SOURCE "${TRACY_SOURCE_DIR}/public/TracyClient.cpp" CACHE FILEPATH "Tracy client source")
        set(TRACY_INCLUDE_DIR "${TRACY_SOURCE_DIR}/public" CACHE PATH "Tracy include directory")

        # Tracy requires threading support
        find_package(Threads REQUIRED)

        message(STATUS "Tracy profiler enabled (v0.11.1)")
    endif()

    # FMOD (manual setup required)
    set(FMOD_DIR "${CMAKE_SOURCE_DIR}/external/fmod")
    if(EXISTS "${FMOD_DIR}/include/fmod.h")
        add_library(fmod SHARED IMPORTED GLOBAL)
        target_include_directories(fmod INTERFACE "${FMOD_DIR}/include")

        if(APPLE)
            set_target_properties(fmod PROPERTIES
                IMPORTED_LOCATION "${FMOD_DIR}/lib/macos/libfmod.dylib"
            )
        elseif(WIN32)
            set_target_properties(fmod PROPERTIES
                IMPORTED_LOCATION "${FMOD_DIR}/lib/windows/fmod.dll"
                IMPORTED_IMPLIB "${FMOD_DIR}/lib/windows/fmod_vc.lib"
            )
        else()
            set_target_properties(fmod PROPERTIES
                IMPORTED_LOCATION "${FMOD_DIR}/lib/linux/libfmod.so"
            )
        endif()

        message(STATUS "FMOD found at ${FMOD_DIR}")
    else()
        message(WARNING "FMOD not found. Audio system will not be available.")
    endif()

    # stb headers
    set(STB_DIR "${CMAKE_SOURCE_DIR}/external/stb")
    if(EXISTS "${STB_DIR}/stb_image.h")
        add_library(stb INTERFACE)
        target_include_directories(stb INTERFACE "${STB_DIR}")
        message(STATUS "stb headers found at ${STB_DIR}")
    else()
        message(WARNING "stb headers not found. Please add stb_image.h and stb_truetype.h to external/stb/")
    endif()
endfunction()

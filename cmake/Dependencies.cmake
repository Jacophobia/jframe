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
    find_package(unofficial-lua CONFIG REQUIRED)
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

    # Development tools (optional in release)
    if(JFRAME_DEV_TOOLS)
        find_package(efsw CONFIG REQUIRED)
    endif()

    # Tracy profiler (optional) - automatically fetched via FetchContent
    # We don't create a separate tracy target to avoid CMake export issues.
    # Instead, we set variables that jframe-metrics uses to compile Tracy directly.
    if(JFRAME_ENABLE_TRACY)
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

        # Set variables for jframe-metrics to use (no separate target to avoid export issues)
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

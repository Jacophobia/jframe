# Dependencies.cmake
# Finds and configures all third-party dependencies

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

    # Tracy profiler (optional)
    if(JFRAME_ENABLE_TRACY)
        # Tracy is typically included as a submodule
        if(EXISTS "${CMAKE_SOURCE_DIR}/external/tracy/public/TracyClient.cpp")
            add_library(tracy STATIC
                "${CMAKE_SOURCE_DIR}/external/tracy/public/TracyClient.cpp"
            )
            target_include_directories(tracy PUBLIC
                "${CMAKE_SOURCE_DIR}/external/tracy/public"
            )
            target_compile_definitions(tracy PUBLIC TRACY_ENABLE)
            message(STATUS "Tracy profiler enabled")
        else()
            message(WARNING "Tracy enabled but not found in external/tracy")
            set(JFRAME_ENABLE_TRACY OFF PARENT_SCOPE)
        endif()
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

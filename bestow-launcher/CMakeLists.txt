# bestow-launcher/CMakeLists.txt
# Single executable entry point - `bestow run <game.lua>`

# Include Windows runtime helper for DLL copying
include(WindowsRuntime)

add_executable(bestow)

target_sources(bestow
    PRIVATE
        src/main.cpp
        src/CommandLine.cpp
        src/GameRunner.cpp
)

find_package(nlohmann_json CONFIG REQUIRED)

target_link_libraries(bestow
    PRIVATE
        bestow-core
        bestow-contract
        bestow-entity
        bestow-input
        bestow-audio
        bestow-assets
        bestow-events
        bestow-physics
        bestow-animation
        bestow-config
        bestow-scene
        bestow-ui
        bestow-script
        bestow-luabind
        unofficial::luajit::luajit
        sol2::sol2
        spdlog::spdlog
        nlohmann_json::nlohmann_json
)

# Link Vulkan backend if available
if(TARGET bestow-vulkan)
    target_link_libraries(bestow PRIVATE bestow-vulkan)
    target_compile_definitions(bestow PRIVATE BESTOW_HAS_VULKAN=1)
endif()

# Link 3D physics if available
if(TARGET bestow-physics3d)
    target_link_libraries(bestow PRIVATE bestow-physics3d)
    target_compile_definitions(bestow PRIVATE BESTOW_HAS_PHYSICS3D=1)
endif()

target_compile_features(bestow PUBLIC cxx_std_23)

# Enable module support
set_target_properties(bestow PROPERTIES
    CXX_SCAN_FOR_MODULES ON
)

# Enable import std; support
target_use_std_module(bestow)

# Windows: Copy all runtime DLLs to output directory
if(WIN32)
    copy_runtime_dlls(bestow)
endif()

# macOS app bundle configuration
# Uses Info.plist with LSArchitecturePriority to force Rosetta 2 for LuaJIT
if(APPLE)
    set_target_properties(bestow PROPERTIES
        MACOSX_BUNDLE TRUE
        MACOSX_BUNDLE_BUNDLE_NAME "Bestow"
        MACOSX_BUNDLE_BUNDLE_VERSION "${PROJECT_VERSION}"
        MACOSX_BUNDLE_SHORT_VERSION_STRING "${PROJECT_VERSION}"
        MACOSX_BUNDLE_GUI_IDENTIFIER "com.bestow.launcher"
        MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_SOURCE_DIR}/Info.plist.in"
        # Set rpath to look for libraries in Frameworks directory within app bundle
        INSTALL_RPATH "@executable_path/../Frameworks"
        BUILD_RPATH "${CMAKE_SOURCE_DIR}/external/fmod/lib/macos;/usr/local/lib"
    )
endif()

# Installation rules
include(GNUInstallDirs)

# Install the bestow executable
install(TARGETS bestow
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    BUNDLE DESTINATION ${CMAKE_INSTALL_BINDIR}
)

# Copy dynamic libraries into app bundle Frameworks directory
if(APPLE)
    # Collect all dylibs that need to be bundled
    set(DYLIBS_TO_BUNDLE "")

    # FMOD
    if(TARGET fmod)
        get_target_property(FMOD_LOCATION fmod IMPORTED_LOCATION)
        list(APPEND DYLIBS_TO_BUNDLE "${FMOD_LOCATION}")
    endif()

    # Vulkan - need to find the actual dylib and create versioned symlink
    find_package(Vulkan QUIET)
    if(Vulkan_FOUND AND Vulkan_LIBRARY)
        # Get the directory containing the vulkan library
        get_filename_component(VULKAN_LIB_DIR "${Vulkan_LIBRARY}" DIRECTORY)
        # The actual versioned library
        set(VULKAN_DYLIB "${VULKAN_LIB_DIR}/libvulkan.1.dylib")
        if(EXISTS "${VULKAN_DYLIB}")
            list(APPEND DYLIBS_TO_BUNDLE "${VULKAN_DYLIB}")
        endif()
    endif()

    # Install all collected dylibs
    install(CODE "
        set(FRAMEWORKS_DIR \"\$ENV{DESTDIR}\${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_BINDIR}/bestow.app/Contents/Frameworks\")
        file(MAKE_DIRECTORY \"\${FRAMEWORKS_DIR}\")
        set(DYLIBS \"${DYLIBS_TO_BUNDLE}\")
        foreach(DYLIB \${DYLIBS})
            if(EXISTS \"\${DYLIB}\")
                # Resolve symlinks to get real path
                get_filename_component(REAL_DYLIB \"\${DYLIB}\" REALPATH)
                get_filename_component(DYLIB_NAME \"\${DYLIB}\" NAME)
                get_filename_component(REAL_NAME \"\${REAL_DYLIB}\" NAME)
                message(STATUS \"Bundling: \${REAL_DYLIB} -> \${FRAMEWORKS_DIR}\")
                file(COPY \"\${REAL_DYLIB}\" DESTINATION \"\${FRAMEWORKS_DIR}\")
                # Create symlink if original was a symlink
                if(NOT \"\${DYLIB_NAME}\" STREQUAL \"\${REAL_NAME}\")
                    message(STATUS \"Creating symlink: \${DYLIB_NAME} -> \${REAL_NAME}\")
                    execute_process(
                        COMMAND \${CMAKE_COMMAND} -E create_symlink \"\${REAL_NAME}\" \"\${FRAMEWORKS_DIR}/\${DYLIB_NAME}\"
                    )
                endif()
            endif()
        endforeach()
    ")
endif()

# Create symlink for easy command-line access on macOS
if(APPLE)
    install(CODE "
        set(BESTOW_BUNDLE \"\$ENV{DESTDIR}\${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_BINDIR}/bestow.app/Contents/MacOS/bestow\")
        set(BESTOW_LINK \"\$ENV{DESTDIR}\${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_BINDIR}/bestow\")
        message(STATUS \"Creating symlink: \${BESTOW_LINK} -> \${BESTOW_BUNDLE}\")
        execute_process(COMMAND \${CMAKE_COMMAND} -E create_symlink \${BESTOW_BUNDLE} \${BESTOW_LINK})
    ")
endif()

# Install template directory for 'bestow init' command
# This places template at: <prefix>/share/bestow/template
# Which is found by: exeDir / ".." / "share" / "bestow" / "template"
# Copy everything (including dot-prefixed dirs like .claude/)
install(DIRECTORY ${CMAKE_SOURCE_DIR}/template/
    DESTINATION ${CMAKE_INSTALL_DATADIR}/bestow/template
)

# On macOS, also install template into app bundle Resources directory
# This ensures 'bestow init' works when running from the bundled app
if(APPLE)
    install(DIRECTORY ${CMAKE_SOURCE_DIR}/template/
        DESTINATION ${CMAKE_INSTALL_BINDIR}/bestow.app/Contents/Resources/template
    )
endif()

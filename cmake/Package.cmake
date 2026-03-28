# Package.cmake
# Creates a distributable package for Bestow

# Define the package target
function(create_package_target)
    # Only create package targets if bestow executable is being built
    if(NOT TARGET bestow)
        message(STATUS "Package targets not created: 'bestow' target not found (requires BESTOW_BUILD_VULKAN=ON)")
        return()
    endif()
    # Determine platform-specific settings
    if(WIN32)
        set(PLATFORM_NAME "windows-x64")
        set(ARCHIVE_EXT "zip")
        set(EXE_SUFFIX ".exe")
    elseif(APPLE)
        set(PLATFORM_NAME "macos-x64")
        set(ARCHIVE_EXT "tar.gz")
        set(EXE_SUFFIX "")
    else()
        set(PLATFORM_NAME "linux-x64")
        set(ARCHIVE_EXT "tar.gz")
        set(EXE_SUFFIX "")
    endif()

    set(DIST_DIR "${CMAKE_BINARY_DIR}/dist/bestow-${PLATFORM_NAME}")
    set(DIST_BIN_DIR "${DIST_DIR}/bin")
    set(DIST_LIB_DIR "${DIST_DIR}/library")
    set(DIST_TPL_DIR "${DIST_DIR}/template")
    set(DIST_EX_DIR "${DIST_DIR}/examples")

    # Create the package target
    add_custom_target(package
        DEPENDS bestow
        COMMENT "Creating distribution package for ${PLATFORM_NAME}..."

        # Clean and create directory structure
        COMMAND ${CMAKE_COMMAND} -E rm -rf "${DIST_DIR}"
        COMMAND ${CMAKE_COMMAND} -E make_directory "${DIST_BIN_DIR}"
        COMMAND ${CMAKE_COMMAND} -E make_directory "${DIST_LIB_DIR}"
        COMMAND ${CMAKE_COMMAND} -E make_directory "${DIST_TPL_DIR}"
        COMMAND ${CMAKE_COMMAND} -E make_directory "${DIST_EX_DIR}/snake-lua"

        # Copy executable
        COMMAND ${CMAKE_COMMAND} -E copy "$<TARGET_FILE:bestow>" "${DIST_BIN_DIR}/"

        # Copy asset library
        COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_SOURCE_DIR}/asset-library" "${DIST_LIB_DIR}"

        # Copy template directory (all files including .claude/, .mcp.json, etc.)
        COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_SOURCE_DIR}/template" "${DIST_TPL_DIR}"

        # Copy snake-lua example
        COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_SOURCE_DIR}/games/snake-lua" "${DIST_EX_DIR}/snake-lua"

        # Copy README
        COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_SOURCE_DIR}/cmake/dist-readme.txt" "${DIST_DIR}/README.txt"
    )

    # Platform-specific: Copy DLLs on Windows
    if(WIN32)
        add_custom_command(TARGET package POST_BUILD
            COMMAND ${CMAKE_COMMAND}
                -DSRC_DIR="$<TARGET_FILE_DIR:bestow>"
                -DDST_DIR="${DIST_BIN_DIR}"
                -P "${CMAKE_SOURCE_DIR}/cmake/CopyDLLsOnly.cmake"
            COMMENT "Copying DLLs to distribution..."
        )
    endif()

    # Platform-specific: Copy shared libraries on Unix
    if(UNIX AND NOT APPLE)
        add_custom_command(TARGET package POST_BUILD
            COMMAND ${CMAKE_COMMAND}
                -DSRC_DIR="$<TARGET_FILE_DIR:bestow>"
                -DDST_DIR="${DIST_BIN_DIR}"
                -P "${CMAKE_SOURCE_DIR}/cmake/CopySharedLibs.cmake"
            COMMENT "Copying shared libraries to distribution..."
        )
    endif()

    # Platform-specific: Copy dylibs on macOS
    if(APPLE)
        # Find Vulkan library to bundle
        find_package(Vulkan QUIET CONFIG)
        if(NOT Vulkan_FOUND)
            find_package(Vulkan QUIET)
        endif()

        if(Vulkan_FOUND AND Vulkan_LIBRARY)
            # Get the directory containing the vulkan library
            get_filename_component(VULKAN_LIB_DIR "${Vulkan_LIBRARY}" DIRECTORY)
            # The actual versioned library (e.g., libvulkan.1.4.309.dylib)
            get_filename_component(VULKAN_REAL "${Vulkan_LIBRARY}" REALPATH)
            get_filename_component(VULKAN_REAL_NAME "${VULKAN_REAL}" NAME)
            # The symlink name (e.g., libvulkan.1.dylib)
            set(VULKAN_SYMLINK "libvulkan.1.dylib")

            # Copy Vulkan library with symlink
            add_custom_command(TARGET package POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy "${VULKAN_REAL}" "${DIST_BIN_DIR}/${VULKAN_REAL_NAME}"
                COMMAND ${CMAKE_COMMAND} -E create_symlink "${VULKAN_REAL_NAME}" "${DIST_BIN_DIR}/${VULKAN_SYMLINK}"
                COMMENT "Bundling Vulkan library for distribution..."
            )
        endif()

        # Copy any other dylibs from build directory
        add_custom_command(TARGET package POST_BUILD
            COMMAND ${CMAKE_COMMAND}
                -DSRC_DIR="$<TARGET_FILE_DIR:bestow>"
                -DDST_DIR="${DIST_BIN_DIR}"
                -P "${CMAKE_SOURCE_DIR}/cmake/CopyDylibs.cmake"
            COMMENT "Copying dylibs to distribution..."
        )

        # Fix rpaths in the distribution executable to use @executable_path
        # This allows the executable to find libraries in the same directory
        add_custom_command(TARGET package POST_BUILD
            COMMAND install_name_tool -add_rpath "@executable_path" "${DIST_BIN_DIR}/bestow"
            COMMENT "Fixing rpaths for distribution..."
        )
    endif()

    # Create archive target
    if(WIN32)
        add_custom_target(package-zip
            DEPENDS package
            COMMENT "Creating distribution zip..."
            COMMAND ${CMAKE_COMMAND} -E tar "cfv" "${CMAKE_BINARY_DIR}/dist/bestow-${PLATFORM_NAME}.zip" --format=zip "bestow-${PLATFORM_NAME}"
            WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/dist"
        )
    else()
        # Use tar.gz for Unix systems
        add_custom_target(package-zip
            DEPENDS package
            COMMENT "Creating distribution tarball..."
            COMMAND ${CMAKE_COMMAND} -E tar "czvf" "${CMAKE_BINARY_DIR}/dist/bestow-${PLATFORM_NAME}.tar.gz" "bestow-${PLATFORM_NAME}"
            WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/dist"
        )
    endif()

    message(STATUS "Package targets available: 'package' and 'package-zip' (${PLATFORM_NAME})")
endfunction()

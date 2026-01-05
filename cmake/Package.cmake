# Package.cmake
# Creates a distributable package for Bestow

# Define the package target
function(create_package_target)
    if(NOT WIN32)
        # macOS/Linux packaging would go here
        return()
    endif()

    set(DIST_DIR "${CMAKE_BINARY_DIR}/dist/bestow-windows-x64")
    set(DIST_BIN_DIR "${DIST_DIR}/bin")
    set(DIST_LIB_DIR "${DIST_DIR}/library")
    set(DIST_TPL_DIR "${DIST_DIR}/template")
    set(DIST_EX_DIR "${DIST_DIR}/examples")

    # Create the package target
    add_custom_target(package
        DEPENDS bestow
        COMMENT "Creating distribution package..."

        # Clean and create directory structure
        COMMAND ${CMAKE_COMMAND} -E rm -rf "${DIST_DIR}"
        COMMAND ${CMAKE_COMMAND} -E make_directory "${DIST_BIN_DIR}"
        COMMAND ${CMAKE_COMMAND} -E make_directory "${DIST_LIB_DIR}"
        COMMAND ${CMAKE_COMMAND} -E make_directory "${DIST_TPL_DIR}"
        COMMAND ${CMAKE_COMMAND} -E make_directory "${DIST_EX_DIR}/snake-lua"

        # Copy executable only (not the whole directory to avoid PDBs)
        COMMAND ${CMAKE_COMMAND} -E copy "$<TARGET_FILE:bestow>" "${DIST_BIN_DIR}/"

        # Copy DLLs using a script that filters out PDBs
        COMMAND ${CMAKE_COMMAND}
            -DSRC_DIR="$<TARGET_FILE_DIR:bestow>"
            -DDST_DIR="${DIST_BIN_DIR}"
            -P "${CMAKE_SOURCE_DIR}/cmake/CopyDLLsOnly.cmake"

        # Copy asset library
        COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_SOURCE_DIR}/asset-library" "${DIST_LIB_DIR}"

        # Copy template (excluding .claude folder)
        COMMAND ${CMAKE_COMMAND}
            -DSRC_DIR="${CMAKE_SOURCE_DIR}/template"
            -DDST_DIR="${DIST_TPL_DIR}"
            -DEXCLUDE_PATTERN=".claude"
            -P "${CMAKE_SOURCE_DIR}/cmake/CopyFiltered.cmake"

        # Copy snake-lua example
        COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_SOURCE_DIR}/games/snake-lua" "${DIST_EX_DIR}/snake-lua"

        # Copy README
        COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_SOURCE_DIR}/cmake/dist-readme.txt" "${DIST_DIR}/README.txt"
    )

    # Add a zip target that depends on package
    add_custom_target(package-zip
        DEPENDS package
        COMMENT "Creating distribution zip..."
        COMMAND ${CMAKE_COMMAND} -E tar "cfv" "${CMAKE_BINARY_DIR}/dist/bestow-windows-x64.zip" --format=zip "bestow-windows-x64"
        WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/dist"
    )

    message(STATUS "Package targets available: 'package' and 'package-zip'")
endfunction()

# WindowsRuntime.cmake
# Helper functions for copying runtime DLLs on Windows

# Copy all runtime DLLs to the target's output directory
# This ensures executables can find their dependencies when run from the build tree
function(copy_runtime_dlls TARGET_NAME)
    if(NOT WIN32)
        return()
    endif()

    # Use CMake 3.21+ TARGET_RUNTIME_DLLS for automatic DLL discovery
    # This handles vcpkg dependencies that properly set IMPORTED_LOCATION
    add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_RUNTIME_DLLS:${TARGET_NAME}>
            $<TARGET_FILE_DIR:${TARGET_NAME}>
        COMMAND_EXPAND_LISTS
        COMMENT "Copying runtime DLLs for ${TARGET_NAME}"
    )

    # Additionally copy DLLs from vcpkg that may not be discovered automatically
    # Some packages (like ozz-animation) put DLLs in lib/ instead of bin/
    if(DEFINED _VCPKG_INSTALLED_DIR AND DEFINED VCPKG_TARGET_TRIPLET)
        set(VCPKG_DEBUG_BIN "${_VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/debug/bin")
        set(VCPKG_DEBUG_LIB "${_VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/debug/lib")
        set(VCPKG_RELEASE_BIN "${_VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/bin")
        set(VCPKG_RELEASE_LIB "${_VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/lib")

        # Copy DLLs from debug/bin and debug/lib for Debug builds
        # Copy DLLs from bin and lib for Release builds
        add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND}
                -DTARGET_DIR=$<TARGET_FILE_DIR:${TARGET_NAME}>
                -DVCPKG_DEBUG_BIN=${VCPKG_DEBUG_BIN}
                -DVCPKG_DEBUG_LIB=${VCPKG_DEBUG_LIB}
                -DVCPKG_RELEASE_BIN=${VCPKG_RELEASE_BIN}
                -DVCPKG_RELEASE_LIB=${VCPKG_RELEASE_LIB}
                -DBUILD_TYPE=$<CONFIG>
                -P "${CMAKE_CURRENT_FUNCTION_LIST_DIR}/CopyVcpkgDLLs.cmake"
            COMMENT "Copying vcpkg DLLs for ${TARGET_NAME}"
        )
    endif()
endfunction()

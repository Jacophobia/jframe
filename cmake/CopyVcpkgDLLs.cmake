# CopyVcpkgDLLs.cmake
# Script to copy DLLs from vcpkg installed directories
# Called as a POST_BUILD step with -P flag

# Determine source directories based on build type
if(BUILD_TYPE STREQUAL "Debug" OR BUILD_TYPE STREQUAL "RelWithDebInfo")
    set(BIN_DIR "${VCPKG_DEBUG_BIN}")
    set(LIB_DIR "${VCPKG_DEBUG_LIB}")
else()
    set(BIN_DIR "${VCPKG_RELEASE_BIN}")
    set(LIB_DIR "${VCPKG_RELEASE_LIB}")
endif()

# Copy all DLLs from bin directory
if(EXISTS "${BIN_DIR}")
    file(GLOB BIN_DLLS "${BIN_DIR}/*.dll")
    foreach(DLL ${BIN_DLLS})
        get_filename_component(DLL_NAME "${DLL}" NAME)
        set(DEST "${TARGET_DIR}/${DLL_NAME}")
        if(NOT EXISTS "${DEST}" OR "${DLL}" IS_NEWER_THAN "${DEST}")
            file(COPY "${DLL}" DESTINATION "${TARGET_DIR}")
        endif()
    endforeach()
endif()

# Copy all DLLs from lib directory (for packages like ozz-animation)
if(EXISTS "${LIB_DIR}")
    file(GLOB LIB_DLLS "${LIB_DIR}/*.dll")
    foreach(DLL ${LIB_DLLS})
        get_filename_component(DLL_NAME "${DLL}" NAME)
        set(DEST "${TARGET_DIR}/${DLL_NAME}")
        if(NOT EXISTS "${DEST}" OR "${DLL}" IS_NEWER_THAN "${DEST}")
            file(COPY "${DLL}" DESTINATION "${TARGET_DIR}")
        endif()
    endforeach()
endif()

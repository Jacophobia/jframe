# CopyFiltered.cmake
# Copies directory from SRC_DIR to DST_DIR, excluding paths matching EXCLUDE_PATTERN

file(GLOB_RECURSE ALL_FILES RELATIVE "${SRC_DIR}" "${SRC_DIR}/*")
foreach(FILE ${ALL_FILES})
    # Skip files matching exclude pattern
    if(FILE MATCHES "${EXCLUDE_PATTERN}")
        continue()
    endif()

    get_filename_component(FILE_DIR "${FILE}" DIRECTORY)
    if(FILE_DIR)
        file(MAKE_DIRECTORY "${DST_DIR}/${FILE_DIR}")
    endif()

    # Only copy files, not directories
    if(NOT IS_DIRECTORY "${SRC_DIR}/${FILE}")
        file(COPY "${SRC_DIR}/${FILE}" DESTINATION "${DST_DIR}/${FILE_DIR}")
    endif()
endforeach()

# CopySharedLibs.cmake
# Copies only shared library files (.so) from SRC_DIR to DST_DIR

file(GLOB SO_FILES "${SRC_DIR}/*.so" "${SRC_DIR}/*.so.*")
foreach(LIB ${SO_FILES})
    get_filename_component(LIB_NAME "${LIB}" NAME)
    file(COPY "${LIB}" DESTINATION "${DST_DIR}")
endforeach()

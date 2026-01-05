# CopyDylibs.cmake
# Copies only dynamic library files (.dylib) from SRC_DIR to DST_DIR

file(GLOB DYLIB_FILES "${SRC_DIR}/*.dylib")
foreach(LIB ${DYLIB_FILES})
    get_filename_component(LIB_NAME "${LIB}" NAME)
    file(COPY "${LIB}" DESTINATION "${DST_DIR}")
endforeach()

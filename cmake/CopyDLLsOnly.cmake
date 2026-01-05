# CopyDLLsOnly.cmake
# Copies only DLL files from SRC_DIR to DST_DIR (excludes PDB, EXE, etc.)

file(GLOB DLL_FILES "${SRC_DIR}/*.dll")
foreach(DLL ${DLL_FILES})
    get_filename_component(DLL_NAME "${DLL}" NAME)
    file(COPY "${DLL}" DESTINATION "${DST_DIR}")
endforeach()

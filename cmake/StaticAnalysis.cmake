# StaticAnalysis.cmake
# Sets up static analysis tools (clang-tidy)

option(JFRAME_ENABLE_CLANG_TIDY "Enable clang-tidy" OFF)

function(enable_static_analysis target_name)
    if(JFRAME_ENABLE_CLANG_TIDY)
        find_program(CLANG_TIDY_EXE NAMES "clang-tidy")
        if(CLANG_TIDY_EXE)
            set_target_properties(${target_name} PROPERTIES
                CXX_CLANG_TIDY "${CLANG_TIDY_EXE}"
            )
            message(STATUS "clang-tidy enabled for ${target_name}")
        else()
            message(WARNING "clang-tidy requested but not found")
        endif()
    endif()
endfunction()

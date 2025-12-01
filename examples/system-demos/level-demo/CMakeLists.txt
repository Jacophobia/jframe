cmake_minimum_required(VERSION 3.28)
project(level-demo CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Level demo executable
add_executable(level-demo
    src/main.cpp
)

# Enable std module support
target_use_std_module(level-demo)

target_link_libraries(level-demo
    PRIVATE
        jframe-contract
        jframe-level
        jframe-entity
        jframe-assets
        jframe-events
)

# Set output directory
set_target_properties(level-demo PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

# Copy data files to build directory
add_custom_command(TARGET level-demo POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_SOURCE_DIR}/data
        ${CMAKE_BINARY_DIR}/bin/data
    COMMENT "Copying level data files to build directory"
)

# Enable warnings
if(MSVC)
    target_compile_options(level-demo PRIVATE /W4)
else()
    target_compile_options(level-demo PRIVATE -Wall -Wextra -pedantic)
endif()

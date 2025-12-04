cmake_minimum_required(VERSION 3.28)
project(input-demo CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Input demo executable
add_executable(input-demo
    src/main.cpp
)
target_use_std_module(input-demo)

target_link_libraries(input-demo
    PRIVATE
        bestow-contract
        bestow-input
        bestow-core
)

# Set output directory
set_target_properties(input-demo PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

# Enable warnings
if(MSVC)
    target_compile_options(input-demo PRIVATE /W4)
else()
    target_compile_options(input-demo PRIVATE -Wall -Wextra -pedantic)
endif()

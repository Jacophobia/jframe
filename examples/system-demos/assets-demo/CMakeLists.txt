cmake_minimum_required(VERSION 3.28)
project(assets-demo CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Assets demo executable
add_executable(assets-demo
    src/main.cpp
)

# Enable import std; support
target_use_std_module(assets-demo)

target_link_libraries(assets-demo
    PRIVATE
        jframe-contract
        jframe-assets
        jframe-core
)

# Set output directory
set_target_properties(assets-demo PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

# Enable warnings
if(MSVC)
    target_compile_options(assets-demo PRIVATE /W4)
else()
    target_compile_options(assets-demo PRIVATE -Wall -Wextra -pedantic)
endif()

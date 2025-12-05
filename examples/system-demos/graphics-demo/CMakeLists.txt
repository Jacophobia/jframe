cmake_minimum_required(VERSION 3.28)
project(graphics-demo CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Graphics demo executable
add_executable(graphics-demo
    src/main.cpp
)

# Enable std module support
target_use_std_module(graphics-demo)

target_link_libraries(graphics-demo
    PRIVATE
        bestow-contract
        bestow-graphics
        bestow-entity
        bestow-assets
        bestow-core
)

# Set output directory
set_target_properties(graphics-demo PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

# Enable warnings
if(MSVC)
    target_compile_options(graphics-demo PRIVATE /W4)
else()
    target_compile_options(graphics-demo PRIVATE -Wall -Wextra -pedantic)
endif()

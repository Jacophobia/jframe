cmake_minimum_required(VERSION 3.28)
project(ai-demo CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# AI demo executable
add_executable(ai-demo
    src/main.cpp
)

# Enable std module support
target_use_std_module(ai-demo)

# Add module files
target_sources(ai-demo
    PUBLIC
        FILE_SET CXX_MODULES FILES
            src/AIDemo.cppm
)

target_link_libraries(ai-demo
    PRIVATE
        bestow-contract
        bestow-ai
        bestow-entity
        bestow-physics
        bestow-assets
        bestow-events
        bestow-core
)

# Set output directory
set_target_properties(ai-demo PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

# Enable warnings
if(MSVC)
    target_compile_options(ai-demo PRIVATE /W4)
else()
    target_compile_options(ai-demo PRIVATE -Wall -Wextra -pedantic)
endif()

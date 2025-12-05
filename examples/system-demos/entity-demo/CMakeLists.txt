cmake_minimum_required(VERSION 3.28)
project(entity-demo CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Entity demo executable
add_executable(entity-demo
    src/main.cpp
)

# Enable std module support
target_use_std_module(entity-demo)

# Add module files
target_sources(entity-demo
    PUBLIC
        FILE_SET CXX_MODULES FILES
            src/EntityDemo.cppm
)

target_link_libraries(entity-demo
    PRIVATE
        bestow-contract
        bestow-entity
        bestow-events
)

# Set output directory
set_target_properties(entity-demo PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

# Enable warnings
if(MSVC)
    target_compile_options(entity-demo PRIVATE /W4)
else()
    target_compile_options(entity-demo PRIVATE -Wall -Wextra -pedantic)
endif()

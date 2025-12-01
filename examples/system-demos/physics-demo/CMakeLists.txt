cmake_minimum_required(VERSION 3.28)
project(physics-demo CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Physics demo executable
add_executable(physics-demo
    src/main.cpp
)

# Enable std module support
target_use_std_module(physics-demo)

# Add module files
target_sources(physics-demo
    PUBLIC
        FILE_SET CXX_MODULES FILES
            src/PhysicsDemo.cppm
)

target_link_libraries(physics-demo
    PRIVATE
        jframe-contract
        jframe-physics
        jframe-entity
        jframe-events
)

# Set output directory
set_target_properties(physics-demo PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

# Enable warnings
if(MSVC)
    target_compile_options(physics-demo PRIVATE /W4)
else()
    target_compile_options(physics-demo PRIVATE -Wall -Wextra -pedantic)
endif()

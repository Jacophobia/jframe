# Save System Demo
# Comprehensive demonstration of ISaveSystem interface

cmake_minimum_required(VERSION 3.28)
project(save-demo CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Create executable
add_executable(save-demo
    src/main.cpp
)

# Enable std module support
target_use_std_module(save-demo)

# Link against bestow-save and bestow-contract
target_link_libraries(save-demo
    PRIVATE
        bestow-contract
        bestow-save
)

# Set output directory
set_target_properties(save-demo PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

# Enable warnings
if(MSVC)
    target_compile_options(save-demo PRIVATE /W4)
else()
    target_compile_options(save-demo PRIVATE -Wall -Wextra -pedantic)
endif()

# Installation
install(TARGETS save-demo
    RUNTIME DESTINATION bin/examples/system-demos
)

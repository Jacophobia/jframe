cmake_minimum_required(VERSION 3.28)
project(camera-demo CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Camera demo executable
add_executable(camera-demo
    src/main.cpp
)

# Enable std module support
target_use_std_module(camera-demo)

# Add module files
target_sources(camera-demo
    PUBLIC
        FILE_SET CXX_MODULES FILES
            src/CameraDemo.cppm
)

target_link_libraries(camera-demo
    PRIVATE
        jframe-contract
        jframe-camera
        jframe-entity
)

# Set output directory
set_target_properties(camera-demo PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

# Enable warnings
if(MSVC)
    target_compile_options(camera-demo PRIVATE /W4)
else()
    target_compile_options(camera-demo PRIVATE -Wall -Wextra -pedantic)
endif()

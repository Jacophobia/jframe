# Config System Demo
# Comprehensive demonstration of IConfigSystem interface

cmake_minimum_required(VERSION 3.28)
project(config-demo LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Create executable
add_executable(config-demo
    src/main.cpp
)

# Enable std module support
target_use_std_module(config-demo)

# Link against jframe-config and jframe-contract
# Note: sol2 linked explicitly for Windows - module imports require the symbols
target_link_libraries(config-demo PRIVATE
    jframe-contract
    jframe-config
    lua
    sol2::sol2
)

# Copy data files to build directory
add_custom_command(TARGET config-demo POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_SOURCE_DIR}/data
        $<TARGET_FILE_DIR:config-demo>/data
    COMMENT "Copying config data files to build directory"
)

# Installation
install(TARGETS config-demo
    RUNTIME DESTINATION bin/examples/system-demos
)

install(DIRECTORY data/
    DESTINATION bin/examples/system-demos/data
)

cmake_minimum_required(VERSION 3.28)
project(blueprints-demo CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find Lua (same as bestow-blueprints)
find_package(Lua REQUIRED)

# Blueprints demo executable
add_executable(blueprints-demo
    src/main.cpp
)

# Enable std module support
target_use_std_module(blueprints-demo)

target_link_libraries(blueprints-demo
    PRIVATE
        bestow-contract
        bestow-entity
        bestow-physics
        bestow-events
        bestow-blueprints
        ${LUA_LIBRARIES}
        sol2::sol2
)

# Set output directory
set_target_properties(blueprints-demo PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

# Copy data files to binary directory
add_custom_command(TARGET blueprints-demo POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_SOURCE_DIR}/data
        ${CMAKE_BINARY_DIR}/bin/blueprints-demo-data
    COMMENT "Copying blueprints demo data files"
)

# Enable warnings
if(MSVC)
    target_compile_options(blueprints-demo PRIVATE /W4)
else()
    target_compile_options(blueprints-demo PRIVATE -Wall -Wextra -pedantic)
endif()

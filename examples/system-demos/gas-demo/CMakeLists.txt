# GAS (Gameplay Ability System) Demo
# Comprehensive demonstration of IGASSystem interface

cmake_minimum_required(VERSION 3.28)
project(gas-demo LANGUAGES CXX)

# Create executable
add_executable(gas-demo
    src/main.cpp
)

# Enable C++23 module support and import std
target_use_std_module(gas-demo)
target_compile_features(gas-demo PRIVATE cxx_std_23)

# Link against bestow libraries
target_link_libraries(gas-demo PRIVATE
    bestow-contract
    bestow-gas
    bestow-entity
    bestow-events
)

# Enable warnings
if(MSVC)
    target_compile_options(gas-demo PRIVATE /W4)
else()
    target_compile_options(gas-demo PRIVATE -Wall -Wextra -pedantic)
endif()

# Installation
install(TARGETS gas-demo
    RUNTIME DESTINATION bin/examples/system-demos
)

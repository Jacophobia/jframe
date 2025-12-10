# Events System Demo
# Comprehensive demonstration of IEventSystem interface

cmake_minimum_required(VERSION 3.28)
project(events-demo LANGUAGES CXX)

# Create executable
add_executable(events-demo
    src/main.cpp
)

# Enable C++23 module support and import std
target_use_std_module(events-demo)
target_compile_features(events-demo PRIVATE cxx_std_23)

# Link against bestow modules
target_link_libraries(events-demo PRIVATE
    bestow-contract
    bestow-events
    bestow-core
)

# Installation
install(TARGETS events-demo
    RUNTIME DESTINATION bin/examples/system-demos
)

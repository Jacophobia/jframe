# Audio System Demo
# Comprehensive demonstration of IAudioSystem interface

cmake_minimum_required(VERSION 3.28)
project(audio-demo LANGUAGES CXX)

# Create executable
add_executable(audio-demo
    src/main.cpp
)

# Enable C++23 module support and import std
target_use_std_module(audio-demo)
target_compile_features(audio-demo PRIVATE cxx_std_23)

# Link against bestow-audio, bestow-assets, and bestow-contract
target_link_libraries(audio-demo PRIVATE
    bestow-contract
    bestow-audio
    bestow-assets
)

# Installation
install(TARGETS audio-demo
    RUNTIME DESTINATION bin/examples/system-demos
)

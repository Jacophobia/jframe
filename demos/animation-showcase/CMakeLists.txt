# demos/animation-showcase - Animation System Showcase
# Demonstrates loading FBX models and playing skeletal animations using Vulkan

# Only build if Vulkan is available
if(NOT TARGET bestow-vulkan)
    message(STATUS "Skipping animation-showcase - bestow-vulkan not available")
    return()
endif()

add_executable(animation-showcase
    src/main.cpp
)

# The demo module
target_sources(animation-showcase
    PUBLIC FILE_SET CXX_MODULES FILES
        src/animation.showcase.cppm
)

# Link against Bestow core and implementation libraries
target_link_libraries(animation-showcase
    PRIVATE
        # Core (includes Engine class and contracts)
        bestow-core

        # Implementation libraries used by this demo
        bestow-vulkan      # Vulkan 3D graphics
        bestow-input       # Input system
        bestow-events      # Event system
        bestow-assets      # Asset system (model loading, shader compilation)
        bestow-lua         # Lua runtime (config loading)
        bestow-animation   # Animation system
)

target_compile_features(animation-showcase PRIVATE cxx_std_23)

# Enable import std; support
target_use_std_module(animation-showcase)

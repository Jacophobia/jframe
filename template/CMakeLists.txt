# Bestow Game Template
# Copy this directory to start a new game project

cmake_minimum_required(VERSION 3.28)
project(MyGame LANGUAGES CXX)

# Point this to your Bestow installation
set(BESTOW_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/.." CACHE PATH "Path to Bestow engine")

# Include Bestow
add_subdirectory(${BESTOW_ROOT} bestow EXCLUDE_FROM_ALL)

# Create executable
add_executable(my-game src/main.cpp)

target_sources(my-game
    PUBLIC FILE_SET CXX_MODULES FILES
        src/game.cppm
)

# Link against Bestow contract and implementation libraries
target_link_libraries(my-game
    PRIVATE
        # Core (includes Engine class and contracts)
        bestow-core

        # Implementation libraries - choose which ones you need:
        bestow-entity      # EntitySystem (EnTT-based)
        bestow-events      # EventSystem
        bestow-config      # ConfigSystem
        bestow-assets      # AssetSystem
        bestow-input       # InputSystem (GLFW-based)
        bestow-audio       # AudioSystem (FMOD-based)
        bestow-shader      # ShaderSystem
)

# Vulkan renderer (if available)
if(TARGET bestow-vulkan)
    target_link_libraries(my-game PRIVATE bestow-vulkan)
else()
    message(WARNING "bestow-vulkan not available. Enable BESTOW_BUILD_VULKAN=ON")
endif()

target_compile_features(my-game PRIVATE cxx_std_23)
target_use_std_module(my-game)

# Copy data directory if it exists
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/data")
    add_custom_command(TARGET my-game POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_CURRENT_SOURCE_DIR}/data"
            "$<TARGET_FILE_DIR:my-game>/data"
    )
endif()

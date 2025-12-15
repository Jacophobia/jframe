# Bestow Game Template
# Copy this directory to start a new game project

cmake_minimum_required(VERSION 3.28)
project(MyGame LANGUAGES CXX)

# Point this to your Bestow installation
set(BESTOW_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/.." CACHE PATH "Path to Bestow engine")

# Include Bestow
add_subdirectory(${BESTOW_ROOT} bestow EXCLUDE_FROM_ALL)

# Check if runtime is available
if(NOT TARGET bestow-runtime)
    message(FATAL_ERROR "bestow-runtime not available. Make sure BESTOW_BUILD_VULKAN=ON")
endif()

add_executable(my-game src/main.cpp)

target_sources(my-game
    PUBLIC FILE_SET CXX_MODULES FILES
        src/game.cppm
)

target_link_libraries(my-game PRIVATE bestow-runtime)
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

# demos/ui-menu-demo - UI Menu Demo
# Tests the Vulkan UI render backend with a clickable menu

# Only build if Vulkan is available
if(NOT TARGET bestow-vulkan)
    message(STATUS "Skipping ui-menu-demo - bestow-vulkan not available")
    return()
endif()

add_executable(ui-menu-demo
    src/main.cpp
)

# The demo module
target_sources(ui-menu-demo
    PUBLIC FILE_SET CXX_MODULES FILES
        src/ui.menu.demo.cppm
)

# Link against Bestow core and implementation libraries
target_link_libraries(ui-menu-demo
    PRIVATE
        # Core (includes Engine class and contracts)
        bestow-core

        # Implementation libraries used by this demo
        bestow-vulkan      # Vulkan 3D graphics
        bestow-input       # Input system
        bestow-events      # Event system
        bestow-assets      # Asset system (shader compilation)
        bestow-config      # Config system
        bestow-ui          # UI system (RmlUi)
)

target_compile_features(ui-menu-demo PRIVATE cxx_std_23)

# Enable import std; support
target_use_std_module(ui-menu-demo)

# Copy data directory if it exists
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/data")
    add_custom_command(TARGET ui-menu-demo POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_CURRENT_SOURCE_DIR}/data"
            "$<TARGET_FILE_DIR:ui-menu-demo>/data"
    )
endif()

# Copy asset-library to output directory as "library" (for :library:/ path resolution)
if(EXISTS "${CMAKE_SOURCE_DIR}/asset-library")
    add_custom_command(TARGET ui-menu-demo POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_SOURCE_DIR}/asset-library"
            "$<TARGET_FILE_DIR:ui-menu-demo>/library"
        COMMENT "Copying asset-library to output directory"
    )
endif()

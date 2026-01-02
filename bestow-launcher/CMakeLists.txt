# bestow-launcher/CMakeLists.txt
# Single executable entry point - `bestow run <game.lua>`

add_executable(bestow)

target_sources(bestow
    PRIVATE
        src/main.cpp
        src/CommandLine.cpp
        src/GameRunner.cpp
)

target_link_libraries(bestow
    PRIVATE
        bestow-core
        bestow-contract
        bestow-entity
        bestow-input
        bestow-audio
        bestow-assets
        bestow-events
        bestow-physics
        bestow-animation
        bestow-config
        bestow-script
        bestow-luabind
        lua
        sol2::sol2
        spdlog::spdlog
)

# Link Vulkan backend if available
if(TARGET bestow-vulkan)
    target_link_libraries(bestow PRIVATE bestow-vulkan)
    target_compile_definitions(bestow PRIVATE BESTOW_HAS_VULKAN=1)
endif()

# Link 3D physics if available
if(TARGET bestow-physics3d)
    target_link_libraries(bestow PRIVATE bestow-physics3d)
    target_compile_definitions(bestow PRIVATE BESTOW_HAS_PHYSICS3D=1)
endif()

target_compile_features(bestow PUBLIC cxx_std_23)

# Enable module support
set_target_properties(bestow PROPERTIES
    CXX_SCAN_FOR_MODULES ON
)

# Enable import std; support
target_use_std_module(bestow)

# Installation rules
include(GNUInstallDirs)

# Install the bestow executable
install(TARGETS bestow
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

# Install template directory for 'bestow init' command
# This places template at: <prefix>/share/bestow/template
# Which is found by: exeDir / ".." / "share" / "bestow" / "template"
install(DIRECTORY ${CMAKE_SOURCE_DIR}/template/
    DESTINATION ${CMAKE_INSTALL_DATADIR}/bestow/template
    FILES_MATCHING
        PATTERN "*.md"
        PATTERN "*.lua"
        PATTERN "*.json"
)

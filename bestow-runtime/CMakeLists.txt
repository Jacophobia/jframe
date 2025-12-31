# bestow-runtime/CMakeLists.txt
# Lua-first game runtime executable (PROTOTYPE)
#
# This creates the 'bestow' executable that runs Lua games.
# Usage: bestow [options] <main.lua>
#
# NOTE: This is a prototype. It currently runs Lua scripts without
# full engine integration. Full integration will connect to:
# - EntitySystem for entity creation
# - InputSystem for input handling
# - AudioSystem for sound playback
# - Graphics for rendering

add_executable(bestow-runtime)

target_sources(bestow-runtime
    PRIVATE
        src/main.cpp
        src/ComponentRegistry.cpp
        src/LuaBindings.cpp
        src/LuaGameAPI.cpp
)

# Include directories for our headers
target_include_directories(bestow-runtime
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${CMAKE_CURRENT_SOURCE_DIR}/../include
)

target_link_libraries(bestow-runtime
    PRIVATE
        # Lua bindings
        lua
        sol2::sol2

        # Utilities
        spdlog::spdlog
        glm::glm

        # EnTT for entity management (header-only via contract)
        bestow-contract
)

target_compile_features(bestow-runtime PUBLIC cxx_std_23)

# Output as 'bestow' executable
set_target_properties(bestow-runtime PROPERTIES
    OUTPUT_NAME "bestow"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

# Install the runtime executable
install(TARGETS bestow-runtime
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

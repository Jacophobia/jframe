# bestow-physics3d - 3D Physics System using Jolt Physics

# Check for Jolt Physics target (from vcpkg or FetchContent)
# Dependencies.cmake handles fetching if not found via vcpkg

# Determine the correct target name (supports vcpkg and FetchContent)
if(TARGET unofficial::joltphysics::Jolt)
    set(JOLT_TARGET unofficial::joltphysics::Jolt)
    set(JOLT_FOUND TRUE)
elseif(TARGET Jolt::Jolt)
    set(JOLT_TARGET Jolt::Jolt)
    set(JOLT_FOUND TRUE)
elseif(TARGET Jolt)
    set(JOLT_TARGET Jolt)
    set(JOLT_FOUND TRUE)
elseif(TARGET joltphysics::joltphysics)
    set(JOLT_TARGET joltphysics::joltphysics)
    set(JOLT_FOUND TRUE)
else()
    set(JOLT_FOUND FALSE)
endif()

if(JOLT_FOUND)
    message(STATUS "Jolt Physics found (${JOLT_TARGET}) - building bestow-physics3d")

    add_library(bestow-physics3d)
    target_use_std_module(bestow-physics3d)

    target_sources(bestow-physics3d
        PUBLIC FILE_SET CXX_MODULES FILES
            src/bestow.physics3d.impl.cppm
    )

    target_link_libraries(bestow-physics3d
        PUBLIC
            bestow-contract
            bestow-physics-common
            ${JOLT_TARGET}
            kangaru
    )

    target_compile_features(bestow-physics3d PUBLIC cxx_std_23)
else()
    message(WARNING "Jolt Physics not found - bestow-physics3d will not be built")
    message(STATUS "Jolt should be fetched automatically via FetchContent. Check Dependencies.cmake.")
endif()

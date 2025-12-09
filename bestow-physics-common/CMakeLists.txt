# bestow-physics-common/CMakeLists.txt
# Shared physics code between 2D and 3D physics systems

add_library(bestow-physics-common)
target_use_std_module(bestow-physics-common)

target_sources(bestow-physics-common
    PUBLIC
        FILE_SET CXX_MODULES FILES
            src/bestow.physics.common.cppm
)

target_compile_features(bestow-physics-common PUBLIC cxx_std_23)

# Link GLM for vector types
target_link_libraries(bestow-physics-common
    PUBLIC
        glm::glm
)

# Set C++23 module properties
set_target_properties(bestow-physics-common PROPERTIES
    CXX_STANDARD 23
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)

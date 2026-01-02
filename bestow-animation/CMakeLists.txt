add_library(bestow-animation)
target_use_std_module(bestow-animation)

target_sources(bestow-animation
    PUBLIC FILE_SET CXX_MODULES FILES
        src/bestow.animation.impl.cppm
    PRIVATE
        src/AnimationSystem.cpp
        src/AnimationStateMachine.cpp
)

target_link_libraries(bestow-animation
    PUBLIC
        bestow-contract
        kangaru::kangaru
        glm::glm
)

# ozz-animation for SIMD-optimized runtime animation sampling/blending
find_package(ozz-animation CONFIG REQUIRED)

target_link_libraries(bestow-animation
    PRIVATE
        ozz::animation
        ozz::animation_offline
        ozz::base
)

target_compile_features(bestow-animation PUBLIC cxx_std_23)

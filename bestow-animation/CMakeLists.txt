add_library(bestow-animation)
target_use_std_module(bestow-animation)

target_sources(bestow-animation
    PUBLIC FILE_SET CXX_MODULES FILES
        src/bestow.animation.impl.cppm
    PRIVATE
        src/AnimationSystem.cpp
)

target_link_libraries(bestow-animation
    PUBLIC
        bestow-contract
        kangaru::kangaru
        glm::glm
)

# ozz-animation for runtime animation sampling/blending
if(TARGET ozz_animation)
    target_link_libraries(bestow-animation PRIVATE ozz_animation)
    target_compile_definitions(bestow-animation PRIVATE BESTOW_HAS_OZZ)
endif()

target_compile_features(bestow-animation PUBLIC cxx_std_23)

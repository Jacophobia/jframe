add_library(bestow-camera STATIC)
target_use_std_module(bestow-camera)

target_sources(bestow-camera
    PUBLIC
        FILE_SET CXX_MODULES FILES
        src/bestow.camera.impl.cppm
    PRIVATE
        src/CameraSystem.cpp
)

target_link_libraries(bestow-camera
    PUBLIC
        bestow-contract
        glm::glm
        EnTT::EnTT
)

target_compile_features(bestow-camera PUBLIC cxx_std_23)

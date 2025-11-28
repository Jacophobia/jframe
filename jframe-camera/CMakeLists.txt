add_library(jframe-camera STATIC)
target_use_std_module(jframe-camera)

target_sources(jframe-camera
    PUBLIC
        FILE_SET CXX_MODULES FILES
        src/jframe.camera.impl.cppm
    PRIVATE
        src/CameraSystem.cpp
)

target_link_libraries(jframe-camera
    PUBLIC
        jframe-contract
        glm::glm
        EnTT::EnTT
)

target_compile_features(jframe-camera PUBLIC cxx_std_23)

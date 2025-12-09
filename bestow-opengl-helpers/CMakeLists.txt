cmake_minimum_required(VERSION 3.28)

project(bestow-opengl-helpers CXX)

add_library(bestow-opengl-helpers)
target_use_std_module(bestow-opengl-helpers)

target_sources(bestow-opengl-helpers
    PUBLIC
        FILE_SET CXX_MODULES FILES
        src/bestow.opengl.helpers.cppm
    PRIVATE
        src/OpenGLHelpers.cpp
)

target_compile_features(bestow-opengl-helpers PUBLIC cxx_std_23)

# Dependencies
find_package(glad CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)

target_link_libraries(bestow-opengl-helpers
    PUBLIC
        glad::glad
        glm::glm
)

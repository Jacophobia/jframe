add_library(jframe-audio)
target_use_std_module(jframe-audio)

target_sources(jframe-audio
    PUBLIC FILE_SET CXX_MODULES FILES
        src/jframe.audio.impl.cppm
    PRIVATE
        src/FMODAudioSystem.cpp
)

target_link_libraries(jframe-audio
    PUBLIC
        jframe-contract
)

# FMOD is manually linked if available
if(TARGET fmod)
    target_link_libraries(jframe-audio PRIVATE fmod)
    target_compile_definitions(jframe-audio PRIVATE JFRAME_HAS_FMOD)
endif()

target_compile_features(jframe-audio PUBLIC cxx_std_23)

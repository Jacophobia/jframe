add_library(bestow-audio)
target_use_std_module(bestow-audio)

target_sources(bestow-audio
    PUBLIC FILE_SET CXX_MODULES FILES
        src/bestow.audio.impl.cppm
    PRIVATE
        src/FMODAudioSystem.cpp
)

target_link_libraries(bestow-audio
    PUBLIC
        bestow-contract
        kangaru::kangaru
)

# FMOD is manually linked if available
if(TARGET fmod)
    target_link_libraries(bestow-audio PRIVATE fmod)
    target_compile_definitions(bestow-audio PRIVATE BESTOW_HAS_FMOD)
endif()

target_compile_features(bestow-audio PUBLIC cxx_std_23)

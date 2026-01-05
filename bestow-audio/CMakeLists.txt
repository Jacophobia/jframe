add_library(bestow-audio)
target_use_std_module(bestow-audio)

# Core source files
set(AUDIO_SOURCES
    src/bestow.audio.impl.cppm
    src/FMODAudioSystem.cpp
)

# Check for miniaudio (always available as it's a header-only library in external/)
set(MINIAUDIO_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/external/miniaudio")
set(STB_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/external/stb")
if(EXISTS "${MINIAUDIO_INCLUDE_DIR}/miniaudio.h")
    set(BESTOW_HAS_MINIAUDIO_AVAILABLE TRUE)
    list(APPEND AUDIO_SOURCES src/MiniaudioSystem.cpp)
    message(STATUS "bestow-audio: miniaudio found at ${MINIAUDIO_INCLUDE_DIR}")
else()
    set(BESTOW_HAS_MINIAUDIO_AVAILABLE FALSE)
    message(STATUS "bestow-audio: miniaudio not found (optional fallback)")
endif()

target_sources(bestow-audio
    PUBLIC FILE_SET CXX_MODULES FILES
        src/bestow.audio.impl.cppm
    PRIVATE
        src/FMODAudioSystem.cpp
        $<$<BOOL:${BESTOW_HAS_MINIAUDIO_AVAILABLE}>:src/MiniaudioSystem.cpp>
        $<$<BOOL:${BESTOW_HAS_MINIAUDIO_AVAILABLE}>:src/stb_vorbis_wrapper.c>
)

target_link_libraries(bestow-audio
    PUBLIC
        bestow-contract
        kangaru::kangaru
)

# FMOD is manually linked if available (primary audio backend)
if(TARGET fmod)
    target_link_libraries(bestow-audio PRIVATE fmod)
    target_compile_definitions(bestow-audio PRIVATE BESTOW_HAS_FMOD)
    message(STATUS "bestow-audio: Using FMOD as primary audio backend")
elseif(BESTOW_HAS_MINIAUDIO_AVAILABLE)
    # Use miniaudio as fallback when FMOD is not available
    target_include_directories(bestow-audio PRIVATE ${MINIAUDIO_INCLUDE_DIR} ${STB_INCLUDE_DIR})
    target_compile_definitions(bestow-audio PRIVATE BESTOW_HAS_MINIAUDIO)
    message(STATUS "bestow-audio: Using miniaudio as fallback audio backend")
else()
    message(STATUS "bestow-audio: No audio backend available (stub mode)")
endif()

# Always include miniaudio headers if building MiniaudioSystem
if(BESTOW_HAS_MINIAUDIO_AVAILABLE)
    target_include_directories(bestow-audio PRIVATE ${MINIAUDIO_INCLUDE_DIR} ${STB_INCLUDE_DIR})
    # If FMOD is also available, still compile miniaudio support for explicit selection
    if(TARGET fmod)
        target_compile_definitions(bestow-audio PRIVATE BESTOW_HAS_MINIAUDIO)
    endif()
endif()

target_compile_features(bestow-audio PUBLIC cxx_std_23)

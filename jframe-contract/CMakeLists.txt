add_library(jframe-contract)

target_sources(jframe-contract
    PUBLIC FILE_SET CXX_MODULES FILES
        src/jframe.types.cppm
        src/jframe.entity.cppm
        src/jframe.graphics.cppm
        src/jframe.audio.cppm
        src/jframe.input.cppm
        src/jframe.assets.cppm
        src/jframe.save.cppm
        src/jframe.level.cppm
        src/jframe.events.cppm
        src/jframe.physics.cppm
        src/jframe.ai.cppm
        src/jframe.cppm
)

target_link_libraries(jframe-contract
    PUBLIC
        glm::glm
        EnTT::EnTT
)

target_compile_features(jframe-contract PUBLIC cxx_std_23)

# Enable import std; support
target_use_std_module(jframe-contract)

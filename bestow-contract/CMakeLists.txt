add_library(bestow-contract)

target_sources(bestow-contract
    PUBLIC FILE_SET CXX_MODULES FILES
        src/bestow.types.cppm
        src/bestow.entity.cppm
        src/bestow.graphics.cppm
        src/bestow.graphics3d.cppm
        src/bestow.audio.cppm
        src/bestow.input.cppm
        src/bestow.assets.cppm
        src/bestow.save.cppm
        src/bestow.level.cppm
        src/bestow.events.cppm
        src/bestow.physics.cppm
        src/bestow.physics3d.cppm
        src/bestow.ai.cppm
        src/bestow.camera.cppm
        src/bestow.config.cppm
        src/bestow.gas.cppm
        src/bestow.blueprints.cppm
        src/bestow.ui.cppm
        src/bestow.gamestate.cppm
        src/bestow.shader.cppm
        src/bestow.services.cppm
        src/bestow.cppm
)

target_link_libraries(bestow-contract
    PUBLIC
        glm::glm
        EnTT::EnTT
        kangaru::kangaru
)

target_compile_features(bestow-contract PUBLIC cxx_std_23)

# Enable import std; support
target_use_std_module(bestow-contract)
